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

#include "config.h"
#include "PeerConnection.h"

#if ENABLE(MEDIA_STREAM)

#include "ExceptionCode.h"
#include "MediaStreamEvent.h"
#include "MessageEvent.h"
#include "ScriptExecutionContext.h"
#include "SecurityOrigin.h"

namespace WebCore {

PassRefPtr<PeerConnection> PeerConnection::create(ScriptExecutionContext* context, const String& serverConfiguration, PassRefPtr<SignalingCallback> signalingCallback)
{
    RefPtr<PeerConnection> connection = adoptRef(new PeerConnection(context, serverConfiguration, signalingCallback));
    connection->setPendingActivity(connection.get());
    connection->scheduleInitialNegotiation();
    connection->suspendIfNeeded();

    return connection.release();
}

PeerConnection::PeerConnection(ScriptExecutionContext* context, const String& serverConfiguration, PassRefPtr<SignalingCallback> signalingCallback)
    : ActiveDOMObject(context, this)
    , m_signalingCallback(signalingCallback)
    , m_readyState(NEW)
    , m_iceStarted(false)
    , m_localStreams(MediaStreamList::create())
    , m_remoteStreams(MediaStreamList::create())
    , m_initialNegotiationTimer(this, &PeerConnection::initialNegotiationTimerFired)
    , m_streamChangeTimer(this, &PeerConnection::streamChangeTimerFired)
    , m_readyStateChangeTimer(this, &PeerConnection::readyStateChangeTimerFired)
    , m_peerHandler(PeerConnectionHandler::create(this, serverConfiguration, context->securityOrigin()->toString()))
{
}

PeerConnection::~PeerConnection()
{
}

void PeerConnection::processSignalingMessage(const String& message, ExceptionCode& ec)
{
    if (m_readyState == CLOSED) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!message.startsWith("SDP\n"))
        return;

    String sdp = message.substring(4);

    if (m_iceStarted) {
        if (m_peerHandler)
            m_peerHandler->processSDP(sdp);
        return;
    }

    if (m_peerHandler)
        m_peerHandler->handleInitialOffer(sdp);

    ensureStreamChangeScheduled();
    m_iceStarted = true;
    scheduleReadyStateChange(NEGOTIATING);
}

PeerConnection::ReadyState PeerConnection::readyState() const
{
    return m_readyState;
}

void PeerConnection::send(const String& text, ExceptionCode& ec)
{
    if (m_readyState == CLOSED) {
        ec = INVALID_STATE_ERR;
        return;
    }

    CString data = text.utf8();
    unsigned length = data.length();

    if (length > 504) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    if (m_peerHandler)
        m_peerHandler->sendDataStreamMessage(data.data(), length);
}

void PeerConnection::addStream(PassRefPtr<MediaStream> prpStream, ExceptionCode& ec)
{
    RefPtr<MediaStream> stream = prpStream;
    if (!stream) {
        ec =  TYPE_MISMATCH_ERR;
        return;
    }

    if (m_readyState == CLOSED) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (m_localStreams->contains(stream.get()))
        return;

    m_localStreams->append(stream);

    MediaStreamDescriptor* streamDescriptor = stream->descriptor();
    size_t i = m_pendingRemoveStreams.find(streamDescriptor);
    if (i != notFound) {
        m_pendingRemoveStreams.remove(i);
        return;
    }

    m_pendingAddStreams.append(streamDescriptor);
    if (m_iceStarted)
        ensureStreamChangeScheduled();
}

void PeerConnection::removeStream(MediaStream* stream, ExceptionCode& ec)
{
    if (m_readyState == CLOSED) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!stream) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    if (!m_localStreams->contains(stream))
        return;

    m_localStreams->remove(stream);

    MediaStreamDescriptor* streamDescriptor = stream->descriptor();
    size_t i = m_pendingAddStreams.find(streamDescriptor);
    if (i != notFound) {
        m_pendingAddStreams.remove(i);
        return;
    }

    m_pendingRemoveStreams.append(streamDescriptor);
    if (m_iceStarted)
        ensureStreamChangeScheduled();
}

MediaStreamList* PeerConnection::localStreams() const
{
    return m_localStreams.get();
}

MediaStreamList* PeerConnection::remoteStreams() const
{
    return m_remoteStreams.get();
}

void PeerConnection::close(ExceptionCode& ec)
{
    if (m_readyState == CLOSED) {
        ec = INVALID_STATE_ERR;
        return;
    }

    stop();
}

void PeerConnection::didCompleteICEProcessing()
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeReadyState(ACTIVE);
}

void PeerConnection::didGenerateSDP(const String& sdp)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    m_signalingCallback->handleEvent("SDP\n" + sdp, this);
}

void PeerConnection::didReceiveDataStreamMessage(const char* data, size_t length)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    const String& message = String::fromUTF8(data, length);
    dispatchEvent(MessageEvent::create(PassOwnPtr<MessagePortArray>(), SerializedScriptValue::create(message)));
}

void PeerConnection::didAddRemoteStream(PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_readyState == CLOSED)
        return;

    RefPtr<MediaStream> stream = MediaStream::create(scriptExecutionContext(), streamDescriptor);
    m_remoteStreams->append(stream);

    dispatchEvent(MediaStreamEvent::create(eventNames().addstreamEvent, false, false, stream.release()));
}

void PeerConnection::didRemoveRemoteStream(MediaStreamDescriptor* streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    ASSERT(streamDescriptor->owner());

    RefPtr<MediaStream> stream = static_cast<MediaStream*>(streamDescriptor->owner());
    stream->streamEnded();

    if (m_readyState == CLOSED)
        return;

    ASSERT(m_remoteStreams->contains(stream.get()));
    m_remoteStreams->remove(stream.get());

    dispatchEvent(MediaStreamEvent::create(eventNames().removestreamEvent, false, false, stream.release()));
}

const AtomicString& PeerConnection::interfaceName() const
{
    return eventNames().interfaceForPeerConnection;
}

ScriptExecutionContext* PeerConnection::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void PeerConnection::stop()
{
    if (m_readyState == CLOSED)
        return;

    m_initialNegotiationTimer.stop();
    m_streamChangeTimer.stop();
    m_readyStateChangeTimer.stop();

    if (m_peerHandler)
        m_peerHandler->stop();

    unsetPendingActivity(this);
    m_peerHandler.clear();

    changeReadyState(CLOSED);
}

EventTargetData* PeerConnection::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* PeerConnection::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void PeerConnection::scheduleInitialNegotiation()
{
    ASSERT(!m_initialNegotiationTimer.isActive());

    m_initialNegotiationTimer.startOneShot(0);
}

void PeerConnection::initialNegotiationTimerFired(Timer<PeerConnection>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_initialNegotiationTimer);

    if (m_iceStarted)
        return;

    MediaStreamDescriptorVector pendingAddStreams;
    m_pendingAddStreams.swap(pendingAddStreams);
    if (m_peerHandler)
        m_peerHandler->produceInitialOffer(pendingAddStreams);

    m_iceStarted = true;

    changeReadyState(NEGOTIATING);
}

void PeerConnection::ensureStreamChangeScheduled()
{
    if (!m_streamChangeTimer.isActive())
        m_streamChangeTimer.startOneShot(0);
}

void PeerConnection::streamChangeTimerFired(Timer<PeerConnection>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_streamChangeTimer);

    if (!m_pendingAddStreams.isEmpty() && !m_pendingRemoveStreams.isEmpty())
        return;

    MediaStreamDescriptorVector pendingAddStreams;
    MediaStreamDescriptorVector pendingRemoveStreams;

    m_pendingAddStreams.swap(pendingAddStreams);
    m_pendingRemoveStreams.swap(pendingRemoveStreams);

    if (m_peerHandler)
        m_peerHandler->processPendingStreams(pendingAddStreams, pendingRemoveStreams);

    if (!pendingAddStreams.isEmpty())
        changeReadyState(NEGOTIATING);
}

void PeerConnection::scheduleReadyStateChange(ReadyState readyState)
{
    m_pendingReadyStates.append(readyState);
    if (!m_readyStateChangeTimer.isActive())
        m_readyStateChangeTimer.startOneShot(0);
}

void PeerConnection::readyStateChangeTimerFired(Timer<PeerConnection>* timer)
{
    ASSERT_UNUSED(timer, timer == &m_readyStateChangeTimer);

    Vector<ReadyState> pendingReadyStates;
    m_pendingReadyStates.swap(pendingReadyStates);

    for (size_t i = 0; i < pendingReadyStates.size(); i++)
        changeReadyState(pendingReadyStates[i]);
}

void PeerConnection::changeReadyState(ReadyState readyState)
{
    if (readyState == m_readyState)
        return;

    m_readyState = readyState;

    switch (m_readyState) {
    case NEW:
        ASSERT_NOT_REACHED();
        break;
    case NEGOTIATING:
        dispatchEvent(Event::create(eventNames().connectingEvent, false, false));
        break;
    case ACTIVE:
        dispatchEvent(Event::create(eventNames().openEvent, false, false));
        break;
    case CLOSED:
        break;
    }

    dispatchEvent(Event::create(eventNames().statechangeEvent, false, false));
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
