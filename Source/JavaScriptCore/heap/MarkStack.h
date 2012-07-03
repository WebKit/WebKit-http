/*
 * Copyright (C) 2009, 2011 Apple Inc. All rights reserved.
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

#ifndef MarkStack_h
#define MarkStack_h

#include "CopiedSpace.h"
#include "HandleTypes.h"
#include "JSValue.h"
#include "Options.h"
#include "Register.h"
#include "UnconditionalFinalizer.h"
#include "VTableSpectrum.h"
#include "WeakReferenceHarvester.h"
#include <wtf/DataLog.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/OSAllocator.h>
#include <wtf/PageBlock.h>
#include <wtf/TCSpinLock.h>
#include <wtf/text/StringHash.h>
#include <wtf/Vector.h>

#if ENABLE(OBJECT_MARK_LOGGING)
#define MARK_LOG_MESSAGE0(message) dataLog(message)
#define MARK_LOG_MESSAGE1(message, arg1) dataLog(message, arg1)
#define MARK_LOG_MESSAGE2(message, arg1, arg2) dataLog(message, arg1, arg2)
#define MARK_LOG_ROOT(visitor, rootName) \
    dataLog("\n%s: ", rootName); \
    (visitor).resetChildCount()
#define MARK_LOG_PARENT(visitor, parent) \
    dataLog("\n%p (%s): ", parent, parent->className() ? parent->className() : "unknown"); \
    (visitor).resetChildCount()
#define MARK_LOG_CHILD(visitor, child) \
    if ((visitor).childCount()) \
    dataLogString(", "); \
    dataLog("%p", child); \
    (visitor).incrementChildCount()
#else
#define MARK_LOG_MESSAGE0(message) do { } while (false)
#define MARK_LOG_MESSAGE1(message, arg1) do { } while (false)
#define MARK_LOG_MESSAGE2(message, arg1, arg2) do { } while (false)
#define MARK_LOG_ROOT(visitor, rootName) do { } while (false)
#define MARK_LOG_PARENT(visitor, parent) do { } while (false)
#define MARK_LOG_CHILD(visitor, child) do { } while (false)
#endif

namespace JSC {

    class ConservativeRoots;
    class JSGlobalData;
    class MarkStack;
    class ParallelModeEnabler;
    class Register;
    class SlotVisitor;
    template<typename T> class WriteBarrierBase;
    template<typename T> class JITWriteBarrier;
    
    struct MarkStackSegment {
        MarkStackSegment* m_previous;
#if !ASSERT_DISABLED
        size_t m_top;
#endif
        
        const JSCell** data()
        {
            return bitwise_cast<const JSCell**>(this + 1);
        }
        
        static size_t capacityFromSize(size_t size)
        {
            return (size - sizeof(MarkStackSegment)) / sizeof(const JSCell*);
        }
        
        static size_t sizeFromCapacity(size_t capacity)
        {
            return sizeof(MarkStackSegment) + capacity * sizeof(const JSCell*);
        }
    };

    class MarkStackSegmentAllocator {
    public:
        MarkStackSegmentAllocator();
        ~MarkStackSegmentAllocator();
        
        MarkStackSegment* allocate();
        void release(MarkStackSegment*);
        
        void shrinkReserve();
        
    private:
        SpinLock m_lock;
        MarkStackSegment* m_nextFreeSegment;
    };

    class MarkStackArray {
    public:
        MarkStackArray(MarkStackSegmentAllocator&);
        ~MarkStackArray();

        void append(const JSCell*);

        bool canRemoveLast();
        const JSCell* removeLast();
        bool refill();
        
        bool isEmpty();
        
        void donateSomeCellsTo(MarkStackArray& other);
        
        void stealSomeCellsFrom(MarkStackArray& other, size_t idleThreadCount);

        size_t size();

    private:
        MarkStackSegment* m_topSegment;
        
        JS_EXPORT_PRIVATE void expand();
        
        MarkStackSegmentAllocator& m_allocator;

        size_t m_segmentCapacity;
        size_t m_top;
        size_t m_numberOfPreviousSegments;
        
        size_t postIncTop()
        {
            size_t result = m_top++;
            ASSERT(result == m_topSegment->m_top++);
            return result;
        }
        
        size_t preDecTop()
        {
            size_t result = --m_top;
            ASSERT(result == --m_topSegment->m_top);
            return result;
        }
        
        void setTopForFullSegment()
        {
            ASSERT(m_topSegment->m_top == m_segmentCapacity);
            m_top = m_segmentCapacity;
        }
        
        void setTopForEmptySegment()
        {
            ASSERT(!m_topSegment->m_top);
            m_top = 0;
        }
        
        size_t top()
        {
            ASSERT(m_top == m_topSegment->m_top);
            return m_top;
        }
        
#if ASSERT_DISABLED
        void validatePrevious() { }
#else
        void validatePrevious()
        {
            unsigned count = 0;
            for (MarkStackSegment* current = m_topSegment->m_previous; current; current = current->m_previous)
                count++;
            ASSERT(count == m_numberOfPreviousSegments);
        }
#endif
    };

    class MarkStackThreadSharedData {
    public:
        MarkStackThreadSharedData(JSGlobalData*);
        ~MarkStackThreadSharedData();
        
        void reset();

#if ENABLE(PARALLEL_GC)
        void resetChildren();
        size_t childVisitCount();
        size_t childDupStrings();
#endif
    
    private:
        friend class MarkStack;
        friend class SlotVisitor;

#if ENABLE(PARALLEL_GC)
        void markingThreadMain(SlotVisitor*);
        static void markingThreadStartFunc(void* heap);
#endif

        JSGlobalData* m_globalData;
        CopiedSpace* m_copiedSpace;
        
        MarkStackSegmentAllocator m_segmentAllocator;
        
        bool m_shouldHashConst;

        Vector<ThreadIdentifier> m_markingThreads;
        Vector<MarkStack*> m_markingThreadsMarkStack;
        
        Mutex m_markingLock;
        ThreadCondition m_markingCondition;
        MarkStackArray m_sharedMarkStack;
        unsigned m_numberOfActiveParallelMarkers;
        bool m_parallelMarkersShouldExit;

        Mutex m_opaqueRootsLock;
        HashSet<void*> m_opaqueRoots;

        ListableHandler<WeakReferenceHarvester>::List m_weakReferenceHarvesters;
        ListableHandler<UnconditionalFinalizer>::List m_unconditionalFinalizers;
    };

    class MarkStack {
        WTF_MAKE_NONCOPYABLE(MarkStack);
        friend class HeapRootVisitor; // Allowed to mark a JSValue* or JSCell** directly.

    public:
        MarkStack(MarkStackThreadSharedData&);
        ~MarkStack();

        void append(ConservativeRoots&);
        
        template<typename T> void append(JITWriteBarrier<T>*);
        template<typename T> void append(WriteBarrierBase<T>*);
        void appendValues(WriteBarrierBase<Unknown>*, size_t count);
        
        template<typename T>
        void appendUnbarrieredPointer(T**);
        
        void addOpaqueRoot(void*);
        bool containsOpaqueRoot(void*);
        int opaqueRootCount();

        MarkStackThreadSharedData& sharedData() { return m_shared; }
        bool isEmpty() { return m_stack.isEmpty(); }

        void setup();
        void reset();

        size_t visitCount() const { return m_visitCount; }

#if ENABLE(SIMPLE_HEAP_PROFILING)
        VTableSpectrum m_visitedTypeCounts;
#endif

        void addWeakReferenceHarvester(WeakReferenceHarvester* weakReferenceHarvester)
        {
            m_shared.m_weakReferenceHarvesters.addThreadSafe(weakReferenceHarvester);
        }
        
        void addUnconditionalFinalizer(UnconditionalFinalizer* unconditionalFinalizer)
        {
            m_shared.m_unconditionalFinalizers.addThreadSafe(unconditionalFinalizer);
        }

#if ENABLE(OBJECT_MARK_LOGGING)
        inline void resetChildCount() { m_logChildCount = 0; }
        inline unsigned childCount() { return m_logChildCount; }
        inline void incrementChildCount() { m_logChildCount++; }
#endif

    protected:
        JS_EXPORT_PRIVATE static void validate(JSCell*);

        void append(JSValue*);
        void append(JSValue*, size_t count);
        void append(JSCell**);

        void internalAppend(JSCell*);
        void internalAppend(JSValue);
        void internalAppend(JSValue*);
        
        JS_EXPORT_PRIVATE void mergeOpaqueRoots();
        
        void mergeOpaqueRootsIfNecessary()
        {
            if (m_opaqueRoots.isEmpty())
                return;
            mergeOpaqueRoots();
        }
        
        void mergeOpaqueRootsIfProfitable()
        {
            if (static_cast<unsigned>(m_opaqueRoots.size()) < Options::opaqueRootMergeThreshold())
                return;
            mergeOpaqueRoots();
        }
        
        MarkStackArray m_stack;
        HashSet<void*> m_opaqueRoots; // Handle-owning data structures not visible to the garbage collector.
        
#if !ASSERT_DISABLED
    public:
        bool m_isCheckingForDefaultMarkViolation;
        bool m_isDraining;
#endif
    protected:
        friend class ParallelModeEnabler;
        
        size_t m_visitCount;
        bool m_isInParallelMode;
        
        MarkStackThreadSharedData& m_shared;

        bool m_shouldHashConst; // Local per-thread copy of shared flag for performance reasons
        typedef HashMap<StringImpl*, JSValue> UniqueStringMap;
        UniqueStringMap m_uniqueStrings;

#if ENABLE(OBJECT_MARK_LOGGING)
        unsigned m_logChildCount;
#endif
    };

    inline MarkStack::MarkStack(MarkStackThreadSharedData& shared)
        : m_stack(shared.m_segmentAllocator)
#if !ASSERT_DISABLED
        , m_isCheckingForDefaultMarkViolation(false)
        , m_isDraining(false)
#endif
        , m_visitCount(0)
        , m_isInParallelMode(false)
        , m_shared(shared)
        , m_shouldHashConst(false)
    {
    }

    inline MarkStack::~MarkStack()
    {
        ASSERT(m_stack.isEmpty());
    }

    inline void MarkStack::addOpaqueRoot(void* root)
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

    inline bool MarkStack::containsOpaqueRoot(void* root)
    {
        ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
        ASSERT(m_opaqueRoots.isEmpty());
        return m_shared.m_opaqueRoots.contains(root);
#else
        return m_opaqueRoots.contains(root);
#endif
    }

    inline int MarkStack::opaqueRootCount()
    {
        ASSERT(!m_isInParallelMode);
#if ENABLE(PARALLEL_GC)
        ASSERT(m_opaqueRoots.isEmpty());
        return m_shared.m_opaqueRoots.size();
#else
        return m_opaqueRoots.size();
#endif
    }

    inline void MarkStackArray::append(const JSCell* cell)
    {
        if (m_top == m_segmentCapacity)
            expand();
        m_topSegment->data()[postIncTop()] = cell;
    }

    inline bool MarkStackArray::canRemoveLast()
    {
        return !!m_top;
    }

    inline const JSCell* MarkStackArray::removeLast()
    {
        return m_topSegment->data()[preDecTop()];
    }

    inline bool MarkStackArray::isEmpty()
    {
        if (m_top)
            return false;
        if (m_topSegment->m_previous) {
            ASSERT(m_topSegment->m_previous->m_top == m_segmentCapacity);
            return false;
        }
        return true;
    }

    inline size_t MarkStackArray::size()
    {
        return m_top + m_segmentCapacity * m_numberOfPreviousSegments;
    }

    ALWAYS_INLINE void MarkStack::append(JSValue* slot, size_t count)
    {
        for (size_t i = 0; i < count; ++i) {
            JSValue& value = slot[i];
            if (!value)
                continue;
            internalAppend(value);
        }
    }

    template<typename T>
    inline void MarkStack::appendUnbarrieredPointer(T** slot)
    {
        ASSERT(slot);
        JSCell* cell = *slot;
        if (cell)
            internalAppend(cell);
    }
    
    ALWAYS_INLINE void MarkStack::append(JSValue* slot)
    {
        ASSERT(slot);
        internalAppend(*slot);
    }

    ALWAYS_INLINE void MarkStack::append(JSCell** slot)
    {
        ASSERT(slot);
        internalAppend(*slot);
    }

    ALWAYS_INLINE void MarkStack::internalAppend(JSValue value)
    {
        ASSERT(value);
        if (!value.isCell())
            return;
        internalAppend(value.asCell());
    }

    class ParallelModeEnabler {
    public:
        ParallelModeEnabler(MarkStack& stack)
            : m_stack(stack)
        {
            ASSERT(!m_stack.m_isInParallelMode);
            m_stack.m_isInParallelMode = true;
        }
        
        ~ParallelModeEnabler()
        {
            ASSERT(m_stack.m_isInParallelMode);
            m_stack.m_isInParallelMode = false;
        }
        
    private:
        MarkStack& m_stack;
    };

} // namespace JSC

#endif
