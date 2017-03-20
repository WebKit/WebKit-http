/*
 * Copyright (C) 2016 TATA ELXSI
 * Copyright (C) 2016 Metrological
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "APIAutomationSessionClient.h"
#include "WPEWebAutomationClient.h"
#include "WebPageProxy.h"
#include "WebProcessPool.h"

#include <WebCore/NotImplemented.h>

namespace WKWPE {

AutomationSessionClient::AutomationSessionClient() {
}

AutomationSessionClient::~AutomationSessionClient() {
}

String AutomationSessionClient::sessionIdentifier() const { 
    return emptyString(); 
}

void AutomationSessionClient::didDisconnectFromRemote(WebKit::WebAutomationSession&) { 
}

WebKit::WebPageProxy* AutomationSessionClient::didRequestNewWindow(WebKit::WebAutomationSession& webAutomationSession) { 

    for (auto& process : webAutomationSession.processPool()->processes()) {
        for (auto* page : process->pages()) {
            ASSERT(page);
            if (!page->isControlledByAutomation())
                continue;
            return page;
        }
    }
    return nullptr; 
}

bool AutomationSessionClient::isShowingJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) { 
    return false; 
}

void AutomationSessionClient::dismissCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) { 
    notImplemented();
}

void AutomationSessionClient::acceptCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) { 
    notImplemented();
}

String AutomationSessionClient::messageOfCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) { 
    return String(); 
}

void AutomationSessionClient::setUserInputForCurrentJavaScriptPromptOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&, const String&) { 
    notImplemented();
}

} // namespace WKWPE
