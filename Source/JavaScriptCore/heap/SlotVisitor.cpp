#include "config.h"
#include "SlotVisitor.h"

#include "ConservativeRoots.h"
#include "CopiedSpace.h"
#include "CopiedSpaceInlineMethods.h"
#include "JSArray.h"
#include "JSGlobalData.h"
#include "JSObject.h"
#include "JSString.h"

namespace JSC {

SlotVisitor::SlotVisitor(GCThreadSharedData& shared)
    : m_stack(shared.m_segmentAllocator)
    , m_visitCount(0)
    , m_isInParallelMode(false)
    , m_shared(shared)
    , m_shouldHashConst(false)
#if !ASSERT_DISABLED
    , m_isCheckingForDefaultMarkViolation(false)
    , m_isDraining(false)
#endif
{
}

SlotVisitor::~SlotVisitor()
{
    ASSERT(m_stack.isEmpty());
}

void SlotVisitor::setup()
{
    m_shared.m_shouldHashConst = m_shared.m_globalData->haveEnoughNewStringsToHashConst();
    m_shouldHashConst = m_shared.m_shouldHashConst;
#if ENABLE(PARALLEL_GC)
    for (unsigned i = 0; i < m_shared.m_markingThreadsMarkStack.size(); ++i)
        m_shared.m_markingThreadsMarkStack[i]->m_shouldHashConst = m_shared.m_shouldHashConst;
#endif
}

void SlotVisitor::reset()
{
    m_visitCount = 0;
    ASSERT(m_stack.isEmpty());
#if ENABLE(PARALLEL_GC)
    ASSERT(m_opaqueRoots.isEmpty()); // Should have merged by now.
#else
    m_opaqueRoots.clear();
#endif
    if (m_shouldHashConst) {
        m_uniqueStrings.clear();
        m_shouldHashConst = false;
    }
}

void SlotVisitor::append(ConservativeRoots& conservativeRoots)
{
    JSCell** roots = conservativeRoots.roots();
    size_t size = conservativeRoots.size();
    for (size_t i = 0; i < size; ++i)
        internalAppend(roots[i]);
}

ALWAYS_INLINE static void visitChildren(SlotVisitor& visitor, const JSCell* cell)
{
#if ENABLE(SIMPLE_HEAP_PROFILING)
    m_visitedTypeCounts.count(cell);
#endif

    ASSERT(Heap::isMarked(cell));
    
    if (isJSString(cell)) {
        JSString::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSFinalObject(cell)) {
        JSFinalObject::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    if (isJSArray(cell)) {
        JSArray::visitChildren(const_cast<JSCell*>(cell), visitor);
        return;
    }

    cell->methodTable()->visitChildren(const_cast<JSCell*>(cell), visitor);
}

void SlotVisitor::donateKnownParallel()
{
    // NOTE: Because we re-try often, we can afford to be conservative, and
    // assume that donating is not profitable.

    // Avoid locking when a thread reaches a dead end in the object graph.
    if (m_stack.size() < 2)
        return;

    // If there's already some shared work queued up, be conservative and assume
    // that donating more is not profitable.
    if (m_shared.m_sharedMarkStack.size())
        return;

    // If we're contending on the lock, be conservative and assume that another
    // thread is already donating.
    MutexTryLocker locker(m_shared.m_markingLock);
    if (!locker.locked())
        return;

    // Otherwise, assume that a thread will go idle soon, and donate.
    m_stack.donateSomeCellsTo(m_shared.m_sharedMarkStack);

    if (m_shared.m_numberOfActiveParallelMarkers < Options::numberOfGCMarkers())
        m_shared.m_markingCondition.broadcast();
}

void SlotVisitor::drain()
{
    ASSERT(m_isInParallelMode);
   
#if ENABLE(PARALLEL_GC)
    if (Options::numberOfGCMarkers() > 1) {
        while (!m_stack.isEmpty()) {
            m_stack.refill();
            for (unsigned countdown = Options::minimumNumberOfScansBetweenRebalance(); m_stack.canRemoveLast() && countdown--;)
                visitChildren(*this, m_stack.removeLast());
            donateKnownParallel();
        }
        
        mergeOpaqueRootsIfNecessary();
        return;
    }
#endif
    
    while (!m_stack.isEmpty()) {
        m_stack.refill();
        while (m_stack.canRemoveLast())
            visitChildren(*this, m_stack.removeLast());
    }
}

void SlotVisitor::drainFromShared(SharedDrainMode sharedDrainMode)
{
    ASSERT(m_isInParallelMode);
    
    ASSERT(Options::numberOfGCMarkers());
    
    bool shouldBeParallel;

#if ENABLE(PARALLEL_GC)
    shouldBeParallel = Options::numberOfGCMarkers() > 1;
#else
    ASSERT(Options::numberOfGCMarkers() == 1);
    shouldBeParallel = false;
#endif
    
    if (!shouldBeParallel) {
        // This call should be a no-op.
        ASSERT_UNUSED(sharedDrainMode, sharedDrainMode == MasterDrain);
        ASSERT(m_stack.isEmpty());
        ASSERT(m_shared.m_sharedMarkStack.isEmpty());
        return;
    }
    
#if ENABLE(PARALLEL_GC)
    {
        MutexLocker locker(m_shared.m_markingLock);
        m_shared.m_numberOfActiveParallelMarkers++;
    }
    while (true) {
        {
            MutexLocker locker(m_shared.m_markingLock);
            m_shared.m_numberOfActiveParallelMarkers--;

            // How we wait differs depending on drain mode.
            if (sharedDrainMode == MasterDrain) {
                // Wait until either termination is reached, or until there is some work
                // for us to do.
                while (true) {
                    // Did we reach termination?
                    if (!m_shared.m_numberOfActiveParallelMarkers && m_shared.m_sharedMarkStack.isEmpty()) {
                        // Let any sleeping slaves know it's time for them to give their private CopiedBlocks back
                        m_shared.m_markingCondition.broadcast();
                        return;
                    }
                    
                    // Is there work to be done?
                    if (!m_shared.m_sharedMarkStack.isEmpty())
                        break;
                    
                    // Otherwise wait.
                    m_shared.m_markingCondition.wait(m_shared.m_markingLock);
                }
            } else {
                ASSERT(sharedDrainMode == SlaveDrain);
                
                // Did we detect termination? If so, let the master know.
                if (!m_shared.m_numberOfActiveParallelMarkers && m_shared.m_sharedMarkStack.isEmpty())
                    m_shared.m_markingCondition.broadcast();
                
                while (m_shared.m_sharedMarkStack.isEmpty() && !m_shared.m_parallelMarkersShouldExit) {
                    if (!m_shared.m_numberOfActiveParallelMarkers && m_shared.m_sharedMarkStack.isEmpty())
                        doneCopying();
                    m_shared.m_markingCondition.wait(m_shared.m_markingLock);
                }
                
                // Is the VM exiting? If so, exit this thread.
                if (m_shared.m_parallelMarkersShouldExit) {
                    doneCopying();
                    return;
                }
            }
           
            size_t idleThreadCount = Options::numberOfGCMarkers() - m_shared.m_numberOfActiveParallelMarkers;
            m_stack.stealSomeCellsFrom(m_shared.m_sharedMarkStack, idleThreadCount);
            m_shared.m_numberOfActiveParallelMarkers++;
        }
        
        drain();
    }
#endif
}

void SlotVisitor::mergeOpaqueRoots()
{
    ASSERT(!m_opaqueRoots.isEmpty()); // Should only be called when opaque roots are non-empty.
    {
        MutexLocker locker(m_shared.m_opaqueRootsLock);
        HashSet<void*>::iterator begin = m_opaqueRoots.begin();
        HashSet<void*>::iterator end = m_opaqueRoots.end();
        for (HashSet<void*>::iterator iter = begin; iter != end; ++iter)
            m_shared.m_opaqueRoots.add(*iter);
    }
    m_opaqueRoots.clear();
}

void SlotVisitor::startCopying()
{
    ASSERT(!m_copiedAllocator.isValid());
}

void* SlotVisitor::allocateNewSpaceSlow(size_t bytes)
{
    m_shared.m_copiedSpace->doneFillingBlock(m_copiedAllocator.resetCurrentBlock());
    m_copiedAllocator.setCurrentBlock(m_shared.m_copiedSpace->allocateBlockForCopyingPhase());

    void* result = 0;
    CheckedBoolean didSucceed = m_copiedAllocator.tryAllocate(bytes, &result);
    ASSERT(didSucceed);
    return result;
}

void* SlotVisitor::allocateNewSpaceOrPin(void* ptr, size_t bytes)
{
    if (!checkIfShouldCopyAndPinOtherwise(ptr, bytes))
        return 0;
    
    return allocateNewSpace(bytes);
}

ALWAYS_INLINE bool JSString::tryHashConstLock()
{
#if ENABLE(PARALLEL_GC)
    unsigned currentFlags = m_flags;

    if (currentFlags & HashConstLock)
        return false;

    unsigned newFlags = currentFlags | HashConstLock;

    if (!WTF::weakCompareAndSwap(&m_flags, currentFlags, newFlags))
        return false;

    WTF::memoryBarrierAfterLock();
    return true;
#else
    if (isHashConstSingleton())
        return false;

    m_flags |= HashConstLock;

    return true;
#endif
}

ALWAYS_INLINE void JSString::releaseHashConstLock()
{
#if ENABLE(PARALLEL_GC)
    WTF::memoryBarrierBeforeUnlock();
#endif
    m_flags &= ~HashConstLock;
}

ALWAYS_INLINE bool JSString::shouldTryHashConst()
{
    return ((length() > 1) && !isRope() && !isHashConstSingleton());
}

ALWAYS_INLINE void SlotVisitor::internalAppend(JSValue* slot)
{
    // This internalAppend is only intended for visits to object and array backing stores.
    // as it can change the JSValue pointed to be the argument when the original JSValue
    // is a string that contains the same contents as another string.

    ASSERT(slot);
    JSValue value = *slot;
    ASSERT(value);
    if (!value.isCell())
        return;

    JSCell* cell = value.asCell();
    if (!cell)
        return;

    if (m_shouldHashConst && cell->isString()) {
        JSString* string = jsCast<JSString*>(cell);
        if (string->shouldTryHashConst() && string->tryHashConstLock()) {
            UniqueStringMap::AddResult addResult = m_uniqueStrings.add(string->string().impl(), value);
            if (addResult.isNewEntry)
                string->setHashConstSingleton();
            else {
                JSValue existingJSValue = addResult.iterator->second;
                if (value != existingJSValue)
                    jsCast<JSString*>(existingJSValue.asCell())->clearHashConstSingleton();
                *slot = existingJSValue;
                string->releaseHashConstLock();
                return;
            }
            string->releaseHashConstLock();
        }
    }

    internalAppend(cell);
}

void SlotVisitor::copyAndAppend(void** ptr, size_t bytes, JSValue* values, unsigned length)
{
    void* oldPtr = *ptr;
    void* newPtr = allocateNewSpaceOrPin(oldPtr, bytes);
    if (newPtr) {
        size_t jsValuesOffset = static_cast<size_t>(reinterpret_cast<char*>(values) - static_cast<char*>(oldPtr));

        JSValue* newValues = reinterpret_cast_ptr<JSValue*>(static_cast<char*>(newPtr) + jsValuesOffset);
        for (unsigned i = 0; i < length; i++) {
            JSValue& value = values[i];
            newValues[i] = value;
            if (!value)
                continue;
            internalAppend(&newValues[i]);
        }

        memcpy(newPtr, oldPtr, jsValuesOffset);
        *ptr = newPtr;
    } else
        append(values, length);
}
    
void SlotVisitor::doneCopying()
{
    if (!m_copiedAllocator.isValid())
        return;

    m_shared.m_copiedSpace->doneFillingBlock(m_copiedAllocator.resetCurrentBlock());
}

void SlotVisitor::harvestWeakReferences()
{
    for (WeakReferenceHarvester* current = m_shared.m_weakReferenceHarvesters.head(); current; current = current->next())
        current->visitWeakReferences(*this);
}

void SlotVisitor::finalizeUnconditionalFinalizers()
{
    while (m_shared.m_unconditionalFinalizers.hasNext())
        m_shared.m_unconditionalFinalizers.removeNext()->finalizeUnconditionally();
}

#if ENABLE(GC_VALIDATION)
void SlotVisitor::validate(JSCell* cell)
{
    if (!cell) {
        dataLog("cell is NULL\n");
        CRASH();
    }

    if (!cell->structure()) {
        dataLog("cell at %p has a null structure\n" , cell);
        CRASH();
    }

    // Both the cell's structure, and the cell's structure's structure should be the Structure Structure.
    // I hate this sentence.
    if (cell->structure()->structure()->JSCell::classInfo() != cell->structure()->JSCell::classInfo()) {
        const char* parentClassName = 0;
        const char* ourClassName = 0;
        if (cell->structure()->structure() && cell->structure()->structure()->JSCell::classInfo())
            parentClassName = cell->structure()->structure()->JSCell::classInfo()->className;
        if (cell->structure()->JSCell::classInfo())
            ourClassName = cell->structure()->JSCell::classInfo()->className;
        dataLog("parent structure (%p <%s>) of cell at %p doesn't match cell's structure (%p <%s>)\n",
                cell->structure()->structure(), parentClassName, cell, cell->structure(), ourClassName);
        CRASH();
    }
}
#else
void SlotVisitor::validate(JSCell*)
{
}
#endif

} // namespace JSC
