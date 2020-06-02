/*
 * Copyright (C) 2017-2018 Apple Inc. All rights reserved.
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

#include "bmalloc.h"

#include "DebugHeap.h"
#include "Environment.h"
#include "PerProcess.h"

namespace bmalloc { namespace api {

void* mallocOutOfLine(size_t size, HeapKind kind)
{
    return malloc(size, kind);
}

void freeOutOfLine(void* object, HeapKind kind)
{
    free(object, kind);
}

void* tryLargeZeroedMemalignVirtual(size_t requiredAlignment, size_t requestedSize, HeapKind kind)
{
    RELEASE_BASSERT(isPowerOfTwo(requiredAlignment));

    size_t pageSize = vmPageSize();
    size_t alignment = roundUpToMultipleOf(pageSize, requiredAlignment);
    size_t size = roundUpToMultipleOf(pageSize, requestedSize);
    RELEASE_BASSERT(alignment >= requiredAlignment);
    RELEASE_BASSERT(size >= requestedSize);

    void* result;
    if (auto* debugHeap = DebugHeap::tryGet())
        result = debugHeap->memalignLarge(alignment, size);
    else {
        kind = mapToActiveHeapKind(kind);
        Heap& heap = PerProcess<PerHeapKind<Heap>>::get()->at(kind);

        UniqueLockHolder lock(Heap::mutex());
        result = heap.allocateLarge(lock, alignment, size, FailureAction::ReturnNull);
        if (result) {
            // Don't track this as dirty memory that dictates how we drive the scavenger.
            // FIXME: We should make it so that users of this API inform bmalloc which
            // pages they dirty:
            // https://bugs.webkit.org/show_bug.cgi?id=184207
            heap.externalDecommit(lock, result, size);
        }
    }

    if (result)
        vmZeroAndPurge(result, size);
    return result;
}

void freeLargeVirtual(void* object, size_t size, HeapKind kind)
{
    if (auto* debugHeap = DebugHeap::tryGet()) {
        debugHeap->freeLarge(object);
        return;
    }
    kind = mapToActiveHeapKind(kind);
    Heap& heap = PerProcess<PerHeapKind<Heap>>::get()->at(kind);
    UniqueLockHolder lock(Heap::mutex());
    // Balance out the externalDecommit when we allocated the zeroed virtual memory.
    heap.externalCommit(lock, object, size);
    heap.deallocateLarge(lock, object);
}

void scavenge()
{
    scavengeThisThread();

    if (DebugHeap* debugHeap = DebugHeap::tryGet())
        debugHeap->scavenge();
    else
        Scavenger::get()->scavenge();
}

bool isEnabled(HeapKind)
{
    return !Environment::get()->isDebugHeapEnabled();
}

#if BOS(DARWIN)
void setScavengerThreadQOSClass(qos_class_t overrideClass)
{
    if (DebugHeap::tryGet())
        return;
    UniqueLockHolder lock(Heap::mutex());
    Scavenger::get()->setScavengerThreadQOSClass(overrideClass);
}
#endif

void commitAlignedPhysical(void* object, size_t size, HeapKind kind)
{
    vmValidatePhysical(object, size);
    vmAllocatePhysicalPages(object, size);
    if (!DebugHeap::tryGet())
        PerProcess<PerHeapKind<Heap>>::get()->at(kind).externalCommit(object, size);
}

void decommitAlignedPhysical(void* object, size_t size, HeapKind kind)
{
    vmValidatePhysical(object, size);
    vmDeallocatePhysicalPages(object, size);
    if (!DebugHeap::tryGet())
        PerProcess<PerHeapKind<Heap>>::get()->at(kind).externalDecommit(object, size);
}

void enableMiniMode()
{
    if (!DebugHeap::tryGet())
        Scavenger::get()->enableMiniMode();
}

void disableScavenger()
{
    if (!DebugHeap::tryGet())
        Scavenger::get()->disable();
}

} } // namespace bmalloc::api

