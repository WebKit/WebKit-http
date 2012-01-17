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

import chromium_linux
import chromium_mac
import chromium_win


def _test_expectations_overrides(port, super):
    # The chrome ports use the regular overrides plus anything in the
    # official test_expectations as well. Hopefully we don't get collisions.
    chromium_overrides = super.test_expectations_overrides(port)

    # FIXME: It used to be that AssertionError would get raised by
    # path_from_chromium_base() if we weren't in a Chromium checkout, but
    # this changed in r60427. This should probably be changed back.
    overrides_path = port.path_from_chromium_base('webkit', 'tools',
            'layout_tests', 'test_expectations_chrome.txt')
    if not port._filesystem.exists(overrides_path):
        return chromium_overrides

    chromium_overrides = chromium_overrides or ''
    return chromium_overrides + port._filesystem.read_text_file(overrides_path)


class GoogleChromeLinux32Port(chromium_linux.ChromiumLinuxPort):
    port_name = 'google-chrome-linux32'

    # FIXME: Make google-chrome-XXX work as a port name.
    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        return 'chromium-linux-x86'

    def baseline_search_path(self):
        paths = chromium_linux.ChromiumLinuxPort.baseline_search_path(self)
        paths.insert(0, self._webkit_baseline_path('google-chrome-linux32'))
        return paths

    def test_expectations_overrides(self):
        return _test_expectations_overrides(self, chromium_linux.ChromiumLinuxPort)

    def architecture(self):
        return 'x86'


class GoogleChromeLinux64Port(chromium_linux.ChromiumLinuxPort):
    port_name = 'google-chrome-linux64'

    # FIXME: Make google-chrome-XXX work as a port name.
    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        return 'chromium-linux-x86_64'

    def baseline_search_path(self):
        paths = chromium_linux.ChromiumLinuxPort.baseline_search_path(self)
        paths.insert(0, self._webkit_baseline_path('google-chrome-linux64'))
        return paths

    def test_expectations_overrides(self):
        return _test_expectations_overrides(self, chromium_linux.ChromiumLinuxPort)

    def architecture(self):
        return 'x86_64'


class GoogleChromeMacPort(chromium_mac.ChromiumMacPort):
    port_name = 'google-chrome-mac'

    # FIXME: Make google-chrome-XXX work as a port name.
    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        return 'chromium-mac-leopard'

    def baseline_search_path(self):
        paths = chromium_mac.ChromiumMacPort.baseline_search_path(self)
        paths.insert(0, self._webkit_baseline_path('google-chrome-mac'))
        return paths

    def test_expectations_overrides(self):
        return _test_expectations_overrides(self, chromium_mac.ChromiumMacPort)


class GoogleChromeWinPort(chromium_win.ChromiumWinPort):
    port_name = 'google-chrome-win'

    # FIXME: Make google-chrome-XXX work as a port name.
    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        return 'chromium-win-xp'

    def baseline_search_path(self):
        paths = chromium_win.ChromiumWinPort.baseline_search_path(self)
        paths.insert(0, self._webkit_baseline_path('google-chrome-win'))
        return paths

    def test_expectations_overrides(self):
        return _test_expectations_overrides(self, chromium_win.ChromiumWinPort)
