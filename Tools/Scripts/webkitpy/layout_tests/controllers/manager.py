#!/usr/bin/env python
# Copyright (C) 2010 Google Inc. All rights reserved.
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

"""
The Manager runs a series of tests (TestType interface) against a set
of test files.  If a test file fails a TestType, it returns a list of TestFailure
objects to the Manager. The Manager then aggregates the TestFailures to
create a final report.
"""

import errno
import logging
import math
import Queue
import random
import re
import sys
import time

from webkitpy.common import message_pool
from webkitpy.layout_tests.controllers import worker
from webkitpy.layout_tests.controllers.test_result_writer import TestResultWriter
from webkitpy.layout_tests.layout_package import json_layout_results_generator
from webkitpy.layout_tests.layout_package import json_results_generator
from webkitpy.layout_tests.models import test_expectations
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models import test_results
from webkitpy.layout_tests.models.test_input import TestInput
from webkitpy.layout_tests.models.result_summary import ResultSummary
from webkitpy.layout_tests.views import printing

from webkitpy.tool import grammar

_log = logging.getLogger(__name__)

# Builder base URL where we have the archived test results.
BUILDER_BASE_URL = "http://build.chromium.org/buildbot/layout_test_results/"

TestExpectations = test_expectations.TestExpectations


def interpret_test_failures(port, test_name, failures):
    """Interpret test failures and returns a test result as dict.

    Args:
        port: interface to port-specific hooks
        test_name: test name relative to layout_tests directory
        failures: list of test failures
    Returns:
        A dictionary like {'is_reftest': True, ...}
    """
    test_dict = {}
    failure_types = [type(failure) for failure in failures]
    # FIXME: get rid of all this is_* values once there is a 1:1 map between
    # TestFailure type and test_expectations.EXPECTATION.
    if test_failures.FailureMissingAudio in failure_types:
        test_dict['is_missing_audio'] = True

    for failure in failures:
        if isinstance(failure, test_failures.FailureImageHashMismatch):
            test_dict['image_diff_percent'] = failure.diff_percent
        elif isinstance(failure, test_failures.FailureReftestMismatch):
            test_dict['is_reftest'] = True
            test_dict['image_diff_percent'] = failure.diff_percent
        elif isinstance(failure, test_failures.FailureReftestMismatchDidNotOccur):
            test_dict['is_mismatch_reftest'] = True

    if test_failures.FailureMissingResult in failure_types:
        test_dict['is_missing_text'] = True

    if test_failures.FailureMissingImage in failure_types or test_failures.FailureMissingImageHash in failure_types:
        test_dict['is_missing_image'] = True
    return test_dict


def use_trac_links_in_results_html(port_obj):
    # We only use trac links on the buildbots.
    # Use existence of builder_name as a proxy for knowing we're on a bot.
    return port_obj.get_option("builder_name")


# FIXME: This should be on the Manager class (since that's the only caller)
# or split off from Manager onto another helper class, but should not be a free function.
# Most likely this should be made into its own class, and this super-long function
# split into many helper functions.
def summarize_results(port_obj, expectations, result_summary, retry_summary, test_timings, only_unexpected, interrupted):
    """Summarize failing results as a dict.

    FIXME: split this data structure into a separate class?

    Args:
        port_obj: interface to port-specific hooks
        expectations: test_expectations.TestExpectations object
        result_summary: summary object from initial test runs
        retry_summary: summary object from final test run of retried tests
        test_timings: a list of TestResult objects which contain test runtimes in seconds
        only_unexpected: whether to return a summary only for the unexpected results
    Returns:
        A dictionary containing a summary of the unexpected results from the
        run, with the following fields:
        'version': a version indicator
        'fixable': The number of fixable tests (NOW - PASS)
        'skipped': The number of skipped tests (NOW & SKIPPED)
        'num_regressions': The number of non-flaky failures
        'num_flaky': The number of flaky failures
        'num_missing': The number of tests with missing results
        'num_passes': The number of unexpected passes
        'tests': a dict of tests -> {'expected': '...', 'actual': '...'}
    """
    results = {}
    results['version'] = 3

    tbe = result_summary.tests_by_expectation
    tbt = result_summary.tests_by_timeline
    results['fixable'] = len(tbt[test_expectations.NOW] - tbe[test_expectations.PASS])
    results['skipped'] = len(tbt[test_expectations.NOW] & tbe[test_expectations.SKIP])

    num_passes = 0
    num_flaky = 0
    num_missing = 0
    num_regressions = 0
    keywords = {}
    for expecation_string, expectation_enum in TestExpectations.EXPECTATIONS.iteritems():
        keywords[expectation_enum] = expecation_string.upper()

    for modifier_string, modifier_enum in TestExpectations.MODIFIERS.iteritems():
        keywords[modifier_enum] = modifier_string.upper()

    tests = {}
    original_results = result_summary.unexpected_results if only_unexpected else result_summary.results

    for test_name, result in original_results.iteritems():
        # Note that if a test crashed in the original run, we ignore
        # whether or not it crashed when we retried it (if we retried it),
        # and always consider the result not flaky.
        expected = expectations.get_expectations_string(test_name)
        result_type = result.type
        actual = [keywords[result_type]]

        if result_type == test_expectations.SKIP:
            continue

        test_dict = {}
        if result.has_stderr:
            test_dict['has_stderr'] = True

        if expectations.has_modifier(test_name, test_expectations.WONTFIX):
            test_dict['wontfix'] = True

        if result_type == test_expectations.PASS:
            num_passes += 1
            # FIXME: include passing tests that have stderr output.
            if expected == 'PASS':
                continue
        elif result_type == test_expectations.CRASH:
            num_regressions += 1
        elif result_type == test_expectations.MISSING:
            if test_name in result_summary.unexpected_results:
                num_missing += 1
        elif test_name in result_summary.unexpected_results:
            if test_name not in retry_summary.unexpected_results:
                actual.extend(expectations.get_expectations_string(test_name).split(" "))
                num_flaky += 1
            else:
                retry_result_type = retry_summary.unexpected_results[test_name].type
                if result_type != retry_result_type:
                    actual.append(keywords[retry_result_type])
                    num_flaky += 1
                else:
                    num_regressions += 1

        test_dict['expected'] = expected
        test_dict['actual'] = " ".join(actual)
        # FIXME: Set this correctly once https://webkit.org/b/37739 is fixed
        # and only set it if there actually is stderr data.

        test_dict.update(interpret_test_failures(port_obj, test_name, result.failures))

        # Store test hierarchically by directory. e.g.
        # foo/bar/baz.html: test_dict
        # foo/bar/baz1.html: test_dict
        #
        # becomes
        # foo: {
        #     bar: {
        #         baz.html: test_dict,
        #         baz1.html: test_dict
        #     }
        # }
        parts = test_name.split('/')
        current_map = tests
        for i, part in enumerate(parts):
            if i == (len(parts) - 1):
                current_map[part] = test_dict
                break
            if part not in current_map:
                current_map[part] = {}
            current_map = current_map[part]

    results['tests'] = tests
    results['num_passes'] = num_passes
    results['num_flaky'] = num_flaky
    results['num_missing'] = num_missing
    results['num_regressions'] = num_regressions
    results['uses_expectations_file'] = port_obj.uses_test_expectations_file()
    results['interrupted'] = interrupted  # Does results.html have enough information to compute this itself? (by checking total number of results vs. total number of tests?)
    results['layout_tests_dir'] = port_obj.layout_tests_dir()
    results['has_wdiff'] = port_obj.wdiff_available()
    results['has_pretty_patch'] = port_obj.pretty_patch_available()
    results['pixel_tests_enabled'] = port_obj.get_option('pixel_tests')

    try:
        # We only use the svn revision for using trac links in the results.html file,
        # Don't do this by default since it takes >100ms.
        if use_trac_links_in_results_html(port_obj):
            port_obj.host._initialize_scm()
            results['revision'] = port_obj.host.scm().head_svn_revision()
    except Exception, e:
        _log.warn("Failed to determine svn revision for checkout (cwd: %s, webkit_base: %s), leaving 'revision' key blank in full_results.json.\n%s" % (port_obj._filesystem.getcwd(), port_obj.path_from_webkit_base(), e))
        # Handle cases where we're running outside of version control.
        import traceback
        _log.debug('Failed to learn head svn revision:')
        _log.debug(traceback.format_exc())
        results['revision'] = ""

    return results


class TestRunInterruptedException(Exception):
    """Raised when a test run should be stopped immediately."""
    def __init__(self, reason):
        Exception.__init__(self)
        self.reason = reason
        self.msg = reason

    def __reduce__(self):
        return self.__class__, (self.reason,)


# Export this so callers don't need to know about message pools.
WorkerException = message_pool.WorkerException


class TestShard(object):
    """A test shard is a named list of TestInputs."""

    # FIXME: Make this class visible, used by workers as well.
    def __init__(self, name, test_inputs):
        self.name = name
        self.test_inputs = test_inputs

    def __repr__(self):
        return "TestShard(name='%s', test_inputs=%s'" % (self.name, self.test_inputs)

    def __eq__(self, other):
        return self.name == other.name and self.test_inputs == other.test_inputs


class Manager(object):
    """A class for managing running a series of tests on a series of layout
    test files."""

    def __init__(self, port, options, printer):
        """Initialize test runner data structures.

        Args:
          port: an object implementing port-specific
          options: a dictionary of command line options
          printer: a Printer object to record updates to.
        """
        self._port = port
        self._filesystem = port.host.filesystem
        self._options = options
        self._printer = printer
        self._expectations = None

        self.HTTP_SUBDIR = 'http' + port.TEST_PATH_SEPARATOR
        self.PERF_SUBDIR = 'perf'
        self.WEBSOCKET_SUBDIR = 'websocket' + port.TEST_PATH_SEPARATOR
        self.LAYOUT_TESTS_DIRECTORY = 'LayoutTests'
        self._has_http_lock = False

        self._remaining_locked_shards = []

        # disable wss server. need to install pyOpenSSL on buildbots.
        # self._websocket_secure_server = websocket_server.PyWebSocket(
        #        options.results_directory, use_tls=True, port=9323)

        # a set of test files, and the same tests as a list

        self._paths = set()

        # FIXME: Rename to test_names.
        self._test_files = set()
        self._test_files_list = None
        self._result_queue = Queue.Queue()
        self._retrying = False
        self._results_directory = self._port.results_directory()

        self._all_results = []
        self._group_stats = {}
        self._worker_stats = {}
        self._current_result_summary = None

    def collect_tests(self, args):
        """Find all the files to test.

        Args:
          args: list of test arguments from the command line

        """
        paths = self._strip_test_dir_prefixes(args)
        if self._options.test_list:
            paths += self._strip_test_dir_prefixes(read_test_files(self._filesystem, self._options.test_list, self._port.TEST_PATH_SEPARATOR))
        self._paths = set(paths)
        self._test_files = self._port.tests(paths)

    def _strip_test_dir_prefixes(self, paths):
        return [self._strip_test_dir_prefix(path) for path in paths if path]

    def _strip_test_dir_prefix(self, path):
        # Handle both "LayoutTests/foo/bar.html" and "LayoutTests\foo\bar.html" if
        # the filesystem uses '\\' as a directory separator.
        if path.startswith(self.LAYOUT_TESTS_DIRECTORY + self._port.TEST_PATH_SEPARATOR):
            return path[len(self.LAYOUT_TESTS_DIRECTORY + self._port.TEST_PATH_SEPARATOR):]
        if path.startswith(self.LAYOUT_TESTS_DIRECTORY + self._filesystem.sep):
            return path[len(self.LAYOUT_TESTS_DIRECTORY + self._filesystem.sep):]
        return path

    def _is_http_test(self, test):
        return self.HTTP_SUBDIR in test or self.WEBSOCKET_SUBDIR in test

    def _http_tests(self):
        return set(test for test in self._test_files if self._is_http_test(test))

    def _websocket_tests(self):
        return set(test for test in self._test_files if self.WEBSOCKET_SUBDIR in test)

    def _is_perf_test(self, test):
        return self.PERF_SUBDIR == test or (self.PERF_SUBDIR + self._port.TEST_PATH_SEPARATOR) in test

    def parse_expectations(self):
        self._expectations = test_expectations.TestExpectations(self._port, self._test_files)

    def _split_into_chunks_if_necessary(self, skipped):
        if not self._options.run_chunk and not self._options.run_part:
            return skipped

        # If the user specifies they just want to run a subset of the tests,
        # just grab a subset of the non-skipped tests.
        chunk_value = self._options.run_chunk or self._options.run_part
        test_files = self._test_files_list
        try:
            (chunk_num, chunk_len) = chunk_value.split(":")
            chunk_num = int(chunk_num)
            assert(chunk_num >= 0)
            test_size = int(chunk_len)
            assert(test_size > 0)
        except AssertionError:
            _log.critical("invalid chunk '%s'" % chunk_value)
            return None

        # Get the number of tests
        num_tests = len(test_files)

        # Get the start offset of the slice.
        if self._options.run_chunk:
            chunk_len = test_size
            # In this case chunk_num can be really large. We need
            # to make the slave fit in the current number of tests.
            slice_start = (chunk_num * chunk_len) % num_tests
        else:
            # Validate the data.
            assert(test_size <= num_tests)
            assert(chunk_num <= test_size)

            # To count the chunk_len, and make sure we don't skip
            # some tests, we round to the next value that fits exactly
            # all the parts.
            rounded_tests = num_tests
            if rounded_tests % test_size != 0:
                rounded_tests = (num_tests + test_size - (num_tests % test_size))

            chunk_len = rounded_tests / test_size
            slice_start = chunk_len * (chunk_num - 1)
            # It does not mind if we go over test_size.

        # Get the end offset of the slice.
        slice_end = min(num_tests, slice_start + chunk_len)

        files = test_files[slice_start:slice_end]

        _log.debug('chunk slice [%d:%d] of %d is %d tests' % (slice_start, slice_end, num_tests, (slice_end - slice_start)))

        # If we reached the end and we don't have enough tests, we run some
        # from the beginning.
        if slice_end - slice_start < chunk_len:
            extra = chunk_len - (slice_end - slice_start)
            _log.debug('   last chunk is partial, appending [0:%d]' % extra)
            files.extend(test_files[0:extra])

        len_skip_chunk = int(len(files) * len(skipped) / float(len(self._test_files)))
        skip_chunk_list = list(skipped)[0:len_skip_chunk]
        skip_chunk = set(skip_chunk_list)

        # FIXME: This is a total hack.
        # Update expectations so that the stats are calculated correctly.
        # We need to pass a list that includes the right # of skipped files
        # to ParseExpectations so that ResultSummary() will get the correct
        # stats. So, we add in the subset of skipped files, and then
        # subtract them back out.
        self._test_files_list = files + skip_chunk_list
        self._test_files = set(self._test_files_list)

        self.parse_expectations()

        self._test_files = set(files)
        self._test_files_list = files

        return skip_chunk

    # FIXME: This method is way too long and needs to be broken into pieces.
    def prepare_lists_and_print_output(self):
        """Create appropriate subsets of test lists and returns a
        ResultSummary object. Also prints expected test counts.
        """

        # Remove skipped - both fixable and ignored - files from the
        # top-level list of files to test.
        found_test_files = set(self._test_files)
        num_all_test_files = len(self._test_files)

        skipped = self._expectations.get_tests_with_result_type(test_expectations.SKIP)
        if not self._options.http:
            skipped.update(set(self._http_tests()))

        if self._options.skipped == 'only':
            self._test_files = self._test_files.intersection(skipped)
        elif self._options.skipped == 'default':
            self._test_files -= skipped
        elif self._options.skipped == 'ignore':
            pass  # just to be clear that we're ignoring the skip list.

        if self._options.skip_failing_tests:
            self._test_files -= self._expectations.get_tests_with_result_type(test_expectations.FAIL)
            self._test_files -= self._expectations.get_tests_with_result_type(test_expectations.FLAKY)

        # now make sure we're explicitly running any tests passed on the command line.
        self._test_files.update(found_test_files.intersection(self._paths))

        num_to_run = len(self._test_files)
        num_skipped = num_all_test_files - num_to_run

        if not num_to_run:
            _log.critical('No tests to run.')
            return None

        # Create a sorted list of test files so the subset chunk,
        # if used, contains alphabetically consecutive tests.
        self._test_files_list = list(self._test_files)
        if self._options.randomize_order:
            random.shuffle(self._test_files_list)
        else:
            self._test_files_list.sort(key=lambda test: test_key(self._port, test))

        skipped = self._split_into_chunks_if_necessary(skipped)

        # FIXME: It's unclear how --repeat-each and --iterations should interact with chunks?
        if self._options.repeat_each:
            list_with_repetitions = []
            for test in self._test_files_list:
                list_with_repetitions += ([test] * self._options.repeat_each)
            self._test_files_list = list_with_repetitions

        if self._options.iterations:
            self._test_files_list = self._test_files_list * self._options.iterations

        iterations =  \
            (self._options.repeat_each if self._options.repeat_each else 1) * \
            (self._options.iterations if self._options.iterations else 1)
        result_summary = ResultSummary(self._expectations, self._test_files | skipped, iterations)

        self._printer.print_expected(num_all_test_files, result_summary, self._expectations.get_tests_with_result_type)

        if self._options.skipped != 'ignore':
            # Note that we don't actually run the skipped tests (they were
            # subtracted out of self._test_files, above), but we stub out the
            # results here so the statistics can remain accurate.
            for test in skipped:
                result = test_results.TestResult(test)
                result.type = test_expectations.SKIP
                for iteration in range(iterations):
                    result_summary.add(result, expected=True, test_is_slow=self._test_is_slow(test))

        return result_summary

    def _get_dir_for_test_file(self, test_file):
        """Returns the highest-level directory by which to shard the given
        test file."""
        directory, test_file = self._port.split_test(test_file)

        # The http tests are very stable on mac/linux.
        # TODO(ojan): Make the http server on Windows be apache so we can
        # turn shard the http tests there as well. Switching to apache is
        # what made them stable on linux/mac.
        return directory

    def _get_test_input_for_file(self, test_file):
        """Returns the appropriate TestInput object for the file. Mostly this
        is used for looking up the timeout value (in ms) to use for the given
        test."""
        if self._test_is_slow(test_file):
            return TestInput(test_file, self._options.slow_time_out_ms)
        return TestInput(test_file, self._options.time_out_ms)

    def _test_requires_lock(self, test_file):
        """Return True if the test needs to be locked when
        running multiple copies of NRWTs. Perf tests are locked
        because heavy load caused by running other tests in parallel
        might cause some of them to timeout."""
        return self._is_http_test(test_file) or self._is_perf_test(test_file)

    def _test_is_slow(self, test_file):
        return self._expectations.has_modifier(test_file, test_expectations.SLOW)

    def _is_ref_test(self, test_input):
        if test_input.reference_files is None:
            # Lazy initialization.
            test_input.reference_files = self._port.reference_files(test_input.test_name)
        return bool(test_input.reference_files)

    def _shard_tests(self, test_files, num_workers, fully_parallel, shard_ref_tests):
        """Groups tests into batches.
        This helps ensure that tests that depend on each other (aka bad tests!)
        continue to run together as most cross-tests dependencies tend to
        occur within the same directory.
        Return:
            Two list of TestShards. The first contains tests that must only be
            run under the server lock, the second can be run whenever.
        """

        # FIXME: Move all of the sharding logic out of manager into its
        # own class or module. Consider grouping it with the chunking logic
        # in prepare_lists as well.
        if num_workers == 1:
            return self._shard_in_two(test_files, shard_ref_tests)
        elif fully_parallel:
            return self._shard_every_file(test_files)
        return self._shard_by_directory(test_files, num_workers, shard_ref_tests)

    def _shard_in_two(self, test_files, shard_ref_tests):
        """Returns two lists of shards, one with all the tests requiring a lock and one with the rest.

        This is used when there's only one worker, to minimize the per-shard overhead."""
        locked_inputs = []
        locked_ref_test_inputs = []
        unlocked_inputs = []
        unlocked_ref_test_inputs = []
        for test_file in test_files:
            test_input = self._get_test_input_for_file(test_file)
            if self._test_requires_lock(test_file):
                if shard_ref_tests and self._is_ref_test(test_input):
                    locked_ref_test_inputs.append(test_input)
                else:
                    locked_inputs.append(test_input)
            else:
                if shard_ref_tests and self._is_ref_test(test_input):
                    unlocked_ref_test_inputs.append(test_input)
                else:
                    unlocked_inputs.append(test_input)
        locked_inputs.extend(locked_ref_test_inputs)
        unlocked_inputs.extend(unlocked_ref_test_inputs)

        locked_shards = []
        unlocked_shards = []
        if locked_inputs:
            locked_shards = [TestShard('locked_tests', locked_inputs)]
        if unlocked_inputs:
            unlocked_shards.append(TestShard('unlocked_tests', unlocked_inputs))

        return locked_shards, unlocked_shards

    def _shard_every_file(self, test_files):
        """Returns two lists of shards, each shard containing a single test file.

        This mode gets maximal parallelism at the cost of much higher flakiness."""
        locked_shards = []
        unlocked_shards = []
        for test_file in test_files:
            test_input = self._get_test_input_for_file(test_file)

            # Note that we use a '.' for the shard name; the name doesn't really
            # matter, and the only other meaningful value would be the filename,
            # which would be really redundant.
            if self._test_requires_lock(test_file):
                locked_shards.append(TestShard('.', [test_input]))
            else:
                unlocked_shards.append(TestShard('.', [test_input]))

        return locked_shards, unlocked_shards

    def _shard_by_directory(self, test_files, num_workers, shard_ref_tests):
        """Returns two lists of shards, each shard containing all the files in a directory.

        This is the default mode, and gets as much parallelism as we can while
        minimizing flakiness caused by inter-test dependencies."""
        locked_shards = []
        unlocked_shards = []
        tests_by_dir = {}
        ref_tests_by_dir = {}
        # FIXME: Given that the tests are already sorted by directory,
        # we can probably rewrite this to be clearer and faster.
        for test_file in test_files:
            directory = self._get_dir_for_test_file(test_file)
            test_input = self._get_test_input_for_file(test_file)
            if shard_ref_tests and self._is_ref_test(test_input):
                ref_tests_by_dir.setdefault(directory, [])
                ref_tests_by_dir[directory].append(test_input)
            else:
                tests_by_dir.setdefault(directory, [])
                tests_by_dir[directory].append(test_input)

        for directory, test_inputs in tests_by_dir.iteritems():
            shard = TestShard(directory, test_inputs)
            if self._test_requires_lock(directory):
                locked_shards.append(shard)
            else:
                unlocked_shards.append(shard)

        for directory, test_inputs in ref_tests_by_dir.iteritems():
            # '~' to place the ref tests after other tests after sorted.
            shard = TestShard('~ref:' + directory, test_inputs)
            if self._test_requires_lock(directory):
                locked_shards.append(shard)
            else:
                unlocked_shards.append(shard)

        # Sort the shards by directory name.
        locked_shards.sort(key=lambda shard: shard.name)
        unlocked_shards.sort(key=lambda shard: shard.name)

        return (self._resize_shards(locked_shards, self._max_locked_shards(num_workers),
                                    'locked_shard'),
                unlocked_shards)

    def _max_locked_shards(self, num_workers):
        # Put a ceiling on the number of locked shards, so that we
        # don't hammer the servers too badly.

        # FIXME: For now, limit to one shard or set it
        # with the --max-locked-shards. After testing to make sure we
        # can handle multiple shards, we should probably do something like
        # limit this to no more than a quarter of all workers, e.g.:
        # return max(math.ceil(num_workers / 4.0), 1)
        if self._options.max_locked_shards:
            num_of_locked_shards = self._options.max_locked_shards
        else:
            num_of_locked_shards = 1

        return num_of_locked_shards

    def _resize_shards(self, old_shards, max_new_shards, shard_name_prefix):
        """Takes a list of shards and redistributes the tests into no more
        than |max_new_shards| new shards."""

        # This implementation assumes that each input shard only contains tests from a
        # single directory, and that tests in each shard must remain together; as a
        # result, a given input shard is never split between output shards.
        #
        # Each output shard contains the tests from one or more input shards and
        # hence may contain tests from multiple directories.

        def divide_and_round_up(numerator, divisor):
            return int(math.ceil(float(numerator) / divisor))

        def extract_and_flatten(shards):
            test_inputs = []
            for shard in shards:
                test_inputs.extend(shard.test_inputs)
            return test_inputs

        def split_at(seq, index):
            return (seq[:index], seq[index:])

        num_old_per_new = divide_and_round_up(len(old_shards), max_new_shards)
        new_shards = []
        remaining_shards = old_shards
        while remaining_shards:
            some_shards, remaining_shards = split_at(remaining_shards, num_old_per_new)
            new_shards.append(TestShard('%s_%d' % (shard_name_prefix, len(new_shards) + 1),
                                        extract_and_flatten(some_shards)))
        return new_shards

    def _run_tests(self, file_list, result_summary, num_workers):
        """Runs the tests in the file_list.

        Return: A tuple (interrupted, keyboard_interrupted, thread_timings,
            test_timings, individual_test_timings)
            interrupted is whether the run was interrupted
            keyboard_interrupted is whether the interruption was because someone
              typed Ctrl^C
            thread_timings is a list of dicts with the total runtime
              of each thread with 'name', 'num_tests', 'total_time' properties
            test_timings is a list of timings for each sharded subdirectory
              of the form [time, directory_name, num_tests]
            individual_test_timings is a list of run times for each test
              in the form {filename:filename, test_run_time:test_run_time}
            result_summary: summary object to populate with the results
        """
        self._current_result_summary = result_summary
        self._all_results = []
        self._group_stats = {}
        self._worker_stats = {}

        keyboard_interrupted = False
        interrupted = False

        self._printer.write_update('Sharding tests ...')
        locked_shards, unlocked_shards = self._shard_tests(file_list, int(self._options.child_processes), self._options.fully_parallel, self._options.shard_ref_tests)

        # FIXME: We don't have a good way to coordinate the workers so that
        # they don't try to run the shards that need a lock if we don't actually
        # have the lock. The easiest solution at the moment is to grab the
        # lock at the beginning of the run, and then run all of the locked
        # shards first. This minimizes the time spent holding the lock, but
        # means that we won't be running tests while we're waiting for the lock.
        # If this becomes a problem in practice we'll need to change this.

        all_shards = locked_shards + unlocked_shards
        self._remaining_locked_shards = locked_shards
        if locked_shards and self._options.http:
            self.start_servers_with_lock(2 * min(num_workers, len(locked_shards)))

        num_workers = min(num_workers, len(all_shards))
        self._printer.print_workers_and_shards(num_workers, len(all_shards), len(locked_shards))

        def worker_factory(worker_connection):
            return worker.Worker(worker_connection, self.results_directory(), self._options)

        if self._options.dry_run:
            return (keyboard_interrupted, interrupted, self._worker_stats.values(), self._group_stats, self._all_results)

        self._printer.write_update('Starting %s ...' % grammar.pluralize('worker', num_workers))

        try:
            with message_pool.get(self, worker_factory, num_workers, self._port.worker_startup_delay_secs(), self._port.host) as pool:
                pool.run(('test_list', shard.name, shard.test_inputs) for shard in all_shards)
        except KeyboardInterrupt:
            self._printer.flush()
            self._printer.write('Interrupted, exiting ...')
            keyboard_interrupted = True
        except TestRunInterruptedException, e:
            _log.warning(e.reason)
            interrupted = True
        except Exception, e:
            _log.debug('%s("%s") raised, exiting' % (e.__class__.__name__, str(e)))
            raise
        finally:
            self.stop_servers_with_lock()

        # FIXME: should this be a class instead of a tuple?
        return (interrupted, keyboard_interrupted, self._worker_stats.values(), self._group_stats, self._all_results)

    def results_directory(self):
        if not self._retrying:
            return self._results_directory
        else:
            self._filesystem.maybe_make_directory(self._filesystem.join(self._results_directory, 'retries'))
            return self._filesystem.join(self._results_directory, 'retries')

    def needs_servers(self):
        return any(self._test_requires_lock(test_name) for test_name in self._test_files) and self._options.http

    def _set_up_run(self):
        """Configures the system to be ready to run tests.

        Returns a ResultSummary object if we should continue to run tests,
        or None if we should abort.

        """
        # This must be started before we check the system dependencies,
        # since the helper may do things to make the setup correct.
        if self._options.pixel_tests:
            self._printer.write_update("Starting pixel test helper ...")
            self._port.start_helper()

        # Check that the system dependencies (themes, fonts, ...) are correct.
        if not self._options.nocheck_sys_deps:
            self._printer.write_update("Checking system dependencies ...")
            if not self._port.check_sys_deps(self.needs_servers()):
                self._port.stop_helper()
                return None

        if self._options.clobber_old_results:
            self._clobber_old_results()

        # Create the output directory if it doesn't already exist.
        self._port.host.filesystem.maybe_make_directory(self._results_directory)

        self._port.setup_test_run()

        self._printer.write_update("Preparing tests ...")
        result_summary = self.prepare_lists_and_print_output()
        if not result_summary:
            return None

        return result_summary

    def run(self):
        """Run all our tests on all our test files.

        For each test file, we run each test type. If there are any failures,
        we collect them for reporting.

        Args:
          result_summary: a summary object tracking the test results.

        Return:
          The number of unexpected results (0 == success)
        """
        # collect_tests() must have been called first to initialize us.
        # If we didn't find any files to test, we've errored out already in
        # prepare_lists_and_print_output().

        result_summary = self._set_up_run()
        if not result_summary:
            return -1

        assert(len(self._test_files))

        start_time = time.time()

        interrupted, keyboard_interrupted, thread_timings, test_timings, individual_test_timings = self._run_tests(self._test_files_list, result_summary, int(self._options.child_processes))

        # We exclude the crashes from the list of results to retry, because
        # we want to treat even a potentially flaky crash as an error.
        failures = self._get_failures(result_summary, include_crashes=self._port.should_retry_crashes(), include_missing=False)
        retry_summary = result_summary
        while (len(failures) and self._options.retry_failures and not self._retrying and not interrupted and not keyboard_interrupted):
            _log.info('')
            _log.info("Retrying %d unexpected failure(s) ..." % len(failures))
            _log.info('')
            self._retrying = True
            retry_summary = ResultSummary(self._expectations, failures.keys())
            # Note that we intentionally ignore the return value here.
            self._run_tests(failures.keys(), retry_summary, num_workers=1)
            failures = self._get_failures(retry_summary, include_crashes=True, include_missing=True)

        end_time = time.time()

        # Some crash logs can take a long time to be written out so look
        # for new logs after the test run finishes.
        self._look_for_new_crash_logs(result_summary, start_time)
        self._look_for_new_crash_logs(retry_summary, start_time)
        self._clean_up_run()

        unexpected_results = summarize_results(self._port, self._expectations, result_summary, retry_summary, individual_test_timings, only_unexpected=True, interrupted=interrupted)

        self._printer.print_results(end_time - start_time, thread_timings, test_timings, individual_test_timings, result_summary, unexpected_results)

        # Re-raise a KeyboardInterrupt if necessary so the caller can handle it.
        if keyboard_interrupted:
            raise KeyboardInterrupt

        # FIXME: remove record_results. It's just used for testing. There's no need
        # for it to be a commandline argument.
        if (self._options.record_results and not self._options.dry_run and not keyboard_interrupted):
            self._port.print_leaks_summary()
            # Write the same data to log files and upload generated JSON files to appengine server.
            summarized_results = summarize_results(self._port, self._expectations, result_summary, retry_summary, individual_test_timings, only_unexpected=False, interrupted=interrupted)
            self._upload_json_files(summarized_results, result_summary, individual_test_timings)

        # Write the summary to disk (results.html) and display it if requested.
        if not self._options.dry_run:
            self._copy_results_html_file()
            if self._options.show_results:
                self._show_results_html_file(result_summary)

        return self._port.exit_code_from_summarized_results(unexpected_results)

    def start_servers_with_lock(self, number_of_servers):
        self._printer.write_update('Acquiring http lock ...')
        self._port.acquire_http_lock()
        if self._http_tests():
            self._printer.write_update('Starting HTTP server ...')
            self._port.start_http_server(number_of_servers=number_of_servers)
        if self._websocket_tests():
            self._printer.write_update('Starting WebSocket server ...')
            self._port.start_websocket_server()
        self._has_http_lock = True

    def stop_servers_with_lock(self):
        if self._has_http_lock:
            if self._http_tests():
                self._printer.write_update('Stopping HTTP server ...')
                self._port.stop_http_server()
            if self._websocket_tests():
                self._printer.write_update('Stopping WebSocket server ...')
                self._port.stop_websocket_server()
            self._printer.write_update('Releasing server lock ...')
            self._port.release_http_lock()
            self._has_http_lock = False

    def _clean_up_run(self):
        """Restores the system after we're done running tests."""
        _log.debug("flushing stdout")
        sys.stdout.flush()
        _log.debug("flushing stderr")
        sys.stderr.flush()
        _log.debug("stopping helper")
        self._port.stop_helper()
        _log.debug("cleaning up port")
        self._port.clean_up_test_run()

    def _look_for_new_crash_logs(self, result_summary, start_time):
        """Since crash logs can take a long time to be written out if the system is
           under stress do a second pass at the end of the test run.

           result_summary: the results of the test run
           start_time: time the tests started at.  We're looking for crash
               logs after that time.
        """
        crashed_processes = []
        for test, result in result_summary.unexpected_results.iteritems():
            if (result.type != test_expectations.CRASH):
                continue
            for failure in result.failures:
                if not isinstance(failure, test_failures.FailureCrash):
                    continue
                crashed_processes.append([test, failure.process_name, failure.pid])

        crash_logs = self._port.look_for_new_crash_logs(crashed_processes, start_time)
        if crash_logs:
            for test, crash_log in crash_logs.iteritems():
                writer = TestResultWriter(self._port._filesystem, self._port, self._port.results_directory(), test)
                writer.write_crash_log(crash_log)

    def _mark_interrupted_tests_as_skipped(self, result_summary):
        for test_name in self._test_files:
            if test_name not in result_summary.results:
                result = test_results.TestResult(test_name, [test_failures.FailureEarlyExit()])
                # FIXME: We probably need to loop here if there are multiple iterations.
                # FIXME: Also, these results are really neither expected nor unexpected. We probably
                # need a third type of result.
                result_summary.add(result, expected=False, test_is_slow=self._test_is_slow(test_name))

    def _interrupt_if_at_failure_limits(self, result_summary):
        # Note: The messages in this method are constructed to match old-run-webkit-tests
        # so that existing buildbot grep rules work.
        def interrupt_if_at_failure_limit(limit, failure_count, result_summary, message):
            if limit and failure_count >= limit:
                message += " %d tests run." % (result_summary.expected + result_summary.unexpected)
                self._mark_interrupted_tests_as_skipped(result_summary)
                raise TestRunInterruptedException(message)

        interrupt_if_at_failure_limit(
            self._options.exit_after_n_failures,
            result_summary.unexpected_failures,
            result_summary,
            "Exiting early after %d failures." % result_summary.unexpected_failures)
        interrupt_if_at_failure_limit(
            self._options.exit_after_n_crashes_or_timeouts,
            result_summary.unexpected_crashes + result_summary.unexpected_timeouts,
            result_summary,
            # This differs from ORWT because it does not include WebProcess crashes.
            "Exiting early after %d crashes and %d timeouts." % (result_summary.unexpected_crashes, result_summary.unexpected_timeouts))

    def _update_summary_with_result(self, result_summary, result):
        if result.type == test_expectations.SKIP:
            exp_str = got_str = 'SKIP'
            expected = True
        else:
            expected = self._expectations.matches_an_expected_result(result.test_name, result.type, self._options.pixel_tests or test_failures.is_reftest_failure(result.failures))
            exp_str = self._expectations.get_expectations_string(result.test_name)
            got_str = self._expectations.expectation_to_string(result.type)

        result_summary.add(result, expected, self._test_is_slow(result.test_name))

        # FIXME: there's too many arguments to this function.
        self._printer.print_finished_test(result, expected, exp_str, got_str, result_summary, self._retrying, self._test_files_list)

        self._interrupt_if_at_failure_limits(result_summary)

    def _clobber_old_results(self):
        # Just clobber the actual test results directories since the other
        # files in the results directory are explicitly used for cross-run
        # tracking.
        self._printer.write_update("Clobbering old results in %s" %
                                   self._results_directory)
        layout_tests_dir = self._port.layout_tests_dir()
        possible_dirs = self._port.test_dirs()
        for dirname in possible_dirs:
            if self._filesystem.isdir(self._filesystem.join(layout_tests_dir, dirname)):
                self._filesystem.rmtree(self._filesystem.join(self._results_directory, dirname))

    def _get_failures(self, result_summary, include_crashes, include_missing):
        """Filters a dict of results and returns only the failures.

        Args:
          result_summary: the results of the test run
          include_crashes: whether crashes are included in the output.
            We use False when finding the list of failures to retry
            to see if the results were flaky. Although the crashes may also be
            flaky, we treat them as if they aren't so that they're not ignored.
        Returns:
          a dict of files -> results
        """
        failed_results = {}
        for test, result in result_summary.unexpected_results.iteritems():
            if (result.type == test_expectations.PASS or
                (result.type == test_expectations.CRASH and not include_crashes) or
                (result.type == test_expectations.MISSING and not include_missing)):
                continue
            failed_results[test] = result.type

        return failed_results

    def _char_for_result(self, result):
        result = result.lower()
        if result in TestExpectations.EXPECTATIONS:
            result_enum_value = TestExpectations.EXPECTATIONS[result]
        else:
            result_enum_value = TestExpectations.MODIFIERS[result]
        return json_layout_results_generator.JSONLayoutResultsGenerator.FAILURE_TO_CHAR[result_enum_value]

    def _upload_json_files(self, summarized_results, result_summary, individual_test_timings):
        """Writes the results of the test run as JSON files into the results
        dir and upload the files to the appengine server.

        Args:
          unexpected_results: dict of unexpected results
          summarized_results: dict of results
          result_summary: full summary object
          individual_test_timings: list of test times (used by the flakiness
            dashboard).
        """
        _log.debug("Writing JSON files in %s." % self._results_directory)

        times_trie = json_results_generator.test_timings_trie(self._port, individual_test_timings)
        times_json_path = self._filesystem.join(self._results_directory, "times_ms.json")
        json_results_generator.write_json(self._filesystem, times_trie, times_json_path)

        full_results_path = self._filesystem.join(self._results_directory, "full_results.json")
        # We write full_results.json out as jsonp because we need to load it from a file url and Chromium doesn't allow that.
        json_results_generator.write_json(self._filesystem, summarized_results, full_results_path, callback="ADD_RESULTS")

        generator = json_layout_results_generator.JSONLayoutResultsGenerator(
            self._port, self._options.builder_name, self._options.build_name,
            self._options.build_number, self._results_directory,
            BUILDER_BASE_URL, individual_test_timings,
            self._expectations, result_summary, self._test_files_list,
            self._options.test_results_server,
            "layout-tests",
            self._options.master_name)

        _log.debug("Finished writing JSON files.")

        json_files = ["incremental_results.json", "full_results.json", "times_ms.json"]

        generator.upload_json_files(json_files)

        incremental_results_path = self._filesystem.join(self._results_directory, "incremental_results.json")

        # Remove these files from the results directory so they don't take up too much space on the buildbot.
        # The tools use the version we uploaded to the results server anyway.
        self._filesystem.remove(times_json_path)
        self._filesystem.remove(incremental_results_path)

    def _num_digits(self, num):
        """Returns the number of digits needed to represent the length of a
        sequence."""
        ndigits = 1
        if len(num):
            ndigits = int(math.log10(len(num))) + 1
        return ndigits

    def _copy_results_html_file(self):
        base_dir = self._port.path_from_webkit_base('LayoutTests', 'fast', 'harness')
        results_file = self._filesystem.join(base_dir, 'results.html')
        # FIXME: What should we do if this doesn't exist (e.g., in unit tests)?
        if self._filesystem.exists(results_file):
            self._filesystem.copyfile(results_file, self._filesystem.join(self._results_directory, "results.html"))

    def _show_results_html_file(self, result_summary):
        """Shows the results.html page."""
        if self._options.full_results_html:
            test_files = result_summary.failures.keys()
        else:
            unexpected_failures = self._get_failures(result_summary, include_crashes=True, include_missing=True)
            test_files = unexpected_failures.keys()

        if not len(test_files):
            return

        results_filename = self._filesystem.join(self._results_directory, "results.html")
        self._port.show_results_html_file(results_filename)

    def handle(self, name, source, *args):
        method = getattr(self, '_handle_' + name)
        if method:
            return method(source, *args)
        raise AssertionError('unknown message %s received from %s, args=%s' % (name, source, repr(args)))

    def _handle_started_test(self, worker_name, test_input, test_timeout_sec):
        # FIXME: log that we've started another test.
        pass

    def _handle_finished_test_list(self, worker_name, list_name, num_tests, elapsed_time):
        self._group_stats[list_name] = (num_tests, elapsed_time)

        def find(name, test_lists):
            for i in range(len(test_lists)):
                if test_lists[i].name == name:
                    return i
            return -1

        index = find(list_name, self._remaining_locked_shards)
        if index >= 0:
            self._remaining_locked_shards.pop(index)
            if not self._remaining_locked_shards:
                self.stop_servers_with_lock()

    def _handle_finished_test(self, worker_name, result, elapsed_time, log_messages=[]):
        self._worker_stats.setdefault(worker_name, {'name': worker_name, 'num_tests': 0, 'total_time': 0})
        self._worker_stats[worker_name]['total_time'] += elapsed_time
        self._worker_stats[worker_name]['num_tests'] += 1
        self._all_results.append(result)
        self._update_summary_with_result(self._current_result_summary, result)


def read_test_files(fs, filenames, test_path_separator):
    tests = []
    for filename in filenames:
        try:
            if test_path_separator != fs.sep:
                filename = filename.replace(test_path_separator, fs.sep)
            file_contents = fs.read_text_file(filename).split('\n')
            for line in file_contents:
                line = test_expectations.strip_comments(line)
                if line:
                    tests.append(line)
        except IOError, e:
            if e.errno == errno.ENOENT:
                _log.critical('')
                _log.critical('--test-list file "%s" not found' % file)
            raise
    return tests


# FIXME: These two free functions belong either on manager (since it's the only one
# which uses them) or in a different file (if they need to be re-used).
def test_key(port, test_name):
    """Turns a test name into a list with two sublists, the natural key of the
    dirname, and the natural key of the basename.

    This can be used when sorting paths so that files in a directory.
    directory are kept together rather than being mixed in with files in
    subdirectories."""
    dirname, basename = port.split_test(test_name)
    return (natural_sort_key(dirname + port.TEST_PATH_SEPARATOR), natural_sort_key(basename))


def natural_sort_key(string_to_split):
    """ Turn a string into a list of string and number chunks.
        "z23a" -> ["z", 23, "a"]

        Can be used to implement "natural sort" order. See:
            http://www.codinghorror.com/blog/2007/12/sorting-for-humans-natural-sort-order.html
            http://nedbatchelder.com/blog/200712.html#e20071211T054956
    """
    def tryint(val):
        try:
            return int(val)
        except ValueError:
            return val

    return [tryint(chunk) for chunk in re.split('(\d+)', string_to_split)]
