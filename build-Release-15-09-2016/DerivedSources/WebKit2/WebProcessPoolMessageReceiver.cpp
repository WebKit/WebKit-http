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

#include "WebProcessPool.h"

#include "ArgumentCoders.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "StatisticsData.h"
#include "UserData.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcessPoolMessages.h"
#include <WebCore/SessionID.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebProcessPool::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebProcessPool::HandleMessage::name()) {
        IPC::handleMessage<Messages::WebProcessPool::HandleMessage>(connection, decoder, this, &WebProcessPool::handleMessage);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessPool::DidGetStatistics::name()) {
        IPC::handleMessage<Messages::WebProcessPool::DidGetStatistics>(decoder, this, &WebProcessPool::didGetStatistics);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessPool::AddPlugInAutoStartOriginHash::name()) {
        IPC::handleMessage<Messages::WebProcessPool::AddPlugInAutoStartOriginHash>(decoder, this, &WebProcessPool::addPlugInAutoStartOriginHash);
        return;
    }
    if (decoder.messageName() == Messages::WebProcessPool::PlugInDidReceiveUserInteraction::name()) {
        IPC::handleMessage<Messages::WebProcessPool::PlugInDidReceiveUserInteraction>(decoder, this, &WebProcessPool::plugInDidReceiveUserInteraction);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

void WebProcessPool::didReceiveSyncMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder, std::unique_ptr<IPC::MessageEncoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::WebProcessPool::HandleSynchronousMessage::name()) {
        IPC::handleMessage<Messages::WebProcessPool::HandleSynchronousMessage>(connection, decoder, *replyEncoder, this, &WebProcessPool::handleSynchronousMessage);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
