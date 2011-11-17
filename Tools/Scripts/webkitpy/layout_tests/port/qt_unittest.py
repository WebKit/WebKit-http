# Copyright (C) 2010 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import unittest

from webkitpy.common.system.executive_mock import MockExecutive2
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.port.qt import QtPort
from webkitpy.layout_tests.port import port_testcase
from webkitpy.tool.mocktool import MockOptions
from webkitpy.common.system.executive_mock import MockExecutive
from webkitpy.common.host_mock import MockHost


class QtPortTest(port_testcase.PortTestCase):
    port_maker = QtPort

    def _assert_search_path(self, search_paths, sys_platform, use_webkit2=False, qt_version='4.7'):
        # FIXME: Port constructors should not "parse" the port name, but
        # rather be passed components (directly or via setters).  Once
        # we fix that, this method will need a re-write.
        port = QtPort(sys_platform=sys_platform,
            options=MockOptions(webkit_test_runner=use_webkit2, platform='qt'),
            host=MockHost(),
            executive=MockExecutive2(self._qt_version(qt_version)))
        absolute_search_paths = map(port._webkit_baseline_path, search_paths)
        self.assertEquals(port.baseline_search_path(), absolute_search_paths)

    def _qt_version(self, qt_version):
        if qt_version in '4.7':
            return 'QMake version 2.01a\nUsing Qt version 4.7.3 in /usr/local/Trolltech/Qt-4.7.3/lib'
        if qt_version in '4.8':
            return 'QMake version 2.01a\nUsing Qt version 4.8.0 in /usr/local/Trolltech/Qt-4.8.2/lib'
        if qt_version in '5.0':
            return 'QMake version 2.01a\nUsing Qt version 5.0.0 in /usr/local/Trolltech/Qt-5.0.0/lib'

    def test_baseline_search_path(self):
        self._assert_search_path(['qt-mac', 'qt-4.7', 'qt'], 'darwin')
        self._assert_search_path(['qt-win', 'qt-4.7', 'qt'], 'win32')
        self._assert_search_path(['qt-win', 'qt-4.7', 'qt'], 'cygwin')
        self._assert_search_path(['qt-linux', 'qt-4.7', 'qt'], 'linux2')
        self._assert_search_path(['qt-linux', 'qt-4.7', 'qt'], 'linux3')

        self._assert_search_path(['qt-mac', 'qt-4.8', 'qt'], 'darwin', qt_version='4.8')
        self._assert_search_path(['qt-win', 'qt-4.8', 'qt'], 'win32', qt_version='4.8')
        self._assert_search_path(['qt-win', 'qt-4.8', 'qt'], 'cygwin', qt_version='4.8')
        self._assert_search_path(['qt-linux', 'qt-4.8', 'qt'], 'linux2', qt_version='4.8')
        self._assert_search_path(['qt-linux', 'qt-4.8', 'qt'], 'linux3', qt_version='4.8')

        self._assert_search_path(['qt-wk2', 'qt-mac', 'qt-5.0', 'qt'], 'darwin', use_webkit2=True, qt_version='5.0')
        self._assert_search_path(['qt-wk2', 'qt-win', 'qt-5.0', 'qt'], 'cygwin', use_webkit2=True, qt_version='5.0')
        self._assert_search_path(['qt-wk2', 'qt-linux', 'qt-5.0', 'qt'], 'linux2', use_webkit2=True, qt_version='5.0')

    def test_show_results_html_file(self):
        port = self.make_port()
        port._executive = MockExecutive(should_log=True)
        expected_stderr = "MOCK run_command: ['Tools/Scripts/run-launcher', '--release', '--qt', 'file://test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_stderr=expected_stderr)

    def test_setup_environ_for_server(self):
        port = self.make_port()
        env = port.setup_environ_for_server(port.driver_name())
        self.assertEquals(env['QTWEBKIT_PLUGIN_PATH'], 'MOCK output of child process/lib/plugins')
