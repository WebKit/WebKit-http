/*
 * Copyright (C) 2016 Igalia S.L.
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
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    m_webAutomationSession = new WebKit::WebAutomationSession();
    m_webAutomationSession->connect((Inspector::FrontendChannel*) this, true);
    m_connected = true;
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

WebAutomation::~WebAutomation () {
    if (m_connected) {
        m_webAutomationSession->disconnect((Inspector::FrontendChannel*) this);	
        m_connected = false;	
    }
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__); fflush(stdout);
    delete m_webAutomationSession;
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__); fflush(stdout);
//    delete m_webAutomationSessionClient;
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__); fflush(stdout);
}

WebAutomation* WebAutomation::create () {
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    return new WebAutomation();
}

void WebAutomation::setClient()
{
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    m_webAutomationSessionClient = new AutomationSessionClient();
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    m_webAutomationSession->setClient((std::unique_ptr<API::AutomationSessionClient>) m_webAutomationSessionClient);    
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

void WebAutomation::setProcessPool(WebKit::WebProcessPool* processPool) 
{
    //Link Process Pool and Automation Session together
    processPool->setAutomationSession(m_webAutomationSession);
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

void (*s_commandStatusCallback)(WKStringRef rspMsg);

void WebAutomation::sendMessageToTarget(const String& command, void (*commandCallback)(WKStringRef rspMsg)) {
    s_commandStatusCallback = commandCallback;
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    if (m_connected)
        m_webAutomationSession->dispatchMessageFromRemote(command);
    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
    fflush(stdout);    printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
}

bool WebAutomation::sendMessageToFrontend(const String& rspMsg) {
   printf("%s:%s:%d\n", __FILE__, __func__, __LINE__);
   s_commandStatusCallback (WebKit::toAPI(API::String::create(rspMsg).ptr()));
   return true; 
}

} // WKWPE

