/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebRTCPeerConnectionHandlerClient_h
#define WebRTCPeerConnectionHandlerClient_h

namespace WebKit {
class WebMediaStreamDescriptor;
class WebRTCICECandidate;

class WebRTCPeerConnectionHandlerClient {
public:
    enum ReadyState {
        ReadyStateNew = 1,
        ReadyStateOpening = 2,
        ReadyStateActive = 3,
        ReadyStateClosing = 4,
        ReadyStateClosed = 5
    };

    enum ICEState {
        ICEStateNew = 1,
        ICEStateGathering = 2,
        ICEStateWaiting = 3,
        ICEStateChecking = 4,
        ICEStateConnected = 5,
        ICEStateCompleted = 6,
        ICEStateFailed = 7,
        ICEStateClosed = 8
    };

    virtual ~WebRTCPeerConnectionHandlerClient() { }

    virtual void negotiationNeeded() = 0;
    virtual void didGenerateICECandidate(const WebRTCICECandidate&) = 0;
    virtual void didChangeReadyState(ReadyState) = 0;
    virtual void didChangeICEState(ICEState) = 0;
    virtual void didAddRemoteStream(const WebMediaStreamDescriptor&) = 0;
    virtual void didRemoveRemoteStream(const WebMediaStreamDescriptor&) = 0;
};

} // namespace WebKit

#endif // WebRTCPeerConnectionHandlerClient_h
