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

import cPickle

from webkitpy.layout_tests.models import test_failures


class TestResult(object):
    """Data object containing the results of a single test."""

    @staticmethod
    def loads(str):
        return cPickle.loads(str)

    def __init__(self, test_name, failures=None, test_run_time=None, has_stderr=False):
        self.test_name = test_name
        self.failures = failures or []
        self.test_run_time = test_run_time or 0
        self.has_stderr = has_stderr
        # FIXME: Setting this in the constructor makes this class hard to mutate.
        self.type = test_failures.determine_result_type(failures)

    def __eq__(self, other):
        return (self.test_name == other.test_name and
                self.failures == other.failures and
                self.test_run_time == other.test_run_time)

    def __ne__(self, other):
        return not (self == other)

    def has_failure_matching_types(self, *args, **kargs):
        for failure in self.failures:
            if type(failure) in args:
                return True
        return False

    def dumps(self):
        return cPickle.dumps(self)
