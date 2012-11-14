# Copyright (C) 2009 Google Inc. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
#    * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
#    * Neither the name of Google Inc. nor the names of its
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

import os
import StringIO
import tempfile
import unittest

from webkitpy.common.system.executive import ScriptError
from webkitpy.common.system.deprecated_logging import *

class LoggingTest(unittest.TestCase):

    def assert_log_equals(self, log_input, expected_output):
        original_stderr = sys.stderr
        test_stderr = StringIO.StringIO()
        sys.stderr = test_stderr

        try:
            log(log_input)
            actual_output = test_stderr.getvalue()
        finally:
            sys.stderr = original_stderr

        self.assertEqual(actual_output, expected_output, "log(\"%s\") expected: %s actual: %s" % (log_input, expected_output, actual_output))

    def test_log(self):
        self.assert_log_equals("test", "test\n")

        # Test that log() does not throw an exception when passed an object instead of a string.
        self.assert_log_equals(ScriptError(message="ScriptError"), "ScriptError\n")


if __name__ == '__main__':
    unittest.main()
