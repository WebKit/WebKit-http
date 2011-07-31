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

#include "config.h"
#include "V8DOMMap.h"

#include "DOMData.h"
#include "DOMDataStore.h"
#include "ScopedDOMDataStore.h"
#include "V8Binding.h"
#include <wtf/MainThread.h>

namespace WebCore {

DOMDataStoreHandle::DOMDataStoreHandle()
    : m_store(adoptPtr(new ScopedDOMDataStore()))
{
    V8BindingPerIsolateData::current()->registerDOMDataStore(m_store.get());
}

DOMDataStoreHandle::~DOMDataStoreHandle()
{
    V8BindingPerIsolateData::current()->unregisterDOMDataStore(m_store.get());
}

static inline DOMDataStore& getDOMDataStore()
{
    return DOMData::getCurrentStore();
}

void enableFasterDOMStoreAccess()
{
}

DOMNodeMapping& getDOMNodeMap()
{
    return getDOMDataStore().domNodeMap();
}

DOMWrapperMap<void>& getDOMObjectMap()
{
    return getDOMDataStore().domObjectMap();
}

DOMWrapperMap<void>& getActiveDOMObjectMap()
{
    return getDOMDataStore().activeDomObjectMap();
}

#if ENABLE(SVG)

DOMWrapperMap<SVGElementInstance>& getDOMSVGElementInstanceMap()
{
    return getDOMDataStore().domSvgElementInstanceMap();
}

#endif // ENABLE(SVG)

void removeAllDOMObjects()
{
    DOMDataStore& store = getDOMDataStore();

    v8::HandleScope scope;

    // The DOM objects with the following types only exist on the main thread.
    if (isMainThread()) {
        // Remove all DOM nodes.
        DOMData::removeObjectsFromWrapperMap<Node>(&store, store.domNodeMap());

#if ENABLE(SVG)
        // Remove all SVG element instances in the wrapper map.
        DOMData::removeObjectsFromWrapperMap<SVGElementInstance>(&store, store.domSvgElementInstanceMap());
#endif
    }

    // Remove all DOM objects in the wrapper map.
    DOMData::removeObjectsFromWrapperMap<void>(&store, store.domObjectMap());

    // Remove all active DOM objects in the wrapper map.
    DOMData::removeObjectsFromWrapperMap<void>(&store, store.activeDomObjectMap());
}

void visitDOMNodes(DOMWrapperMap<Node>::Visitor* visitor)
{
    v8::HandleScope scope;

    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];

        store->domNodeMap().visit(store, visitor);
    }
}

void visitDOMObjects(DOMWrapperMap<void>::Visitor* visitor)
{
    v8::HandleScope scope;

    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];

        store->domObjectMap().visit(store, visitor);
    }
}

void visitActiveDOMObjects(DOMWrapperMap<void>::Visitor* visitor)
{
    v8::HandleScope scope;

    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];

        store->activeDomObjectMap().visit(store, visitor);
    }
}

#if ENABLE(SVG)

void visitDOMSVGElementInstances(DOMWrapperMap<SVGElementInstance>::Visitor* visitor)
{
    v8::HandleScope scope;

    DOMDataList& list = DOMDataStore::allStores();
    for (size_t i = 0; i < list.size(); ++i) {
        DOMDataStore* store = list[i];

        store->domSvgElementInstanceMap().visit(store, visitor);
    }
}

#endif

} // namespace WebCore
