/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "WebBatteryManagerProxy.h"

#if ENABLE(BATTERY_STATUS)

#include "WebBatteryManagerMessages.h"
#include "WebContext.h"

namespace WebKit {

PassRefPtr<WebBatteryManagerProxy> WebBatteryManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebBatteryManagerProxy(context));
}

WebBatteryManagerProxy::WebBatteryManagerProxy(WebContext* context)
    : m_isUpdating(false)
    , m_context(context)
{
}

WebBatteryManagerProxy::~WebBatteryManagerProxy()
{
}

void WebBatteryManagerProxy::invalidate()
{
    stopUpdating();
}

void WebBatteryManagerProxy::initializeProvider(const WKBatteryProvider* provider)
{
    m_provider.initialize(provider);
}

void WebBatteryManagerProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebBatteryManagerProxyMessage(connection, messageID, arguments);
}

void WebBatteryManagerProxy::startUpdating()
{
    if (m_isUpdating)
        return;

    m_provider.startUpdating(this);
    m_isUpdating = true;
}

void WebBatteryManagerProxy::stopUpdating()
{
    if (!m_isUpdating)
        return;

    m_provider.stopUpdating(this);
    m_isUpdating = false;
}

void WebBatteryManagerProxy::providerDidChangeBatteryStatus(const WTF::AtomicString& eventType, WebBatteryStatus* status)
{
    if (!m_context)
        return;

    m_context->sendToAllProcesses(Messages::WebBatteryManager::DidChangeBatteryStatus(eventType, status->data()));
}

void WebBatteryManagerProxy::providerUpdateBatteryStatus(WebBatteryStatus* status)
{
    if (!m_context)
        return;

    m_context->sendToAllProcesses(Messages::WebBatteryManager::UpdateBatteryStatus(status->data()));
}

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)
