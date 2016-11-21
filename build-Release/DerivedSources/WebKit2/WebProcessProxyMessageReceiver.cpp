/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "WebProcessProxy.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "SessionState.h"
#if ENABLE(NETSCAPE_PLUGIN_API)
#include "WebCoreArgumentCoders.h"
#endif
#include "WebProcessProxyMessages.h"
#include "WebsiteData.h"
#if ENABLE(NETSCAPE_PLUGIN_API)
#include <WebCore/PluginData.h>
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
#include <wtf/Vector.h>
#endif
#include <wtf/text/WTFString.h>

namespace Messages {

namespace WebProcessProxy {

#if ENABLE(NETSCAPE_PLUGIN_API)

GetPluginProcessConnection::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

GetPluginProcessConnection::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool GetPluginProcessConnection::DelayedReply::send(const IPC::Attachment& connectionHandle, bool supportsAsynchronousInitialization)
{
    ASSERT(m_encoder);
    *m_encoder << connectionHandle;
    *m_encoder << supportsAsynchronousInitialization;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

#endif

GetNetworkProcessConnection::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

GetNetworkProcessConnection::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool GetNetworkProcessConnection::DelayedReply::send(const IPC::Attachment& connectionHandle)
{
    ASSERT(m_encoder);
    *m_encoder << connectionHandle;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

#if ENABLE(DATABASE_PROCESS)

GetDatabaseProcessConnection::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::Encoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

GetDatabaseProcessConnection::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool GetDatabaseProcessConnection::DelayedReply::send(const IPC::Attachment& connectionHandle)
{
    ASSERT(m_encoder);
    *m_encoder << connectionHandle;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

#endif

} // namespace WebProcessProxy

} // namespace Messages

namespace WebKit {

void WebProcessProxy::didReceiveWebProcessProxyMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebProcessProxy::AddBackForwardItem::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::AddBackForwardItem>(decoder, this, &WebProcessProxy::addBackForwardItem);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidDestroyFrame::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidDestroyFrame>(decoder, this, &WebProcessProxy::didDestroyFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidDestroyUserGestureToken::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidDestroyUserGestureToken>(decoder, this, &WebProcessProxy::didDestroyUserGestureToken);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::EnableSuddenTermination::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::EnableSuddenTermination>(decoder, this, &WebProcessProxy::enableSuddenTermination);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DisableSuddenTermination::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DisableSuddenTermination>(decoder, this, &WebProcessProxy::disableSuddenTermination);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidFetchWebsiteData::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidFetchWebsiteData>(decoder, this, &WebProcessProxy::didFetchWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidDeleteWebsiteData::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidDeleteWebsiteData>(decoder, this, &WebProcessProxy::didDeleteWebsiteData);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidDeleteWebsiteDataForOrigins::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidDeleteWebsiteDataForOrigins>(decoder, this, &WebProcessProxy::didDeleteWebsiteDataForOrigins);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::ProcessReadyToSuspend::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::ProcessReadyToSuspend>(decoder, this, &WebProcessProxy::processReadyToSuspend);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidCancelProcessSuspension::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidCancelProcessSuspension>(decoder, this, &WebProcessProxy::didCancelProcessSuspension);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::SetIsHoldingLockedFiles::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::SetIsHoldingLockedFiles>(decoder, this, &WebProcessProxy::setIsHoldingLockedFiles);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::RetainIconForPageURL::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::RetainIconForPageURL>(decoder, this, &WebProcessProxy::retainIconForPageURL);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::ReleaseIconForPageURL::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::ReleaseIconForPageURL>(decoder, this, &WebProcessProxy::releaseIconForPageURL);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessProxy::DidReceiveMainThreadPing::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::DidReceiveMainThreadPing>(decoder, this, &WebProcessProxy::didReceiveMainThreadPing);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebProcessProxy::didReceiveSyncWebProcessProxyMessage(IPC::Connection& connection, IPC::Decoder& decoder, std::unique_ptr<IPC::Encoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebProcessProxy::ShouldTerminate::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::ShouldTerminate>(decoder, *replyEncoder, this, &WebProcessProxy::shouldTerminate);
        return;
    }
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebProcessProxy::GetPlugins::name()) {
        IPC::handleMessage<Messages::WebProcessProxy::GetPlugins>(decoder, *replyEncoder, this, &WebProcessProxy::getPlugins);
        return;
    }
#endif
#if ENABLE(NETSCAPE_PLUGIN_API)
    if (decoder.messageName() == Messages::WebProcessProxy::GetPluginProcessConnection::name()) {
        IPC::handleMessageDelayed<Messages::WebProcessProxy::GetPluginProcessConnection>(connection, decoder, replyEncoder, this, &WebProcessProxy::getPluginProcessConnection);
        return;
    }
#endif
    if (decoder.messageName() == Messages::WebProcessProxy::GetNetworkProcessConnection::name()) {
        IPC::handleMessageDelayed<Messages::WebProcessProxy::GetNetworkProcessConnection>(connection, decoder, replyEncoder, this, &WebProcessProxy::getNetworkProcessConnection);
        return;
    }
#if ENABLE(DATABASE_PROCESS)
    if (decoder.messageName() == Messages::WebProcessProxy::GetDatabaseProcessConnection::name()) {
        IPC::handleMessageDelayed<Messages::WebProcessProxy::GetDatabaseProcessConnection>(connection, decoder, replyEncoder, this, &WebProcessProxy::getDatabaseProcessConnection);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
