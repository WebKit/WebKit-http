/*
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2008 Holger Hans Peter Freyther
 *
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ResourceHandle.h"

#include "Page.h"
#include "QNetworkReplyHandler.h"
#include "ResourceHandleClient.h"
#include "ResourceHandleInternal.h"
#include "SharedBuffer.h"

#include <QAbstractNetworkCache>
#include <QCoreApplication>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrl>

namespace WebCore {

//class WebCoreSynchronousLoader final : public ResourceHandleClient {
//public:
//    WebCoreSynchronousLoader(ResourceError& error, ResourceResponse& response, Vector<char>& data)
//        : m_error(error)
//        , m_response(response)
//        , m_data(data)
//    {}

//    void willSendRequest(ResourceHandle*, ResourceRequest&, const ResourceResponse&) override;
//    void didReceiveResponse(ResourceHandle*, const ResourceResponse& response) override { m_response = response; }
//    void didReceiveData(ResourceHandle*, const char*, unsigned, int) override;
//    void didReceiveBuffer(ResourceHandle*, Ref<SharedBuffer>&&, int /*encodedDataLength*/) override;
//    void didFinishLoading(ResourceHandle*) override {}
//    void didFail(ResourceHandle*, const ResourceError& error) override { m_error = error; }
//private:
//    ResourceError& m_error;
//    ResourceResponse& m_response;
//    Vector<char>& m_data;
//};

//void WebCoreSynchronousLoader::willSendRequest(ResourceHandle* handle, ResourceRequest& request, const ResourceResponse& /*redirectResponse*/)
//{
//    // FIXME: This needs to be fixed to follow the redirect correctly even for cross-domain requests.
//    if (!protocolHostAndPortAreEqual(handle->firstRequest().url(), request.url())) {
//        ASSERT(m_error.isNull());
//        m_error.setType(ResourceErrorBase::Type::Cancellation);
//        request = ResourceRequest();
//        return;
//    }
//}

//void WebCoreSynchronousLoader::didReceiveData(ResourceHandle*, const char*, unsigned, int)
//{
//    ASSERT_NOT_REACHED();
//}

//void WebCoreSynchronousLoader::didReceiveBuffer(ResourceHandle*, Ref<SharedBuffer>&& buffer, int)
//{
//    // This pattern is suggested by SharedBuffer.h.
////    const char* segment;
//    unsigned position = 0;
////    while (auto segment = WTFMove(buffer)->getSomeData(position)) {
//    while (data.size() > position) {
//        auto incrementalData = data.getSomeData(position);
//        m_data.append(segment.data(), segment.size());
//        position += length;
//    }
//}

ResourceHandleInternal::~ResourceHandleInternal()
{
}

ResourceHandle::~ResourceHandle()
{
    if (d->m_job)
        cancel();
}

bool ResourceHandle::start()
{
    // If NetworkingContext is invalid then we are no longer attached to a Page,
    // this must be an attempted load from an unload event handler, so let's just block it.
    if (d->m_context && !d->m_context->isValid())
        return false;

    if (!d->m_user.isEmpty() || !d->m_pass.isEmpty()) {
        // If credentials were specified for this request, add them to the url,
        // so that they will be passed to QNetworkRequest.
        URL urlWithCredentials(firstRequest().url());
        urlWithCredentials.setUser(d->m_user);
        urlWithCredentials.setPass(d->m_pass);
        d->m_firstRequest.setURL(urlWithCredentials);
    }

    ResourceHandleInternal *d = getInternal();
    d->m_job = new QNetworkReplyHandler(this, QNetworkReplyHandler::AsynchronousLoad, d->m_defersLoading);
    return true;
}

void ResourceHandle::cancel()
{
    if (d->m_job) {
        d->m_job->abort();
        d->m_job = 0;
    }
}

void ResourceHandle::platformLoadResourceSynchronously(NetworkingContext* context, const ResourceRequest& request, StoredCredentialsPolicy /*storedCredentials*/, ResourceError& error, ResourceResponse& response, Vector<char>& data)
{
    // QTFIXME
//    WebCoreSynchronousLoader syncLoader(error, response, data);
//    auto handle = adoptRef(*new ResourceHandle(context, request, &syncLoader, true, false));

//    ResourceHandleInternal* d = handle->getInternal();
//    if (!d->m_user.isEmpty() || !d->m_pass.isEmpty()) {
//        // If credentials were specified for this request, add them to the url,
//        // so that they will be passed to QNetworkRequest.
//        URL urlWithCredentials(d->m_firstRequest.url());
//        urlWithCredentials.setUser(d->m_user);
//        urlWithCredentials.setPass(d->m_pass);
//        d->m_firstRequest.setURL(urlWithCredentials);
//    }

//    // starting in deferred mode gives d->m_job the chance of being set before sending the request.
//    d->m_job = new QNetworkReplyHandler(handle.ptr(), QNetworkReplyHandler::SynchronousLoad, true);
//    d->m_job->setLoadingDeferred(false);
}

void ResourceHandle::platformSetDefersLoading(bool defers)
{
    if (!d->m_job)
        return;
    d->m_job->setLoadingDeferred(defers);
}

} // namespace WebCore
