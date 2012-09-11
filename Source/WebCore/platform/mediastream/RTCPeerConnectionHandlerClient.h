/*
 * Copyright (C) 2012 Google AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
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

#ifndef RTCPeerConnectionHandlerClient_h
#define RTCPeerConnectionHandlerClient_h

#if ENABLE(MEDIA_STREAM)

#include <wtf/PassRefPtr.h>

namespace WebCore {

class MediaStreamDescriptor;
class RTCIceCandidateDescriptor;

class RTCPeerConnectionHandlerClient {
public:
    enum ReadyState {
        ReadyStateNew = 1,
        ReadyStateOpening = 2,
        ReadyStateActive = 3,
        ReadyStateClosing = 4,
        ReadyStateClosed = 5
    };

    enum IceState {
        IceStateNew = 1,
        IceStateGathering = 2,
        IceStateWaiting = 3,
        IceStateChecking = 4,
        IceStateConnected = 5,
        IceStateCompleted = 6,
        IceStateFailed = 7,
        IceStateClosed = 8
    };

    virtual ~RTCPeerConnectionHandlerClient() { }

    virtual void negotiationNeeded() = 0;
    virtual void didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor>) = 0;
    virtual void didChangeReadyState(ReadyState) = 0;
    virtual void didChangeIceState(IceState) = 0;
    virtual void didAddRemoteStream(PassRefPtr<MediaStreamDescriptor>) = 0;
    virtual void didRemoveRemoteStream(MediaStreamDescriptor*) = 0;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // RTCPeerConnectionHandlerClient_h
