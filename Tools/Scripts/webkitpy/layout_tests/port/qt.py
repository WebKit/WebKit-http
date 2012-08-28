# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#     * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#     * Neither the Google name nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

"""QtWebKit implementation of the Port interface."""

import glob
import logging
import re
import sys
import os

from webkitpy.common.memoized import memoized
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.layout_tests.port.base import Port
from webkitpy.layout_tests.port.xvfbdriver import XvfbDriver

_log = logging.getLogger(__name__)


class QtPort(Port):
    ALL_VERSIONS = ['linux', 'win', 'mac']
    port_name = "qt"

    def _wk2_port_name(self):
        return "qt-5.0-wk2"

    def _port_flag_for_scripts(self):
        return "--qt"

    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        if port_name and port_name != cls.port_name:
            return port_name
        return port_name + '-' + host.platform.os_name

    # sys_platform exists only for unit testing.
    def __init__(self, host, port_name, **kwargs):
        super(QtPort, self).__init__(host, port_name, **kwargs)

        # FIXME: This will allow Port.baseline_search_path and Port._skipped_file_search_paths
        # to do the right thing, but doesn't include support for qt-4.8 or qt-arm (seen in LayoutTests/platform) yet.
        self._operating_system = port_name.replace('qt-', '')

        # FIXME: Why is this being set at all?
        self._version = self.operating_system()

    def _generate_all_test_configurations(self):
        configurations = []
        for version in self.ALL_VERSIONS:
            for build_type in self.ALL_BUILD_TYPES:
                configurations.append(TestConfiguration(version=version, architecture='x86', build_type=build_type))
        return configurations

    def _build_driver(self):
        # The Qt port builds DRT as part of the main build step
        return True

    def _path_to_driver(self):
        return self._build_path('bin/%s' % self.driver_name())

    def _path_to_image_diff(self):
        return self._build_path('bin/ImageDiff')

    def _path_to_webcore_library(self):
        if self.operating_system() == 'mac':
            return self._build_path('lib/QtWebKit.framework/QtWebKit')
        else:
            return self._build_path('lib/libQtWebKit.so')

    def _modules_to_search_for_symbols(self):
        # We search in every library to be reliable in the case of building with CONFIG+=force_static_libs_as_shared.
        if self.operating_system() == 'mac':
            frameworks = glob.glob(os.path.join(self._build_path('lib'), '*.framework'))
            return [os.path.join(framework, os.path.splitext(os.path.basename(framework))[0]) for framework in frameworks]
        else:
            suffix = 'dll' if self.operating_system() == 'win' else 'so'
            return glob.glob(os.path.join(self._build_path('lib'), 'lib*.' + suffix))

    @memoized
    def qt_version(self):
        version = ''
        try:
            for line in self._executive.run_command(['qmake', '-v']).split('\n'):
                match = re.search('Qt\sversion\s(?P<version>\d\.\d)', line)
                if match:
                    version = match.group('version')
                    break
        except OSError:
            version = '4.8'
        return version

    def _search_paths(self):
        # Qt port uses same paths for baseline_search_path and _skipped_file_search_paths
        #
        # qt-5.0-wk1    qt-5.0-wk2
        #            \/
        #         qt-5.0    qt-4.8
        #                \/
        #    (qt-linux|qt-mac|qt-win)
        #                |
        #               qt
        search_paths = []
        version = self.qt_version()
        if '5.0' in version:
            if self.get_option('webkit_test_runner'):
                search_paths.append('qt-5.0-wk2')
            else:
                search_paths.append('qt-5.0-wk1')
        if '4.8' in version:
            search_paths.append('qt-4.8')
        elif version:
            search_paths.append('qt-5.0')
        search_paths.append(self.port_name + '-' + self.host.platform.os_name)
        search_paths.append(self.port_name)
        return search_paths

    def default_baseline_search_path(self):
        return map(self._webkit_baseline_path, self._search_paths())

    def _skipped_file_search_paths(self):
        skipped_path = self._search_paths()
        if self.get_option('webkit_test_runner') and '5.0' in self.qt_version():
            skipped_path.append('wk2')
        return skipped_path

    def expectations_files(self):
        # expectations_files() uses the directories listed in _search_paths reversed.
        # e.g. qt -> qt-linux -> qt-4.8
        return list(reversed([self._filesystem.join(self._webkit_baseline_path(p), 'TestExpectations') for p in self._search_paths()]))

    def setup_environ_for_server(self, server_name=None):
        clean_env = super(QtPort, self).setup_environ_for_server(server_name)
        clean_env['QTWEBKIT_PLUGIN_PATH'] = self._build_path('lib/plugins')
        self._copy_value_from_environ_if_set(clean_env, 'QT_DRT_WEBVIEW_MODE')
        self._copy_value_from_environ_if_set(clean_env, 'DYLD_IMAGE_SUFFIX')
        self._copy_value_from_environ_if_set(clean_env, 'QT_WEBKIT_LOG')
        self._copy_value_from_environ_if_set(clean_env, 'DISABLE_NI_WARNING')
        self._copy_value_from_environ_if_set(clean_env, 'QT_WEBKIT_PAUSE_UI_PROCESS')
        self._copy_value_from_environ_if_set(clean_env, 'QT_QPA_PLATFORM_PLUGIN_PATH')
        return clean_env

    # FIXME: We should find a way to share this implmentation with Gtk,
    # or teach run-launcher how to call run-safari and move this down to Port.
    def show_results_html_file(self, results_filename):
        run_launcher_args = []
        if self.get_option('webkit_test_runner'):
            run_launcher_args.append('-2')
        run_launcher_args.append("file://%s" % results_filename)
        self._run_script("run-launcher", run_launcher_args)

    def operating_system(self):
        return self._operating_system

    def check_sys_deps(self, needs_http):
        result = super(QtPort, self).check_sys_deps(needs_http)
        if not 'WEBKIT_TESTFONTS' in os.environ:
            _log.error('\nThe WEBKIT_TESTFONTS environment variable is not defined or not set properly.')
            _log.error('You must set it before running the tests.')
            _log.error('Use git to grab the actual fonts from http://gitorious.org/qtwebkit/testfonts')
            return False
        return result

