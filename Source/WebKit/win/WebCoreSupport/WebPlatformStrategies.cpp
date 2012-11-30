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

#include "WebFrameNetworkingContext.h"
#include <WebCore/Page.h>
#include <WebCore/PageGroup.h>
#include <WebCore/PlatformCookieJar.h>
#include <WebCore/PluginDatabase.h>
#if USE(CFNETWORK)
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

using namespace WebCore;

void WebPlatformStrategies::initialize()
{
    DEFINE_STATIC_LOCAL(WebPlatformStrategies, platformStrategies, ());
}

WebPlatformStrategies::WebPlatformStrategies()
{
    setPlatformStrategies(this);
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
    return 0;
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

#if USE(CFNETWORK)
RetainPtr<CFHTTPCookieStorageRef> WebPlatformStrategies::defaultCookieStorage()
{
    if (CFURLStorageSessionRef session = WebFrameNetworkingContext::defaultStorageSession())
        return adoptCF(wkCopyHTTPCookieStorage(session));

    return wkGetDefaultHTTPCookieStorage();
}
#endif

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
    PluginDatabase::installedPlugins()->refresh();
}

void WebPlatformStrategies::getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>& outPlugins)
{
    const Vector<PluginPackage*>& plugins = PluginDatabase::installedPlugins()->plugins();

    outPlugins.resize(plugins.size());

    for (size_t i = 0; i < plugins.size(); ++i) {
        PluginPackage* package = plugins[i];

        PluginInfo info;
        info.name = package->name();
        info.file = package->fileName();
        info.desc = package->description();

        const MIMEToDescriptionsMap& mimeToDescriptions = package->mimeToDescriptions();

        info.mimes.reserveCapacity(mimeToDescriptions.size());

        MIMEToDescriptionsMap::const_iterator end = mimeToDescriptions.end();
        for (MIMEToDescriptionsMap::const_iterator it = mimeToDescriptions.begin(); it != end; ++it) {
            MimeClassInfo mime;

            mime.type = it->key;
            mime.desc = it->value;
            mime.extensions = package->mimeToExtensions().get(mime.type);

            info.mimes.append(mime);
        }

        outPlugins[i] = info;
    }
}

bool WebPlatformStrategies::isLinkVisited(Page* page, LinkHash hash, const KURL&, const AtomicString&)
{
    return page->group().isLinkVisited(hash);
}

void WebPlatformStrategies::addVisitedLink(Page* page, LinkHash hash)
{
    page->group().addVisitedLinkHash(hash);
}
