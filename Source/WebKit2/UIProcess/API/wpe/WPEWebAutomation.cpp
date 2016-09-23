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
#include "WKAPICast.h"
#include "WPEWebAutomation.h"
#include "WebProcessPool.h"

namespace WKWPE {

WebAutomation::WebAutomation()
    : m_commandStatusCallback(nullptr) {
    m_webAutomationSession.reset(new WebKit::WebAutomationSession());
    if (m_webAutomationSession) {
        m_webAutomationSession->connect((Inspector::FrontendChannel*) this, true);
        m_webAutomationSession->setClient(std::make_unique<AutomationSessionClient>());
    }
}

WebAutomation::~WebAutomation() {
    if (m_webAutomationSession) {
        m_webAutomationSession->disconnect((Inspector::FrontendChannel*) this);	
        m_webAutomationSession.reset();
        m_webAutomationSession = nullptr;
    }
}

WebAutomation* WebAutomation::create() {
    return new WebAutomation();
}

void WebAutomation::setProcessPool(WebKit::WebProcessPool* processPool) 
{
    //Link Process Pool and Automation Session together
    if (m_webAutomationSession)
        processPool->setAutomationSession(m_webAutomationSession.get());
}

void WebAutomation::setSessionIdentifier(const String& sessionIdentifier)
{
    if (m_webAutomationSession)
        m_webAutomationSession->setSessionIdentifier(sessionIdentifier);
}

//void (*s_commandStatusCallback)(WKStringRef rspMsg);

void WebAutomation::sendMessageToTarget(const String& command, void (*commandCallback)(WKStringRef rspMsg)) {
    if (m_webAutomationSession) {
        m_commandStatusCallback = commandCallback;
        m_webAutomationSession->dispatchMessageFromRemote(command);
    }
}

bool WebAutomation::sendMessageToFrontend(const String& rspMsg) {
    if (m_commandStatusCallback) {
        m_commandStatusCallback (WebKit::toAPI(API::String::create(rspMsg).ptr()));
        return true;
    }
    return false;
}

} // WKWPE
