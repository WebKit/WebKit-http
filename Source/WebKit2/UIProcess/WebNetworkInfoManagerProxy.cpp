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
#include "WebNetworkInfoManagerProxy.h"

#if ENABLE(NETWORK_INFO)

#include "WebContext.h"
#include "WebNetworkInfo.h"
#include "WebNetworkInfoManagerMessages.h"

namespace WebKit {

PassRefPtr<WebNetworkInfoManagerProxy> WebNetworkInfoManagerProxy::create(WebContext* context)
{
    return adoptRef(new WebNetworkInfoManagerProxy(context));
}

WebNetworkInfoManagerProxy::WebNetworkInfoManagerProxy(WebContext* context)
    : m_isUpdating(false)
    , m_context(context)
{
}

WebNetworkInfoManagerProxy::~WebNetworkInfoManagerProxy()
{
}

void WebNetworkInfoManagerProxy::invalidate()
{
    stopUpdating();
}

void WebNetworkInfoManagerProxy::initializeProvider(const WKNetworkInfoProvider* provider)
{
    m_provider.initialize(provider);
}

void WebNetworkInfoManagerProxy::providerDidChangeNetworkInformation(const AtomicString& eventType, WebNetworkInfo* networkInformation)
{
    if (!m_context)
        return;

    m_context->sendToAllProcesses(Messages::WebNetworkInfoManager::DidChangeNetworkInformation(eventType, networkInformation->data()));
}

void WebNetworkInfoManagerProxy::didReceiveMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments)
{
    didReceiveWebNetworkInfoManagerProxyMessage(connection, messageID, arguments);
}

void WebNetworkInfoManagerProxy::didReceiveSyncMessage(CoreIPC::Connection* connection, CoreIPC::MessageID messageID, CoreIPC::ArgumentDecoder* arguments, WTF::OwnPtr<CoreIPC::ArgumentEncoder>& reply)
{
    didReceiveSyncWebNetworkInfoManagerProxyMessage(connection, messageID, arguments, reply);
}

void WebNetworkInfoManagerProxy::startUpdating()
{
    if (m_isUpdating)
        return;

    m_provider.startUpdating(this);
    m_isUpdating = true;
}

void WebNetworkInfoManagerProxy::stopUpdating()
{
    if (!m_isUpdating)
        return;

    m_provider.stopUpdating(this);
    m_isUpdating = false;
}

void WebNetworkInfoManagerProxy::getBandwidth(double& bandwidth)
{
    bandwidth = m_provider.bandwidth(this);
}

void WebNetworkInfoManagerProxy::isMetered(bool& isMetered)
{
    isMetered = m_provider.isMetered(this);
}

} // namespace WebKit

#endif // ENABLE(NETWORK_INFO)
