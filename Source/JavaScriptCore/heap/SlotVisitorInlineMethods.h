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

#ifndef SlotVisitorInlineMethods_h
#define SlotVisitorInlineMethods_h

#include "CopiedSpaceInlineMethods.h"
#include "Options.h"
#include "SlotVisitor.h"

namespace JSC {

ALWAYS_INLINE void SlotVisitor::append(JSValue* slot, size_t count)
{
    for (size_t i = 0; i < count; ++i) {
        JSValue& value = slot[i];
        internalAppend(value);
    }
}

template<typename T>
inline void SlotVisitor::appendUnbarrieredPointer(T** slot)
{
    ASSERT(slot);
    JSCell* cell = *slot;
    internalAppend(cell);
}

ALWAYS_INLINE void SlotVisitor::append(JSValue* slot)
{
    ASSERT(slot);
    internalAppend(*slot);
}

ALWAYS_INLINE void SlotVisitor::appendUnbarrieredValue(JSValue* slot)
{
    ASSERT(slot);
    internalAppend(*slot);
}

ALWAYS_INLINE void SlotVisitor::append(JSCell** slot)
{
    ASSERT(slot);
    internalAppend(*slot);
}

ALWAYS_INLINE void SlotVisitor::internalAppend(JSValue value)
{
    if (!value || !value.isCell())
        return;
    internalAppend(value.asCell());
}

inline void SlotVisitor::addWeakReferenceHarvester(WeakReferenceHarvester* weakReferenceHarvester)
{
    m_shared.m_weakReferenceHarvesters.addThreadSafe(weakReferenceHarvester);
}

inline void SlotVisitor::addUnconditionalFinalizer(UnconditionalFinalizer* unconditionalFinalizer)
{
    m_shared.m_unconditionalFinalizers.addThreadSafe(unconditionalFinalizer);
}

inline void SlotVisitor::addOpaqueRoot(void* root)
{
#if ENABLE(PARALLEL_GC)
    if (Options::numberOfGCMarkers() == 1) {
        // Put directly into the shared HashSet.
        m_shared.m_opaqueRoots.add(root);
        return;
    }
    // Put into the local set, but merge with the shared one every once in
    // a while to make sure that the local sets don't grow too large.
    mergeOpaqueRootsIfProfitable();
    m_opaqueRoots.add(root);
#else
    m_opaqueRoots.add(root);
#endif
}

inline bool SlotVisitor::containsOpaqueRoot(void* root)
{
    ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty());
    return m_shared.m_opaqueRoots.contains(root);
#else
    return m_opaqueRoots.contains(root);
#endif
}

inline int SlotVisitor::opaqueRootCount()
{
    ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty());
    return m_shared.m_opaqueRoots.size();
#else
    return m_opaqueRoots.size();
#endif
}

inline void SlotVisitor::mergeOpaqueRootsIfNecessary()
{
    if (m_opaqueRoots.isEmpty())
        return;
    mergeOpaqueRoots();
}
    
inline void SlotVisitor::mergeOpaqueRootsIfProfitable()
{
    if (static_cast<unsigned>(m_opaqueRoots.size()) < Options::opaqueRootMergeThreshold())
        return;
    mergeOpaqueRoots();
}
    
ALWAYS_INLINE bool SlotVisitor::checkIfShouldCopyAndPinOtherwise(void* oldPtr, size_t bytes)
{
    if (CopiedSpace::isOversize(bytes)) {
        m_shared.m_copiedSpace->pin(CopiedSpace::oversizeBlockFor(oldPtr));
        return false;
    }

    if (m_shared.m_copiedSpace->isPinned(oldPtr))
        return false;
    
    return true;
}

ALWAYS_INLINE void* SlotVisitor::allocateNewSpace(size_t bytes)
{
    void* result = 0; // Compilers don't realize that this will be assigned.
    if (LIKELY(m_copiedAllocator.tryAllocate(bytes, &result)))
        return result;
    
    result = allocateNewSpaceSlow(bytes);
    ASSERT(result);
    return result;
}       

inline void SlotVisitor::donate()
{
    ASSERT(m_isInParallelMode);
    if (Options::numberOfGCMarkers() == 1)
        return;
    
    donateKnownParallel();
}

inline void SlotVisitor::donateAndDrain()
{
    donate();
    drain();
}

} // namespace JSC

#endif // SlotVisitorInlineMethods_h

