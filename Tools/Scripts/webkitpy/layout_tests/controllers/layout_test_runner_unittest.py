#!/usr/bin/python
# Copyright (C) 2012 Google Inc. All rights reserved.
# Copyright (C) 2010 Gabor Rapcsanyi (rgabor@inf.u-szeged.hu), University of Szeged
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

from webkitpy.common.host_mock import MockHost
from webkitpy.layout_tests import run_webkit_tests
from webkitpy.layout_tests.models import test_expectations
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models.result_summary import ResultSummary
from webkitpy.layout_tests.models.test_input import TestInput
from webkitpy.layout_tests.models.test_results import TestResult
from webkitpy.layout_tests.controllers.layout_test_runner import LayoutTestRunner, Sharder, TestRunInterruptedException


TestExpectations = test_expectations.TestExpectations


class FakePrinter(object):
    num_completed = 0
    num_tests = 0

    def print_workers_and_shards(self, num_workers, num_shards, num_locked_shards):
        pass

    def print_started_test(self, test_name):
        pass

    def print_finished_test(self, result, expected, exp_str, got_str):
        pass

    def write(self, msg):
        pass

    def write_update(self, msg):
        pass

    def flush(self):
        pass


class LockCheckingRunner(LayoutTestRunner):
    def __init__(self, port, options, printer, tester, http_lock):
        super(LockCheckingRunner, self).__init__(options, port, printer, port.results_directory(), TestExpectations(port, []), lambda test_name: False)
        self._finished_list_called = False
        self._tester = tester
        self._should_have_http_lock = http_lock

    def handle_finished_list(self, source, list_name, num_tests, elapsed_time):
        if not self._finished_list_called:
            self._tester.assertEquals(list_name, 'locked_tests')
            self._tester.assertTrue(self._remaining_locked_shards)
            self._tester.assertTrue(self._has_http_lock is self._should_have_http_lock)

        super(LockCheckingRunner, self).handle_finished_list(source, list_name, num_tests, elapsed_time)

        if not self._finished_list_called:
            self._tester.assertEquals(self._remaining_locked_shards, [])
            self._tester.assertFalse(self._has_http_lock)
            self._finished_list_called = True


class LayoutTestRunnerTests(unittest.TestCase):
    def _runner(self, port=None):
        # FIXME: we shouldn't have to use run_webkit_tests.py to get the options we need.
        options = run_webkit_tests.parse_args(['--platform', 'test-mac-snowleopard'])[0]
        options.child_processes = '1'

        host = MockHost()
        port = port or host.port_factory.get(options.platform, options=options)
        return LockCheckingRunner(port, options, FakePrinter(), self, True)

    def _result_summary(self, runner, tests):
        return ResultSummary(TestExpectations(runner._port, tests), tests, 1, set())

    def _run_tests(self, runner, tests):
        test_inputs = [TestInput(test, 6000) for test in tests]
        expectations = TestExpectations(runner._port, tests)
        runner.run_tests(test_inputs, expectations, self._result_summary(runner, tests),
            num_workers=1, needs_http=any('http' in test for test in tests), needs_websockets=any(['websocket' in test for test in tests]), retrying=False)

    def test_http_locking(self):
        runner = self._runner()
        self._run_tests(runner, ['http/tests/passes/text.html', 'passes/text.html'])

    def test_perf_locking(self):
        runner = self._runner()
        self._run_tests(runner, ['http/tests/passes/text.html', 'perf/foo/test.html'])

    def test_interrupt_if_at_failure_limits(self):
        runner = self._runner()
        runner._options.exit_after_n_failures = None
        runner._options.exit_after_n_crashes_or_times = None
        test_names = ['passes/text.html', 'passes/image.html']
        runner._test_files_list = test_names

        result_summary = self._result_summary(runner, test_names)
        result_summary.unexpected_failures = 100
        result_summary.unexpected_crashes = 50
        result_summary.unexpected_timeouts = 50
        # No exception when the exit_after* options are None.
        runner._interrupt_if_at_failure_limits(result_summary)

        # No exception when we haven't hit the limit yet.
        runner._options.exit_after_n_failures = 101
        runner._options.exit_after_n_crashes_or_timeouts = 101
        runner._interrupt_if_at_failure_limits(result_summary)

        # Interrupt if we've exceeded either limit:
        runner._options.exit_after_n_crashes_or_timeouts = 10
        self.assertRaises(TestRunInterruptedException, runner._interrupt_if_at_failure_limits, result_summary)
        self.assertEquals(result_summary.results['passes/text.html'].type, test_expectations.SKIP)
        self.assertEquals(result_summary.results['passes/image.html'].type, test_expectations.SKIP)

        runner._options.exit_after_n_crashes_or_timeouts = None
        runner._options.exit_after_n_failures = 10
        exception = self.assertRaises(TestRunInterruptedException, runner._interrupt_if_at_failure_limits, result_summary)

    def test_update_summary_with_result(self):
        # Reftests expected to be image mismatch should be respected when pixel_tests=False.
        runner = self._runner()
        runner._options.pixel_tests = False
        test = 'failures/expected/reftest.html'
        expectations = TestExpectations(runner._port, tests=[test])
        runner._expectations = expectations
        result_summary = ResultSummary(expectations, [test], 1, set())
        result = TestResult(test_name=test, failures=[test_failures.FailureReftestMismatchDidNotOccur()])
        runner._update_summary_with_result(result_summary, result)
        self.assertEquals(1, result_summary.expected)
        self.assertEquals(0, result_summary.unexpected)

    def test_servers_started(self):

        def start_http_server(number_of_servers=None):
            self.http_started = True

        def start_websocket_server():
            self.websocket_started = True

        def stop_http_server():
            self.http_stopped = True

        def stop_websocket_server():
            self.websocket_stopped = True

        host = MockHost()
        port = host.port_factory.get('test-mac-leopard')
        port.start_http_server = start_http_server
        port.start_websocket_server = start_websocket_server
        port.stop_http_server = stop_http_server
        port.stop_websocket_server = stop_websocket_server

        self.http_started = self.http_stopped = self.websocket_started = self.websocket_stopped = False
        runner = self._runner(port=port)
        runner._needs_http = True
        runner._needs_websockets = False
        runner.start_servers_with_lock(number_of_servers=4)
        self.assertEquals(self.http_started, True)
        self.assertEquals(self.websocket_started, False)
        runner.stop_servers_with_lock()
        self.assertEquals(self.http_stopped, True)
        self.assertEquals(self.websocket_stopped, False)

        self.http_started = self.http_stopped = self.websocket_started = self.websocket_stopped = False
        runner._needs_http = True
        runner._needs_websockets = True
        runner.start_servers_with_lock(number_of_servers=4)
        self.assertEquals(self.http_started, True)
        self.assertEquals(self.websocket_started, True)
        runner.stop_servers_with_lock()
        self.assertEquals(self.http_stopped, True)
        self.assertEquals(self.websocket_stopped, True)

        self.http_started = self.http_stopped = self.websocket_started = self.websocket_stopped = False
        runner._needs_http = False
        runner._needs_websockets = False
        runner.start_servers_with_lock(number_of_servers=4)
        self.assertEquals(self.http_started, False)
        self.assertEquals(self.websocket_started, False)
        runner.stop_servers_with_lock()
        self.assertEquals(self.http_stopped, False)
        self.assertEquals(self.websocket_stopped, False)


class SharderTests(unittest.TestCase):

    test_list = [
        "http/tests/websocket/tests/unicode.htm",
        "animations/keyframes.html",
        "http/tests/security/view-source-no-refresh.html",
        "http/tests/websocket/tests/websocket-protocol-ignored.html",
        "fast/css/display-none-inline-style-change-crash.html",
        "http/tests/xmlhttprequest/supported-xml-content-types.html",
        "dom/html/level2/html/HTMLAnchorElement03.html",
        "ietestcenter/Javascript/11.1.5_4-4-c-1.html",
        "dom/html/level2/html/HTMLAnchorElement06.html",
        "perf/object-keys.html",
    ]

    def get_test_input(self, test_file):
        return TestInput(test_file, requires_lock=(test_file.startswith('http') or test_file.startswith('perf')))

    def get_shards(self, num_workers, fully_parallel, test_list=None, max_locked_shards=1):
        def split(test_name):
            idx = test_name.rfind('/')
            if idx != -1:
                return (test_name[0:idx], test_name[idx + 1:])

        self.sharder = Sharder(split, '/', max_locked_shards)
        test_list = test_list or self.test_list
        return self.sharder.shard_tests([self.get_test_input(test) for test in test_list], num_workers, fully_parallel)

    def assert_shards(self, actual_shards, expected_shard_names):
        self.assertEquals(len(actual_shards), len(expected_shard_names))
        for i, shard in enumerate(actual_shards):
            expected_shard_name, expected_test_names = expected_shard_names[i]
            self.assertEquals(shard.name, expected_shard_name)
            self.assertEquals([test_input.test_name for test_input in shard.test_inputs],
                              expected_test_names)

    def test_shard_by_dir(self):
        locked, unlocked = self.get_shards(num_workers=2, fully_parallel=False)

        # Note that although there are tests in multiple dirs that need locks,
        # they are crammed into a single shard in order to reduce the # of
        # workers hitting the server at once.
        self.assert_shards(locked,
             [('locked_shard_1',
               ['http/tests/security/view-source-no-refresh.html',
                'http/tests/websocket/tests/unicode.htm',
                'http/tests/websocket/tests/websocket-protocol-ignored.html',
                'http/tests/xmlhttprequest/supported-xml-content-types.html',
                'perf/object-keys.html'])])
        self.assert_shards(unlocked,
            [('animations', ['animations/keyframes.html']),
             ('dom/html/level2/html', ['dom/html/level2/html/HTMLAnchorElement03.html',
                                      'dom/html/level2/html/HTMLAnchorElement06.html']),
             ('fast/css', ['fast/css/display-none-inline-style-change-crash.html']),
             ('ietestcenter/Javascript', ['ietestcenter/Javascript/11.1.5_4-4-c-1.html'])])

    def test_shard_every_file(self):
        locked, unlocked = self.get_shards(num_workers=2, fully_parallel=True)
        self.assert_shards(locked,
            [('.', ['http/tests/websocket/tests/unicode.htm']),
             ('.', ['http/tests/security/view-source-no-refresh.html']),
             ('.', ['http/tests/websocket/tests/websocket-protocol-ignored.html']),
             ('.', ['http/tests/xmlhttprequest/supported-xml-content-types.html']),
             ('.', ['perf/object-keys.html'])]),
        self.assert_shards(unlocked,
            [('.', ['animations/keyframes.html']),
             ('.', ['fast/css/display-none-inline-style-change-crash.html']),
             ('.', ['dom/html/level2/html/HTMLAnchorElement03.html']),
             ('.', ['ietestcenter/Javascript/11.1.5_4-4-c-1.html']),
             ('.', ['dom/html/level2/html/HTMLAnchorElement06.html'])])

    def test_shard_in_two(self):
        locked, unlocked = self.get_shards(num_workers=1, fully_parallel=False)
        self.assert_shards(locked,
            [('locked_tests',
              ['http/tests/websocket/tests/unicode.htm',
               'http/tests/security/view-source-no-refresh.html',
               'http/tests/websocket/tests/websocket-protocol-ignored.html',
               'http/tests/xmlhttprequest/supported-xml-content-types.html',
               'perf/object-keys.html'])])
        self.assert_shards(unlocked,
            [('unlocked_tests',
              ['animations/keyframes.html',
               'fast/css/display-none-inline-style-change-crash.html',
               'dom/html/level2/html/HTMLAnchorElement03.html',
               'ietestcenter/Javascript/11.1.5_4-4-c-1.html',
               'dom/html/level2/html/HTMLAnchorElement06.html'])])

    def test_shard_in_two_has_no_locked_shards(self):
        locked, unlocked = self.get_shards(num_workers=1, fully_parallel=False,
             test_list=['animations/keyframe.html'])
        self.assertEquals(len(locked), 0)
        self.assertEquals(len(unlocked), 1)

    def test_shard_in_two_has_no_unlocked_shards(self):
        locked, unlocked = self.get_shards(num_workers=1, fully_parallel=False,
             test_list=['http/tests/websocket/tests/unicode.htm'])
        self.assertEquals(len(locked), 1)
        self.assertEquals(len(unlocked), 0)

    def test_multiple_locked_shards(self):
        locked, unlocked = self.get_shards(num_workers=4, fully_parallel=False, max_locked_shards=2)
        self.assert_shards(locked,
            [('locked_shard_1',
              ['http/tests/security/view-source-no-refresh.html',
               'http/tests/websocket/tests/unicode.htm',
               'http/tests/websocket/tests/websocket-protocol-ignored.html']),
             ('locked_shard_2',
              ['http/tests/xmlhttprequest/supported-xml-content-types.html',
               'perf/object-keys.html'])])

        locked, unlocked = self.get_shards(num_workers=4, fully_parallel=False)
        self.assert_shards(locked,
            [('locked_shard_1',
              ['http/tests/security/view-source-no-refresh.html',
               'http/tests/websocket/tests/unicode.htm',
               'http/tests/websocket/tests/websocket-protocol-ignored.html',
               'http/tests/xmlhttprequest/supported-xml-content-types.html',
               'perf/object-keys.html'])])


class NaturalCompareTest(unittest.TestCase):
    def assert_cmp(self, x, y, result):
        self.assertEquals(cmp(Sharder.natural_sort_key(x), Sharder.natural_sort_key(y)), result)

    def test_natural_compare(self):
        self.assert_cmp('a', 'a', 0)
        self.assert_cmp('ab', 'a', 1)
        self.assert_cmp('a', 'ab', -1)
        self.assert_cmp('', '', 0)
        self.assert_cmp('', 'ab', -1)
        self.assert_cmp('1', '2', -1)
        self.assert_cmp('2', '1', 1)
        self.assert_cmp('1', '10', -1)
        self.assert_cmp('2', '10', -1)
        self.assert_cmp('foo_1.html', 'foo_2.html', -1)
        self.assert_cmp('foo_1.1.html', 'foo_2.html', -1)
        self.assert_cmp('foo_1.html', 'foo_10.html', -1)
        self.assert_cmp('foo_2.html', 'foo_10.html', -1)
        self.assert_cmp('foo_23.html', 'foo_10.html', 1)
        self.assert_cmp('foo_23.html', 'foo_100.html', -1)


class KeyCompareTest(unittest.TestCase):
    def setUp(self):
        def split(test_name):
            idx = test_name.rfind('/')
            if idx != -1:
                return (test_name[0:idx], test_name[idx + 1:])

        self.sharder = Sharder(split, '/', 1)

    def assert_cmp(self, x, y, result):
        self.assertEquals(cmp(self.sharder.test_key(x), self.sharder.test_key(y)), result)

    def test_test_key(self):
        self.assert_cmp('/a', '/a', 0)
        self.assert_cmp('/a', '/b', -1)
        self.assert_cmp('/a2', '/a10', -1)
        self.assert_cmp('/a2/foo', '/a10/foo', -1)
        self.assert_cmp('/a/foo11', '/a/foo2', 1)
        self.assert_cmp('/ab', '/a/a/b', -1)
        self.assert_cmp('/a/a/b', '/ab', 1)
        self.assert_cmp('/foo-bar/baz', '/foo/baz', -1)
