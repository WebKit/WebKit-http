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

#include "config.h"
#include "CopiedSpace.h"

#include "CopiedSpaceInlineMethods.h"
#include "GCActivityCallback.h"

namespace JSC {

CopiedSpace::CopiedSpace(Heap* heap)
    : m_heap(heap)
    , m_toSpace(0)
    , m_fromSpace(0)
    , m_inCopyingPhase(false)
    , m_numberOfLoanedBlocks(0)
{
    m_toSpaceLock.Init();
}

CopiedSpace::~CopiedSpace()
{
    while (!m_toSpace->isEmpty())
        m_heap->blockAllocator().deallocate(CopiedBlock::destroy(m_toSpace->removeHead()));

    while (!m_fromSpace->isEmpty())
        m_heap->blockAllocator().deallocate(CopiedBlock::destroy(m_fromSpace->removeHead()));

    while (!m_oversizeBlocks.isEmpty())
        CopiedBlock::destroy(m_oversizeBlocks.removeHead()).deallocate();
}

void CopiedSpace::init()
{
    m_toSpace = &m_blocks1;
    m_fromSpace = &m_blocks2;
    
    allocateBlock();
}   

CheckedBoolean CopiedSpace::tryAllocateSlowCase(size_t bytes, void** outPtr)
{
    if (isOversize(bytes))
        return tryAllocateOversize(bytes, outPtr);
    
    ASSERT(m_heap->globalData()->apiLock().currentThreadIsHoldingLock());
    m_heap->didAllocate(m_allocator.currentCapacity());

    allocateBlock();

    *outPtr = m_allocator.forceAllocate(bytes);
    return true;
}

CheckedBoolean CopiedSpace::tryAllocateOversize(size_t bytes, void** outPtr)
{
    ASSERT(isOversize(bytes));
    
    size_t blockSize = WTF::roundUpToMultipleOf(WTF::pageSize(), sizeof(CopiedBlock) + bytes);

    PageAllocationAligned allocation = PageAllocationAligned::allocate(blockSize, WTF::pageSize(), OSAllocator::JSGCHeapPages);
    if (!static_cast<bool>(allocation)) {
        *outPtr = 0;
        return false;
    }

    CopiedBlock* block = CopiedBlock::create(allocation);
    m_oversizeBlocks.push(block);
    m_blockFilter.add(reinterpret_cast<Bits>(block));
    m_blockSet.add(block);
    
    CopiedAllocator allocator;
    allocator.setCurrentBlock(block);
    *outPtr = allocator.forceAllocate(bytes);
    allocator.resetCurrentBlock();

    m_heap->didAllocate(blockSize);

    return true;
}

CheckedBoolean CopiedSpace::tryReallocate(void** ptr, size_t oldSize, size_t newSize)
{
    if (oldSize >= newSize)
        return true;
    
    void* oldPtr = *ptr;
    ASSERT(!m_heap->globalData()->isInitializingObject());
    
    if (isOversize(oldSize) || isOversize(newSize))
        return tryReallocateOversize(ptr, oldSize, newSize);
    
    if (m_allocator.tryReallocate(oldPtr, oldSize, newSize))
        return true;

    void* result = 0;
    if (!tryAllocate(newSize, &result)) {
        *ptr = 0;
        return false;
    }
    memcpy(result, oldPtr, oldSize);
    *ptr = result;
    return true;
}

CheckedBoolean CopiedSpace::tryReallocateOversize(void** ptr, size_t oldSize, size_t newSize)
{
    ASSERT(isOversize(oldSize) || isOversize(newSize));
    ASSERT(newSize > oldSize);

    void* oldPtr = *ptr;
    
    void* newPtr = 0;
    if (!tryAllocateOversize(newSize, &newPtr)) {
        *ptr = 0;
        return false;
    }

    memcpy(newPtr, oldPtr, oldSize);

    if (isOversize(oldSize)) {
        CopiedBlock* oldBlock = oversizeBlockFor(oldPtr);
        m_oversizeBlocks.remove(oldBlock);
        m_blockSet.remove(oldBlock);
        CopiedBlock::destroy(oldBlock).deallocate();
    }
    
    *ptr = newPtr;
    return true;
}

void CopiedSpace::doneFillingBlock(CopiedBlock* block)
{
    ASSERT(m_inCopyingPhase);
    
    if (!block)
        return;

    if (!block->dataSize()) {
        recycleBlock(block);
        return;
    }

    block->zeroFillWilderness();

    {
        SpinLockHolder locker(&m_toSpaceLock);
        m_toSpace->push(block);
        m_blockSet.add(block);
        m_blockFilter.add(reinterpret_cast<Bits>(block));
    }

    {
        MutexLocker locker(m_loanedBlocksLock);
        ASSERT(m_numberOfLoanedBlocks > 0);
        m_numberOfLoanedBlocks--;
        if (!m_numberOfLoanedBlocks)
            m_loanedBlocksCondition.signal();
    }
}

void CopiedSpace::doneCopying()
{
    {
        MutexLocker locker(m_loanedBlocksLock);
        while (m_numberOfLoanedBlocks > 0)
            m_loanedBlocksCondition.wait(m_loanedBlocksLock);
    }

    ASSERT(m_inCopyingPhase);
    m_inCopyingPhase = false;
    while (!m_fromSpace->isEmpty()) {
        CopiedBlock* block = m_fromSpace->removeHead();
        if (block->m_isPinned) {
            block->m_isPinned = false;
            // We don't add the block to the blockSet because it was never removed.
            ASSERT(m_blockSet.contains(block));
            m_blockFilter.add(reinterpret_cast<Bits>(block));
            m_toSpace->push(block);
            continue;
        }

        m_blockSet.remove(block);
        m_heap->blockAllocator().deallocate(CopiedBlock::destroy(block));
    }

    CopiedBlock* curr = m_oversizeBlocks.head();
    while (curr) {
        CopiedBlock* next = curr->next();
        if (!curr->m_isPinned) {
            m_oversizeBlocks.remove(curr);
            m_blockSet.remove(curr);
            CopiedBlock::destroy(curr).deallocate();
        } else {
            m_blockFilter.add(reinterpret_cast<Bits>(curr));
            curr->m_isPinned = false;
        }
        curr = next;
    }

    if (!m_toSpace->head())
        allocateBlock();
    else
        m_allocator.setCurrentBlock(m_toSpace->head());
}

size_t CopiedSpace::size()
{
    size_t calculatedSize = 0;

    for (CopiedBlock* block = m_toSpace->head(); block; block = block->next())
        calculatedSize += block->size();

    for (CopiedBlock* block = m_fromSpace->head(); block; block = block->next())
        calculatedSize += block->size();

    for (CopiedBlock* block = m_oversizeBlocks.head(); block; block = block->next())
        calculatedSize += block->size();

    return calculatedSize;
}

size_t CopiedSpace::capacity()
{
    size_t calculatedCapacity = 0;

    for (CopiedBlock* block = m_toSpace->head(); block; block = block->next())
        calculatedCapacity += block->capacity();

    for (CopiedBlock* block = m_fromSpace->head(); block; block = block->next())
        calculatedCapacity += block->capacity();

    for (CopiedBlock* block = m_oversizeBlocks.head(); block; block = block->next())
        calculatedCapacity += block->capacity();

    return calculatedCapacity;
}

static bool isBlockListPagedOut(double deadline, DoublyLinkedList<CopiedBlock>* list)
{
    unsigned itersSinceLastTimeCheck = 0;
    CopiedBlock* current = list->head();
    while (current) {
        current = current->next();
        ++itersSinceLastTimeCheck;
        if (itersSinceLastTimeCheck >= Heap::s_timeCheckResolution) {
            double currentTime = WTF::monotonicallyIncreasingTime();
            if (currentTime > deadline)
                return true;
            itersSinceLastTimeCheck = 0;
        }
    }

    return false;
}

bool CopiedSpace::isPagedOut(double deadline)
{
    return isBlockListPagedOut(deadline, m_toSpace) 
        || isBlockListPagedOut(deadline, m_fromSpace) 
        || isBlockListPagedOut(deadline, &m_oversizeBlocks);
}

} // namespace JSC
