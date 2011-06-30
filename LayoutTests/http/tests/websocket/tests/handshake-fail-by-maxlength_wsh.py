# Copyright (C) Research In Motion Limited 2011. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#
# 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
# disclaimer.
#
# 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
# following disclaimer in the documentation and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS “AS IS” AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL
# APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
# HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
# TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import time


def web_socket_do_extra_handshake(request):
    # This will cause the handshake to fail because it pushes the length of the
    # status line past 1024 characters
    msg = '.' * 1024
    msg += 'HTTP/1.1 101 WebSocket Protocol Handshake\r\n'
    msg += 'Upgrade: WebSocket\r\n'
    msg += 'Connection: Upgrade\r\n'
    msg += 'Sec-WebSocket-Location: ' + request.ws_location + '\r\n'
    msg += 'Sec-WebSocket-Origin: ' + request.ws_origin + '\r\n'
    msg += '\r\n'
    msg += request.ws_challenge_md5
    request.connection.write(msg)
    # continue writing data until the client disconnects
    while True:
        time.sleep(1)
        request.connection.write('keepalive\n')


def web_socket_transfer_data(request):
    pass
