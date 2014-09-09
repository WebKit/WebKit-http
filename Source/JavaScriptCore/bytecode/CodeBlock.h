/*
 * Copyright (C) 2008, 2009, 2010, 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#ifndef CodeBlock_h
#define CodeBlock_h

#include "ArrayProfile.h"
#include "ByValInfo.h"
#include "BytecodeConventions.h"
#include "BytecodeLivenessAnalysis.h"
#include "CallLinkInfo.h"
#include "CallReturnOffsetToBytecodeOffset.h"
#include "CodeBlockHash.h"
#include "CodeBlockSet.h"
#include "ConcurrentJITLock.h"
#include "CodeOrigin.h"
#include "CodeType.h"
#include "CompactJITCodeMap.h"
#include "DFGCommon.h"
#include "DFGCommonData.h"
#include "DFGExitProfile.h"
#include "DeferredCompilationCallback.h"
#include "EvalCodeCache.h"
#include "ExecutionCounter.h"
#include "ExpressionRangeInfo.h"
#include "HandlerInfo.h"
#include "ObjectAllocationProfile.h"
#include "Options.h"
#include "PutPropertySlot.h"
#include "Instruction.h"
#include "JITCode.h"
#include "JITWriteBarrier.h"
#include "JSGlobalObject.h"
#include "JumpTable.h"
#include "LLIntCallLinkInfo.h"
#include "LazyOperandValueProfile.h"
#include "ProfilerCompilation.h"
#include "ProfilerJettisonReason.h"
#include "RegExpObject.h"
#include "StructureStubInfo.h"
#include "UnconditionalFinalizer.h"
#include "ValueProfile.h"
#include "VirtualRegister.h"
#include "Watchpoint.h"
#include <wtf/Bag.h>
#include <wtf/FastMalloc.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCountedArray.h>
#include <wtf/RefPtr.h>
#include <wtf/SegmentedVector.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace JSC {

class ExecState;
class LLIntOffsetsExtractor;
class RepatchBuffer;

inline VirtualRegister unmodifiedArgumentsRegister(VirtualRegister argumentsRegister) { return VirtualRegister(argumentsRegister.offset() + 1); }

static ALWAYS_INLINE int missingThisObjectMarker() { return std::numeric_limits<int>::max(); }

enum ReoptimizationMode { DontCountReoptimization, CountReoptimization };

class CodeBlock : public ThreadSafeRefCounted<CodeBlock>, public UnconditionalFinalizer, public WeakReferenceHarvester {
    WTF_MAKE_FAST_ALLOCATED;
    friend class BytecodeLivenessAnalysis;
    friend class JIT;
    friend class LLIntOffsetsExtractor;
public:
    enum CopyParsedBlockTag { CopyParsedBlock };
protected:
    CodeBlock(CopyParsedBlockTag, CodeBlock& other);
        
    CodeBlock(ScriptExecutable* ownerExecutable, UnlinkedCodeBlock*, JSScope*, PassRefPtr<SourceProvider>, unsigned sourceOffset, unsigned firstLineColumnOffset);

    WriteBarrier<JSGlobalObject> m_globalObject;
    Heap* m_heap;

public:
    JS_EXPORT_PRIVATE virtual ~CodeBlock();

    UnlinkedCodeBlock* unlinkedCodeBlock() const { return m_unlinkedCode.get(); }

    CString inferredName() const;
    CodeBlockHash hash() const;
    bool hasHash() const;
    bool isSafeToComputeHash() const;
    CString hashAsStringIfPossible() const;
    CString sourceCodeForTools() const; // Not quite the actual source we parsed; this will do things like prefix the source for a function with a reified signature.
    CString sourceCodeOnOneLine() const; // As sourceCodeForTools(), but replaces all whitespace runs with a single space.
    void dumpAssumingJITType(PrintStream&, JITCode::JITType) const;
    void dump(PrintStream&) const;

    int numParameters() const { return m_numParameters; }
    void setNumParameters(int newValue);

    int* addressOfNumParameters() { return &m_numParameters; }
    static ptrdiff_t offsetOfNumParameters() { return OBJECT_OFFSETOF(CodeBlock, m_numParameters); }

    CodeBlock* alternative() { return m_alternative.get(); }
    PassRefPtr<CodeBlock> releaseAlternative() { return m_alternative.release(); }
    void setAlternative(PassRefPtr<CodeBlock> alternative) { m_alternative = alternative; }

    template <typename Functor> void forEachRelatedCodeBlock(Functor&& functor)
    {
        Functor f(std::forward<Functor>(functor));
        Vector<CodeBlock*, 4> codeBlocks;
        codeBlocks.append(this);

        while (!codeBlocks.isEmpty()) {
            CodeBlock* currentCodeBlock = codeBlocks.takeLast();
            f(currentCodeBlock);

            if (CodeBlock* alternative = currentCodeBlock->alternative())
                codeBlocks.append(alternative);
            if (CodeBlock* osrEntryBlock = currentCodeBlock->specialOSREntryBlockOrNull())
                codeBlocks.append(osrEntryBlock);
        }
    }
    
    CodeSpecializationKind specializationKind() const
    {
        return specializationFromIsConstruct(m_isConstructor);
    }
    
    CodeBlock* baselineAlternative();
    
    // FIXME: Get rid of this.
    // https://bugs.webkit.org/show_bug.cgi?id=123677
    CodeBlock* baselineVersion();

    void visitAggregate(SlotVisitor&);

    void dumpBytecode(PrintStream& = WTF::dataFile());
    void dumpBytecode(
        PrintStream&, unsigned bytecodeOffset,
        const StubInfoMap& = StubInfoMap(), const CallLinkInfoMap& = CallLinkInfoMap());
    void printStructures(PrintStream&, const Instruction*);
    void printStructure(PrintStream&, const char* name, const Instruction*, int operand);

    bool isStrictMode() const { return m_isStrictMode; }
    ECMAMode ecmaMode() const { return isStrictMode() ? StrictMode : NotStrictMode; }

    inline bool isKnownNotImmediate(int index)
    {
        if (index == m_thisRegister.offset() && !m_isStrictMode)
            return true;

        if (isConstantRegisterIndex(index))
            return getConstant(index).isCell();

        return false;
    }

    ALWAYS_INLINE bool isTemporaryRegisterIndex(int index)
    {
        return index >= m_numVars;
    }

    HandlerInfo* handlerForBytecodeOffset(unsigned bytecodeOffset);
    unsigned lineNumberForBytecodeOffset(unsigned bytecodeOffset);
    unsigned columnNumberForBytecodeOffset(unsigned bytecodeOffset);
    void expressionRangeForBytecodeOffset(unsigned bytecodeOffset, int& divot,
                                          int& startOffset, int& endOffset, unsigned& line, unsigned& column);

    void getStubInfoMap(const ConcurrentJITLocker&, StubInfoMap& result);
    void getStubInfoMap(StubInfoMap& result);
    
    void getCallLinkInfoMap(const ConcurrentJITLocker&, CallLinkInfoMap& result);
    void getCallLinkInfoMap(CallLinkInfoMap& result);
    
#if ENABLE(JIT)
    StructureStubInfo* addStubInfo();
    Bag<StructureStubInfo>::iterator stubInfoBegin() { return m_stubInfos.begin(); }
    Bag<StructureStubInfo>::iterator stubInfoEnd() { return m_stubInfos.end(); }
    
    // O(n) operation. Use getStubInfoMap() unless you really only intend to get one
    // stub info.
    StructureStubInfo* findStubInfo(CodeOrigin);

    void resetStub(StructureStubInfo&);
    
    ByValInfo& getByValInfo(unsigned bytecodeIndex)
    {
        return *(binarySearch<ByValInfo, unsigned>(m_byValInfos, m_byValInfos.size(), bytecodeIndex, getByValInfoBytecodeIndex));
    }

    CallLinkInfo* addCallLinkInfo();
    Bag<CallLinkInfo>::iterator callLinkInfosBegin() { return m_callLinkInfos.begin(); }
    Bag<CallLinkInfo>::iterator callLinkInfosEnd() { return m_callLinkInfos.end(); }

    // This is a slow function call used primarily for compiling OSR exits in the case
    // that there had been inlining. Chances are if you want to use this, you're really
    // looking for a CallLinkInfoMap to amortize the cost of calling this.
    CallLinkInfo* getCallLinkInfoForBytecodeIndex(unsigned bytecodeIndex);
#endif // ENABLE(JIT)

    void unlinkIncomingCalls();

#if ENABLE(JIT)
    void unlinkCalls();
        
    void linkIncomingCall(ExecState* callerFrame, CallLinkInfo*);
        
    bool isIncomingCallAlreadyLinked(CallLinkInfo* incoming)
    {
        return m_incomingCalls.isOnList(incoming);
    }
#endif // ENABLE(JIT)

    void linkIncomingCall(ExecState* callerFrame, LLIntCallLinkInfo*);

    void setJITCodeMap(PassOwnPtr<CompactJITCodeMap> jitCodeMap)
    {
        m_jitCodeMap = jitCodeMap;
    }
    CompactJITCodeMap* jitCodeMap()
    {
        return m_jitCodeMap.get();
    }
    
    unsigned bytecodeOffset(Instruction* returnAddress)
    {
        RELEASE_ASSERT(returnAddress >= instructions().begin() && returnAddress < instructions().end());
        return static_cast<Instruction*>(returnAddress) - instructions().begin();
    }

    bool isNumericCompareFunction() { return m_unlinkedCode->isNumericCompareFunction(); }

    unsigned numberOfInstructions() const { return m_instructions.size(); }
    RefCountedArray<Instruction>& instructions() { return m_instructions; }
    const RefCountedArray<Instruction>& instructions() const { return m_instructions; }

    size_t predictedMachineCodeSize();

    bool usesOpcode(OpcodeID);

    unsigned instructionCount() const { return m_instructions.size(); }

    int argumentIndexAfterCapture(size_t argument);
    
    bool hasSlowArguments();
    const SlowArgument* machineSlowArguments();

    // Exactly equivalent to codeBlock->ownerExecutable()->installCode(codeBlock);
    void install();
    
    // Exactly equivalent to codeBlock->ownerExecutable()->newReplacementCodeBlockFor(codeBlock->specializationKind())
    PassRefPtr<CodeBlock> newReplacement();
    
    void setJITCode(PassRefPtr<JITCode> code)
    {
        ASSERT(m_heap->isDeferred());
        m_heap->reportExtraMemoryCost(code->size());
        ConcurrentJITLocker locker(m_lock);
        WTF::storeStoreFence(); // This is probably not needed because the lock will also do something similar, but it's good to be paranoid.
        m_jitCode = code;
    }
    PassRefPtr<JITCode> jitCode() { return m_jitCode; }
    JITCode::JITType jitType() const
    {
        JITCode* jitCode = m_jitCode.get();
        WTF::loadLoadFence();
        JITCode::JITType result = JITCode::jitTypeFor(jitCode);
        WTF::loadLoadFence(); // This probably isn't needed. Oh well, paranoia is good.
        return result;
    }

    bool hasBaselineJITProfiling() const
    {
        return jitType() == JITCode::BaselineJIT;
    }
    
#if ENABLE(JIT)
    virtual CodeBlock* replacement() = 0;

    virtual DFG::CapabilityLevel capabilityLevelInternal() = 0;
    DFG::CapabilityLevel capabilityLevel();
    DFG::CapabilityLevel capabilityLevelState() { return m_capabilityLevelState; }

    bool hasOptimizedReplacement(JITCode::JITType typeToReplace);
    bool hasOptimizedReplacement(); // the typeToReplace is my JITType
#endif

    void jettison(Profiler::JettisonReason, ReoptimizationMode = DontCountReoptimization);
    
    ScriptExecutable* ownerExecutable() const { return m_ownerExecutable.get(); }

    void setVM(VM* vm) { m_vm = vm; }
    VM* vm() { return m_vm; }

    void setThisRegister(VirtualRegister thisRegister) { m_thisRegister = thisRegister; }
    VirtualRegister thisRegister() const { return m_thisRegister; }

    bool usesEval() const { return m_unlinkedCode->usesEval(); }

    void setArgumentsRegister(VirtualRegister argumentsRegister)
    {
        ASSERT(argumentsRegister.isValid());
        m_argumentsRegister = argumentsRegister;
        ASSERT(usesArguments());
    }
    VirtualRegister argumentsRegister() const
    {
        ASSERT(usesArguments());
        return m_argumentsRegister;
    }
    VirtualRegister uncheckedArgumentsRegister()
    {
        if (!usesArguments())
            return VirtualRegister();
        return argumentsRegister();
    }
    void setActivationRegister(VirtualRegister activationRegister)
    {
        m_activationRegister = activationRegister;
    }

    VirtualRegister activationRegister() const
    {
        ASSERT(m_activationRegister.isValid());
        return m_activationRegister;
    }

    VirtualRegister uncheckedActivationRegister()
    {
        return m_activationRegister;
    }

    bool usesArguments() const { return m_argumentsRegister.isValid(); }

    bool needsActivation() const
    {
        ASSERT(m_activationRegister.isValid() == m_needsActivation);
        return m_needsActivation;
    }
    
    unsigned captureCount() const
    {
        if (!symbolTable())
            return 0;
        return symbolTable()->captureCount();
    }
    
    int captureStart() const
    {
        if (!symbolTable())
            return 0;
        return symbolTable()->captureStart();
    }
    
    int captureEnd() const
    {
        if (!symbolTable())
            return 0;
        return symbolTable()->captureEnd();
    }

    bool isCaptured(VirtualRegister operand, InlineCallFrame* = 0) const;
    
    int framePointerOffsetToGetActivationRegisters(int machineCaptureStart);
    int framePointerOffsetToGetActivationRegisters();

    CodeType codeType() const { return m_unlinkedCode->codeType(); }
    PutPropertySlot::Context putByIdContext() const
    {
        if (codeType() == EvalCode)
            return PutPropertySlot::PutByIdEval;
        return PutPropertySlot::PutById;
    }

    SourceProvider* source() const { return m_source.get(); }
    unsigned sourceOffset() const { return m_sourceOffset; }
    unsigned firstLineColumnOffset() const { return m_firstLineColumnOffset; }

    size_t numberOfJumpTargets() const { return m_unlinkedCode->numberOfJumpTargets(); }
    unsigned jumpTarget(int index) const { return m_unlinkedCode->jumpTarget(index); }

    void clearEvalCache();

    String nameForRegister(VirtualRegister);

#if ENABLE(JIT)
    void setNumberOfByValInfos(size_t size) { m_byValInfos.resizeToFit(size); }
    size_t numberOfByValInfos() const { return m_byValInfos.size(); }
    ByValInfo& byValInfo(size_t index) { return m_byValInfos[index]; }
#endif

    unsigned numberOfArgumentValueProfiles()
    {
        ASSERT(m_numParameters >= 0);
        ASSERT(m_argumentValueProfiles.size() == static_cast<unsigned>(m_numParameters));
        return m_argumentValueProfiles.size();
    }
    ValueProfile* valueProfileForArgument(unsigned argumentIndex)
    {
        ValueProfile* result = &m_argumentValueProfiles[argumentIndex];
        ASSERT(result->m_bytecodeOffset == -1);
        return result;
    }

    unsigned numberOfValueProfiles() { return m_valueProfiles.size(); }
    ValueProfile* valueProfile(int index) { return &m_valueProfiles[index]; }
    ValueProfile* valueProfileForBytecodeOffset(int bytecodeOffset)
    {
        ValueProfile* result = binarySearch<ValueProfile, int>(
            m_valueProfiles, m_valueProfiles.size(), bytecodeOffset,
            getValueProfileBytecodeOffset<ValueProfile>);
        ASSERT(result->m_bytecodeOffset != -1);
        ASSERT(instructions()[bytecodeOffset + opcodeLength(
            m_vm->interpreter->getOpcodeID(
                instructions()[bytecodeOffset].u.opcode)) - 1].u.profile == result);
        return result;
    }
    SpeculatedType valueProfilePredictionForBytecodeOffset(const ConcurrentJITLocker& locker, int bytecodeOffset)
    {
        return valueProfileForBytecodeOffset(bytecodeOffset)->computeUpdatedPrediction(locker);
    }

    unsigned totalNumberOfValueProfiles()
    {
        return numberOfArgumentValueProfiles() + numberOfValueProfiles();
    }
    ValueProfile* getFromAllValueProfiles(unsigned index)
    {
        if (index < numberOfArgumentValueProfiles())
            return valueProfileForArgument(index);
        return valueProfile(index - numberOfArgumentValueProfiles());
    }

    RareCaseProfile* addRareCaseProfile(int bytecodeOffset)
    {
        m_rareCaseProfiles.append(RareCaseProfile(bytecodeOffset));
        return &m_rareCaseProfiles.last();
    }
    unsigned numberOfRareCaseProfiles() { return m_rareCaseProfiles.size(); }
    RareCaseProfile* rareCaseProfile(int index) { return &m_rareCaseProfiles[index]; }
    RareCaseProfile* rareCaseProfileForBytecodeOffset(int bytecodeOffset);

    bool likelyToTakeSlowCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned value = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        return value >= Options::likelyToTakeSlowCaseMinimumCount();
    }

    bool couldTakeSlowCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned value = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        return value >= Options::couldTakeSlowCaseMinimumCount();
    }

    RareCaseProfile* addSpecialFastCaseProfile(int bytecodeOffset)
    {
        m_specialFastCaseProfiles.append(RareCaseProfile(bytecodeOffset));
        return &m_specialFastCaseProfiles.last();
    }
    unsigned numberOfSpecialFastCaseProfiles() { return m_specialFastCaseProfiles.size(); }
    RareCaseProfile* specialFastCaseProfile(int index) { return &m_specialFastCaseProfiles[index]; }
    RareCaseProfile* specialFastCaseProfileForBytecodeOffset(int bytecodeOffset)
    {
        return tryBinarySearch<RareCaseProfile, int>(
            m_specialFastCaseProfiles, m_specialFastCaseProfiles.size(), bytecodeOffset,
            getRareCaseProfileBytecodeOffset);
    }

    bool likelyToTakeSpecialFastCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        return specialFastCaseCount >= Options::likelyToTakeSlowCaseMinimumCount();
    }

    bool couldTakeSpecialFastCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        return specialFastCaseCount >= Options::couldTakeSlowCaseMinimumCount();
    }

    bool likelyToTakeDeepestSlowCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned slowCaseCount = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        unsigned value = slowCaseCount - specialFastCaseCount;
        return value >= Options::likelyToTakeSlowCaseMinimumCount();
    }

    bool likelyToTakeAnySlowCase(int bytecodeOffset)
    {
        if (!hasBaselineJITProfiling())
            return false;
        unsigned slowCaseCount = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
        unsigned value = slowCaseCount + specialFastCaseCount;
        return value >= Options::likelyToTakeSlowCaseMinimumCount();
    }

    unsigned numberOfArrayProfiles() const { return m_arrayProfiles.size(); }
    const ArrayProfileVector& arrayProfiles() { return m_arrayProfiles; }
    ArrayProfile* addArrayProfile(unsigned bytecodeOffset)
    {
        m_arrayProfiles.append(ArrayProfile(bytecodeOffset));
        return &m_arrayProfiles.last();
    }
    ArrayProfile* getArrayProfile(unsigned bytecodeOffset);
    ArrayProfile* getOrAddArrayProfile(unsigned bytecodeOffset);

    // Exception handling support

    size_t numberOfExceptionHandlers() const { return m_rareData ? m_rareData->m_exceptionHandlers.size() : 0; }
    HandlerInfo& exceptionHandler(int index) { RELEASE_ASSERT(m_rareData); return m_rareData->m_exceptionHandlers[index]; }

    bool hasExpressionInfo() { return m_unlinkedCode->hasExpressionInfo(); }

#if ENABLE(DFG_JIT)
    Vector<CodeOrigin, 0, UnsafeVectorOverflow>& codeOrigins()
    {
        return m_jitCode->dfgCommon()->codeOrigins;
    }
    
    // Having code origins implies that there has been some inlining.
    bool hasCodeOrigins()
    {
        return JITCode::isOptimizingJIT(jitType());
    }
        
    bool canGetCodeOrigin(unsigned index)
    {
        if (!hasCodeOrigins())
            return false;
        return index < codeOrigins().size();
    }

    CodeOrigin codeOrigin(unsigned index)
    {
        return codeOrigins()[index];
    }

    bool addFrequentExitSite(const DFG::FrequentExitSite& site)
    {
        ASSERT(JITCode::isBaselineCode(jitType()));
        ConcurrentJITLocker locker(m_lock);
        return m_exitProfile.add(locker, site);
    }

    bool hasExitSite(const ConcurrentJITLocker& locker, const DFG::FrequentExitSite& site) const
    {
        return m_exitProfile.hasExitSite(locker, site);
    }
    bool hasExitSite(const DFG::FrequentExitSite& site) const
    {
        ConcurrentJITLocker locker(m_lock);
        return hasExitSite(locker, site);
    }

    DFG::ExitProfile& exitProfile() { return m_exitProfile; }

    CompressedLazyOperandValueProfileHolder& lazyOperandValueProfiles()
    {
        return m_lazyOperandValueProfiles;
    }
#endif // ENABLE(DFG_JIT)

    // Constant Pool
#if ENABLE(DFG_JIT)
    size_t numberOfIdentifiers() const { return m_unlinkedCode->numberOfIdentifiers() + numberOfDFGIdentifiers(); }
    size_t numberOfDFGIdentifiers() const
    {
        if (!JITCode::isOptimizingJIT(jitType()))
            return 0;

        return m_jitCode->dfgCommon()->dfgIdentifiers.size();
    }

    const Identifier& identifier(int index) const
    {
        size_t unlinkedIdentifiers = m_unlinkedCode->numberOfIdentifiers();
        if (static_cast<unsigned>(index) < unlinkedIdentifiers)
            return m_unlinkedCode->identifier(index);
        ASSERT(JITCode::isOptimizingJIT(jitType()));
        return m_jitCode->dfgCommon()->dfgIdentifiers[index - unlinkedIdentifiers];
    }
#else
    size_t numberOfIdentifiers() const { return m_unlinkedCode->numberOfIdentifiers(); }
    const Identifier& identifier(int index) const { return m_unlinkedCode->identifier(index); }
#endif

    Vector<WriteBarrier<Unknown>>& constants() { return m_constantRegisters; }
    size_t numberOfConstantRegisters() const { return m_constantRegisters.size(); }
    unsigned addConstant(JSValue v)
    {
        unsigned result = m_constantRegisters.size();
        m_constantRegisters.append(WriteBarrier<Unknown>());
        m_constantRegisters.last().set(m_globalObject->vm(), m_ownerExecutable.get(), v);
        return result;
    }

    unsigned addConstantLazily()
    {
        unsigned result = m_constantRegisters.size();
        m_constantRegisters.append(WriteBarrier<Unknown>());
        return result;
    }

    bool findConstant(JSValue, unsigned& result);
    unsigned addOrFindConstant(JSValue);
    WriteBarrier<Unknown>& constantRegister(int index) { return m_constantRegisters[index - FirstConstantRegisterIndex]; }
    ALWAYS_INLINE bool isConstantRegisterIndex(int index) const { return index >= FirstConstantRegisterIndex; }
    ALWAYS_INLINE JSValue getConstant(int index) const { return m_constantRegisters[index - FirstConstantRegisterIndex].get(); }

    FunctionExecutable* functionDecl(int index) { return m_functionDecls[index].get(); }
    int numberOfFunctionDecls() { return m_functionDecls.size(); }
    FunctionExecutable* functionExpr(int index) { return m_functionExprs[index].get(); }

    RegExp* regexp(int index) const { return m_unlinkedCode->regexp(index); }

    unsigned numberOfConstantBuffers() const
    {
        if (!m_rareData)
            return 0;
        return m_rareData->m_constantBuffers.size();
    }
    unsigned addConstantBuffer(const Vector<JSValue>& buffer)
    {
        createRareDataIfNecessary();
        unsigned size = m_rareData->m_constantBuffers.size();
        m_rareData->m_constantBuffers.append(buffer);
        return size;
    }

    Vector<JSValue>& constantBufferAsVector(unsigned index)
    {
        ASSERT(m_rareData);
        return m_rareData->m_constantBuffers[index];
    }
    JSValue* constantBuffer(unsigned index)
    {
        return constantBufferAsVector(index).data();
    }

    Heap* heap() const { return m_heap; }
    JSGlobalObject* globalObject() { return m_globalObject.get(); }

    JSGlobalObject* globalObjectFor(CodeOrigin);

    BytecodeLivenessAnalysis& livenessAnalysis()
    {
        {
            ConcurrentJITLocker locker(m_lock);
            if (!!m_livenessAnalysis)
                return *m_livenessAnalysis;
        }
        std::unique_ptr<BytecodeLivenessAnalysis> analysis =
            std::make_unique<BytecodeLivenessAnalysis>(this);
        {
            ConcurrentJITLocker locker(m_lock);
            if (!m_livenessAnalysis)
                m_livenessAnalysis = WTF::move(analysis);
            return *m_livenessAnalysis;
        }
    }
    
    void validate();

    // Jump Tables

    size_t numberOfSwitchJumpTables() const { return m_rareData ? m_rareData->m_switchJumpTables.size() : 0; }
    SimpleJumpTable& addSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_switchJumpTables.append(SimpleJumpTable()); return m_rareData->m_switchJumpTables.last(); }
    SimpleJumpTable& switchJumpTable(int tableIndex) { RELEASE_ASSERT(m_rareData); return m_rareData->m_switchJumpTables[tableIndex]; }
    void clearSwitchJumpTables()
    {
        if (!m_rareData)
            return;
        m_rareData->m_switchJumpTables.clear();
    }

    size_t numberOfStringSwitchJumpTables() const { return m_rareData ? m_rareData->m_stringSwitchJumpTables.size() : 0; }
    StringJumpTable& addStringSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_stringSwitchJumpTables.append(StringJumpTable()); return m_rareData->m_stringSwitchJumpTables.last(); }
    StringJumpTable& stringSwitchJumpTable(int tableIndex) { RELEASE_ASSERT(m_rareData); return m_rareData->m_stringSwitchJumpTables[tableIndex]; }


    SymbolTable* symbolTable() const { return m_symbolTable.get(); }

    EvalCodeCache& evalCodeCache() { createRareDataIfNecessary(); return m_rareData->m_evalCodeCache; }

    enum ShrinkMode {
        // Shrink prior to generating machine code that may point directly into vectors.
        EarlyShrink,

        // Shrink after generating machine code, and after possibly creating new vectors
        // and appending to others. At this time it is not safe to shrink certain vectors
        // because we would have generated machine code that references them directly.
        LateShrink
    };
    void shrinkToFit(ShrinkMode);

    // Functions for controlling when JITting kicks in, in a mixed mode
    // execution world.

    bool checkIfJITThresholdReached()
    {
        return m_llintExecuteCounter.checkIfThresholdCrossedAndSet(this);
    }

    void dontJITAnytimeSoon()
    {
        m_llintExecuteCounter.deferIndefinitely();
    }

    void jitAfterWarmUp()
    {
        m_llintExecuteCounter.setNewThreshold(Options::thresholdForJITAfterWarmUp(), this);
    }

    void jitSoon()
    {
        m_llintExecuteCounter.setNewThreshold(Options::thresholdForJITSoon(), this);
    }

    const BaselineExecutionCounter& llintExecuteCounter() const
    {
        return m_llintExecuteCounter;
    }

    // Functions for controlling when tiered compilation kicks in. This
    // controls both when the optimizing compiler is invoked and when OSR
    // entry happens. Two triggers exist: the loop trigger and the return
    // trigger. In either case, when an addition to m_jitExecuteCounter
    // causes it to become non-negative, the optimizing compiler is
    // invoked. This includes a fast check to see if this CodeBlock has
    // already been optimized (i.e. replacement() returns a CodeBlock
    // that was optimized with a higher tier JIT than this one). In the
    // case of the loop trigger, if the optimized compilation succeeds
    // (or has already succeeded in the past) then OSR is attempted to
    // redirect program flow into the optimized code.

    // These functions are called from within the optimization triggers,
    // and are used as a single point at which we define the heuristics
    // for how much warm-up is mandated before the next optimization
    // trigger files. All CodeBlocks start out with optimizeAfterWarmUp(),
    // as this is called from the CodeBlock constructor.

    // When we observe a lot of speculation failures, we trigger a
    // reoptimization. But each time, we increase the optimization trigger
    // to avoid thrashing.
    JS_EXPORT_PRIVATE unsigned reoptimizationRetryCounter() const;
    void countReoptimization();
#if ENABLE(JIT)
    unsigned numberOfDFGCompiles();

    int32_t codeTypeThresholdMultiplier() const;

    int32_t adjustedCounterValue(int32_t desiredThreshold);

    int32_t* addressOfJITExecuteCounter()
    {
        return &m_jitExecuteCounter.m_counter;
    }

    static ptrdiff_t offsetOfJITExecuteCounter() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(BaselineExecutionCounter, m_counter); }
    static ptrdiff_t offsetOfJITExecutionActiveThreshold() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(BaselineExecutionCounter, m_activeThreshold); }
    static ptrdiff_t offsetOfJITExecutionTotalCount() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(BaselineExecutionCounter, m_totalCount); }

    const BaselineExecutionCounter& jitExecuteCounter() const { return m_jitExecuteCounter; }

    unsigned optimizationDelayCounter() const { return m_optimizationDelayCounter; }

    // Check if the optimization threshold has been reached, and if not,
    // adjust the heuristics accordingly. Returns true if the threshold has
    // been reached.
    bool checkIfOptimizationThresholdReached();

    // Call this to force the next optimization trigger to fire. This is
    // rarely wise, since optimization triggers are typically more
    // expensive than executing baseline code.
    void optimizeNextInvocation();

    // Call this to prevent optimization from happening again. Note that
    // optimization will still happen after roughly 2^29 invocations,
    // so this is really meant to delay that as much as possible. This
    // is called if optimization failed, and we expect it to fail in
    // the future as well.
    void dontOptimizeAnytimeSoon();

    // Call this to reinitialize the counter to its starting state,
    // forcing a warm-up to happen before the next optimization trigger
    // fires. This is called in the CodeBlock constructor. It also
    // makes sense to call this if an OSR exit occurred. Note that
    // OSR exit code is code generated, so the value of the execute
    // counter that this corresponds to is also available directly.
    void optimizeAfterWarmUp();

    // Call this to force an optimization trigger to fire only after
    // a lot of warm-up.
    void optimizeAfterLongWarmUp();

    // Call this to cause an optimization trigger to fire soon, but
    // not necessarily the next one. This makes sense if optimization
    // succeeds. Successfuly optimization means that all calls are
    // relinked to the optimized code, so this only affects call
    // frames that are still executing this CodeBlock. The value here
    // is tuned to strike a balance between the cost of OSR entry
    // (which is too high to warrant making every loop back edge to
    // trigger OSR immediately) and the cost of executing baseline
    // code (which is high enough that we don't necessarily want to
    // have a full warm-up). The intuition for calling this instead of
    // optimizeNextInvocation() is for the case of recursive functions
    // with loops. Consider that there may be N call frames of some
    // recursive function, for a reasonably large value of N. The top
    // one triggers optimization, and then returns, and then all of
    // the others return. We don't want optimization to be triggered on
    // each return, as that would be superfluous. It only makes sense
    // to trigger optimization if one of those functions becomes hot
    // in the baseline code.
    void optimizeSoon();

    void forceOptimizationSlowPathConcurrently();

    void setOptimizationThresholdBasedOnCompilationResult(CompilationResult);
    
    uint32_t osrExitCounter() const { return m_osrExitCounter; }

    void countOSRExit() { m_osrExitCounter++; }

    uint32_t* addressOfOSRExitCounter() { return &m_osrExitCounter; }

    static ptrdiff_t offsetOfOSRExitCounter() { return OBJECT_OFFSETOF(CodeBlock, m_osrExitCounter); }

    uint32_t adjustedExitCountThreshold(uint32_t desiredThreshold);
    uint32_t exitCountThresholdForReoptimization();
    uint32_t exitCountThresholdForReoptimizationFromLoop();
    bool shouldReoptimizeNow();
    bool shouldReoptimizeFromLoopNow();
#else // No JIT
    void optimizeAfterWarmUp() { }
    unsigned numberOfDFGCompiles() { return 0; }
#endif

    bool shouldOptimizeNow();
    void updateAllValueProfilePredictions();
    void updateAllArrayPredictions();
    void updateAllPredictions();

    unsigned frameRegisterCount();
    int stackPointerOffset();

    bool hasOpDebugForLineAndColumn(unsigned line, unsigned column);

    bool hasDebuggerRequests() const { return m_debuggerRequests; }
    void* debuggerRequestsAddress() { return &m_debuggerRequests; }

    void addBreakpoint(unsigned numBreakpoints);
    void removeBreakpoint(unsigned numBreakpoints)
    {
        ASSERT(m_numBreakpoints >= numBreakpoints);
        m_numBreakpoints -= numBreakpoints;
    }

    enum SteppingMode {
        SteppingModeDisabled,
        SteppingModeEnabled
    };
    void setSteppingMode(SteppingMode);

    void clearDebuggerRequests()
    {
        m_steppingMode = SteppingModeDisabled;
        m_numBreakpoints = 0;
    }
    
    // FIXME: Make these remaining members private.

    int m_numCalleeRegisters;
    int m_numVars;
    bool m_isConstructor : 1;
    
    // This is intentionally public; it's the responsibility of anyone doing any
    // of the following to hold the lock:
    //
    // - Modifying any inline cache in this code block.
    //
    // - Quering any inline cache in this code block, from a thread other than
    //   the main thread.
    //
    // Additionally, it's only legal to modify the inline cache on the main
    // thread. This means that the main thread can query the inline cache without
    // locking. This is crucial since executing the inline cache is effectively
    // "querying" it.
    //
    // Another exception to the rules is that the GC can do whatever it wants
    // without holding any locks, because the GC is guaranteed to wait until any
    // concurrent compilation threads finish what they're doing.
    mutable ConcurrentJITLock m_lock;
    
    bool m_shouldAlwaysBeInlined; // Not a bitfield because the JIT wants to store to it.
    bool m_allTransitionsHaveBeenMarked : 1; // Initialized and used on every GC.
    
    bool m_didFailFTLCompilation : 1;
    bool m_hasBeenCompiledWithFTL : 1;

    // Internal methods for use by validation code. It would be private if it wasn't
    // for the fact that we use it from anonymous namespaces.
    void beginValidationDidFail();
    NO_RETURN_DUE_TO_CRASH void endValidationDidFail();

    bool isKnownToBeLiveDuringGC(); // Will only return valid results when called during GC. Assumes that you've already established that the owner executable is live.


protected:
    virtual void visitWeakReferences(SlotVisitor&) override;
    virtual void finalizeUnconditionally() override;

#if ENABLE(DFG_JIT)
    void tallyFrequentExitSites();
#else
    void tallyFrequentExitSites() { }
#endif

private:
    friend class CodeBlockSet;
    
    CodeBlock* specialOSREntryBlockOrNull();
    
    void noticeIncomingCall(ExecState* callerFrame);
    
    double optimizationThresholdScalingFactor();

#if ENABLE(JIT)
    ClosureCallStubRoutine* findClosureCallForReturnPC(ReturnAddressPtr);
#endif
        
    void updateAllPredictionsAndCountLiveness(unsigned& numberOfLiveNonArgumentValueProfiles, unsigned& numberOfSamplesInProfiles);

    void setConstantRegisters(const Vector<WriteBarrier<Unknown>>& constants)
    {
        size_t count = constants.size();
        m_constantRegisters.resize(count);
        for (size_t i = 0; i < count; i++)
            m_constantRegisters[i].set(*m_vm, ownerExecutable(), constants[i].get());
    }

    void dumpBytecode(
        PrintStream&, ExecState*, const Instruction* begin, const Instruction*&,
        const StubInfoMap& = StubInfoMap(), const CallLinkInfoMap& = CallLinkInfoMap());

    CString registerName(int r) const;
    void printUnaryOp(PrintStream&, ExecState*, int location, const Instruction*&, const char* op);
    void printBinaryOp(PrintStream&, ExecState*, int location, const Instruction*&, const char* op);
    void printConditionalJump(PrintStream&, ExecState*, const Instruction*, const Instruction*&, int location, const char* op);
    void printGetByIdOp(PrintStream&, ExecState*, int location, const Instruction*&);
    void printGetByIdCacheStatus(PrintStream&, ExecState*, int location, const StubInfoMap&);
    enum CacheDumpMode { DumpCaches, DontDumpCaches };
    void printCallOp(PrintStream&, ExecState*, int location, const Instruction*&, const char* op, CacheDumpMode, bool& hasPrintedProfiling, const CallLinkInfoMap&);
    void printPutByIdOp(PrintStream&, ExecState*, int location, const Instruction*&, const char* op);
    void printPutByIdCacheStatus(PrintStream&, ExecState*, int location, const StubInfoMap&);
    void printLocationAndOp(PrintStream&, ExecState*, int location, const Instruction*&, const char* op);
    void printLocationOpAndRegisterOperand(PrintStream&, ExecState*, int location, const Instruction*& it, const char* op, int operand);

    void beginDumpProfiling(PrintStream&, bool& hasPrintedProfiling);
    void dumpValueProfiling(PrintStream&, const Instruction*&, bool& hasPrintedProfiling);
    void dumpArrayProfiling(PrintStream&, const Instruction*&, bool& hasPrintedProfiling);
    void dumpRareCaseProfile(PrintStream&, const char* name, RareCaseProfile*, bool& hasPrintedProfiling);
        
    bool shouldImmediatelyAssumeLivenessDuringScan();
    
    void propagateTransitions(SlotVisitor&);
    void determineLiveness(SlotVisitor&);
        
    void stronglyVisitStrongReferences(SlotVisitor&);
    void stronglyVisitWeakReferences(SlotVisitor&);

    void createRareDataIfNecessary()
    {
        if (!m_rareData)
            m_rareData = adoptPtr(new RareData);
    }
    
#if ENABLE(JIT)
    void resetStubInternal(RepatchBuffer&, StructureStubInfo&);
    void resetStubDuringGCInternal(RepatchBuffer&, StructureStubInfo&);
#endif
    WriteBarrier<UnlinkedCodeBlock> m_unlinkedCode;
    int m_numParameters;
    union {
        unsigned m_debuggerRequests;
        struct {
            unsigned m_hasDebuggerStatement : 1;
            unsigned m_steppingMode : 1;
            unsigned m_numBreakpoints : 30;
        };
    };
    WriteBarrier<ScriptExecutable> m_ownerExecutable;
    VM* m_vm;

    RefCountedArray<Instruction> m_instructions;
    WriteBarrier<SymbolTable> m_symbolTable;
    VirtualRegister m_thisRegister;
    VirtualRegister m_argumentsRegister;
    VirtualRegister m_activationRegister;

    bool m_isStrictMode;
    bool m_needsActivation;
    bool m_mayBeExecuting;
    uint8_t m_visitAggregateHasBeenCalled;

    RefPtr<SourceProvider> m_source;
    unsigned m_sourceOffset;
    unsigned m_firstLineColumnOffset;
    unsigned m_codeType;

    Vector<LLIntCallLinkInfo> m_llintCallLinkInfos;
    SentinelLinkedList<LLIntCallLinkInfo, BasicRawSentinelNode<LLIntCallLinkInfo>> m_incomingLLIntCalls;
    RefPtr<JITCode> m_jitCode;
#if ENABLE(JIT)
    Bag<StructureStubInfo> m_stubInfos;
    Vector<ByValInfo> m_byValInfos;
    Bag<CallLinkInfo> m_callLinkInfos;
    SentinelLinkedList<CallLinkInfo, BasicRawSentinelNode<CallLinkInfo>> m_incomingCalls;
#endif
    OwnPtr<CompactJITCodeMap> m_jitCodeMap;
#if ENABLE(DFG_JIT)
    // This is relevant to non-DFG code blocks that serve as the profiled code block
    // for DFG code blocks.
    DFG::ExitProfile m_exitProfile;
    CompressedLazyOperandValueProfileHolder m_lazyOperandValueProfiles;
#endif
    Vector<ValueProfile> m_argumentValueProfiles;
    Vector<ValueProfile> m_valueProfiles;
    SegmentedVector<RareCaseProfile, 8> m_rareCaseProfiles;
    SegmentedVector<RareCaseProfile, 8> m_specialFastCaseProfiles;
    Vector<ArrayAllocationProfile> m_arrayAllocationProfiles;
    ArrayProfileVector m_arrayProfiles;
    Vector<ObjectAllocationProfile> m_objectAllocationProfiles;

    // Constant Pool
    COMPILE_ASSERT(sizeof(Register) == sizeof(WriteBarrier<Unknown>), Register_must_be_same_size_as_WriteBarrier_Unknown);
    // TODO: This could just be a pointer to m_unlinkedCodeBlock's data, but the DFG mutates
    // it, so we're stuck with it for now.
    Vector<WriteBarrier<Unknown>> m_constantRegisters;
    Vector<WriteBarrier<FunctionExecutable>> m_functionDecls;
    Vector<WriteBarrier<FunctionExecutable>> m_functionExprs;

    RefPtr<CodeBlock> m_alternative;
    
    BaselineExecutionCounter m_llintExecuteCounter;

    BaselineExecutionCounter m_jitExecuteCounter;
    int32_t m_totalJITExecutions;
    uint32_t m_osrExitCounter;
    uint16_t m_optimizationDelayCounter;
    uint16_t m_reoptimizationRetryCounter;
    
    mutable CodeBlockHash m_hash;

    std::unique_ptr<BytecodeLivenessAnalysis> m_livenessAnalysis;

    struct RareData {
        WTF_MAKE_FAST_ALLOCATED;
    public:
        Vector<HandlerInfo> m_exceptionHandlers;

        // Buffers used for large array literals
        Vector<Vector<JSValue>> m_constantBuffers;

        // Jump Tables
        Vector<SimpleJumpTable> m_switchJumpTables;
        Vector<StringJumpTable> m_stringSwitchJumpTables;

        EvalCodeCache m_evalCodeCache;
    };
#if COMPILER(MSVC)
    friend void WTF::deleteOwnedPtr<RareData>(RareData*);
#endif
    OwnPtr<RareData> m_rareData;
#if ENABLE(JIT)
    DFG::CapabilityLevel m_capabilityLevelState;
#endif
};

// Program code is not marked by any function, so we make the global object
// responsible for marking it.

class GlobalCodeBlock : public CodeBlock {
protected:
    GlobalCodeBlock(CopyParsedBlockTag, GlobalCodeBlock& other)
    : CodeBlock(CopyParsedBlock, other)
    {
    }
        
    GlobalCodeBlock(ScriptExecutable* ownerExecutable, UnlinkedCodeBlock* unlinkedCodeBlock, JSScope* scope, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, unsigned firstLineColumnOffset)
        : CodeBlock(ownerExecutable, unlinkedCodeBlock, scope, sourceProvider, sourceOffset, firstLineColumnOffset)
    {
    }
};

class ProgramCodeBlock : public GlobalCodeBlock {
public:
    ProgramCodeBlock(CopyParsedBlockTag, ProgramCodeBlock& other)
    : GlobalCodeBlock(CopyParsedBlock, other)
    {
    }

    ProgramCodeBlock(ProgramExecutable* ownerExecutable, UnlinkedProgramCodeBlock* unlinkedCodeBlock, JSScope* scope, PassRefPtr<SourceProvider> sourceProvider, unsigned firstLineColumnOffset)
        : GlobalCodeBlock(ownerExecutable, unlinkedCodeBlock, scope, sourceProvider, 0, firstLineColumnOffset)
    {
    }

#if ENABLE(JIT)
protected:
    virtual CodeBlock* replacement() override;
    virtual DFG::CapabilityLevel capabilityLevelInternal() override;
#endif
};

class EvalCodeBlock : public GlobalCodeBlock {
public:
    EvalCodeBlock(CopyParsedBlockTag, EvalCodeBlock& other)
    : GlobalCodeBlock(CopyParsedBlock, other)
    {
    }
        
    EvalCodeBlock(EvalExecutable* ownerExecutable, UnlinkedEvalCodeBlock* unlinkedCodeBlock, JSScope* scope, PassRefPtr<SourceProvider> sourceProvider)
        : GlobalCodeBlock(ownerExecutable, unlinkedCodeBlock, scope, sourceProvider, 0, 1)
    {
    }
    
    const Identifier& variable(unsigned index) { return unlinkedEvalCodeBlock()->variable(index); }
    unsigned numVariables() { return unlinkedEvalCodeBlock()->numVariables(); }
    
#if ENABLE(JIT)
protected:
    virtual CodeBlock* replacement() override;
    virtual DFG::CapabilityLevel capabilityLevelInternal() override;
#endif
    
private:
    UnlinkedEvalCodeBlock* unlinkedEvalCodeBlock() const { return jsCast<UnlinkedEvalCodeBlock*>(unlinkedCodeBlock()); }
};

class FunctionCodeBlock : public CodeBlock {
public:
    FunctionCodeBlock(CopyParsedBlockTag, FunctionCodeBlock& other)
    : CodeBlock(CopyParsedBlock, other)
    {
    }

    FunctionCodeBlock(FunctionExecutable* ownerExecutable, UnlinkedFunctionCodeBlock* unlinkedCodeBlock, JSScope* scope, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, unsigned firstLineColumnOffset)
        : CodeBlock(ownerExecutable, unlinkedCodeBlock, scope, sourceProvider, sourceOffset, firstLineColumnOffset)
    {
    }
    
#if ENABLE(JIT)
protected:
    virtual CodeBlock* replacement() override;
    virtual DFG::CapabilityLevel capabilityLevelInternal() override;
#endif
};

inline CodeBlock* baselineCodeBlockForInlineCallFrame(InlineCallFrame* inlineCallFrame)
{
    RELEASE_ASSERT(inlineCallFrame);
    ExecutableBase* executable = inlineCallFrame->executable.get();
    RELEASE_ASSERT(executable->structure()->classInfo() == FunctionExecutable::info());
    return static_cast<FunctionExecutable*>(executable)->baselineCodeBlockFor(inlineCallFrame->specializationKind());
}

inline CodeBlock* baselineCodeBlockForOriginAndBaselineCodeBlock(const CodeOrigin& codeOrigin, CodeBlock* baselineCodeBlock)
{
    if (codeOrigin.inlineCallFrame)
        return baselineCodeBlockForInlineCallFrame(codeOrigin.inlineCallFrame);
    return baselineCodeBlock;
}

inline int CodeBlock::argumentIndexAfterCapture(size_t argument)
{
    if (argument >= static_cast<size_t>(symbolTable()->parameterCount()))
        return CallFrame::argumentOffset(argument);
    
    const SlowArgument* slowArguments = symbolTable()->slowArguments();
    if (!slowArguments || slowArguments[argument].status == SlowArgument::Normal)
        return CallFrame::argumentOffset(argument);
    
    ASSERT(slowArguments[argument].status == SlowArgument::Captured);
    return slowArguments[argument].index;
}

inline bool CodeBlock::hasSlowArguments()
{
    return !!symbolTable()->slowArguments();
}

inline Register& ExecState::r(int index)
{
    CodeBlock* codeBlock = this->codeBlock();
    if (codeBlock->isConstantRegisterIndex(index))
        return *reinterpret_cast<Register*>(&codeBlock->constantRegister(index));
    return this[index];
}

inline Register& ExecState::uncheckedR(int index)
{
    RELEASE_ASSERT(index < FirstConstantRegisterIndex);
    return this[index];
}

inline JSValue ExecState::argumentAfterCapture(size_t argument)
{
    if (argument >= argumentCount())
        return jsUndefined();
    
    if (!codeBlock())
        return this[argumentOffset(argument)].jsValue();
    
    return this[codeBlock()->argumentIndexAfterCapture(argument)].jsValue();
}

inline void CodeBlockSet::mark(void* candidateCodeBlock)
{
    // We have to check for 0 and -1 because those are used by the HashMap as markers.
    uintptr_t value = reinterpret_cast<uintptr_t>(candidateCodeBlock);
    
    // This checks for both of those nasty cases in one go.
    // 0 + 1 = 1
    // -1 + 1 = 0
    if (value + 1 <= 1)
        return;

    CodeBlock* codeBlock = static_cast<CodeBlock*>(candidateCodeBlock); 
    if (!m_oldCodeBlocks.contains(codeBlock) && !m_newCodeBlocks.contains(codeBlock))
        return;

    mark(codeBlock);
}

inline void CodeBlockSet::mark(CodeBlock* codeBlock)
{
    if (!codeBlock)
        return;
    
    if (codeBlock->m_mayBeExecuting)
        return;
    
    codeBlock->m_mayBeExecuting = true;
    // We might not have cleared the marks for this CodeBlock, but we need to visit it.
    codeBlock->m_visitAggregateHasBeenCalled = false;
#if ENABLE(GGC)
    m_currentlyExecuting.append(codeBlock);
#endif
}

template <typename Functor> inline void ScriptExecutable::forEachCodeBlock(Functor&& functor)
{
    switch (type()) {
    case ProgramExecutableType: {
        if (CodeBlock* codeBlock = jsCast<ProgramExecutable*>(this)->m_programCodeBlock.get())
            codeBlock->forEachRelatedCodeBlock(std::forward<Functor>(functor));
        break;
    }
        
    case EvalExecutableType: {
        if (CodeBlock* codeBlock = jsCast<EvalExecutable*>(this)->m_evalCodeBlock.get())
            codeBlock->forEachRelatedCodeBlock(std::forward<Functor>(functor));
        break;
    }
        
    case FunctionExecutableType: {
        Functor f(std::forward<Functor>(functor));
        FunctionExecutable* executable = jsCast<FunctionExecutable*>(this);
        if (CodeBlock* codeBlock = executable->m_codeBlockForCall.get())
            codeBlock->forEachRelatedCodeBlock(f);
        if (CodeBlock* codeBlock = executable->m_codeBlockForConstruct.get())
            codeBlock->forEachRelatedCodeBlock(f);
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

} // namespace JSC

#endif // CodeBlock_h
