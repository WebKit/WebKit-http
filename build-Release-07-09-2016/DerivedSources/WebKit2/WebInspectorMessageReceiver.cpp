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

#include "WebInspector.h"

#include "ArgumentCoders.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "WebInspectorMessages.h"
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebInspector::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebInspector::Show::name()) {
        IPC::handleMessage<Messages::WebInspector::Show>(decoder, this, &WebInspector::show);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::Close::name()) {
        IPC::handleMessage<Messages::WebInspector::Close>(decoder, this, &WebInspector::close);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::SetAttached::name()) {
        IPC::handleMessage<Messages::WebInspector::SetAttached>(decoder, this, &WebInspector::setAttached);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::ShowConsole::name()) {
        IPC::handleMessage<Messages::WebInspector::ShowConsole>(decoder, this, &WebInspector::showConsole);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::ShowResources::name()) {
        IPC::handleMessage<Messages::WebInspector::ShowResources>(decoder, this, &WebInspector::showResources);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::ShowTimelines::name()) {
        IPC::handleMessage<Messages::WebInspector::ShowTimelines>(decoder, this, &WebInspector::showTimelines);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::ShowMainResourceForFrame::name()) {
        IPC::handleMessage<Messages::WebInspector::ShowMainResourceForFrame>(decoder, this, &WebInspector::showMainResourceForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::OpenInNewTab::name()) {
        IPC::handleMessage<Messages::WebInspector::OpenInNewTab>(decoder, this, &WebInspector::openInNewTab);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::StartPageProfiling::name()) {
        IPC::handleMessage<Messages::WebInspector::StartPageProfiling>(decoder, this, &WebInspector::startPageProfiling);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::StopPageProfiling::name()) {
        IPC::handleMessage<Messages::WebInspector::StopPageProfiling>(decoder, this, &WebInspector::stopPageProfiling);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::StartElementSelection::name()) {
        IPC::handleMessage<Messages::WebInspector::StartElementSelection>(decoder, this, &WebInspector::startElementSelection);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::StopElementSelection::name()) {
        IPC::handleMessage<Messages::WebInspector::StopElementSelection>(decoder, this, &WebInspector::stopElementSelection);
        return;
    }
    if (decoder.messageName() == Messages::WebInspector::SendMessageToBackend::name()) {
        IPC::handleMessage<Messages::WebInspector::SendMessageToBackend>(decoder, this, &WebInspector::sendMessageToBackend);
        return;
    }
#if ENABLE(INSPECTOR_SERVER)
    if (decoder.messageName() == Messages::WebInspector::RemoteFrontendConnected::name()) {
        IPC::handleMessage<Messages::WebInspector::RemoteFrontendConnected>(decoder, this, &WebInspector::remoteFrontendConnected);
        return;
    }
#endif
#if ENABLE(INSPECTOR_SERVER)
    if (decoder.messageName() == Messages::WebInspector::RemoteFrontendDisconnected::name()) {
        IPC::handleMessage<Messages::WebInspector::RemoteFrontendDisconnected>(decoder, this, &WebInspector::remoteFrontendDisconnected);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
