# Copyright (c) 2011 Google Inc. All rights reserved.
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


class ExpectedFailures(object):
    def __init__(self):
        self._failures = set()
        # If the set of failures is unbounded, self._failures isn't very
        # meaningful because we can't store an unbounded set in memory.
        self._failures_are_bounded = True

    def _has_failures(self, results):
        return bool(results and len(results.failing_tests()) != 0)

    def has_bounded_failures(self, results):
        assert(results)  # You probably want to call _has_failures first!
        return bool(results.failure_limit_count() and len(results.failing_tests()) < results.failure_limit_count())

    def _can_trust_results(self, results):
        return self._has_failures(results) and self.has_bounded_failures(results)

    def failures_were_expected(self, results):
        if not self._can_trust_results(results):
            return False
        return set(results.failing_tests()) <= self._failures

    def unexpected_failures_observed(self, results):
        if not self._has_failures(results):
            return None
        if not self._failures_are_bounded:
            return None
        return set(results.failing_tests()) - self._failures

    def shrink_expected_failures(self, results, run_success):
        if run_success:
            self._failures = set()
            self._failures_are_bounded = True
        elif self._can_trust_results(results):
            # Remove all expected failures which are not in the new failing results.
            self._failures.intersection_update(set(results.failing_tests()))
            self._failures_are_bounded = True

    def grow_expected_failures(self, results):
        if not self._can_trust_results(results):
            self._failures_are_bounded = False
            return
        self._failures.update(results.failing_tests())
        self._failures_are_bounded = True
        # FIXME: Should we assert() here that expected_failures never crosses a certain size?
