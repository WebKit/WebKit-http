/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CompositingManager_h
#define CompositingManager_h

#include "Connection.h"
#include "MessageReceiver.h"
#include <WebCore/PlatformDisplayWPE.h>

namespace WebKit {

class WebPage;

class CompositingManager final : public IPC::Connection::Client, public WebCore::PlatformDisplayWPE::Surface::Client {
    WTF_MAKE_FAST_ALLOCATED;
public:
    class Client {
    public:
        virtual void releaseBuffer(uint32_t) = 0;
        virtual void frameComplete() = 0;
    };

    CompositingManager(Client&);
    virtual ~CompositingManager();

    void establishConnection(WebPage&, WTF::RunLoop&);

    Vector<uint8_t> authenticate();
    uint32_t constructRenderingTarget(uint32_t, uint32_t);
    void commitBuffer(const WebCore::PlatformDisplayWPE::BufferExport&);

    CompositingManager(const CompositingManager&) = delete;
    CompositingManager& operator=(const CompositingManager&) = delete;
    CompositingManager(CompositingManager&&) = delete;
    CompositingManager& operator=(CompositingManager&&) = delete;

private:
    // IPC::MessageReceiver
    virtual void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;

    // IPC::Connection::Client
    void didClose(IPC::Connection&) override { }
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference, IPC::StringReference) override { }
    IPC::ProcessType localProcessType() override { return IPC::ProcessType::Web; }
    IPC::ProcessType remoteProcessType() override { return IPC::ProcessType::UI; }

    // PlatformDisplayWPE::Surface::Client
    void destroyBuffer(uint32_t) override;

    void releaseBuffer(uint32_t);
    void frameComplete();

    Client& m_client;

    RefPtr<IPC::Connection> m_connection;
};

} // namespace WebKit

#endif // CompositingManager
