#!/usr/bin/env python
# Copyright (c) 2009, Google Inc. All rights reserved.
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

import unittest

from webkitpy.common.config.ports import *


class DeprecatedPortTest(unittest.TestCase):
    def test_mac_port(self):
        self.assertEqual(MacPort().flag(), "--port=mac")
        self.assertEqual(MacPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests"))
        self.assertEqual(MacPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit"))
        self.assertEqual(MacPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug"])
        self.assertEqual(MacPort().build_webkit_command(build_style="release"), DeprecatedPort().script_shell_command("build-webkit") + ["--release"])

    def test_gtk_port(self):
        self.assertEqual(GtkPort().flag(), "--port=gtk")
        self.assertEqual(GtkPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests") + ["--gtk"])
        self.assertEqual(GtkPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--gtk", "--update-gtk", DeprecatedPort().makeArgs()])
        self.assertEqual(GtkPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--gtk", "--update-gtk", DeprecatedPort().makeArgs()])

    def test_efl_port(self):
        self.assertEqual(EflPort().flag(), "--port=efl")
        self.assertEqual(EflPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--efl", "--update-efl", DeprecatedPort().makeArgs()])
        self.assertEqual(EflPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--efl", "--update-efl", DeprecatedPort().makeArgs()])

    def test_qt_port(self):
        self.assertEqual(QtPort().flag(), "--port=qt")
        self.assertEqual(QtPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("run-webkit-tests"))
        self.assertEqual(QtPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--qt", DeprecatedPort().makeArgs()])
        self.assertEqual(QtPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--qt", DeprecatedPort().makeArgs()])

    def test_chromium_port(self):
        self.assertEqual(ChromiumPort().flag(), "--port=chromium")
        self.assertEqual(ChromiumPort().run_webkit_tests_command(), DeprecatedPort().script_shell_command("new-run-webkit-tests") + ["--chromium", "--skip-failing-tests"])
        self.assertEqual(ChromiumPort().build_webkit_command(), DeprecatedPort().script_shell_command("build-webkit") + ["--chromium", "--update-chromium"])
        self.assertEqual(ChromiumPort().build_webkit_command(build_style="debug"), DeprecatedPort().script_shell_command("build-webkit") + ["--debug", "--chromium", "--update-chromium"])
        self.assertEqual(ChromiumPort().update_webkit_command(), DeprecatedPort().script_shell_command("update-webkit") + ["--chromium"])

    def test_chromium_android_port(self):
        self.assertEqual(ChromiumAndroidPort().build_webkit_command(), ChromiumPort().build_webkit_command() + ["--chromium-android"])
        self.assertEqual(ChromiumAndroidPort().update_webkit_command(), ChromiumPort().update_webkit_command() + ["--chromium-android"])

    def test_chromium_xvfb_port(self):
        self.assertEqual(ChromiumXVFBPort().run_webkit_tests_command(), ['xvfb-run'] + DeprecatedPort().script_shell_command('new-run-webkit-tests') + ['--chromium', '--skip-failing-tests'])


if __name__ == '__main__':
    unittest.main()
