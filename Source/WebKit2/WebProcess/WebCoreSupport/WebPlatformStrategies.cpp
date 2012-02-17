/*
 * Copyright (C) 2010, 2011 Apple Inc. All rights reserved.
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

#include "PluginInfoStore.h"
#include "WebContextMessages.h"
#include "WebCookieManager.h"
#include "WebCoreArgumentCoders.h"
#include "WebProcess.h"
#include <WebCore/Color.h>
#include <WebCore/Page.h>
#include <WebCore/PlatformPasteboard.h>

#if USE(CF)
#include <wtf/RetainPtr.h>
#endif

using namespace WebCore;

namespace WebKit {

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

WebPlatformStrategies::WebPlatformStrategies()
    : m_pluginCacheIsPopulated(false)
    , m_shouldRefreshPlugins(false)
{
}

CookiesStrategy* WebPlatformStrategies::createCookiesStrategy()
{
    return this;
}

PluginStrategy* WebPlatformStrategies::createPluginStrategy()
{
    return this;
}

VisitedLinkStrategy* WebPlatformStrategies::createVisitedLinkStrategy()
{
    return this;
}

PasteboardStrategy* WebPlatformStrategies::createPasteboardStrategy()
{
    return this;
}

// CookiesStrategy

void WebPlatformStrategies::notifyCookiesChanged()
{
    WebCookieManager::shared().dispatchCookiesDidChange();
}

// PluginStrategy

void WebPlatformStrategies::refreshPlugins()
{
    m_cachedPlugins.clear();
    m_pluginCacheIsPopulated = false;
    m_shouldRefreshPlugins = true;

    populatePluginCache();
}

void WebPlatformStrategies::getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>& plugins)
{
    populatePluginCache();
    plugins = m_cachedPlugins;
}

void WebPlatformStrategies::populatePluginCache()
{
    if (m_pluginCacheIsPopulated)
        return;

    ASSERT(m_cachedPlugins.isEmpty());
    
    Vector<PluginInfo> plugins;
    
    // FIXME: Should we do something in case of error here?
    WebProcess::shared().connection()->sendSync(Messages::WebContext::GetPlugins(m_shouldRefreshPlugins),
                                                Messages::WebContext::GetPlugins::Reply(plugins), 0);

    m_cachedPlugins.swap(plugins);
    
    m_shouldRefreshPlugins = false;
    m_pluginCacheIsPopulated = true;
}

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
    PlatformPasteboard(pasteboardName).getTypes(types);
}

PassRefPtr<WebCore::SharedBuffer> WebPlatformStrategies::bufferForType(const String& pasteboardType, const String& pasteboardName)
{
    return PlatformPasteboard(pasteboardName).bufferForType(pasteboardType);
}

void WebPlatformStrategies::getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).getPathnamesForType(pathnames, pasteboardType);
}

String WebPlatformStrategies::stringForType(const String& pasteboardType, const String& pasteboardName)
{
    return PlatformPasteboard(pasteboardName).stringForType(pasteboardType);
}

void WebPlatformStrategies::copy(const String& fromPasteboard, const String& toPasteboard)
{
    PlatformPasteboard(toPasteboard).copy(fromPasteboard);
}

int WebPlatformStrategies::changeCount(const WTF::String &pasteboardName)
{
    return PlatformPasteboard(pasteboardName).changeCount();
}

String WebPlatformStrategies::uniqueName()
{
    return PlatformPasteboard::uniqueName();
}

Color WebPlatformStrategies::color(const String& pasteboardName)
{
    return PlatformPasteboard(pasteboardName).color();    
}

void WebPlatformStrategies::setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).setTypes(pasteboardTypes);
}

void WebPlatformStrategies::setBufferForType(PassRefPtr<SharedBuffer> buffer, const String& pasteboardType, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).setBufferForType(buffer, pasteboardType);
}

void WebPlatformStrategies::setPathnamesForType(const Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).setPathnamesForType(pathnames, pasteboardType);
}

void WebPlatformStrategies::setStringForType(const String& string, const String& pasteboardType, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).setStringForType(string, pasteboardType);    
}
#endif

} // namespace WebKit

#endif // USE(PLATFORM_STRATEGIES)
