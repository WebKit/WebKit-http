/*
 * Copyright (C) 2010 Nokia Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
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

#include "config.h"
#include "SocketStreamHandle.h"

#include "Logging.h"
#include "NotImplemented.h"
#include "SocketStreamError.h"
#include "SocketStreamHandleClient.h"
#include "SocketStreamHandlePrivate.h"
#include "StorageSessionProvider.h"
#include <wtf/URL.h>

namespace WebCore {

SocketStreamHandlePrivate::SocketStreamHandlePrivate(SocketStreamHandleImpl* streamHandle, const URL& url)
{
    m_streamHandle = streamHandle;
    m_socket = 0;
    bool isSecure = url.protocolIs("wss");

    if (isSecure) {
#ifndef QT_NO_SSL
        m_socket = new QSslSocket(this);
#endif
    } else
        m_socket = new QTcpSocket(this);

    if (!m_socket)
        return;

    initConnections();

    unsigned int port = url.port() ? url.port().value() : (isSecure ? 443 : 80);

    QString host = url.host().toStringWithoutCopying(); // QTFIXME: Convert StringView to QString directly?
    if (isSecure) {
#ifndef QT_NO_SSL
        static_cast<QSslSocket*>(m_socket)->connectToHostEncrypted(host, port);
#endif
    } else
        m_socket->connectToHost(host, port);
}

SocketStreamHandlePrivate::SocketStreamHandlePrivate(SocketStreamHandleImpl* streamHandle, QTcpSocket* socket)
{
    m_streamHandle = streamHandle;
    m_socket = socket;
    initConnections();
}

SocketStreamHandlePrivate::~SocketStreamHandlePrivate()
{
    Q_ASSERT(!(m_socket && m_socket->state() == QAbstractSocket::ConnectedState));
}

void SocketStreamHandlePrivate::initConnections()
{
    connect(m_socket, SIGNAL(connected()), this, SLOT(socketConnected()));
    connect(m_socket, SIGNAL(readyRead()), this, SLOT(socketReadyRead()));
    connect(m_socket, SIGNAL(disconnected()), this, SLOT(socketClosed()));
    connect(m_socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(socketError(QAbstractSocket::SocketError)));
#ifndef QT_NO_SSL
    if (qobject_cast<QSslSocket*>(m_socket))
        connect(m_socket, SIGNAL(sslErrors(const QList<QSslError>&)), this, SLOT(socketSslErrors(const QList<QSslError>&)));
#endif

    // Check for missed signals and call the slots asynchronously to allow a client to be set first.
    if (m_socket->state() >= QAbstractSocket::ConnectedState)
        QMetaObject::invokeMethod(this, "socketConnected", Qt::QueuedConnection);
    if (m_socket->bytesAvailable())
        QMetaObject::invokeMethod(this, "socketReadyRead", Qt::QueuedConnection);
}

void SocketStreamHandlePrivate::socketConnected()
{
    if (m_streamHandle) {
        m_streamHandle->m_state = SocketStreamHandle::Open;
        m_streamHandle->m_client.didOpenSocketStream(*m_streamHandle);
    }
}

void SocketStreamHandlePrivate::socketReadyRead()
{
    if (m_streamHandle) {
        QByteArray data = m_socket->read(m_socket->bytesAvailable());
        m_streamHandle->m_client.didReceiveSocketStreamData(*m_streamHandle, data.constData(), data.size());
    }
}

Optional<size_t> SocketStreamHandlePrivate::send(const uint8_t* data, size_t len)
{
    if (!m_socket || m_socket->state() != QAbstractSocket::ConnectedState)
        return WTF::nullopt;
    size_t sentSize = static_cast<size_t>(m_socket->write(reinterpret_cast<const char*>(data), len));
    QMetaObject::invokeMethod(this, "socketSentData", Qt::QueuedConnection);
    return sentSize;
}

void SocketStreamHandlePrivate::close()
{
    if (m_socket && m_streamHandle && m_streamHandle->m_state == SocketStreamHandle::Connecting) {
        m_socket->abort();
        m_streamHandle->m_client.didCloseSocketStream(*m_streamHandle);
        return;
    }
    if (m_socket && m_socket->state() == QAbstractSocket::ConnectedState)
        m_socket->close();
}

void SocketStreamHandlePrivate::socketSentData()
{
    if (m_streamHandle)
        m_streamHandle->sendPendingData();
}

void SocketStreamHandlePrivate::socketClosed()
{
    QMetaObject::invokeMethod(this, "socketClosedCallback", Qt::QueuedConnection);
}

void SocketStreamHandlePrivate::socketError(QAbstractSocket::SocketError error)
{
    QMetaObject::invokeMethod(this, "socketErrorCallback", Qt::QueuedConnection, Q_ARG(int, error));
}

void SocketStreamHandlePrivate::socketClosedCallback()
{
    if (m_streamHandle) {
        SocketStreamHandleImpl* streamHandle = m_streamHandle;
        m_streamHandle = 0;
        // This following call deletes _this_. Nothing should be after it.
        streamHandle->m_client.didCloseSocketStream(*streamHandle);
    }
}

void SocketStreamHandlePrivate::socketErrorCallback(int error)
{
    // FIXME - in the future, we might not want to treat all errors as fatal.
    if (m_streamHandle) {
        SocketStreamHandleImpl* streamHandle = m_streamHandle;
        m_streamHandle = 0;

        // QTFIXME: Add URL
        streamHandle->m_client.didFailSocketStream(*streamHandle, SocketStreamError(error, String(), m_socket->errorString()));

        // This following call deletes _this_. Nothing should be after it.
        streamHandle->m_client.didCloseSocketStream(*streamHandle);
    }
}

#ifndef QT_NO_SSL
void SocketStreamHandlePrivate::socketSslErrors(const QList<QSslError>& error)
{
    QMetaObject::invokeMethod(this, "socketErrorCallback", Qt::QueuedConnection, Q_ARG(int, error[0].error()));
}
#endif

SocketStreamHandleImpl::SocketStreamHandleImpl(const URL& url, SocketStreamHandleClient& client)
    : SocketStreamHandle(url, client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, &m_client);
    m_p = new SocketStreamHandlePrivate(this, url);
}

SocketStreamHandleImpl::SocketStreamHandleImpl(QTcpSocket* socket, SocketStreamHandleClient& client)
    : SocketStreamHandle(URL(), client)
{
    LOG(Network, "SocketStreamHandle %p new client %p", this, &m_client);
    m_p = new SocketStreamHandlePrivate(this, socket);
    if (socket->isOpen())
        m_state = Open;
}

SocketStreamHandleImpl::~SocketStreamHandleImpl()
{
    LOG(Network, "SocketStreamHandle %p delete", this);
    // QTFIXME check
//    setClient(0);
    delete m_p;
}

Optional<size_t> SocketStreamHandleImpl::platformSendInternal(const uint8_t* data, size_t length)
{
    LOG(Network, "SocketStreamHandle %p platformSend", this);
    return m_p->send(data, length);
}

void SocketStreamHandleImpl::platformClose()
{
    LOG(Network, "SocketStreamHandle %p platformClose", this);
    if (m_p)
        m_p->close();
}

void SocketStreamHandleImpl::didReceiveAuthenticationChallenge(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandleImpl::receivedCredential(const AuthenticationChallenge&, const Credential&)
{
    notImplemented();
}

void SocketStreamHandleImpl::receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&)
{
    notImplemented();
}

void SocketStreamHandleImpl::receivedCancellation(const AuthenticationChallenge&)
{
    notImplemented();
}

} // namespace WebCore

#include "moc_SocketStreamHandlePrivate.cpp"
