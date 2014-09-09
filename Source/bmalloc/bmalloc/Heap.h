/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef Heap_h
#define Heap_h

#include "FixedVector.h"
#include "VMHeap.h"
#include "MediumLine.h"
#include "Mutex.h"
#include "SmallPage.h"
#include "MediumChunk.h"
#include "MediumPage.h"
#include "SegregatedFreeList.h"
#include "SmallChunk.h"
#include "SmallLine.h"
#include "Vector.h"
#include <array>
#include <mutex>

namespace bmalloc {

class BeginTag;
class EndTag;

class Heap {
public:
    Heap(std::lock_guard<StaticMutex>&);

    SmallLine* allocateSmallLine(std::lock_guard<StaticMutex>&, size_t smallSizeClass);
    void deallocateSmallLine(std::lock_guard<StaticMutex>&, SmallLine*);

    MediumLine* allocateMediumLine(std::lock_guard<StaticMutex>&);
    void deallocateMediumLine(std::lock_guard<StaticMutex>&, MediumLine*);
    
    void* allocateLarge(std::lock_guard<StaticMutex>&, size_t);
    void deallocateLarge(std::lock_guard<StaticMutex>&, void*);

    void* allocateXLarge(std::lock_guard<StaticMutex>&, size_t);
    void deallocateXLarge(std::lock_guard<StaticMutex>&, void*);

    void scavenge(std::unique_lock<StaticMutex>&, std::chrono::milliseconds sleepDuration);
    
private:
    ~Heap() = delete;

    SmallLine* allocateSmallLineSlowCase(std::lock_guard<StaticMutex>&, size_t smallSizeClass);
    MediumLine* allocateMediumLineSlowCase(std::lock_guard<StaticMutex>&);

    void* allocateLarge(Range, size_t);
    Range allocateLargeChunk();

    void splitLarge(BeginTag*, size_t, EndTag*&, Range&);
    void mergeLarge(BeginTag*&, EndTag*&, Range&);
    void mergeLargeLeft(EndTag*&, BeginTag*&, Range&, bool& hasPhysicalPages);
    void mergeLargeRight(EndTag*&, BeginTag*&, Range&, bool& hasPhysicalPages);
    
    void concurrentScavenge();
    void scavengeSmallPages(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);
    void scavengeMediumPages(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);
    void scavengeLargeRanges(std::unique_lock<StaticMutex>&, std::chrono::milliseconds);

    std::array<Vector<SmallLine*>, smallMax / alignment> m_smallLines;
    Vector<MediumLine*> m_mediumLines;

    Vector<SmallPage*> m_smallPages;
    Vector<MediumPage*> m_mediumPages;

    SegregatedFreeList m_largeRanges;

    bool m_isAllocatingPages;

    VMHeap m_vmHeap;
    AsyncTask<Heap, decltype(&Heap::concurrentScavenge)> m_scavenger;
};

inline void Heap::deallocateSmallLine(std::lock_guard<StaticMutex>& lock, SmallLine* line)
{
    BASSERT(!line->refCount(lock));
    SmallPage* page = SmallPage::get(line);
    if (page->deref(lock)) {
        m_smallPages.push(page);
        m_scavenger.run();
        return;
    }
    m_smallLines[page->smallSizeClass()].push(line);
}

inline SmallLine* Heap::allocateSmallLine(std::lock_guard<StaticMutex>& lock, size_t smallSizeClass)
{
    Vector<SmallLine*>& smallLines = m_smallLines[smallSizeClass];
    while (smallLines.size()) {
        SmallLine* line = smallLines.pop();
        SmallPage* page = SmallPage::get(line);
        if (!page->refCount(lock) || page->smallSizeClass() != smallSizeClass) // The line was promoted to the small pages list.
            continue;
        BASSERT(!line->refCount(lock));
        page->ref(lock);
        return line;
    }

    return allocateSmallLineSlowCase(lock, smallSizeClass);
}

inline void Heap::deallocateMediumLine(std::lock_guard<StaticMutex>& lock, MediumLine* line)
{
    BASSERT(!line->refCount(lock));
    MediumPage* page = MediumPage::get(line);
    if (page->deref(lock)) {
        m_mediumPages.push(page);
        m_scavenger.run();
        return;
    }
    m_mediumLines.push(line);
}

inline MediumLine* Heap::allocateMediumLine(std::lock_guard<StaticMutex>& lock)
{
    while (m_mediumLines.size()) {
        MediumLine* line = m_mediumLines.pop();
        MediumPage* page = MediumPage::get(line);
        if (!page->refCount(lock)) // The line was promoted to the medium pages list.
            continue;
        BASSERT(!line->refCount(lock));
        page->ref(lock);
        return line;
    }

    return allocateMediumLineSlowCase(lock);
}

} // namespace bmalloc

#endif // Heap_h
