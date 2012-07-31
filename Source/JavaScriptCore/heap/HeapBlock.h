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

#ifndef HeapBlock_h
#define HeapBlock_h

#include <wtf/DoublyLinkedList.h>
#include <wtf/PageAllocationAligned.h>
#include <wtf/StdLibExtras.h>

namespace JSC {

enum AllocationEffort { AllocationCanFail, AllocationMustSucceed };

#if COMPILER(GCC)
#define CLASS_IF_GCC class
#else
#define CLASS_IF_GCC
#endif

template<typename T>
class HeapBlock : public DoublyLinkedListNode<T> {
    friend CLASS_IF_GCC DoublyLinkedListNode<T>;
public:
    static const size_t s_blockSize = 64 * KB;

    static PageAllocationAligned destroy(HeapBlock* block)
    {
        static_cast<T*>(block)->~T();

        PageAllocationAligned allocation;
        std::swap(allocation, block->m_allocation);
        return allocation;
    }

    HeapBlock(const PageAllocationAligned& allocation)
        : DoublyLinkedListNode<T>()
        , m_allocation(allocation)
        , m_prev(0)
        , m_next(0)
    {
        ASSERT(m_allocation);
    }

    const PageAllocationAligned allocation() const { return m_allocation; }

private:
    PageAllocationAligned m_allocation;
    T* m_prev;
    T* m_next;
};

} // namespace JSC

#endif    
