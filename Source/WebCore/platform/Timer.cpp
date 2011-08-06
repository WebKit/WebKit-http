/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Timer.h"

#include "SharedTimer.h"
#include "ThreadGlobalData.h"
#include "ThreadTimers.h"
#include <limits.h>
#include <limits>
#include <math.h>
#include <wtf/CurrentTime.h>
#include <wtf/HashSet.h>
#include <wtf/Vector.h>

using namespace std;

namespace WebCore {

class TimerHeapReference;

// Timers are stored in a heap data structure, used to implement a priority queue.
// This allows us to efficiently determine which timer needs to fire the soonest.
// Then we set a single shared system timer to fire at that time.
//
// When a timer's "next fire time" changes, we need to move it around in the priority queue.

// Simple accessors to thread-specific data.
static Vector<TimerBase*>& timerHeap()
{
    return threadGlobalData().threadTimers().timerHeap();
}

// ----------------

class TimerHeapPointer {
public:
    TimerHeapPointer(TimerBase** pointer) : m_pointer(pointer) { }
    TimerHeapReference operator*() const;
    TimerBase* operator->() const { return *m_pointer; }
private:
    TimerBase** m_pointer;
};

class TimerHeapReference {
public:
    TimerHeapReference(TimerBase*& reference) : m_reference(reference) { }
    operator TimerBase*() const { return m_reference; }
    TimerHeapPointer operator&() const { return &m_reference; }
    TimerHeapReference& operator=(TimerBase*);
    TimerHeapReference& operator=(TimerHeapReference);
private:
    TimerBase*& m_reference;
};

inline TimerHeapReference TimerHeapPointer::operator*() const
{
    return *m_pointer;
}

inline TimerHeapReference& TimerHeapReference::operator=(TimerBase* timer)
{
    m_reference = timer;
    Vector<TimerBase*>& heap = timerHeap();
    if (&m_reference >= heap.data() && &m_reference < heap.data() + heap.size())
        timer->m_heapIndex = &m_reference - heap.data();
    return *this;
}

inline TimerHeapReference& TimerHeapReference::operator=(TimerHeapReference b)
{
    TimerBase* timer = b;
    return *this = timer;
}

inline void swap(TimerHeapReference a, TimerHeapReference b)
{
    TimerBase* timerA = a;
    TimerBase* timerB = b;

    // Invoke the assignment operator, since that takes care of updating m_heapIndex.
    a = timerB;
    b = timerA;
}

// ----------------

// Class to represent iterators in the heap when calling the standard library heap algorithms.
// Uses a custom pointer and reference type that update indices for pointers in the heap.
class TimerHeapIterator : public iterator<random_access_iterator_tag, TimerBase*, ptrdiff_t, TimerHeapPointer, TimerHeapReference> {
public:
    explicit TimerHeapIterator(TimerBase** pointer) : m_pointer(pointer) { checkConsistency(); }

    TimerHeapIterator& operator++() { checkConsistency(); ++m_pointer; checkConsistency(); return *this; }
    TimerHeapIterator operator++(int) { checkConsistency(1); return TimerHeapIterator(m_pointer++); }

    TimerHeapIterator& operator--() { checkConsistency(); --m_pointer; checkConsistency(); return *this; }
    TimerHeapIterator operator--(int) { checkConsistency(-1); return TimerHeapIterator(m_pointer--); }

    TimerHeapIterator& operator+=(ptrdiff_t i) { checkConsistency(); m_pointer += i; checkConsistency(); return *this; }
    TimerHeapIterator& operator-=(ptrdiff_t i) { checkConsistency(); m_pointer -= i; checkConsistency(); return *this; }

    TimerHeapReference operator*() const { return TimerHeapReference(*m_pointer); }
    TimerHeapReference operator[](ptrdiff_t i) const { return TimerHeapReference(m_pointer[i]); }
    TimerBase* operator->() const { return *m_pointer; }

private:
    void checkConsistency(ptrdiff_t offset = 0) const
    {
        ASSERT(m_pointer >= timerHeap().data());
        ASSERT(m_pointer <= timerHeap().data() + timerHeap().size());
        ASSERT_UNUSED(offset, m_pointer + offset >= timerHeap().data());
        ASSERT_UNUSED(offset, m_pointer + offset <= timerHeap().data() + timerHeap().size());
    }

    friend bool operator==(TimerHeapIterator, TimerHeapIterator);
    friend bool operator!=(TimerHeapIterator, TimerHeapIterator);
    friend bool operator<(TimerHeapIterator, TimerHeapIterator);
    friend bool operator>(TimerHeapIterator, TimerHeapIterator);
    friend bool operator<=(TimerHeapIterator, TimerHeapIterator);
    friend bool operator>=(TimerHeapIterator, TimerHeapIterator);
    
    friend TimerHeapIterator operator+(TimerHeapIterator, size_t);
    friend TimerHeapIterator operator+(size_t, TimerHeapIterator);
    
    friend TimerHeapIterator operator-(TimerHeapIterator, size_t);
    friend ptrdiff_t operator-(TimerHeapIterator, TimerHeapIterator);

    TimerBase** m_pointer;
};

inline bool operator==(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer == b.m_pointer; }
inline bool operator!=(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer != b.m_pointer; }
inline bool operator<(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer < b.m_pointer; }
inline bool operator>(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer > b.m_pointer; }
inline bool operator<=(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer <= b.m_pointer; }
inline bool operator>=(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer >= b.m_pointer; }

inline TimerHeapIterator operator+(TimerHeapIterator a, size_t b) { return TimerHeapIterator(a.m_pointer + b); }
inline TimerHeapIterator operator+(size_t a, TimerHeapIterator b) { return TimerHeapIterator(a + b.m_pointer); }

inline TimerHeapIterator operator-(TimerHeapIterator a, size_t b) { return TimerHeapIterator(a.m_pointer - b); }
inline ptrdiff_t operator-(TimerHeapIterator a, TimerHeapIterator b) { return a.m_pointer - b.m_pointer; }

// ----------------

class TimerHeapLessThanFunction {
public:
    bool operator()(TimerBase*, TimerBase*) const;
};

inline bool TimerHeapLessThanFunction::operator()(TimerBase* a, TimerBase* b) const
{
    // The comparisons below are "backwards" because the heap puts the largest 
    // element first and we want the lowest time to be the first one in the heap.
    double aFireTime = a->m_nextFireTime;
    double bFireTime = b->m_nextFireTime;
    if (bFireTime != aFireTime)
        return bFireTime < aFireTime;
    
    // We need to look at the difference of the insertion orders instead of comparing the two 
    // outright in case of overflow. 
    unsigned difference = a->m_heapInsertionOrder - b->m_heapInsertionOrder;
    return difference < numeric_limits<unsigned>::max() / 2;
}

// ----------------

TimerBase::TimerBase()
    : m_nextFireTime(0)
    , m_repeatInterval(0)
    , m_heapIndex(-1)
#ifndef NDEBUG
    , m_thread(currentThread())
#endif
{
}

TimerBase::~TimerBase()
{
    stop();
    ASSERT(!inHeap());
}

void TimerBase::start(double nextFireInterval, double repeatInterval)
{
    ASSERT(m_thread == currentThread());

    m_repeatInterval = repeatInterval;
    setNextFireTime(monotonicallyIncreasingTime() + nextFireInterval);
}

void TimerBase::stop()
{
    ASSERT(m_thread == currentThread());

    m_repeatInterval = 0;
    setNextFireTime(0);

    ASSERT(m_nextFireTime == 0);
    ASSERT(m_repeatInterval == 0);
    ASSERT(!inHeap());
}

double TimerBase::nextFireInterval() const
{
    ASSERT(isActive());
    double current = monotonicallyIncreasingTime();
    if (m_nextFireTime < current)
        return 0;
    return m_nextFireTime - current;
}

inline void TimerBase::checkHeapIndex() const
{
    ASSERT(!timerHeap().isEmpty());
    ASSERT(m_heapIndex >= 0);
    ASSERT(m_heapIndex < static_cast<int>(timerHeap().size()));
    ASSERT(timerHeap()[m_heapIndex] == this);
}

inline void TimerBase::checkConsistency() const
{
    // Timers should be in the heap if and only if they have a non-zero next fire time.
    ASSERT(inHeap() == (m_nextFireTime != 0));
    if (inHeap())
        checkHeapIndex();
}

void TimerBase::heapDecreaseKey()
{
    ASSERT(m_nextFireTime != 0);
    checkHeapIndex();
    TimerBase** heapData = timerHeap().data();
    push_heap(TimerHeapIterator(heapData), TimerHeapIterator(heapData + m_heapIndex + 1), TimerHeapLessThanFunction());
    checkHeapIndex();
}

inline void TimerBase::heapDelete()
{
    ASSERT(m_nextFireTime == 0);
    heapPop();
    timerHeap().removeLast();
    m_heapIndex = -1;
}

void TimerBase::heapDeleteMin()
{
    ASSERT(m_nextFireTime == 0);
    heapPopMin();
    timerHeap().removeLast();
    m_heapIndex = -1;
}

inline void TimerBase::heapIncreaseKey()
{
    ASSERT(m_nextFireTime != 0);
    heapPop();
    heapDecreaseKey();
}

inline void TimerBase::heapInsert()
{
    ASSERT(!inHeap());
    timerHeap().append(this);
    m_heapIndex = timerHeap().size() - 1;
    heapDecreaseKey();
}

inline void TimerBase::heapPop()
{
    // Temporarily force this timer to have the minimum key so we can pop it.
    double fireTime = m_nextFireTime;
    m_nextFireTime = -numeric_limits<double>::infinity();
    heapDecreaseKey();
    heapPopMin();
    m_nextFireTime = fireTime;
}

void TimerBase::heapPopMin()
{
    ASSERT(this == timerHeap().first());
    checkHeapIndex();
    Vector<TimerBase*>& heap = timerHeap();
    TimerBase** heapData = heap.data();
    pop_heap(TimerHeapIterator(heapData), TimerHeapIterator(heapData + heap.size()), TimerHeapLessThanFunction());
    checkHeapIndex();
    ASSERT(this == timerHeap().last());
}

void TimerBase::setNextFireTime(double newTime)
{
    ASSERT(m_thread == currentThread());

    // Keep heap valid while changing the next-fire time.
    double oldTime = m_nextFireTime;
    if (oldTime != newTime) {
        m_nextFireTime = newTime;
        static unsigned currentHeapInsertionOrder;
        m_heapInsertionOrder = currentHeapInsertionOrder++;

        bool wasFirstTimerInHeap = m_heapIndex == 0;

        if (oldTime == 0)
            heapInsert();
        else if (newTime == 0)
            heapDelete();
        else if (newTime < oldTime)
            heapDecreaseKey();
        else
            heapIncreaseKey();

        bool isFirstTimerInHeap = m_heapIndex == 0;

        if (wasFirstTimerInHeap || isFirstTimerInHeap)
            threadGlobalData().threadTimers().updateSharedTimer();
    }

    checkConsistency();
}

void TimerBase::fireTimersInNestedEventLoop()
{
    // Redirect to ThreadTimers.
    threadGlobalData().threadTimers().fireTimersInNestedEventLoop();
}

} // namespace WebCore

