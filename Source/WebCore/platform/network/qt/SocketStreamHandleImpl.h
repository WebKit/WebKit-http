/*
 * Copyright (C) 2010 Nokia Inc. All rights reserved.
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef SocketStreamHandle_h
#define SocketStreamHandle_h

#include "SocketStreamHandleBase.h"

#include <wtf/RefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/StreamBuffer.h>

#if !PLATFORM(QT)
#error This should only be built on Qt
#endif

QT_BEGIN_NAMESPACE
class QTcpSocket;
QT_END_NAMESPACE

namespace WebCore {

    class AuthenticationChallenge;
    class Credential;
    class NetworkingContext;
    class SocketStreamHandleClient;
    class SocketStreamHandlePrivate;

    class SocketStreamHandleImpl final : public SocketStreamHandle {
    public:
        static RefPtr<SocketStreamHandleImpl> create(const URL& url, SocketStreamHandleClient* client, NetworkingContext&) { return adoptRef(*new SocketStreamHandleImpl(url, client)); }
        static RefPtr<SocketStreamHandleImpl> create(QTcpSocket* socket, SocketStreamHandleClient* client) { return adoptRef(*new SocketStreamHandleImpl(socket, client)); }

        ~SocketStreamHandle();

        WEBCORE_EXPORT void platformSend(const uint8_t* data, size_t length, Function<void(bool)>&& completionHandler) final;
        WEBCORE_EXPORT void platformSendHandshake(const uint8_t* data, size_t length, const Optional<CookieRequestHeaderFieldProxy>&, Function<void(bool, bool)>&&) final;
        WEBCORE_EXPORT void platformClose() final;

    private:
        SocketStreamHandle(const URL&, SocketStreamHandleClient*);
        SocketStreamHandle(QTcpSocket*, SocketStreamHandleClient*);

        bool sendPendingData();
        size_t bufferedAmount() final;
        Optional<size_t> platformSendInternal(const uint8_t*, size_t);

        // No authentication for streams per se, but proxy may ask for credentials.
        void didReceiveAuthenticationChallenge(const AuthenticationChallenge&);
        void receivedCredential(const AuthenticationChallenge&, const Credential&);
        void receivedRequestToContinueWithoutCredential(const AuthenticationChallenge&);
        void receivedCancellation(const AuthenticationChallenge&);

        StreamBuffer<uint8_t, 1024 * 1024> m_buffer;
        static const unsigned maxBufferSize = 100 * 1024 * 1024;

        SocketStreamHandlePrivate* m_p;

        friend class SocketStreamHandlePrivate;
    };

}  // namespace WebCore

#endif  // SocketStreamHandle_h
