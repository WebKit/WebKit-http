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

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCPeerConnection.h"

#include "ArrayValue.h"
#include "Event.h"
#include "ExceptionCode.h"
#include "MediaConstraintsImpl.h"
#include "MediaStreamEvent.h"
#include "RTCConfiguration.h"
#include "RTCErrorCallback.h"
#include "RTCIceCandidate.h"
#include "RTCIceCandidateDescriptor.h"
#include "RTCIceCandidateEvent.h"
#include "RTCSessionDescription.h"
#include "RTCSessionDescriptionCallback.h"
#include "RTCSessionDescriptionDescriptor.h"
#include "RTCSessionDescriptionRequestImpl.h"
#include "RTCVoidRequestImpl.h"
#include "ScriptExecutionContext.h"
#include "VoidCallback.h"

namespace WebCore {

PassRefPtr<RTCConfiguration> RTCPeerConnection::parseConfiguration(const Dictionary& configuration, ExceptionCode& ec)
{
    if (configuration.isUndefinedOrNull())
        return 0;

    ArrayValue iceServers;
    bool ok = configuration.get("iceServers", iceServers);
    if (!ok || iceServers.isUndefinedOrNull()) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }

    size_t numberOfServers;
    ok = iceServers.length(numberOfServers);
    if (!ok) {
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }

    RefPtr<RTCConfiguration> rtcConfiguration = RTCConfiguration::create();

    for (size_t i = 0; i < numberOfServers; ++i) {
        Dictionary iceServer;
        ok = iceServers.get(i, iceServer);
        if (!ok) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }

        String urlString, credential;
        ok = iceServer.get("url", urlString);
        if (!ok) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }
        KURL url(KURL(), urlString);
        if (!url.isValid() || !(url.protocolIs("turn") || url.protocolIs("stun"))) {
            ec = TYPE_MISMATCH_ERR;
            return 0;
        }

        iceServer.get("credential", credential);

        rtcConfiguration->appendServer(RTCIceServer::create(url, credential));
    }

    return rtcConfiguration.release();
}

PassRefPtr<RTCPeerConnection> RTCPeerConnection::create(ScriptExecutionContext* context, const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    RefPtr<RTCConfiguration> configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return 0;

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return 0;

    RefPtr<RTCPeerConnection> peerConnection = adoptRef(new RTCPeerConnection(context, configuration.release(), constraints.release(), ec));
    peerConnection->suspendIfNeeded();
    if (ec)
        return 0;

    return peerConnection.release();
}

RTCPeerConnection::RTCPeerConnection(ScriptExecutionContext* context, PassRefPtr<RTCConfiguration> configuration, PassRefPtr<MediaConstraints> constraints, ExceptionCode& ec)
    : ActiveDOMObject(context, this)
    , m_readyState(ReadyStateNew)
    , m_iceState(IceStateClosed)
    , m_localStreams(MediaStreamList::create())
    , m_remoteStreams(MediaStreamList::create())
{
    m_peerHandler = RTCPeerConnectionHandler::create(this);
    if (!m_peerHandler || !m_peerHandler->initialize(configuration, constraints))
        ec = NOT_SUPPORTED_ERR;
}

RTCPeerConnection::~RTCPeerConnection()
{
}

void RTCPeerConnection::createOffer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createOffer(request.release(), constraints);
}

void RTCPeerConnection::createAnswer(PassRefPtr<RTCSessionDescriptionCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!successCallback) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    RefPtr<RTCSessionDescriptionRequestImpl> request = RTCSessionDescriptionRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->createAnswer(request.release(), constraints.release());
}

void RTCPeerConnection::setLocalDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setLocalDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::localDescription(ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->localDescription();
    if (!descriptor)
        return 0;

    RefPtr<RTCSessionDescription> sessionDescription = RTCSessionDescription::create(descriptor.release());
    return sessionDescription.release();
}

void RTCPeerConnection::setRemoteDescription(PassRefPtr<RTCSessionDescription> prpSessionDescription, PassRefPtr<VoidCallback> successCallback, PassRefPtr<RTCErrorCallback> errorCallback, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCSessionDescription> sessionDescription = prpSessionDescription;
    if (!sessionDescription) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<RTCVoidRequestImpl> request = RTCVoidRequestImpl::create(scriptExecutionContext(), successCallback, errorCallback);
    m_peerHandler->setRemoteDescription(request.release(), sessionDescription->descriptor());
}

PassRefPtr<RTCSessionDescription> RTCPeerConnection::remoteDescription(ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return 0;
    }

    RefPtr<RTCSessionDescriptionDescriptor> descriptor = m_peerHandler->remoteDescription();
    if (!descriptor)
        return 0;

    RefPtr<RTCSessionDescription> desc = RTCSessionDescription::create(descriptor.release());
    return desc.release();
}

void RTCPeerConnection::updateIce(const Dictionary& rtcConfiguration, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<RTCConfiguration> configuration = parseConfiguration(rtcConfiguration, ec);
    if (ec)
        return;

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    bool valid = m_peerHandler->updateIce(configuration, constraints);
    if (!valid)
        ec = SYNTAX_ERR;
}

void RTCPeerConnection::addIceCandidate(RTCIceCandidate* iceCandidate, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    if (!iceCandidate) {
        ec = TYPE_MISMATCH_ERR;
        return;
    }

    bool valid = m_peerHandler->addIceCandidate(iceCandidate->descriptor());
    if (!valid)
        ec = SYNTAX_ERR;
}

String RTCPeerConnection::readyState() const
{
    switch (m_readyState) {
    case ReadyStateNew:
        return ASCIILiteral("new");
    case ReadyStateOpening:
        return ASCIILiteral("opening");
    case ReadyStateActive:
        return ASCIILiteral("active");
    case ReadyStateClosing:
        return ASCIILiteral("closing");
    case ReadyStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return ASCIILiteral("");
}

String RTCPeerConnection::iceState() const
{
    switch (m_iceState) {
    case IceStateNew:
        return ASCIILiteral("new");
    case IceStateGathering:
        return ASCIILiteral("gathering");
    case IceStateWaiting:
        return ASCIILiteral("waiting");
    case IceStateChecking:
        return ASCIILiteral("checking");
    case IceStateConnected:
        return ASCIILiteral("connected");
    case IceStateCompleted:
        return ASCIILiteral("completed");
    case IceStateFailed:
        return ASCIILiteral("failed");
    case IceStateClosed:
        return ASCIILiteral("closed");
    }

    ASSERT_NOT_REACHED();
    return ASCIILiteral("");
}

void RTCPeerConnection::addStream(PassRefPtr<MediaStream> prpStream, const Dictionary& mediaConstraints, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    RefPtr<MediaStream> stream = prpStream;
    if (!stream) {
        ec =  TYPE_MISMATCH_ERR;
        return;
    }

    RefPtr<MediaConstraints> constraints = MediaConstraintsImpl::create(mediaConstraints, ec);
    if (ec)
        return;

    if (m_localStreams->contains(stream.get()))
        return;

    m_localStreams->append(stream);

    bool valid = m_peerHandler->addStream(stream->descriptor(), constraints);
    if (!valid)
        ec = SYNTAX_ERR;
}

void RTCPeerConnection::removeStream(MediaStream* stream, ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosed) {
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

    m_peerHandler->removeStream(stream->descriptor());
}

MediaStreamList* RTCPeerConnection::localStreams() const
{
    return m_localStreams.get();
}

MediaStreamList* RTCPeerConnection::remoteStreams() const
{
    return m_remoteStreams.get();
}

void RTCPeerConnection::close(ExceptionCode& ec)
{
    if (m_readyState == ReadyStateClosing || m_readyState == ReadyStateClosed) {
        ec = INVALID_STATE_ERR;
        return;
    }

    changeIceState(IceStateClosed);
    changeReadyState(ReadyStateClosed);
    stop();
}

void RTCPeerConnection::negotiationNeeded()
{
    dispatchEvent(Event::create(eventNames().negotationneededEvent, false, false));
}

void RTCPeerConnection::didGenerateIceCandidate(PassRefPtr<RTCIceCandidateDescriptor> iceCandidateDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    if (!iceCandidateDescriptor)
        dispatchEvent(RTCIceCandidateEvent::create(false, false, 0));
    else {
        RefPtr<RTCIceCandidate> iceCandidate = RTCIceCandidate::create(iceCandidateDescriptor);
        dispatchEvent(RTCIceCandidateEvent::create(false, false, iceCandidate.release()));
    }
}

void RTCPeerConnection::didChangeReadyState(ReadyState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeReadyState(newState);
}

void RTCPeerConnection::didChangeIceState(IceState newState)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    changeIceState(newState);
}

void RTCPeerConnection::didAddRemoteStream(PassRefPtr<MediaStreamDescriptor> streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());

    if (m_readyState == ReadyStateClosed)
        return;

    RefPtr<MediaStream> stream = MediaStream::create(scriptExecutionContext(), streamDescriptor);
    m_remoteStreams->append(stream);

    dispatchEvent(MediaStreamEvent::create(eventNames().addstreamEvent, false, false, stream.release()));
}

void RTCPeerConnection::didRemoveRemoteStream(MediaStreamDescriptor* streamDescriptor)
{
    ASSERT(scriptExecutionContext()->isContextThread());
    ASSERT(streamDescriptor->owner());

    RefPtr<MediaStream> stream = static_cast<MediaStream*>(streamDescriptor->owner());
    stream->streamEnded();

    if (m_readyState == ReadyStateClosed)
        return;

    ASSERT(m_remoteStreams->contains(stream.get()));
    m_remoteStreams->remove(stream.get());

    dispatchEvent(MediaStreamEvent::create(eventNames().removestreamEvent, false, false, stream.release()));
}

const AtomicString& RTCPeerConnection::interfaceName() const
{
    return eventNames().interfaceForRTCPeerConnection;
}

ScriptExecutionContext* RTCPeerConnection::scriptExecutionContext() const
{
    return ActiveDOMObject::scriptExecutionContext();
}

void RTCPeerConnection::stop()
{
    m_iceState = IceStateClosed;
    m_readyState = ReadyStateClosed;

    if (m_peerHandler) {
        m_peerHandler->stop();
        m_peerHandler.clear();
    }
}

EventTargetData* RTCPeerConnection::eventTargetData()
{
    return &m_eventTargetData;
}

EventTargetData* RTCPeerConnection::ensureEventTargetData()
{
    return &m_eventTargetData;
}

void RTCPeerConnection::changeReadyState(ReadyState readyState)
{
    if (readyState == m_readyState || m_readyState == ReadyStateClosed)
        return;

    m_readyState = readyState;

    switch (m_readyState) {
    case ReadyStateOpening:
        break;
    case ReadyStateActive:
        dispatchEvent(Event::create(eventNames().openEvent, false, false));
        break;
    case ReadyStateClosing:
    case ReadyStateClosed:
        break;
    case ReadyStateNew:
        ASSERT_NOT_REACHED();
        break;
    }

    dispatchEvent(Event::create(eventNames().statechangeEvent, false, false));
}

void RTCPeerConnection::changeIceState(IceState iceState)
{
    if (iceState == m_iceState || m_readyState == ReadyStateClosed)
        return;

    m_iceState = iceState;
    dispatchEvent(Event::create(eventNames().icechangeEvent, false, false));
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
