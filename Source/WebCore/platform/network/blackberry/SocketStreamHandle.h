/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (C) 2010 Research In Motion Limited. All rights reserved.
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

#include <network/FilterStream.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class AuthenticationChallenge;
class Credential;
class SocketStreamHandleClient;

class SocketStreamHandle : public RefCounted<SocketStreamHandle>, public BlackBerry::Platform::FilterStream, public SocketStreamHandleBase {
public:
    static PassRefPtr<SocketStreamHandle> create(const String& groupName, const KURL& url, SocketStreamHandleClient* client) { return adoptRef(new SocketStreamHandle(groupName, url, client)); }

    virtual ~SocketStreamHandle();

    // FilterStream interface
    virtual void notifyStatusReceived(int status, const char* message);
    virtual void notifyDataReceived(BlackBerry::Platform::NetworkBuffer*);
    virtual void notifyReadyToSendData();
    virtual void notifyClose(int status);
    virtual int status() const { return m_status; }

protected:
    virtual int platformSend(const char* data, int length);
    virtual void platformClose();

private:
    SocketStreamHandle(const String& groupName, const KURL&, SocketStreamHandleClient*);

    OwnPtr<BlackBerry::Platform::FilterStream> m_socketStream;
    int m_status;
};

} // namespace WebCore

#endif // SocketStreamHandle_h
