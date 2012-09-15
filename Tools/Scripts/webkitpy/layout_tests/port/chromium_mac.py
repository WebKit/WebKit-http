#!/usr/bin/env python
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
#     * Neither the name of Google Inc. nor the names of its
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

"""Chromium Mac implementation of the Port interface."""

import logging
import signal

from webkitpy.layout_tests.port import chromium


_log = logging.getLogger(__name__)


class ChromiumMacPort(chromium.ChromiumPort):
    SUPPORTED_OS_VERSIONS = ('snowleopard', 'lion', 'mountainlion', 'future')
    port_name = 'chromium-mac'

    FALLBACK_PATHS = {
        'snowleopard': [
            'chromium-mac-snowleopard',
            'chromium-mac',
            'chromium',
            'mac',
        ],
        'lion': [
            'chromium-mac',
            'chromium',
            'mac',
        ],
        'mountainlion': [  # FIXME: we don't treat ML different from Lion yet.
            'chromium-mac',
            'chromium',
            'mac',
        ],
        'future': [
            'chromium-mac',
            'chromium',
            'mac',
        ],
    }

    DEFAULT_BUILD_DIRECTORIES = ('xcodebuild', 'out')

    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        if port_name.endswith('-mac'):
            return port_name + '-' + host.platform.os_version
        return port_name

    def __init__(self, host, port_name, **kwargs):
        chromium.ChromiumPort.__init__(self, host, port_name, **kwargs)
        self._version = port_name[port_name.index('chromium-mac-') + len('chromium-mac-'):]
        assert self._version in self.SUPPORTED_OS_VERSIONS

    def _modules_to_search_for_symbols(self):
        return [self._build_path('ffmpegsumo.so')]

    def check_build(self, needs_http):
        result = chromium.ChromiumPort.check_build(self, needs_http)
        if not result:
            _log.error('For complete Mac build requirements, please see:')
            _log.error('')
            _log.error('    http://code.google.com/p/chromium/wiki/MacBuildInstructions')

        return result

    def operating_system(self):
        return 'mac'

    #
    # PROTECTED METHODS
    #

    def _lighttpd_path(self, *comps):
        return self.path_from_chromium_base('third_party', 'lighttpd', 'mac', *comps)

    def _wdiff_missing_message(self):
        return 'wdiff is not installed; please install from MacPorts or elsewhere'

    def _path_to_apache(self):
        return '/usr/sbin/httpd'

    def _path_to_apache_config_file(self):
        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', 'apache2-httpd.conf')

    def _path_to_lighttpd(self):
        return self._lighttpd_path('bin', 'lighttpd')

    def _path_to_lighttpd_modules(self):
        return self._lighttpd_path('lib')

    def _path_to_lighttpd_php(self):
        return self._lighttpd_path('bin', 'php-cgi')

    def _path_to_driver(self, configuration=None):
        # FIXME: make |configuration| happy with case-sensitive file systems.
        return self._build_path_with_configuration(configuration, self.driver_name() + '.app', 'Contents', 'MacOS', self.driver_name())

    def _path_to_helper(self):
        binary_name = 'LayoutTestHelper'
        return self._build_path(binary_name)

    def _path_to_wdiff(self):
        return 'wdiff'
