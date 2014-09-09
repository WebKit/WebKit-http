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

#ifndef DiskCacheMonitor_h
#define DiskCacheMonitor_h

#include "ResourceRequest.h"
#include "SessionID.h"
#include <wtf/PassRefPtr.h>

typedef const struct _CFCachedURLResponse* CFCachedURLResponseRef;

namespace WebCore {

class SharedBuffer;

class DiskCacheMonitor {
public:
    static void monitorFileBackingStoreCreation(const ResourceRequest&, SessionID, CFCachedURLResponseRef);
    static PassRefPtr<SharedBuffer> tryGetFileBackedSharedBufferFromCFURLCachedResponse(CFCachedURLResponseRef);
    virtual ~DiskCacheMonitor() { }

protected:
    DiskCacheMonitor(const ResourceRequest&, SessionID, CFCachedURLResponseRef);

    virtual void resourceBecameFileBacked(PassRefPtr<SharedBuffer>);

    const ResourceRequest& resourceRequest() const { return m_resourceRequest; }
    SessionID sessionID() const { return m_sessionID; }

private:
    ResourceRequest m_resourceRequest;
    SessionID m_sessionID;
};

#if (PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED < 80000) || (PLATFORM(MAC) && __MAC_OS_X_VERSION_MIN_REQUIRED < 1090)

void DiskCacheMonitor::monitorFileBackingStoreCreation(const ResourceRequest&, SessionID, CFCachedURLResponseRef) { }
PassRefPtr<SharedBuffer> DiskCacheMonitor::tryGetFileBackedSharedBufferFromCFURLCachedResponse(CFCachedURLResponseRef) { return nullptr; }

#endif
} // namespace WebKit

#endif // DiskCacheMonitor_h
