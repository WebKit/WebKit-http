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

#ifndef Butterfly_h
#define Butterfly_h

#include "IndexingHeader.h"
#include "PropertyOffset.h"
#include "PropertyStorage.h"
#include <wtf/Noncopyable.h>
#include <wtf/Platform.h>

namespace JSC {

class JSGlobalData;
class SlotVisitor;
struct ArrayStorage;

class Butterfly {
    WTF_MAKE_NONCOPYABLE(Butterfly);
private:
    Butterfly() { } // Not instantiable.
public:
    
    static size_t totalSize(size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes)
    {
        ASSERT(indexingPayloadSizeInBytes ? hasIndexingHeader : true);
        ASSERT(sizeof(EncodedJSValue) == sizeof(IndexingHeader));
        return (preCapacity + propertyCapacity) * sizeof(EncodedJSValue) + (hasIndexingHeader ? sizeof(IndexingHeader) : 0) + indexingPayloadSizeInBytes;
    }

    static Butterfly* fromBase(void* base, size_t preCapacity, size_t propertyCapacity)
    {
        return reinterpret_cast<Butterfly*>(static_cast<EncodedJSValue*>(base) + preCapacity + propertyCapacity + 1);
    }
    
    static ptrdiff_t offsetOfIndexingHeader() { return IndexingHeader::offsetOfIndexingHeader(); }
    static ptrdiff_t offsetOfPublicLength() { return offsetOfIndexingHeader() + IndexingHeader::offsetOfPublicLength(); }
    static ptrdiff_t offsetOfVectorLength() { return offsetOfIndexingHeader() + IndexingHeader::offsetOfVectorLength(); }
    
    static Butterfly* createUninitialized(JSGlobalData&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes);

    static Butterfly* create(JSGlobalData&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, const IndexingHeader&, size_t indexingPayloadSizeInBytes);
    static Butterfly* create(JSGlobalData&, Structure*);
    static Butterfly* createUninitializedDuringCollection(SlotVisitor&, size_t preCapacity, size_t propertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes);
    
    IndexingHeader* indexingHeader() { return IndexingHeader::from(this); }
    const IndexingHeader* indexingHeader() const { return IndexingHeader::from(this); }
    PropertyStorage propertyStorage() { return indexingHeader()->propertyStorage(); }
    ConstPropertyStorage propertyStorage() const { return indexingHeader()->propertyStorage(); }
    template<typename T>
    T* indexingPayload() { return reinterpret_cast<T*>(this); }
    ArrayStorage* arrayStorage() { return indexingPayload<ArrayStorage>(); }
    
    static ptrdiff_t offsetOfPropertyStorage() { return -static_cast<ptrdiff_t>(sizeof(IndexingHeader)); }
    static int indexOfPropertyStorage()
    {
        ASSERT(sizeof(IndexingHeader) == sizeof(EncodedJSValue));
        return -1;
    }

    void* base(size_t preCapacity, size_t propertyCapacity) { return propertyStorage() - propertyCapacity - preCapacity; }
    void* base(Structure*);
    
    // The butterfly reallocation methods perform the reallocation itself but do not change any
    // of the meta-data to reflect that the reallocation occurred. Note that this set of
    // methods is not exhaustive and is not intended to encapsulate all possible allocation
    // modes of butterflies - there are code paths that allocate butterflies by calling
    // directly into Heap::tryAllocateStorage.
    Butterfly* growPropertyStorage(JSGlobalData&, size_t preCapacity, size_t oldPropertyCapacity, bool hasIndexingHeader, size_t indexingPayloadSizeInBytes, size_t newPropertyCapacity);
    Butterfly* growPropertyStorage(JSGlobalData&, Structure* oldStructure, size_t oldPropertyCapacity, size_t newPropertyCapacity);
    Butterfly* growPropertyStorage(JSGlobalData&, Structure* oldStructure, size_t newPropertyCapacity);
    Butterfly* growArrayRight(JSGlobalData&, Structure* oldStructure, size_t propertyCapacity, bool hadIndexingHeader, size_t oldIndexingPayloadSizeInBytes, size_t newIndexingPayloadSizeInBytes); // Assumes that preCapacity is zero, and asserts as much.
    Butterfly* growArrayRight(JSGlobalData&, Structure*, size_t newIndexingPayloadSizeInBytes);
    Butterfly* resizeArray(JSGlobalData&, size_t propertyCapacity, bool oldHasIndexingHeader, size_t oldIndexingPayloadSizeInBytes, size_t newPreCapacity, bool newHasIndexingHeader, size_t newIndexingPayloadSizeInBytes);
    Butterfly* resizeArray(JSGlobalData&, Structure*, size_t newPreCapacity, size_t newIndexingPayloadSizeInBytes); // Assumes that you're not changing whether or not the object has an indexing header.
    Butterfly* unshift(Structure*, size_t numberOfSlots);
    Butterfly* shift(Structure*, size_t numberOfSlots);
};

} // namespace JSC

#endif // Butterfly_h

