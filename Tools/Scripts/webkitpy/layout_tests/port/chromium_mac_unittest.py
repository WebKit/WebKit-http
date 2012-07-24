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

from webkitpy.layout_tests.port import chromium_mac
from webkitpy.layout_tests.port import chromium_port_testcase
from webkitpy.tool.mocktool import MockOptions


class ChromiumMacPortTest(chromium_port_testcase.ChromiumPortTestCase):
    os_name = 'mac'
    os_version = 'snowleopard'
    port_name = 'chromium-mac'
    port_maker = chromium_mac.ChromiumMacPort

    def assert_name(self, port_name, os_version_string, expected):
        port = self.make_port(os_version=os_version_string, port_name=port_name)
        self.assertEquals(expected, port.name())

    def test_versions(self):
        self.assertTrue(self.make_port().name() in ('chromium-mac-snowleopard', 'chromium-mac-lion', 'chromium-mac-future'))

        self.assert_name(None, 'snowleopard', 'chromium-mac-snowleopard')
        self.assert_name('chromium-mac', 'snowleopard', 'chromium-mac-snowleopard')
        self.assert_name('chromium-mac-snowleopard', 'leopard', 'chromium-mac-snowleopard')
        self.assert_name('chromium-mac-snowleopard', 'snowleopard', 'chromium-mac-snowleopard')

        self.assert_name(None, 'lion', 'chromium-mac-lion')
        self.assert_name(None, 'future', 'chromium-mac-future')
        self.assert_name('chromium-mac', 'lion', 'chromium-mac-lion')
        self.assert_name('chromium-mac-future', 'tiger', 'chromium-mac-future')
        self.assert_name('chromium-mac-future', 'leopard', 'chromium-mac-future')
        self.assert_name('chromium-mac-future', 'snowleopard', 'chromium-mac-future')
        self.assert_name('chromium-mac-future', 'lion', 'chromium-mac-future')

        self.assertRaises(AssertionError, self.assert_name, None, 'tiger', 'should-raise-assertion-so-this-value-does-not-matter')

    def test_baseline_path(self):
        port = self.make_port(port_name='chromium-mac-snowleopard')
        self.assertEquals(port.baseline_path(), port._webkit_baseline_path('chromium-mac-snowleopard'))

        port = self.make_port(port_name='chromium-mac-lion')
        self.assertEquals(port.baseline_path(), port._webkit_baseline_path('chromium-mac'))

    def test_operating_system(self):
        self.assertEqual('mac', self.make_port().operating_system())

    def test_build_path(self):
        # Test that optional paths are used regardless of whether they exist.
        options = MockOptions(configuration='Release', build_directory='/foo')
        self.assert_build_path(options, ['/mock-checkout/Source/WebKit/chromium/out/Release'], '/foo/Release')

        # Test that optional relative paths are returned unmodified.
        options = MockOptions(configuration='Release', build_directory='foo')
        self.assert_build_path(options, ['/mock-checkout/Source/WebKit/chromium/out/Release'], 'foo/Release')

        # Test that we look in a chromium directory before the webkit directory.
        options = MockOptions(configuration='Release', build_directory=None)
        self.assert_build_path(options, ['/mock-checkout/Source/WebKit/chromium/out/Release', '/mock-checkout/out/Release'], '/mock-checkout/Source/WebKit/chromium/out/Release')

        # Test that we prefer the legacy dir over the new dir.
        options = MockOptions(configuration='Release', build_directory=None)
        self.assert_build_path(options, ['/mock-checkout/Source/WebKit/chromium/xcodebuild/Release', '/mock-checkout/Source/WebKit/chromium/out/Release'], '/mock-checkout/Source/WebKit/chromium/xcodebuild/Release')

    def test_driver_name_option(self):
        self.assertTrue(self.make_port()._path_to_driver().endswith('DumpRenderTree'))
        self.assertTrue(self.make_port(options=MockOptions(driver_name='OtherDriver'))._path_to_driver().endswith('OtherDriver'))

    def test_path_to_image_diff(self):
        self.assertEquals(self.make_port()._path_to_image_diff(), '/mock-checkout/out/Release/ImageDiff')


if __name__ == '__main__':
    port_testcase.main()
