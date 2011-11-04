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

import threading

from webkitpy.common.host_mock import MockHost
from webkitpy.common.net.statusserver_mock import MockStatusServer
from webkitpy.common.net.irc.irc_mock import MockIRC

# FIXME: Old-style "Ports" need to die and be replaced by modern layout_tests.port which needs to move to common.
from webkitpy.common.config.ports_mock import MockPort


# FIXME: This should be moved somewhere in common and renamed
# something without Mock in the name.
class MockOptions(object):
    """Mock implementation of optparse.Values."""

    def __init__(self, **kwargs):
        # The caller can set option values using keyword arguments. We don't
        # set any values by default because we don't know how this
        # object will be used. Generally speaking unit tests should
        # subclass this or provider wrapper functions that set a common
        # set of options.
        for key, value in kwargs.items():
            self.__dict__[key] = value


# FIXME: This should be renamed MockWebKitPatch.
class MockTool(MockHost):
    def __init__(self, *args, **kwargs):
        MockHost.__init__(self, *args, **kwargs)

        self._deprecated_port = MockPort()
        self.status_server = MockStatusServer()

        self._irc = None
        self.irc_password = "MOCK irc password"
        self.wakeup_event = threading.Event()

    def port(self):
        return self._deprecated_port

    def path(self):
        return "echo"

    def ensure_irc_connected(self, delegate):
        if not self._irc:
            self._irc = MockIRC()

    def irc(self):
        return self._irc
