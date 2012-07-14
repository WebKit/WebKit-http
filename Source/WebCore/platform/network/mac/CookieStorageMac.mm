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

#import "ResourceHandle.h"

#import "WebCoreSystemInterface.h"
#import <wtf/RetainPtr.h>
#import <wtf/UnusedParam.h>

#if USE(PLATFORM_STRATEGIES)
#include "CookiesStrategy.h"
#include "PlatformStrategies.h"
#endif

using namespace WebCore;

@interface WebCookieStorageObjCAdapter : NSObject
-(void)notifyCookiesChangedOnMainThread;
-(void)cookiesChangedNotificationHandler:(NSNotification *)notification;
-(void)startListeningForCookieChangeNotifications;
-(void)stopListeningForCookieChangeNotifications;
@end

@implementation WebCookieStorageObjCAdapter

-(void)notifyCookiesChangedOnMainThread
{
#if USE(PLATFORM_STRATEGIES)
    platformStrategies()->cookiesStrategy()->notifyCookiesChanged();
#endif
}

-(void)cookiesChangedNotificationHandler:(NSNotification *)notification
{
    UNUSED_PARAM(notification);

    [self performSelectorOnMainThread:@selector(notifyCookiesChangedOnMainThread) withObject:nil waitUntilDone:FALSE];
}

-(void)startListeningForCookieChangeNotifications
{
    [[NSNotificationCenter defaultCenter] addObserver:self selector:@selector(cookiesChangedNotificationHandler:) name:NSHTTPCookieManagerCookiesChangedNotification object:[NSHTTPCookieStorage sharedHTTPCookieStorage]];
}

-(void)stopListeningForCookieChangeNotifications
{
    [[NSNotificationCenter defaultCenter] removeObserver:self name:NSHTTPCookieManagerCookiesChangedNotification object:nil];
}

@end

namespace WebCore {

void setCookieStoragePrivateBrowsingEnabled(bool enabled)
{
#if !PLATFORM(IOS) && __MAC_OS_X_VERSION_MIN_REQUIRED == 1060 && USE(CFURLSTORAGESESSIONS)
    // Don't call wkSetCookieStoragePrivateBrowsingEnabled() when cookie storage is set up via sessions.
    // That would make NSURLConnect use global private browsing cookie storage regardless of request session.
    // The global private cookie storage has different semantics - it makes new cookies non-persistent,
    // but doesn't start with a clean state.
    if (enabled && currentCFHTTPCookieStorage())
        return;
#endif

    // FIXME: When Private Browsing is enabled, the Private Browsing Cookie Storage should be
    // observed for changes, not the default Cookie Storage.

    // There is nothing to do here if sessions are supported. But we don't know if they are,
    // so enable legacy private browsing mode on sharedHTTPCookieStorage, too.
    wkSetCookieStoragePrivateBrowsingEnabled(enabled);
}

static WebCookieStorageObjCAdapter *cookieStorageAdapter;

void startObservingCookieChanges()
{
    if (!cookieStorageAdapter)
        cookieStorageAdapter = [[WebCookieStorageObjCAdapter alloc] init];
    [cookieStorageAdapter startListeningForCookieChangeNotifications];
}

void stopObservingCookieChanges()
{
    // cookieStorageAdapter can be nil here, if the WebProcess crashed and was restarted between
    // when startObservingCookieChanges was called, and stopObservingCookieChanges is currently being called.
    if (!cookieStorageAdapter)
        return;
    [cookieStorageAdapter stopListeningForCookieChangeNotifications];
}

}
