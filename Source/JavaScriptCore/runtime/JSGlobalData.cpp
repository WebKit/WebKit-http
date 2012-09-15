/*
 * Copyright (C) 2008, 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "JSGlobalData.h"

#include "ArgList.h"
#include "CommonIdentifiers.h"
#include "DebuggerActivation.h"
#include "FunctionConstructor.h"
#include "GCActivityCallback.h"
#include "GetterSetter.h"
#include "Heap.h"
#include "HostCallReturnValue.h"
#include "IncrementalSweeper.h"
#include "Interpreter.h"
#include "JSActivation.h"
#include "JSAPIValueWrapper.h"
#include "JSArray.h"
#include "JSClassRef.h"
#include "JSFunction.h"
#include "JSLock.h"
#include "JSNameScope.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSWithScope.h"
#include "Lexer.h"
#include "Lookup.h"
#include "Nodes.h"
#include "ParserArena.h"
#include "RegExpCache.h"
#include "RegExpObject.h"
#include "StrictEvalActivation.h"
#include "StrongInlines.h"
#include <wtf/RetainPtr.h>
#include <wtf/Threading.h>
#include <wtf/WTFThreadData.h>

#if ENABLE(DFG_JIT)
#include "ConservativeRoots.h"
#endif

#if ENABLE(REGEXP_TRACING)
#include "RegExp.h"
#endif

#if USE(CF)
#include <CoreFoundation/CoreFoundation.h>
#endif

using namespace WTF;

namespace JSC {

extern const HashTable arrayConstructorTable;
extern const HashTable arrayPrototypeTable;
extern const HashTable booleanPrototypeTable;
extern const HashTable jsonTable;
extern const HashTable dateTable;
extern const HashTable dateConstructorTable;
extern const HashTable errorPrototypeTable;
extern const HashTable globalObjectTable;
extern const HashTable mathTable;
extern const HashTable numberConstructorTable;
extern const HashTable numberPrototypeTable;
JS_EXPORTDATA extern const HashTable objectConstructorTable;
extern const HashTable objectPrototypeTable;
extern const HashTable privateNamePrototypeTable;
extern const HashTable regExpTable;
extern const HashTable regExpConstructorTable;
extern const HashTable regExpPrototypeTable;
extern const HashTable stringTable;
extern const HashTable stringConstructorTable;

#if ENABLE(ASSEMBLER) && (ENABLE(CLASSIC_INTERPRETER) || ENABLE(LLINT))
static bool enableAssembler(ExecutableAllocator& executableAllocator)
{
    if (!executableAllocator.isValid() || (!Options::useJIT() && !Options::useRegExpJIT()))
        return false;

#if USE(CF)
#if COMPILER(GCC) && !COMPILER(CLANG)
    // FIXME: remove this once the EWS have been upgraded to LLVM.
    // Work around a bug of GCC with strict-aliasing.
    RetainPtr<CFStringRef> canUseJITKeyRetain(AdoptCF, CFStringCreateWithCString(0 , "JavaScriptCoreUseJIT", kCFStringEncodingMacRoman));
    CFStringRef canUseJITKey = canUseJITKeyRetain.get();
#else
    CFStringRef canUseJITKey = CFSTR("JavaScriptCoreUseJIT");
#endif // COMPILER(GCC) && !COMPILER(CLANG)
    RetainPtr<CFBooleanRef> canUseJIT(AdoptCF, (CFBooleanRef)CFPreferencesCopyAppValue(canUseJITKey, kCFPreferencesCurrentApplication));
    if (canUseJIT)
        return kCFBooleanTrue == canUseJIT.get();
#endif

#if USE(CF) || OS(UNIX)
    char* canUseJITString = getenv("JavaScriptCoreUseJIT");
    return !canUseJITString || atoi(canUseJITString);
#else
    return true;
#endif
}
#endif

JSGlobalData::JSGlobalData(GlobalDataType globalDataType, ThreadStackType threadStackType, HeapType heapType)
    :
#if ENABLE(ASSEMBLER)
      executableAllocator(*this),
#endif
      heap(this, heapType)
    , globalDataType(globalDataType)
    , clientData(0)
    , topCallFrame(CallFrame::noCaller())
    , arrayConstructorTable(fastNew<HashTable>(JSC::arrayConstructorTable))
    , arrayPrototypeTable(fastNew<HashTable>(JSC::arrayPrototypeTable))
    , booleanPrototypeTable(fastNew<HashTable>(JSC::booleanPrototypeTable))
    , dateTable(fastNew<HashTable>(JSC::dateTable))
    , dateConstructorTable(fastNew<HashTable>(JSC::dateConstructorTable))
    , errorPrototypeTable(fastNew<HashTable>(JSC::errorPrototypeTable))
    , globalObjectTable(fastNew<HashTable>(JSC::globalObjectTable))
    , jsonTable(fastNew<HashTable>(JSC::jsonTable))
    , mathTable(fastNew<HashTable>(JSC::mathTable))
    , numberConstructorTable(fastNew<HashTable>(JSC::numberConstructorTable))
    , numberPrototypeTable(fastNew<HashTable>(JSC::numberPrototypeTable))
    , objectConstructorTable(fastNew<HashTable>(JSC::objectConstructorTable))
    , objectPrototypeTable(fastNew<HashTable>(JSC::objectPrototypeTable))
    , privateNamePrototypeTable(fastNew<HashTable>(JSC::privateNamePrototypeTable))
    , regExpTable(fastNew<HashTable>(JSC::regExpTable))
    , regExpConstructorTable(fastNew<HashTable>(JSC::regExpConstructorTable))
    , regExpPrototypeTable(fastNew<HashTable>(JSC::regExpPrototypeTable))
    , stringTable(fastNew<HashTable>(JSC::stringTable))
    , stringConstructorTable(fastNew<HashTable>(JSC::stringConstructorTable))
    , identifierTable(globalDataType == Default ? wtfThreadData().currentIdentifierTable() : createIdentifierTable())
    , propertyNames(new CommonIdentifiers(this))
    , emptyList(new MarkedArgumentBuffer)
    , parserArena(adoptPtr(new ParserArena))
    , keywords(adoptPtr(new Keywords(this)))
    , interpreter(0)
    , jsArrayClassInfo(&JSArray::s_info)
    , jsFinalObjectClassInfo(&JSFinalObject::s_info)
#if ENABLE(DFG_JIT)
    , sizeOfLastScratchBuffer(0)
#endif
    , dynamicGlobalObject(0)
    , cachedUTCOffset(std::numeric_limits<double>::quiet_NaN())
    , maxReentryDepth(threadStackType == ThreadStackTypeSmall ? MaxSmallThreadReentryDepth : MaxLargeThreadReentryDepth)
    , m_enabledProfiler(0)
    , m_regExpCache(new RegExpCache(this))
#if ENABLE(REGEXP_TRACING)
    , m_rtTraceList(new RTTraceList())
#endif
#ifndef NDEBUG
    , exclusiveThread(0)
#endif
#if CPU(X86) && ENABLE(JIT)
    , m_timeoutCount(512)
#endif
    , m_newStringsSinceLastHashConst(0)
#if ENABLE(ASSEMBLER) && (ENABLE(CLASSIC_INTERPRETER) || ENABLE(LLINT))
    , m_canUseAssembler(enableAssembler(executableAllocator))
    , m_canUseJIT(m_canUseAssembler && Options::useJIT())
    , m_canUseRegExpJIT(m_canUseAssembler && Options::useRegExpJIT())
#endif
#if ENABLE(GC_VALIDATION)
    , m_initializingObjectClass(0)
#endif
    , m_inDefineOwnProperty(false)
{
    interpreter = new Interpreter;

    // Need to be careful to keep everything consistent here
    JSLockHolder lock(this);
    IdentifierTable* existingEntryIdentifierTable = wtfThreadData().setCurrentIdentifierTable(identifierTable);
    structureStructure.set(*this, Structure::createStructure(*this));
    debuggerActivationStructure.set(*this, DebuggerActivation::createStructure(*this, 0, jsNull()));
    interruptedExecutionErrorStructure.set(*this, InterruptedExecutionError::createStructure(*this, 0, jsNull()));
    terminatedExecutionErrorStructure.set(*this, TerminatedExecutionError::createStructure(*this, 0, jsNull()));
    stringStructure.set(*this, JSString::createStructure(*this, 0, jsNull()));
    notAnObjectStructure.set(*this, JSNotAnObject::createStructure(*this, 0, jsNull()));
    propertyNameIteratorStructure.set(*this, JSPropertyNameIterator::createStructure(*this, 0, jsNull()));
    getterSetterStructure.set(*this, GetterSetter::createStructure(*this, 0, jsNull()));
    apiWrapperStructure.set(*this, JSAPIValueWrapper::createStructure(*this, 0, jsNull()));
    JSScopeStructure.set(*this, JSScope::createStructure(*this, 0, jsNull()));
    executableStructure.set(*this, ExecutableBase::createStructure(*this, 0, jsNull()));
    nativeExecutableStructure.set(*this, NativeExecutable::createStructure(*this, 0, jsNull()));
    evalExecutableStructure.set(*this, EvalExecutable::createStructure(*this, 0, jsNull()));
    programExecutableStructure.set(*this, ProgramExecutable::createStructure(*this, 0, jsNull()));
    functionExecutableStructure.set(*this, FunctionExecutable::createStructure(*this, 0, jsNull()));
    regExpStructure.set(*this, RegExp::createStructure(*this, 0, jsNull()));
    sharedSymbolTableStructure.set(*this, SharedSymbolTable::createStructure(*this, 0, jsNull()));
    structureChainStructure.set(*this, StructureChain::createStructure(*this, 0, jsNull()));
    sparseArrayValueMapStructure.set(*this, SparseArrayValueMap::createStructure(*this, 0, jsNull()));

    wtfThreadData().setCurrentIdentifierTable(existingEntryIdentifierTable);

#if ENABLE(JIT)
    jitStubs = adoptPtr(new JITThunks(this));
#endif
    
    interpreter->initialize(this->canUseJIT());
    
    initializeHostCallReturnValue(); // This is needed to convince the linker not to drop host call return support.

    heap.notifyIsSafeToCollect();
    
    LLInt::Data::performAssertions(*this);
}

JSGlobalData::~JSGlobalData()
{
    ASSERT(!m_apiLock.currentThreadIsHoldingLock());
    heap.didStartVMShutdown();

    delete interpreter;
#ifndef NDEBUG
    interpreter = reinterpret_cast<Interpreter*>(0xbbadbeef);
#endif

    arrayPrototypeTable->deleteTable();
    arrayConstructorTable->deleteTable();
    booleanPrototypeTable->deleteTable();
    dateTable->deleteTable();
    dateConstructorTable->deleteTable();
    errorPrototypeTable->deleteTable();
    globalObjectTable->deleteTable();
    jsonTable->deleteTable();
    mathTable->deleteTable();
    numberConstructorTable->deleteTable();
    numberPrototypeTable->deleteTable();
    objectConstructorTable->deleteTable();
    objectPrototypeTable->deleteTable();
    privateNamePrototypeTable->deleteTable();
    regExpTable->deleteTable();
    regExpConstructorTable->deleteTable();
    regExpPrototypeTable->deleteTable();
    stringTable->deleteTable();
    stringConstructorTable->deleteTable();

    fastDelete(const_cast<HashTable*>(arrayConstructorTable));
    fastDelete(const_cast<HashTable*>(arrayPrototypeTable));
    fastDelete(const_cast<HashTable*>(booleanPrototypeTable));
    fastDelete(const_cast<HashTable*>(dateTable));
    fastDelete(const_cast<HashTable*>(dateConstructorTable));
    fastDelete(const_cast<HashTable*>(errorPrototypeTable));
    fastDelete(const_cast<HashTable*>(globalObjectTable));
    fastDelete(const_cast<HashTable*>(jsonTable));
    fastDelete(const_cast<HashTable*>(mathTable));
    fastDelete(const_cast<HashTable*>(numberConstructorTable));
    fastDelete(const_cast<HashTable*>(numberPrototypeTable));
    fastDelete(const_cast<HashTable*>(objectConstructorTable));
    fastDelete(const_cast<HashTable*>(objectPrototypeTable));
    fastDelete(const_cast<HashTable*>(privateNamePrototypeTable));
    fastDelete(const_cast<HashTable*>(regExpTable));
    fastDelete(const_cast<HashTable*>(regExpConstructorTable));
    fastDelete(const_cast<HashTable*>(regExpPrototypeTable));
    fastDelete(const_cast<HashTable*>(stringTable));
    fastDelete(const_cast<HashTable*>(stringConstructorTable));

    opaqueJSClassData.clear();

    delete emptyList;

    delete propertyNames;
    if (globalDataType != Default)
        deleteIdentifierTable(identifierTable);

    delete clientData;
    delete m_regExpCache;
#if ENABLE(REGEXP_TRACING)
    delete m_rtTraceList;
#endif

#if ENABLE(DFG_JIT)
    for (unsigned i = 0; i < scratchBuffers.size(); ++i)
        fastFree(scratchBuffers[i]);
#endif
}

PassRefPtr<JSGlobalData> JSGlobalData::createContextGroup(ThreadStackType type, HeapType heapType)
{
    return adoptRef(new JSGlobalData(APIContextGroup, type, heapType));
}

PassRefPtr<JSGlobalData> JSGlobalData::create(ThreadStackType type, HeapType heapType)
{
    return adoptRef(new JSGlobalData(Default, type, heapType));
}

PassRefPtr<JSGlobalData> JSGlobalData::createLeaked(ThreadStackType type, HeapType heapType)
{
    return create(type, heapType);
}

bool JSGlobalData::sharedInstanceExists()
{
    return sharedInstanceInternal();
}

JSGlobalData& JSGlobalData::sharedInstance()
{
    GlobalJSLock globalLock;
    JSGlobalData*& instance = sharedInstanceInternal();
    if (!instance) {
        instance = adoptRef(new JSGlobalData(APIShared, ThreadStackTypeSmall, SmallHeap)).leakRef();
        instance->makeUsableFromMultipleThreads();
    }
    return *instance;
}

JSGlobalData*& JSGlobalData::sharedInstanceInternal()
{
    static JSGlobalData* sharedInstance;
    return sharedInstance;
}

#if ENABLE(JIT)
static ThunkGenerator thunkGeneratorForIntrinsic(Intrinsic intrinsic)
{
    switch (intrinsic) {
    case CharCodeAtIntrinsic:
        return charCodeAtThunkGenerator;
    case CharAtIntrinsic:
        return charAtThunkGenerator;
    case FromCharCodeIntrinsic:
        return fromCharCodeThunkGenerator;
    case SqrtIntrinsic:
        return sqrtThunkGenerator;
    case PowIntrinsic:
        return powThunkGenerator;
    case AbsIntrinsic:
        return absThunkGenerator;
    case FloorIntrinsic:
        return floorThunkGenerator;
    case CeilIntrinsic:
        return ceilThunkGenerator;
    case RoundIntrinsic:
        return roundThunkGenerator;
    case ExpIntrinsic:
        return expThunkGenerator;
    case LogIntrinsic:
        return logThunkGenerator;
    default:
        return 0;
    }
}

NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function, NativeFunction constructor)
{
#if ENABLE(CLASSIC_INTERPRETER)
    if (!canUseJIT())
        return NativeExecutable::create(*this, function, constructor);
#endif
    return jitStubs->hostFunctionStub(this, function, constructor);
}
NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function, Intrinsic intrinsic)
{
    ASSERT(canUseJIT());
    return jitStubs->hostFunctionStub(this, function, intrinsic != NoIntrinsic ? thunkGeneratorForIntrinsic(intrinsic) : 0, intrinsic);
}

#else // !ENABLE(JIT)
NativeExecutable* JSGlobalData::getHostFunction(NativeFunction function, NativeFunction constructor)
{
    return NativeExecutable::create(*this, function, constructor);
}
#endif // !ENABLE(JIT)

JSGlobalData::ClientData::~ClientData()
{
}

void JSGlobalData::resetDateCache()
{
    cachedUTCOffset = std::numeric_limits<double>::quiet_NaN();
    dstOffsetCache.reset();
    cachedDateString = String();
    cachedDateStringValue = std::numeric_limits<double>::quiet_NaN();
    dateInstanceCache.reset();
}

void JSGlobalData::startSampling()
{
    interpreter->startSampling();
}

void JSGlobalData::stopSampling()
{
    interpreter->stopSampling();
}

void JSGlobalData::dumpSampleData(ExecState* exec)
{
    interpreter->dumpSampleData(exec);
#if ENABLE(ASSEMBLER)
    ExecutableAllocator::dumpProfile();
#endif
}

struct StackPreservingRecompiler : public MarkedBlock::VoidFunctor {
    HashSet<FunctionExecutable*> currentlyExecutingFunctions;
    void operator()(JSCell* cell)
    {
        if (!cell->inherits(&FunctionExecutable::s_info))
            return;
        FunctionExecutable* executable = jsCast<FunctionExecutable*>(cell);
        if (currentlyExecutingFunctions.contains(executable))
            return;
        executable->clearCodeIfNotCompiling();
    }
};

void JSGlobalData::releaseExecutableMemory()
{
    if (dynamicGlobalObject) {
        StackPreservingRecompiler recompiler;
        HashSet<JSCell*> roots;
        heap.getConservativeRegisterRoots(roots);
        HashSet<JSCell*>::iterator end = roots.end();
        for (HashSet<JSCell*>::iterator ptr = roots.begin(); ptr != end; ++ptr) {
            ScriptExecutable* executable = 0;
            JSCell* cell = *ptr;
            if (cell->inherits(&ScriptExecutable::s_info))
                executable = static_cast<ScriptExecutable*>(*ptr);
            else if (cell->inherits(&JSFunction::s_info)) {
                JSFunction* function = jsCast<JSFunction*>(*ptr);
                if (function->isHostFunction())
                    continue;
                executable = function->jsExecutable();
            } else
                continue;
            ASSERT(executable->inherits(&ScriptExecutable::s_info));
            executable->unlinkCalls();
            if (executable->inherits(&FunctionExecutable::s_info))
                recompiler.currentlyExecutingFunctions.add(static_cast<FunctionExecutable*>(executable));
                
        }
        heap.objectSpace().forEachLiveCell<StackPreservingRecompiler>(recompiler);
    }
    m_regExpCache->invalidateCode();
    heap.collectAllGarbage();
}
    
void releaseExecutableMemory(JSGlobalData& globalData)
{
    globalData.releaseExecutableMemory();
}

#if ENABLE(DFG_JIT)
void JSGlobalData::gatherConservativeRoots(ConservativeRoots& conservativeRoots)
{
    for (size_t i = 0; i < scratchBuffers.size(); i++) {
        ScratchBuffer* scratchBuffer = scratchBuffers[i];
        if (scratchBuffer->activeLength()) {
            void* bufferStart = scratchBuffer->dataBuffer();
            conservativeRoots.add(bufferStart, static_cast<void*>(static_cast<char*>(bufferStart) + scratchBuffer->activeLength()));
        }
    }
}
#endif

#if ENABLE(REGEXP_TRACING)
void JSGlobalData::addRegExpToTrace(RegExp* regExp)
{
    m_rtTraceList->add(regExp);
}

void JSGlobalData::dumpRegExpTrace()
{
    // The first RegExp object is ignored.  It is create by the RegExpPrototype ctor and not used.
    RTTraceList::iterator iter = ++m_rtTraceList->begin();
    
    if (iter != m_rtTraceList->end()) {
        dataLog("\nRegExp Tracing\n");
        dataLog("                                                            match()    matches\n");
        dataLog("Regular Expression                          JIT Address      calls      found\n");
        dataLog("----------------------------------------+----------------+----------+----------\n");
    
        unsigned reCount = 0;
    
        for (; iter != m_rtTraceList->end(); ++iter, ++reCount)
            (*iter)->printTraceData();

        dataLog("%d Regular Expressions\n", reCount);
    }
    
    m_rtTraceList->clear();
}
#else
void JSGlobalData::dumpRegExpTrace()
{
}
#endif

} // namespace JSC
