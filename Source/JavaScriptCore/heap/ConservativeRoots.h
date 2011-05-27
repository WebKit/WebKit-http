/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
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

#ifndef ConservativeRoots_h
#define ConservativeRoots_h

#include "Heap.h"
#include <wtf/OSAllocator.h>
#include <wtf/Vector.h>

namespace JSC {

class JSCell;
class Heap;

class ConservativeRoots {
public:
    ConservativeRoots(Heap*);
    ~ConservativeRoots();

    void add(void*);
    void add(void* begin, void* end);
    
    size_t size();
    JSCell** roots();

private:
    static const size_t inlineCapacity = 128;
    static const size_t nonInlineCapacity = 8192 / sizeof(JSCell*);
    
    void grow();

    Heap* m_heap;
    JSCell** m_roots;
    size_t m_size;
    size_t m_capacity;
    JSCell* m_inlineRoots[inlineCapacity];
};

inline ConservativeRoots::ConservativeRoots(Heap* heap)
    : m_heap(heap)
    , m_roots(m_inlineRoots)
    , m_size(0)
    , m_capacity(inlineCapacity)
{
}

inline ConservativeRoots::~ConservativeRoots()
{
    if (m_roots != m_inlineRoots)
        OSAllocator::decommitAndRelease(m_roots, m_capacity * sizeof(JSCell*));
}

inline void ConservativeRoots::add(void* p)
{
    if (!m_heap->contains(p))
        return;

    // The conservative set inverts the typical meaning of mark bits: We only
    // visit marked pointers, and our visit clears the mark bit. This efficiently
    // sifts out pointers to dead objects and duplicate pointers.
    if (!m_heap->testAndClearMarked(p))
        return;

    if (m_size == m_capacity)
        grow();

    m_roots[m_size++] = static_cast<JSCell*>(p);
}

inline size_t ConservativeRoots::size()
{
    return m_size;
}

inline JSCell** ConservativeRoots::roots()
{
    return m_roots;
}

} // namespace JSC

#endif // ConservativeRoots_h
