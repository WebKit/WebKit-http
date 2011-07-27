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

"""Dummy Port implementation used for testing."""
from __future__ import with_statement

import base64
import time

from webkitpy.common.system import filesystem_mock
from webkitpy.layout_tests.port import Port, Driver, DriverOutput
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.tool import mocktool


# This sets basic expectations for a test. Each individual expectation
# can be overridden by a keyword argument in TestList.add().
class TestInstance(object):
    def __init__(self, name):
        self.name = name
        self.base = name[(name.rfind("/") + 1):name.rfind(".html")]
        self.crash = False
        self.exception = False
        self.hang = False
        self.keyboard = False
        self.error = ''
        self.timeout = False
        self.is_reftest = False

        # The values of each field are treated as raw byte strings. They
        # will be converted to unicode strings where appropriate using
        # MockFileSystem.read_text_file().
        self.actual_text = self.base + '-txt'
        self.actual_checksum = self.base + '-checksum'

        # We add the '\x8a' for the image file to prevent the value from
        # being treated as UTF-8 (the character is invalid)
        self.actual_image = self.base + '\x8a' + '-png' + 'tEXtchecksum\x00' + self.actual_checksum

        self.expected_text = self.actual_text
        self.expected_checksum = self.actual_checksum
        self.expected_image = self.actual_image

        self.actual_audio = None
        self.expected_audio = None


# This is an in-memory list of tests, what we want them to produce, and
# what we want to claim are the expected results.
class TestList(object):
    def __init__(self):
        self.tests = {}

    def add(self, name, **kwargs):
        test = TestInstance(name)
        for key, value in kwargs.items():
            test.__dict__[key] = value
        self.tests[name] = test

    def add_reftest(self, name, reference_name, same_image):
        self.add(name, actual_checksum='xxx', actual_image='XXX', is_reftest=True)
        if same_image:
            self.add(reference_name, actual_checksum='xxx', actual_image='XXX', is_reftest=True)
        else:
            self.add(reference_name, actual_checksum='yyy', actual_image='YYY', is_reftest=True)

    def keys(self):
        return self.tests.keys()

    def __contains__(self, item):
        return item in self.tests

    def __getitem__(self, item):
        return self.tests[item]


def unit_test_list():
    tests = TestList()
    tests.add('failures/expected/checksum.html',
              actual_checksum='checksum_fail-checksum')
    tests.add('failures/expected/crash.html', crash=True)
    tests.add('failures/expected/exception.html', exception=True)
    tests.add('failures/expected/timeout.html', timeout=True)
    tests.add('failures/expected/hang.html', hang=True)
    tests.add('failures/expected/missing_text.html', expected_text=None)
    tests.add('failures/expected/image.html',
              actual_image='image_fail-pngtEXtchecksum\x00checksum_fail',
              expected_image='image-pngtEXtchecksum\x00checksum-png')
    tests.add('failures/expected/image_checksum.html',
              actual_checksum='image_checksum_fail-checksum',
              actual_image='image_checksum_fail-png')
    tests.add('failures/expected/audio.html',
              actual_audio=base64.b64encode('audio_fail-wav'), expected_audio='audio-wav',
              actual_text=None, expected_text=None,
              actual_image=None, expected_image=None,
              actual_checksum=None, expected_checksum=None)
    tests.add('failures/expected/keyboard.html', keyboard=True)
    tests.add('failures/expected/missing_check.html',
              expected_checksum=None,
              expected_image=None)
    tests.add('failures/expected/missing_image.html', expected_image=None)
    tests.add('failures/expected/missing_audio.html', expected_audio=None,
              actual_text=None, expected_text=None,
              actual_image=None, expected_image=None,
              actual_checksum=None, expected_checksum=None)
    tests.add('failures/expected/missing_text.html', expected_text=None)
    tests.add('failures/expected/newlines_leading.html',
              expected_text="\nfoo\n", actual_text="foo\n")
    tests.add('failures/expected/newlines_trailing.html',
              expected_text="foo\n\n", actual_text="foo\n")
    tests.add('failures/expected/newlines_with_excess_CR.html',
              expected_text="foo\r\r\r\n", actual_text="foo\n")
    tests.add('failures/expected/text.html', actual_text='text_fail-png')
    tests.add('failures/unexpected/missing_text.html', expected_text=None)
    tests.add('failures/unexpected/missing_image.html', expected_image=None)
    tests.add('failures/unexpected/crash.html', crash=True)
    tests.add('failures/unexpected/crash-with-stderr.html', crash=True,
              error="mock-std-error-output")
    tests.add('failures/unexpected/text-image-checksum.html',
              actual_text='text-image-checksum_fail-txt',
              actual_checksum='text-image-checksum_fail-checksum')
    tests.add('failures/unexpected/checksum-with-matching-image.html',
              actual_checksum='text-image-checksum_fail-checksum')
    tests.add('failures/unexpected/timeout.html', timeout=True)
    tests.add('http/tests/passes/text.html')
    tests.add('http/tests/passes/image.html')
    tests.add('http/tests/ssl/text.html')
    tests.add('passes/error.html', error='stuff going to stderr')
    tests.add('passes/image.html')
    tests.add('passes/audio.html',
              actual_audio=base64.b64encode('audio-wav'), expected_audio='audio-wav',
              actual_text=None, expected_text=None,
              actual_image=None, expected_image=None,
              actual_checksum=None, expected_checksum=None)
    tests.add('passes/platform_image.html')
    tests.add('passes/checksum_in_image.html',
              expected_checksum=None,
              expected_image='tEXtchecksum\x00checksum_in_image-checksum')

    # Text output files contain "\r\n" on Windows.  This may be
    # helpfully filtered to "\r\r\n" by our Python/Cygwin tooling.
    tests.add('passes/text.html',
              expected_text='\nfoo\n\n', actual_text='\nfoo\r\n\r\r\n')

    # For reftests.
    tests.add_reftest('passes/reftest.html', 'passes/reftest-expected.html', same_image=True)
    tests.add_reftest('passes/mismatch.html', 'passes/mismatch-expected-mismatch.html', same_image=False)
    tests.add_reftest('failures/expected/reftest.html', 'failures/expected/reftest-expected.html', same_image=False)
    tests.add_reftest('failures/expected/mismatch.html', 'failures/expected/mismatch-expected-mismatch.html', same_image=True)
    tests.add_reftest('failures/unexpected/reftest.html', 'failures/unexpected/reftest-expected.html', same_image=False)
    tests.add_reftest('failures/unexpected/mismatch.html', 'failures/unexpected/mismatch-expected-mismatch.html', same_image=True)
    # FIXME: Add a reftest which crashes.

    tests.add('websocket/tests/passes/text.html')

    # For --no-http tests, test that platform specific HTTP tests are properly skipped.
    tests.add('platform/test-snow-leopard/http/test.html')
    tests.add('platform/test-snow-leopard/websocket/test.html')

    return tests


# Here we use a non-standard location for the layout tests, to ensure that
# this works. The path contains a '.' in the name because we've seen bugs
# related to this before.

LAYOUT_TEST_DIR = '/test.checkout/LayoutTests'


# Here we synthesize an in-memory filesystem from the test list
# in order to fully control the test output and to demonstrate that
# we don't need a real filesystem to run the tests.

def unit_test_filesystem(files=None):
    """Return the FileSystem object used by the unit tests."""
    test_list = unit_test_list()
    files = files or {}

    def add_file(files, test, suffix, contents):
        dirname = test.name[0:test.name.rfind('/')]
        base = test.base
        path = LAYOUT_TEST_DIR + '/' + dirname + '/' + base + suffix
        files[path] = contents

    # Add each test and the expected output, if any.
    for test in test_list.tests.values():
        add_file(files, test, '.html', '')
        if test.is_reftest:
            continue
        if test.actual_audio:
            add_file(files, test, '-expected.wav', test.expected_audio)
            continue

        add_file(files, test, '-expected.txt', test.expected_text)
        add_file(files, test, '-expected.png', test.expected_image)


    # Add the test_expectations file.
    files[LAYOUT_TEST_DIR + '/platform/test/test_expectations.txt'] = """
WONTFIX : failures/expected/checksum.html = IMAGE
WONTFIX : failures/expected/crash.html = CRASH
WONTFIX : failures/expected/image.html = IMAGE
WONTFIX : failures/expected/audio.html = AUDIO
WONTFIX : failures/expected/image_checksum.html = IMAGE
WONTFIX : failures/expected/mismatch.html = IMAGE
WONTFIX : failures/expected/missing_check.html = MISSING PASS
WONTFIX : failures/expected/missing_image.html = MISSING PASS
WONTFIX : failures/expected/missing_audio.html = MISSING PASS
WONTFIX : failures/expected/missing_text.html = MISSING PASS
WONTFIX : failures/expected/newlines_leading.html = TEXT
WONTFIX : failures/expected/newlines_trailing.html = TEXT
WONTFIX : failures/expected/newlines_with_excess_CR.html = TEXT
WONTFIX : failures/expected/reftest.html = IMAGE
WONTFIX : failures/expected/text.html = TEXT
WONTFIX : failures/expected/timeout.html = TIMEOUT
WONTFIX SKIP : failures/expected/hang.html = TIMEOUT
WONTFIX SKIP : failures/expected/keyboard.html = CRASH
WONTFIX SKIP : failures/expected/exception.html = CRASH
"""

    # Add in a file should be ignored by test_files.find().
    files[LAYOUT_TEST_DIR + 'userscripts/resources/iframe.html'] = 'iframe'

    fs = filesystem_mock.MockFileSystem(files, dirs=set(['/mock-checkout']))  # Make sure at least the checkout_root exists as a directory.
    fs._tests = test_list
    return fs


class TestPort(Port):
    """Test implementation of the Port interface."""
    ALL_BASELINE_VARIANTS = (
        'test-mac-snowleopard', 'test-mac-leopard',
        'test-win-win7', 'test-win-vista', 'test-win-xp',
        'test-linux-x86_64',
    )

    def __init__(self, port_name=None, user=None, filesystem=None, **kwargs):
        if not port_name or port_name == 'test':
            port_name = 'test-mac-leopard'
        user = user or mocktool.MockUser()
        filesystem = filesystem or unit_test_filesystem()
        Port.__init__(self, port_name=port_name, filesystem=filesystem, user=user, **kwargs)
        self._results_directory = None

        assert filesystem._tests
        self._tests = filesystem._tests

        self._operating_system = 'mac'
        if port_name.startswith('test-win'):
            self._operating_system = 'win'
        elif port_name.startswith('test-linux'):
            self._operating_system = 'linux'

        version_map = {
            'test-win-xp': 'xp',
            'test-win-win7': 'win7',
            'test-win-vista': 'vista',
            'test-mac-leopard': 'leopard',
            'test-mac-snowleopard': 'snowleopard',
            'test-linux-x86_64': 'lucid',
        }
        self._version = version_map[port_name]

        self._expectations_path = LAYOUT_TEST_DIR + '/platform/test/test_expectations.txt'

    def _path_to_driver(self):
        # This routine shouldn't normally be called, but it is called by
        # the mock_drt Driver. We return something, but make sure it's useless.
        return 'junk'

    def baseline_search_path(self):
        search_paths = {
            'test-mac-snowleopard': ['test-mac-snowleopard'],
            'test-mac-leopard': ['test-mac-leopard', 'test-mac-snowleopard'],
            'test-win-win7': ['test-win-win7'],
            'test-win-vista': ['test-win-vista', 'test-win-win7'],
            'test-win-xp': ['test-win-xp', 'test-win-vista', 'test-win-win7'],
            'test-linux-x86_64': ['test-linux', 'test-win-win7'],
        }
        return [self._webkit_baseline_path(d) for d in search_paths[self.name()]]

    def default_child_processes(self):
        return 1

    def default_worker_model(self):
        return 'inline'

    def check_build(self, needs_http):
        return True

    def check_sys_deps(self, needs_http):
        return True

    def default_configuration(self):
        return 'Release'

    def diff_image(self, expected_contents, actual_contents):
        diffed = actual_contents != expected_contents
        if diffed:
            return "< %s\n---\n> %s\n" % (expected_contents, actual_contents)
        return None

    def layout_tests_dir(self):
        return LAYOUT_TEST_DIR

    def name(self):
        return self._name

    def _path_to_wdiff(self):
        return None

    def default_results_directory(self):
        return '/tmp/layout-test-results'

    def setup_test_run(self):
        pass

    def create_driver(self, worker_number):
        return TestDriver(self, worker_number)

    def start_http_server(self):
        pass

    def start_websocket_server(self):
        pass

    def acquire_http_lock(self):
        pass

    def stop_http_server(self):
        pass

    def stop_websocket_server(self):
        pass

    def release_http_lock(self):
        pass

    def path_to_test_expectations_file(self):
        return self._expectations_path

    def all_test_configurations(self):
        """Returns a sequence of the TestConfigurations the port supports."""
        # By default, we assume we want to test every graphics type in
        # every configuration on every system.
        test_configurations = []
        for version, architecture in self._all_systems():
            for build_type in self._all_build_types():
                for graphics_type in self._all_graphics_types():
                    test_configurations.append(TestConfiguration(
                        version=version,
                        architecture=architecture,
                        build_type=build_type,
                        graphics_type=graphics_type))
        return test_configurations

    def _all_systems(self):
        return (('leopard', 'x86'),
                ('snowleopard', 'x86'),
                ('xp', 'x86'),
                ('vista', 'x86'),
                ('win7', 'x86'),
                ('lucid', 'x86'),
                ('lucid', 'x86_64'))

    def _all_build_types(self):
        return ('debug', 'release')

    def _all_graphics_types(self):
        return ('cpu', 'gpu')

    def configuration_specifier_macros(self):
        """To avoid surprises when introducing new macros, these are intentionally fixed in time."""
        return {'mac': ['leopard', 'snowleopard'], 'win': ['xp', 'vista', 'win7'], 'linux': ['lucid']}

    def all_baseline_variants(self):
        return self.ALL_BASELINE_VARIANTS

    # FIXME: These next two routines are copied from base.py with
    # the calls to path.abspath_to_uri() removed. We shouldn't have
    # to do this.
    def test_to_uri(self, test_name):
        """Convert a test file (which is an absolute path) to a URI."""
        LAYOUTTEST_HTTP_DIR = "http/tests/"
        LAYOUTTEST_WEBSOCKET_DIR = "http/tests/websocket/tests/"

        port = None
        use_ssl = False

        relative_path = test_name
        if (relative_path.startswith(LAYOUTTEST_WEBSOCKET_DIR)
            or relative_path.startswith(LAYOUTTEST_HTTP_DIR)):
            relative_path = relative_path[len(LAYOUTTEST_HTTP_DIR):]
            port = 8000

        # Make http/tests/local run as local files. This is to mimic the
        # logic in run-webkit-tests.
        #
        # TODO(dpranke): remove the media reference and the SSL reference?
        if (port and not relative_path.startswith("local/") and
            not relative_path.startswith("media/")):
            if relative_path.startswith("ssl/"):
                port += 443
                protocol = "https"
            else:
                protocol = "http"
            return "%s://127.0.0.1:%u/%s" % (protocol, port, relative_path)

        return "file://" + self.abspath_for_test(test_name)

    def abspath_for_test(self, test_name):
        return self.layout_tests_dir() + self.TEST_PATH_SEPARATOR + test_name

    def uri_to_test_name(self, uri):
        """Return the base layout test name for a given URI.

        This returns the test name for a given URI, e.g., if you passed in
        "file:///src/LayoutTests/fast/html/keygen.html" it would return
        "fast/html/keygen.html".

        """
        test = uri
        if uri.startswith("file:///"):
            prefix = "file://" + self.layout_tests_dir() + "/"
            return test[len(prefix):]

        if uri.startswith("http://127.0.0.1:8880/"):
            # websocket tests
            return test.replace('http://127.0.0.1:8880/', '')

        if uri.startswith("http://"):
            # regular HTTP test
            return test.replace('http://127.0.0.1:8000/', 'http/tests/')

        if uri.startswith("https://"):
            return test.replace('https://127.0.0.1:8443/', 'http/tests/')

        raise NotImplementedError('unknown url type: %s' % uri)


class TestDriver(Driver):
    """Test/Dummy implementation of the DumpRenderTree interface."""

    def cmd_line(self):
        return [self._port._path_to_driver()] + self._port.get_option('additional_drt_flag', [])

    def poll(self):
        return True

    def run_test(self, test_input):
        start_time = time.time()
        test_name = test_input.test_name
        test = self._port._tests[test_name]
        if test.keyboard:
            raise KeyboardInterrupt
        if test.exception:
            raise ValueError('exception from ' + test_name)
        if test.hang:
            time.sleep((float(test_input.timeout) * 4) / 1000.0)

        audio = None
        if test.actual_audio:
            audio = base64.b64decode(test.actual_audio)
        return DriverOutput(test.actual_text, test.actual_image,
            test.actual_checksum, audio, crash=test.crash,
            test_time=time.time() - start_time, timeout=test.timeout, error=test.error)

    def start(self):
        pass

    def stop(self):
        pass
