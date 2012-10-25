/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DOMWrapperWorld_h
#define DOMWrapperWorld_h

#include "DOMDataStore.h"
#include "SecurityOrigin.h"
#include "V8DOMMap.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

// This class represent a collection of DOM wrappers for a specific world.
class DOMWrapperWorld : public WTF::RefCountedBase {
public:
    static const int mainWorldId = 0;
    static const int mainWorldExtensionGroup = 0;
    static const int uninitializedWorldId = -1;
    static const int uninitializedExtensionGroup = -1;
    // If uninitializedWorldId is passed as worldId, the world will be assigned a temporary id instead.
    static PassRefPtr<DOMWrapperWorld> ensureIsolatedWorld(int worldId, int extensionGroup);
    static bool isolatedWorldsExist() { return isolatedWorldCount; }
    static bool isIsolatedWorldId(int worldId) { return worldId != mainWorldId && worldId != uninitializedWorldId; }
    // Associates an isolated world (see above for description) with a security
    // origin. XMLHttpRequest instances used in that world will be considered
    // to come from that origin, not the frame's.
    static void setIsolatedWorldSecurityOrigin(int worldID, PassRefPtr<SecurityOrigin>);
    static void clearIsolatedWorldSecurityOrigin(int worldID);
    SecurityOrigin* isolatedWorldSecurityOrigin();
    // FIXME: this is a workaround for a problem in WebViewImpl.
    // Do not use this anywhere else!!
    static PassRefPtr<DOMWrapperWorld> createUninitializedWorld();

    bool isMainWorld() { return m_worldId == mainWorldId; }
    bool isIsolatedWorld() { return isIsolatedWorldId(m_worldId); }
    int worldId() const { return m_worldId; }
    int extensionGroup() const { return m_extensionGroup; }
    DOMDataStore* domDataStore() const
    {
        ASSERT(m_worldId != uninitializedWorldId);
        return m_domDataStore.get();
    }
    void deref()
    {
        if (derefBase())
            deallocate(this);
    }

private:
    static int isolatedWorldCount;
    static PassRefPtr<DOMWrapperWorld> createMainWorld();
    static void deallocate(DOMWrapperWorld*);

    DOMWrapperWorld(int worldId, int extensionGroup)
        : m_worldId(worldId)
        , m_extensionGroup(extensionGroup)
    {
        if (worldId != uninitializedWorldId)
            m_domDataStore = adoptPtr(new DOMDataStore(DOMDataStore::IsolatedWorld));
    }

    const int m_worldId;
    const int m_extensionGroup;
    OwnPtr<DOMDataStore> m_domDataStore;

    friend DOMWrapperWorld* mainThreadNormalWorld();
};

DOMWrapperWorld* mainThreadNormalWorld();

} // namespace WebCore

#endif // DOMWrapperWorld_h
