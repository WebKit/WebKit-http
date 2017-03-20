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

#ifndef WebAutomationClientWPE_h
#define WebAutomationClientWPE_h

#include "WebAutomationSession.h"
namespace WKWPE {

class AutomationSessionClient  final : public API::AutomationSessionClient {
public:

    // From API::AutomationSessionClient
    AutomationSessionClient();
    virtual ~AutomationSessionClient();

    String sessionIdentifier() const override;
    WebKit::WebPageProxy* didRequestNewWindow(WebKit::WebAutomationSession&) override;
    void didDisconnectFromRemote(WebKit::WebAutomationSession&) override;

    bool isShowingJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) override;
    void dismissCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) override;
    void acceptCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) override;
    String messageOfCurrentJavaScriptDialogOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&) override;
    void setUserInputForCurrentJavaScriptPromptOnPage(WebKit::WebAutomationSession&, WebKit::WebPageProxy&, const String&) override;

private:
    struct {
        bool didRequestNewWindow : 1;
        bool didDisconnectFromRemote : 1;
        bool isShowingJavaScriptDialogOnPage : 1;
        bool dismissCurrentJavaScriptDialogOnPage : 1;
        bool acceptCurrentJavaScriptDialogOnPage : 1;
        bool messageOfCurrentJavaScriptDialogOnPage : 1;
        bool setUserInputForCurrentJavaScriptPromptOnPage : 1;
    } m_delegateMethods;
};

} // namespace WKWPE

#endif // WebAutomationClientWEP_h
