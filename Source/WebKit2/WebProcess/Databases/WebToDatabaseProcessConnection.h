/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef WebToDatabaseProcessConnection_h
#define WebToDatabaseProcessConnection_h

#include "Connection.h"
#include "MessageSender.h"
#include <wtf/RefCounted.h>

#if ENABLE(INDEXED_DATABASE)

namespace WebKit {

class WebProcessIDBDatabaseBackend;

class WebToDatabaseProcessConnection : public RefCounted<WebToDatabaseProcessConnection>, public CoreIPC::Connection::Client, public CoreIPC::MessageSender {
public:
    static PassRefPtr<WebToDatabaseProcessConnection> create(CoreIPC::Connection::Identifier connectionIdentifier)
    {
        return adoptRef(new WebToDatabaseProcessConnection(connectionIdentifier));
    }
    ~WebToDatabaseProcessConnection();
    
    CoreIPC::Connection* connection() const { return m_connection.get(); }

    void didReceiveNetworkProcessConnectionMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&);

private:
    WebToDatabaseProcessConnection(CoreIPC::Connection::Identifier);

    // CoreIPC::Connection::Client
    virtual void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageDecoder&) OVERRIDE;
    virtual void didClose(CoreIPC::Connection*) OVERRIDE;
    virtual void didReceiveInvalidMessage(CoreIPC::Connection*, CoreIPC::StringReference messageReceiverName, CoreIPC::StringReference messageName) OVERRIDE;

    // CoreIPC::MessageSender
    virtual CoreIPC::Connection* messageSenderConnection() OVERRIDE { return m_connection.get(); }
    virtual uint64_t messageSenderDestinationID() OVERRIDE { return 0; }

    RefPtr<CoreIPC::Connection> m_connection;
};

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
#endif // WebToDatabaseProcessConnection_h
