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

#include "config.h"
#include "CompositingManager.h"

#include "CompositingManagerMessages.h"
#include "CompositingManagerProxyMessages.h"
#include "DataReference.h"
#include "WebPage.h"
#include "WebProcess.h"

namespace WebKit {

CompositingManager::CompositingManager(Client& client)
    : m_client(client)
{
}

CompositingManager::~CompositingManager() = default;

void CompositingManager::establishConnection(WebPage& webPage, WTF::RunLoop& runLoop)
{
    IPC::Connection::SocketPair socketPair = IPC::Connection::createPlatformConnection();
    IPC::Connection::Identifier connectionIdentifier(socketPair.server);
    IPC::Attachment connectionClientPort(socketPair.client);

    m_connection = IPC::Connection::createServerConnection(connectionIdentifier, *this, runLoop);
    m_connection->open();
    webPage.sendSync(Messages::CompositingManagerProxy::EstablishConnection(connectionClientPort),
        Messages::CompositingManagerProxy::EstablishConnection::Reply(), webPage.pageID());
}

Vector<uint8_t> CompositingManager::authenticate()
{
    IPC::DataReference data;
    m_connection->sendSync(Messages::CompositingManagerProxy::Authenticate(),
        Messages::CompositingManagerProxy::Authenticate::Reply(data), 0);
    return data.vector();
}

uint32_t CompositingManager::constructRenderingTarget(uint32_t width, uint32_t height)
{
    uint32_t handle = 0;
    m_connection->sendSync(Messages::CompositingManagerProxy::ConstructRenderingTarget(width, height),
        Messages::CompositingManagerProxy::ConstructRenderingTarget::Reply(handle), 0);
    return handle;
}

void CompositingManager::commitBuffer(const WebCore::PlatformDisplayWPE::BufferExport& bufferExport)
{
    m_connection->send(Messages::CompositingManagerProxy::CommitBuffer(
        IPC::Attachment(std::get<0>(bufferExport)),
        IPC::DataReference(std::get<1>(bufferExport), std::get<2>(bufferExport))), 0);
}

void CompositingManager::destroyBuffer(uint32_t handle)
{
    m_connection->send(Messages::CompositingManagerProxy::DestroyBuffer(handle), 0);
}

void CompositingManager::releaseBuffer(uint32_t handle)
{
    m_client.releaseBuffer(handle);
}

void CompositingManager::frameComplete()
{
    m_client.frameComplete();
}

} // namespace WebKit
