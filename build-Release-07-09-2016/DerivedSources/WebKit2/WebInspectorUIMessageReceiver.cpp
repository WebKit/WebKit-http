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

#include "WebInspectorUI.h"

#include "ArgumentCoders.h"
#include "Attachment.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "WebInspectorUIMessages.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebInspectorUI::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebInspectorUI::EstablishConnection::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::EstablishConnection>(decoder, this, &WebInspectorUI::establishConnection);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::AttachedBottom::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::AttachedBottom>(decoder, this, &WebInspectorUI::attachedBottom);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::AttachedRight::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::AttachedRight>(decoder, this, &WebInspectorUI::attachedRight);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::Detached::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::Detached>(decoder, this, &WebInspectorUI::detached);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::SetDockingUnavailable::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::SetDockingUnavailable>(decoder, this, &WebInspectorUI::setDockingUnavailable);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::SetIsVisible::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::SetIsVisible>(decoder, this, &WebInspectorUI::setIsVisible);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::ShowConsole::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::ShowConsole>(decoder, this, &WebInspectorUI::showConsole);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::ShowResources::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::ShowResources>(decoder, this, &WebInspectorUI::showResources);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::ShowTimelines::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::ShowTimelines>(decoder, this, &WebInspectorUI::showTimelines);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::ShowMainResourceForFrame::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::ShowMainResourceForFrame>(decoder, this, &WebInspectorUI::showMainResourceForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::StartPageProfiling::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::StartPageProfiling>(decoder, this, &WebInspectorUI::startPageProfiling);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::StopPageProfiling::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::StopPageProfiling>(decoder, this, &WebInspectorUI::stopPageProfiling);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::StartElementSelection::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::StartElementSelection>(decoder, this, &WebInspectorUI::startElementSelection);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::StopElementSelection::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::StopElementSelection>(decoder, this, &WebInspectorUI::stopElementSelection);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::DidSave::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::DidSave>(decoder, this, &WebInspectorUI::didSave);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::DidAppend::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::DidAppend>(decoder, this, &WebInspectorUI::didAppend);
        return;
    }
    if (decoder.messageName() == Messages::WebInspectorUI::SendMessageToFrontend::name()) {
        IPC::handleMessage<Messages::WebInspectorUI::SendMessageToFrontend>(decoder, this, &WebInspectorUI::sendMessageToFrontend);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
