/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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
#include "WebPlatformStrategies.h"

#if USE(PLATFORM_STRATEGIES)

#include "BlockingResponseMap.h"
#include "PluginInfoStore.h"
#include "WebContextMessages.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include "WebProcessProxyMessages.h"
#include <WebCore/Color.h>
#include <WebCore/KURL.h>
#include <WebCore/LoaderStrategy.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformCookieJar.h>
#include <WebCore/PlatformPasteboard.h>
#include <wtf/Atomics.h>

#if ENABLE(NETWORK_PROCESS)
#include "NetworkConnectionToWebProcessMessages.h"
#include "NetworkProcessConnection.h"
#endif

#if PLATFORM(WIN) && USE(CFNETWORK)
#include "WebFrameNetworkingContext.h"
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

using namespace WebCore;

namespace WebKit {

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

WebPlatformStrategies::WebPlatformStrategies()
#if ENABLE(NETSCAPE_PLUGIN_API)
    : m_pluginCacheIsPopulated(false)
    , m_shouldRefreshPlugins(false)
#endif // ENABLE(NETSCAPE_PLUGIN_API)
{
}

CookiesStrategy* WebPlatformStrategies::createCookiesStrategy()
{
    return this;
}

LoaderStrategy* WebPlatformStrategies::createLoaderStrategy()
{
    return this;
}

PasteboardStrategy* WebPlatformStrategies::createPasteboardStrategy()
{
    return this;
}

PluginStrategy* WebPlatformStrategies::createPluginStrategy()
{
    return this;
}

SharedWorkerStrategy* WebPlatformStrategies::createSharedWorkerStrategy()
{
    return this;
}

VisitedLinkStrategy* WebPlatformStrategies::createVisitedLinkStrategy()
{
    return this;
}

// CookiesStrategy

void WebPlatformStrategies::notifyCookiesChanged()
{
    WebCookieManager::shared().dispatchCookiesDidChange();
}

#if PLATFORM(WIN) && USE(CFNETWORK)
RetainPtr<CFHTTPCookieStorageRef> WebPlatformStrategies::defaultCookieStorage()
{
    return 0;
}
#endif

String WebPlatformStrategies::cookiesForDOM(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        String result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookiesForDOM(firstParty, url), Messages::NetworkConnectionToWebProcess::CookiesForDOM::Reply(result), 0))
            return String();
        return result;
    }
#endif

    return WebCore::cookiesForDOM(context, firstParty, url);
}

void WebPlatformStrategies::setCookiesFromDOM(NetworkingContext* context, const KURL& firstParty, const KURL& url, const String& cookieString)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::SetCookiesFromDOM(firstParty, url, cookieString), 0);
        return;
    }
#endif

    WebCore::setCookiesFromDOM(context, firstParty, url, cookieString);
}

bool WebPlatformStrategies::cookiesEnabled(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        bool result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookiesEnabled(firstParty, url), Messages::NetworkConnectionToWebProcess::CookiesEnabled::Reply(result), 0))
            return false;
        return result;
    }
#endif

    return WebCore::cookiesEnabled(context, firstParty, url);
}

String WebPlatformStrategies::cookieRequestHeaderFieldValue(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        String result;
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue(firstParty, url), Messages::NetworkConnectionToWebProcess::CookieRequestHeaderFieldValue::Reply(result), 0))
            return String();
        return result;
    }
#endif

    return WebCore::cookieRequestHeaderFieldValue(context, firstParty, url);
}

bool WebPlatformStrategies::getRawCookies(NetworkingContext* context, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        if (!WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::GetRawCookies(firstParty, url), Messages::NetworkConnectionToWebProcess::GetRawCookies::Reply(rawCookies), 0))
            return false;
        return true;
    }
#endif

    return WebCore::getRawCookies(context, firstParty, url, rawCookies);
}

void WebPlatformStrategies::deleteCookie(NetworkingContext* context, const KURL& url, const String& cookieName)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::DeleteCookie(url, cookieName), 0);
        return;
    }
#endif

    WebCore::deleteCookie(context, url, cookieName);
}

void WebPlatformStrategies::getHostnamesWithCookies(NetworkingContext* context, HashSet<String>& hostnames)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        Vector<String> hostnamesVector;
        WebProcess::shared().networkConnection()->connection()->sendSync(Messages::NetworkConnectionToWebProcess::GetHostnamesWithCookies(), Messages::NetworkConnectionToWebProcess::GetHostnamesWithCookies::Reply(hostnamesVector), 0);
        for (unsigned i = 0; i != hostnamesVector.size(); ++i)
            hostnames.add(hostnamesVector[i]);
        return;
    }
#endif

    WebCore::getHostnamesWithCookies(context, hostnames);
}

void WebPlatformStrategies::deleteCookiesForHostname(NetworkingContext* context, const String& hostname)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::DeleteCookiesForHostname(hostname), 0);
        return;
    }
#endif

    WebCore::deleteCookiesForHostname(context, hostname);
}

void WebPlatformStrategies::deleteAllCookies(NetworkingContext* context)
{
#if ENABLE(NETWORK_PROCESS)
    if (WebProcess::shared().usesNetworkProcess()) {
        WebProcess::shared().networkConnection()->connection()->send(Messages::NetworkConnectionToWebProcess::DeleteAllCookies(), 0);
        return;
    }
#endif

    WebCore::deleteAllCookies(context);
}

// LoaderStrategy

#if ENABLE(NETWORK_PROCESS)
ResourceLoadScheduler* WebPlatformStrategies::resourceLoadScheduler()
{
    static ResourceLoadScheduler* scheduler;
    if (!scheduler) {
        if (WebProcess::shared().usesNetworkProcess())
            scheduler = &WebProcess::shared().webResourceLoadScheduler();
        else
            scheduler = WebCore::resourceLoadScheduler();
    }
    
    return scheduler;
}
#endif

// PluginStrategy

void WebPlatformStrategies::refreshPlugins()
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    m_cachedPlugins.clear();
    m_pluginCacheIsPopulated = false;
    m_shouldRefreshPlugins = true;

    populatePluginCache();
#endif // ENABLE(NETSCAPE_PLUGIN_API)
}

void WebPlatformStrategies::getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>& plugins)
{
#if ENABLE(NETSCAPE_PLUGIN_API)
    populatePluginCache();
    plugins = m_cachedPlugins;
#endif // ENABLE(NETSCAPE_PLUGIN_API)
}

#if ENABLE(NETSCAPE_PLUGIN_API)
static BlockingResponseMap<Vector<WebCore::PluginInfo> >& responseMap()
{
    AtomicallyInitializedStatic(BlockingResponseMap<Vector<WebCore::PluginInfo> >&, responseMap = *new BlockingResponseMap<Vector<WebCore::PluginInfo> >);
    return responseMap;
}

void handleDidGetPlugins(uint64_t requestID, const Vector<WebCore::PluginInfo>& plugins)
{
    responseMap().didReceiveResponse(requestID, adoptPtr(new Vector<WebCore::PluginInfo>(plugins)));
}

static uint64_t generateRequestID()
{
    static int uniqueID;
    return atomicIncrement(&uniqueID);
}

void WebPlatformStrategies::populatePluginCache()
{
    if (m_pluginCacheIsPopulated)
        return;

    ASSERT(m_cachedPlugins.isEmpty());
    
    // FIXME: Should we do something in case of error here?
    uint64_t requestID = generateRequestID();
    WebProcess::shared().connection()->send(Messages::WebProcessProxy::GetPlugins(requestID, m_shouldRefreshPlugins), 0);

    m_cachedPlugins = *responseMap().waitForResponse(requestID);
    
    m_shouldRefreshPlugins = false;
    m_pluginCacheIsPopulated = true;
}
#endif // ENABLE(NETSCAPE_PLUGIN_API)

// VisitedLinkStrategy

bool WebPlatformStrategies::isLinkVisited(Page*, LinkHash linkHash, const KURL&, const AtomicString&)
{
    return WebProcess::shared().isLinkVisited(linkHash);
}

void WebPlatformStrategies::addVisitedLink(Page*, LinkHash linkHash)
{
    WebProcess::shared().addVisitedLink(linkHash);
}

#if PLATFORM(MAC)
// PasteboardStrategy

void WebPlatformStrategies::getTypes(Vector<String>& types, const String& pasteboardName)
{
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardTypes(pasteboardName),
                                                Messages::WebContext::GetPasteboardTypes::Reply(types), 0);
}

PassRefPtr<WebCore::SharedBuffer> WebPlatformStrategies::bufferForType(const String& pasteboardType, const String& pasteboardName)
{
    SharedMemory::Handle handle;
    uint64_t size = 0;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardBufferForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardBufferForType::Reply(handle, size), 0);
    if (handle.isNull())
        return 0;
    RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(handle, SharedMemory::ReadOnly);
    return SharedBuffer::create(static_cast<unsigned char *>(sharedMemoryBuffer->data()), size);
}

void WebPlatformStrategies::getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardPathnamesForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardPathnamesForType::Reply(pathnames), 0);
}

String WebPlatformStrategies::stringForType(const String& pasteboardType, const String& pasteboardName)
{
    String value;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardStringForType(pasteboardName, pasteboardType),
                                                Messages::WebContext::GetPasteboardStringForType::Reply(value), 0);
    return value;
}

void WebPlatformStrategies::copy(const String& fromPasteboard, const String& toPasteboard)
{
    WebProcess::shared().connection()->send(Messages::WebContext::PasteboardCopy(fromPasteboard, toPasteboard), 0);
}

int WebPlatformStrategies::changeCount(const WTF::String &pasteboardName)
{
    uint64_t changeCount;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardChangeCount(pasteboardName),
                                                Messages::WebContext::GetPasteboardChangeCount::Reply(changeCount), 0);
    return changeCount;
}

String WebPlatformStrategies::uniqueName()
{
    String pasteboardName;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardUniqueName(),
                                                Messages::WebContext::GetPasteboardUniqueName::Reply(pasteboardName), 0);
    return pasteboardName;
}

Color WebPlatformStrategies::color(const String& pasteboardName)
{
    Color color;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardColor(pasteboardName),
                                                Messages::WebContext::GetPasteboardColor::Reply(color), 0);
    return color;
}

KURL WebPlatformStrategies::url(const String& pasteboardName)
{
    String urlString;
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPasteboardURL(pasteboardName),
                                                Messages::WebContext::GetPasteboardURL::Reply(urlString), 0);
    return KURL(ParsedURLString, urlString);
}

void WebPlatformStrategies::addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    WebProcess::shared().connection()->send(Messages::WebContext::AddPasteboardTypes(pasteboardName, pasteboardTypes), 0);
}

void WebPlatformStrategies::setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    WebProcess::shared().connection()->send(Messages::WebContext::SetPasteboardTypes(pasteboardName, pasteboardTypes), 0);
}

void WebPlatformStrategies::setBufferForType(PassRefPtr<SharedBuffer> buffer, const String& pasteboardType, const String& pasteboardName)
{
    SharedMemory::Handle handle;
    if (buffer) {
        RefPtr<SharedMemory> sharedMemoryBuffer = SharedMemory::create(buffer->size());
        memcpy(sharedMemoryBuffer->data(), buffer->data(), buffer->size());
        sharedMemoryBuffer->createHandle(handle, SharedMemory::ReadOnly);
    }
    WebProcess::shared().connection()->send(Messages::WebContext::SetPasteboardBufferForType(pasteboardName, pasteboardType, handle, buffer ? buffer->size() : 0), 0);
}

void WebPlatformStrategies::setPathnamesForType(const Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().connection()->send(Messages::WebContext::SetPasteboardPathnamesForType(pasteboardName, pasteboardType, pathnames), 0);
}

void WebPlatformStrategies::setStringForType(const String& string, const String& pasteboardType, const String& pasteboardName)
{
    WebProcess::shared().connection()->send(Messages::WebContext::SetPasteboardStringForType(pasteboardName, pasteboardType, string), 0);
}
#endif

} // namespace WebKit

#endif // USE(PLATFORM_STRATEGIES)
