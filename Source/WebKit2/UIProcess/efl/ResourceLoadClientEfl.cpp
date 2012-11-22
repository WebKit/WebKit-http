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
#include "ResourceLoadClientEfl.h"

#include "EwkViewImpl.h"
#include "WKAPICast.h"
#include "WKFrame.h"
#include "WKPage.h"
#include "WKRetainPtr.h"
#include "ewk_error_private.h"
#include "ewk_url_request_private.h"
#include "ewk_url_response_private.h"
#include "ewk_view.h"

using namespace WebCore;
using namespace EwkViewCallbacks;

namespace WebKit {

static inline ResourceLoadClientEfl* toResourceLoadClientEfl(const void* clientInfo)
{
    return static_cast<ResourceLoadClientEfl*>(const_cast<void*>(clientInfo));
}

void ResourceLoadClientEfl::didInitiateLoadForResource(WKPageRef, WKFrameRef wkFrame, uint64_t resourceIdentifier, WKURLRequestRef wkRequest, bool pageIsProvisionallyLoading, const void* clientInfo)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(clientInfo);
    bool isMainResource = (WKFrameIsMainFrame(wkFrame) && pageIsProvisionallyLoading);
    WKRetainPtr<WKURLRef> wkUrl(AdoptWK, WKURLRequestCopyURL(wkRequest));

    RefPtr<EwkResource> resource = EwkResource::create(wkUrl.get(), isMainResource);

    // Keep the resource internally to reuse it later.
    resourceLoadClient->m_loadingResourcesMap.add(resourceIdentifier, resource);

    RefPtr<EwkUrlRequest> request = EwkUrlRequest::create(wkRequest);
    Ewk_Resource_Request resourceRequest = {resource.get(), request.get(), 0};
    resourceLoadClient->m_viewImpl->smartCallback<ResourceLoadStarted>().call(&resourceRequest);
}

void ResourceLoadClientEfl::didSendRequestForResource(WKPageRef, WKFrameRef, uint64_t resourceIdentifier, WKURLRequestRef wkRequest, WKURLResponseRef wkRedirectResponse, const void* clientInfo)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(clientInfo);

    RefPtr<EwkResource> resource = resourceLoadClient->m_loadingResourcesMap.get(resourceIdentifier);
    // Only process if we know about this resource.
    if (!resource)
        return;

    RefPtr<EwkUrlRequest> request = EwkUrlRequest::create(wkRequest);
    RefPtr<EwkUrlResponse> redirectResponse = EwkUrlResponse::create(wkRedirectResponse);
    Ewk_Resource_Request resourceRequest = {resource.get(), request.get(), redirectResponse.get()};
    resourceLoadClient->m_viewImpl->smartCallback<ResourceRequestSent>().call(&resourceRequest);
}

void ResourceLoadClientEfl::didReceiveResponseForResource(WKPageRef, WKFrameRef, uint64_t resourceIdentifier, WKURLResponseRef wkResponse, const void* clientInfo)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(clientInfo);

    RefPtr<EwkResource> resource = resourceLoadClient->m_loadingResourcesMap.get(resourceIdentifier);
    // Only process if we know about this resource.
    if (!resource)
        return;

    RefPtr<EwkUrlResponse> response = EwkUrlResponse::create(wkResponse);
    Ewk_Resource_Load_Response resourceLoadResponse = {resource.get(), response.get()};
    resourceLoadClient->m_viewImpl->smartCallback<ResourceLoadResponse>().call(&resourceLoadResponse);
}

void ResourceLoadClientEfl::didFinishLoadForResource(WKPageRef, WKFrameRef, uint64_t resourceIdentifier, const void* clientInfo)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(clientInfo);

    RefPtr<EwkResource> resource = resourceLoadClient->m_loadingResourcesMap.get(resourceIdentifier);
    // Only process if we know about this resource.
    if (!resource)
        return;

    resourceLoadClient->m_viewImpl->smartCallback<ResourceLoadFinished>().call(resource.get());
}

void ResourceLoadClientEfl::didFailLoadForResource(WKPageRef, WKFrameRef, uint64_t resourceIdentifier, WKErrorRef wkError, const void* clientInfo)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(clientInfo);

    RefPtr<EwkResource> resource = resourceLoadClient->m_loadingResourcesMap.take(resourceIdentifier);
    // Only process if we know about this resource.
    if (!resource)
        return;

    OwnPtr<EwkError> ewkError = EwkError::create(wkError);
    Ewk_Resource_Load_Error resourceLoadError = {resource.get(), ewkError.get()};
    resourceLoadClient->m_viewImpl->smartCallback<ResourceLoadFailed>().call(&resourceLoadError);
    resourceLoadClient->m_viewImpl->smartCallback<ResourceLoadFinished>().call(resource.get());
}

void ResourceLoadClientEfl::onViewProvisionalLoadStarted(void* userData, Evas_Object*, void*)
{
    ResourceLoadClientEfl* resourceLoadClient = toResourceLoadClientEfl(userData);

    // The view started a new load, clear internal resource map.
    resourceLoadClient->m_loadingResourcesMap.clear();
}

ResourceLoadClientEfl::ResourceLoadClientEfl(EwkViewImpl* viewImpl)
    : m_viewImpl(viewImpl)
{
    // Listen for "load,provisional,started" on the view to clear internal resources map.
    evas_object_smart_callback_add(m_viewImpl->view(), CallBackInfo<ProvisionalLoadStarted>::name(), onViewProvisionalLoadStarted, this);

    WKPageRef pageRef = m_viewImpl->wkPage();
    ASSERT(pageRef);

    WKPageResourceLoadClient wkResourceLoadClient;
    memset(&wkResourceLoadClient, 0, sizeof(WKPageResourceLoadClient));
    wkResourceLoadClient.version = kWKPageResourceLoadClientCurrentVersion;
    wkResourceLoadClient.clientInfo = this;
    wkResourceLoadClient.didInitiateLoadForResource = didInitiateLoadForResource;
    wkResourceLoadClient.didSendRequestForResource = didSendRequestForResource;
    wkResourceLoadClient.didReceiveResponseForResource = didReceiveResponseForResource;
    wkResourceLoadClient.didFinishLoadForResource = didFinishLoadForResource;
    wkResourceLoadClient.didFailLoadForResource = didFailLoadForResource;

    WKPageSetPageResourceLoadClient(pageRef, &wkResourceLoadClient);
}

ResourceLoadClientEfl::~ResourceLoadClientEfl()
{
    evas_object_smart_callback_del(m_viewImpl->view(), CallBackInfo<ProvisionalLoadStarted>::name(), onViewProvisionalLoadStarted);
}

} // namespace WebKit
