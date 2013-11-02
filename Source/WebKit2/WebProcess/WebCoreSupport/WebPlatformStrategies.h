/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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

#ifndef WebPlatformStrategies_h
#define WebPlatformStrategies_h

#include <WebCore/CookiesStrategy.h>
#include <WebCore/DatabaseStrategy.h>
#include <WebCore/LoaderStrategy.h>
#include <WebCore/PasteboardStrategy.h>
#include <WebCore/PlatformStrategies.h>
#include <WebCore/PluginStrategy.h>
#include <WebCore/SharedWorkerStrategy.h>
#include <WebCore/StorageStrategy.h>
#include <WebCore/VisitedLinkStrategy.h>

namespace WebKit {

class WebPlatformStrategies : public WebCore::PlatformStrategies, private WebCore::CookiesStrategy, private WebCore::DatabaseStrategy, private WebCore::LoaderStrategy, private WebCore::PasteboardStrategy, private WebCore::PluginStrategy, private WebCore::SharedWorkerStrategy, private WebCore::StorageStrategy, private WebCore::VisitedLinkStrategy {
public:
    static void initialize();
    
private:
    WebPlatformStrategies();
    
    // WebCore::PlatformStrategies
    virtual WebCore::CookiesStrategy* createCookiesStrategy() OVERRIDE;
    virtual WebCore::DatabaseStrategy* createDatabaseStrategy() OVERRIDE;
    virtual WebCore::LoaderStrategy* createLoaderStrategy() OVERRIDE;
    virtual WebCore::PasteboardStrategy* createPasteboardStrategy() OVERRIDE;
    virtual WebCore::PluginStrategy* createPluginStrategy() OVERRIDE;
    virtual WebCore::SharedWorkerStrategy* createSharedWorkerStrategy() OVERRIDE;
    virtual WebCore::StorageStrategy* createStorageStrategy() OVERRIDE;
    virtual WebCore::VisitedLinkStrategy* createVisitedLinkStrategy() OVERRIDE;

    // WebCore::CookiesStrategy
    virtual String cookiesForDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) OVERRIDE;
    virtual void setCookiesFromDOM(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, const String&) OVERRIDE;
    virtual bool cookiesEnabled(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) OVERRIDE;
    virtual String cookieRequestHeaderFieldValue(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&) OVERRIDE;
    virtual bool getRawCookies(const WebCore::NetworkStorageSession&, const WebCore::URL& firstParty, const WebCore::URL&, Vector<WebCore::Cookie>&) OVERRIDE;
    virtual void deleteCookie(const WebCore::NetworkStorageSession&, const WebCore::URL&, const String&) OVERRIDE;

    // WebCore::DatabaseStrategy
#if ENABLE(SQL_DATABASE)
    virtual WebCore::AbstractDatabaseServer* getDatabaseServer() OVERRIDE;
#endif
#if ENABLE(INDEXED_DATABASE)
    virtual PassRefPtr<WebCore::IDBFactoryBackendInterface> createIDBFactoryBackend(const String& databaseDirectoryIdentifier) OVERRIDE;
#endif

    // WebCore::LoaderStrategy
#if ENABLE(NETWORK_PROCESS)
    virtual WebCore::ResourceLoadScheduler* resourceLoadScheduler() OVERRIDE;
    virtual void loadResourceSynchronously(WebCore::NetworkingContext*, unsigned long resourceLoadIdentifier, const WebCore::ResourceRequest&, WebCore::StoredCredentials, WebCore::ClientCredentialPolicy, WebCore::ResourceError&, WebCore::ResourceResponse&, Vector<char>& data) OVERRIDE;
#if ENABLE(BLOB)
    virtual WebCore::BlobRegistry* createBlobRegistry() OVERRIDE;
#endif
#endif

    // WebCore::PluginStrategy
    virtual void refreshPlugins() OVERRIDE;
    virtual void getPluginInfo(const WebCore::Page*, Vector<WebCore::PluginInfo>&) OVERRIDE;

    // WebCore::SharedWorkerStrategy
    virtual bool isAvailable() const OVERRIDE;

    // WebCore::StorageStrategy.
    virtual PassRefPtr<WebCore::StorageNamespace> localStorageNamespace(WebCore::PageGroup*) OVERRIDE;
    virtual PassRefPtr<WebCore::StorageNamespace> transientLocalStorageNamespace(WebCore::PageGroup*, WebCore::SecurityOrigin*) OVERRIDE;
    virtual PassRefPtr<WebCore::StorageNamespace> sessionStorageNamespace(WebCore::Page*) OVERRIDE;

    // WebCore::VisitedLinkStrategy
    virtual bool isLinkVisited(WebCore::Page*, WebCore::LinkHash, const WebCore::URL& baseURL, const WTF::AtomicString& attributeURL) OVERRIDE;
    virtual void addVisitedLink(WebCore::Page*, WebCore::LinkHash) OVERRIDE;

    // WebCore::PasteboardStrategy
#if PLATFORM(IOS)
    virtual void writeToPasteboard(const WebCore::PasteboardWebContent&) OVERRIDE;
    virtual void writeToPasteboard(const WebCore::PasteboardImage&) OVERRIDE;
    virtual void writeToPasteboard(const String& pasteboardType, const String&) OVERRIDE;
    virtual int getPasteboardItemsCount() OVERRIDE;
    virtual String readStringFromPasteboard(int index, const String& pasteboardType) OVERRIDE;
    virtual PassRefPtr<WebCore::SharedBuffer> readBufferFromPasteboard(int index, const String& pasteboardType) OVERRIDE;
    virtual WebCore::URL readURLFromPasteboard(int index, const String& pasteboardType) OVERRIDE;
    virtual long changeCount() OVERRIDE;
#endif
#if PLATFORM(MAC)
    virtual void getTypes(Vector<String>& types, const String& pasteboardName) OVERRIDE;
    virtual PassRefPtr<WebCore::SharedBuffer> bufferForType(const String& pasteboardType, const String& pasteboardName) OVERRIDE;
    virtual void getPathnamesForType(Vector<String>& pathnames, const String& pasteboardType, const String& pasteboardName) OVERRIDE;
    virtual String stringForType(const String& pasteboardType, const String& pasteboardName) OVERRIDE;
    virtual long changeCount(const String& pasteboardName) OVERRIDE;
    virtual String uniqueName() OVERRIDE;
    virtual WebCore::Color color(const String& pasteboardName) OVERRIDE;
    virtual WebCore::URL url(const String& pasteboardName) OVERRIDE;

    virtual long addTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) OVERRIDE;
    virtual long setTypes(const Vector<String>& pasteboardTypes, const String& pasteboardName) OVERRIDE;
    virtual long copy(const String& fromPasteboard, const String& toPasteboard) OVERRIDE;
    virtual long setBufferForType(PassRefPtr<WebCore::SharedBuffer>, const String& pasteboardType, const String& pasteboardName) OVERRIDE;
    virtual long setPathnamesForType(const Vector<String>&, const String& pasteboardType, const String& pasteboardName) OVERRIDE;
    virtual long setStringForType(const String&, const String& pasteboardType, const String& pasteboardName) OVERRIDE;
#endif

#if ENABLE(NETSCAPE_PLUGIN_API)
    // WebCore::PluginStrategy implementation.
    void populatePluginCache();
    bool m_pluginCacheIsPopulated;
    bool m_shouldRefreshPlugins;
    Vector<WebCore::PluginInfo> m_cachedPlugins;
    Vector<WebCore::PluginInfo> m_cachedApplicationPlugins;
#endif // ENABLE(NETSCAPE_PLUGIN_API)
};

#if ENABLE(NETSCAPE_PLUGIN_API)
void handleDidGetPlugins(uint64_t requestID, const Vector<WebCore::PluginInfo>&, const Vector<WebCore::PluginInfo>& applicationPlugins);
#endif // ENABLE(NETSCAPE_PLUGIN_API)

} // namespace WebKit

#endif // WebPlatformStrategies_h
