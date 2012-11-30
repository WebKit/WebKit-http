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

#import "WebPlatformStrategies.h"

#import "WebFrameNetworkingContext.h"
#import "WebPluginDatabase.h"
#import "WebPluginPackage.h"
#import <WebCore/BlockExceptions.h>
#import <WebCore/Color.h>
#import <WebCore/Page.h>
#import <WebCore/PageGroup.h>
#import <WebCore/PlatformCookieJar.h>
#import <WebCore/PlatformPasteboard.h>
#import <WebKitSystemInterface.h>

using namespace WebCore;

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
    setPlatformStrategies(&platformStrategies);
}

WebPlatformStrategies::WebPlatformStrategies()
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

void WebPlatformStrategies::notifyCookiesChanged()
{
}

RetainPtr<CFHTTPCookieStorageRef> WebPlatformStrategies::defaultCookieStorage()
{
    if (CFURLStorageSessionRef session = WebFrameNetworkingContext::defaultStorageSession())
        return adoptCF(WKCopyHTTPCookieStorage(session));

#if USE(CFNETWORK)
    return WKGetDefaultHTTPCookieStorage();
#else
    // When using NSURLConnection, we also use its shared cookie storage.
    return 0;
#endif
}

String WebPlatformStrategies::cookiesForDOM(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesForDOM(context, firstParty, url);
}

void WebPlatformStrategies::setCookiesFromDOM(NetworkingContext* context, const KURL& firstParty, const KURL& url, const String& cookieString)
{
    WebCore::setCookiesFromDOM(context, firstParty, url, cookieString);
}

bool WebPlatformStrategies::cookiesEnabled(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookiesEnabled(context, firstParty, url);
}

String WebPlatformStrategies::cookieRequestHeaderFieldValue(NetworkingContext* context, const KURL& firstParty, const KURL& url)
{
    return WebCore::cookieRequestHeaderFieldValue(context, firstParty, url);
}

bool WebPlatformStrategies::getRawCookies(NetworkingContext* context, const KURL& firstParty, const KURL& url, Vector<Cookie>& rawCookies)
{
    return WebCore::getRawCookies(context, firstParty, url, rawCookies);
}

void WebPlatformStrategies::deleteCookie(NetworkingContext* context, const KURL& url, const String& cookieName)
{
    WebCore::deleteCookie(context, url, cookieName);
}

void WebPlatformStrategies::getHostnamesWithCookies(NetworkingContext* context, HashSet<String>& hostnames)
{
    WebCore::getHostnamesWithCookies(context, hostnames);
}

void WebPlatformStrategies::deleteCookiesForHostname(NetworkingContext* context, const String& hostname)
{
    WebCore::deleteCookiesForHostname(context, hostname);
}

void WebPlatformStrategies::deleteAllCookies(NetworkingContext* context)
{
    WebCore::deleteAllCookies(context);
}

void WebPlatformStrategies::refreshPlugins()
{
    [[WebPluginDatabase sharedDatabase] refresh];
}

void WebPlatformStrategies::getPluginInfo(const Page*, Vector<PluginInfo>& plugins)
{
    BEGIN_BLOCK_OBJC_EXCEPTIONS;

    NSArray* pluginsArray = [[WebPluginDatabase sharedDatabase] plugins];
    for (unsigned int i = 0; i < [pluginsArray count]; ++i) {
        WebPluginPackage *plugin = [pluginsArray objectAtIndex:i];

        plugins.append([plugin pluginInfo]);
    }
    
    END_BLOCK_OBJC_EXCEPTIONS;
}

bool WebPlatformStrategies::isLinkVisited(Page* page, LinkHash hash, const KURL&, const AtomicString&)
{
    return page->group().isLinkVisited(hash);
}

void WebPlatformStrategies::addVisitedLink(Page* page, LinkHash hash)
{
    return page->group().addVisitedLinkHash(hash);
}

void WebPlatformStrategies::getTypes(Vector<String>& types, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).getTypes(types);
}

PassRefPtr<SharedBuffer> WebPlatformStrategies::bufferForType(const String& pasteboardType, const String& pasteboardName)
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

int WebPlatformStrategies::changeCount(const String &pasteboardName)
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

KURL WebPlatformStrategies::url(const String& pasteboardName)
{
    return PlatformPasteboard(pasteboardName).url();
}

void WebPlatformStrategies::addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName)
{
    PlatformPasteboard(pasteboardName).addTypes(pasteboardTypes);
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
