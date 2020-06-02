/*
 * Copyright (C) 2014-2018 Apple Inc. All rights reserved.
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

#import "config.h"
#import "NetworkProcess.h"

#import "CookieStorageUtilsCF.h"
#import "Logging.h"
#import "NetworkCache.h"
#import "NetworkProcessCreationParameters.h"
#import "NetworkResourceLoader.h"
#import "NetworkSessionCocoa.h"
#import "SandboxExtension.h"
#import <WebCore/NetworkStorageSession.h>
#import <WebCore/PublicSuffix.h>
#import <WebCore/ResourceRequestCFNet.h>
#import <WebCore/RuntimeApplicationChecks.h>
#import <WebCore/SecurityOrigin.h>
#import <WebCore/SecurityOriginData.h>
#import <WebCore/SocketStreamHandleImpl.h>
#import <pal/spi/cf/CFNetworkSPI.h>
#import <wtf/BlockPtr.h>
#import <wtf/CallbackAggregator.h>
#import <wtf/ProcessPrivilege.h>
#import <wtf/RetainPtr.h>
#import <wtf/cocoa/RuntimeApplicationChecksCocoa.h>

namespace WebKit {

static void initializeNetworkSettings()
{
    static const unsigned preferredConnectionCount = 6;

    _CFNetworkHTTPConnectionCacheSetLimit(kHTTPLoadWidth, preferredConnectionCount);

    Boolean keyExistsAndHasValidFormat = false;
    Boolean prefValue = CFPreferencesGetAppBooleanValue(CFSTR("WebKitEnableHTTPPipelining"), kCFPreferencesCurrentApplication, &keyExistsAndHasValidFormat);
    if (keyExistsAndHasValidFormat)
        WebCore::ResourceRequest::setHTTPPipeliningEnabled(prefValue);

    if (WebCore::ResourceRequest::resourcePrioritiesEnabled()) {
        const unsigned fastLaneConnectionCount = 1;

        _CFNetworkHTTPConnectionCacheSetLimit(kHTTPPriorityNumLevels, WebCore::resourceLoadPriorityCount);
        _CFNetworkHTTPConnectionCacheSetLimit(kHTTPMinimumFastLanePriority, toPlatformRequestPriority(WebCore::ResourceLoadPriority::Medium));
        _CFNetworkHTTPConnectionCacheSetLimit(kHTTPNumFastLanes, fastLaneConnectionCount);
    }
}

void NetworkProcess::platformInitializeNetworkProcessCocoa(const NetworkProcessCreationParameters& parameters)
{
    WebCore::setApplicationBundleIdentifier(parameters.uiProcessBundleIdentifier);
    setApplicationSDKVersion(parameters.uiProcessSDKVersion);

#if HAVE(HSTS_STORAGE_PATH)
    if (!parameters.hstsStorageDirectory.isNull()) {
        SandboxExtension::consumePermanently(parameters.hstsStorageDirectoryExtensionHandle);
        _CFNetworkSetHSTSStoragePath(parameters.hstsStorageDirectory.createCFString().get());
    }
#endif

#if PLATFORM(IOS_FAMILY)
    SandboxExtension::consumePermanently(parameters.cookieStorageDirectoryExtensionHandle);
    SandboxExtension::consumePermanently(parameters.containerCachesDirectoryExtensionHandle);
    SandboxExtension::consumePermanently(parameters.parentBundleDirectoryExtensionHandle);
#if ENABLE(INDEXED_DATABASE)
    SandboxExtension::consumePermanently(parameters.defaultDataStoreParameters.indexedDatabaseTempBlobDirectoryExtensionHandle);
#endif
#endif

    _CFNetworkSetATSContext(parameters.networkATSContext.get());

    m_uiProcessBundleIdentifier = parameters.uiProcessBundleIdentifier;
    
    initializeNetworkSettings();

#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
    setSharedHTTPCookieStorage(parameters.uiProcessCookieStorageIdentifier);
#endif

    WebCore::NetworkStorageSession::setStorageAccessAPIEnabled(parameters.storageAccessAPIEnabled);

    // FIXME: Most of what this function does for cache size gets immediately overridden by setCacheModel().
    // - memory cache size passed from UI process is always ignored;
    // - disk cache size passed from UI process is effectively a minimum size.
    // One non-obvious constraint is that we need to use -setSharedURLCache: even in testing mode, to prevent creating a default one on disk later, when some other code touches the cache.

    m_cacheOptions = { NetworkCache::CacheOption::RegisterNotify };

    // Disable NSURLCache.
    auto urlCache(adoptNS([[NSURLCache alloc] initWithMemoryCapacity:0 diskCapacity:0 diskPath:nil]));
    [NSURLCache setSharedURLCache:urlCache.get()];
}

std::unique_ptr<WebCore::NetworkStorageSession> NetworkProcess::platformCreateDefaultStorageSession() const
{
    return makeUnique<WebCore::NetworkStorageSession>(PAL::SessionID::defaultSessionID());
}

RetainPtr<CFDataRef> NetworkProcess::sourceApplicationAuditData() const
{
#if USE(SOURCE_APPLICATION_AUDIT_DATA)
    ASSERT(parentProcessConnection());
    if (!parentProcessConnection())
        return nullptr;
    Optional<audit_token_t> auditToken = parentProcessConnection()->getAuditToken();
    if (!auditToken)
        return nullptr;
    return adoptCF(CFDataCreate(nullptr, (const UInt8*)&*auditToken, sizeof(*auditToken)));
#else
    return nullptr;
#endif
}

static void filterPreloadHSTSEntry(const void* key, const void* value, void* context)
{
    RELEASE_ASSERT(context);

    ASSERT(key);
    ASSERT(value);
    if (!key || !value)
        return;

    ASSERT(key != kCFNull);
    if (key == kCFNull)
        return;
    
    auto* hostnames = static_cast<HashSet<String>*>(context);
    auto val = static_cast<CFDictionaryRef>(value);
    if (CFDictionaryGetValue(val, _kCFNetworkHSTSPreloaded) != kCFBooleanTrue)
        hostnames->add((CFStringRef)key);
}

void NetworkProcess::getHostNamesWithHSTSCache(WebCore::NetworkStorageSession& session, HashSet<String>& hostNames)
{
    if (auto HSTSPolicies = adoptCF(_CFNetworkCopyHSTSPolicies(session.platformSession())))
        CFDictionaryApplyFunction(HSTSPolicies.get(), filterPreloadHSTSEntry, &hostNames);
}

void NetworkProcess::deleteHSTSCacheForHostNames(WebCore::NetworkStorageSession& session, const Vector<String>& hostNames)
{
    for (auto& hostName : hostNames) {
        auto url = URL({ }, makeString("https://", hostName));
        _CFNetworkResetHSTS(url.createCFURL().get(), session.platformSession());
    }
}

void NetworkProcess::clearHSTSCache(WebCore::NetworkStorageSession& session, WallTime modifiedSince)
{
    NSTimeInterval timeInterval = modifiedSince.secondsSinceEpoch().seconds();
    NSDate *date = [NSDate dateWithTimeIntervalSince1970:timeInterval];

    _CFNetworkResetHSTSHostsSinceDate(session.platformSession(), (__bridge CFDateRef)date);
}

void NetworkProcess::clearDiskCache(WallTime modifiedSince, CompletionHandler<void()>&& completionHandler)
{
    if (!m_clearCacheDispatchGroup)
        m_clearCacheDispatchGroup = dispatch_group_create();

    auto group = m_clearCacheDispatchGroup;
    dispatch_group_async(group, dispatch_get_main_queue(), makeBlockPtr([this, protectedThis = makeRef(*this), modifiedSince, completionHandler = WTFMove(completionHandler)] () mutable {
        auto aggregator = CallbackAggregator::create(WTFMove(completionHandler));
        forEachNetworkSession([modifiedSince, &aggregator](NetworkSession& session) {
            if (auto* cache = session.cache())
                cache->clear(modifiedSince, [aggregator = aggregator.copyRef()] () { });
        });
    }).get());
}

#if PLATFORM(MAC) || PLATFORM(MACCATALYST)
void NetworkProcess::setSharedHTTPCookieStorage(const Vector<uint8_t>& identifier)
{
    ASSERT(hasProcessPrivilege(ProcessPrivilege::CanAccessRawCookies));
    [NSHTTPCookieStorage _setSharedHTTPCookieStorage:adoptNS([[NSHTTPCookieStorage alloc] _initWithCFHTTPCookieStorage:cookieStorageFromIdentifyingData(identifier).get()]).get()];
}
#endif

void NetworkProcess::setStorageAccessAPIEnabled(bool enabled)
{
    WebCore::NetworkStorageSession::setStorageAccessAPIEnabled(enabled);
}

void NetworkProcess::syncAllCookies()
{
    platformSyncAllCookies([this] {
        didSyncAllCookies();
    });
}

#if HAVE(FOUNDATION_WITH_SAVE_COOKIES_WITH_COMPLETION_HANDLER)
static void saveCookies(NSHTTPCookieStorage *cookieStorage, CompletionHandler<void()>&& completionHandler)
{
    ASSERT(RunLoop::isMain());
    [cookieStorage _saveCookies:makeBlockPtr([completionHandler = WTFMove(completionHandler)]() mutable {
        // CFNetwork may call the completion block on a background queue, so we need to redispatch to the main thread.
        RunLoop::main().dispatch([completionHandler = WTFMove(completionHandler)]() mutable {
            completionHandler();
        });
    }).get()];
}
#endif

void NetworkProcess::platformSyncAllCookies(CompletionHandler<void()>&& completionHander) {
    ASSERT(hasProcessPrivilege(ProcessPrivilege::CanAccessRawCookies));
    ALLOW_DEPRECATED_DECLARATIONS_BEGIN

#if HAVE(FOUNDATION_WITH_SAVE_COOKIES_WITH_COMPLETION_HANDLER)
    RefPtr<CallbackAggregator> callbackAggregator = CallbackAggregator::create(WTFMove(completionHander));
    forEachNetworkStorageSession([&] (auto& networkStorageSession) {
        saveCookies(networkStorageSession.nsCookieStorage(), [callbackAggregator] { });
    });
#else
    _CFHTTPCookieStorageFlushCookieStores();
    completionHander();
#endif

    ALLOW_DEPRECATED_DECLARATIONS_END
}

void NetworkProcess::platformProcessDidTransitionToBackground()
{
}

void NetworkProcess::platformProcessDidTransitionToForeground()
{
}

NetworkHTTPSUpgradeChecker& NetworkProcess::networkHTTPSUpgradeChecker()
{
    if (!m_networkHTTPSUpgradeChecker)
        m_networkHTTPSUpgradeChecker = makeUnique<NetworkHTTPSUpgradeChecker>();
    return *m_networkHTTPSUpgradeChecker;
}

} // namespace WebKit
