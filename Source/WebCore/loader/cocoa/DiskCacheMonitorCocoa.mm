/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#import "DiskCacheMonitorCocoa.h"

#import "CachedResource.h"
#import "MemoryCache.h"
#import "ResourceRequest.h"
#import "SessionID.h"
#import "SharedBuffer.h"
#import <wtf/MainThread.h>
#import <wtf/OwnPtr.h>
#import <wtf/PassOwnPtr.h>
#import <wtf/PassRefPtr.h>
#import <wtf/RefPtr.h>

#ifdef __has_include
#if __has_include(<CFNetwork/CFURLCachePriv.h>)
#include <CFNetwork/CFURLCachePriv.h>
#endif
#endif

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090)

typedef void (^CFCachedURLResponseCallBackBlock)(CFCachedURLResponseRef);
extern "C" void _CFCachedURLResponseSetBecameFileBackedCallBackBlock(CFCachedURLResponseRef, CFCachedURLResponseCallBackBlock, dispatch_queue_t);
extern "C" CFDataRef _CFCachedURLResponseGetMemMappedData(CFCachedURLResponseRef);

namespace WebCore {

// The maximum number of seconds we'll try to wait for a resource to be disk cached before we forget the request.
static const double diskCacheMonitorTimeout = 20;

PassRefPtr<SharedBuffer> DiskCacheMonitor::tryGetFileBackedSharedBufferFromCFURLCachedResponse(CFCachedURLResponseRef cachedResponse)
{
    CFDataRef data = _CFCachedURLResponseGetMemMappedData(cachedResponse);
    if (!data)
        return nullptr;

    return SharedBuffer::wrapCFData(data);
}

void DiskCacheMonitor::monitorFileBackingStoreCreation(const ResourceRequest& request, SessionID sessionID, CFCachedURLResponseRef cachedResponse)
{
    if (!cachedResponse)
        return;

    new DiskCacheMonitor(request, sessionID, cachedResponse); // Balanced by adoptPtr in the blocks setup in the constructor, one of which is guaranteed to run.
}

DiskCacheMonitor::DiskCacheMonitor(const ResourceRequest& request, SessionID sessionID, CFCachedURLResponseRef cachedResponse)
    : m_resourceRequest(request)
    , m_sessionID(sessionID)
{
    ASSERT(isMainThread());

    // Set up a delayed callback to cancel this monitor if the resource hasn't been cached yet.
    __block DiskCacheMonitor* rawMonitor = this;

    dispatch_after(dispatch_time(DISPATCH_TIME_NOW, NSEC_PER_SEC * diskCacheMonitorTimeout), dispatch_get_main_queue(), ^{
        adoptPtr(rawMonitor); // Balanced by `new DiskCacheMonitor` in monitorFileBackingStoreCreation.
        rawMonitor = 0;
    });

    // Set up the disk caching callback to create the ShareableResource and send it to the WebProcess.
    CFCachedURLResponseCallBackBlock block = ^(CFCachedURLResponseRef cachedResponse)
    {
        // If the monitor isn't there then it timed out before this resource was cached to disk.
        if (!rawMonitor)
            return;

        OwnPtr<DiskCacheMonitor> monitor = adoptPtr(rawMonitor); // Balanced by `new DiskCacheMonitor` in monitorFileBackingStoreCreation.
        rawMonitor = 0;

        RefPtr<SharedBuffer> fileBackedBuffer = DiskCacheMonitor::tryGetFileBackedSharedBufferFromCFURLCachedResponse(cachedResponse);
        if (!fileBackedBuffer)
            return;

        monitor->resourceBecameFileBacked(fileBackedBuffer);
    };

    _CFCachedURLResponseSetBecameFileBackedCallBackBlock(cachedResponse, block, dispatch_get_main_queue());
}

void DiskCacheMonitor::resourceBecameFileBacked(PassRefPtr<SharedBuffer> fileBackedBuffer)
{
    CachedResource* resource = memoryCache()->resourceForRequest(m_resourceRequest, m_sessionID);
    if (!resource)
        return;

    resource->tryReplaceEncodedData(fileBackedBuffer);
}


} // namespace WebCore

#endif // (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED >= 1090)
