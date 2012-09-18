/*
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2011 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "Heap.h"

#include "CopiedSpace.h"
#include "CopiedSpaceInlineMethods.h"
#include "CodeBlock.h"
#include "ConservativeRoots.h"
#include "GCActivityCallback.h"
#include "HeapRootVisitor.h"
#include "IncrementalSweeper.h"
#include "Interpreter.h"
#include "JSGlobalData.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "JSONObject.h"
#include "Tracing.h"
#include "WeakSetInlines.h"
#include <algorithm>
#include <wtf/RAMSize.h>
#include <wtf/CurrentTime.h>

using namespace std;
using namespace JSC;

namespace JSC {

namespace { 

static const size_t largeHeapSize = 32 * MB; // About 1.5X the average webpage.
static const size_t smallHeapSize = 1 * MB; // Matches the FastMalloc per-thread cache.

#if ENABLE(GC_LOGGING)
#if COMPILER(CLANG)
#define DEFINE_GC_LOGGING_GLOBAL(type, name, arguments) \
_Pragma("clang diagnostic push") \
_Pragma("clang diagnostic ignored \"-Wglobal-constructors\"") \
_Pragma("clang diagnostic ignored \"-Wexit-time-destructors\"") \
static type name arguments; \
_Pragma("clang diagnostic pop")
#else
#define DEFINE_GC_LOGGING_GLOBAL(type, name, arguments) \
static type name arguments;
#endif // COMPILER(CLANG)

struct GCTimer {
    GCTimer(const char* name)
        : m_time(0)
        , m_min(100000000)
        , m_max(0)
        , m_count(0)
        , m_name(name)
    {
    }
    ~GCTimer()
    {
        dataLog("%s: %.2lfms (avg. %.2lf, min. %.2lf, max. %.2lf)\n", m_name, m_time * 1000, m_time * 1000 / m_count, m_min*1000, m_max*1000);
    }
    double m_time;
    double m_min;
    double m_max;
    size_t m_count;
    const char* m_name;
};

struct GCTimerScope {
    GCTimerScope(GCTimer* timer)
        : m_timer(timer)
        , m_start(WTF::currentTime())
    {
    }
    ~GCTimerScope()
    {
        double delta = WTF::currentTime() - m_start;
        if (delta < m_timer->m_min)
            m_timer->m_min = delta;
        if (delta > m_timer->m_max)
            m_timer->m_max = delta;
        m_timer->m_count++;
        m_timer->m_time += delta;
    }
    GCTimer* m_timer;
    double m_start;
};

struct GCCounter {
    GCCounter(const char* name)
        : m_name(name)
        , m_count(0)
        , m_total(0)
        , m_min(10000000)
        , m_max(0)
    {
    }
    
    void count(size_t amount)
    {
        m_count++;
        m_total += amount;
        if (amount < m_min)
            m_min = amount;
        if (amount > m_max)
            m_max = amount;
    }
    ~GCCounter()
    {
        dataLog("%s: %zu values (avg. %zu, min. %zu, max. %zu)\n", m_name, m_total, m_total / m_count, m_min, m_max);
    }
    const char* m_name;
    size_t m_count;
    size_t m_total;
    size_t m_min;
    size_t m_max;
};

#define GCPHASE(name) DEFINE_GC_LOGGING_GLOBAL(GCTimer, name##Timer, (#name)); GCTimerScope name##TimerScope(&name##Timer)
#define COND_GCPHASE(cond, name1, name2) DEFINE_GC_LOGGING_GLOBAL(GCTimer, name1##Timer, (#name1)); DEFINE_GC_LOGGING_GLOBAL(GCTimer, name2##Timer, (#name2)); GCTimerScope name1##CondTimerScope(cond ? &name1##Timer : &name2##Timer)
#define GCCOUNTER(name, value) do { DEFINE_GC_LOGGING_GLOBAL(GCCounter, name##Counter, (#name)); name##Counter.count(value); } while (false)
    
#else

#define GCPHASE(name) do { } while (false)
#define COND_GCPHASE(cond, name1, name2) do { } while (false)
#define GCCOUNTER(name, value) do { } while (false)
#endif

static inline size_t minHeapSize(HeapType heapType, size_t ramSize)
{
    if (heapType == LargeHeap)
        return min(largeHeapSize, ramSize / 4);
    return smallHeapSize;
}

static inline size_t proportionalHeapSize(size_t heapSize, size_t ramSize)
{
    // Try to stay under 1/2 RAM size to leave room for the DOM, rendering, networking, etc.
    if (heapSize < ramSize / 4)
        return 2 * heapSize;
    if (heapSize < ramSize / 2)
        return 1.5 * heapSize;
    return 1.25 * heapSize;
}

static inline bool isValidSharedInstanceThreadState(JSGlobalData* globalData)
{
    return globalData->apiLock().currentThreadIsHoldingLock();
}

static inline bool isValidThreadState(JSGlobalData* globalData)
{
    if (globalData->identifierTable != wtfThreadData().currentIdentifierTable())
        return false;

    if (globalData->isSharedInstance() && !isValidSharedInstanceThreadState(globalData))
        return false;

    return true;
}

struct Count : public MarkedBlock::CountFunctor {
    void operator()(JSCell*) { count(1); }
};

struct CountIfGlobalObject : MarkedBlock::CountFunctor {
    void operator()(JSCell* cell) {
        if (!cell->isObject())
            return;
        if (!asObject(cell)->isGlobalObject())
            return;
        count(1);
    }
};

class RecordType {
public:
    typedef PassOwnPtr<TypeCountSet> ReturnType;

    RecordType();
    void operator()(JSCell*);
    ReturnType returnValue();

private:
    const char* typeName(JSCell*);
    OwnPtr<TypeCountSet> m_typeCountSet;
};

inline RecordType::RecordType()
    : m_typeCountSet(adoptPtr(new TypeCountSet))
{
}

inline const char* RecordType::typeName(JSCell* cell)
{
    const ClassInfo* info = cell->classInfo();
    if (!info || !info->className)
        return "[unknown]";
    return info->className;
}

inline void RecordType::operator()(JSCell* cell)
{
    m_typeCountSet->add(typeName(cell));
}

inline PassOwnPtr<TypeCountSet> RecordType::returnValue()
{
    return m_typeCountSet.release();
}

} // anonymous namespace

Heap::Heap(JSGlobalData* globalData, HeapType heapType)
    : m_heapType(heapType)
    , m_ramSize(ramSize())
    , m_minBytesPerCycle(minHeapSize(m_heapType, m_ramSize))
    , m_sizeAfterLastCollect(0)
    , m_bytesAllocatedLimit(m_minBytesPerCycle)
    , m_bytesAllocated(0)
    , m_bytesAbandoned(0)
    , m_operationInProgress(NoOperation)
    , m_objectSpace(this)
    , m_storageSpace(this)
    , m_machineThreads(this)
    , m_sharedData(globalData)
    , m_slotVisitor(m_sharedData)
    , m_handleSet(globalData)
    , m_isSafeToCollect(false)
    , m_globalData(globalData)
    , m_lastGCLength(0)
    , m_lastCodeDiscardTime(WTF::currentTime())
    , m_activityCallback(DefaultGCActivityCallback::create(this))
    , m_sweeper(IncrementalSweeper::create(this))
{
    m_storageSpace.init();
}

Heap::~Heap()
{
}

bool Heap::isPagedOut(double deadline)
{
    return m_objectSpace.isPagedOut(deadline) || m_storageSpace.isPagedOut(deadline);
}

// The JSGlobalData is being destroyed and the collector will never run again.
// Run all pending finalizers now because we won't get another chance.
void Heap::lastChanceToFinalize()
{
    ASSERT(!m_globalData->dynamicGlobalObject);
    ASSERT(m_operationInProgress == NoOperation);

    m_objectSpace.lastChanceToFinalize();

#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_slotVisitor.m_visitedTypeCounts.dump(WTF::dataFile(), "Visited Type Counts");
    m_destroyedTypeCounts.dump(WTF::dataFile(), "Destroyed Type Counts");
#endif
}

void Heap::reportExtraMemoryCostSlowCase(size_t cost)
{
    // Our frequency of garbage collection tries to balance memory use against speed
    // by collecting based on the number of newly created values. However, for values
    // that hold on to a great deal of memory that's not in the form of other JS values,
    // that is not good enough - in some cases a lot of those objects can pile up and
    // use crazy amounts of memory without a GC happening. So we track these extra
    // memory costs. Only unusually large objects are noted, and we only keep track
    // of this extra cost until the next GC. In garbage collected languages, most values
    // are either very short lived temporaries, or have extremely long lifetimes. So
    // if a large value survives one garbage collection, there is not much point to
    // collecting more frequently as long as it stays alive.

    didAllocate(cost);
    if (shouldCollect())
        collect(DoNotSweep);
}

void Heap::reportAbandonedObjectGraph()
{
    // Our clients don't know exactly how much memory they
    // are abandoning so we just guess for them.
    double abandonedBytes = 0.10 * m_sizeAfterLastCollect;

    // We want to accelerate the next collection. Because memory has just 
    // been abandoned, the next collection has the potential to 
    // be more profitable. Since allocation is the trigger for collection, 
    // we hasten the next collection by pretending that we've allocated more memory. 
    didAbandon(abandonedBytes);
}

void Heap::didAbandon(size_t bytes)
{
    m_activityCallback->didAllocate(m_bytesAllocated + m_bytesAbandoned);
    m_bytesAbandoned += bytes;
}

void Heap::protect(JSValue k)
{
    ASSERT(k);
    ASSERT(m_globalData->apiLock().currentThreadIsHoldingLock());

    if (!k.isCell())
        return;

    m_protectedValues.add(k.asCell());
}

bool Heap::unprotect(JSValue k)
{
    ASSERT(k);
    ASSERT(m_globalData->apiLock().currentThreadIsHoldingLock());

    if (!k.isCell())
        return false;

    return m_protectedValues.remove(k.asCell());
}

void Heap::jettisonDFGCodeBlock(PassOwnPtr<CodeBlock> codeBlock)
{
    m_dfgCodeBlocks.jettison(codeBlock);
}

void Heap::markProtectedObjects(HeapRootVisitor& heapRootVisitor)
{
    ProtectCountSet::iterator end = m_protectedValues.end();
    for (ProtectCountSet::iterator it = m_protectedValues.begin(); it != end; ++it)
        heapRootVisitor.visit(&it->first);
}

void Heap::pushTempSortVector(Vector<ValueStringPair>* tempVector)
{
    m_tempSortingVectors.append(tempVector);
}

void Heap::popTempSortVector(Vector<ValueStringPair>* tempVector)
{
    ASSERT_UNUSED(tempVector, tempVector == m_tempSortingVectors.last());
    m_tempSortingVectors.removeLast();
}

void Heap::markTempSortVectors(HeapRootVisitor& heapRootVisitor)
{
    typedef Vector<Vector<ValueStringPair>* > VectorOfValueStringVectors;

    VectorOfValueStringVectors::iterator end = m_tempSortingVectors.end();
    for (VectorOfValueStringVectors::iterator it = m_tempSortingVectors.begin(); it != end; ++it) {
        Vector<ValueStringPair>* tempSortingVector = *it;

        Vector<ValueStringPair>::iterator vectorEnd = tempSortingVector->end();
        for (Vector<ValueStringPair>::iterator vectorIt = tempSortingVector->begin(); vectorIt != vectorEnd; ++vectorIt) {
            if (vectorIt->first)
                heapRootVisitor.visit(&vectorIt->first);
        }
    }
}

void Heap::harvestWeakReferences()
{
    m_slotVisitor.harvestWeakReferences();
}

void Heap::finalizeUnconditionalFinalizers()
{
    m_slotVisitor.finalizeUnconditionalFinalizers();
}

inline RegisterFile& Heap::registerFile()
{
    return m_globalData->interpreter->registerFile();
}

void Heap::getConservativeRegisterRoots(HashSet<JSCell*>& roots)
{
    ASSERT(isValidThreadState(m_globalData));
    ConservativeRoots registerFileRoots(&m_objectSpace.blocks(), &m_storageSpace);
    registerFile().gatherConservativeRoots(registerFileRoots);
    size_t registerFileRootCount = registerFileRoots.size();
    JSCell** registerRoots = registerFileRoots.roots();
    for (size_t i = 0; i < registerFileRootCount; i++) {
        setMarked(registerRoots[i]);
        roots.add(registerRoots[i]);
    }
}

void Heap::markRoots(bool fullGC)
{
    SamplingRegion samplingRegion("Garbage Collection: Tracing");

    COND_GCPHASE(fullGC, MarkFullRoots, MarkYoungRoots);
    UNUSED_PARAM(fullGC);
    ASSERT(isValidThreadState(m_globalData));

#if ENABLE(OBJECT_MARK_LOGGING)
    double gcStartTime = WTF::currentTime();
#endif

    void* dummy;
    
    // We gather conservative roots before clearing mark bits because conservative
    // gathering uses the mark bits to determine whether a reference is valid.
    ConservativeRoots machineThreadRoots(&m_objectSpace.blocks(), &m_storageSpace);
    m_jitStubRoutines.clearMarks();
    {
        GCPHASE(GatherConservativeRoots);
        m_machineThreads.gatherConservativeRoots(machineThreadRoots, &dummy);
    }

    ConservativeRoots registerFileRoots(&m_objectSpace.blocks(), &m_storageSpace);
    m_dfgCodeBlocks.clearMarks();
    {
        GCPHASE(GatherRegisterFileRoots);
        registerFile().gatherConservativeRoots(
            registerFileRoots, m_jitStubRoutines, m_dfgCodeBlocks);
    }

#if ENABLE(DFG_JIT)
    ConservativeRoots scratchBufferRoots(&m_objectSpace.blocks(), &m_storageSpace);
    {
        GCPHASE(GatherScratchBufferRoots);
        m_globalData->gatherConservativeRoots(scratchBufferRoots);
    }
#endif

#if ENABLE(GGC)
    MarkedBlock::DirtyCellVector dirtyCells;
    if (!fullGC) {
        GCPHASE(GatheringDirtyCells);
        m_objectSpace.gatherDirtyCells(dirtyCells);
    } else
#endif
    {
        GCPHASE(clearMarks);
        m_objectSpace.clearMarks();
    }

    m_storageSpace.startedCopying();
    SlotVisitor& visitor = m_slotVisitor;
    visitor.setup();
    HeapRootVisitor heapRootVisitor(visitor);

    {
        ParallelModeEnabler enabler(visitor);
#if ENABLE(GGC)
        {
            size_t dirtyCellCount = dirtyCells.size();
            GCPHASE(VisitDirtyCells);
            GCCOUNTER(DirtyCellCount, dirtyCellCount);
            for (size_t i = 0; i < dirtyCellCount; i++) {
                heapRootVisitor.visitChildren(dirtyCells[i]);
                visitor.donateAndDrain();
            }
        }
#endif
    
        if (m_globalData->codeBlocksBeingCompiled.size()) {
            GCPHASE(VisitActiveCodeBlock);
            for (size_t i = 0; i < m_globalData->codeBlocksBeingCompiled.size(); i++)
                m_globalData->codeBlocksBeingCompiled[i]->visitAggregate(visitor);
        }
    
        {
            GCPHASE(VisitMachineRoots);
            MARK_LOG_ROOT(visitor, "C++ Stack");
            visitor.append(machineThreadRoots);
            visitor.donateAndDrain();
        }
        {
            GCPHASE(VisitRegisterFileRoots);
            MARK_LOG_ROOT(visitor, "Register File");
            visitor.append(registerFileRoots);
            visitor.donateAndDrain();
        }
#if ENABLE(DFG_JIT)
        {
            GCPHASE(VisitScratchBufferRoots);
            MARK_LOG_ROOT(visitor, "Scratch Buffers");
            visitor.append(scratchBufferRoots);
            visitor.donateAndDrain();
        }
#endif
        {
            GCPHASE(VisitProtectedObjects);
            MARK_LOG_ROOT(visitor, "Protected Objects");
            markProtectedObjects(heapRootVisitor);
            visitor.donateAndDrain();
        }
        {
            GCPHASE(VisitTempSortVectors);
            MARK_LOG_ROOT(visitor, "Temp Sort Vectors");
            markTempSortVectors(heapRootVisitor);
            visitor.donateAndDrain();
        }

        {
            GCPHASE(MarkingArgumentBuffers);
            if (m_markListSet && m_markListSet->size()) {
                MARK_LOG_ROOT(visitor, "Argument Buffers");
                MarkedArgumentBuffer::markLists(heapRootVisitor, *m_markListSet);
                visitor.donateAndDrain();
            }
        }
        if (m_globalData->exception) {
            GCPHASE(MarkingException);
            MARK_LOG_ROOT(visitor, "Exceptions");
            heapRootVisitor.visit(&m_globalData->exception);
            visitor.donateAndDrain();
        }
    
        {
            GCPHASE(VisitStrongHandles);
            MARK_LOG_ROOT(visitor, "Strong Handles");
            m_handleSet.visitStrongHandles(heapRootVisitor);
            visitor.donateAndDrain();
        }
    
        {
            GCPHASE(HandleStack);
            MARK_LOG_ROOT(visitor, "Handle Stack");
            m_handleStack.visit(heapRootVisitor);
            visitor.donateAndDrain();
        }
    
        {
            GCPHASE(TraceCodeBlocksAndJITStubRoutines);
            MARK_LOG_ROOT(visitor, "Trace Code Blocks and JIT Stub Routines");
            m_dfgCodeBlocks.traceMarkedCodeBlocks(visitor);
            m_jitStubRoutines.traceMarkedStubRoutines(visitor);
            visitor.donateAndDrain();
        }
    
#if ENABLE(PARALLEL_GC)
        {
            GCPHASE(Convergence);
            visitor.drainFromShared(SlotVisitor::MasterDrain);
        }
#endif
    }

    // Weak references must be marked last because their liveness depends on
    // the liveness of the rest of the object graph.
    {
        GCPHASE(VisitingLiveWeakHandles);
        MARK_LOG_ROOT(visitor, "Live Weak Handles");
        while (true) {
            m_objectSpace.visitWeakSets(heapRootVisitor);
            harvestWeakReferences();
            if (visitor.isEmpty())
                break;
            {
                ParallelModeEnabler enabler(visitor);
                visitor.donateAndDrain();
#if ENABLE(PARALLEL_GC)
                visitor.drainFromShared(SlotVisitor::MasterDrain);
#endif
            }
        }
    }

    GCCOUNTER(VisitedValueCount, visitor.visitCount());

    visitor.doneCopying();
#if ENABLE(OBJECT_MARK_LOGGING)
    size_t visitCount = visitor.visitCount();
#if ENABLE(PARALLEL_GC)
    visitCount += m_sharedData.childVisitCount();
#endif
    MARK_LOG_MESSAGE2("\nNumber of live Objects after full GC %lu, took %.6f secs\n", visitCount, WTF::currentTime() - gcStartTime);
#endif

    visitor.reset();
#if ENABLE(PARALLEL_GC)
    m_sharedData.resetChildren();
#endif
    m_sharedData.reset();
    m_storageSpace.doneCopying();
}

size_t Heap::objectCount()
{
    return m_objectSpace.objectCount();
}

size_t Heap::size()
{
    return m_objectSpace.size() + m_storageSpace.size();
}

size_t Heap::capacity()
{
    return m_objectSpace.capacity() + m_storageSpace.capacity();
}

size_t Heap::protectedGlobalObjectCount()
{
    return forEachProtectedCell<CountIfGlobalObject>();
}

size_t Heap::globalObjectCount()
{
    return m_objectSpace.forEachLiveCell<CountIfGlobalObject>();
}

size_t Heap::protectedObjectCount()
{
    return forEachProtectedCell<Count>();
}

PassOwnPtr<TypeCountSet> Heap::protectedObjectTypeCounts()
{
    return forEachProtectedCell<RecordType>();
}

PassOwnPtr<TypeCountSet> Heap::objectTypeCounts()
{
    return m_objectSpace.forEachLiveCell<RecordType>();
}

void Heap::deleteAllCompiledCode()
{
    // If JavaScript is running, it's not safe to delete code, since we'll end
    // up deleting code that is live on the stack.
    if (m_globalData->dynamicGlobalObject)
        return;

    for (ExecutableBase* current = m_compiledCode.head(); current; current = current->next()) {
        if (!current->isFunctionExecutable())
            continue;
        static_cast<FunctionExecutable*>(current)->clearCodeIfNotCompiling();
    }

    m_dfgCodeBlocks.clearMarks();
    m_dfgCodeBlocks.deleteUnmarkedJettisonedCodeBlocks();
}

void Heap::deleteUnmarkedCompiledCode()
{
    ExecutableBase* next;
    for (ExecutableBase* current = m_compiledCode.head(); current; current = next) {
        next = current->next();
        if (isMarked(current))
            continue;

        // We do this because executable memory is limited on some platforms and because
        // CodeBlock requires eager finalization.
        ExecutableBase::clearCodeVirtual(current);
        m_compiledCode.remove(current);
    }

    m_dfgCodeBlocks.deleteUnmarkedJettisonedCodeBlocks();
    m_jitStubRoutines.deleteUnmarkedJettisonedStubRoutines();
}

void Heap::collectAllGarbage()
{
    if (!m_isSafeToCollect)
        return;

    collect(DoSweep);
}

static double minute = 60.0;

void Heap::collect(SweepToggle sweepToggle)
{
    SamplingRegion samplingRegion("Garbage Collection");
    
    GCPHASE(Collect);
    ASSERT(globalData()->apiLock().currentThreadIsHoldingLock());
    ASSERT(globalData()->identifierTable == wtfThreadData().currentIdentifierTable());
    ASSERT(m_isSafeToCollect);
    JAVASCRIPTCORE_GC_BEGIN();
    if (m_operationInProgress != NoOperation)
        CRASH();
    m_operationInProgress = Collection;

    m_activityCallback->willCollect();

    double lastGCStartTime = WTF::currentTime();
    if (lastGCStartTime - m_lastCodeDiscardTime > minute) {
        deleteAllCompiledCode();
        m_lastCodeDiscardTime = WTF::currentTime();
    }

#if ENABLE(GGC)
    bool fullGC = sweepToggle == DoSweep;
    if (!fullGC)
        fullGC = (capacity() > 4 * m_sizeAfterLastCollect);  
#else
    bool fullGC = true;
#endif
    {
        GCPHASE(Canonicalize);
        m_objectSpace.canonicalizeCellLivenessData();
    }

    markRoots(fullGC);
    
    {
        GCPHASE(ReapingWeakHandles);
        m_objectSpace.reapWeakSets();
    }

    JAVASCRIPTCORE_GC_MARKED();

    {
        GCPHASE(FinalizeUnconditionalFinalizers);
        finalizeUnconditionalFinalizers();
    }

    {
        GCPHASE(finalizeSmallStrings);
        m_globalData->smallStrings.finalizeSmallStrings();
    }

    {
        GCPHASE(DeleteCodeBlocks);
        deleteUnmarkedCompiledCode();
    }

    if (sweepToggle == DoSweep) {
        SamplingRegion samplingRegion("Garbage Collection: Sweeping");
        GCPHASE(Sweeping);
        m_objectSpace.sweep();
        m_objectSpace.shrink();
    }

    m_sweeper->startSweeping(m_objectSpace.blocks().set());
    m_bytesAbandoned = 0;

    {
        GCPHASE(ResetAllocators);
        m_objectSpace.resetAllocators();
    }
    
    if (Options::useZombieMode())
        zombifyDeadObjects();

    size_t currentHeapSize = size();
    if (fullGC) {
        m_sizeAfterLastCollect = currentHeapSize;

        // To avoid pathological GC churn in very small and very large heaps, we set
        // the new allocation limit based on the current size of the heap, with a
        // fixed minimum.
        size_t maxHeapSize = max(minHeapSize(m_heapType, m_ramSize), proportionalHeapSize(currentHeapSize, m_ramSize));
        m_bytesAllocatedLimit = maxHeapSize - currentHeapSize;
    }
    m_bytesAllocated = 0;
    double lastGCEndTime = WTF::currentTime();
    m_lastGCLength = lastGCEndTime - lastGCStartTime;
    if (m_operationInProgress != Collection)
        CRASH();
    m_operationInProgress = NoOperation;
    JAVASCRIPTCORE_GC_END();
}

void Heap::setActivityCallback(GCActivityCallback* activityCallback)
{
    m_activityCallback = activityCallback;
}

GCActivityCallback* Heap::activityCallback()
{
    return m_activityCallback;
}

IncrementalSweeper* Heap::sweeper()
{
    return m_sweeper;
}

void Heap::setGarbageCollectionTimerEnabled(bool enable)
{
    activityCallback()->setEnabled(enable);
}

void Heap::didAllocate(size_t bytes)
{
    m_activityCallback->didAllocate(m_bytesAllocated + m_bytesAbandoned);
    m_bytesAllocated += bytes;
}

bool Heap::isValidAllocation(size_t)
{
    if (!isValidThreadState(m_globalData))
        return false;

    if (m_operationInProgress != NoOperation)
        return false;
    
    return true;
}

void Heap::addFinalizer(JSCell* cell, Finalizer finalizer)
{
    WeakSet::allocate(cell, &m_finalizerOwner, reinterpret_cast<void*>(finalizer)); // Balanced by FinalizerOwner::finalize().
}

void Heap::FinalizerOwner::finalize(Handle<Unknown> handle, void* context)
{
    HandleSlot slot = handle.slot();
    Finalizer finalizer = reinterpret_cast<Finalizer>(context);
    finalizer(slot->asCell());
    WeakSet::deallocate(WeakImpl::asWeakImpl(slot));
}

void Heap::addCompiledCode(ExecutableBase* executable)
{
    m_compiledCode.append(executable);
}

bool Heap::isSafeToSweepStructures()
{
    return !m_sweeper || m_sweeper->structuresCanBeSwept();
}

void Heap::didStartVMShutdown()
{
    m_activityCallback->didStartVMShutdown();
    m_activityCallback = 0;
    m_sweeper->didStartVMShutdown();
    m_sweeper = 0;
    lastChanceToFinalize();
}

class ZombifyCellFunctor : public MarkedBlock::VoidFunctor {
public:
    ZombifyCellFunctor(size_t cellSize)
        : m_cellSize(cellSize)
    {
    }

    void operator()(JSCell* cell)
    {
        if (Options::zombiesAreImmortal())
            MarkedBlock::blockFor(cell)->setMarked(cell);

        void** current = reinterpret_cast<void**>(cell);

        // We want to maintain zapped-ness because that's how we know if we've called 
        // the destructor.
        if (cell->isZapped())
            current++;

        void* limit = static_cast<void*>(reinterpret_cast<char*>(cell) + m_cellSize);
        for (; current < limit; current++)
            *current = reinterpret_cast<void*>(0xbbadbeef);
    }

private:
    size_t m_cellSize;
};

class ZombifyBlockFunctor : public MarkedBlock::VoidFunctor {
public:
    void operator()(MarkedBlock* block)
    {
        ZombifyCellFunctor functor(block->cellSize());
        block->forEachDeadCell(functor);
    }
};

void Heap::zombifyDeadObjects()
{
    m_objectSpace.sweep();

    ZombifyBlockFunctor functor;
    m_objectSpace.forEachBlock(functor);
}

} // namespace JSC
