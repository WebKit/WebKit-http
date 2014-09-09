/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef StructureInlines_h
#define StructureInlines_h

#include "JSArrayBufferView.h"
#include "PropertyMapHashTable.h"
#include "Structure.h"

namespace JSC {

inline Structure* Structure::create(VM& vm, JSGlobalObject* globalObject, JSValue prototype, const TypeInfo& typeInfo, const ClassInfo* classInfo, IndexingType indexingType, unsigned inlineCapacity)
{
    ASSERT(vm.structureStructure);
    ASSERT(classInfo);
    Structure* structure = new (NotNull, allocateCell<Structure>(vm.heap)) Structure(vm, globalObject, prototype, typeInfo, classInfo, indexingType, inlineCapacity);
    structure->finishCreation(vm);
    return structure;
}

inline Structure* Structure::createStructure(VM& vm)
{
    ASSERT(!vm.structureStructure);
    Structure* structure = new (NotNull, allocateCell<Structure>(vm.heap)) Structure(vm);
    structure->finishCreation(vm, CreatingEarlyCell);
    return structure;
}

inline Structure* Structure::create(VM& vm, Structure* structure)
{
    ASSERT(vm.structureStructure);
    Structure* newStructure = new (NotNull, allocateCell<Structure>(vm.heap)) Structure(vm, structure);
    newStructure->finishCreation(vm);
    return newStructure;
}

inline JSObject* Structure::storedPrototypeObject() const
{
    JSValue value = m_prototype.get();
    if (value.isNull())
        return nullptr;
    return asObject(value);
}

inline Structure* Structure::storedPrototypeStructure() const
{
    JSObject* object = storedPrototypeObject();
    if (!object)
        return nullptr;
    return object->structure();
}

ALWAYS_INLINE PropertyOffset Structure::get(VM& vm, PropertyName propertyName)
{
    ASSERT(!isCompilationThread());
    ASSERT(structure()->classInfo() == info());
    PropertyTable* propertyTable;
    materializePropertyMapIfNecessary(vm, propertyTable);
    if (!propertyTable)
        return invalidOffset;

    PropertyMapEntry* entry = propertyTable->get(propertyName.uid());
    return entry ? entry->offset : invalidOffset;
}
    
ALWAYS_INLINE PropertyOffset Structure::get(VM& vm, PropertyName propertyName, unsigned& attributes)
{
    ASSERT(!isCompilationThread());
    ASSERT(structure()->classInfo() == info());

    PropertyTable* propertyTable;
    materializePropertyMapIfNecessary(vm, propertyTable);
    if (!propertyTable)
        return invalidOffset;

    PropertyMapEntry* entry = propertyTable->get(propertyName.uid());
    if (!entry)
        return invalidOffset;

    attributes = entry->attributes;
    return entry->offset;
}

inline PropertyOffset Structure::getConcurrently(VM& vm, StringImpl* uid)
{
    unsigned attributesIgnored;
    return getConcurrently(vm, uid, attributesIgnored);
}

inline bool Structure::hasIndexingHeader(const JSCell* cell) const
{
    if (hasIndexedProperties(indexingType()))
        return true;
    
    if (!isTypedView(m_classInfo->typedArrayStorageType))
        return false;
    
    return jsCast<const JSArrayBufferView*>(cell)->mode() == WastefulTypedArray;
}

inline bool Structure::masqueradesAsUndefined(JSGlobalObject* lexicalGlobalObject)
{
    return typeInfo().masqueradesAsUndefined() && globalObject() == lexicalGlobalObject;
}

inline bool Structure::transitivelyTransitionedFrom(Structure* structureToFind)
{
    for (Structure* current = this; current; current = current->previousID()) {
        if (current == structureToFind)
            return true;
    }
    return false;
}

inline JSValue Structure::prototypeForLookup(JSGlobalObject* globalObject) const
{
    if (isObject())
        return m_prototype.get();

    ASSERT(typeInfo().type() == StringType);
    return globalObject->stringPrototype();
}

inline JSValue Structure::prototypeForLookup(ExecState* exec) const
{
    return prototypeForLookup(exec->lexicalGlobalObject());
}

inline StructureChain* Structure::prototypeChain(VM& vm, JSGlobalObject* globalObject) const
{
    // We cache our prototype chain so our clients can share it.
    if (!isValid(globalObject, m_cachedPrototypeChain.get())) {
        JSValue prototype = prototypeForLookup(globalObject);
        m_cachedPrototypeChain.set(vm, this, StructureChain::create(vm, prototype.isNull() ? 0 : asObject(prototype)->structure()));
    }
    return m_cachedPrototypeChain.get();
}

inline StructureChain* Structure::prototypeChain(ExecState* exec) const
{
    return prototypeChain(exec->vm(), exec->lexicalGlobalObject());
}

inline bool Structure::isValid(JSGlobalObject* globalObject, StructureChain* cachedPrototypeChain) const
{
    if (!cachedPrototypeChain)
        return false;

    JSValue prototype = prototypeForLookup(globalObject);
    WriteBarrier<Structure>* cachedStructure = cachedPrototypeChain->head();
    while (*cachedStructure && !prototype.isNull()) {
        if (asObject(prototype)->structure() != cachedStructure->get())
            return false;
        ++cachedStructure;
        prototype = asObject(prototype)->prototype();
    }
    return prototype.isNull() && !*cachedStructure;
}

inline bool Structure::isValid(ExecState* exec, StructureChain* cachedPrototypeChain) const
{
    return isValid(exec->lexicalGlobalObject(), cachedPrototypeChain);
}

inline bool Structure::putWillGrowOutOfLineStorage()
{
    checkOffsetConsistency();

    ASSERT(outOfLineCapacity() >= outOfLineSize());

    if (!propertyTable()) {
        unsigned currentSize = numberOfOutOfLineSlotsForLastOffset(m_offset);
        ASSERT(outOfLineCapacity() >= currentSize);
        return currentSize == outOfLineCapacity();
    }

    ASSERT(totalStorageCapacity() >= propertyTable()->propertyStorageSize());
    if (propertyTable()->hasDeletedOffset())
        return false;

    ASSERT(totalStorageCapacity() >= propertyTable()->size());
    return propertyTable()->size() == totalStorageCapacity();
}

ALWAYS_INLINE WriteBarrier<PropertyTable>& Structure::propertyTable()
{
    ASSERT(!globalObject() || !globalObject()->vm().heap.isCollecting());
    return m_propertyTableUnsafe;
}

inline void Structure::didReplaceProperty(PropertyOffset offset)
{
    if (LIKELY(!hasRareData()))
        return;
    StructureRareData::PropertyWatchpointMap* map = rareData()->m_replacementWatchpointSets.get();
    if (LIKELY(!map))
        return;
    WatchpointSet* set = map->get(offset);
    if (LIKELY(!set))
        return;
    set->fireAll("Property did get replaced");
}

inline WatchpointSet* Structure::propertyReplacementWatchpointSet(PropertyOffset offset)
{
    ConcurrentJITLocker locker(m_lock);
    if (!hasRareData())
        return nullptr;
    WTF::loadLoadFence();
    StructureRareData::PropertyWatchpointMap* map = rareData()->m_replacementWatchpointSets.get();
    if (!map)
        return nullptr;
    return map->get(offset);
}

ALWAYS_INLINE bool Structure::checkOffsetConsistency() const
{
    PropertyTable* propertyTable = m_propertyTableUnsafe.get();

    if (!propertyTable) {
        ASSERT(!isPinnedPropertyTable());
        return true;
    }

    // We cannot reliably assert things about the property table in the concurrent
    // compilation thread. It is possible for the table to be stolen and then have
    // things added to it, which leads to the offsets being all messed up. We could
    // get around this by grabbing a lock here, but I think that would be overkill.
    if (isCompilationThread())
        return true;
    
    RELEASE_ASSERT(numberOfSlotsForLastOffset(m_offset, m_inlineCapacity) == propertyTable->propertyStorageSize());
    unsigned totalSize = propertyTable->propertyStorageSize();
    RELEASE_ASSERT((totalSize < inlineCapacity() ? 0 : totalSize - inlineCapacity()) == numberOfOutOfLineSlotsForLastOffset(m_offset));

    return true;
}

inline size_t nextOutOfLineStorageCapacity(size_t currentCapacity)
{
    if (!currentCapacity)
        return initialOutOfLineCapacity;
    return currentCapacity * outOfLineGrowthFactor;
}

inline size_t Structure::suggestedNewOutOfLineStorageCapacity()
{
    return nextOutOfLineStorageCapacity(outOfLineCapacity());
}

} // namespace JSC

#endif // StructureInlines_h

