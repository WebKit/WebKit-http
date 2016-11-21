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

#include "WebAutomationSession.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "ShareableBitmap.h"
#include "WebAutomationSessionMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/Cookie.h>
#include <WebCore/IntRect.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebAutomationSession::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebAutomationSession::DidEvaluateJavaScriptFunction::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidEvaluateJavaScriptFunction>(decoder, this, &WebAutomationSession::didEvaluateJavaScriptFunction);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidResolveChildFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidResolveChildFrame>(decoder, this, &WebAutomationSession::didResolveChildFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidResolveParentFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidResolveParentFrame>(decoder, this, &WebAutomationSession::didResolveParentFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidComputeElementLayout::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidComputeElementLayout>(decoder, this, &WebAutomationSession::didComputeElementLayout);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidTakeScreenshot::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidTakeScreenshot>(decoder, this, &WebAutomationSession::didTakeScreenshot);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidGetCookiesForFrame::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidGetCookiesForFrame>(decoder, this, &WebAutomationSession::didGetCookiesForFrame);
        return;
    }
    if (decoder.messageName() == Messages::WebAutomationSession::DidDeleteCookie::name()) {
        IPC::handleMessage<Messages::WebAutomationSession::DidDeleteCookie>(decoder, this, &WebAutomationSession::didDeleteCookie);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
