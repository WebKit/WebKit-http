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

import logging
import sys

import webkit

from webkitpy.layout_tests.port.webkit import WebKitPort


_log = logging.getLogger(__name__)


class QtPort(WebKitPort):
    port_name = "qt"

    def _operating_system_for_platform(self, platform):
        if platform.startswith('linux'):
            return "linux"
        elif platform in ('win32', 'cygwin'):
            return "win"
        elif platform == 'darwin':
            return "mac"
        return None

    # sys_platform exists only for unit testing.
    def __init__(self, sys_platform=None, **kwargs):
        WebKitPort.__init__(self, **kwargs)
        self._operating_system = self._operating_system_for_platform(sys_platform or sys.platform)

        # FIXME: This will allow WebKitPort.baseline_search_path and WebKitPort._skipped_file_search_paths
        # to do the right thing, but doesn't include support for qt-4.8 or qt-arm (seen in LayoutTests/platform) yet.
        name_components = [self.port_name]
        if self._operating_system:
            name_components.append(self._operating_system)
        self._name = "-".join(name_components)

    def _path_to_apache_config_file(self):
        # FIXME: This needs to detect the distribution and change config files.
        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', 'apache2-debian-httpd.conf')

    def _build_driver(self):
        # The Qt port builds DRT as part of the main build step
        return True

    def _path_to_driver(self):
        return self._build_path('bin/%s' % self.driver_name())

    def _path_to_image_diff(self):
        return self._build_path('bin/ImageDiff')

    def _path_to_webcore_library(self):
        return self._build_path('lib/libQtWebKit.so')

    def _runtime_feature_list(self):
        return None

    def setup_environ_for_server(self):
        env = WebKitPort.setup_environ_for_server(self)
        env['QTWEBKIT_PLUGIN_PATH'] = self._build_path('lib/plugins')
        return env
