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
#include "PasteboardStrategy.h"
#include "PlatformStrategies.h"
#include "PluginStrategy.h"
#include "VisitedLinkStrategy.h"

class PlatformStrategiesEfl : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy, private WebCore::PluginStrategy, private WebCore::VisitedLinkStrategy {
public:
    static void initialize();

private:
    PlatformStrategiesEfl();

    // WebCore::PlatformStrategies
    virtual WebCore::CookiesStrategy* createCookiesStrategy();
    virtual WebCore::PluginStrategy* createPluginStrategy();
    virtual WebCore::VisitedLinkStrategy* createVisitedLinkStrategy();
    virtual WebCore::PasteboardStrategy* createPasteboardStrategy();

    // WebCore::CookiesStrategy
    virtual void notifyCookiesChanged();

    // WebCore::PluginStrategy
    virtual void refreshPlugins();
    virtual void getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>&);

    // WebCore::VisitedLinkStrategy
    virtual bool isLinkVisited(WebCore::Page*, WebCore::LinkHash, const WebCore::KURL& baseURL, const WTF::AtomicString& attributeURL);
    virtual void addVisitedLink(WebCore::Page*, WebCore::LinkHash);
};

#endif // PlatformStrategiesEfl_h
