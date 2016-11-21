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

#include "WebAutomationSessionProxy.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "WebAutomationSessionProxyMessages.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebAutomationSessionProxy::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::EvaluateJavaScriptFunction::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::EvaluateJavaScriptFunction>(decoder, this, &WebAutomationSessionProxy::evaluateJavaScriptFunction);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::ResolveChildFrameWithOrdinal::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::ResolveChildFrameWithOrdinal>(decoder, this, &WebAutomationSessionProxy::resolveChildFrameWithOrdinal);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::ResolveChildFrameWithNodeHandle::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::ResolveChildFrameWithNodeHandle>(decoder, this, &WebAutomationSessionProxy::resolveChildFrameWithNodeHandle);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::ResolveChildFrameWithName::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::ResolveChildFrameWithName>(decoder, this, &WebAutomationSessionProxy::resolveChildFrameWithName);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::ResolveParentFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::ResolveParentFrame>(decoder, this, &WebAutomationSessionProxy::resolveParentFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::FocusFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::FocusFrame>(decoder, this, &WebAutomationSessionProxy::focusFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::ComputeElementLayout::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::ComputeElementLayout>(decoder, this, &WebAutomationSessionProxy::computeElementLayout);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::TakeScreenshot::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::TakeScreenshot>(decoder, this, &WebAutomationSessionProxy::takeScreenshot);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::GetCookiesForFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::GetCookiesForFrame>(decoder, this, &WebAutomationSessionProxy::getCookiesForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSessionProxy::DeleteCookie::name()) {
        IPC::handleMessage<Messages::WebAutomationSessionProxy::DeleteCookie>(decoder, this, &WebAutomationSessionProxy::deleteCookie);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
