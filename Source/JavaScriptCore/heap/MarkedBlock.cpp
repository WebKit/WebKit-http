/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "MarkedBlock.h"

#include "JSCell.h"
#include "JSObject.h"
#include "ScopeChain.h"

namespace JSC {

MarkedBlock* MarkedBlock::create(Heap* heap, size_t cellSize)
{
    PageAllocationAligned allocation = PageAllocationAligned::allocate(blockSize, blockSize, OSAllocator::JSGCHeapPages);
    if (!static_cast<bool>(allocation))
        CRASH();
    return new (allocation.base()) MarkedBlock(allocation, heap, cellSize);
}

MarkedBlock* MarkedBlock::recycle(MarkedBlock* block, size_t cellSize)
{
    return new (block) MarkedBlock(block->m_allocation, block->m_heap, cellSize);
}

void MarkedBlock::destroy(MarkedBlock* block)
{
    block->m_allocation.deallocate();
}

MarkedBlock::MarkedBlock(const PageAllocationAligned& allocation, Heap* heap, size_t cellSize)
    : m_atomsPerCell((cellSize + atomSize - 1) / atomSize)
    , m_endAtom(atomsPerBlock - m_atomsPerCell + 1)
    , m_state(New) // All cells start out unmarked.
    , m_allocation(allocation)
    , m_heap(heap)
{
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);
}

inline void MarkedBlock::callDestructor(JSCell* cell, void* jsFinalObjectVPtr)
{
    // A previous eager sweep may already have run cell's destructor.
    if (cell->isZapped())
        return;

    void* vptr = cell->vptr();
#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_heap->m_destroyedTypeCounts.countVPtr(vptr);
#endif
    if (vptr != jsFinalObjectVPtr)
        cell->~JSCell();

    cell->zap();
}

template<MarkedBlock::BlockState blockState, MarkedBlock::SweepMode sweepMode>
MarkedBlock::FreeCell* MarkedBlock::specializedSweep()
{
    ASSERT(blockState != Allocated && blockState != FreeListed);

    // This produces a free list that is ordered in reverse through the block.
    // This is fine, since the allocation code makes no assumptions about the
    // order of the free list.
    FreeCell* head = 0;
    void* jsFinalObjectVPtr = m_heap->globalData()->jsFinalObjectVPtr;
    for (size_t i = firstAtom(); i < m_endAtom; i += m_atomsPerCell) {
        if (blockState == Marked && m_marks.get(i))
            continue;

        JSCell* cell = reinterpret_cast<JSCell*>(&atoms()[i]);
        if (blockState == Zapped && !cell->isZapped())
            continue;

        if (blockState != New)
            callDestructor(cell, jsFinalObjectVPtr);

        if (sweepMode == SweepToFreeList) {
            FreeCell* freeCell = reinterpret_cast<FreeCell*>(cell);
            freeCell->next = head;
            head = freeCell;
        }
    }

    m_state = ((sweepMode == SweepToFreeList) ? FreeListed : Zapped);
    return head;
}

MarkedBlock::FreeCell* MarkedBlock::sweep(SweepMode sweepMode)
{
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);

    switch (m_state) {
    case New:
        ASSERT(sweepMode == SweepToFreeList);
        return specializedSweep<New, SweepToFreeList>();
    case FreeListed:
        // Happens when a block transitions to fully allocated.
        ASSERT(sweepMode == SweepToFreeList);
        return 0;
    case Allocated:
        ASSERT_NOT_REACHED();
        return 0;
    case Marked:
        return sweepMode == SweepToFreeList
            ? specializedSweep<Marked, SweepToFreeList>()
            : specializedSweep<Marked, SweepOnly>();
    case Zapped:
        return sweepMode == SweepToFreeList
            ? specializedSweep<Zapped, SweepToFreeList>()
            : specializedSweep<Zapped, SweepOnly>();
    }

    ASSERT_NOT_REACHED();
    return 0;
}

void MarkedBlock::zapFreeList(FreeCell* firstFreeCell)
{
    HEAP_LOG_BLOCK_STATE_TRANSITION(this);

    // Roll back to a coherent state for Heap introspection. Cells newly
    // allocated from our free list are not currently marked, so we need another
    // way to tell what's live vs dead. We use zapping for that.

    FreeCell* next;
    for (FreeCell* current = firstFreeCell; current; current = next) {
        next = current->next;
        reinterpret_cast<JSCell*>(current)->zap();
    }

    m_state = Zapped;
}

} // namespace JSC
