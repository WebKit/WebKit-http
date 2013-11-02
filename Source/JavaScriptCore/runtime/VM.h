/*
 * Copyright (C) 2008, 2009, 2013 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VM_h
#define VM_h

#include "DateInstanceCache.h"
#include "ExecutableAllocator.h"
#include "Heap.h"
#include "Intrinsic.h"
#include "JITThunks.h"
#include "JSCJSValue.h"
#include "JSLock.h"
#include "LLIntData.h"
#include "MacroAssemblerCodeRef.h"
#include "NumericStrings.h"
#include "ProfilerDatabase.h"
#include "PrivateName.h"
#include "PrototypeMap.h"
#include "SmallStrings.h"
#include "Strong.h"
#include "ThunkGenerators.h"
#include "TypedArrayController.h"
#include "Watchdog.h"
#include "WeakRandom.h"
#include <wtf/BumpPointerAllocator.h>
#include <wtf/DateMath.h>
#include <wtf/Forward.h>
#include <wtf/HashMap.h>
#include <wtf/RefCountedArray.h>
#include <wtf/SimpleStats.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/ThreadSpecific.h>
#include <wtf/WTFThreadData.h>
#if ENABLE(REGEXP_TRACING)
#include <wtf/ListHashSet.h>
#endif

namespace JSC {

    class CodeBlock;
    class CodeCache;
    class CommonIdentifiers;
    class ExecState;
    class HandleStack;
    class IdentifierTable;
    class Interpreter;
    class JSGlobalObject;
    class JSObject;
    class Keywords;
    class LLIntOffsetsExtractor;
    class LegacyProfiler;
    class NativeExecutable;
    class ParserArena;
    class RegExpCache;
    class SourceProvider;
    class SourceProviderCache;
    struct StackFrame;
    class Stringifier;
    class Structure;
#if ENABLE(REGEXP_TRACING)
    class RegExp;
#endif
    class UnlinkedCodeBlock;
    class UnlinkedEvalCodeBlock;
    class UnlinkedFunctionExecutable;
    class UnlinkedProgramCodeBlock;

#if ENABLE(DFG_JIT)
    namespace DFG {
    class LongLivedState;
    class Worklist;
    }
#endif // ENABLE(DFG_JIT)
#if ENABLE(FTL_JIT)
    namespace FTL {
    class Thunks;
    }
#endif // ENABLE(FTL_JIT)

    struct HashTable;
    struct Instruction;

    struct LocalTimeOffsetCache {
        LocalTimeOffsetCache()
            : start(0.0)
            , end(-1.0)
            , increment(0.0)
        {
        }
        
        void reset()
        {
            offset = LocalTimeOffset();
            start = 0.0;
            end = -1.0;
            increment = 0.0;
        }

        LocalTimeOffset offset;
        double start;
        double end;
        double increment;
    };

    class ConservativeRoots;

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable: 4200) // Disable "zero-sized array in struct/union" warning
#endif
    struct ScratchBuffer {
        ScratchBuffer()
        {
            u.m_activeLength = 0;
        }

        static ScratchBuffer* create(size_t size)
        {
            ScratchBuffer* result = new (fastMalloc(ScratchBuffer::allocationSize(size))) ScratchBuffer;

            return result;
        }

        static size_t allocationSize(size_t bufferSize) { return sizeof(ScratchBuffer) + bufferSize; }
        void setActiveLength(size_t activeLength) { u.m_activeLength = activeLength; }
        size_t activeLength() const { return u.m_activeLength; };
        size_t* activeLengthPtr() { return &u.m_activeLength; };
        void* dataBuffer() { return m_buffer; }

        union {
            size_t m_activeLength;
            double pad; // Make sure m_buffer is double aligned.
        } u;
#if CPU(MIPS) && (defined WTF_MIPS_ARCH_REV && WTF_MIPS_ARCH_REV == 2)
        void* m_buffer[0] __attribute__((aligned(8)));
#else
        void* m_buffer[0];
#endif
    };
#if COMPILER(MSVC)
#pragma warning(pop)
#endif

    class VM : public ThreadSafeRefCounted<VM> {
    public:
        // WebCore has a one-to-one mapping of threads to VMs;
        // either create() or createLeaked() should only be called once
        // on a thread, this is the 'default' VM (it uses the
        // thread's default string uniquing table from wtfThreadData).
        // API contexts created using the new context group aware interface
        // create APIContextGroup objects which require less locking of JSC
        // than the old singleton APIShared VM created for use by
        // the original API.
        enum VMType { Default, APIContextGroup, APIShared };
        
        struct ClientData {
            JS_EXPORT_PRIVATE virtual ~ClientData() = 0;
        };

        bool isSharedInstance() { return vmType == APIShared; }
        bool usingAPI() { return vmType != Default; }
        JS_EXPORT_PRIVATE static bool sharedInstanceExists();
        JS_EXPORT_PRIVATE static VM& sharedInstance();

        JS_EXPORT_PRIVATE static PassRefPtr<VM> create(HeapType = SmallHeap);
        JS_EXPORT_PRIVATE static PassRefPtr<VM> createLeaked(HeapType = SmallHeap);
        static PassRefPtr<VM> createContextGroup(HeapType = SmallHeap);
        JS_EXPORT_PRIVATE ~VM();

        void makeUsableFromMultipleThreads() { heap.machineThreads().makeUsableFromMultipleThreads(); }
        
#if ENABLE(DFG_JIT)
        DFG::Worklist* ensureWorklist();
#endif // ENABLE(DFG_JIT)

    private:
        RefPtr<JSLock> m_apiLock;

    public:
#if ENABLE(ASSEMBLER)
        // executableAllocator should be destructed after the heap, as the heap can call executableAllocator
        // in its destructor.
        ExecutableAllocator executableAllocator;
#endif

        // The heap should be just after executableAllocator and before other members to ensure that it's
        // destructed after all the objects that reference it.
        Heap heap;
        
#if ENABLE(DFG_JIT)
        OwnPtr<DFG::LongLivedState> dfgState;
        RefPtr<DFG::Worklist> worklist;
#endif // ENABLE(DFG_JIT)

        VMType vmType;
        ClientData* clientData;
        ExecState* topCallFrame;
        Watchdog watchdog;

        const OwnPtr<const HashTable> arrayConstructorTable;
        const OwnPtr<const HashTable> arrayPrototypeTable;
        const OwnPtr<const HashTable> booleanPrototypeTable;
        const OwnPtr<const HashTable> dataViewTable;
        const OwnPtr<const HashTable> dateTable;
        const OwnPtr<const HashTable> dateConstructorTable;
        const OwnPtr<const HashTable> errorPrototypeTable;
        const OwnPtr<const HashTable> globalObjectTable;
        const OwnPtr<const HashTable> jsonTable;
        const OwnPtr<const HashTable> numberConstructorTable;
        const OwnPtr<const HashTable> numberPrototypeTable;
        const OwnPtr<const HashTable> objectConstructorTable;
        const OwnPtr<const HashTable> privateNamePrototypeTable;
        const OwnPtr<const HashTable> regExpTable;
        const OwnPtr<const HashTable> regExpConstructorTable;
        const OwnPtr<const HashTable> regExpPrototypeTable;
        const OwnPtr<const HashTable> stringConstructorTable;
#if ENABLE(PROMISES)
        const OwnPtr<const HashTable> promisePrototypeTable;
        const OwnPtr<const HashTable> promiseConstructorTable;
        const OwnPtr<const HashTable> promiseResolverPrototypeTable;
#endif

        Strong<Structure> structureStructure;
        Strong<Structure> structureRareDataStructure;
        Strong<Structure> debuggerActivationStructure;
        Strong<Structure> terminatedExecutionErrorStructure;
        Strong<Structure> stringStructure;
        Strong<Structure> notAnObjectStructure;
        Strong<Structure> propertyNameIteratorStructure;
        Strong<Structure> getterSetterStructure;
        Strong<Structure> apiWrapperStructure;
        Strong<Structure> JSScopeStructure;
        Strong<Structure> executableStructure;
        Strong<Structure> nativeExecutableStructure;
        Strong<Structure> evalExecutableStructure;
        Strong<Structure> programExecutableStructure;
        Strong<Structure> functionExecutableStructure;
        Strong<Structure> regExpStructure;
        Strong<Structure> sharedSymbolTableStructure;
        Strong<Structure> structureChainStructure;
        Strong<Structure> sparseArrayValueMapStructure;
        Strong<Structure> withScopeStructure;
        Strong<Structure> unlinkedFunctionExecutableStructure;
        Strong<Structure> unlinkedProgramCodeBlockStructure;
        Strong<Structure> unlinkedEvalCodeBlockStructure;
        Strong<Structure> unlinkedFunctionCodeBlockStructure;
        Strong<Structure> propertyTableStructure;
        Strong<Structure> mapDataStructure;
        Strong<Structure> weakMapDataStructure;
        Strong<JSCell> iterationTerminator;

        IdentifierTable* identifierTable;
        CommonIdentifiers* propertyNames;
        const MarkedArgumentBuffer* emptyList; // Lists are supposed to be allocated on the stack to have their elements properly marked, which is not the case here - but this list has nothing to mark.
        SmallStrings smallStrings;
        NumericStrings numericStrings;
        DateInstanceCache dateInstanceCache;
        WTF::SimpleStats machineCodeBytesPerBytecodeWordForBaselineJIT;

        void setInDefineOwnProperty(bool inDefineOwnProperty)
        {
            m_inDefineOwnProperty = inDefineOwnProperty;
        }

        bool isInDefineOwnProperty()
        {
            return m_inDefineOwnProperty;
        }

        LegacyProfiler* enabledProfiler()
        {
            return m_enabledProfiler;
        }

#if ENABLE(JIT) && ENABLE(LLINT)
        bool canUseJIT() { return m_canUseJIT; }
#elif ENABLE(JIT)
        bool canUseJIT() { return true; } // jit only
#else
        bool canUseJIT() { return false; } // interpreter only
#endif

#if ENABLE(YARR_JIT)
        bool canUseRegExpJIT() { return m_canUseRegExpJIT; }
#else
        bool canUseRegExpJIT() { return false; } // interpreter only
#endif

        SourceProviderCache* addSourceProviderCache(SourceProvider*);
        void clearSourceProviderCaches();

        PrototypeMap prototypeMap;

        OwnPtr<ParserArena> parserArena;
        typedef HashMap<RefPtr<SourceProvider>, RefPtr<SourceProviderCache>> SourceProviderCacheMap;
        SourceProviderCacheMap sourceProviderCacheMap;
        OwnPtr<Keywords> keywords;
        Interpreter* interpreter;
#if ENABLE(JIT)
        OwnPtr<JITThunks> jitStubs;
        MacroAssemblerCodeRef getCTIStub(ThunkGenerator generator)
        {
            return jitStubs->ctiStub(this, generator);
        }
        NativeExecutable* getHostFunction(NativeFunction, Intrinsic);
#endif
#if ENABLE(FTL_JIT)
        std::unique_ptr<FTL::Thunks> ftlThunks;
#endif
        NativeExecutable* getHostFunction(NativeFunction, NativeFunction constructor);

        static ptrdiff_t exceptionOffset()
        {
            return OBJECT_OFFSETOF(VM, m_exception);
        }

        static ptrdiff_t callFrameForThrowOffset()
        {
            return OBJECT_OFFSETOF(VM, callFrameForThrow);
        }

        static ptrdiff_t targetMachinePCForThrowOffset()
        {
            return OBJECT_OFFSETOF(VM, targetMachinePCForThrow);
        }

        JS_EXPORT_PRIVATE void clearException();
        JS_EXPORT_PRIVATE void clearExceptionStack();
        void getExceptionInfo(JSValue& exception, RefCountedArray<StackFrame>& exceptionStack);
        void setExceptionInfo(JSValue& exception, RefCountedArray<StackFrame>& exceptionStack);
        JSValue exception() const { return m_exception; }
        JSValue* addressOfException() { return &m_exception; }
        const RefCountedArray<StackFrame>& exceptionStack() const { return m_exceptionStack; }

        JS_EXPORT_PRIVATE JSValue throwException(ExecState*, JSValue);
        JS_EXPORT_PRIVATE JSObject* throwException(ExecState*, JSObject*);
        
        const ClassInfo* const jsArrayClassInfo;
        const ClassInfo* const jsFinalObjectClassInfo;

        ReturnAddressPtr exceptionLocation;
        JSValue hostCallReturnValue;
        ExecState* callFrameForThrow;
        void* targetMachinePCForThrow;
        Instruction* targetInterpreterPCForThrow;
        uint32_t osrExitIndex;
        void* osrExitJumpDestination;
        Vector<ScratchBuffer*> scratchBuffers;
        size_t sizeOfLastScratchBuffer;
        
        ScratchBuffer* scratchBufferForSize(size_t size)
        {
            if (!size)
                return 0;
            
            if (size > sizeOfLastScratchBuffer) {
                // Protect against a N^2 memory usage pathology by ensuring
                // that at worst, we get a geometric series, meaning that the
                // total memory usage is somewhere around
                // max(scratch buffer size) * 4.
                sizeOfLastScratchBuffer = size * 2;

                scratchBuffers.append(ScratchBuffer::create(sizeOfLastScratchBuffer));
            }

            ScratchBuffer* result = scratchBuffers.last();
            result->setActiveLength(0);
            return result;
        }

        void gatherConservativeRoots(ConservativeRoots&);

        JSGlobalObject* dynamicGlobalObject;

        HashSet<JSObject*> stringRecursionCheckVisitedObjects;

        LocalTimeOffsetCache localTimeOffsetCache;
        
        String cachedDateString;
        double cachedDateStringValue;

        LegacyProfiler* m_enabledProfiler;
        OwnPtr<Profiler::Database> m_perBytecodeProfiler;
        RefPtr<TypedArrayController> m_typedArrayController;
        RegExpCache* m_regExpCache;
        BumpPointerAllocator m_regExpAllocator;

#if ENABLE(REGEXP_TRACING)
        typedef ListHashSet<RefPtr<RegExp>> RTTraceList;
        RTTraceList* m_rtTraceList;
#endif

        ThreadIdentifier exclusiveThread;

        JS_EXPORT_PRIVATE void resetDateCache();

        JS_EXPORT_PRIVATE void startSampling();
        JS_EXPORT_PRIVATE void stopSampling();
        JS_EXPORT_PRIVATE void dumpSampleData(ExecState* exec);
        RegExpCache* regExpCache() { return m_regExpCache; }
#if ENABLE(REGEXP_TRACING)
        void addRegExpToTrace(PassRefPtr<RegExp> regExp);
#endif
        JS_EXPORT_PRIVATE void dumpRegExpTrace();

        bool isCollectorBusy() { return heap.isBusy(); }
        JS_EXPORT_PRIVATE void releaseExecutableMemory();

#if ENABLE(GC_VALIDATION)
        bool isInitializingObject() const; 
        void setInitializingObjectClass(const ClassInfo*);
#endif

        unsigned m_newStringsSinceLastHashCons;

        static const unsigned s_minNumberOfNewStringsToHashCons = 100;

        bool haveEnoughNewStringsToHashCons() { return m_newStringsSinceLastHashCons > s_minNumberOfNewStringsToHashCons; }
        void resetNewStringsSinceLastHashCons() { m_newStringsSinceLastHashCons = 0; }

        bool currentThreadIsHoldingAPILock() const
        {
            return m_apiLock->currentThreadIsHoldingLock() || exclusiveThread == currentThread();
        }

        JSLock& apiLock() { return *m_apiLock; }
        CodeCache* codeCache() { return m_codeCache.get(); }

        void prepareToDiscardCode();
        
        JS_EXPORT_PRIVATE void discardAllCode();

    private:
        friend class LLIntOffsetsExtractor;
        friend class ClearExceptionScope;
        
        VM(VMType, HeapType);
        static VM*& sharedInstanceInternal();
        void createNativeThunk();
#if ENABLE(ASSEMBLER)
        bool m_canUseAssembler;
#endif
#if ENABLE(JIT)
        bool m_canUseJIT;
#endif
#if ENABLE(YARR_JIT)
        bool m_canUseRegExpJIT;
#endif
#if ENABLE(GC_VALIDATION)
        const ClassInfo* m_initializingObjectClass;
#endif
        JSValue m_exception;
        bool m_inDefineOwnProperty;
        OwnPtr<CodeCache> m_codeCache;
        RefCountedArray<StackFrame> m_exceptionStack;
    };

#if ENABLE(GC_VALIDATION)
    inline bool VM::isInitializingObject() const
    {
        return !!m_initializingObjectClass;
    }

    inline void VM::setInitializingObjectClass(const ClassInfo* initializingObjectClass)
    {
        m_initializingObjectClass = initializingObjectClass;
    }
#endif

    inline Heap* WeakSet::heap() const
    {
        return &m_vm->heap;
    }

} // namespace JSC

#endif // VM_h
