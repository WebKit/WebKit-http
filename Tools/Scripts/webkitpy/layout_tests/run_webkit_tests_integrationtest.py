#!/usr/bin/python
# Copyright (C) 2010 Google Inc. All rights reserved.
# Copyright (C) 2010 Gabor Rapcsanyi (rgabor@inf.u-szeged.hu), University of Szeged
# Copyright (C) 2011 Apple Inc. All rights reserved.
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

import codecs
import itertools
import json
import logging
import platform
import Queue
import re
import StringIO
import sys
import thread
import time
import threading
import unittest

from webkitpy.common.system import outputcapture, path
from webkitpy.common.system.crashlogs_unittest import make_mock_crash_report_darwin
from webkitpy.common.system.systemhost import SystemHost
from webkitpy.common.host import Host
from webkitpy.common.host_mock import MockHost

from webkitpy.layout_tests import port
from webkitpy.layout_tests import run_webkit_tests
from webkitpy.layout_tests.controllers.manager import WorkerException
from webkitpy.layout_tests.port import Port
from webkitpy.layout_tests.port.test import TestPort, TestDriver
from webkitpy.test.skip import skip_if
from webkitpy.tool.mocktool import MockOptions


def parse_args(extra_args=None, record_results=False, tests_included=False, new_results=False, print_nothing=True):
    extra_args = extra_args or []
    args = []
    if not '--platform' in extra_args:
        args.extend(['--platform', 'test'])
    if not record_results:
        args.append('--no-record-results')
    if not new_results:
        args.append('--no-new-test-results')

    if not '--child-processes' in extra_args:
        args.extend(['--child-processes', 1])
    args.extend(extra_args)
    if not tests_included:
        # We use the glob to test that globbing works.
        args.extend(['passes',
                     'http/tests',
                     'websocket/tests',
                     'failures/expected/*'])
    return run_webkit_tests.parse_args(args)


def passing_run(extra_args=None, port_obj=None, record_results=False, tests_included=False, host=None, shared_port=True):
    options, parsed_args = parse_args(extra_args, record_results, tests_included)
    if not port_obj:
        host = host or MockHost()
        port_obj = host.port_factory.get(port_name=options.platform, options=options)

    if shared_port:
        port_obj.host.port_factory.get = lambda *args, **kwargs: port_obj

    buildbot_output = StringIO.StringIO()
    regular_output = StringIO.StringIO()
    res = run_webkit_tests.run(port_obj, options, parsed_args, buildbot_output=buildbot_output, regular_output=regular_output)
    return res == 0


def logging_run(extra_args=None, port_obj=None, record_results=False, tests_included=False, host=None, new_results=False, shared_port=True):
    options, parsed_args = parse_args(extra_args=extra_args,
                                      record_results=record_results,
                                      tests_included=tests_included,
                                      print_nothing=False, new_results=new_results)
    host = host or MockHost()
    if not port_obj:
        port_obj = host.port_factory.get(port_name=options.platform, options=options)

    res, buildbot_output, regular_output = run_and_capture(port_obj, options, parsed_args, shared_port)
    return (res, buildbot_output, regular_output, host.user)


def run_and_capture(port_obj, options, parsed_args, shared_port=True):
    if shared_port:
        port_obj.host.port_factory.get = lambda *args, **kwargs: port_obj
    oc = outputcapture.OutputCapture()
    try:
        oc.capture_output()
        buildbot_output = StringIO.StringIO()
        regular_output = StringIO.StringIO()
        res = run_webkit_tests.run(port_obj, options, parsed_args,
                                   buildbot_output=buildbot_output,
                                   regular_output=regular_output)
    finally:
        oc.restore_output()
    return (res, buildbot_output, regular_output)


def get_tests_run(extra_args=None, tests_included=False, flatten_batches=False,
                  host=None, include_reference_html=False):
    extra_args = extra_args or []
    if not tests_included:
        # Not including http tests since they get run out of order (that
        # behavior has its own test, see test_get_test_file_queue)
        extra_args = ['passes', 'failures'] + extra_args
    options, parsed_args = parse_args(extra_args, tests_included=True)

    host = host or MockHost()
    test_batches = []

    class RecordingTestDriver(TestDriver):
        def __init__(self, port, worker_number):
            TestDriver.__init__(self, port, worker_number, pixel_tests=port.get_option('pixel_test'), no_timeout=False)
            self._current_test_batch = None

        def start(self):
            pass

        def stop(self):
            self._current_test_batch = None

        def run_test(self, test_input, stop_when_done):
            if self._current_test_batch is None:
                self._current_test_batch = []
                test_batches.append(self._current_test_batch)
            test_name = test_input.test_name
            # In case of reftest, one test calls the driver's run_test() twice.
            # We should not add a reference html used by reftests to tests unless include_reference_html parameter
            # is explicitly given.
            filesystem = self._port.host.filesystem
            dirname, filename = filesystem.split(test_name)
            if include_reference_html or not Port.is_reference_html_file(filesystem, dirname, filename):
                self._current_test_batch.append(test_name)
            return TestDriver.run_test(self, test_input, stop_when_done)

    class RecordingTestPort(TestPort):
        def create_driver(self, worker_number):
            return RecordingTestDriver(self, worker_number)

    recording_port = RecordingTestPort(host, options=options)
    run_and_capture(recording_port, options, parsed_args)

    if flatten_batches:
        return list(itertools.chain(*test_batches))

    return test_batches


# Update this magic number if you add an unexpected test to webkitpy.layout_tests.port.test
# FIXME: It's nice to have a routine in port/test.py that returns this number.
unexpected_tests_count = 14


class StreamTestingMixin(object):
    def assertContains(self, stream, string):
        self.assertTrue(string in stream.getvalue())

    def assertEmpty(self, stream):
        self.assertFalse(stream.getvalue())

    def assertNotEmpty(self, stream):
        self.assertTrue(stream.getvalue())


class LintTest(unittest.TestCase, StreamTestingMixin):
    def test_all_configurations(self):

        class FakePort(object):
            def __init__(self, host, name, path):
                self.host = host
                self.name = name
                self.path = path

            def path_to_test_expectations_file(self):
                return self.path

            def test_configuration(self):
                return None

            def expectations_dict(self):
                self.host.ports_parsed.append(self.name)
                return {self.path: ''}

            def skipped_layout_tests(self, tests):
                return set([])

            def all_test_configurations(self):
                return []

            def configuration_specifier_macros(self):
                return []

            def path_from_webkit_base(self):
                return ''

            def get_option(self, name, val):
                return val

        class FakeFactory(object):
            def __init__(self, host, ports):
                self.host = host
                self.ports = {}
                for port in ports:
                    self.ports[port.name] = port

            def get(self, port_name, *args, **kwargs):
                return self.ports[port_name]

            def all_port_names(self):
                return sorted(self.ports.keys())

        host = MockHost()
        host.ports_parsed = []
        host.port_factory = FakeFactory(host, (FakePort(host, 'a', 'path-to-a'),
                                               FakePort(host, 'b', 'path-to-b'),
                                               FakePort(host, 'b-win', 'path-to-b')))

        self.assertEquals(run_webkit_tests.lint(host.port_factory.ports['a'], MockOptions(platform=None)), 0)
        self.assertEquals(host.ports_parsed, ['a', 'b'])

        host.ports_parsed = []
        self.assertEquals(run_webkit_tests.lint(host.port_factory.ports['a'], MockOptions(platform='a')), 0)
        self.assertEquals(host.ports_parsed, ['a'])

    def test_lint_test_files(self):
        res, out, err, user = logging_run(['--lint-test-files'])
        self.assertEqual(res, 0)
        self.assertEmpty(out)
        self.assertContains(err, 'Lint succeeded')

    def test_lint_test_files__errors(self):
        options, parsed_args = parse_args(['--lint-test-files'])
        host = MockHost()
        port_obj = host.port_factory.get(options.platform, options=options)
        port_obj.expectations_dict = lambda: {'': '# syntax error'}
        res, out, err = run_and_capture(port_obj, options, parsed_args)

        self.assertEqual(res, -1)
        self.assertEmpty(out)
        self.assertTrue(any(['Lint failed' in msg for msg in err.buflist]))


class MainTest(unittest.TestCase, StreamTestingMixin):
    def setUp(self):
        # A real PlatformInfo object is used here instead of a
        # MockPlatformInfo because we need to actually check for
        # Windows and Mac to skip some tests.
        self._platform = SystemHost().platform

        # FIXME: Remove this when we fix test-webkitpy to work
        # properly on cygwin (bug 63846).
        self.should_test_processes = not self._platform.is_win()

    def test_accelerated_compositing(self):
        # This just tests that we recognize the command line args
        self.assertTrue(passing_run(['--accelerated-video']))
        self.assertTrue(passing_run(['--no-accelerated-video']))

    def test_accelerated_2d_canvas(self):
        # This just tests that we recognize the command line args
        self.assertTrue(passing_run(['--accelerated-2d-canvas']))
        self.assertTrue(passing_run(['--no-accelerated-2d-canvas']))

    def test_all(self):
        res, out, err, user = logging_run([], tests_included=True)
        self.assertEquals(res, unexpected_tests_count)

    def test_basic(self):
        self.assertTrue(passing_run())

    def test_batch_size(self):
        batch_tests_run = get_tests_run(['--batch-size', '2'])
        for batch in batch_tests_run:
            self.assertTrue(len(batch) <= 2, '%s had too many tests' % ', '.join(batch))

    def test_child_processes_2(self):
        if self.should_test_processes:
            _, _, regular_output, _ = logging_run(
                ['--debug-rwt-logging', '--child-processes', '2'], shared_port=False)
            self.assertTrue(any(['Running 2 ' in line for line in regular_output.buflist]))

    def test_child_processes_min(self):
        if self.should_test_processes:
            _, _, regular_output, _ = logging_run(
                ['--debug-rwt-logging', '--child-processes', '2', 'passes'],
                tests_included=True, shared_port=False)
            self.assertTrue(any(['Running 1 ' in line for line in regular_output.buflist]))

    def test_dryrun(self):
        batch_tests_run = get_tests_run(['--dry-run'])
        self.assertEqual(batch_tests_run, [])

        batch_tests_run = get_tests_run(['-n'])
        self.assertEqual(batch_tests_run, [])

    def test_exception_raised(self):
        # Exceptions raised by a worker are treated differently depending on
        # whether they are in-process or out. inline exceptions work as normal,
        # which allows us to get the full stack trace and traceback from the
        # worker. The downside to this is that it could be any error, but this
        # is actually useful in testing.
        #
        # Exceptions raised in a separate process are re-packaged into
        # WorkerExceptions, which have a string capture of the stack which can
        # be printed, but don't display properly in the unit test exception handlers.
        self.assertRaises(ValueError, logging_run,
            ['failures/expected/exception.html', '--child-processes', '1'], tests_included=True)

        if self.should_test_processes:
            self.assertRaises(WorkerException, logging_run,
                ['--child-processes', '2', '--force', 'failures/expected/exception.html', 'passes/text.html'], tests_included=True, shared_port=False)

    def test_full_results_html(self):
        # FIXME: verify html?
        res, out, err, user = logging_run(['--full-results-html'])
        self.assertEqual(res, 0)

    def test_hung_thread(self):
        res, out, err, user = logging_run(['--run-singly', '--time-out-ms=50',
                                          'failures/expected/hang.html'],
                                          tests_included=True)
        self.assertEqual(res, 0)
        self.assertNotEmpty(out)
        self.assertNotEmpty(err)

    def test_keyboard_interrupt(self):
        # Note that this also tests running a test marked as SKIP if
        # you specify it explicitly.
        self.assertRaises(KeyboardInterrupt, logging_run,
            ['failures/expected/keyboard.html', '--child-processes', '1'],
            tests_included=True)

        if self.should_test_processes:
            self.assertRaises(KeyboardInterrupt, logging_run,
                ['failures/expected/keyboard.html', 'passes/text.html', '--child-processes', '2', '--force'], tests_included=True, shared_port=False)

    def test_no_tests_found(self):
        res, out, err, user = logging_run(['resources'], tests_included=True)
        self.assertEqual(res, -1)
        self.assertEmpty(out)
        self.assertContains(err, 'No tests to run.\n')

    def test_no_tests_found_2(self):
        res, out, err, user = logging_run(['foo'], tests_included=True)
        self.assertEqual(res, -1)
        self.assertEmpty(out)
        self.assertContains(err, 'No tests to run.\n')

    def test_randomize_order(self):
        # FIXME: verify order was shuffled
        self.assertTrue(passing_run(['--randomize-order']))

    def test_gc_between_tests(self):
        self.assertTrue(passing_run(['--gc-between-tests']))

    def test_complex_text(self):
        self.assertTrue(passing_run(['--complex-text']))

    def test_threaded(self):
        self.assertTrue(passing_run(['--threaded']))

    def test_repeat_each(self):
        tests_to_run = ['passes/image.html', 'passes/text.html']
        tests_run = get_tests_run(['--repeat-each', '2'] + tests_to_run, tests_included=True, flatten_batches=True)
        self.assertEquals(tests_run, ['passes/image.html', 'passes/image.html', 'passes/text.html', 'passes/text.html'])

    def test_ignore_flag(self):
        # Note that passes/image.html is expected to be run since we specified it directly.
        tests_run = get_tests_run(['-i', 'passes', 'passes/image.html'], flatten_batches=True, tests_included=True)
        self.assertFalse('passes/text.html' in tests_run)
        self.assertTrue('passes/image.html' in tests_run)

    def test_skipped_flag(self):
        tests_run = get_tests_run(['passes'], tests_included=True, flatten_batches=True)
        self.assertFalse('passes/skipped/skip.html' in tests_run)
        num_tests_run_by_default = len(tests_run)

        # Check that nothing changes when we specify skipped=default.
        self.assertEquals(len(get_tests_run(['--skipped=default', 'passes'], tests_included=True, flatten_batches=True)),
                          num_tests_run_by_default)

        # Now check that we run one more test (the skipped one).
        tests_run = get_tests_run(['--skipped=ignore', 'passes'], tests_included=True, flatten_batches=True)
        self.assertTrue('passes/skipped/skip.html' in tests_run)
        self.assertEquals(len(tests_run), num_tests_run_by_default + 1)

        # Now check that we only run the skipped test.
        self.assertEquals(get_tests_run(['--skipped=only', 'passes'], tests_included=True, flatten_batches=True),
                          ['passes/skipped/skip.html'])

    def test_iterations(self):
        tests_to_run = ['passes/image.html', 'passes/text.html']
        tests_run = get_tests_run(['--iterations', '2'] + tests_to_run, tests_included=True, flatten_batches=True)
        self.assertEquals(tests_run, ['passes/image.html', 'passes/text.html', 'passes/image.html', 'passes/text.html'])

    def test_repeat_each_iterations_num_tests(self):
        # The total number of tests should be: number_of_tests *
        # repeat_each * iterations
        host = MockHost()
        res, out, err, _ = logging_run(['--iterations', '2',
                                        '--repeat-each', '4',
                                        '--debug-rwt-logging',
                                        'passes/text.html', 'failures/expected/text.html'],
                                       tests_included=True, host=host, record_results=True)
        self.assertContains(out, "=> Results: 8/16 tests passed (50.0%)\n")
        self.assertContains(err, "All 16 tests ran as expected.\n")

    def test_run_chunk(self):
        # Test that we actually select the right chunk
        all_tests_run = get_tests_run(flatten_batches=True)
        chunk_tests_run = get_tests_run(['--run-chunk', '1:4'], flatten_batches=True)
        self.assertEquals(all_tests_run[4:8], chunk_tests_run)

        # Test that we wrap around if the number of tests is not evenly divisible by the chunk size
        tests_to_run = ['passes/error.html', 'passes/image.html', 'passes/platform_image.html', 'passes/text.html']
        chunk_tests_run = get_tests_run(['--run-chunk', '1:3'] + tests_to_run, tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/text.html', 'passes/error.html', 'passes/image.html'], chunk_tests_run)

    def test_run_force(self):
        # This raises an exception because we run
        # failures/expected/exception.html, which is normally SKIPped.

        # See also the comments in test_exception_raised() about ValueError vs. WorkerException.
        self.assertRaises(ValueError, logging_run, ['--force'])

    def test_run_part(self):
        # Test that we actually select the right part
        tests_to_run = ['passes/error.html', 'passes/image.html', 'passes/platform_image.html', 'passes/text.html']
        tests_run = get_tests_run(['--run-part', '1:2'] + tests_to_run, tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/error.html', 'passes/image.html'], tests_run)

        # Test that we wrap around if the number of tests is not evenly divisible by the chunk size
        # (here we end up with 3 parts, each with 2 tests, and we only have 4 tests total, so the
        # last part repeats the first two tests).
        chunk_tests_run = get_tests_run(['--run-part', '3:3'] + tests_to_run, tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/error.html', 'passes/image.html'], chunk_tests_run)

    def test_run_singly(self):
        batch_tests_run = get_tests_run(['--run-singly'])
        for batch in batch_tests_run:
            self.assertEquals(len(batch), 1, '%s had too many tests' % ', '.join(batch))

    def test_skip_failing_tests(self):
        # This tests that we skip both known failing and known flaky tests. Because there are
        # no known flaky tests in the default test_expectations, we add additional expectations.
        host = MockHost()
        host.filesystem.write_text_file('/tmp/overrides.txt', 'BUGX : passes/image.html = IMAGE PASS\n')

        batches = get_tests_run(['--skip-failing-tests', '--additional-expectations', '/tmp/overrides.txt'], host=host)
        has_passes_text = False
        for batch in batches:
            self.assertFalse('failures/expected/text.html' in batch)
            self.assertFalse('passes/image.html' in batch)
            has_passes_text = has_passes_text or ('passes/text.html' in batch)
        self.assertTrue(has_passes_text)

    def test_run_singly_actually_runs_tests(self):
        res, _, _, _ = logging_run(['--run-singly', 'failures/unexpected'])
        self.assertEquals(res, 10)

    def test_single_file(self):
        # FIXME: We should consider replacing more of the get_tests_run()-style tests
        # with tests that read the tests_run* files, like this one.
        host = MockHost()
        tests_run = passing_run(['passes/text.html'], tests_included=True, host=host)
        self.assertEquals(host.filesystem.read_text_file('/tmp/layout-test-results/tests_run0.txt'),
                          'passes/text.html\n')

    def test_single_file_with_prefix(self):
        tests_run = get_tests_run(['LayoutTests/passes/text.html'], tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/text.html'], tests_run)

    def test_single_skipped_file(self):
        tests_run = get_tests_run(['failures/expected/keybaord.html'], tests_included=True, flatten_batches=True)
        self.assertEquals([], tests_run)

    def test_stderr_is_saved(self):
        host = MockHost()
        self.assertTrue(passing_run(host=host))
        self.assertEquals(host.filesystem.read_text_file('/tmp/layout-test-results/passes/error-stderr.txt'),
                          'stuff going to stderr')

    def test_test_list(self):
        host = MockHost()
        filename = '/tmp/foo.txt'
        host.filesystem.write_text_file(filename, 'passes/text.html')
        tests_run = get_tests_run(['--test-list=%s' % filename], tests_included=True, flatten_batches=True, host=host)
        self.assertEquals(['passes/text.html'], tests_run)
        host.filesystem.remove(filename)
        res, out, err, user = logging_run(['--test-list=%s' % filename],
                                          tests_included=True, host=host)
        self.assertEqual(res, -1)
        self.assertNotEmpty(err)

    def test_test_list_with_prefix(self):
        host = MockHost()
        filename = '/tmp/foo.txt'
        host.filesystem.write_text_file(filename, 'LayoutTests/passes/text.html')
        tests_run = get_tests_run(['--test-list=%s' % filename], tests_included=True, flatten_batches=True, host=host)
        self.assertEquals(['passes/text.html'], tests_run)

    def test_unexpected_failures(self):
        # Run tests including the unexpected failures.
        self._url_opened = None
        res, out, err, user = logging_run(tests_included=True)

        self.assertEqual(res, unexpected_tests_count)
        self.assertNotEmpty(out)
        self.assertNotEmpty(err)
        self.assertEqual(user.opened_urls, [path.abspath_to_uri(MockHost().platform, '/tmp/layout-test-results/results.html')])

    def test_missing_and_unexpected_results(self):
        # Test that we update expectations in place. If the expectation
        # is missing, update the expected generic location.
        host = MockHost()
        res, out, err, _ = logging_run(['--no-show-results',
            'failures/expected/missing_image.html',
            'failures/unexpected/missing_text.html',
            'failures/unexpected/text-image-checksum.html'],
            tests_included=True, host=host, record_results=True)
        file_list = host.filesystem.written_files.keys()
        file_list.remove('/tmp/layout-test-results/tests_run0.txt')
        self.assertEquals(res, 1)
        expected_token = '"unexpected":{"text-image-checksum.html":{"expected":"PASS","actual":"FAIL"},"missing_text.html":{"expected":"PASS","is_missing_text":true,"actual":"MISSING"}'
        json_string = host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json')
        self.assertTrue(json_string.find(expected_token) != -1)
        self.assertTrue(json_string.find('"num_regressions":1') != -1)
        self.assertTrue(json_string.find('"num_flaky":0') != -1)
        self.assertTrue(json_string.find('"num_missing":1') != -1)

    def test_pixel_test_directories(self):
        host = MockHost()

        """Both tests have faling checksum. We include only the first in pixel tests so only that should fail."""
        args = ['--pixel-tests', '--pixel-test-directory', 'failures/unexpected/pixeldir',
                'failures/unexpected/pixeldir/image_in_pixeldir.html',
                'failures/unexpected/image_not_in_pixeldir.html']
        res, out, err, _ = logging_run(extra_args=args, host=host, record_results=True, tests_included=True)

        self.assertEquals(res, 1)
        expected_token = '"unexpected":{"pixeldir":{"image_in_pixeldir.html":{"expected":"PASS","actual":"IMAGE"'
        json_string = host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json')
        self.assertTrue(json_string.find(expected_token) != -1)

    def test_missing_and_unexpected_results_with_custom_exit_code(self):
        # Test that we update expectations in place. If the expectation
        # is missing, update the expected generic location.
        class CustomExitCodePort(TestPort):
            def exit_code_from_summarized_results(self, unexpected_results):
                return unexpected_results['num_regressions'] + unexpected_results['num_missing']

        host = MockHost()
        options, parsed_args = run_webkit_tests.parse_args(['--pixel-tests', '--no-new-test-results'])
        test_port = CustomExitCodePort(host, options=options)
        res, out, err, _ = logging_run(['--no-show-results',
            'failures/expected/missing_image.html',
            'failures/unexpected/missing_text.html',
            'failures/unexpected/text-image-checksum.html'],
            tests_included=True, host=host, record_results=True, port_obj=test_port)
        self.assertEquals(res, 2)

    def test_crash_with_stderr(self):
        host = MockHost()
        res, buildbot_output, regular_output, user = logging_run([
                'failures/unexpected/crash-with-stderr.html',
            ],
            tests_included=True,
            record_results=True,
            host=host)
        self.assertTrue(host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json').find('{"crash-with-stderr.html":{"expected":"PASS","actual":"CRASH","has_stderr":true}}') != -1)

    def test_no_image_failure_with_image_diff(self):
        host = MockHost()
        res, buildbot_output, regular_output, user = logging_run([
                'failures/unexpected/checksum-with-matching-image.html',
            ],
            tests_included=True,
            record_results=True,
            host=host)
        self.assertTrue(host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json').find('"num_regressions":0') != -1)

    def test_crash_log(self):
        # FIXME: Need to rewrite these tests to not be mac-specific, or move them elsewhere.
        # Currently CrashLog uploading only works on Darwin.
        if not self._platform.is_mac():
            return
        mock_crash_report = make_mock_crash_report_darwin('DumpRenderTree', 12345)
        host = MockHost()
        host.filesystem.write_text_file('/Users/mock/Library/Logs/DiagnosticReports/DumpRenderTree_2011-06-13-150719_quadzen.crash', mock_crash_report)
        res, buildbot_output, regular_output, user = logging_run([
                'failures/unexpected/crash-with-stderr.html',
            ],
            tests_included=True,
            record_results=True,
            host=host)
        expected_crash_log = mock_crash_report
        self.assertEquals(host.filesystem.read_text_file('/tmp/layout-test-results/failures/unexpected/crash-with-stderr-crash-log.txt'), expected_crash_log)

    def test_web_process_crash_log(self):
        # FIXME: Need to rewrite these tests to not be mac-specific, or move them elsewhere.
        # Currently CrashLog uploading only works on Darwin.
        if not self._platform.is_mac():
            return
        mock_crash_report = make_mock_crash_report_darwin('WebProcess', 12345)
        host = MockHost()
        host.filesystem.write_text_file('/Users/mock/Library/Logs/DiagnosticReports/WebProcess_2011-06-13-150719_quadzen.crash', mock_crash_report)
        res, buildbot_output, regular_output, user = logging_run([
                'failures/unexpected/web-process-crash-with-stderr.html',
            ],
            tests_included=True,
            record_results=True,
            host=host)
        self.assertEquals(host.filesystem.read_text_file('/tmp/layout-test-results/failures/unexpected/web-process-crash-with-stderr-crash-log.txt'), mock_crash_report)

    def test_exit_after_n_failures_upload(self):
        host = MockHost()
        res, buildbot_output, regular_output, user = logging_run([
                'failures/unexpected/text-image-checksum.html',
                'passes/text.html',
                '--exit-after-n-failures', '1',
            ],
            tests_included=True,
            record_results=True,
            host=host)

        # By returning False, we know that the incremental results were generated and then deleted.
        self.assertFalse(host.filesystem.exists('/tmp/layout-test-results/incremental_results.json'))

        # This checks that we report only the number of tests that actually failed.
        self.assertEquals(res, 1)

        # This checks that passes/text.html is considered SKIPped.
        self.assertTrue('"skipped":1' in host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json'))

        # This checks that we told the user we bailed out.
        self.assertTrue('Exiting early after 1 failures. 1 tests run.\n' in regular_output.getvalue())

        # This checks that neither test ran as expected.
        # FIXME: This log message is confusing; tests that were skipped should be called out separately.
        self.assertTrue('0 tests ran as expected, 2 didn\'t:\n' in regular_output.getvalue())

    def test_exit_after_n_failures(self):
        # Unexpected failures should result in tests stopping.
        tests_run = get_tests_run([
                'failures/unexpected/text-image-checksum.html',
                'passes/text.html',
                '--exit-after-n-failures', '1',
            ],
            tests_included=True,
            flatten_batches=True)
        self.assertEquals(['failures/unexpected/text-image-checksum.html'], tests_run)

        # But we'll keep going for expected ones.
        tests_run = get_tests_run([
                'failures/expected/text.html',
                'passes/text.html',
                '--exit-after-n-failures', '1',
            ],
            tests_included=True,
            flatten_batches=True)
        self.assertEquals(['failures/expected/text.html', 'passes/text.html'], tests_run)

    def test_exit_after_n_crashes(self):
        # Unexpected crashes should result in tests stopping.
        tests_run = get_tests_run([
                'failures/unexpected/crash.html',
                'passes/text.html',
                '--exit-after-n-crashes-or-timeouts', '1',
            ],
            tests_included=True,
            flatten_batches=True)
        self.assertEquals(['failures/unexpected/crash.html'], tests_run)

        # Same with timeouts.
        tests_run = get_tests_run([
                'failures/unexpected/timeout.html',
                'passes/text.html',
                '--exit-after-n-crashes-or-timeouts', '1',
            ],
            tests_included=True,
            flatten_batches=True)
        self.assertEquals(['failures/unexpected/timeout.html'], tests_run)

        # But we'll keep going for expected ones.
        tests_run = get_tests_run([
                'failures/expected/crash.html',
                'passes/text.html',
                '--exit-after-n-crashes-or-timeouts', '1',
            ],
            tests_included=True,
            flatten_batches=True)
        self.assertEquals(['failures/expected/crash.html', 'passes/text.html'], tests_run)

    def test_results_directory_absolute(self):
        # We run a configuration that should fail, to generate output, then
        # look for what the output results url was.

        host = MockHost()
        with host.filesystem.mkdtemp() as tmpdir:
            res, out, err, user = logging_run(['--results-directory=' + str(tmpdir)],
                                              tests_included=True, host=host)
            self.assertEqual(user.opened_urls, [path.abspath_to_uri(host.platform, host.filesystem.join(tmpdir, 'results.html'))])

    def test_results_directory_default(self):
        # We run a configuration that should fail, to generate output, then
        # look for what the output results url was.

        # This is the default location.
        res, out, err, user = logging_run(tests_included=True)
        self.assertEqual(user.opened_urls, [path.abspath_to_uri(MockHost().platform, '/tmp/layout-test-results/results.html')])

    def test_results_directory_relative(self):
        # We run a configuration that should fail, to generate output, then
        # look for what the output results url was.
        host = MockHost()
        host.filesystem.maybe_make_directory('/tmp/cwd')
        host.filesystem.chdir('/tmp/cwd')
        res, out, err, user = logging_run(['--results-directory=foo'],
                                          tests_included=True, host=host)
        self.assertEqual(user.opened_urls, [path.abspath_to_uri(host.platform, '/tmp/cwd/foo/results.html')])

    def test_retrying_and_flaky_tests(self):
        host = MockHost()
        res, out, err, _ = logging_run(['--debug-rwt-logging', 'failures/flaky'], tests_included=True, host=host)
        self.assertEquals(res, 0)
        self.assertTrue('Retrying' in err.getvalue())
        self.assertTrue('Unexpected flakiness' in out.getvalue())
        self.assertTrue(host.filesystem.exists('/tmp/layout-test-results/failures/flaky/text-actual.txt'))
        self.assertTrue(host.filesystem.exists('/tmp/layout-test-results/retries/tests_run0.txt'))
        self.assertFalse(host.filesystem.exists('/tmp/layout-test-results/retries/failures/flaky/text-actual.txt'))

        # Now we test that --clobber-old-results does remove the old entries and the old retries,
        # and that we don't retry again.
        host = MockHost()
        res, out, err, _ = logging_run(['--no-retry-failures', '--clobber-old-results', 'failures/flaky'], tests_included=True, host=host)
        self.assertEquals(res, 1)
        self.assertTrue('Clobbering old results' in err.getvalue())
        self.assertTrue('flaky/text.html' in err.getvalue())
        self.assertTrue('Unexpected failures' in out.getvalue())
        self.assertFalse('Unexpected flakiness' in out.getvalue())
        self.assertTrue(host.filesystem.exists('/tmp/layout-test-results/failures/flaky/text-actual.txt'))
        self.assertFalse(host.filesystem.exists('retries'))

    def test_run_order__inline(self):
        # These next tests test that we run the tests in ascending alphabetical
        # order per directory. HTTP tests are sharded separately from other tests,
        # so we have to test both.
        tests_run = get_tests_run(['passes'], tests_included=True, flatten_batches=True)
        self.assertEquals(tests_run, sorted(tests_run))

        tests_run = get_tests_run(['http/tests/passes'], tests_included=True, flatten_batches=True)
        self.assertEquals(tests_run, sorted(tests_run))

    def test_tolerance(self):
        class ImageDiffTestPort(TestPort):
            def diff_image(self, expected_contents, actual_contents, tolerance=None):
                self.tolerance_used_for_diff_image = self._options.tolerance
                return (True, 1, None)

        def get_port_for_run(args):
            options, parsed_args = run_webkit_tests.parse_args(args)
            host = MockHost()
            test_port = ImageDiffTestPort(host, options=options)
            res = passing_run(args, port_obj=test_port, tests_included=True)
            self.assertTrue(res)
            return test_port

        base_args = ['--pixel-tests', '--no-new-test-results', 'failures/expected/*']

        # If we pass in an explicit tolerance argument, then that will be used.
        test_port = get_port_for_run(base_args + ['--tolerance', '.1'])
        self.assertEqual(0.1, test_port.tolerance_used_for_diff_image)
        test_port = get_port_for_run(base_args + ['--tolerance', '0'])
        self.assertEqual(0, test_port.tolerance_used_for_diff_image)

        # Otherwise the port's default tolerance behavior (including ignoring it)
        # should be used.
        test_port = get_port_for_run(base_args)
        self.assertEqual(None, test_port.tolerance_used_for_diff_image)

    def test_virtual(self):
        self.assertTrue(passing_run(['passes/text.html', 'passes/args.html',
                                     'virtual/passes/text.html', 'virtual/passes/args.html']))

    def test_reftest_run(self):
        tests_run = get_tests_run(['passes/reftest.html'], tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/reftest.html'], tests_run)

    def test_reftest_run_reftests_if_pixel_tests_are_disabled(self):
        tests_run = get_tests_run(['--no-pixel-tests', 'passes/reftest.html'], tests_included=True, flatten_batches=True)
        self.assertEquals(['passes/reftest.html'], tests_run)

    def test_reftest_skip_reftests_if_no_ref_tests(self):
        tests_run = get_tests_run(['--no-ref-tests', 'passes/reftest.html'], tests_included=True, flatten_batches=True)
        self.assertEquals([], tests_run)
        tests_run = get_tests_run(['--no-ref-tests', '--no-pixel-tests', 'passes/reftest.html'], tests_included=True, flatten_batches=True)
        self.assertEquals([], tests_run)

    def test_reftest_expected_html_should_be_ignored(self):
        tests_run = get_tests_run(['passes/reftest-expected.html'], tests_included=True, flatten_batches=True)
        self.assertEquals([], tests_run)

    def test_reftest_driver_should_run_expected_html(self):
        tests_run = get_tests_run(['passes/reftest.html'], tests_included=True, flatten_batches=True, include_reference_html=True)
        self.assertEquals(['passes/reftest.html', 'passes/reftest-expected.html'], tests_run)

    def test_reftest_driver_should_run_expected_mismatch_html(self):
        tests_run = get_tests_run(['passes/mismatch.html'], tests_included=True, flatten_batches=True, include_reference_html=True)
        self.assertEquals(['passes/mismatch.html', 'passes/mismatch-expected-mismatch.html'], tests_run)

    def test_reftest_should_not_use_naming_convention_if_not_listed_in_reftestlist(self):
        host = MockHost()
        res, out, err, _ = logging_run(['--no-show-results', 'reftests/foo/'], tests_included=True, host=host, record_results=True)
        json_string = host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json')
        self.assertTrue(json_string.find('"unlistedtest.html":{"expected":"PASS","is_missing_text":true,"actual":"MISSING","is_missing_image":true}') != -1)
        self.assertTrue(json_string.find('"num_regressions":4') != -1)
        self.assertTrue(json_string.find('"num_flaky":0') != -1)
        self.assertTrue(json_string.find('"num_missing":1') != -1)

    def test_additional_platform_directory(self):
        self.assertTrue(passing_run(['--additional-platform-directory', '/tmp/foo']))
        self.assertTrue(passing_run(['--additional-platform-directory', '/tmp/../foo']))
        self.assertTrue(passing_run(['--additional-platform-directory', '/tmp/foo', '--additional-platform-directory', '/tmp/bar']))

        res, buildbot_output, regular_output, user = logging_run(['--additional-platform-directory', 'foo'])
        self.assertContains(regular_output, '--additional-platform-directory=foo is ignored since it is not absolute\n')

    def test_additional_expectations(self):
        host = MockHost()
        host.filesystem.write_text_file('/tmp/overrides.txt', 'BUGX : failures/unexpected/mismatch.html = IMAGE\n')
        self.assertTrue(passing_run(['--additional-expectations', '/tmp/overrides.txt', 'failures/unexpected/mismatch.html'],
                                     tests_included=True, host=host))

    def test_no_http_and_force(self):
        # See test_run_force, using --force raises an exception.
        # FIXME: We would like to check the warnings generated.
        self.assertRaises(ValueError, logging_run, ['--force', '--no-http'])

    @staticmethod
    def has_test_of_type(tests, type):
        return [test for test in tests if type in test]

    def test_no_http_tests(self):
        batch_tests_dryrun = get_tests_run(['LayoutTests/http', 'websocket/'], flatten_batches=True)
        self.assertTrue(MainTest.has_test_of_type(batch_tests_dryrun, 'http'))
        self.assertTrue(MainTest.has_test_of_type(batch_tests_dryrun, 'websocket'))

        batch_tests_run_no_http = get_tests_run(['--no-http', 'LayoutTests/http', 'websocket/'], flatten_batches=True)
        self.assertFalse(MainTest.has_test_of_type(batch_tests_run_no_http, 'http'))
        self.assertFalse(MainTest.has_test_of_type(batch_tests_run_no_http, 'websocket'))

        batch_tests_run_http = get_tests_run(['--http', 'LayoutTests/http', 'websocket/'], flatten_batches=True)
        self.assertTrue(MainTest.has_test_of_type(batch_tests_run_http, 'http'))
        self.assertTrue(MainTest.has_test_of_type(batch_tests_run_http, 'websocket'))

    def test_platform_tests_are_found(self):
        tests_run = get_tests_run(['--platform', 'test-mac-leopard', 'http'], tests_included=True, flatten_batches=True)
        self.assertTrue('platform/test-mac-leopard/http/test.html' in tests_run)
        self.assertFalse('platform/test-win-win7/http/test.html' in tests_run)

    def test_output_diffs(self):
        # Test to ensure that we don't generate -wdiff.html or -pretty.html if wdiff and PrettyPatch
        # aren't available.
        host = MockHost()
        res, out, err, _ = logging_run(['--pixel-tests', 'failures/unexpected/text-image-checksum.html'],
                                       tests_included=True, record_results=True, host=host)
        written_files = host.filesystem.written_files
        self.assertTrue(any(path.endswith('-diff.txt') for path in written_files.keys()))
        self.assertFalse(any(path.endswith('-wdiff.html') for path in written_files.keys()))
        self.assertFalse(any(path.endswith('-pretty-diff.html') for path in written_files.keys()))

        full_results_text = host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json')
        full_results = json.loads(full_results_text.replace("ADD_RESULTS(", "").replace(");", ""))
        self.assertEquals(full_results['has_wdiff'], False)
        self.assertEquals(full_results['has_pretty_patch'], False)

    def test_unsupported_platform(self):
        oc = outputcapture.OutputCapture()
        try:
            oc.capture_output()
            res = run_webkit_tests.main(['--platform', 'foo'])
        finally:
            stdout, stderr, logs = oc.restore_output()

        self.assertEquals(res, run_webkit_tests.EXCEPTIONAL_EXIT_STATUS)
        self.assertEquals(stdout, '')
        self.assertTrue('unsupported platform' in stderr)

        # This is empty because we don't even get a chance to configure the logger before failing.
        self.assertEquals(logs, '')

    def test_verbose_in_child_processes(self):
        # When we actually run multiple processes, we may have to reconfigure logging in the
        # child process (e.g., on win32) and we need to make sure that works and we still
        # see the verbose log output. However, we can't use logging_run() because using
        # outputcapture to capture stdout and stderr latter results in a nonpicklable host.
        options, parsed_args = parse_args(['--verbose', '--fully-parallel', '--child-processes', '2', 'passes/text.html', 'passes/image.html'], tests_included=True, print_nothing=False)
        host = MockHost()
        port_obj = host.port_factory.get(port_name=options.platform, options=options)
        buildbot_output = StringIO.StringIO()
        regular_output = StringIO.StringIO()
        res = run_webkit_tests.run(port_obj, options, parsed_args, buildbot_output=buildbot_output, regular_output=regular_output)
        self.assertTrue('text.html passed' in regular_output.getvalue())
        self.assertTrue('image.html passed' in regular_output.getvalue())


class EndToEndTest(unittest.TestCase):
    def parse_full_results(self, full_results_text):
        json_to_eval = full_results_text.replace("ADD_RESULTS(", "").replace(");", "")
        compressed_results = json.loads(json_to_eval)
        return compressed_results

    def test_end_to_end(self):
        host = MockHost()
        res, out, err, user = logging_run(record_results=True, tests_included=True, host=host)

        self.assertEquals(res, unexpected_tests_count)
        results = self.parse_full_results(host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json'))

        # Check to ensure we're passing back image diff %age correctly.
        self.assertEquals(results['tests']['failures']['expected']['image.html']['image_diff_percent'], 1)

        # Check that we attempted to display the results page in a browser.
        self.assertTrue(user.opened_urls)

    def test_reftest_with_two_notrefs(self):
        # Test that we update expectations in place. If the expectation
        # is missing, update the expected generic location.
        host = MockHost()
        res, out, err, _ = logging_run(['--no-show-results', 'reftests/foo/'], tests_included=True, host=host, record_results=True)
        file_list = host.filesystem.written_files.keys()
        file_list.remove('/tmp/layout-test-results/tests_run0.txt')
        json_string = host.filesystem.read_text_file('/tmp/layout-test-results/full_results.json')
        json = self.parse_full_results(json_string)
        self.assertTrue("multiple-match-success.html" not in json["tests"]["reftests"]["foo"])
        self.assertTrue("multiple-mismatch-success.html" not in json["tests"]["reftests"]["foo"])
        self.assertTrue("multiple-both-success.html" not in json["tests"]["reftests"]["foo"])
        self.assertEqual(json["tests"]["reftests"]["foo"]["multiple-match-failure.html"],
            {"expected": "PASS", "actual": "IMAGE", "image_diff_percent": 1, 'is_reftest': True})
        self.assertEqual(json["tests"]["reftests"]["foo"]["multiple-mismatch-failure.html"],
            {"expected": "PASS", "actual": "IMAGE", "is_mismatch_reftest": True})
        self.assertEqual(json["tests"]["reftests"]["foo"]["multiple-both-failure.html"],
            {"expected": "PASS", "actual": "IMAGE", "is_mismatch_reftest": True})


class RebaselineTest(unittest.TestCase, StreamTestingMixin):
    def assertBaselines(self, file_list, file, extensions, err):
        "assert that the file_list contains the baselines."""
        for ext in extensions:
            baseline = file + "-expected" + ext
            baseline_msg = 'Writing new expected result "%s"\n' % baseline
            self.assertTrue(any(f.find(baseline) != -1 for f in file_list))
            self.assertContains(err, baseline_msg)

    # FIXME: Add tests to ensure that we're *not* writing baselines when we're not
    # supposed to be.

    def test_reset_results(self):
        # Test that we update expectations in place. If the expectation
        # is missing, update the expected generic location.
        host = MockHost()
        res, out, err, _ = logging_run(['--pixel-tests',
                        '--reset-results',
                        'passes/image.html',
                        'failures/expected/missing_image.html'],
                        tests_included=True, host=host, new_results=True)
        file_list = host.filesystem.written_files.keys()
        file_list.remove('/tmp/layout-test-results/tests_run0.txt')
        self.assertEquals(res, 0)
        self.assertEmpty(out)
        self.assertEqual(len(file_list), 4)
        self.assertBaselines(file_list, "passes/image", [".txt", ".png"], err)
        self.assertBaselines(file_list, "failures/expected/missing_image", [".txt", ".png"], err)

    def test_missing_results(self):
        # Test that we update expectations in place. If the expectation
        # is missing, update the expected generic location.
        host = MockHost()
        res, out, err, _ = logging_run(['--no-show-results',
                     'failures/unexpected/missing_text.html',
                     'failures/unexpected/missing_image.html',
                     'failures/unexpected/missing_audio.html',
                     'failures/unexpected/missing_render_tree_dump.html'],
                     tests_included=True, host=host, new_results=True)
        file_list = host.filesystem.written_files.keys()
        file_list.remove('/tmp/layout-test-results/tests_run0.txt')
        self.assertEquals(res, 0)
        self.assertNotEmpty(out)
        self.assertEqual(len(file_list), 6)
        self.assertBaselines(file_list, "failures/unexpected/missing_text", [".txt"], err)
        self.assertBaselines(file_list, "platform/test/failures/unexpected/missing_image", [".png"], err)
        self.assertBaselines(file_list, "platform/test/failures/unexpected/missing_render_tree_dump", [".txt"], err)

    def test_new_baseline(self):
        # Test that we update the platform expectations in the version-specific directories
        # for both existing and new baselines.
        host = MockHost()
        res, out, err, _ = logging_run(['--pixel-tests',
                        '--new-baseline',
                        'passes/image.html',
                        'failures/expected/missing_image.html'],
                    tests_included=True, host=host, new_results=True)
        file_list = host.filesystem.written_files.keys()
        file_list.remove('/tmp/layout-test-results/tests_run0.txt')
        self.assertEquals(res, 0)
        self.assertEmpty(out)
        self.assertEqual(len(file_list), 4)
        self.assertBaselines(file_list,
            "platform/test-mac-leopard/passes/image", [".txt", ".png"], err)
        self.assertBaselines(file_list,
            "platform/test-mac-leopard/failures/expected/missing_image", [".txt", ".png"], err)


class PortTest(unittest.TestCase):
    def assert_mock_port_works(self, port_name, args=[]):
        self.assertTrue(passing_run(args + ['--platform', 'mock-' + port_name, 'fast/harness/results.html'], tests_included=True, host=Host()))

    def disabled_test_chromium_mac_lion(self):
        self.assert_mock_port_works('chromium-mac-lion')

    def disabled_test_chromium_mac_lion_in_test_shell_mode(self):
        self.assert_mock_port_works('chromium-mac-lion', args=['--additional-drt-flag=--test-shell'])

    def disabled_test_qt_linux(self):
        self.assert_mock_port_works('qt-linux')

    def disabled_test_mac_lion(self):
        self.assert_mock_port_works('mac-lion')

if __name__ == '__main__':
    unittest.main()
