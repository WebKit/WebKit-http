/*
 * Copyright (c) 2018 Metrological
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR OR; PROFITS BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY OF THEORY LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "WKWebAutomationSession.h"

#include "APIAutomationSessionClient.h"
#include "WKAPICast.h"
#include "WKString.h"
#include "WebAutomationSession.h"
#include "WebPageProxy.h"

namespace API {
template<> struct ClientTraits<WKWebAutomationSessionClientBase> {
    typedef std::tuple<WKWebAutomationSessionClientV0> Versions;
};
}

using namespace WebKit;

WKTypeID WKWebAutomationSessionGetTypeID()
{
    return toAPI(WebAutomationSession::APIType);
}

WKWebAutomationSessionRef WKWebAutomationSessionCreate(WKStringRef identifier)
{
    auto session = adoptRef(new WebAutomationSession());
    session->setSessionIdentifier(toWTFString(identifier));
    return toAPI(session.leakRef());
}

void WKWebAutomationSessionSetClient(WKWebAutomationSessionRef session, const WKWebAutomationSessionClientBase* wkClient)
{
    class AutomationSessionClient final : public API::Client<WKWebAutomationSessionClientBase>, public API::AutomationSessionClient {
    public:
        explicit AutomationSessionClient(const WKWebAutomationSessionClientBase* client)
        {
            initialize(client);
        }
    private:
        void requestNewPageWithOptions(WebAutomationSession& session, API::AutomationSessionBrowsingContextOptions, CompletionHandler<void(WebPageProxy*)>&& completionHandler) override
        {
            if (!m_client.requestNewPage) {
                completionHandler(nullptr);
                return;
            }
            if (auto* page = m_client.requestNewPage(toAPI(&session), m_client.base.clientInfo)) {
                WKPageSetControlledByAutomation(page, true);
                completionHandler(toImpl(page));
                return;
            }
            completionHandler(nullptr);
        }
    };

    toImpl(session)->setClient(std::make_unique<AutomationSessionClient>(wkClient));
}
