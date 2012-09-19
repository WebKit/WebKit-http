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

import base64
import sys
import time

from webkitpy.layout_tests.port import Port, Driver, DriverOutput
from webkitpy.layout_tests.port.base import VirtualTestSuite
from webkitpy.layout_tests.models.test_configuration import TestConfiguration
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.crashlogs import CrashLogs


# This sets basic expectations for a test. Each individual expectation
# can be overridden by a keyword argument in TestList.add().
class TestInstance(object):
    def __init__(self, name):
        self.name = name
        self.base = name[(name.rfind("/") + 1):name.rfind(".")]
        self.crash = False
        self.web_process_crash = False
        self.exception = False
        self.hang = False
        self.keyboard = False
        self.error = ''
        self.timeout = False
        self.is_reftest = False

        # The values of each field are treated as raw byte strings. They
        # will be converted to unicode strings where appropriate using
        # FileSystem.read_text_file().
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
    tests.add('failures/expected/skip_text.html', actual_text='text diff')
    tests.add('failures/flaky/text.html')
    tests.add('failures/unexpected/missing_text.html', expected_text=None)
    tests.add('failures/unexpected/missing_image.html', expected_image=None)
    tests.add('failures/unexpected/missing_render_tree_dump.html', actual_text="""layer at (0,0) size 800x600
  RenderView at (0,0) size 800x600
layer at (0,0) size 800x34
  RenderBlock {HTML} at (0,0) size 800x34
    RenderBody {BODY} at (8,8) size 784x18
      RenderText {#text} at (0,0) size 133x18
        text run at (0,0) width 133: "This is an image test!"
""", expected_text=None)
    tests.add('failures/unexpected/crash.html', crash=True)
    tests.add('failures/unexpected/crash-with-stderr.html', crash=True,
              error="mock-std-error-output")
    tests.add('failures/unexpected/web-process-crash-with-stderr.html', web_process_crash=True,
              error="mock-std-error-output")
    tests.add('failures/unexpected/text-image-checksum.html',
              actual_text='text-image-checksum_fail-txt',
              actual_checksum='text-image-checksum_fail-checksum')
    tests.add('failures/unexpected/checksum-with-matching-image.html',
              actual_checksum='text-image-checksum_fail-checksum')
    tests.add('failures/unexpected/skip_pass.html')
    tests.add('failures/unexpected/timeout.html', timeout=True)
    tests.add('http/tests/passes/text.html')
    tests.add('http/tests/passes/image.html')
    tests.add('http/tests/ssl/text.html')
    tests.add('passes/args.html')
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
    tests.add('passes/skipped/skip.html')

    # Note that here the checksums don't match but the images do, so this test passes "unexpectedly".
    # See https://bugs.webkit.org/show_bug.cgi?id=69444 .
    tests.add('failures/unexpected/checksum.html', actual_checksum='checksum_fail-checksum')

    # Text output files contain "\r\n" on Windows.  This may be
    # helpfully filtered to "\r\r\n" by our Python/Cygwin tooling.
    tests.add('passes/text.html',
              expected_text='\nfoo\n\n', actual_text='\nfoo\r\n\r\r\n')

    # For reftests.
    tests.add_reftest('passes/reftest.html', 'passes/reftest-expected.html', same_image=True)
    tests.add_reftest('passes/mismatch.html', 'passes/mismatch-expected-mismatch.html', same_image=False)
    tests.add_reftest('passes/svgreftest.svg', 'passes/svgreftest-expected.svg', same_image=True)
    tests.add_reftest('passes/xhtreftest.xht', 'passes/xhtreftest-expected.html', same_image=True)
    tests.add_reftest('passes/phpreftest.php', 'passes/phpreftest-expected-mismatch.svg', same_image=False)
    tests.add_reftest('failures/expected/reftest.html', 'failures/expected/reftest-expected.html', same_image=False)
    tests.add_reftest('failures/expected/mismatch.html', 'failures/expected/mismatch-expected-mismatch.html', same_image=True)
    tests.add_reftest('failures/unexpected/reftest.html', 'failures/unexpected/reftest-expected.html', same_image=False)
    tests.add_reftest('failures/unexpected/mismatch.html', 'failures/unexpected/mismatch-expected-mismatch.html', same_image=True)
    tests.add('failures/unexpected/reftest-nopixel.html', actual_checksum=None, actual_image=None, is_reftest=True)
    tests.add('failures/unexpected/reftest-nopixel-expected.html', actual_checksum=None, actual_image=None, is_reftest=True)
    # FIXME: Add a reftest which crashes.
    tests.add('reftests/foo/test.html')
    tests.add('reftests/foo/test-ref.html')

    tests.add('reftests/foo/multiple-match-success.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/multiple-match-failure.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/multiple-mismatch-success.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/multiple-mismatch-failure.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/multiple-both-success.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/multiple-both-failure.html', actual_checksum='abc', actual_image='abc')

    tests.add('reftests/foo/matching-ref.html', actual_checksum='abc', actual_image='abc')
    tests.add('reftests/foo/mismatching-ref.html', actual_checksum='def', actual_image='def')
    tests.add('reftests/foo/second-mismatching-ref.html', actual_checksum='ghi', actual_image='ghi')

    # The following files shouldn't be treated as reftests
    tests.add_reftest('reftests/foo/unlistedtest.html', 'reftests/foo/unlistedtest-expected.html', same_image=True)
    tests.add('reftests/foo/reference/bar/common.html')
    tests.add('reftests/foo/reftest/bar/shared.html')

    tests.add('websocket/tests/passes/text.html')

    # For testing test are properly included from platform directories.
    tests.add('platform/test-mac-leopard/http/test.html')
    tests.add('platform/test-win-win7/http/test.html')

    # For --no-http tests, test that platform specific HTTP tests are properly skipped.
    tests.add('platform/test-snow-leopard/http/test.html')
    tests.add('platform/test-snow-leopard/websocket/test.html')

    # For testing if perf tests are running in a locked shard.
    tests.add('perf/foo/test.html')
    tests.add('perf/foo/test-ref.html')

    # For testing --pixel-test-directories.
    tests.add('failures/unexpected/pixeldir/image_in_pixeldir.html',
        actual_image='image_in_pixeldir-pngtEXtchecksum\x00checksum_fail',
        expected_image='image_in_pixeldir-pngtEXtchecksum\x00checksum-png')
    tests.add('failures/unexpected/image_not_in_pixeldir.html',
        actual_image='image_not_in_pixeldir-pngtEXtchecksum\x00checksum_fail',
        expected_image='image_not_in_pixeldir-pngtEXtchecksum\x00checksum-png')

    return tests


# Here we use a non-standard location for the layout tests, to ensure that
# this works. The path contains a '.' in the name because we've seen bugs
# related to this before.

LAYOUT_TEST_DIR = '/test.checkout/LayoutTests'
PERF_TEST_DIR = '/test.checkout/PerformanceTests'


# Here we synthesize an in-memory filesystem from the test list
# in order to fully control the test output and to demonstrate that
# we don't need a real filesystem to run the tests.
def add_unit_tests_to_mock_filesystem(filesystem):
    # Add the test_expectations file.
    filesystem.maybe_make_directory(LAYOUT_TEST_DIR + '/platform/test')
    if not filesystem.exists(LAYOUT_TEST_DIR + '/platform/test/TestExpectations'):
        filesystem.write_text_file(LAYOUT_TEST_DIR + '/platform/test/TestExpectations', """
WONTFIX : failures/expected/crash.html = CRASH
WONTFIX : failures/expected/image.html = IMAGE
WONTFIX : failures/expected/audio.html = FAIL
WONTFIX : failures/expected/image_checksum.html = IMAGE
WONTFIX : failures/expected/mismatch.html = IMAGE
WONTFIX : failures/expected/missing_check.html = MISSING PASS
WONTFIX : failures/expected/missing_image.html = MISSING PASS
WONTFIX : failures/expected/missing_audio.html = MISSING PASS
WONTFIX : failures/expected/missing_text.html = MISSING PASS
WONTFIX : failures/expected/newlines_leading.html = FAIL
WONTFIX : failures/expected/newlines_trailing.html = FAIL
WONTFIX : failures/expected/newlines_with_excess_CR.html = FAIL
WONTFIX : failures/expected/reftest.html = IMAGE
WONTFIX : failures/expected/text.html = FAIL
WONTFIX : failures/expected/timeout.html = TIMEOUT
WONTFIX SKIP : failures/expected/hang.html = TIMEOUT
WONTFIX SKIP : failures/expected/keyboard.html = CRASH
WONTFIX SKIP : failures/expected/exception.html = CRASH
WONTFIX SKIP : passes/skipped/skip.html = PASS
""")

    filesystem.maybe_make_directory(LAYOUT_TEST_DIR + '/reftests/foo')
    filesystem.write_text_file(LAYOUT_TEST_DIR + '/reftests/foo/reftest.list', """
== test.html test-ref.html

== multiple-match-success.html mismatching-ref.html
== multiple-match-success.html matching-ref.html
== multiple-match-failure.html mismatching-ref.html
== multiple-match-failure.html second-mismatching-ref.html
!= multiple-mismatch-success.html mismatching-ref.html
!= multiple-mismatch-success.html second-mismatching-ref.html
!= multiple-mismatch-failure.html mismatching-ref.html
!= multiple-mismatch-failure.html matching-ref.html
== multiple-both-success.html matching-ref.html
== multiple-both-success.html mismatching-ref.html
!= multiple-both-success.html second-mismatching-ref.html
== multiple-both-failure.html matching-ref.html
!= multiple-both-failure.html second-mismatching-ref.html
!= multiple-both-failure.html matching-ref.html
""")

    # FIXME: This test was only being ignored because of missing a leading '/'.
    # Fixing the typo causes several tests to assert, so disabling the test entirely.
    # Add in a file should be ignored by port.find_test_files().
    #files[LAYOUT_TEST_DIR + '/userscripts/resources/iframe.html'] = 'iframe'

    def add_file(test, suffix, contents):
        dirname = filesystem.join(LAYOUT_TEST_DIR, test.name[0:test.name.rfind('/')])
        base = test.base
        filesystem.maybe_make_directory(dirname)
        filesystem.write_binary_file(filesystem.join(dirname, base + suffix), contents)

    # Add each test and the expected output, if any.
    test_list = unit_test_list()
    for test in test_list.tests.values():
        add_file(test, test.name[test.name.rfind('.'):], '')
        if test.is_reftest:
            continue
        if test.actual_audio:
            add_file(test, '-expected.wav', test.expected_audio)
            continue
        add_file(test, '-expected.txt', test.expected_text)
        add_file(test, '-expected.png', test.expected_image)

    filesystem.write_text_file(filesystem.join(LAYOUT_TEST_DIR, 'virtual', 'passes', 'args-expected.txt'), 'args-txt --virtual-arg')
    # Clear the list of written files so that we can watch what happens during testing.
    filesystem.clear_written_files()


class TestPort(Port):
    port_name = 'test'

    """Test implementation of the Port interface."""
    ALL_BASELINE_VARIANTS = (
        'test-linux-x86_64',
        'test-mac-snowleopard', 'test-mac-leopard',
        'test-win-vista', 'test-win-win7', 'test-win-xp',
    )

    @classmethod
    def determine_full_port_name(cls, host, options, port_name):
        if port_name == 'test':
            return 'test-mac-leopard'
        return port_name

    def __init__(self, host, port_name=None, **kwargs):
        # FIXME: Consider updating all of the callers to pass in a port_name so it can be a
        # required parameter like all of the other Port objects.
        port_name = port_name or 'test-mac-leopard'
        Port.__init__(self, host, port_name, **kwargs)
        self._tests = unit_test_list()
        self._flakes = set()
        self._expectations_path = LAYOUT_TEST_DIR + '/platform/test/TestExpectations'
        self._results_directory = None

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

    def default_pixel_tests(self):
        return True

    def _path_to_driver(self):
        # This routine shouldn't normally be called, but it is called by
        # the mock_drt Driver. We return something, but make sure it's useless.
        return 'MOCK _path_to_driver'

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

    def worker_startup_delay_secs(self):
        return 0

    def check_build(self, needs_http):
        return True

    def check_sys_deps(self, needs_http):
        return True

    def default_configuration(self):
        return 'Release'

    def diff_image(self, expected_contents, actual_contents, tolerance=None):
        diffed = actual_contents != expected_contents
        if 'ref' in expected_contents:
            assert tolerance == 0
        if diffed:
            return ("< %s\n---\n> %s\n" % (expected_contents, actual_contents), 1, None)
        return (None, 0, None)

    def layout_tests_dir(self):
        return LAYOUT_TEST_DIR

    def perf_tests_dir(self):
        return PERF_TEST_DIR

    def webkit_base(self):
        return '/test.checkout'

    def skipped_layout_tests(self, test_list):
        # This allows us to test the handling Skipped files, both with a test
        # that actually passes, and a test that does fail.
        return set(['failures/expected/skip_text.html',
                    'failures/unexpected/skip_pass.html',
                    'virtual/skipped'])

    def name(self):
        return self._name

    def operating_system(self):
        return self._operating_system

    def _path_to_wdiff(self):
        return None

    def default_results_directory(self):
        return '/tmp/layout-test-results'

    def setup_test_run(self):
        pass

    def _driver_class(self):
        return TestDriver

    def start_http_server(self, additional_dirs=None, number_of_servers=None):
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

    def _path_to_lighttpd(self):
        return "/usr/sbin/lighttpd"

    def _path_to_lighttpd_modules(self):
        return "/usr/lib/lighttpd"

    def _path_to_lighttpd_php(self):
        return "/usr/bin/php-cgi"

    def _path_to_apache(self):
        return "/usr/sbin/httpd"

    def _path_to_apache_config_file(self):
        return self._filesystem.join(self.layout_tests_dir(), 'http', 'conf', 'httpd.conf')

    def path_to_test_expectations_file(self):
        return self._expectations_path

    def all_test_configurations(self):
        """Returns a sequence of the TestConfigurations the port supports."""
        # By default, we assume we want to test every graphics type in
        # every configuration on every system.
        test_configurations = []
        for version, architecture in self._all_systems():
            for build_type in self._all_build_types():
                test_configurations.append(TestConfiguration(
                    version=version,
                    architecture=architecture,
                    build_type=build_type))
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

    def configuration_specifier_macros(self):
        """To avoid surprises when introducing new macros, these are intentionally fixed in time."""
        return {'mac': ['leopard', 'snowleopard'], 'win': ['xp', 'vista', 'win7'], 'linux': ['lucid']}

    def all_baseline_variants(self):
        return self.ALL_BASELINE_VARIANTS

    def virtual_test_suites(self):
        return [
            VirtualTestSuite('virtual/passes', 'passes', ['--virtual-arg']),
            VirtualTestSuite('virtual/skipped', 'failures/expected', ['--virtual-arg2']),
        ]


class TestDriver(Driver):
    """Test/Dummy implementation of the DumpRenderTree interface."""

    def cmd_line(self, pixel_tests, per_test_args):
        pixel_tests_flag = '-p' if pixel_tests else ''
        return [self._port._path_to_driver()] + [pixel_tests_flag] + self._port.get_option('additional_drt_flag', []) + per_test_args

    def run_test(self, test_input, stop_when_done):
        start_time = time.time()
        test_name = test_input.test_name
        test_args = test_input.args or []
        test = self._port._tests[test_name]
        if test.keyboard:
            raise KeyboardInterrupt
        if test.exception:
            raise ValueError('exception from ' + test_name)
        if test.hang:
            time.sleep((float(test_input.timeout) * 4) / 1000.0)

        audio = None
        actual_text = test.actual_text

        if 'flaky' in test_name and not test_name in self._port._flakes:
            self._port._flakes.add(test_name)
            actual_text = 'flaky text failure'

        if actual_text and test_args and test_name == 'passes/args.html':
            actual_text = actual_text + ' ' + ' '.join(test_args)

        if test.actual_audio:
            audio = base64.b64decode(test.actual_audio)
        crashed_process_name = None
        crashed_pid = None
        if test.crash:
            crashed_process_name = self._port.driver_name()
            crashed_pid = 1
        elif test.web_process_crash:
            crashed_process_name = 'WebProcess'
            crashed_pid = 2

        crash_log = ''
        if crashed_process_name:
            crash_logs = CrashLogs(self._port.host)
            crash_log = crash_logs.find_newest_log(crashed_process_name, None) or ''

        if stop_when_done:
            self.stop()

        return DriverOutput(actual_text, test.actual_image, test.actual_checksum, audio,
            crash=test.crash or test.web_process_crash, crashed_process_name=crashed_process_name,
            crashed_pid=crashed_pid, crash_log=crash_log,
            test_time=time.time() - start_time, timeout=test.timeout, error=test.error)

    def start(self, pixel_tests, per_test_args):
        pass

    def stop(self):
        pass
