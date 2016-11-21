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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "WebProcessConnection.h"

#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "PluginCreationParameters.h"
#include "WebProcessConnectionMessages.h"

namespace Messages {

namespace WebProcessConnection {

CreatePlugin::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::MessageEncoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

CreatePlugin::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool CreatePlugin::DelayedReply::send(bool creationResult, bool wantsWheelEvents, uint32_t remoteLayerClientID)
{
    ASSERT(m_encoder);
    *m_encoder << creationResult;
    *m_encoder << wantsWheelEvents;
    *m_encoder << remoteLayerClientID;
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

DestroyPlugin::DelayedReply::DelayedReply(PassRefPtr<IPC::Connection> connection, std::unique_ptr<IPC::MessageEncoder> encoder)
    : m_connection(connection)
    , m_encoder(WTFMove(encoder))
{
}

DestroyPlugin::DelayedReply::~DelayedReply()
{
    ASSERT(!m_connection);
}

bool DestroyPlugin::DelayedReply::send()
{
    ASSERT(m_encoder);
    bool _result = m_connection->sendSyncReply(WTFMove(m_encoder));
    m_connection = nullptr;
    return _result;
}

} // namespace WebProcessConnection

} // namespace Messages

namespace WebKit {

void WebProcessConnection::didReceiveWebProcessConnectionMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebProcessConnection::CreatePluginAsynchronously::name()) {
        IPC::handleMessage<Messages::WebProcessConnection::CreatePluginAsynchronously>(decoder, this, &WebProcessConnection::createPluginAsynchronously);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebProcessConnection::didReceiveSyncWebProcessConnectionMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder, std::unique_ptr<IPC::MessageEncoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebProcessConnection::CreatePlugin::name()) {
        IPC::handleMessageDelayed<Messages::WebProcessConnection::CreatePlugin>(connection, decoder, replyEncoder, this, &WebProcessConnection::createPlugin);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessConnection::DestroyPlugin::name()) {
        IPC::handleMessageDelayed<Messages::WebProcessConnection::DestroyPlugin>(connection, decoder, replyEncoder, this, &WebProcessConnection::destroyPlugin);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
