# Copyright (C) 2011 Google Inc. All rights reserved.
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


import logging
import re
import time

from webkitpy.layout_tests.controllers import test_result_writer
from webkitpy.layout_tests.port.driver import DriverInput, DriverOutput
from webkitpy.layout_tests.models import test_expectations
from webkitpy.layout_tests.models import test_failures
from webkitpy.layout_tests.models.test_results import TestResult


_log = logging.getLogger(__name__)


def run_single_test(port, options, test_input, driver, worker_name, stop_when_done):
    runner = SingleTestRunner(options, port, driver, test_input, worker_name, stop_when_done)
    return runner.run()


class SingleTestRunner(object):
    (ALONGSIDE_TEST, PLATFORM_DIR, VERSION_DIR, UPDATE) = ('alongside', 'platform', 'version', 'update')

    def __init__(self, options, port, driver, test_input, worker_name, stop_when_done):
        self._options = options
        self._port = port
        self._filesystem = port.host.filesystem
        self._driver = driver
        self._timeout = test_input.timeout
        self._worker_name = worker_name
        self._test_name = test_input.test_name
        self._should_run_pixel_test = test_input.should_run_pixel_test
        self._reference_files = test_input.reference_files
        self._stop_when_done = stop_when_done

        if self._reference_files:
            # Detect and report a test which has a wrong combination of expectation files.
            # For example, if 'foo.html' has two expectation files, 'foo-expected.html' and
            # 'foo-expected.txt', we should warn users. One test file must be used exclusively
            # in either layout tests or reftests, but not in both.
            for suffix in ('.txt', '.png', '.wav'):
                expected_filename = self._port.expected_filename(self._test_name, suffix)
                if self._filesystem.exists(expected_filename):
                    _log.error('%s is a reftest, but has an unused expectation file. Please remove %s.',
                        self._test_name, expected_filename)

    def _expected_driver_output(self):
        return DriverOutput(self._port.expected_text(self._test_name),
                                 self._port.expected_image(self._test_name),
                                 self._port.expected_checksum(self._test_name),
                                 self._port.expected_audio(self._test_name))

    def _should_fetch_expected_checksum(self):
        return self._should_run_pixel_test and not (self._options.new_baseline or self._options.reset_results)

    def _driver_input(self):
        # The image hash is used to avoid doing an image dump if the
        # checksums match, so it should be set to a blank value if we
        # are generating a new baseline.  (Otherwise, an image from a
        # previous run will be copied into the baseline."""
        image_hash = None
        if self._should_fetch_expected_checksum():
            image_hash = self._port.expected_checksum(self._test_name)
        return DriverInput(self._test_name, self._timeout, image_hash, self._should_run_pixel_test)

    def run(self):
        if self._reference_files:
            if self._port.get_option('no_ref_tests') or self._options.reset_results:
                result = TestResult(self._test_name)
                result.type = test_expectations.SKIP
                return result
            return self._run_reftest()
        if self._options.reset_results:
            return self._run_rebaseline()
        return self._run_compare_test()

    def _run_compare_test(self):
        driver_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        expected_driver_output = self._expected_driver_output()

        if self._options.ignore_metrics:
            expected_driver_output.strip_metrics()
            driver_output.strip_metrics()

        test_result = self._compare_output(expected_driver_output, driver_output)
        if self._options.new_test_results:
            self._add_missing_baselines(test_result, driver_output)
        test_result_writer.write_test_result(self._filesystem, self._port, self._test_name, driver_output, expected_driver_output, test_result.failures)
        return test_result

    def _run_rebaseline(self):
        driver_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        failures = self._handle_error(driver_output)
        test_result_writer.write_test_result(self._filesystem, self._port, self._test_name, driver_output, None, failures)
        # FIXME: It the test crashed or timed out, it might be better to avoid
        # to write new baselines.
        self._overwrite_baselines(driver_output)
        return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr())

    _render_tree_dump_pattern = re.compile(r"^layer at \(\d+,\d+\) size \d+x\d+\n")

    def _add_missing_baselines(self, test_result, driver_output):
        missingImage = test_result.has_failure_matching_types(test_failures.FailureMissingImage, test_failures.FailureMissingImageHash)
        if test_result.has_failure_matching_types(test_failures.FailureMissingResult):
            self._save_baseline_data(driver_output.text, '.txt', self._location_for_new_baseline(driver_output.text, '.txt'))
        if test_result.has_failure_matching_types(test_failures.FailureMissingAudio):
            self._save_baseline_data(driver_output.audio, '.wav', self._location_for_new_baseline(driver_output.audio, '.wav'))
        if missingImage:
            self._save_baseline_data(driver_output.image, '.png', self._location_for_new_baseline(driver_output.image, '.png'))

    def _location_for_new_baseline(self, data, extension):
        if self._options.add_platform_exceptions:
            return self.VERSION_DIR
        if extension == '.png':
            return self.PLATFORM_DIR
        if extension == '.wav':
            return self.ALONGSIDE_TEST
        if extension == '.txt' and self._render_tree_dump_pattern.match(data):
            return self.PLATFORM_DIR
        return self.ALONGSIDE_TEST

    def _overwrite_baselines(self, driver_output):
        location = self.VERSION_DIR if self._options.add_platform_exceptions else self.UPDATE
        self._save_baseline_data(driver_output.text, '.txt', location)
        self._save_baseline_data(driver_output.audio, '.wav', location)
        if self._should_run_pixel_test:
            self._save_baseline_data(driver_output.image, '.png', location)

    def _save_baseline_data(self, data, extension, location):
        if data is None:
            return
        port = self._port
        fs = self._filesystem
        if location == self.ALONGSIDE_TEST:
            output_dir = fs.dirname(port.abspath_for_test(self._test_name))
        elif location == self.VERSION_DIR:
            output_dir = fs.join(port.baseline_version_dir(), fs.dirname(self._test_name))
        elif location == self.PLATFORM_DIR:
            output_dir = fs.join(port.baseline_platform_dir(), fs.dirname(self._test_name))
        elif location == self.UPDATE:
            output_dir = fs.dirname(port.expected_filename(self._test_name, extension))
        else:
            raise AssertionError('unrecognized baseline location: %s' % location)

        fs.maybe_make_directory(output_dir)
        output_basename = fs.basename(fs.splitext(self._test_name)[0] + "-expected" + extension)
        output_path = fs.join(output_dir, output_basename)
        _log.info('Writing new expected result "%s"' % port.relative_test_filename(output_path))
        port.update_baseline(output_path, data)

    def _handle_error(self, driver_output, reference_filename=None):
        """Returns test failures if some unusual errors happen in driver's run.

        Args:
          driver_output: The output from the driver.
          reference_filename: The full path to the reference file which produced the driver_output.
              This arg is optional and should be used only in reftests until we have a better way to know
              which html file is used for producing the driver_output.
        """
        failures = []
        fs = self._filesystem
        if driver_output.timeout:
            failures.append(test_failures.FailureTimeout(bool(reference_filename)))

        if reference_filename:
            testname = self._port.relative_test_filename(reference_filename)
        else:
            testname = self._test_name

        if driver_output.crash:
            failures.append(test_failures.FailureCrash(bool(reference_filename),
                                                       driver_output.crashed_process_name,
                                                       driver_output.crashed_pid))
            if driver_output.error:
                _log.debug("%s %s crashed, (stderr lines):" % (self._worker_name, testname))
            else:
                _log.debug("%s %s crashed, (no stderr)" % (self._worker_name, testname))
        elif driver_output.error:
            _log.debug("%s %s output stderr lines:" % (self._worker_name, testname))
        for line in driver_output.error.splitlines():
            _log.debug("  %s" % line)
        return failures

    def _compare_output(self, expected_driver_output, driver_output):
        failures = []
        failures.extend(self._handle_error(driver_output))

        if driver_output.crash:
            # Don't continue any more if we already have a crash.
            # In case of timeouts, we continue since we still want to see the text and image output.
            return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr())

        failures.extend(self._compare_text(expected_driver_output.text, driver_output.text))
        failures.extend(self._compare_audio(expected_driver_output.audio, driver_output.audio))
        if self._should_run_pixel_test:
            failures.extend(self._compare_image(expected_driver_output, driver_output))
        return TestResult(self._test_name, failures, driver_output.test_time, driver_output.has_stderr())

    def _compare_text(self, expected_text, actual_text):
        failures = []
        if (expected_text and actual_text and
            # Assuming expected_text is already normalized.
            self._port.do_text_results_differ(expected_text, self._get_normalized_output_text(actual_text))):
            failures.append(test_failures.FailureTextMismatch())
        elif actual_text and not expected_text:
            failures.append(test_failures.FailureMissingResult())
        return failures

    def _compare_audio(self, expected_audio, actual_audio):
        failures = []
        if (expected_audio and actual_audio and
            self._port.do_audio_results_differ(expected_audio, actual_audio)):
            failures.append(test_failures.FailureAudioMismatch())
        elif actual_audio and not expected_audio:
            failures.append(test_failures.FailureMissingAudio())
        return failures

    def _get_normalized_output_text(self, output):
        """Returns the normalized text output, i.e. the output in which
        the end-of-line characters are normalized to "\n"."""
        # Running tests on Windows produces "\r\n".  The "\n" part is helpfully
        # changed to "\r\n" by our system (Python/Cygwin), resulting in
        # "\r\r\n", when, in fact, we wanted to compare the text output with
        # the normalized text expectation files.
        return output.replace("\r\r\n", "\r\n").replace("\r\n", "\n")

    # FIXME: This function also creates the image diff. Maybe that work should
    # be handled elsewhere?
    def _compare_image(self, expected_driver_output, driver_output):
        failures = []
        # If we didn't produce a hash file, this test must be text-only.
        if driver_output.image_hash is None:
            return failures
        if not expected_driver_output.image:
            failures.append(test_failures.FailureMissingImage())
        elif not expected_driver_output.image_hash:
            failures.append(test_failures.FailureMissingImageHash())
        elif driver_output.image_hash != expected_driver_output.image_hash:
            diff_result = self._port.diff_image(expected_driver_output.image, driver_output.image)
            err_str = diff_result[2]
            if err_str:
                _log.warning('  %s : %s' % (self._test_name, err_str))
                failures.append(test_failures.FailureImageHashMismatch())
                driver_output.error = (driver_output.error or '') + err_str
            else:
                driver_output.image_diff = diff_result[0]
                if driver_output.image_diff:
                    failures.append(test_failures.FailureImageHashMismatch(diff_result[1]))
                else:
                    # See https://bugs.webkit.org/show_bug.cgi?id=69444 for why this isn't a full failure.
                    _log.warning('  %s -> pixel hash failed (but diff passed)' % self._test_name)
        return failures

    def _run_reftest(self):
        test_output = self._driver.run_test(self._driver_input(), self._stop_when_done)
        total_test_time = 0
        reference_output = None
        test_result = None

        # A reftest can have multiple match references and multiple mismatch references;
        # the test fails if any mismatch matches and all of the matches don't match.
        # To minimize the number of references we have to check, we run all of the mismatches first,
        # then the matches, and short-circuit out as soon as we can.
        # Note that sorting by the expectation sorts "!=" before "==" so this is easy to do.

        putAllMismatchBeforeMatch = sorted
        for expectation, reference_filename in putAllMismatchBeforeMatch(self._reference_files):
            reference_test_name = self._port.relative_test_filename(reference_filename)
            reference_output = self._driver.run_test(DriverInput(reference_test_name, self._timeout, test_output.image_hash, should_run_pixel_test=True), self._stop_when_done)
            test_result = self._compare_output_with_reference(reference_output, test_output, reference_filename, expectation == '!=')

            if (expectation == '!=' and test_result.failures) or (expectation == '==' and not test_result.failures):
                break
            total_test_time += test_result.test_run_time

        assert(reference_output)
        test_result_writer.write_test_result(self._filesystem, self._port, self._test_name, test_output, reference_output, test_result.failures)
        return TestResult(self._test_name, test_result.failures, total_test_time + test_result.test_run_time, test_result.has_stderr)

    def _compare_output_with_reference(self, reference_driver_output, actual_driver_output, reference_filename, mismatch):
        total_test_time = reference_driver_output.test_time + actual_driver_output.test_time
        has_stderr = reference_driver_output.has_stderr() or actual_driver_output.has_stderr()
        failures = []
        failures.extend(self._handle_error(actual_driver_output))
        if failures:
            # Don't continue any more if we already have crash or timeout.
            return TestResult(self._test_name, failures, total_test_time, has_stderr)
        failures.extend(self._handle_error(reference_driver_output, reference_filename=reference_filename))
        if failures:
            return TestResult(self._test_name, failures, total_test_time, has_stderr)

        if not reference_driver_output.image_hash and not actual_driver_output.image_hash:
            failures.append(test_failures.FailureReftestNoImagesGenerated(reference_filename))
        elif mismatch:
            if reference_driver_output.image_hash == actual_driver_output.image_hash:
                diff_result = self._port.diff_image(reference_driver_output.image, actual_driver_output.image, tolerance=0)
                if not diff_result[0]:
                    failures.append(test_failures.FailureReftestMismatchDidNotOccur(reference_filename))
                else:
                    _log.warning("  %s -> ref test hashes matched but diff failed" % self._test_name)

        elif reference_driver_output.image_hash != actual_driver_output.image_hash:
            diff_result = self._port.diff_image(reference_driver_output.image, actual_driver_output.image, tolerance=0)
            if diff_result[0]:
                failures.append(test_failures.FailureReftestMismatch(reference_filename))
            else:
                _log.warning("  %s -> ref test hashes didn't match but diff passed" % self._test_name)

        return TestResult(self._test_name, failures, total_test_time, has_stderr)
