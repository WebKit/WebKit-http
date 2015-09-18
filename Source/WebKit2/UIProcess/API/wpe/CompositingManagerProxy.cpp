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
#include "CompositingManagerProxy.h"

#include "Attachment.h"
#include "CompositingManagerMessages.h"
#include "CompositingManagerProxyMessages.h"
#include "DrawingAreaMessages.h"
#include "WPEView.h"
#include "WebProcessProxy.h"

namespace WebKit {

CompositingManagerProxy::CompositingManagerProxy(WKWPE::View& view)
    : m_webPageProxy(view.page())
    , m_viewBackend(view.viewBackend())
{
    m_webPageProxy.process().addMessageReceiver(Messages::CompositingManagerProxy::messageReceiverName(), m_webPageProxy.pageID(), *this);
    m_viewBackend.setClient(this);
}

void CompositingManagerProxy::establishConnection(IPC::Attachment encodedConnectionIdentifier)
{
    IPC::Connection::Identifier connectionIdentifier(encodedConnectionIdentifier.releaseFileDescriptor());
    m_connection = IPC::Connection::createClientConnection(connectionIdentifier, *this, RunLoop::main());
    m_connection->open();
}

#if PLATFORM(GBM)
void CompositingManagerProxy::commitPrimeBuffer(uint32_t handle, uint32_t width, uint32_t height, uint32_t stride, uint32_t format, IPC::Attachment fd)
{
    m_viewBackend.commitPrimeBuffer(fd.fileDescriptor(), handle, width, height, stride, format);
}

void CompositingManagerProxy::destroyPrimeBuffer(uint32_t handle)
{
    m_viewBackend.destroyPrimeBuffer(handle);
}
#endif

#if PLATFORM(BCM_RPI)
void CompositingManagerProxy::commitBCMBuffer(uint32_t handle1, uint32_t handle2, uint32_t width, uint32_t height, uint32_t format)
{
    m_viewBackend.commitBCMBuffer(handle1, handle2, width, height, format);
}
#endif

void CompositingManagerProxy::releaseBuffer(uint32_t handle)
{
    m_connection->send(Messages::CompositingManager::ReleaseBuffer(handle), 0);
}

void CompositingManagerProxy::frameComplete()
{
    m_connection->send(Messages::CompositingManager::FrameComplete(), 0);
}

} // namespace WebKit
