/*
 * Copyright (C) 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef WebProcessCreationParameters_h
#define WebProcessCreationParameters_h

#include "CacheModel.h"
#include "SandboxExtension.h"
#include "TextCheckerState.h"
#include <WebCore/SessionIDHash.h>
#include <wtf/RetainPtr.h>
#include <wtf/Vector.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

#if PLATFORM(COCOA)
#include "MachPort.h"

#endif

#if USE(SOUP)
#include "HTTPCookieAcceptPolicy.h"
#endif

namespace API {
class Data;
}

namespace IPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

struct WebProcessCreationParameters {
    WebProcessCreationParameters();

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, WebProcessCreationParameters&);

    String injectedBundlePath;
    SandboxExtension::Handle injectedBundlePathExtensionHandle;

    String applicationCacheDirectory;    
    SandboxExtension::Handle applicationCacheDirectoryExtensionHandle;
    String webSQLDatabaseDirectory;
    SandboxExtension::Handle webSQLDatabaseDirectoryExtensionHandle;
    String diskCacheDirectory;
    SandboxExtension::Handle diskCacheDirectoryExtensionHandle;
    String cookieStorageDirectory;
#if PLATFORM(IOS)
    SandboxExtension::Handle cookieStorageDirectoryExtensionHandle;
    SandboxExtension::Handle openGLCacheDirectoryExtensionHandle;
    SandboxExtension::Handle containerTemporaryDirectoryExtensionHandle;
    // FIXME: Remove this once <rdar://problem/17726660> is fixed.
    SandboxExtension::Handle hstsDatabasePathExtensionHandle;
#endif

    bool shouldUseTestingNetworkSession;

    Vector<String> urlSchemesRegistererdAsEmptyDocument;
    Vector<String> urlSchemesRegisteredAsSecure;
    Vector<String> urlSchemesForWhichDomainRelaxationIsForbidden;
    Vector<String> urlSchemesRegisteredAsLocal;
    Vector<String> urlSchemesRegisteredAsNoAccess;
    Vector<String> urlSchemesRegisteredAsDisplayIsolated;
    Vector<String> urlSchemesRegisteredAsCORSEnabled;
#if ENABLE(CACHE_PARTITIONING)
    Vector<String> urlSchemesRegisteredAsCachePartitioned;
#endif
#if ENABLE(CUSTOM_PROTOCOLS)
    Vector<String> urlSchemesRegisteredForCustomProtocols;
#endif
#if USE(SOUP)
#if !ENABLE(CUSTOM_PROTOCOLS)
    Vector<String> urlSchemesRegistered;
#endif
    String cookiePersistentStoragePath;
    uint32_t cookiePersistentStorageType;
    HTTPCookieAcceptPolicy cookieAcceptPolicy;
    bool ignoreTLSErrors;
#endif

    CacheModel cacheModel;

    bool shouldAlwaysUseComplexTextCodePath;
    bool shouldUseFontSmoothing;

    bool iconDatabaseEnabled;

    double terminationTimeout;

    Vector<String> languages;

    TextCheckerState textCheckerState;

    bool fullKeyboardAccessEnabled;

    double defaultRequestTimeoutInterval;

#if PLATFORM(COCOA) || USE(CFNETWORK)
    String uiProcessBundleIdentifier;
#endif

#if PLATFORM(COCOA)
    pid_t presenterApplicationPid;

    bool accessibilityEnhancedUserInterfaceEnabled;

    uint64_t nsURLCacheMemoryCapacity;
    uint64_t nsURLCacheDiskCapacity;

    IPC::MachPort acceleratedCompositingPort;

    String uiProcessBundleResourcePath;
    SandboxExtension::Handle uiProcessBundleResourcePathExtensionHandle;

    bool shouldForceScreenFontSubstitution;
    bool shouldEnableKerningAndLigaturesByDefault;
    bool shouldEnableJIT;
    bool shouldEnableFTLJIT;
    bool shouldEnableMemoryPressureReliefLogging;
    
    RefPtr<API::Data> bundleParameterData;

#endif // PLATFORM(COCOA)

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
    HashMap<String, bool> notificationPermissions;
#endif

#if ENABLE(NETWORK_PROCESS)
    bool usesNetworkProcess;
#endif

    HashMap<WebCore::SessionID, HashMap<unsigned, double>> plugInAutoStartOriginHashes;
    Vector<String> plugInAutoStartOrigins;

    bool memoryCacheDisabled;

#if ENABLE(SERVICE_CONTROLS)
    bool hasImageServices;
    bool hasSelectionServices;
    bool hasRichContentServices;
#endif
};

} // namespace WebKit

#endif // WebProcessCreationParameters_h
