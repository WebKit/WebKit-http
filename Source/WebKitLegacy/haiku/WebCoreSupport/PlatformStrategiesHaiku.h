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

#ifndef PlatformStrategiesHaiku_h
#define PlatformStrategiesHaiku_h

#include "CookiesStrategy.h"
#include "LoaderStrategy.h"
#include "PasteboardStrategy.h"
#include "PlatformStrategies.h"

#include <wtf/Forward.h>

class PlatformStrategiesHaiku : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy {
public:
    static void initialize();

private:
    PlatformStrategiesHaiku();

    friend class NeverDestroyed<PlatformStrategiesHaiku>;

    // WebCore::PlatformStrategies
    WebCore::LoaderStrategy* createLoaderStrategy() override;
    WebCore::PasteboardStrategy* createPasteboardStrategy() override;
    WebCore::BlobRegistry* createBlobRegistry() override;
    WebCore::CookiesStrategy* createCookiesStrategy() override;

    // WebCore::CookiesStrategy
    std::pair<String, bool> cookiesForDOM(const WebCore::NetworkStorageSession&,
		const WTF::URL& firstParty, const WebCore::SameSiteInfo&, const WTF::URL&,
		WTF::Optional<uint64_t> frameID, WTF::Optional<uint64_t> pageID,
		WebCore::IncludeSecureCookies) override;
    virtual void setCookiesFromDOM(const WebCore::NetworkStorageSession&,
		const WTF::URL& firstParty, const WebCore::SameSiteInfo&, const WTF::URL&,
		WTF::Optional<uint64_t> frameID, WTF::Optional<uint64_t> pageID, const String&);
    virtual bool cookiesEnabled(const WebCore::NetworkStorageSession&);
    std::pair<String, bool> cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession&,
		const WTF::URL& firstParty, const WebCore::SameSiteInfo&,
		const WTF::URL&, WTF::Optional<uint64_t> frameID,
		WTF::Optional<uint64_t> pageID, WebCore::IncludeSecureCookies) override;
    std::pair<String, bool> cookieRequestHeaderFieldValue(PAL::SessionID,
		const WTF::URL& firstParty, const WebCore::SameSiteInfo&, const WTF::URL&,
		WTF::Optional<uint64_t> frameID, WTF::Optional<uint64_t> pageID,
		WebCore::IncludeSecureCookies) override;
    virtual bool getRawCookies(const WebCore::NetworkStorageSession&,
		const WTF::URL& firstParty, const WebCore::SameSiteInfo&, const WTF::URL&,
		WTF::Optional<uint64_t> frameID, WTF::Optional<uint64_t> pageID, Vector<WebCore::Cookie>&);
    virtual void deleteCookie(const WebCore::NetworkStorageSession&, const WTF::URL&, const String&);
};

#endif // PlatformStrategiesHaiku_h
