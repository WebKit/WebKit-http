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

from webkitpy.layout_tests.models.test_expectations import TestExpectations, SKIP, CRASH, TIMEOUT


class ResultSummary(object):
    def __init__(self, expectations, test_files):
        self.total = len(test_files)
        self.remaining = self.total
        self.expectations = expectations
        self.expected = 0
        self.unexpected = 0
        self.unexpected_failures = 0
        self.unexpected_crashes = 0
        self.unexpected_timeouts = 0
        self.tests_by_expectation = {}
        self.tests_by_timeline = {}
        self.results = {}
        self.unexpected_results = {}
        self.failures = {}
        self.tests_by_expectation[SKIP] = set()
        for expectation in TestExpectations.EXPECTATIONS.values():
            self.tests_by_expectation[expectation] = set()
        for timeline in TestExpectations.TIMELINES.values():
            self.tests_by_timeline[timeline] = expectations.get_tests_with_timeline(timeline)

    def add(self, test_result, expected):
        self.tests_by_expectation[test_result.type].add(test_result.test_name)
        self.results[test_result.test_name] = test_result
        self.remaining -= 1
        if len(test_result.failures):
            self.failures[test_result.test_name] = test_result.failures
        if expected:
            self.expected += 1
        else:
            self.unexpected_results[test_result.test_name] = test_result
            self.unexpected += 1
            if len(test_result.failures):
                self.unexpected_failures += 1
            if test_result.type == CRASH:
                self.unexpected_crashes += 1
            elif test_result.type == TIMEOUT:
                self.unexpected_timeouts += 1
