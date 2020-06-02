/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import <wtf/MainThread.h>

#import <CoreFoundation/CoreFoundation.h>
#import <Foundation/NSThread.h>
#import <dispatch/dispatch.h>
#import <stdio.h>
#import <wtf/Assertions.h>
#import <wtf/HashSet.h>
#import <wtf/RetainPtr.h>
#import <wtf/SchedulePair.h>
#import <wtf/Threading.h>

#if USE(WEB_THREAD)
#import <wtf/ios/WebCoreThread.h>
#endif

@interface JSWTFMainThreadCaller : NSObject
- (void)call;
@end

@implementation JSWTFMainThreadCaller

- (void)call
{
    WTF::dispatchFunctionsFromMainThread();
}

@end

#define LOG_CHANNEL_PREFIX Log

namespace WTF {

#if RELEASE_LOG_DISABLED
WTFLogChannel LogThreading = { WTFLogChannelState::On, "Threading", WTFLogLevel::Error };
#else
WTFLogChannel LogThreading = { WTFLogChannelState::On, "Threading", WTFLogLevel::Error, LOG_CHANNEL_WEBKIT_SUBSYSTEM, OS_LOG_DEFAULT };
#endif


static JSWTFMainThreadCaller* staticMainThreadCaller;
static bool isTimerPosted; // This is only accessed on the main thread.

#if USE(WEB_THREAD)
// When the Web thread is enabled, we consider it to be the main thread, not pthread main.
static pthread_t mainThreadPthread { nullptr };
static NSThread* mainThreadNSThread { nullptr };

static Thread* sApplicationUIThread;
static Thread* sWebThread;
#endif

void initializeMainThreadPlatform()
{
    if (!pthread_main_np())
        RELEASE_LOG_FAULT(Threading, "WebKit Threading Violation - initial use of WebKit from a secondary thread.");
    ASSERT(pthread_main_np());

    ASSERT(!staticMainThreadCaller);
    staticMainThreadCaller = [[JSWTFMainThreadCaller alloc] init];
}

static void timerFired(CFRunLoopTimerRef timer, void*)
{
    CFRelease(timer);
    isTimerPosted = false;

    @autoreleasepool {
        WTF::dispatchFunctionsFromMainThread();
    }
}

static void postTimer()
{
    ASSERT(isMainThread());

    if (isTimerPosted)
        return;

    isTimerPosted = true;
    CFRunLoopAddTimer(CFRunLoopGetCurrent(), CFRunLoopTimerCreate(0, 0, 0, 0, 0, timerFired, 0), kCFRunLoopCommonModes);
}

void scheduleDispatchFunctionsOnMainThread()
{
    ASSERT(staticMainThreadCaller);
    
#if USE(WEB_THREAD)
    if (isWebThread()) {
        postTimer();
        return;
    }

    if (mainThreadPthread) {
        [staticMainThreadCaller performSelector:@selector(call) onThread:mainThreadNSThread withObject:nil waitUntilDone:NO];
        return;
    }
#else
    if (isMainThread()) {
        postTimer();
        return;
    }
#endif

    [staticMainThreadCaller performSelectorOnMainThread:@selector(call) withObject:nil waitUntilDone:NO];
}

void dispatchAsyncOnMainThreadWithWebThreadLockIfNeeded(void (^block)())
{
#if USE(WEB_THREAD)
    if (WebCoreWebThreadIsEnabled && WebCoreWebThreadIsEnabled()) {
        dispatch_async(dispatch_get_main_queue(), ^{
            WebCoreWebThreadLock();
            block();
        });
        return;
    }
#endif
    dispatch_async(dispatch_get_main_queue(), block);
}

void callOnWebThreadOrDispatchAsyncOnMainThread(void (^block)())
{
#if USE(WEB_THREAD)
    if (WebCoreWebThreadIsEnabled && WebCoreWebThreadIsEnabled()) {
        WebCoreWebThreadRun(block);
        return;
    }
#endif
    dispatch_async(dispatch_get_main_queue(), block);
}

#if USE(WEB_THREAD)

static bool webThreadIsUninitializedOrLockedOrDisabled()
{
    return !WebCoreWebThreadIsLockedOrDisabled || WebCoreWebThreadIsLockedOrDisabled();
}

bool isMainThread()
{
    return (isWebThread() || pthread_main_np()) && webThreadIsUninitializedOrLockedOrDisabled();
}

bool isUIThread()
{
    return pthread_main_np();
}

// Keep in mind that isWebThread can be called even when destroying the current thread.
bool isWebThread()
{
    return pthread_equal(pthread_self(), mainThreadPthread);
}

void initializeApplicationUIThread()
{
    ASSERT(pthread_main_np());
    sApplicationUIThread = &Thread::current();
}

void initializeWebThread()
{
    static std::once_flag initializeKey;
    std::call_once(initializeKey, [] {
        ASSERT(!pthread_main_np());
        mainThreadPthread = pthread_self();
        mainThreadNSThread = [NSThread currentThread];
        sWebThread = &Thread::current();
    });
}

bool canCurrentThreadAccessThreadLocalData(Thread& thread)
{
    Thread& currentThread = Thread::current();
    if (&thread == &currentThread)
        return true;

    if (&thread == sWebThread || &thread == sApplicationUIThread)
        return (&currentThread == sWebThread || &currentThread == sApplicationUIThread) && webThreadIsUninitializedOrLockedOrDisabled();

    return false;
}

#else

bool isMainThread()
{
    return pthread_main_np();
}

#endif // USE(WEB_THREAD)

} // namespace WTF
