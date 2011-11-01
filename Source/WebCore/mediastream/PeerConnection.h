/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Ericsson AB. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PeerConnection_h
#define PeerConnection_h

#if ENABLE(MEDIA_STREAM)

#include "ActiveDOMObject.h"
#include "EventTarget.h"
#include "ExceptionBase.h"
#include "MediaStream.h"
#include "MediaStreamList.h"
#include "PeerConnectionHandler.h"
#include "PeerConnectionHandlerClient.h"
#include "SignalingCallback.h"
#include "Timer.h"
#include <wtf/OwnPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

// Note:
// SDP stands for Session Description Protocol, which is intended for describing
// multimedia sessions for the purposes of session announcement, session
// invitation, and other forms of multimedia session initiation.
//
// More information can be found here:
// http://tools.ietf.org/html/rfc4566
// http://en.wikipedia.org/wiki/Session_Description_Protocol

class PeerConnection : public RefCounted<PeerConnection>, public PeerConnectionHandlerClient, public EventTarget, public ActiveDOMObject {
public:
    static PassRefPtr<PeerConnection> create(ScriptExecutionContext*, const String& serverConfiguration, PassRefPtr<SignalingCallback>);
    ~PeerConnection();

    void processSignalingMessage(const String& message, ExceptionCode&);

    // Name and values of the enum must match the corressponding constants in the .idl file.
    enum ReadyState {
        NEW = 0,
        NEGOTIATING = 1,
        ACTIVE = 2,
        CLOSED = 3
    };

    ReadyState readyState() const;

    void send(const String& text, ExceptionCode&);
    void addStream(const PassRefPtr<MediaStream>, ExceptionCode&);
    void removeStream(MediaStream*, ExceptionCode&);
    MediaStreamList* localStreams() const;
    MediaStreamList* remoteStreams() const;
    void close(ExceptionCode&);

    DEFINE_ATTRIBUTE_EVENT_LISTENER(connecting);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(open);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(message);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(addstream);
    DEFINE_ATTRIBUTE_EVENT_LISTENER(removestream);

    // PeerConnectionHandlerClient
    virtual void didCompleteICEProcessing();
    virtual void didGenerateSDP(const String& sdp);
    virtual void didReceiveDataStreamMessage(const char* data, size_t length);
    virtual void didAddRemoteStream(PassRefPtr<MediaStreamDescriptor>);
    virtual void didRemoveRemoteStream(MediaStreamDescriptor*);

    // EventTarget
    virtual const AtomicString& interfaceName() const;
    virtual ScriptExecutionContext* scriptExecutionContext() const;

    // ActiveDOMObject
    virtual void stop();

    using RefCounted<PeerConnection>::ref;
    using RefCounted<PeerConnection>::deref;

private:
    PeerConnection(ScriptExecutionContext*, const String& serverConfiguration, PassRefPtr<SignalingCallback>);

    // EventTarget implementation.
    virtual EventTargetData* eventTargetData();
    virtual EventTargetData* ensureEventTargetData();
    virtual void refEventTarget() { ref(); }
    virtual void derefEventTarget() { deref(); }

    void scheduleInitialNegotiation();
    void initialNegotiationTimerFired(Timer<PeerConnection>*);
    void ensureStreamChangeScheduled();
    void streamChangeTimerFired(Timer<PeerConnection>*);
    void scheduleReadyStateChange(ReadyState);
    void readyStateChangeTimerFired(Timer<PeerConnection>*);

    void changeReadyState(ReadyState);

    RefPtr<SignalingCallback> m_signalingCallback;

    ReadyState m_readyState;
    bool m_iceStarted;

    RefPtr<MediaStreamList> m_localStreams;
    RefPtr<MediaStreamList> m_remoteStreams;

    // EventTarget implementation.
    EventTargetData m_eventTargetData;

    Timer<PeerConnection> m_initialNegotiationTimer;
    Timer<PeerConnection> m_streamChangeTimer;
    Timer<PeerConnection> m_readyStateChangeTimer;

    MediaStreamDescriptorVector m_pendingAddStreams;
    MediaStreamDescriptorVector m_pendingRemoveStreams;
    Vector<ReadyState> m_pendingReadyStates;

    OwnPtr<PeerConnectionHandler> m_peerHandler;
};

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)

#endif // PeerConnection_h
