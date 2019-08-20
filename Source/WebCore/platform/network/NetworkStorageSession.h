/*
 * Copyright (C) 2012-2018 Apple Inc. All rights reserved.
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

#pragma once

#include "CredentialStorage.h"
#include "PageIdentifier.h"
#include "RegistrableDomain.h"
#include <pal/SessionID.h>
#include <wtf/Function.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/WallTime.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
#include <pal/spi/cf/CFNetworkSPI.h>
#include <wtf/RetainPtr.h>
#endif

#if USE(SOUP)
#include <wtf/Function.h>
#include <wtf/glib/GRefPtr.h>
typedef struct _SoupCookieJar SoupCookieJar;
#endif

#if USE(CURL)
#include "CookieJarCurl.h"
#include "CookieJarDB.h"
#include <wtf/UniqueRef.h>
#endif

#ifdef __OBJC__
#include <objc/objc.h>
#endif

#if PLATFORM(COCOA)
#include "CookieStorageObserver.h"
#endif

#if PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QNetworkCookieJar;
QT_END_NAMESPACE
#endif

namespace WebCore {

class CurlProxySettings;
class NetworkingContext;
class ResourceRequest;

struct Cookie;
struct CookieRequestHeaderFieldProxy;
struct SameSiteInfo;

enum class IncludeSecureCookies : bool;
enum class IncludeHttpOnlyCookies : bool;

class NetworkStorageSession {
    WTF_MAKE_NONCOPYABLE(NetworkStorageSession); WTF_MAKE_FAST_ALLOCATED;
public:
    WEBCORE_EXPORT static void permitProcessToUseCookieAPI(bool);
    WEBCORE_EXPORT static bool processMayUseCookieAPI();

    PAL::SessionID sessionID() const { return m_sessionID; }
    CredentialStorage& credentialStorage() { return m_credentialStorage; }

#ifdef __OBJC__
    WEBCORE_EXPORT NSHTTPCookieStorage *nsCookieStorage() const;
#endif

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
    WEBCORE_EXPORT static RetainPtr<CFURLStorageSessionRef> createCFStorageSessionForIdentifier(CFStringRef identifier);
    WEBCORE_EXPORT NetworkStorageSession(PAL::SessionID, RetainPtr<CFURLStorageSessionRef>&&, RetainPtr<CFHTTPCookieStorageRef>&&);
    WEBCORE_EXPORT explicit NetworkStorageSession(PAL::SessionID);

    // May be null, in which case a Foundation default should be used.
    CFURLStorageSessionRef platformSession() { return m_platformSession.get(); }
    WEBCORE_EXPORT RetainPtr<CFHTTPCookieStorageRef> cookieStorage() const;
    WEBCORE_EXPORT static void setStorageAccessAPIEnabled(bool);
#elif USE(SOUP)
    WEBCORE_EXPORT explicit NetworkStorageSession(PAL::SessionID);
    ~NetworkStorageSession();

    SoupCookieJar* cookieStorage() const { return m_cookieStorage.get(); }
    void setCookieStorage(GRefPtr<SoupCookieJar>&&);
    void setCookieObserverHandler(Function<void ()>&&);
    void getCredentialFromPersistentStorage(const ProtectionSpace&, GCancellable*, Function<void (Credential&&)>&& completionHandler);
    void saveCredentialToPersistentStorage(const ProtectionSpace&, const Credential&);
#elif USE(CURL)
    WEBCORE_EXPORT NetworkStorageSession(PAL::SessionID);
    ~NetworkStorageSession();

    const CookieJarCurl& cookieStorage() const { return m_cookieStorage; };
    CookieJarDB& cookieDatabase() const;
    WEBCORE_EXPORT void setCookieDatabase(UniqueRef<CookieJarDB>&&);

    WEBCORE_EXPORT void setProxySettings(CurlProxySettings&&);
#else
    WEBCORE_EXPORT NetworkStorageSession(PAL::SessionID, NetworkingContext*);
    ~NetworkStorageSession();

    NetworkingContext* context() const;
    QNetworkCookieJar* cookieJar() const;
#endif

    WEBCORE_EXPORT bool cookiesEnabled() const;
    WEBCORE_EXPORT void setCookie(const Cookie&);
    WEBCORE_EXPORT void setCookies(const Vector<Cookie>&, const URL&, const URL& mainDocumentURL);
    WEBCORE_EXPORT void setCookiesFromDOM(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<uint64_t> frameID, Optional<PageIdentifier>, const String&) const;
    WEBCORE_EXPORT void deleteCookie(const Cookie&);
    WEBCORE_EXPORT void deleteCookie(const URL&, const String&) const;
    WEBCORE_EXPORT void deleteAllCookies();
    WEBCORE_EXPORT void deleteAllCookiesModifiedSince(WallTime);
    WEBCORE_EXPORT void deleteCookiesForHostnames(const Vector<String>& cookieHostNames);
    WEBCORE_EXPORT void deleteCookiesForHostnames(const Vector<String>& cookieHostNames, IncludeHttpOnlyCookies);
    WEBCORE_EXPORT Vector<Cookie> getAllCookies();
    WEBCORE_EXPORT Vector<Cookie> getCookies(const URL&);
    WEBCORE_EXPORT bool getRawCookies(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<uint64_t> frameID, Optional<PageIdentifier>, Vector<Cookie>&) const;
    WEBCORE_EXPORT void flushCookieStore();
    WEBCORE_EXPORT void getHostnamesWithCookies(HashSet<String>& hostnames);
    WEBCORE_EXPORT std::pair<String, bool> cookiesForDOM(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<uint64_t> frameID, Optional<PageIdentifier>, IncludeSecureCookies) const;
    WEBCORE_EXPORT std::pair<String, bool> cookieRequestHeaderFieldValue(const URL& firstParty, const SameSiteInfo&, const URL&, Optional<uint64_t> frameID, Optional<PageIdentifier>, IncludeSecureCookies) const;
    WEBCORE_EXPORT std::pair<String, bool> cookieRequestHeaderFieldValue(const CookieRequestHeaderFieldProxy&) const;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    WEBCORE_EXPORT bool shouldBlockCookies(const ResourceRequest&, Optional<uint64_t> frameID, Optional<PageIdentifier>) const;
    WEBCORE_EXPORT bool shouldBlockCookies(const URL& firstPartyForCookies, const URL& resource, Optional<uint64_t> frameID, Optional<PageIdentifier>) const;
    WEBCORE_EXPORT bool shouldBlockThirdPartyCookies(const RegistrableDomain&) const;
    WEBCORE_EXPORT void setPrevalentDomainsToBlockCookiesFor(const Vector<RegistrableDomain>&);
    WEBCORE_EXPORT void setAgeCapForClientSideCookies(Optional<Seconds>);
    WEBCORE_EXPORT void removePrevalentDomains(const Vector<RegistrableDomain>& domains);
    WEBCORE_EXPORT bool hasStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<uint64_t> frameID, PageIdentifier) const;
    WEBCORE_EXPORT Vector<String> getAllStorageAccessEntries() const;
    WEBCORE_EXPORT void grantStorageAccess(const RegistrableDomain& resourceDomain, const RegistrableDomain& firstPartyDomain, Optional<uint64_t> frameID, PageIdentifier);
    WEBCORE_EXPORT void removeStorageAccessForFrame(uint64_t frameID, PageIdentifier);
    WEBCORE_EXPORT void clearPageSpecificDataForResourceLoadStatistics(PageIdentifier);
    WEBCORE_EXPORT void removeAllStorageAccess();
    WEBCORE_EXPORT void setCacheMaxAgeCapForPrevalentResources(Seconds);
    WEBCORE_EXPORT void resetCacheMaxAgeCapForPrevalentResources();
    WEBCORE_EXPORT Optional<Seconds> maxAgeCacheCap(const ResourceRequest&);
    WEBCORE_EXPORT void didCommitCrossSiteLoadWithDataTransferFromPrevalentResource(const RegistrableDomain& toDomain, PageIdentifier);
    WEBCORE_EXPORT void resetCrossSiteLoadsWithLinkDecorationForTesting();
#endif

private:
    PAL::SessionID m_sessionID;

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
    RetainPtr<CFURLStorageSessionRef> m_platformSession;
    RetainPtr<CFHTTPCookieStorageRef> m_platformCookieStorage;
#elif USE(SOUP)
    static void cookiesDidChange(NetworkStorageSession*);

    GRefPtr<SoupCookieJar> m_cookieStorage;
    Function<void ()> m_cookieObserverHandler;
#elif USE(CURL)
    UniqueRef<CookieJarCurl> m_cookieStorage;
    mutable UniqueRef<CookieJarDB> m_cookieDatabase;
#else
    RefPtr<NetworkingContext> m_context;
#endif

    CredentialStorage m_credentialStorage;

#if ENABLE(RESOURCE_LOAD_STATISTICS)
    Optional<Seconds> clientSideCookieCap(const RegistrableDomain& firstParty, Optional<PageIdentifier>) const;
    HashSet<RegistrableDomain> m_registrableDomainsToBlockCookieFor;
    HashMap<PageIdentifier, HashMap<uint64_t, RegistrableDomain, DefaultHash<uint64_t>::Hash, WTF::UnsignedWithZeroKeyHashTraits<uint64_t>>> m_framesGrantedStorageAccess;
    HashMap<PageIdentifier, HashMap<RegistrableDomain, RegistrableDomain>> m_pagesGrantedStorageAccess;
    Optional<Seconds> m_cacheMaxAgeCapForPrevalentResources { };
    Optional<Seconds> m_ageCapForClientSideCookies { };
    Optional<Seconds> m_ageCapForClientSideCookiesShort { };
    HashMap<WebCore::PageIdentifier, RegistrableDomain> m_navigatedToWithLinkDecorationByPrevalentResource;
    bool m_navigationWithLinkDecorationTestMode = false;
#endif

#if PLATFORM(COCOA)
public:
    CookieStorageObserver& cookieStorageObserver() const;

private:
    mutable RefPtr<CookieStorageObserver> m_cookieStorageObserver;
#endif
    static bool m_processMayUseCookieAPI;
};

#if PLATFORM(COCOA) || USE(CFURLCONNECTION)
WEBCORE_EXPORT CFURLStorageSessionRef createPrivateStorageSession(CFStringRef identifier);
#endif

}
