/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef RTCPeerConnection_h
#define RTCPeerConnection_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "Dictionary.h"
#include "EventTarget.h"
#include "ExceptionBase.h"
#include "MediaStream.h"
#include "MediaStreamList.h"
#include "RTCIceCandidate.h"
#include "RTCPeerConnectionHandler.h"
#include "RTCPeerConnectionHandlerClient.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaConstraints;
class RTCConfiguration;
class RTCErrorCallback;
class RTCSessionDescription;
class RTCSessionDescriptionCallback;
class VoidCallback;

class RTCPeerConnection : public RefCounted<RTCPeerConnection>, public RTCPeerConnectionHandlerClient, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<RTCPeerConnection> create(ScriptExecutionContext*, const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode&);
    ~RTCPeerConnection();

    void createOffer(PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCErrorCallback>, const Dictionary& mediaConstraints, ExceptionCode&);

    void createAnswer(PassRefPtr<RTCSessionDescriptionCallback>, PassRefPtr<RTCErrorCallback>, const Dictionary& mediaConstraints, ExceptionCode&);

    void setLocalDescription(PassRefPtr<RTCSessionDescription>, PassRefPtr<VoidCallback>, PassRefPtr<RTCErrorCallback>, ExceptionCode&);
    PassRefPtr<RTCSessionDescription> localDescription(ExceptionCode&);

    void setRemoteDescription(PassRefPtr<RTCSessionDescription>, PassRefPtr<VoidCallback>, PassRefPtr<RTCErrorCallback>, ExceptionCode&);
    PassRefPtr<RTCSessionDescription> remoteDescription(ExceptionCode&);

    String readyState() const;

    void updateIce(const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode&);

    void addIceCandidate(RTCIceCandidate*, ExceptionCode&);

    String iceState() const;

    MediaStreamList* localStreams() const;

    MediaStreamList* remoteStreams() const;

    void addStream(const PassRefPtr<MediaStream>, const Dictionary& mediaConstraints, ExceptionCode&);

    void removeStream(MediaStream*, ExceptionCode&);

    void close(ExceptionCode&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(negotationneeded);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(icecandidate);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(open);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(statechange);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(addstream);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(removestream);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(icechange);

    // RTCPeerConnectionHandlerClient
    virtual void negotiationNeeded() OVERRIDE;
    virtual void didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor>) OVERRIDE;
    virtual void didChangeReadyState(ReadyState) OVERRIDE;
    virtual void didChangeIceState(IceState) OVERRIDE;
    virtual void didAddRemoteStream(PassRefPtr<MediaStreamDescriptor>) OVERRIDE;
    virtual void didRemoveRemoteStream(MediaStreamDescriptor*) OVERRIDE;

    // EventTarget
    virtual const AtomicString& interfaceName() const OVERRIDE;
    virtual ScriptExecutionContext* scriptExecutionContext() const OVERRIDE;

    // ActiveDOMObject
    virtual void stop() OVERRIDE;

    using RefCounted<RTCPeerConnection>::ref;
    using RefCounted<RTCPeerConnection>::deref;

private:
    RTCPeerConnection(ScriptExecutionContext*, PassRefPtr<RTCConfiguration>, PassRefPtr<MediaConstraints>, ExceptionCode&);

    static PassRefPtr<RTCConfiguration> parseConfiguration(const Dictionary& configuration, ExceptionCode&);

    // EventTarget implementation.
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }
    EventTargetData m_eventTargetData;

    void changeReadyState(ReadyState);
    void changeIceState(IceState);

    ReadyState m_readyState;
    IceState m_iceState;

    RefPtr<MediaStreamList> m_localStreams;
    RefPtr<MediaStreamList> m_remoteStreams;

    OwnPtr<RTCPeerConnectionHandler> m_peerHandler;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // RTCPeerConnection_h
