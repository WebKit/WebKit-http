/*
 * Copyright (C) 2005-2018 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the NU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA 
 *
 */

#include "config.h"
#include "JSLock.h"

#include "HeapInlines.h"
#include "JSGlobalObject.h"
#include "MachineStackMarker.h"
#include "SamplingProfiler.h"
#include "WasmCapabilities.h"
#include "WasmMachineThreads.h"
#include <wtf/StackPointer.h>
#include <wtf/Threading.h>
#include <wtf/threads/Signals.h>

#if USE(WEB_THREAD)
#include <wtf/ios/WebCoreThread.h>
#endif

namespace JSC {

Lock GlobalJSLock::s_sharedInstanceMutex;

GlobalJSLock::GlobalJSLock()
{
    s_sharedInstanceMutex.lock();
}

GlobalJSLock::~GlobalJSLock()
{
    s_sharedInstanceMutex.unlock();
}

JSLockHolder::JSLockHolder(JSGlobalObject* globalObject)
    : JSLockHolder(globalObject->vm())
{
}

JSLockHolder::JSLockHolder(VM* vm)
    : JSLockHolder(*vm)
{
}

JSLockHolder::JSLockHolder(VM& vm)
    : m_vm(&vm)
{
    m_vm->apiLock().lock();
}

JSLockHolder::~JSLockHolder()
{
    RefPtr<JSLock> apiLock(&m_vm->apiLock());
    m_vm = nullptr;
    apiLock->unlock();
}

JSLock::JSLock(VM* vm)
    : m_lockCount(0)
    , m_lockDropDepth(0)
    , m_vm(vm)
    , m_entryAtomStringTable(nullptr)
{
}

JSLock::~JSLock()
{
}

void JSLock::willDestroyVM(VM* vm)
{
    ASSERT_UNUSED(vm, m_vm == vm);
    m_vm = nullptr;
}

void JSLock::lock()
{
    lock(1);
}

void JSLock::lock(intptr_t lockCount)
{
    ASSERT(lockCount > 0);
#if USE(WEB_THREAD)
    if (m_isWebThreadAware) {
        ASSERT(WebCoreWebThreadIsEnabled && WebCoreWebThreadIsEnabled());
        WebCoreWebThreadLock();
    }
#endif

    bool success = m_lock.tryLock();
    if (UNLIKELY(!success)) {
        if (currentThreadIsHoldingLock()) {
            m_lockCount += lockCount;
            return;
        }
        m_lock.lock();
    }

    m_ownerThread = &Thread::current();
    WTF::storeStoreFence();
    m_hasOwnerThread = true;
    ASSERT(!m_lockCount);
    m_lockCount = lockCount;

    didAcquireLock();
}

void JSLock::didAcquireLock()
{  
    // FIXME: What should happen to the per-thread identifier table if we don't have a VM?
    if (!m_vm)
        return;
    
    Thread& thread = Thread::current();
    ASSERT(!m_entryAtomStringTable);
    m_entryAtomStringTable = thread.setCurrentAtomStringTable(m_vm->atomStringTable());
    ASSERT(m_entryAtomStringTable);

    m_vm->setLastStackTop(thread.savedLastStackTop());
    ASSERT(thread.stack().contains(m_vm->lastStackTop()));

    if (m_vm->heap.hasAccess())
        m_shouldReleaseHeapAccess = false;
    else {
        m_vm->heap.acquireAccess();
        m_shouldReleaseHeapAccess = true;
    }

    RELEASE_ASSERT(!m_vm->stackPointerAtVMEntry());
    void* p = currentStackPointer();
    m_vm->setStackPointerAtVMEntry(p);

    if (m_vm->heap.machineThreads().addCurrentThread()) {
        if (isKernTCSMAvailable())
            enableKernTCSM();
    }

#if ENABLE(WEBASSEMBLY)
    if (Wasm::isSupported())
        Wasm::startTrackingCurrentThread();
#endif

#if HAVE(MACH_EXCEPTIONS)
    registerThreadForMachExceptionHandling(Thread::current());
#endif

    // Note: everything below must come after addCurrentThread().
    m_vm->traps().notifyGrabAllLocks();
    
    m_vm->firePrimitiveGigacageEnabledIfNecessary();

#if ENABLE(SAMPLING_PROFILER)
    if (SamplingProfiler* samplingProfiler = m_vm->samplingProfiler())
        samplingProfiler->noticeJSLockAcquisition();
#endif
}

void JSLock::unlock()
{
    unlock(1);
}

void JSLock::unlock(intptr_t unlockCount)
{
    RELEASE_ASSERT(currentThreadIsHoldingLock());
    ASSERT(m_lockCount >= unlockCount);

    // Maintain m_lockCount while calling willReleaseLock() so that its callees know that
    // they still have the lock.
    if (unlockCount == m_lockCount)
        willReleaseLock();

    m_lockCount -= unlockCount;

    if (!m_lockCount) {
        m_hasOwnerThread = false;
        m_lock.unlock();
    }
}

void JSLock::willReleaseLock()
{   
    RefPtr<VM> vm = m_vm;
    if (vm) {
        RELEASE_ASSERT_WITH_MESSAGE(!vm->hasCheckpointOSRSideState(), "Releasing JSLock but pending checkpoint side state still available");
        vm->drainMicrotasks();

        if (!vm->topCallFrame)
            vm->clearLastException();

        vm->heap.releaseDelayedReleasedObjects();
        vm->setStackPointerAtVMEntry(nullptr);
        
        if (m_shouldReleaseHeapAccess)
            vm->heap.releaseAccess();
    }

    if (m_entryAtomStringTable) {
        Thread::current().setCurrentAtomStringTable(m_entryAtomStringTable);
        m_entryAtomStringTable = nullptr;
    }
}

void JSLock::lock(JSGlobalObject* globalObject)
{
    globalObject->vm().apiLock().lock();
}

void JSLock::unlock(JSGlobalObject* globalObject)
{
    globalObject->vm().apiLock().unlock();
}

// This function returns the number of locks that were dropped.
unsigned JSLock::dropAllLocks(DropAllLocks* dropper)
{
    if (!currentThreadIsHoldingLock())
        return 0;

    ++m_lockDropDepth;

    dropper->setDropDepth(m_lockDropDepth);

    Thread& thread = Thread::current();
    thread.setSavedStackPointerAtVMEntry(m_vm->stackPointerAtVMEntry());
    thread.setSavedLastStackTop(m_vm->lastStackTop());

    unsigned droppedLockCount = m_lockCount;
    unlock(droppedLockCount);

    return droppedLockCount;
}

void JSLock::grabAllLocks(DropAllLocks* dropper, unsigned droppedLockCount)
{
    // If no locks were dropped, nothing to do!
    if (!droppedLockCount)
        return;

    ASSERT(!currentThreadIsHoldingLock());
    lock(droppedLockCount);

    while (dropper->dropDepth() != m_lockDropDepth) {
        unlock(droppedLockCount);
        Thread::yield();
        lock(droppedLockCount);
    }

    --m_lockDropDepth;

    Thread& thread = Thread::current();
    m_vm->setStackPointerAtVMEntry(thread.savedStackPointerAtVMEntry());
    m_vm->setLastStackTop(thread.savedLastStackTop());
}

JSLock::DropAllLocks::DropAllLocks(VM* vm)
    : m_droppedLockCount(0)
    // If the VM is in the middle of being destroyed then we don't want to resurrect it
    // by allowing DropAllLocks to ref it. By this point the JSLock has already been 
    // released anyways, so it doesn't matter that DropAllLocks is a no-op.
    , m_vm(vm->heap.isShuttingDown() ? nullptr : vm)
{
    if (!m_vm)
        return;
    RELEASE_ASSERT(!m_vm->apiLock().currentThreadIsHoldingLock() || !m_vm->isCollectorBusyOnCurrentThread());
    m_droppedLockCount = m_vm->apiLock().dropAllLocks(this);
}

JSLock::DropAllLocks::DropAllLocks(JSGlobalObject* globalObject)
    : DropAllLocks(globalObject ? &globalObject->vm() : nullptr)
{
}

JSLock::DropAllLocks::DropAllLocks(VM& vm)
    : DropAllLocks(&vm)
{
}

JSLock::DropAllLocks::~DropAllLocks()
{
    if (!m_vm)
        return;
    m_vm->apiLock().grabAllLocks(this, m_droppedLockCount);
}

} // namespace JSC
