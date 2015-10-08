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

#ifndef CompositingManagerProxy_h
#define CompositingManagerProxy_h

#include "Connection.h"
#include "MessageReceiver.h"
#include <WPE/ViewBackend/ViewBackend.h>

namespace IPC {
class Attachment;
}

namespace WKWPE {
class View;
}

namespace WebKit {

class WebPageProxy;

class CompositingManagerProxy final : public IPC::Connection::Client, public WPE::ViewBackend::Client {
public:
    CompositingManagerProxy(WKWPE::View&);

    CompositingManagerProxy(const CompositingManagerProxy&) = delete;
    CompositingManagerProxy& operator=(const CompositingManagerProxy&) = delete;
    CompositingManagerProxy(CompositingManagerProxy&&) = delete;
    CompositingManagerProxy& operator=(CompositingManagerProxy&&) = delete;

private:
    // IPC::MessageReceiver
    virtual void didReceiveMessage(IPC::Connection&, IPC::MessageDecoder&) override;
    virtual void didReceiveSyncMessage(IPC::Connection&, IPC::MessageDecoder&, std::unique_ptr<IPC::MessageEncoder>& replyEncoder) override;

    // IPC::Connection::Client
    void didClose(IPC::Connection&) override { }
    void didReceiveInvalidMessage(IPC::Connection&, IPC::StringReference, IPC::StringReference) override { }
    IPC::ProcessType localProcessType() override { return IPC::ProcessType::UI; }
    IPC::ProcessType remoteProcessType() override { return IPC::ProcessType::Web; }

    void establishConnection(IPC::Attachment);

#if PLATFORM(GBM)
    void commitPrimeBuffer(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t, IPC::Attachment);
    void destroyPrimeBuffer(uint32_t);
#endif

#if PLATFORM(BCM_RPI)
    void createBCMElement(int32_t width, int32_t height, uint32_t& handle);
    void commitBCMBuffer(uint32_t, uint32_t, uint32_t);
#endif

    // WPE::ViewBackend::Client
    void releaseBuffer(uint32_t) override;
    void frameComplete() override;

    WebPageProxy& m_webPageProxy;
    WPE::ViewBackend::ViewBackend& m_viewBackend;

    RefPtr<IPC::Connection> m_connection;
};

} // namespace WebKit

#endif // CompositingManagerProxy_h
