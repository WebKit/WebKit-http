# Copyright (C) 2011 Google Inc. All rights reserved.
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

from webkitpy.thirdparty.mock import Mock
from webkitpy.tool.commands.commandtest import CommandsTest
from webkitpy.tool.commands.roll import *
from webkitpy.tool.mocktool import MockOptions, MockTool


class RollCommandsTest(CommandsTest):
    def test_update_chromium_deps(self):
        expected_stderr = """Updating Chromium DEPS to 6764
MOCK: MockDEPS.write_variable(chromium_rev, 6764)
MOCK: user.open_url: file://...
Was that diff correct?
Committed r49824: <http://trac.webkit.org/changeset/49824>
"""
        self.assert_execute_outputs(RollChromiumDEPS(), [6764], expected_stderr=expected_stderr)

    def test_update_chromium_deps_older_revision(self):
        options = MockOptions(non_interactive=False)
        expected_stderr = """Current Chromium DEPS revision 6564 is newer than 5764.
ERROR: Unable to update Chromium DEPS
"""
        self.assert_execute_outputs(RollChromiumDEPS(), [5764], options=options, expected_stderr=expected_stderr, expected_exception=SystemExit)
