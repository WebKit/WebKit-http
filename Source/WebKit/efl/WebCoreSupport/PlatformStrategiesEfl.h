/*
    Copyright (C) 2012 Intel Corporation

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef PlatformStrategiesEfl_h
#define PlatformStrategiesEfl_h

#include "CookiesStrategy.h"
#include "LoaderStrategy.h"
#include "PasteboardStrategy.h"
#include "PlatformStrategies.h"
#include "PluginStrategy.h"
#include "SharedWorkerStrategy.h"
#include "VisitedLinkStrategy.h"

class PlatformStrategiesEfl : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy, private WebCore::LoaderStrategy, private WebCore::PluginStrategy, private WebCore::SharedWorkerStrategy, private WebCore::VisitedLinkStrategy {
public:
    static void initialize();

private:
    PlatformStrategiesEfl();

    // WebCore::PlatformStrategies
    virtual WebCore::CookiesStrategy* createCookiesStrategy();
    virtual WebCore::LoaderStrategy* createLoaderStrategy();
    virtual WebCore::PasteboardStrategy* createPasteboardStrategy();
    virtual WebCore::PluginStrategy* createPluginStrategy();
    virtual WebCore::SharedWorkerStrategy* createSharedWorkerStrategy();
    virtual WebCore::VisitedLinkStrategy* createVisitedLinkStrategy();

    // WebCore::CookiesStrategy
    virtual void notifyCookiesChanged();
    virtual String cookiesForDOM(WebCore::NetworkingContext*, const WebCore::KURL& firstParty, const WebCore::KURL&);
    virtual void setCookiesFromDOM(WebCore::NetworkingContext*, const WebCore::KURL& firstParty, const WebCore::KURL&, const String&);
    virtual bool cookiesEnabled(WebCore::NetworkingContext*, const WebCore::KURL& firstParty, const WebCore::KURL&);
    virtual String cookieRequestHeaderFieldValue(WebCore::NetworkingContext*, const WebCore::KURL& firstParty, const WebCore::KURL&);
    virtual bool getRawCookies(WebCore::NetworkingContext*, const WebCore::KURL& firstParty, const WebCore::KURL&, Vector<WebCore::Cookie>&);
    virtual void deleteCookie(WebCore::NetworkingContext*, const WebCore::KURL&, const String&);
    virtual void getHostnamesWithCookies(WebCore::NetworkingContext*, HashSet<String>& hostnames);
    virtual void deleteCookiesForHostname(WebCore::NetworkingContext*, const String& hostname);
    virtual void deleteAllCookies(WebCore::NetworkingContext*);

    // WebCore::PluginStrategy
    virtual void refreshPlugins();
    virtual void getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>&);

    // WebCore::VisitedLinkStrategy
    virtual bool isLinkVisited(WebCore::Page*, WebCore::LinkHash, const WebCore::KURL& baseURL, const WTF::AtomicString& attributeURL);
    virtual void addVisitedLink(WebCore::Page*, WebCore::LinkHash);
};

#endif // PlatformStrategiesEfl_h
