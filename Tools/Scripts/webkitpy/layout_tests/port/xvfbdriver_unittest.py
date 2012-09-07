# Copyright (C) 2012 Zan Dobersek <zandobersek@gmail.com>
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

from webkitpy.common.system.deprecated_logging import log
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.common.system.systemhost_mock import MockSystemHost
from webkitpy.layout_tests.port import Port
from webkitpy.layout_tests.port.config_mock import MockConfig
from webkitpy.layout_tests.port.server_process_mock import MockServerProcess
from webkitpy.layout_tests.port.xvfbdriver import XvfbDriver


class XvfbDriverTest(unittest.TestCase):
    def make_driver(self, worker_number=0, xorg_running=False):
        port = Port(host=MockSystemHost(log_executive=True), config=MockConfig())
        port._server_process_constructor = MockServerProcess
        if xorg_running:
            port._executive._running_pids['Xorg'] = 108

        driver = XvfbDriver(port, worker_number=worker_number, pixel_tests=True)
        return driver

    def cleanup_driver(self, driver):
        # Setting _xvfb_process member to None is necessary as the Driver object is stopped on deletion,
        # killing the Xvfb process if present. Thus, this method should only be called from tests that do not
        # intend to test the behavior of XvfbDriver.stop.
        driver._xvfb_process = None

    def assertDriverStartSuccessful(self, driver, expected_stderr, expected_display, pixel_tests=False):
        OutputCapture().assert_outputs(self, driver.start, [pixel_tests, []], expected_stderr=expected_stderr)
        self.assertTrue(driver._server_process.started)
        self.assertEqual(driver._server_process.env["DISPLAY"], expected_display)

    def test_start_no_pixel_tests(self):
        driver = self.make_driver()
        expected_stderr = "MOCK running_pids: []\nMOCK popen: ['Xvfb', ':0', '-screen', '0', '800x600x24', '-nolisten', 'tcp']\n"
        self.assertDriverStartSuccessful(driver, expected_stderr=expected_stderr, expected_display=":0")
        self.cleanup_driver(driver)

    def test_start_pixel_tests(self):
        driver = self.make_driver()
        expected_stderr = "MOCK running_pids: []\nMOCK popen: ['Xvfb', ':1', '-screen', '0', '800x600x24', '-nolisten', 'tcp']\n"
        self.assertDriverStartSuccessful(driver, expected_stderr=expected_stderr, expected_display=":1", pixel_tests=True)
        self.cleanup_driver(driver)

    def test_start_arbitrary_worker_number(self):
        driver = self.make_driver(worker_number=17)
        expected_stderr = "MOCK running_pids: []\nMOCK popen: ['Xvfb', ':35', '-screen', '0', '800x600x24', '-nolisten', 'tcp']\n"
        self.assertDriverStartSuccessful(driver, expected_stderr=expected_stderr, expected_display=":35", pixel_tests=True)
        self.cleanup_driver(driver)

    def test_start_existing_xorg_process(self):
        driver = self.make_driver(xorg_running=True)
        expected_stderr = "MOCK running_pids: [108]\nMOCK popen: ['Xvfb', ':1', '-screen', '0', '800x600x24', '-nolisten', 'tcp']\n"
        self.assertDriverStartSuccessful(driver, expected_stderr=expected_stderr, expected_display=":1")
        self.cleanup_driver(driver)

    def test_stop(self):
        filesystem = MockFileSystem(files={'/tmp/.X42-lock': '1234\n'})
        port = Port(host=MockSystemHost(log_executive=True, filesystem=filesystem), config=MockConfig())
        port._executive.kill_process = lambda x: log("MOCK kill_process pid: " + str(x))
        driver = XvfbDriver(port, worker_number=0, pixel_tests=True)

        class FakeXvfbProcess(object):
            pid = 1234

        driver._xvfb_process = FakeXvfbProcess()
        driver._lock_file = '/tmp/.X42-lock'

        expected_stderr = "MOCK kill_process pid: 1234\n"
        OutputCapture().assert_outputs(self, driver.stop, [], expected_stderr=expected_stderr)

        self.assertEqual(driver._xvfb_process, None)
        self.assertFalse(port._filesystem.exists(driver._lock_file))
