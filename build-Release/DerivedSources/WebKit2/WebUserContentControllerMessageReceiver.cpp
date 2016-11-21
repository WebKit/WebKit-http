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

#include "WebUserContentController.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#if ENABLE(CONTENT_EXTENSIONS)
#include "WebCompiledContentExtensionData.h"
#endif
#include "WebUserContentControllerDataTypes.h"
#include "WebUserContentControllerMessages.h"
#include <utility>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebUserContentController::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebUserContentController::AddUserContentWorlds::name()) {
        IPC::handleMessage<Messages::WebUserContentController::AddUserContentWorlds>(decoder, this, &WebUserContentController::addUserContentWorlds);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveUserContentWorlds::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveUserContentWorlds>(decoder, this, &WebUserContentController::removeUserContentWorlds);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::AddUserScripts::name()) {
        IPC::handleMessage<Messages::WebUserContentController::AddUserScripts>(decoder, this, &WebUserContentController::addUserScripts);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveUserScript::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveUserScript>(decoder, this, &WebUserContentController::removeUserScript);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveAllUserScripts::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveAllUserScripts>(decoder, this, &WebUserContentController::removeAllUserScripts);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::AddUserStyleSheets::name()) {
        IPC::handleMessage<Messages::WebUserContentController::AddUserStyleSheets>(decoder, this, &WebUserContentController::addUserStyleSheets);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveUserStyleSheet::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveUserStyleSheet>(decoder, this, &WebUserContentController::removeUserStyleSheet);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveAllUserStyleSheets::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveAllUserStyleSheets>(decoder, this, &WebUserContentController::removeAllUserStyleSheets);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::AddUserScriptMessageHandlers::name()) {
        IPC::handleMessage<Messages::WebUserContentController::AddUserScriptMessageHandlers>(decoder, this, &WebUserContentController::addUserScriptMessageHandlers);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveUserScriptMessageHandler::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveUserScriptMessageHandler>(decoder, this, &WebUserContentController::removeUserScriptMessageHandler);
        return;
    }
    if (decoder.messageName() == Messages::WebUserContentController::RemoveAllUserScriptMessageHandlers::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveAllUserScriptMessageHandlers>(decoder, this, &WebUserContentController::removeAllUserScriptMessageHandlers);
        return;
    }
#if ENABLE(CONTENT_EXTENSIONS)
    if (decoder.messageName() == Messages::WebUserContentController::AddUserContentExtensions::name()) {
        IPC::handleMessage<Messages::WebUserContentController::AddUserContentExtensions>(decoder, this, &WebUserContentController::addUserContentExtensions);
        return;
    }
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    if (decoder.messageName() == Messages::WebUserContentController::RemoveUserContentExtension::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveUserContentExtension>(decoder, this, &WebUserContentController::removeUserContentExtension);
        return;
    }
#endif
#if ENABLE(CONTENT_EXTENSIONS)
    if (decoder.messageName() == Messages::WebUserContentController::RemoveAllUserContentExtensions::name()) {
        IPC::handleMessage<Messages::WebUserContentController::RemoveAllUserContentExtensions>(decoder, this, &WebUserContentController::removeAllUserContentExtensions);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
