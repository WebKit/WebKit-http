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

#include "NPObjectMessageReceiver.h"

#include "ArgumentCoders.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "NPIdentifierData.h"
#include "NPObjectMessageReceiverMessages.h"
#include "NPVariantData.h"
#include <wtf/Vector.h>

namespace WebKit {


void NPObjectMessageReceiver::didReceiveSyncNPObjectMessageReceiverMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder, std::unique_ptr<IPC::MessageEncoder>& replyEncoder)
{
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::Deallocate::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::Deallocate>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::deallocate);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::HasMethod::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::HasMethod>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::hasMethod);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::Invoke::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::Invoke>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::invoke);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::InvokeDefault::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::InvokeDefault>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::invokeDefault);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::HasProperty::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::HasProperty>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::hasProperty);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::GetProperty::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::GetProperty>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::getProperty);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::SetProperty::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::SetProperty>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::setProperty);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::RemoveProperty::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::RemoveProperty>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::removeProperty);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::Enumerate::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::Enumerate>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::enumerate);
        return;
    }
    if (decoder.messageName() == Messages::NPObjectMessageReceiver::Construct::name()) {
        IPC::handleMessage<Messages::NPObjectMessageReceiver::Construct>(decoder, *replyEncoder, this, &NPObjectMessageReceiver::construct);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    UNUSED_PARAM(replyEncoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit

#endif // ENABLE(NETSCAPE_PLUGIN_API)
