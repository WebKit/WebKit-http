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

#include "WebCookieManagerProxy.h"

#include "ArgumentCoders.h"
#include "HandleMessage.h"
#include "MessageDecoder.h"
#include "WebCookieManagerProxyMessages.h"
#include "WebCoreArgumentCoders.h"
#include <WebCore/Cookie.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

void WebCookieManagerProxy::didReceiveMessage(IPC::Connection& connection, IPC::MessageDecoder& decoder)
{
    if (decoder.messageName() == Messages::WebCookieManagerProxy::DidGetHostnamesWithCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManagerProxy::DidGetHostnamesWithCookies>(decoder, this, &WebCookieManagerProxy::didGetHostnamesWithCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManagerProxy::DidGetHTTPCookieAcceptPolicy::name()) {
        IPC::handleMessage<Messages::WebCookieManagerProxy::DidGetHTTPCookieAcceptPolicy>(decoder, this, &WebCookieManagerProxy::didGetHTTPCookieAcceptPolicy);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManagerProxy::DidGetCookies::name()) {
        IPC::handleMessage<Messages::WebCookieManagerProxy::DidGetCookies>(decoder, this, &WebCookieManagerProxy::didGetCookies);
        return;
    }
    if (decoder.messageName() == Messages::WebCookieManagerProxy::CookiesDidChange::name()) {
        IPC::handleMessage<Messages::WebCookieManagerProxy::CookiesDidChange>(decoder, this, &WebCookieManagerProxy::cookiesDidChange);
        return;
    }
    UNUSED_PARAM(connection);
    UNUSED_PARAM(decoder);
    ASSERT_NOT_REACHED();
}

} // namespace WebKit
