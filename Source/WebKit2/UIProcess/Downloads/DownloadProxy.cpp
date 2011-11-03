/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "DownloadProxy.h"

#include "AuthenticationChallengeProxy.h"
#include "DataReference.h"
#include "WebContext.h"
#include "WebData.h"
#include "WebProcessMessages.h"
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

using namespace WebCore;

namespace WebKit {

static uint64_t generateDownloadID()
{
    static uint64_t uniqueDownloadID = 0;
    return ++uniqueDownloadID;
}
    
PassRefPtr<DownloadProxy> DownloadProxy::create(WebContext* webContext)
{
    return adoptRef(new DownloadProxy(webContext));
}

DownloadProxy::DownloadProxy(WebContext* webContext)
    : m_webContext(webContext)
    , m_downloadID(generateDownloadID())
{
}

DownloadProxy::~DownloadProxy()
{
    ASSERT(!m_webContext);
}

void DownloadProxy::cancel()
{
    if (!m_webContext)
        return;

    // FIXME (Multi-WebProcess): Downloads shouldn't be handled in the web process.
    m_webContext->sendToAllProcesses(Messages::WebProcess::CancelDownload(m_downloadID));
}

void DownloadProxy::invalidate()
{
    ASSERT(m_webContext);
    m_webContext = 0;
}

void DownloadProxy::processDidClose()
{
    if (!m_webContext)
        return;

    m_webContext->downloadClient().processDidCrash(m_webContext, this);
}

void DownloadProxy::didStart(const ResourceRequest& request)
{
    m_request = request;

    if (!m_webContext)
        return;

    m_webContext->downloadClient().didStart(m_webContext, this);
}

void DownloadProxy::didReceiveAuthenticationChallenge(const AuthenticationChallenge& authenticationChallenge, uint64_t challengeID)
{
    if (!m_webContext)
        return;

    RefPtr<AuthenticationChallengeProxy> authenticationChallengeProxy = AuthenticationChallengeProxy::create(authenticationChallenge, challengeID, m_webContext->process());
    m_webContext->downloadClient().didReceiveAuthenticationChallenge(m_webContext, this, authenticationChallengeProxy.get());
}

void DownloadProxy::didReceiveResponse(const ResourceResponse& response)
{
    if (!m_webContext)
        return;

    m_webContext->downloadClient().didReceiveResponse(m_webContext, this, response);
}

void DownloadProxy::didReceiveData(uint64_t length)
{
    if (!m_webContext)
        return;

    m_webContext->downloadClient().didReceiveData(m_webContext, this, length);
}

void DownloadProxy::shouldDecodeSourceDataOfMIMEType(const String& mimeType, bool& result)
{
    if (!m_webContext)
        return;

    result = m_webContext->downloadClient().shouldDecodeSourceDataOfMIMEType(m_webContext, this, mimeType);
}

void DownloadProxy::decideDestinationWithSuggestedFilename(const String& filename, String& destination, bool& allowOverwrite, SandboxExtension::Handle& sandboxExtensionHandle)
{
    if (!m_webContext)
        return;

    destination = m_webContext->downloadClient().decideDestinationWithSuggestedFilename(m_webContext, this, filename, allowOverwrite);

    if (!destination.isNull())
        SandboxExtension::createHandle(destination, SandboxExtension::WriteOnly, sandboxExtensionHandle);
}

void DownloadProxy::didCreateDestination(const String& path)
{
    if (!m_webContext)
        return;

    m_webContext->downloadClient().didCreateDestination(m_webContext, this, path);
}

void DownloadProxy::didFinish()
{
    if (!m_webContext)
        return;

    m_webContext->downloadClient().didFinish(m_webContext, this);

    // This can cause the DownloadProxy object to be deleted.
    m_webContext->downloadFinished(this);
}

static PassRefPtr<WebData> createWebData(const CoreIPC::DataReference& data)
{
    if (data.isEmpty())
        return 0;

    return WebData::create(data.data(), data.size());
}

void DownloadProxy::didFail(const ResourceError& error, const CoreIPC::DataReference& resumeData)
{
    if (!m_webContext)
        return;

    m_resumeData = createWebData(resumeData);

    m_webContext->downloadClient().didFail(m_webContext, this, error);

    // This can cause the DownloadProxy object to be deleted.
    m_webContext->downloadFinished(this);
}

void DownloadProxy::didCancel(const CoreIPC::DataReference& resumeData)
{
    m_resumeData = createWebData(resumeData);

    m_webContext->downloadClient().didCancel(m_webContext, this);

    // This can cause the DownloadProxy object to be deleted.
    m_webContext->downloadFinished(this);
}

#if PLATFORM(QT)
void DownloadProxy::startTransfer(const String& filename)
{
    if (!m_webContext)
        return;

    // FIXME (Multi-WebProcess): Downloads shouldn't be handled in the web process.
    m_webContext->sendToAllProcesses(Messages::WebProcess::StartTransfer(m_downloadID, filename));
}
#endif

} // namespace WebKit

