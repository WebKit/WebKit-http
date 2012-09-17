/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(INSPECTOR)

#include "InspectorResourceAgent.h"

#include "CachedRawResource.h"
#include "CachedResource.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "HTTPHeaderMap.h"
#include "IdentifiersFactory.h"
#include "InspectorClient.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "KURL.h"
#include "MemoryCache.h"
#include "NetworkResourcesData.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "ResourceError.h"
#include "ResourceLoader.h"
#include "ResourceRequest.h"
#include "ResourceResponse.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "ScriptableDocumentParser.h"
#include "SubresourceLoader.h"
#include "WebSocketFrame.h"
#include "WebSocketHandshakeRequest.h"
#include "WebSocketHandshakeResponse.h"
#include "XMLHttpRequest.h"

#include <wtf/CurrentTime.h>
#include <wtf/HexNumber.h>
#include <wtf/ListHashSet.h>
#include <wtf/RefPtr.h>
#include <wtf/text/StringBuilder.h>

namespace WebCore {

namespace ResourceAgentState {
static const char resourceAgentEnabled[] = "resourceAgentEnabled";
static const char extraRequestHeaders[] = "extraRequestHeaders";
static const char cacheDisabled[] = "cacheDisabled";
static const char userAgentOverride[] = "userAgentOverride";
}

void InspectorResourceAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->network();
}

void InspectorResourceAgent::clearFrontend()
{
    m_frontend = 0;
    ErrorString error;
    disable(&error);
}

void InspectorResourceAgent::restore()
{
    if (m_state->getBoolean(ResourceAgentState::resourceAgentEnabled))
        enable();
}

static PassRefPtr<InspectorObject> buildObjectForHeaders(const HTTPHeaderMap& headers)
{
    RefPtr<InspectorObject> headersObject = InspectorObject::create();
    HTTPHeaderMap::const_iterator end = headers.end();
    for (HTTPHeaderMap::const_iterator it = headers.begin(); it != end; ++it)
        headersObject->setString(it->first.string(), it->second);
    return headersObject;
}

static PassRefPtr<TypeBuilder::Network::ResourceTiming> buildObjectForTiming(const ResourceLoadTiming& timing, DocumentLoader* loader)
{
    return TypeBuilder::Network::ResourceTiming::create()
        .setRequestTime(timing.convertResourceLoadTimeToDocumentTime(loader->timing(), 0))
        .setProxyStart(timing.proxyStart)
        .setProxyEnd(timing.proxyEnd)
        .setDnsStart(timing.dnsStart)
        .setDnsEnd(timing.dnsEnd)
        .setConnectStart(timing.connectStart)
        .setConnectEnd(timing.connectEnd)
        .setSslStart(timing.sslStart)
        .setSslEnd(timing.sslEnd)
        .setSendStart(timing.sendStart)
        .setSendEnd(timing.sendEnd)
        .setReceiveHeadersEnd(timing.receiveHeadersEnd)
        .release();
}

static PassRefPtr<TypeBuilder::Network::Request> buildObjectForResourceRequest(const ResourceRequest& request)
{
    RefPtr<TypeBuilder::Network::Request> requestObject = TypeBuilder::Network::Request::create()
        .setUrl(request.url().string())
        .setMethod(request.httpMethod())
        .setHeaders(buildObjectForHeaders(request.httpHeaderFields()));
    if (request.httpBody() && !request.httpBody()->isEmpty())
        requestObject->setPostData(request.httpBody()->flattenToString());
    return requestObject;
}

static PassRefPtr<TypeBuilder::Network::Response> buildObjectForResourceResponse(const ResourceResponse& response, DocumentLoader* loader)
{
    if (response.isNull())
        return 0;


    double status;
    String statusText;
    if (response.resourceLoadInfo() && response.resourceLoadInfo()->httpStatusCode) {
        status = response.resourceLoadInfo()->httpStatusCode;
        statusText = response.resourceLoadInfo()->httpStatusText;
    } else {
        status = response.httpStatusCode();
        statusText = response.httpStatusText();
    }
    RefPtr<InspectorObject> headers;
    if (response.resourceLoadInfo())
        headers = buildObjectForHeaders(response.resourceLoadInfo()->responseHeaders);
    else
        headers = buildObjectForHeaders(response.httpHeaderFields());

    RefPtr<TypeBuilder::Network::Response> responseObject = TypeBuilder::Network::Response::create()
        .setUrl(response.url().string())
        .setStatus(status)
        .setStatusText(statusText)
        .setHeaders(headers)
        .setMimeType(response.mimeType())
        .setConnectionReused(response.connectionReused())
        .setConnectionId(response.connectionID());

    responseObject->setFromDiskCache(response.wasCached());
    if (response.resourceLoadTiming())
        responseObject->setTiming(buildObjectForTiming(*response.resourceLoadTiming(), loader));

    if (response.resourceLoadInfo()) {
        if (!response.resourceLoadInfo()->responseHeadersText.isEmpty())
            responseObject->setHeadersText(response.resourceLoadInfo()->responseHeadersText);

        responseObject->setRequestHeaders(buildObjectForHeaders(response.resourceLoadInfo()->requestHeaders));
        if (!response.resourceLoadInfo()->requestHeadersText.isEmpty())
            responseObject->setRequestHeadersText(response.resourceLoadInfo()->requestHeadersText);
    }

    return responseObject;
}

static PassRefPtr<TypeBuilder::Network::CachedResource> buildObjectForCachedResource(const CachedResource& cachedResource, DocumentLoader* loader)
{
    RefPtr<TypeBuilder::Network::CachedResource> resourceObject = TypeBuilder::Network::CachedResource::create()
        .setUrl(cachedResource.url())
        .setType(InspectorPageAgent::cachedResourceTypeJson(cachedResource))
        .setBodySize(cachedResource.encodedSize());
    RefPtr<TypeBuilder::Network::Response> resourceResponse = buildObjectForResourceResponse(cachedResource.response(), loader);
    if (resourceResponse)
        resourceObject->setResponse(resourceResponse);
    return resourceObject;
}

InspectorResourceAgent::~InspectorResourceAgent()
{
    if (m_state->getBoolean(ResourceAgentState::resourceAgentEnabled)) {
        ErrorString error;
        disable(&error);
    }
    ASSERT(!m_instrumentingAgents->inspectorResourceAgent());
}

void InspectorResourceAgent::willSendRequest(unsigned long identifier, DocumentLoader* loader, ResourceRequest& request, const ResourceResponse& redirectResponse)
{
    String requestId = IdentifiersFactory::requestId(identifier);
    m_resourcesData->resourceCreated(requestId, m_pageAgent->loaderId(loader));

    RefPtr<InspectorObject> headers = m_state->getObject(ResourceAgentState::extraRequestHeaders);

    if (headers) {
        InspectorObject::const_iterator end = headers->end();
        for (InspectorObject::const_iterator it = headers->begin(); it != end; ++it) {
            String value;
            if (it->second->asString(&value))
                request.setHTTPHeaderField(it->first, value);
        }
    }

    request.setReportLoadTiming(true);
    request.setReportRawHeaders(true);

    if (m_state->getBoolean(ResourceAgentState::cacheDisabled)) {
        request.setHTTPHeaderField("Pragma", "no-cache");
        request.setCachePolicy(ReloadIgnoringCacheData);
        request.setHTTPHeaderField("Cache-Control", "no-cache");
    }

    RefPtr<TypeBuilder::Network::Initiator> initiatorObject = buildInitiatorObject(loader->frame() ? loader->frame()->document() : 0);
    m_frontend->requestWillBeSent(requestId, m_pageAgent->frameId(loader->frame()), m_pageAgent->loaderId(loader), loader->url().string(), buildObjectForResourceRequest(request), currentTime(), initiatorObject, buildObjectForResourceResponse(redirectResponse, loader));
}

void InspectorResourceAgent::markResourceAsCached(unsigned long identifier)
{
    m_frontend->requestServedFromCache(IdentifiersFactory::requestId(identifier));
}

void InspectorResourceAgent::didReceiveResponse(unsigned long identifier, DocumentLoader* loader, const ResourceResponse& response, ResourceLoader* resourceLoader)
{
    String requestId = IdentifiersFactory::requestId(identifier);
    RefPtr<TypeBuilder::Network::Response> resourceResponse = buildObjectForResourceResponse(response, loader);
    InspectorPageAgent::ResourceType type = InspectorPageAgent::OtherResource;
    long cachedResourceSize = 0;

    bool isNotModified = response.httpStatusCode() == 304;
    if (loader) {
        CachedResource* cachedResource = 0;
        if (resourceLoader && resourceLoader->isSubresourceLoader() && !isNotModified)
            cachedResource = static_cast<SubresourceLoader*>(resourceLoader)->cachedResource();
        if (!cachedResource)
            cachedResource = InspectorPageAgent::cachedResource(loader->frame(), response.url());
        if (cachedResource) {
            type = InspectorPageAgent::cachedResourceType(*cachedResource);
            cachedResourceSize = cachedResource->encodedSize();
            // Use mime type from cached resource in case the one in response is empty.
            if (resourceResponse && response.mimeType().isEmpty())
                resourceResponse->setString(TypeBuilder::Network::Response::MimeType, cachedResource->response().mimeType());

            m_resourcesData->addCachedResource(requestId, cachedResource);
        }
        if (m_loadingXHRSynchronously || m_resourcesData->resourceType(requestId) == InspectorPageAgent::XHRResource)
            type = InspectorPageAgent::XHRResource;
        else if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::ScriptResource)
            type = InspectorPageAgent::ScriptResource;
        else if (equalIgnoringFragmentIdentifier(response.url(), loader->frameLoader()->icon()->url()))
            type = InspectorPageAgent::ImageResource;
        else if (equalIgnoringFragmentIdentifier(response.url(), loader->url()) && !loader->isCommitted())
            type = InspectorPageAgent::DocumentResource;

        m_resourcesData->responseReceived(requestId, m_pageAgent->frameId(loader->frame()), response);
    }
    m_resourcesData->setResourceType(requestId, type);
    m_frontend->responseReceived(requestId, m_pageAgent->frameId(loader->frame()), m_pageAgent->loaderId(loader), currentTime(), InspectorPageAgent::resourceTypeJson(type), resourceResponse);
    // If we revalidated the resource and got Not modified, send content length following didReceiveResponse
    // as there will be no calls to didReceiveData from the network stack.
    if (cachedResourceSize && isNotModified)
        didReceiveData(identifier, 0, cachedResourceSize, 0);
}

static bool isErrorStatusCode(int statusCode)
{
    return statusCode >= 400;
}

void InspectorResourceAgent::didReceiveData(unsigned long identifier, const char* data, int dataLength, int encodedDataLength)
{
    String requestId = IdentifiersFactory::requestId(identifier);

    if (data) {
        NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
        if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::OtherResource || (resourceData && isErrorStatusCode(resourceData->httpStatusCode()) && resourceData->cachedResource()))
            m_resourcesData->maybeAddResourceData(requestId, data, dataLength);
    }

    m_frontend->dataReceived(requestId, currentTime(), dataLength, encodedDataLength);
}

void InspectorResourceAgent::didFinishLoading(unsigned long identifier, DocumentLoader* loader, double finishTime)
{
    String requestId = IdentifiersFactory::requestId(identifier);
    if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::DocumentResource)
        m_resourcesData->addResourceSharedBuffer(requestId, loader->frameLoader()->documentLoader()->mainResourceData(), loader->frame()->document()->inputEncoding());

    m_resourcesData->maybeDecodeDataToContent(requestId);

    if (!finishTime)
        finishTime = currentTime();

    m_frontend->loadingFinished(requestId, finishTime);
}

void InspectorResourceAgent::didFailLoading(unsigned long identifier, DocumentLoader* loader, const ResourceError& error)
{
    String requestId = IdentifiersFactory::requestId(identifier);

    if (m_resourcesData->resourceType(requestId) == InspectorPageAgent::DocumentResource) {
        Frame* frame = loader ? loader->frame() : 0;
        if (frame && frame->loader()->documentLoader() && frame->document())
            m_resourcesData->addResourceSharedBuffer(requestId, frame->loader()->documentLoader()->mainResourceData(), frame->document()->inputEncoding());
    }

    bool canceled = error.isCancellation();
    m_frontend->loadingFailed(requestId, currentTime(), error.localizedDescription(), canceled ? &canceled : 0);
}

void InspectorResourceAgent::didLoadResourceFromMemoryCache(DocumentLoader* loader, CachedResource* resource)
{
    String loaderId = m_pageAgent->loaderId(loader);
    String frameId = m_pageAgent->frameId(loader->frame());
    unsigned long identifier = loader->frame()->page()->progress()->createUniqueIdentifier();
    String requestId = IdentifiersFactory::requestId(identifier);
    m_resourcesData->resourceCreated(requestId, loaderId);
    m_resourcesData->addCachedResource(requestId, resource);
    if (resource->type() == CachedResource::RawResource) {
        CachedRawResource* rawResource = static_cast<CachedRawResource*>(resource);
        String rawRequestId = IdentifiersFactory::requestId(rawResource->identifier());
        m_resourcesData->reuseXHRReplayData(requestId, rawRequestId);
    }

    RefPtr<TypeBuilder::Network::Initiator> initiatorObject = buildInitiatorObject(loader->frame() ? loader->frame()->document() : 0);

    m_frontend->requestServedFromMemoryCache(requestId, frameId, loaderId, loader->url().string(), currentTime(), initiatorObject, buildObjectForCachedResource(*resource, loader));
}

void InspectorResourceAgent::setInitialScriptContent(unsigned long identifier, const String& sourceString)
{
    m_resourcesData->setResourceContent(IdentifiersFactory::requestId(identifier), sourceString);
}

void InspectorResourceAgent::didReceiveScriptResponse(unsigned long identifier)
{
    m_resourcesData->setResourceType(IdentifiersFactory::requestId(identifier), InspectorPageAgent::ScriptResource);
}

void InspectorResourceAgent::documentThreadableLoaderStartedLoadingForClient(unsigned long identifier, ThreadableLoaderClient* client)
{
    if (!client)
        return;

    PendingXHRReplayDataMap::iterator it = m_pendingXHRReplayData.find(client);
    if (it == m_pendingXHRReplayData.end())
        return;

    XHRReplayData* xhrReplayData = it->second.get();
    String requestId = IdentifiersFactory::requestId(identifier);
    m_resourcesData->setXHRReplayData(requestId, xhrReplayData);
}

void InspectorResourceAgent::willLoadXHR(ThreadableLoaderClient* client, const String& method, const KURL& url, bool async, PassRefPtr<FormData> formData, const HTTPHeaderMap& headers, bool includeCredentials)
{
    RefPtr<XHRReplayData> xhrReplayData = XHRReplayData::create(method, url, async, formData, includeCredentials);
    HTTPHeaderMap::const_iterator end = headers.end();
    for (HTTPHeaderMap::const_iterator it = headers.begin(); it!= end; ++it)
        xhrReplayData->addHeader(it->first, it->second);
    m_pendingXHRReplayData.set(client, xhrReplayData);
}

void InspectorResourceAgent::didFailXHRLoading(ThreadableLoaderClient* client)
{
    m_pendingXHRReplayData.remove(client);
}

void InspectorResourceAgent::didFinishXHRLoading(ThreadableLoaderClient* client, unsigned long identifier, const String& sourceString)
{
    // For Asynchronous XHRs, the inspector can grab the data directly off of the CachedResource. For sync XHRs, we need to
    // provide the data here, since no CachedResource was involved.
    if (m_loadingXHRSynchronously)
        m_resourcesData->setResourceContent(IdentifiersFactory::requestId(identifier), sourceString);
    m_pendingXHRReplayData.remove(client);
}

void InspectorResourceAgent::didReceiveXHRResponse(unsigned long identifier)
{
    m_resourcesData->setResourceType(IdentifiersFactory::requestId(identifier), InspectorPageAgent::XHRResource);
}

void InspectorResourceAgent::willLoadXHRSynchronously()
{
    m_loadingXHRSynchronously = true;
}

void InspectorResourceAgent::didLoadXHRSynchronously()
{
    m_loadingXHRSynchronously = false;
}

void InspectorResourceAgent::willDestroyCachedResource(CachedResource* cachedResource)
{
    Vector<String> requestIds = m_resourcesData->removeCachedResource(cachedResource);
    if (!requestIds.size())
        return;

    String content;
    bool base64Encoded;
    if (!InspectorPageAgent::cachedResourceContent(cachedResource, &content, &base64Encoded))
        return;
    Vector<String>::iterator end = requestIds.end();
    for (Vector<String>::iterator it = requestIds.begin(); it != end; ++it)
        m_resourcesData->setResourceContent(*it, content, base64Encoded);
}

void InspectorResourceAgent::applyUserAgentOverride(String* userAgent)
{
    String userAgentOverride = m_state->getString(ResourceAgentState::userAgentOverride);
    if (!userAgentOverride.isEmpty())
        *userAgent = userAgentOverride;
}

void InspectorResourceAgent::willRecalculateStyle()
{
    m_isRecalculatingStyle = true;
}

void InspectorResourceAgent::didRecalculateStyle()
{
    m_isRecalculatingStyle = false;
    m_styleRecalculationInitiator = nullptr;
}

void InspectorResourceAgent::didScheduleStyleRecalculation(Document* document)
{
    if (!m_styleRecalculationInitiator)
        m_styleRecalculationInitiator = buildInitiatorObject(document);
}

PassRefPtr<TypeBuilder::Network::Initiator> InspectorResourceAgent::buildInitiatorObject(Document* document)
{
    RefPtr<ScriptCallStack> stackTrace = createScriptCallStack(ScriptCallStack::maxCallStackSizeToCapture, true);
    if (stackTrace && stackTrace->size() > 0) {
        RefPtr<TypeBuilder::Network::Initiator> initiatorObject = TypeBuilder::Network::Initiator::create()
            .setType(TypeBuilder::Network::Initiator::Type::Script);
        initiatorObject->setStackTrace(stackTrace->buildInspectorArray());
        return initiatorObject;
    }

    if (document && document->scriptableDocumentParser()) {
        RefPtr<TypeBuilder::Network::Initiator> initiatorObject = TypeBuilder::Network::Initiator::create()
            .setType(TypeBuilder::Network::Initiator::Type::Parser);
        initiatorObject->setUrl(document->url().string());
        initiatorObject->setLineNumber(document->scriptableDocumentParser()->lineNumber().oneBasedInt());
        return initiatorObject;
    }

    if (m_isRecalculatingStyle && m_styleRecalculationInitiator)
        return m_styleRecalculationInitiator;

    return TypeBuilder::Network::Initiator::create()
        .setType(TypeBuilder::Network::Initiator::Type::Other)
        .release();
}

#if ENABLE(WEB_SOCKETS)

// FIXME: More this into the front-end?
// Create human-readable binary representation, like "01:23:45:67:89:AB:CD:EF".
static String createReadableStringFromBinary(const unsigned char* value, size_t length)
{
    ASSERT(length > 0);
    StringBuilder builder;
    builder.reserveCapacity(length * 3 - 1);
    for (size_t i = 0; i < length; ++i) {
        if (i > 0)
            builder.append(':');
        appendByteAsHex(value[i], builder);
    }
    return builder.toString();
}

void InspectorResourceAgent::didCreateWebSocket(unsigned long identifier, const KURL& requestURL)
{
    m_frontend->webSocketCreated(IdentifiersFactory::requestId(identifier), requestURL.string());
}

void InspectorResourceAgent::willSendWebSocketHandshakeRequest(unsigned long identifier, const WebSocketHandshakeRequest& request)
{
    RefPtr<TypeBuilder::Network::WebSocketRequest> requestObject = TypeBuilder::Network::WebSocketRequest::create()
        .setRequestKey3(createReadableStringFromBinary(request.key3().value, sizeof(request.key3().value)))
        .setHeaders(buildObjectForHeaders(request.headerFields()));
    m_frontend->webSocketWillSendHandshakeRequest(IdentifiersFactory::requestId(identifier), currentTime(), requestObject);
}

void InspectorResourceAgent::didReceiveWebSocketHandshakeResponse(unsigned long identifier, const WebSocketHandshakeResponse& response)
{
    RefPtr<TypeBuilder::Network::WebSocketResponse> responseObject = TypeBuilder::Network::WebSocketResponse::create()
        .setStatus(response.statusCode())
        .setStatusText(response.statusText())
        .setHeaders(buildObjectForHeaders(response.headerFields()))
        .setChallengeResponse(createReadableStringFromBinary(response.challengeResponse().value, sizeof(response.challengeResponse().value)));
    m_frontend->webSocketHandshakeResponseReceived(IdentifiersFactory::requestId(identifier), currentTime(), responseObject);
}

void InspectorResourceAgent::didCloseWebSocket(unsigned long identifier)
{
    m_frontend->webSocketClosed(IdentifiersFactory::requestId(identifier), currentTime());
}

void InspectorResourceAgent::didReceiveWebSocketFrame(unsigned long identifier, const WebSocketFrame& frame)
{
    RefPtr<TypeBuilder::Network::WebSocketFrame> frameObject = TypeBuilder::Network::WebSocketFrame::create()
        .setOpcode(frame.opCode)
        .setMask(frame.masked)
        .setPayloadData(String(frame.payload, frame.payloadLength));
    m_frontend->webSocketFrameReceived(IdentifiersFactory::requestId(identifier), currentTime(), frameObject);
}

void InspectorResourceAgent::didSendWebSocketFrame(unsigned long identifier, const WebSocketFrame& frame)
{
    RefPtr<TypeBuilder::Network::WebSocketFrame> frameObject = TypeBuilder::Network::WebSocketFrame::create()
        .setOpcode(frame.opCode)
        .setMask(frame.masked)
        .setPayloadData(String(frame.payload, frame.payloadLength));
    m_frontend->webSocketFrameSent(IdentifiersFactory::requestId(identifier), currentTime(), frameObject);
}

void InspectorResourceAgent::didReceiveWebSocketFrameError(unsigned long identifier, const String& errorMessage)
{
    m_frontend->webSocketFrameError(IdentifiersFactory::requestId(identifier), currentTime(), errorMessage);
}

#endif // ENABLE(WEB_SOCKETS)

// called from Internals for layout test purposes.
void InspectorResourceAgent::setResourcesDataSizeLimitsFromInternals(int maximumResourcesContentSize, int maximumSingleResourceContentSize)
{
    m_resourcesData->setResourcesDataSizeLimits(maximumResourcesContentSize, maximumSingleResourceContentSize);
}

void InspectorResourceAgent::enable(ErrorString*)
{
    enable();
}

void InspectorResourceAgent::enable()
{
    if (!m_frontend)
        return;
    m_state->setBoolean(ResourceAgentState::resourceAgentEnabled, true);
    m_instrumentingAgents->setInspectorResourceAgent(this);
}

void InspectorResourceAgent::disable(ErrorString*)
{
    m_state->setBoolean(ResourceAgentState::resourceAgentEnabled, false);
    m_instrumentingAgents->setInspectorResourceAgent(0);
    m_resourcesData->clear();
}

void InspectorResourceAgent::setUserAgentOverride(ErrorString*, const String& userAgent)
{
    m_state->setString(ResourceAgentState::userAgentOverride, userAgent);
}

void InspectorResourceAgent::setExtraHTTPHeaders(ErrorString*, const RefPtr<InspectorObject>& headers)
{
    m_state->setObject(ResourceAgentState::extraRequestHeaders, headers);
}

void InspectorResourceAgent::getResponseBody(ErrorString* errorString, const String& requestId, String* content, bool* base64Encoded)
{
    NetworkResourcesData::ResourceData const* resourceData = m_resourcesData->data(requestId);
    if (!resourceData) {
        *errorString = "No resource with given identifier found";
        return;
    }

    if (resourceData->hasContent()) {
        *base64Encoded = resourceData->base64Encoded();
        *content = resourceData->content();
        return;
    }

    if (resourceData->buffer() && !resourceData->textEncodingName().isNull()) {
        *base64Encoded = false;
        if (InspectorPageAgent::sharedBufferContent(resourceData->buffer(), resourceData->textEncodingName(), *base64Encoded, content))
            return;
    }

    if (resourceData->cachedResource()) {
        if (InspectorPageAgent::cachedResourceContent(resourceData->cachedResource(), content, base64Encoded))
            return;
    }

    *errorString = "No data found for resource with given identifier";
}

void InspectorResourceAgent::replayXHR(ErrorString*, const String& requestId)
{
    RefPtr<XMLHttpRequest> xhr = XMLHttpRequest::create(m_pageAgent->mainFrame()->document());
    ExceptionCode code;
    String actualRequestId = requestId;

    XHRReplayData* xhrReplayData = m_resourcesData->xhrReplayData(requestId);
    if (!xhrReplayData)
        return;

    CachedResource* cachedResource = memoryCache()->resourceForURL(xhrReplayData->url());
    if (cachedResource)
        memoryCache()->remove(cachedResource);

    xhr->open(xhrReplayData->method(), xhrReplayData->url(), xhrReplayData->async(), code);
    HTTPHeaderMap::const_iterator end = xhrReplayData->headers().end();
    for (HTTPHeaderMap::const_iterator it = xhrReplayData->headers().begin(); it!= end; ++it)
        xhr->setRequestHeader(it->first, it->second, code);
    xhr->sendFromInspector(xhrReplayData->formData(), code);
}

void InspectorResourceAgent::canClearBrowserCache(ErrorString*, bool* result)
{
    *result = m_client->canClearBrowserCache();
}

void InspectorResourceAgent::clearBrowserCache(ErrorString*)
{
    m_client->clearBrowserCache();
}

void InspectorResourceAgent::canClearBrowserCookies(ErrorString*, bool* result)
{
    *result = m_client->canClearBrowserCookies();
}

void InspectorResourceAgent::clearBrowserCookies(ErrorString*)
{
    m_client->clearBrowserCookies();
}

void InspectorResourceAgent::setCacheDisabled(ErrorString*, bool cacheDisabled)
{
    m_state->setBoolean(ResourceAgentState::cacheDisabled, cacheDisabled);
    if (cacheDisabled)
        memoryCache()->evictResources();
}

void InspectorResourceAgent::mainFrameNavigated(DocumentLoader* loader)
{
    if (m_state->getBoolean(ResourceAgentState::cacheDisabled))
        memoryCache()->evictResources();

    m_resourcesData->clear(m_pageAgent->loaderId(loader));
}

InspectorResourceAgent::InspectorResourceAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorClient* client, InspectorState* state)
    : InspectorBaseAgent<InspectorResourceAgent>("Network", instrumentingAgents, state)
    , m_pageAgent(pageAgent)
    , m_client(client)
    , m_frontend(0)
    , m_resourcesData(adoptPtr(new NetworkResourcesData()))
    , m_loadingXHRSynchronously(false)
    , m_isRecalculatingStyle(false)
{
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
