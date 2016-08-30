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

WebAutomation::WebAutomation () {
    m_webAutomationSession = new WebKit::WebAutomationSession();
    m_webAutomationSession->connect((Inspector::FrontendChannel*) this, true);
    m_connected = true;
}

WebAutomation::~WebAutomation () {
    if (m_connected) {
        m_webAutomationSession->disconnect((Inspector::FrontendChannel*) this);	
        m_connected = false;	
    }
    delete m_webAutomationSession;
}

WebAutomation* WebAutomation::create () {
    return new WebAutomation();
}

void WebAutomation::setClient()
{
    m_webAutomationSessionClient = new AutomationSessionClient();
    m_webAutomationSession->setClient((std::unique_ptr<API::AutomationSessionClient>) m_webAutomationSessionClient);    
}

void WebAutomation::setProcessPool(WebKit::WebProcessPool* processPool) 
{
    //Link Process Pool and Automation Session together
    processPool->setAutomationSession(m_webAutomationSession);
}

void WebAutomation::setSessionIdentifier(const String& sessionIdentifier)
{
    m_webAutomationSession->setSessionIdentifier(sessionIdentifier);
}

void (*s_commandStatusCallback)(WKStringRef rspMsg);

void WebAutomation::sendMessageToTarget(const String& command, void (*commandCallback)(WKStringRef rspMsg)) {
    s_commandStatusCallback = commandCallback;
    if (m_connected)
        m_webAutomationSession->dispatchMessageFromRemote(command);
}

bool WebAutomation::sendMessageToFrontend(const String& rspMsg) {
   s_commandStatusCallback (WebKit::toAPI(API::String::create(rspMsg).ptr()));
   return true; 
}

} // WKWPE
