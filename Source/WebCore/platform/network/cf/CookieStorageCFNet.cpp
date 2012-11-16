/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "CookieStorage.h"

#include "CookieStorageCFNet.h"

#include "ResourceHandle.h"
#include <wtf/MainThread.h>

#if PLATFORM(MAC)
#include "WebCoreSystemInterface.h"
#elif PLATFORM(WIN)
#include "LoaderRunLoopCF.h"
#include <CFNetwork/CFHTTPCookiesPriv.h>
#include <WebKitSystemInterface/WebKitSystemInterface.h>
#endif

#if USE(PLATFORM_STRATEGIES)
#include "CookiesStrategy.h"
#include "PlatformStrategies.h"
#endif

namespace WebCore {

#if PLATFORM(WIN)

static RetainPtr<CFHTTPCookieStorageRef>& cookieStorageOverride()
{
    DEFINE_STATIC_LOCAL(RetainPtr<CFHTTPCookieStorageRef>, cookieStorage, ());
    return cookieStorage;
}

#endif

RetainPtr<CFHTTPCookieStorageRef> currentCFHTTPCookieStorage(NetworkingContext* context)
{
#if PLATFORM(WIN)
    if (RetainPtr<CFHTTPCookieStorageRef>& override = cookieStorageOverride())
        return override;
#endif

    if (CFURLStorageSessionRef session = context->storageSession())
        return RetainPtr<CFHTTPCookieStorageRef>(AdoptCF, wkCopyHTTPCookieStorage(session));

#if USE(CFNETWORK)
    return wkGetDefaultHTTPCookieStorage();
#else
    // When using NSURLConnection, we also use its shared cookie storage.
    return 0;
#endif
}

RetainPtr<CFHTTPCookieStorageRef> defaultCFHTTPCookieStorage()
{
#if PLATFORM(WIN)
    if (RetainPtr<CFHTTPCookieStorageRef>& override = cookieStorageOverride())
        return override;
#endif

    return platformStrategies()->cookiesStrategy()->defaultCookieStorage();
}

#if PLATFORM(WIN)

void overrideCookieStorage(CFHTTPCookieStorageRef cookieStorage)
{
    ASSERT(isMainThread());
    // FIXME: Why don't we retain it? The only caller is an API method that takes cookie storage as a raw argument.
    cookieStorageOverride().adoptCF(cookieStorage);
}

CFHTTPCookieStorageRef overridenCookieStorage()
{
    return cookieStorageOverride().get();
}

static void notifyCookiesChangedOnMainThread(void*)
{
    ASSERT(isMainThread());

#if USE(PLATFORM_STRATEGIES)
    platformStrategies()->cookiesStrategy()->notifyCookiesChanged();
#endif
}

static void notifyCookiesChanged(CFHTTPCookieStorageRef, void *)
{
    callOnMainThread(notifyCookiesChangedOnMainThread, 0);
}

static inline CFRunLoopRef cookieStorageObserverRunLoop()
{
    // We're using the loader run loop because we need a CFRunLoop to 
    // call the CFNetwork cookie storage APIs with. Re-using the loader
    // run loop is less overhead than starting a new thread to just listen
    // for changes in cookies.
    
    // FIXME: The loaderRunLoop function name should be a little more generic.
    return loaderRunLoop();
}

void startObservingCookieChanges()
{
    ASSERT(isMainThread());

    CFRunLoopRef runLoop = cookieStorageObserverRunLoop();
    ASSERT(runLoop);

    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = defaultCFHTTPCookieStorage();
    ASSERT(cookieStorage);

    CFHTTPCookieStorageScheduleWithRunLoop(cookieStorage.get(), runLoop, kCFRunLoopCommonModes);
    CFHTTPCookieStorageAddObserver(cookieStorage.get(), runLoop, kCFRunLoopDefaultMode, notifyCookiesChanged, 0);
}

void stopObservingCookieChanges()
{
    ASSERT(isMainThread());

    CFRunLoopRef runLoop = cookieStorageObserverRunLoop();
    ASSERT(runLoop);

    RetainPtr<CFHTTPCookieStorageRef> cookieStorage = defaultCFHTTPCookieStorage();
    ASSERT(cookieStorage);

    CFHTTPCookieStorageRemoveObserver(cookieStorage.get(), runLoop, kCFRunLoopDefaultMode, notifyCookiesChanged, 0);
    CFHTTPCookieStorageUnscheduleFromRunLoop(cookieStorage.get(), runLoop, kCFRunLoopCommonModes);
}

#endif // PLATFORM(WIN)

} // namespace WebCore
