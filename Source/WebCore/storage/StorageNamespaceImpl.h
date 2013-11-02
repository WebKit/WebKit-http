/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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

#ifndef StorageNamespaceImpl_h
#define StorageNamespaceImpl_h

#include "SecurityOriginHash.h"
#include "StorageArea.h"
#include "StorageNamespace.h"
#include <wtf/HashMap.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class StorageAreaImpl;

class StorageNamespaceImpl : public StorageNamespace {
public:
    static PassRefPtr<StorageNamespace> localStorageNamespace(PageGroup*);
    static PassRefPtr<StorageNamespace> transientLocalStorageNamespace(PageGroup*, SecurityOrigin*);
    static PassRefPtr<StorageNamespace> sessionStorageNamespace(Page*);
    virtual ~StorageNamespaceImpl();

    virtual PassRefPtr<StorageArea> storageArea(PassRefPtr<SecurityOrigin>)  OVERRIDE;
    virtual PassRefPtr<StorageNamespace> copy(Page* newPage) OVERRIDE;
    virtual void close() OVERRIDE;

    // Not removing the origin's StorageArea from m_storageAreaMap because
    // we're just deleting the underlying db file. If an item is added immediately
    // after file deletion, we want the same StorageArea to eventually trigger
    // a sync and for StorageAreaSync to recreate the backing db file.
    virtual void clearOriginForDeletion(SecurityOrigin*) OVERRIDE;
    virtual void clearAllOriginsForDeletion() OVERRIDE;
    virtual void sync() OVERRIDE;
    virtual void closeIdleLocalStorageDatabases() OVERRIDE;

private:
    StorageNamespaceImpl(StorageType, const String& path, unsigned quota);

    typedef HashMap<RefPtr<SecurityOrigin>, RefPtr<StorageAreaImpl>> StorageAreaMap;
    StorageAreaMap m_storageAreaMap;

    StorageType m_storageType;

    // Only used if m_storageType == LocalStorage and the path was not "" in our constructor.
    String m_path;
    RefPtr<StorageSyncManager> m_syncManager;

    // The default quota for each new storage area.
    unsigned m_quota;

    bool m_isShutdown;
};

} // namespace WebCore

#endif // StorageNamespaceImpl_h
