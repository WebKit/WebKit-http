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

#include "WebCookieManager.h"

#include "ArgumentCoders.h"
#include "Decoder.h"
#include "HandleMessage.h"
#include "WebCookieManagerMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/Cookie.h>
#include <chrono>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebCookieManager::didReceiveMessage(IPC::Connection& connection, IPC::Decoder& decoder)
{
    if (decoder.messageName() == Messages::WebCookieManager::GetHostnamesWithCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManager::GetHostnamesWithCookies>(decoder, this, &WebCookieManager::getHostnamesWithCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::DeleteCookiesForHostname::name()) {
        IPC::handleMessage<Messages::WebCookieManager::DeleteCookiesForHostname>(decoder, this, &WebCookieManager::deleteCookiesForHostname);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::DeleteAllCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManager::DeleteAllCookies>(decoder, this, &WebCookieManager::deleteAllCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::DeleteAllCookiesModifiedSince::name()) {
        IPC::handleMessage<Messages::WebCookieManager::DeleteAllCookiesModifiedSince>(decoder, this, &WebCookieManager::deleteAllCookiesModifiedSince);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::AddCookie::name()) {
        IPC::handleMessage<Messages::WebCookieManager::AddCookie>(decoder, this, &WebCookieManager::addCookie);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::SetHTTPCookieAcceptPolicy::name()) {
        IPC::handleMessage<Messages::WebCookieManager::SetHTTPCookieAcceptPolicy>(decoder, this, &WebCookieManager::setHTTPCookieAcceptPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::GetHTTPCookieAcceptPolicy::name()) {
        IPC::handleMessage<Messages::WebCookieManager::GetHTTPCookieAcceptPolicy>(decoder, this, &WebCookieManager::getHTTPCookieAcceptPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::SetCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManager::SetCookies>(decoder, this, &WebCookieManager::setCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::GetCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManager::GetCookies>(decoder, this, &WebCookieManager::getCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::StartObservingCookieChanges::name()) {
        IPC::handleMessage<Messages::WebCookieManager::StartObservingCookieChanges>(decoder, this, &WebCookieManager::startObservingCookieChanges);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManager::StopObservingCookieChanges::name()) {
        IPC::handleMessage<Messages::WebCookieManager::StopObservingCookieChanges>(decoder, this, &WebCookieManager::stopObservingCookieChanges);
        return;
    }
#if USE(SOUP)
    if (decoder.messageName() == Messages::WebCookieManager::SetCookiePersistentStorage::name()) {
        IPC::handleMessage<Messages::WebCookieManager::SetCookiePersistentStorage>(decoder, this, &WebCookieManager::setCookiePersistentStorage);
        return;
    }
#endif
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
