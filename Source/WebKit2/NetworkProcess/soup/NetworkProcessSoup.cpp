/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2013 Comapny 100 Inc.
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
#include "NetworkProcess.h"

#include "NetworkCache.h"
#include "NetworkProcessCreationParameters.h"
#include "ResourceCachesToClear.h"
#include "WebCookieManager.h"
#include <WebCore/CertificateInfo.h>
#include <WebCore/FileSystem.h>
#include <WebCore/NetworkStorageSession.h>
#include <WebCore/NotImplemented.h>
#include <WebCore/ResourceHandle.h>
#include <WebCore/SoupNetworkSession.h>
#include <libsoup/soup.h>
#include <wtf/RAMSize.h>
#include <wtf/glib/GRefPtr.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

using namespace WebCore;

namespace WebKit {

static CString buildAcceptLanguages(const Vector<String>& languages)
{
    size_t languagesCount = languages.size();

    // Ignore "C" locale.
    size_t cLocalePosition = languages.find("c");
    if (cLocalePosition != notFound)
        languagesCount--;

    // Fallback to "en" if the list is empty.
    if (!languagesCount)
        return "en";

    // Calculate deltas for the quality values.
    int delta;
    if (languagesCount < 10)
        delta = 10;
    else if (languagesCount < 20)
        delta = 5;
    else
        delta = 1;

    // Set quality values for each language.
    StringBuilder builder;
    for (size_t i = 0; i < languages.size(); ++i) {
        if (i == cLocalePosition)
            continue;

        if (i)
            builder.appendLiteral(", ");

        builder.append(languages[i]);

        int quality = 100 - i * delta;
        if (quality > 0 && quality < 100) {
            char buffer[8];
            g_ascii_formatd(buffer, 8, "%.2f", quality / 100.0);
            builder.append(String::format(";q=%s", buffer));
        }
    }

    return builder.toString().utf8();
}

void NetworkProcess::userPreferredLanguagesChanged(const Vector<String>& languages)
{
    auto acceptLanguages = buildAcceptLanguages(languages);
    SoupNetworkSession::setInitialAcceptLanguages(acceptLanguages);
    NetworkStorageSession::forEach([&acceptLanguages](const WebCore::NetworkStorageSession& session) {
        if (auto* soupSession = session.soupNetworkSession())
            soupSession->setAcceptLanguages(acceptLanguages);
    });
}

void NetworkProcess::setProxies(WebCore::SessionID sessionID, const Vector<WebCore::Proxy>& proxies)
{
    if (auto *storageSession = NetworkStorageSession::storageSession(sessionID)){
        storageSession->getOrCreateSoupNetworkSession().setProxies(proxies);
        return;
    }
    
    NetworkStorageSession::defaultStorageSession().getOrCreateSoupNetworkSession().setProxies(proxies);
}

void NetworkProcess::platformInitializeNetworkProcess(const NetworkProcessCreationParameters& parameters)
{
    if (parameters.proxySettings.mode != SoupNetworkProxySettings::Mode::Default)
        setNetworkProxySettings(parameters.proxySettings);

    ASSERT(!parameters.diskCacheDirectory.isEmpty());
    m_diskCacheDirectory = parameters.diskCacheDirectory;

#if ENABLE(NETWORK_CACHE)
    SoupNetworkSession::clearCache(WebCore::directoryName(m_diskCacheDirectory));

    OptionSet<NetworkCache::Cache::Option> cacheOptions;
    if (parameters.shouldEnableNetworkCacheEfficacyLogging)
        cacheOptions |= NetworkCache::Cache::Option::EfficacyLogging;
#if ENABLE(NETWORK_CACHE_SPECULATIVE_REVALIDATION)
    if (parameters.shouldEnableNetworkCacheSpeculativeRevalidation)
        cacheOptions |= NetworkCache::Cache::Option::SpeculativeRevalidation;
#endif

    NetworkCache::singleton().initialize(m_diskCacheDirectory, cacheOptions);
#else
    // We used to use the given cache directory for the soup cache, but now we use a subdirectory to avoid
    // conflicts with other cache files in the same directory. Remove the old cache files if they still exist.
    SoupNetworkSession::clearCache(WebCore::directoryName(m_diskCacheDirectory));

    GRefPtr<SoupCache> soupCache = adoptGRef(soup_cache_new(m_diskCacheDirectory.utf8().data(), SOUP_CACHE_SINGLE_USER));
    NetworkStorageSession::defaultStorageSession().getOrCreateSoupNetworkSession().setCache(soupCache.get());
    // Set an initial huge max_size for the SoupCache so the call to soup_cache_load() won't evict any cached
    // resource. The final size of the cache will be set by NetworkProcess::platformSetCacheModel().
    unsigned initialMaxSize = soup_cache_get_max_size(soupCache.get());
    soup_cache_set_max_size(soupCache.get(), G_MAXUINT);
    soup_cache_load(soupCache.get());
    soup_cache_set_max_size(soupCache.get(), initialMaxSize);
#endif

    if (!parameters.cookiePersistentStoragePath.isEmpty()) {
        supplement<WebCookieManager>()->setCookiePersistentStorage(parameters.cookiePersistentStoragePath,
            parameters.cookiePersistentStorageType);
    }
    supplement<WebCookieManager>()->setHTTPCookieAcceptPolicy(parameters.cookieAcceptPolicy, 0);

    if (!parameters.languages.isEmpty())
        userPreferredLanguagesChanged(parameters.languages);

    setIgnoreTLSErrors(parameters.ignoreTLSErrors);
}

void NetworkProcess::platformSetURLCacheSize(unsigned /*urlCacheMemoryCapacity*/, uint64_t urlCacheDiskCapacity)
{
#if !ENABLE(NETWORK_CACHE)
    soup_cache_set_max_size(NetworkStorageSession::defaultStorageSession().getOrCreateSoupNetworkSession().cache(), urlCacheDiskCapacity);
#else
    UNUSED_PARAM(urlCacheDiskCapacity);
#endif
}

void NetworkProcess::setIgnoreTLSErrors(bool ignoreTLSErrors)
{
    SoupNetworkSession::setShouldIgnoreTLSErrors(ignoreTLSErrors);
}

void NetworkProcess::allowSpecificHTTPSCertificateForHost(const CertificateInfo& certificateInfo, const String& host)
{
    SoupNetworkSession::allowSpecificHTTPSCertificateForHost(certificateInfo, host);
}

void NetworkProcess::clearCacheForAllOrigins(uint32_t cachesToClear)
{
    if (cachesToClear == InMemoryResourceCachesOnly)
        return;

    clearDiskCache(std::chrono::system_clock::time_point::min(), [] { });
}

void NetworkProcess::clearDiskCache(std::chrono::system_clock::time_point modifiedSince, std::function<void ()> completionHandler)
{
#if ENABLE(NETWORK_CACHE)
    NetworkCache::singleton().clear(modifiedSince, WTFMove(completionHandler));
#else
    UNUSED_PARAM(modifiedSince);
    UNUSED_PARAM(completionHandler);
    soup_cache_clear(NetworkStorageSession::defaultStorageSession().getOrCreateSoupNetworkSession().cache());
#endif
}

void NetworkProcess::platformTerminate()
{
    notImplemented();
}

void NetworkProcess::setNetworkProxySettings(const SoupNetworkProxySettings& settings)
{
    SoupNetworkSession::setProxySettings(settings);
    NetworkStorageSession::forEach([](const NetworkStorageSession& session) {
        if (auto* soupSession = session.soupNetworkSession())
            soupSession->setupProxy();
    });
}

} // namespace WebKit
