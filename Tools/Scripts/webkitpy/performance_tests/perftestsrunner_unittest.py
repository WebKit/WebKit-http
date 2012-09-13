#!/usr/bin/python
# Copyright (C) 2012 Google Inc. All rights reserved.
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

"""Unit tests for run_perf_tests."""

import StringIO
import json
import unittest

from webkitpy.common.host_mock import MockHost
from webkitpy.common.system.filesystem_mock import MockFileSystem
from webkitpy.common.system.outputcapture import OutputCapture
from webkitpy.layout_tests.port.driver import DriverInput, DriverOutput
from webkitpy.layout_tests.port.test import TestPort
from webkitpy.layout_tests.views import printing
from webkitpy.performance_tests.perftest import ChromiumStylePerfTest
from webkitpy.performance_tests.perftest import PerfTest
from webkitpy.performance_tests.perftestsrunner import PerfTestsRunner


class MainTest(unittest.TestCase):
    def assertWritten(self, stream, contents):
        self.assertEquals(stream.buflist, contents)

    class TestDriver:
        def run_test(self, driver_input, stop_when_done):
            text = ''
            timeout = False
            crash = False
            if driver_input.test_name.endswith('pass.html'):
                text = 'RESULT group_name: test_name= 42 ms'
            elif driver_input.test_name.endswith('timeout.html'):
                timeout = True
            elif driver_input.test_name.endswith('failed.html'):
                text = None
            elif driver_input.test_name.endswith('tonguey.html'):
                text = 'we are not expecting an output from perf tests but RESULT blablabla'
            elif driver_input.test_name.endswith('crash.html'):
                crash = True
            elif driver_input.test_name.endswith('event-target-wrapper.html'):
                text = """Running 20 times
Ignoring warm-up run (1502)
1504
1505
1510
1504
1507
1509
1510
1487
1488
1472
1472
1488
1473
1472
1475
1487
1486
1486
1475
1471

Time:
avg 1489.05 ms
median 1487 ms
stdev 14.46 ms
min 1471 ms
max 1510 ms
"""
            elif driver_input.test_name.endswith('some-parser.html'):
                text = """Running 20 times
Ignoring warm-up run (1115)

Time:
avg 1100 ms
median 1101 ms
stdev 11 ms
min 1080 ms
max 1120 ms
"""
            elif driver_input.test_name.endswith('memory-test.html'):
                text = """Running 20 times
Ignoring warm-up run (1115)

Time:
avg 1100 ms
median 1101 ms
stdev 11 ms
min 1080 ms
max 1120 ms

JS Heap:
avg 832000 bytes
median 829000 bytes
stdev 15000 bytes
min 811000 bytes
max 848000 bytes

Malloc:
avg 532000 bytes
median 529000 bytes
stdev 13000 bytes
min 511000 bytes
max 548000 bytes
"""
            return DriverOutput(text, '', '', '', crash=crash, timeout=timeout)

        def start(self):
            """do nothing"""

        def stop(self):
            """do nothing"""

    def create_runner(self, args=[], driver_class=TestDriver):
        options, parsed_args = PerfTestsRunner._parse_args(args)
        test_port = TestPort(host=MockHost(), options=options)
        test_port.create_driver = lambda worker_number=None, no_timeout=False: driver_class()

        runner = PerfTestsRunner(args=args, port=test_port)
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'inspector')
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'Bindings')
        runner._host.filesystem.maybe_make_directory(runner._base_path, 'Parser')

        filesystem = runner._host.filesystem
        runner.load_output_json = lambda: json.loads(filesystem.read_text_file(runner._output_json_path()))
        return runner, test_port

    def run_test(self, test_name):
        runner, port = self.create_runner()
        driver = MainTest.TestDriver()
        return runner._run_single_test(ChromiumStylePerfTest(port, test_name, runner._host.filesystem.join('some-dir', test_name)), driver)

    def test_run_passing_test(self):
        self.assertTrue(self.run_test('pass.html'))

    def test_run_silent_test(self):
        self.assertFalse(self.run_test('silent.html'))

    def test_run_failed_test(self):
        self.assertFalse(self.run_test('failed.html'))

    def test_run_tonguey_test(self):
        self.assertFalse(self.run_test('tonguey.html'))

    def test_run_timeout_test(self):
        self.assertFalse(self.run_test('timeout.html'))

    def test_run_crash_test(self):
        self.assertFalse(self.run_test('crash.html'))

    def _tests_for_runner(self, runner, test_names):
        filesystem = runner._host.filesystem
        tests = []
        for test in test_names:
            path = filesystem.join(runner._base_path, test)
            dirname = filesystem.dirname(path)
            if test.startswith('inspector/'):
                tests.append(ChromiumStylePerfTest(runner._port, test, path))
            else:
                tests.append(PerfTest(runner._port, test, path))
        return tests

    def test_run_test_set(self):
        runner, port = self.create_runner()
        tests = self._tests_for_runner(runner, ['inspector/pass.html', 'inspector/silent.html', 'inspector/failed.html',
            'inspector/tonguey.html', 'inspector/timeout.html', 'inspector/crash.html'])
        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner._run_tests_set(tests, port)
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(unexpected_result_count, len(tests) - 1)
        self.assertTrue('\nRESULT group_name: test_name= 42 ms\n' in log)

    def test_run_test_set_kills_drt_per_run(self):

        class TestDriverWithStopCount(MainTest.TestDriver):
            stop_count = 0

            def stop(self):
                TestDriverWithStopCount.stop_count += 1

        runner, port = self.create_runner(driver_class=TestDriverWithStopCount)

        tests = self._tests_for_runner(runner, ['inspector/pass.html', 'inspector/silent.html', 'inspector/failed.html',
            'inspector/tonguey.html', 'inspector/timeout.html', 'inspector/crash.html'])
        unexpected_result_count = runner._run_tests_set(tests, port)

        self.assertEqual(TestDriverWithStopCount.stop_count, 6)

    def test_run_test_pause_before_testing(self):
        class TestDriverWithStartCount(MainTest.TestDriver):
            start_count = 0

            def start(self):
                TestDriverWithStartCount.start_count += 1

        runner, port = self.create_runner(args=["--pause-before-testing"], driver_class=TestDriverWithStartCount)
        tests = self._tests_for_runner(runner, ['inspector/pass.html'])

        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner._run_tests_set(tests, port)
            self.assertEqual(TestDriverWithStartCount.start_count, 1)
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(stderr, "Ready to run test?\n")
        self.assertEqual(log, "Running inspector/pass.html (1 of 1)\nRESULT group_name: test_name= 42 ms\n\n")

    def test_run_test_set_for_parser_tests(self):
        runner, port = self.create_runner()
        tests = self._tests_for_runner(runner, ['Bindings/event-target-wrapper.html', 'Parser/some-parser.html'])
        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner._run_tests_set(tests, port)
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(unexpected_result_count, 0)
        self.assertEqual(log, '\n'.join(['Running Bindings/event-target-wrapper.html (1 of 2)',
        'RESULT Bindings: event-target-wrapper= 1489.05 ms',
        'median= 1487.0 ms, stdev= 14.46 ms, min= 1471.0 ms, max= 1510.0 ms',
        '',
        'Running Parser/some-parser.html (2 of 2)',
        'RESULT Parser: some-parser= 1100.0 ms',
        'median= 1101.0 ms, stdev= 11.0 ms, min= 1080.0 ms, max= 1120.0 ms',
        '', '']))

    def test_run_memory_test(self):
        runner, port = self.create_runner_and_setup_results_template()
        runner._timestamp = 123456789
        port.host.filesystem.write_text_file(runner._base_path + '/Parser/memory-test.html', 'some content')

        output = OutputCapture()
        output.capture_output()
        try:
            unexpected_result_count = runner.run()
        finally:
            stdout, stderr, log = output.restore_output()
        self.assertEqual(unexpected_result_count, 0)
        self.assertEqual(log, '\n'.join([
            'Running 1 tests',
            'Running Parser/memory-test.html (1 of 1)',
            'RESULT Parser: memory-test= 1100.0 ms',
            'median= 1101.0 ms, stdev= 11.0 ms, min= 1080.0 ms, max= 1120.0 ms',
            'RESULT Parser: memory-test: JSHeap= 832000.0 bytes',
            'median= 829000.0 bytes, stdev= 15000.0 bytes, min= 811000.0 bytes, max= 848000.0 bytes',
            'RESULT Parser: memory-test: Malloc= 532000.0 bytes',
            'median= 529000.0 bytes, stdev= 13000.0 bytes, min= 511000.0 bytes, max= 548000.0 bytes',
            '', '']))
        results = runner.load_output_json()[0]['results']
        self.assertEqual(results['Parser/memory-test'], {'min': 1080.0, 'max': 1120.0, 'median': 1101.0, 'stdev': 11.0, 'avg': 1100.0, 'unit': 'ms'})
        self.assertEqual(results['Parser/memory-test:JSHeap'], {'min': 811000.0, 'max': 848000.0, 'median': 829000.0, 'stdev': 15000.0, 'avg': 832000.0, 'unit': 'bytes'})
        self.assertEqual(results['Parser/memory-test:Malloc'], {'min': 511000.0, 'max': 548000.0, 'median': 529000.0, 'stdev': 13000.0, 'avg': 532000.0, 'unit': 'bytes'})

    def _test_run_with_json_output(self, runner, filesystem, upload_suceeds=False, expected_exit_code=0):
        filesystem.write_text_file(runner._base_path + '/inspector/pass.html', 'some content')
        filesystem.write_text_file(runner._base_path + '/Bindings/event-target-wrapper.html', 'some content')

        uploaded = [False]

        def mock_upload_json(hostname, json_path):
            self.assertEqual(hostname, 'some.host')
            self.assertEqual(json_path, '/mock-checkout/output.json')
            uploaded[0] = upload_suceeds
            return upload_suceeds

        runner._upload_json = mock_upload_json
        runner._timestamp = 123456789
        output_capture = OutputCapture()
        output_capture.capture_output()
        try:
            self.assertEqual(runner.run(), expected_exit_code)
        finally:
            stdout, stderr, logs = output_capture.restore_output()

        if not expected_exit_code:
            self.assertEqual(logs, '\n'.join([
                'Running 2 tests',
                'Running Bindings/event-target-wrapper.html (1 of 2)',
                'RESULT Bindings: event-target-wrapper= 1489.05 ms',
                'median= 1487.0 ms, stdev= 14.46 ms, min= 1471.0 ms, max= 1510.0 ms',
                '',
                'Running inspector/pass.html (2 of 2)',
                'RESULT group_name: test_name= 42 ms',
                '',
                '']))

        self.assertEqual(uploaded[0], upload_suceeds)

        return logs

    _event_target_wrapper_and_inspector_results = {
        "Bindings/event-target-wrapper": {"max": 1510, "avg": 1489.05, "median": 1487, "min": 1471, "stdev": 14.46, "unit": "ms"},
        "inspector/pass.html:group_name:test_name": 42}

    def test_run_with_json_output(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=True)
        self.assertEqual(runner.load_output_json(), {
            "timestamp": 123456789, "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "branch": "webkit-trunk"})

    def test_run_with_description(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host', '--description', 'some description'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=True)
        self.assertEqual(runner.load_output_json(), {
            "timestamp": 123456789, "description": "some description",
            "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "branch": "webkit-trunk"})

    def create_runner_and_setup_results_template(self, args=[]):
        runner, port = self.create_runner(args)
        filesystem = port.host.filesystem
        filesystem.write_text_file(runner._base_path + '/resources/results-template.html',
            'BEGIN<script src="%AbsolutePathToWebKitTrunk%/some.js"></script>'
            '<script src="%AbsolutePathToWebKitTrunk%/other.js"></script><script>%PeformanceTestsResultsJSON%</script>END')
        filesystem.write_text_file(runner._base_path + '/Dromaeo/resources/dromaeo/web/lib/jquery-1.6.4.js', 'jquery content')
        return runner, port

    def test_run_respects_no_results(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host', '--no-results'])
        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=False)
        self.assertFalse(port.host.filesystem.isfile('/mock-checkout/output.json'))

    def test_run_generates_json_by_default(self):
        runner, port = self.create_runner_and_setup_results_template()
        filesystem = port.host.filesystem
        output_json_path = filesystem.join(port.perf_results_directory(), runner._DEFAULT_JSON_FILENAME)
        results_page_path = filesystem.splitext(output_json_path)[0] + '.html'

        self.assertFalse(filesystem.isfile(output_json_path))
        self.assertFalse(filesystem.isfile(results_page_path))

        self._test_run_with_json_output(runner, port.host.filesystem)

        self.assertEqual(json.loads(port.host.filesystem.read_text_file(output_json_path)), [{
            "timestamp": 123456789, "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "branch": "webkit-trunk"}])

        self.assertTrue(filesystem.isfile(output_json_path))
        self.assertTrue(filesystem.isfile(results_page_path))

    def test_run_generates_and_show_results_page(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        page_shown = []
        port.show_results_html_file = lambda path: page_shown.append(path)
        filesystem = port.host.filesystem
        self._test_run_with_json_output(runner, filesystem)

        expected_entry = {"timestamp": 123456789, "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "branch": "webkit-trunk"}

        self.maxDiff = None
        json_output = port.host.filesystem.read_text_file('/mock-checkout/output.json')
        self.assertEqual(json.loads(json_output), [expected_entry])
        self.assertEqual(filesystem.read_text_file('/mock-checkout/output.html'),
            'BEGIN<script src="/test.checkout/some.js"></script><script src="/test.checkout/other.js"></script>'
            '<script>%s</script>END' % json_output)
        self.assertEqual(page_shown[0], '/mock-checkout/output.html')

        self._test_run_with_json_output(runner, filesystem)
        json_output = port.host.filesystem.read_text_file('/mock-checkout/output.json')
        self.assertEqual(json.loads(json_output), [expected_entry, expected_entry])
        self.assertEqual(filesystem.read_text_file('/mock-checkout/output.html'),
            'BEGIN<script src="/test.checkout/some.js"></script><script src="/test.checkout/other.js"></script>'
            '<script>%s</script>END' % json_output)

    def test_run_respects_no_show_results(self):
        show_results_html_file = lambda path: page_shown.append(path)

        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        page_shown = []
        port.show_results_html_file = show_results_html_file
        self._test_run_with_json_output(runner, port.host.filesystem)
        self.assertEqual(page_shown[0], '/mock-checkout/output.html')

        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json',
            '--no-show-results'])
        page_shown = []
        port.show_results_html_file = show_results_html_file
        self._test_run_with_json_output(runner, port.host.filesystem)
        self.assertEqual(page_shown, [])

    def test_run_with_bad_output_json(self):
        runner, port = self.create_runner_and_setup_results_template(args=['--output-json-path=/mock-checkout/output.json'])
        port.host.filesystem.write_text_file('/mock-checkout/output.json', 'bad json')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_MERGE)
        port.host.filesystem.write_text_file('/mock-checkout/output.json', '{"another bad json": "1"}')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_MERGE)

    def test_run_with_slave_config_json(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--source-json-path=/mock-checkout/slave-config.json', '--test-results-server=some.host'])
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', '{"key": "value"}')
        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=True)
        self.assertEqual(runner.load_output_json(), {
            "timestamp": 123456789, "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "branch": "webkit-trunk", "key": "value"})

    def test_run_with_bad_slave_config_json(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--source-json-path=/mock-checkout/slave-config.json', '--test-results-server=some.host'])
        logs = self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)
        self.assertTrue('Missing slave configuration JSON file: /mock-checkout/slave-config.json' in logs)
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', 'bad json')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)
        port.host.filesystem.write_text_file('/mock-checkout/slave-config.json', '["another bad json"]')
        self._test_run_with_json_output(runner, port.host.filesystem, expected_exit_code=PerfTestsRunner.EXIT_CODE_BAD_SOURCE_JSON)

    def test_run_with_multiple_repositories(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server=some.host'])
        port.repository_paths = lambda: [('webkit', '/mock-checkout'), ('some', '/mock-checkout/some')]
        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=True)
        self.assertEqual(runner.load_output_json(), {
            "timestamp": 123456789, "results": self._event_target_wrapper_and_inspector_results,
            "webkit-revision": "5678", "some-revision": "5678", "branch": "webkit-trunk"})

    def test_run_with_upload_json(self):
        runner, port = self.create_runner(args=['--output-json-path=/mock-checkout/output.json',
            '--test-results-server', 'some.host', '--platform', 'platform1', '--builder-name', 'builder1', '--build-number', '123'])

        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=True)
        generated_json = json.loads(port.host.filesystem.files['/mock-checkout/output.json'])
        self.assertEqual(generated_json['platform'], 'platform1')
        self.assertEqual(generated_json['builder-name'], 'builder1')
        self.assertEqual(generated_json['build-number'], 123)

        self._test_run_with_json_output(runner, port.host.filesystem, upload_suceeds=False, expected_exit_code=PerfTestsRunner.EXIT_CODE_FAILED_UPLOADING)

    def test_upload_json(self):
        runner, port = self.create_runner()
        port.host.filesystem.files['/mock-checkout/some.json'] = 'some content'

        called = []
        upload_single_text_file_throws = False
        upload_single_text_file_return_value = StringIO.StringIO('OK')

        class MockFileUploader:
            def __init__(mock, url, timeout):
                self.assertEqual(url, 'https://some.host/api/test/report')
                self.assertTrue(isinstance(timeout, int) and timeout)
                called.append('FileUploader')

            def upload_single_text_file(mock, filesystem, content_type, filename):
                self.assertEqual(filesystem, port.host.filesystem)
                self.assertEqual(content_type, 'application/json')
                self.assertEqual(filename, 'some.json')
                called.append('upload_single_text_file')
                if upload_single_text_file_throws:
                    raise "Some exception"
                return upload_single_text_file_return_value

        runner._upload_json('some.host', 'some.json', MockFileUploader)
        self.assertEqual(called, ['FileUploader', 'upload_single_text_file'])

        output = OutputCapture()
        output.capture_output()
        upload_single_text_file_return_value = StringIO.StringIO('Some error')
        runner._upload_json('some.host', 'some.json', MockFileUploader)
        _, _, logs = output.restore_output()
        self.assertEqual(logs, 'Uploaded JSON but got a bad response:\nSome error\n')

        # Throwing an exception upload_single_text_file shouldn't blow up _upload_json
        called = []
        upload_single_text_file_throws = True
        runner._upload_json('some.host', 'some.json', MockFileUploader)
        self.assertEqual(called, ['FileUploader', 'upload_single_text_file'])

    def _add_file(self, runner, dirname, filename, content=True):
        dirname = runner._host.filesystem.join(runner._base_path, dirname) if dirname else runner._base_path
        runner._host.filesystem.maybe_make_directory(dirname)
        runner._host.filesystem.files[runner._host.filesystem.join(dirname, filename)] = content

    def test_collect_tests(self):
        runner, port = self.create_runner()
        self._add_file(runner, 'inspector', 'a_file.html', 'a content')
        tests = runner._collect_tests()
        self.assertEqual(len(tests), 1)

    def _collect_tests_and_sort_test_name(self, runner):
        return sorted([test.test_name() for test in runner._collect_tests()])

    def test_collect_tests_with_multile_files(self):
        runner, port = self.create_runner(args=['PerformanceTests/test1.html', 'test2.html'])

        def add_file(filename):
            port.host.filesystem.files[runner._host.filesystem.join(runner._base_path, filename)] = 'some content'

        add_file('test1.html')
        add_file('test2.html')
        add_file('test3.html')
        port.host.filesystem.chdir(runner._port.perf_tests_dir()[:runner._port.perf_tests_dir().rfind(runner._host.filesystem.sep)])
        self.assertEqual(self._collect_tests_and_sort_test_name(runner), ['test1.html', 'test2.html'])

    def test_collect_tests_with_skipped_list(self):
        runner, port = self.create_runner()

        self._add_file(runner, 'inspector', 'test1.html')
        self._add_file(runner, 'inspector', 'unsupported_test1.html')
        self._add_file(runner, 'inspector', 'test2.html')
        self._add_file(runner, 'inspector/resources', 'resource_file.html')
        self._add_file(runner, 'unsupported', 'unsupported_test2.html')
        port.skipped_perf_tests = lambda: ['inspector/unsupported_test1.html', 'unsupported']
        self.assertEqual(self._collect_tests_and_sort_test_name(runner), ['inspector/test1.html', 'inspector/test2.html'])

    def test_collect_tests_with_skipped_list(self):
        runner, port = self.create_runner(args=['--force'])

        self._add_file(runner, 'inspector', 'test1.html')
        self._add_file(runner, 'inspector', 'unsupported_test1.html')
        self._add_file(runner, 'inspector', 'test2.html')
        self._add_file(runner, 'inspector/resources', 'resource_file.html')
        self._add_file(runner, 'unsupported', 'unsupported_test2.html')
        port.skipped_perf_tests = lambda: ['inspector/unsupported_test1.html', 'unsupported']
        self.assertEqual(self._collect_tests_and_sort_test_name(runner), ['inspector/test1.html', 'inspector/test2.html', 'inspector/unsupported_test1.html', 'unsupported/unsupported_test2.html'])

    def test_collect_tests_with_page_load_svg(self):
        runner, port = self.create_runner()
        self._add_file(runner, 'PageLoad', 'some-svg-test.svg')
        tests = runner._collect_tests()
        self.assertEqual(len(tests), 1)
        self.assertEqual(tests[0].__class__.__name__, 'PageLoadingPerfTest')

    def test_collect_tests_should_ignore_replay_tests_by_default(self):
        runner, port = self.create_runner()
        self._add_file(runner, 'Replay', 'www.webkit.org.replay')
        self.assertEqual(runner._collect_tests(), [])

    def test_collect_tests_with_replay_tests(self):
        runner, port = self.create_runner(args=['--replay'])
        self._add_file(runner, 'Replay', 'www.webkit.org.replay')
        tests = runner._collect_tests()
        self.assertEqual(len(tests), 1)
        self.assertEqual(tests[0].__class__.__name__, 'ReplayPerfTest')

    def test_parse_args(self):
        runner, port = self.create_runner()
        options, args = PerfTestsRunner._parse_args([
                '--build-directory=folder42',
                '--platform=platform42',
                '--builder-name', 'webkit-mac-1',
                '--build-number=56',
                '--time-out-ms=42',
                '--output-json-path=a/output.json',
                '--source-json-path=a/source.json',
                '--test-results-server=somehost',
                '--debug'])
        self.assertEqual(options.build, True)
        self.assertEqual(options.build_directory, 'folder42')
        self.assertEqual(options.platform, 'platform42')
        self.assertEqual(options.builder_name, 'webkit-mac-1')
        self.assertEqual(options.build_number, '56')
        self.assertEqual(options.time_out_ms, '42')
        self.assertEqual(options.configuration, 'Debug')
        self.assertEqual(options.output_json_path, 'a/output.json')
        self.assertEqual(options.source_json_path, 'a/source.json')
        self.assertEqual(options.test_results_server, 'somehost')


if __name__ == '__main__':
    unittest.main()
