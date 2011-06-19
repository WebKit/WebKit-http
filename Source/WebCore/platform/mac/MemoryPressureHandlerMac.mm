/*
 * Copyright (C) 2011 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#import "config.h"
#import "MemoryPressureHandler.h"

#import <WebCore/GCController.h>
#import <WebCore/FontCache.h>
#import <WebCore/MemoryCache.h>
#import <WebCore/PageCache.h>
#import <wtf/FastMalloc.h>

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)
#import <dispatch/dispatch.h>
#import <notify.h>

#ifndef DISPATCH_SOURCE_TYPE_VM
#define DISPATCH_SOURCE_TYPE_VM (&_dispatch_source_type_vm)
DISPATCH_EXPORT const struct dispatch_source_type_s _dispatch_source_type_vm;

enum {
 DISPATCH_VM_PRESSURE = 0x80000000,
};
#endif

#endif

namespace WebCore {

#if !defined(BUILDING_ON_LEOPARD) && !defined(BUILDING_ON_SNOW_LEOPARD)

static dispatch_source_t _cache_event_source = 0;

void MemoryPressureHandler::install()
{
    static int notifyToken;

    if (m_installed)
        return;

    dispatch_async(dispatch_get_main_queue(), ^{
        _cache_event_source = dispatch_source_create(DISPATCH_SOURCE_TYPE_VM, 0, DISPATCH_VM_PRESSURE, dispatch_get_main_queue());
        if (_cache_event_source) {
            dispatch_set_context(_cache_event_source, this);
            dispatch_source_set_event_handler(_cache_event_source, ^{ memoryPressureHandler().respondToMemoryPressure();});
            dispatch_resume(_cache_event_source);
        }
    });

    notify_register_dispatch("org.WebKit.lowMemory", &notifyToken,
         dispatch_get_main_queue(), ^(int) { memoryPressureHandler().respondToMemoryPressure();});

    m_installed = true;
}

void MemoryPressureHandler::respondToMemoryPressure()
{
    int savedPageCacheCapacity = pageCache()->capacity();
    pageCache()->setCapacity(pageCache()->pageCount()/2);
    pageCache()->setCapacity(savedPageCacheCapacity);
    pageCache()->releaseAutoreleasedPagesNow();

    NSURLCache *nsurlCache = [NSURLCache sharedURLCache];
    NSUInteger savedNsurlCacheMemoryCapacity = [nsurlCache memoryCapacity];
    [nsurlCache setMemoryCapacity:[nsurlCache currentMemoryUsage]/2];
    [nsurlCache setMemoryCapacity:savedNsurlCacheMemoryCapacity];
 
    fontCache()->purgeInactiveFontData();

    memoryCache()->pruneToPercentage(0.5f);

    gcController().garbageCollectNow();

    WTF::releaseFastMallocFreeMemory();
}
#endif

} // namespace WebCore
