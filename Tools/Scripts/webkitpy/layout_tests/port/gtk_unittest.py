# Copyright (C) 2011 Google Inc. All rights reserved.
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
import sys
import os

from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.port.gtk import GtkPort
from webkitpy.layout_tests.port import port_testcase
from webkitpy.common.system.executive_mock import MockExecutive
from webkitpy.thirdparty.mock import Mock
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.tool.mocktool import MockOptions


class GtkPortTest(port_testcase.PortTestCase):
    port_name = 'gtk'
    port_maker = GtkPort

    def test_show_results_html_file(self):
        port = self.make_port()
        port._executive = MockExecutive(should_log=True)
        expected_stderr = "MOCK run_command: ['Tools/Scripts/run-launcher', '--release', '--gtk', 'file://test.html'], cwd=/mock-checkout\n"
        OutputCapture().assert_outputs(self, port.show_results_html_file, ["test.html"], expected_stderr=expected_stderr)

    def test_default_timeout_ms(self):
        self.assertEquals(self.make_port(options=MockOptions(configuration='Release')).default_timeout_ms(), 6000)
        self.assertEquals(self.make_port(options=MockOptions(configuration='Debug')).default_timeout_ms(), 12000)
        self.assertEquals(self.make_port(options=MockOptions(webkit_test_runner=True, configuration='Debug')).default_timeout_ms(), 80000)
        self.assertEquals(self.make_port(options=MockOptions(webkit_test_runner=True, configuration='Release')).default_timeout_ms(), 80000)

    def assertLinesEqual(self, a, b):
        if hasattr(self, 'assertMultiLineEqual'):
            self.assertMultiLineEqual(a, b)
        else:
            self.assertEqual(a.splitlines(), b.splitlines())

    def test_get_crash_log(self):
        core_directory = os.environ.get('WEBKIT_CORE_DUMPS_DIRECTORY', '/path/to/coredumps')
        core_pattern = os.path.join(core_directory, "core-pid_%p-_-process_%e")
        mock_empty_crash_log = """\
Crash log for DumpRenderTree (pid 28529):

Coredump core-pid_28529-_-process_DumpRenderTree not found. To enable crash logs:

- run this command as super-user: echo "%(core_pattern)s" > /proc/sys/kernel/core_pattern
- enable core dumps: ulimit -c unlimited
- set the WEBKIT_CORE_DUMPS_DIRECTORY environment variable: export WEBKIT_CORE_DUMPS_DIRECTORY=%(core_directory)s


STDERR: <empty>""" % locals()

        def _mock_gdb_output(coredump_path):
            return (mock_empty_crash_log, [])

        port = self.make_port()
        port._get_gdb_output = mock_empty_crash_log
        stderr, log = port._get_crash_log("DumpRenderTree", 28529, "", "", newer_than=None)
        self.assertEqual(stderr, "")
        self.assertLinesEqual(log, mock_empty_crash_log)

        stderr, log = port._get_crash_log("DumpRenderTree", 28529, "", "", newer_than=0.0)
        self.assertEqual(stderr, "")
        self.assertLinesEqual(log, mock_empty_crash_log)
