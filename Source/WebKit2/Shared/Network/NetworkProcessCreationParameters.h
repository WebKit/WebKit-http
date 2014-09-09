/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef NetworkProcessCreationParameters_h
#define NetworkProcessCreationParameters_h

#if ENABLE(NETWORK_PROCESS)

#include "CacheModel.h"
#include "SandboxExtension.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

#if USE(SOUP)
#include "HTTPCookieAcceptPolicy.h"
#endif

namespace IPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WebKit {

struct NetworkProcessCreationParameters {
    NetworkProcessCreationParameters();

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, NetworkProcessCreationParameters&);

    bool privateBrowsingEnabled;
    CacheModel cacheModel;

    String diskCacheDirectory;
    SandboxExtension::Handle diskCacheDirectoryExtensionHandle;

    String cookieStorageDirectory;

#if PLATFORM(IOS)
    SandboxExtension::Handle cookieStorageDirectoryExtensionHandle;

    // FIXME: Remove this once <rdar://problem/17726660> is fixed.
    SandboxExtension::Handle hstsDatabasePathExtensionHandle;

    SandboxExtension::Handle parentBundleDirectoryExtensionHandle;
#endif
    bool shouldUseTestingNetworkSession;

#if ENABLE(CUSTOM_PROTOCOLS)
    Vector<String> urlSchemesRegisteredForCustomProtocols;
#endif

#if PLATFORM(COCOA)
    String parentProcessName;
    String uiProcessBundleIdentifier;
    uint64_t nsURLCacheMemoryCapacity;
    uint64_t nsURLCacheDiskCapacity;

    String httpProxy;
    String httpsProxy;
#endif

#if USE(SOUP)
    String cookiePersistentStoragePath;
    uint32_t cookiePersistentStorageType;
    HTTPCookieAcceptPolicy cookieAcceptPolicy;
    bool ignoreTLSErrors;
    Vector<String> languages;
#endif
};

} // namespace WebKit

#endif // ENABLE(NETWORK_PROCESS)

#endif // NetworkProcessCreationParameters_h
