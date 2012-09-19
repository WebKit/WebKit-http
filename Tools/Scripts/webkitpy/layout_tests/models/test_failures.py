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

from webkitpy.layout_tests.models import test_expectations


def is_reftest_failure(failure_list):
    failure_types = [type(f) for f in failure_list]
    return set((FailureReftestMismatch, FailureReftestMismatchDidNotOccur, FailureReftestNoImagesGenerated)).intersection(failure_types)

# FIXME: This is backwards.  Each TestFailure subclass should know what
# test_expectation type it corresponds too.  Then this method just
# collects them all from the failure list and returns the worst one.
def determine_result_type(failure_list):
    """Takes a set of test_failures and returns which result type best fits
    the list of failures. "Best fits" means we use the worst type of failure.

    Returns:
      one of the test_expectations result types - PASS, FAIL, CRASH, etc."""

    if not failure_list or len(failure_list) == 0:
        return test_expectations.PASS

    failure_types = [type(f) for f in failure_list]
    if FailureCrash in failure_types:
        return test_expectations.CRASH
    elif FailureTimeout in failure_types:
        return test_expectations.TIMEOUT
    elif FailureEarlyExit in failure_types:
        return test_expectations.SKIP
    elif (FailureMissingResult in failure_types or
          FailureMissingImage in failure_types or
          FailureMissingImageHash in failure_types or
          FailureMissingAudio in failure_types):
        return test_expectations.MISSING
    else:
        if FailureTextMismatch in failure_types or FailureAudioMismatch in failure_types:
            return test_expectations.FAIL
        elif FailureImageHashIncorrect in failure_types or FailureImageHashMismatch in failure_types or is_reftest_failure(failure_list):
            return test_expectations.IMAGE
        else:
            raise ValueError("unclassifiable set of failures: "
                             + str(failure_types))


class TestFailure(object):
    """Abstract base class that defines the failure interface."""

    @staticmethod
    def loads(s):
        """Creates a TestFailure object from the specified string."""
        return cPickle.loads(s)

    def message(self):
        """Returns a string describing the failure in more detail."""
        raise NotImplementedError

    def __eq__(self, other):
        return self.__class__.__name__ == other.__class__.__name__

    def __ne__(self, other):
        return self.__class__.__name__ != other.__class__.__name__

    def __hash__(self):
        return hash(self.__class__.__name__)

    def dumps(self):
        """Returns the string/JSON representation of a TestFailure."""
        return cPickle.dumps(self)

    def driver_needs_restart(self):
        """Returns True if we should kill DumpRenderTree/WebKitTestRunner before the next test."""
        return False


class FailureTimeout(TestFailure):
    """Test timed out.  We also want to restart DumpRenderTree if this happens."""
    def __init__(self, is_reftest=False):
        super(FailureTimeout, self).__init__()
        self.is_reftest = is_reftest

    def message(self):
        return "Test timed out"

    def driver_needs_restart(self):
        return True


class FailureCrash(TestFailure):
    """DumpRenderTree/WebKitTestRunner crashed."""
    def __init__(self, is_reftest=False, process_name='DumpRenderTree', pid=None):
        super(FailureCrash, self).__init__()
        self.process_name = process_name
        self.pid = pid
        self.is_reftest = is_reftest

    def message(self):
        if self.pid:
            return "%s (pid %d) crashed" % (self.process_name, self.pid)
        return self.process_name + " crashed"

    def driver_needs_restart(self):
        return True


class FailureMissingResult(TestFailure):
    """Expected result was missing."""

    def message(self):
        return "No expected results found"


class FailureTextMismatch(TestFailure):
    """Text diff output failed."""

    def message(self):
        return "Text diff mismatch"


class FailureMissingImageHash(TestFailure):
    """Actual result hash was missing."""

    def message(self):
        return "No expected image hash found"


class FailureMissingImage(TestFailure):
    """Actual result image was missing."""

    def message(self):
        return "No expected image found"


class FailureImageHashMismatch(TestFailure):
    """Image hashes didn't match."""
    def __init__(self, diff_percent=0):
        super(FailureImageHashMismatch, self).__init__()
        self.diff_percent = diff_percent

    def message(self):
        return "Image mismatch"


class FailureImageHashIncorrect(TestFailure):
    """Actual result hash is incorrect."""

    def message(self):
        return "Images match, expected image hash incorrect. "


class FailureReftestMismatch(TestFailure):
    """The result didn't match the reference rendering."""

    def __init__(self, reference_filename=None):
        super(FailureReftestMismatch, self).__init__()
        self.reference_filename = reference_filename
        self.diff_percent = None

    def message(self):
        return "Mismatch with reference"


class FailureReftestMismatchDidNotOccur(TestFailure):
    """Unexpected match between the result and the reference rendering."""

    def __init__(self, reference_filename=None):
        super(FailureReftestMismatchDidNotOccur, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return "Mismatch with the reference did not occur"


class FailureReftestNoImagesGenerated(TestFailure):
    """Both the reftest and the -expected html file didn't generate pixel results."""

    def __init__(self, reference_filename=None):
        super(FailureReftestNoImagesGenerated, self).__init__()
        self.reference_filename = reference_filename

    def message(self):
        return "Reftest didn't generate pixel results."


class FailureMissingAudio(TestFailure):
    """Actual result image was missing."""

    def message(self):
        return "No expected audio found"


class FailureAudioMismatch(TestFailure):
    """Audio files didn't match."""

    def message(self):
        return "Audio mismatch"


class FailureEarlyExit(TestFailure):
    def message(self):
        return "Skipped due to early exit"


# Convenient collection of all failure classes for anything that might
# need to enumerate over them all.
ALL_FAILURE_CLASSES = (FailureTimeout, FailureCrash, FailureMissingResult,
                       FailureTextMismatch, FailureMissingImageHash,
                       FailureMissingImage, FailureImageHashMismatch,
                       FailureImageHashIncorrect, FailureReftestMismatch,
                       FailureReftestMismatchDidNotOccur, FailureReftestNoImagesGenerated,
                       FailureMissingAudio, FailureAudioMismatch,
                       FailureEarlyExit)
