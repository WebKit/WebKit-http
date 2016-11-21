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

#include "WebInspectorProxy.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "WebInspectorProxyMessages.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebInspectorProxy::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebInspectorProxy::CreateInspectorPage::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::CreateInspectorPage>(decoder, this, &WebInspectorProxy::createInspectorPage);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::FrontendLoaded::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::FrontendLoaded>(decoder, this, &WebInspectorProxy::frontendLoaded);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::DidClose::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::DidClose>(decoder, this, &WebInspectorProxy::didClose);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::BringToFront::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::BringToFront>(decoder, this, &WebInspectorProxy::bringToFront);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::InspectedURLChanged::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::InspectedURLChanged>(decoder, this, &WebInspectorProxy::inspectedURLChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::ElementSelectionChanged::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::ElementSelectionChanged>(decoder, this, &WebInspectorProxy::elementSelectionChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::Save::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::Save>(decoder, this, &WebInspectorProxy::save);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::Append::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::Append>(decoder, this, &WebInspectorProxy::append);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::AttachBottom::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::AttachBottom>(decoder, this, &WebInspectorProxy::attachBottom);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::AttachRight::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::AttachRight>(decoder, this, &WebInspectorProxy::attachRight);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::Detach::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::Detach>(decoder, this, &WebInspectorProxy::detach);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::AttachAvailabilityChanged::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::AttachAvailabilityChanged>(decoder, this, &WebInspectorProxy::attachAvailabilityChanged);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::SetAttachedWindowHeight::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::SetAttachedWindowHeight>(decoder, this, &WebInspectorProxy::setAttachedWindowHeight);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::SetAttachedWindowWidth::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::SetAttachedWindowWidth>(decoder, this, &WebInspectorProxy::setAttachedWindowWidth);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorProxy::StartWindowDrag::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::StartWindowDrag>(decoder, this, &WebInspectorProxy::startWindowDrag);
        return;
    }
#if ENABLE(INSPECTOR_SERVER)
    if (decoder.messageName() == Messages::WebInspectorProxy::SendMessageToRemoteFrontend::name()) {
        IPC::handleMessage<Messages::WebInspectorProxy::SendMessageToRemoteFrontend>(decoder, this, &WebInspectorProxy::sendMessageToRemoteFrontend);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
