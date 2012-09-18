/*
 * Copyright (C) 2008, 2009, 2010, 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef CodeBlock_h
#define CodeBlock_h

#include "ArrayProfile.h"
#include "BytecodeConventions.h"
#include "CallLinkInfo.h"
#include "CallReturnOffsetToBytecodeOffset.h"
#include "CodeOrigin.h"
#include "CodeType.h"
#include "Comment.h"
#include "CompactJITCodeMap.h"
#include "DFGCodeBlocks.h"
#include "DFGCommon.h"
#include "DFGExitProfile.h"
#include "DFGMinifiedGraph.h"
#include "DFGOSREntry.h"
#include "DFGOSRExit.h"
#include "DFGVariableEventStream.h"
#include "EvalCodeCache.h"
#include "ExecutionCounter.h"
#include "ExpressionRangeInfo.h"
#include "GlobalResolveInfo.h"
#include "HandlerInfo.h"
#include "MethodCallLinkInfo.h"
#include "Options.h"
#include "Instruction.h"
#include "JITCode.h"
#include "JITWriteBarrier.h"
#include "JSGlobalObject.h"
#include "JumpReplacementWatchpoint.h"
#include "JumpTable.h"
#include "LLIntCallLinkInfo.h"
#include "LazyOperandValueProfile.h"
#include "LineInfo.h"
#include "Nodes.h"
#include "RegExpObject.h"
#include "StructureStubInfo.h"
#include "UnconditionalFinalizer.h"
#include "ValueProfile.h"
#include "Watchpoint.h"
#include <wtf/RefCountedArray.h>
#include <wtf/FastAllocBase.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/Platform.h>
#include <wtf/RefPtr.h>
#include <wtf/SegmentedVector.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

// Set ENABLE_BYTECODE_COMMENTS to 1 to enable recording bytecode generator
// comments for the bytecodes that it generates. This will allow
// CodeBlock::dump() to provide some contextual info about the bytecodes.
//
// The way this comment system works is as follows:
// 1. The BytecodeGenerator calls prependComment() with a constant comment
//    string in .text. The string must not be a stack or heap allocated
//    string.
// 2. When the BytecodeGenerator's emitOpcode() is called, the last
//    prepended comment will be recorded with the PC of the opcode being
//    emitted. This comment is being recorded in the CodeBlock's
//    m_bytecodeComments.
// 3. When CodeBlock::dump() is called, it will pair up the comments with
//    their corresponding bytecodes based on the bytecode and comment's
//    PC. If a matching pair is found, the comment will be printed after
//    the bytecode. If not, no comment is printed.
//
// NOTE: Enabling this will consume additional memory at runtime to store
// the comments. Since these comments are only useful for VM debugging
// (as opposed to app debugging), this feature is to be disabled by default,
// and can be enabled as needed for VM development use only.

#define ENABLE_BYTECODE_COMMENTS 0

namespace JSC {

    class DFGCodeBlocks;
    class ExecState;
    class LLIntOffsetsExtractor;
    class RepatchBuffer;

    inline int unmodifiedArgumentsRegister(int argumentsRegister) { return argumentsRegister - 1; }

    static ALWAYS_INLINE int missingThisObjectMarker() { return std::numeric_limits<int>::max(); }

    class CodeBlock : public UnconditionalFinalizer, public WeakReferenceHarvester {
        WTF_MAKE_FAST_ALLOCATED;
        friend class JIT;
        friend class LLIntOffsetsExtractor;
    public:
        enum CopyParsedBlockTag { CopyParsedBlock };
    protected:
        CodeBlock(CopyParsedBlockTag, CodeBlock& other);
        
        CodeBlock(ScriptExecutable* ownerExecutable, CodeType, JSGlobalObject*, PassRefPtr<SourceProvider>, unsigned sourceOffset, bool isConstructor, PassOwnPtr<CodeBlock> alternative);

        WriteBarrier<JSGlobalObject> m_globalObject;
        Heap* m_heap;

    public:
        JS_EXPORT_PRIVATE virtual ~CodeBlock();
        
        int numParameters() const { return m_numParameters; }
        void setNumParameters(int newValue);
        void addParameter();
        
        int* addressOfNumParameters() { return &m_numParameters; }
        static ptrdiff_t offsetOfNumParameters() { return OBJECT_OFFSETOF(CodeBlock, m_numParameters); }

        CodeBlock* alternative() { return m_alternative.get(); }
        PassOwnPtr<CodeBlock> releaseAlternative() { return m_alternative.release(); }
        void setAlternative(PassOwnPtr<CodeBlock> alternative) { m_alternative = alternative; }
        
        CodeSpecializationKind specializationKind()
        {
            if (m_isConstructor)
                return CodeForConstruct;
            return CodeForCall;
        }
        
#if ENABLE(JIT)
        CodeBlock* baselineVersion()
        {
            CodeBlock* result = replacement();
            if (!result)
                return 0; // This can happen if we're in the process of creating the baseline version.
            while (result->alternative())
                result = result->alternative();
            ASSERT(result);
            ASSERT(JITCode::isBaselineCode(result->getJITType()));
            return result;
        }
#endif
        
        void visitAggregate(SlotVisitor&);

        static void dumpStatistics();

        void dump(ExecState*);
        void printStructures(const Instruction*);
        void printStructure(const char* name, const Instruction*, int operand);

        bool isStrictMode() const { return m_isStrictMode; }

        inline bool isKnownNotImmediate(int index)
        {
            if (index == m_thisRegister && !m_isStrictMode)
                return true;

            if (isConstantRegisterIndex(index))
                return getConstant(index).isCell();

            return false;
        }

        ALWAYS_INLINE bool isTemporaryRegisterIndex(int index)
        {
            return index >= m_numVars;
        }

        void dumpBytecodeCommentAndNewLine(int location);
#if ENABLE(BYTECODE_COMMENTS)
        const char* commentForBytecodeOffset(unsigned bytecodeOffset);
        void dumpBytecodeComments();
#endif

        HandlerInfo* handlerForBytecodeOffset(unsigned bytecodeOffset);
        int lineNumberForBytecodeOffset(unsigned bytecodeOffset);
        void expressionRangeForBytecodeOffset(unsigned bytecodeOffset, int& divot, int& startOffset, int& endOffset);

#if ENABLE(JIT)

        StructureStubInfo& getStubInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<StructureStubInfo, void*, getStructureStubInfoReturnLocation>(m_structureStubInfos.begin(), m_structureStubInfos.size(), returnAddress.value()));
        }

        StructureStubInfo& getStubInfo(unsigned bytecodeIndex)
        {
            return *(binarySearch<StructureStubInfo, unsigned, getStructureStubInfoBytecodeIndex>(m_structureStubInfos.begin(), m_structureStubInfos.size(), bytecodeIndex));
        }
        
        void resetStub(StructureStubInfo&);

        CallLinkInfo& getCallLinkInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<CallLinkInfo, void*, getCallLinkInfoReturnLocation>(m_callLinkInfos.begin(), m_callLinkInfos.size(), returnAddress.value()));
        }
        
        CallLinkInfo& getCallLinkInfo(unsigned bytecodeIndex)
        {
            return *(binarySearch<CallLinkInfo, unsigned, getCallLinkInfoBytecodeIndex>(m_callLinkInfos.begin(), m_callLinkInfos.size(), bytecodeIndex));
        }

        MethodCallLinkInfo& getMethodCallLinkInfo(ReturnAddressPtr returnAddress)
        {
            return *(binarySearch<MethodCallLinkInfo, void*, getMethodCallLinkInfoReturnLocation>(m_methodCallLinkInfos.begin(), m_methodCallLinkInfos.size(), returnAddress.value()));
        }

        MethodCallLinkInfo& getMethodCallLinkInfo(unsigned bytecodeIndex)
        {
            return *(binarySearch<MethodCallLinkInfo, unsigned, getMethodCallLinkInfoBytecodeIndex>(m_methodCallLinkInfos.begin(), m_methodCallLinkInfos.size(), bytecodeIndex));
        }
#endif // ENABLE(JIT)

#if ENABLE(LLINT)
        Instruction* adjustPCIfAtCallSite(Instruction*);
#endif
        unsigned bytecodeOffset(ExecState*, ReturnAddressPtr);

#if ENABLE(JIT)
        unsigned bytecodeOffsetForCallAtIndex(unsigned index)
        {
            if (!m_rareData)
                return 1;
            Vector<CallReturnOffsetToBytecodeOffset>& callIndices = m_rareData->m_callReturnIndexVector;
            if (!callIndices.size())
                return 1;
            ASSERT(index < m_rareData->m_callReturnIndexVector.size());
            return m_rareData->m_callReturnIndexVector[index].bytecodeOffset;
        }

        void unlinkCalls();
        
        bool hasIncomingCalls() { return m_incomingCalls.begin() != m_incomingCalls.end(); }
        
        void linkIncomingCall(CallLinkInfo* incoming)
        {
            m_incomingCalls.push(incoming);
        }
#endif // ENABLE(JIT)

#if ENABLE(LLINT)
        void linkIncomingCall(LLIntCallLinkInfo* incoming)
        {
            m_incomingLLIntCalls.push(incoming);
        }
#endif // ENABLE(LLINT)
        
        void unlinkIncomingCalls();

#if ENABLE(DFG_JIT) || ENABLE(LLINT)
        void setJITCodeMap(PassOwnPtr<CompactJITCodeMap> jitCodeMap)
        {
            m_jitCodeMap = jitCodeMap;
        }
        CompactJITCodeMap* jitCodeMap()
        {
            return m_jitCodeMap.get();
        }
#endif
        
#if ENABLE(DFG_JIT)
        void createDFGDataIfNecessary()
        {
            if (!!m_dfgData)
                return;
            
            m_dfgData = adoptPtr(new DFGData);
        }
        
        DFG::OSREntryData* appendDFGOSREntryData(unsigned bytecodeIndex, unsigned machineCodeOffset)
        {
            createDFGDataIfNecessary();
            DFG::OSREntryData entry;
            entry.m_bytecodeIndex = bytecodeIndex;
            entry.m_machineCodeOffset = machineCodeOffset;
            m_dfgData->osrEntry.append(entry);
            return &m_dfgData->osrEntry.last();
        }
        unsigned numberOfDFGOSREntries() const
        {
            if (!m_dfgData)
                return 0;
            return m_dfgData->osrEntry.size();
        }
        DFG::OSREntryData* dfgOSREntryData(unsigned i) { return &m_dfgData->osrEntry[i]; }
        DFG::OSREntryData* dfgOSREntryDataForBytecodeIndex(unsigned bytecodeIndex)
        {
            if (!m_dfgData)
                return 0;
            if (m_dfgData->osrEntry.isEmpty())
                return 0;
            DFG::OSREntryData* result = binarySearch<
                DFG::OSREntryData, unsigned, DFG::getOSREntryDataBytecodeIndex>(
                    m_dfgData->osrEntry.begin(), m_dfgData->osrEntry.size(),
                    bytecodeIndex, WTF::KeyMustNotBePresentInArray);
            if (result->m_bytecodeIndex != bytecodeIndex)
                return 0;
            return result;
        }
        
        unsigned appendOSRExit(const DFG::OSRExit& osrExit)
        {
            createDFGDataIfNecessary();
            unsigned result = m_dfgData->osrExit.size();
            m_dfgData->osrExit.append(osrExit);
            return result;
        }
        
        DFG::OSRExit& lastOSRExit()
        {
            return m_dfgData->osrExit.last();
        }
        
        unsigned appendSpeculationRecovery(const DFG::SpeculationRecovery& recovery)
        {
            createDFGDataIfNecessary();
            unsigned result = m_dfgData->speculationRecovery.size();
            m_dfgData->speculationRecovery.append(recovery);
            return result;
        }
        
        unsigned appendWatchpoint(const JumpReplacementWatchpoint& watchpoint)
        {
            createDFGDataIfNecessary();
            unsigned result = m_dfgData->watchpoints.size();
            m_dfgData->watchpoints.append(watchpoint);
            return result;
        }
        
        unsigned numberOfOSRExits()
        {
            if (!m_dfgData)
                return 0;
            return m_dfgData->osrExit.size();
        }
        
        unsigned numberOfSpeculationRecoveries()
        {
            if (!m_dfgData)
                return 0;
            return m_dfgData->speculationRecovery.size();
        }
        
        unsigned numberOfWatchpoints()
        {
            if (!m_dfgData)
                return 0;
            return m_dfgData->watchpoints.size();
        }
        
        DFG::OSRExit& osrExit(unsigned index)
        {
            return m_dfgData->osrExit[index];
        }
        
        DFG::SpeculationRecovery& speculationRecovery(unsigned index)
        {
            return m_dfgData->speculationRecovery[index];
        }
        
        JumpReplacementWatchpoint& watchpoint(unsigned index)
        {
            return m_dfgData->watchpoints[index];
        }
        
        void appendWeakReference(JSCell* target)
        {
            createDFGDataIfNecessary();
            m_dfgData->weakReferences.append(WriteBarrier<JSCell>(*globalData(), ownerExecutable(), target));
        }
        
        void appendWeakReferenceTransition(JSCell* codeOrigin, JSCell* from, JSCell* to)
        {
            createDFGDataIfNecessary();
            m_dfgData->transitions.append(
                WeakReferenceTransition(*globalData(), ownerExecutable(), codeOrigin, from, to));
        }
        
        DFG::MinifiedGraph& minifiedDFG()
        {
            createDFGDataIfNecessary();
            return m_dfgData->minifiedDFG;
        }
        
        DFG::VariableEventStream& variableEventStream()
        {
            createDFGDataIfNecessary();
            return m_dfgData->variableEventStream;
        }
#endif

        unsigned bytecodeOffset(Instruction* returnAddress)
        {
            ASSERT(returnAddress >= instructions().begin() && returnAddress < instructions().end());
            return static_cast<Instruction*>(returnAddress) - instructions().begin();
        }

        void setIsNumericCompareFunction(bool isNumericCompareFunction) { m_isNumericCompareFunction = isNumericCompareFunction; }
        bool isNumericCompareFunction() { return m_isNumericCompareFunction; }

        unsigned numberOfInstructions() const { return m_instructions.size(); }
        RefCountedArray<Instruction>& instructions() { return m_instructions; }
        const RefCountedArray<Instruction>& instructions() const { return m_instructions; }
        
#if ENABLE(BYTECODE_COMMENTS)
        Vector<Comment>& bytecodeComments() { return m_bytecodeComments; }
#endif

        size_t predictedMachineCodeSize();
        
        bool usesOpcode(OpcodeID);

        unsigned instructionCount() { return m_instructions.size(); }

#if ENABLE(JIT)
        void setJITCode(const JITCode& code, MacroAssemblerCodePtr codeWithArityCheck)
        {
            m_jitCode = code;
            m_jitCodeWithArityCheck = codeWithArityCheck;
#if ENABLE(DFG_JIT)
            if (m_jitCode.jitType() == JITCode::DFGJIT) {
                createDFGDataIfNecessary();
                m_globalData->heap.m_dfgCodeBlocks.m_set.add(this);
            }
#endif
        }
        JITCode& getJITCode() { return m_jitCode; }
        MacroAssemblerCodePtr getJITCodeWithArityCheck() { return m_jitCodeWithArityCheck; }
        JITCode::JITType getJITType() { return m_jitCode.jitType(); }
        ExecutableMemoryHandle* executableMemory() { return getJITCode().getExecutableMemory(); }
        virtual JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex) = 0;
        virtual void jettison() = 0;
        enum JITCompilationResult { AlreadyCompiled, CouldNotCompile, CompiledSuccessfully };
        JITCompilationResult jitCompile(ExecState* exec)
        {
            if (getJITType() != JITCode::InterpreterThunk) {
                ASSERT(getJITType() == JITCode::BaselineJIT);
                return AlreadyCompiled;
            }
#if ENABLE(JIT)
            if (jitCompileImpl(exec))
                return CompiledSuccessfully;
            return CouldNotCompile;
#else
            UNUSED_PARAM(exec);
            return CouldNotCompile;
#endif
        }
        virtual CodeBlock* replacement() = 0;

        virtual DFG::CapabilityLevel canCompileWithDFGInternal() = 0;
        DFG::CapabilityLevel canCompileWithDFG()
        {
            DFG::CapabilityLevel result = canCompileWithDFGInternal();
            m_canCompileWithDFGState = result;
            return result;
        }
        DFG::CapabilityLevel canCompileWithDFGState() { return m_canCompileWithDFGState; }

        bool hasOptimizedReplacement()
        {
            ASSERT(JITCode::isBaselineCode(getJITType()));
            bool result = replacement()->getJITType() > getJITType();
#if !ASSERT_DISABLED
            if (result)
                ASSERT(replacement()->getJITType() == JITCode::DFGJIT);
            else {
                ASSERT(JITCode::isBaselineCode(replacement()->getJITType()));
                ASSERT(replacement() == this);
            }
#endif
            return result;
        }
#else
        JITCode::JITType getJITType() { return JITCode::BaselineJIT; }
#endif

        ScriptExecutable* ownerExecutable() const { return m_ownerExecutable.get(); }

        void setGlobalData(JSGlobalData* globalData) { m_globalData = globalData; }
        JSGlobalData* globalData() { return m_globalData; }

        void setThisRegister(int thisRegister) { m_thisRegister = thisRegister; }
        int thisRegister() const { return m_thisRegister; }

        void setNeedsFullScopeChain(bool needsFullScopeChain) { m_needsFullScopeChain = needsFullScopeChain; }
        bool needsFullScopeChain() const { return m_needsFullScopeChain; }
        void setUsesEval(bool usesEval) { m_usesEval = usesEval; }
        bool usesEval() const { return m_usesEval; }
        
        void setArgumentsRegister(int argumentsRegister)
        {
            ASSERT(argumentsRegister != -1);
            m_argumentsRegister = argumentsRegister;
            ASSERT(usesArguments());
        }
        int argumentsRegister()
        {
            ASSERT(usesArguments());
            return m_argumentsRegister;
        }
        int uncheckedArgumentsRegister()
        {
            if (!usesArguments())
                return InvalidVirtualRegister;
            return argumentsRegister();
        }
        void setActivationRegister(int activationRegister)
        {
            m_activationRegister = activationRegister;
        }
        int activationRegister()
        {
            ASSERT(needsFullScopeChain());
            return m_activationRegister;
        }
        int uncheckedActivationRegister()
        {
            if (!needsFullScopeChain())
                return InvalidVirtualRegister;
            return activationRegister();
        }
        bool usesArguments() const { return m_argumentsRegister != -1; }
        
        bool needsActivation() const
        {
            return needsFullScopeChain() && codeType() != GlobalCode;
        }
        
        bool isCaptured(int operand, InlineCallFrame* inlineCallFrame = 0) const
        {
            if (inlineCallFrame && !operandIsArgument(operand))
                return inlineCallFrame->capturedVars.get(operand);

            // Our estimate of argument capture is conservative.
            if (operandIsArgument(operand))
                return needsActivation() || usesArguments();

            return operand < m_numCapturedVars;
        }

        CodeType codeType() const { return m_codeType; }

        SourceProvider* source() const { return m_source.get(); }
        unsigned sourceOffset() const { return m_sourceOffset; }

        size_t numberOfJumpTargets() const { return m_jumpTargets.size(); }
        void addJumpTarget(unsigned jumpTarget) { m_jumpTargets.append(jumpTarget); }
        unsigned jumpTarget(int index) const { return m_jumpTargets[index]; }
        unsigned lastJumpTarget() const { return m_jumpTargets.last(); }

        void createActivation(CallFrame*);

        void clearEvalCache();
        
        String nameForRegister(int registerNumber);
        
        void addPropertyAccessInstruction(unsigned propertyAccessInstruction)
        {
            m_propertyAccessInstructions.append(propertyAccessInstruction);
        }
        void addGlobalResolveInstruction(unsigned globalResolveInstruction)
        {
            m_globalResolveInstructions.append(globalResolveInstruction);
        }
        bool hasGlobalResolveInstructionAtBytecodeOffset(unsigned bytecodeOffset);
#if ENABLE(LLINT)
        LLIntCallLinkInfo* addLLIntCallLinkInfo()
        {
            m_llintCallLinkInfos.append(LLIntCallLinkInfo());
            return &m_llintCallLinkInfos.last();
        }
#endif
#if ENABLE(JIT)
        void setNumberOfStructureStubInfos(size_t size) { m_structureStubInfos.grow(size); }
        size_t numberOfStructureStubInfos() const { return m_structureStubInfos.size(); }
        StructureStubInfo& structureStubInfo(int index) { return m_structureStubInfos[index]; }

        void addGlobalResolveInfo(unsigned globalResolveInstruction)
        {
            m_globalResolveInfos.append(GlobalResolveInfo(globalResolveInstruction));
        }
        GlobalResolveInfo& globalResolveInfo(int index) { return m_globalResolveInfos[index]; }
        bool hasGlobalResolveInfoAtBytecodeOffset(unsigned bytecodeOffset);
        GlobalResolveInfo& globalResolveInfoForBytecodeOffset(unsigned bytecodeOffset);
        unsigned numberOfGlobalResolveInfos() { return m_globalResolveInfos.size(); }

        void setNumberOfCallLinkInfos(size_t size) { m_callLinkInfos.grow(size); }
        size_t numberOfCallLinkInfos() const { return m_callLinkInfos.size(); }
        CallLinkInfo& callLinkInfo(int index) { return m_callLinkInfos[index]; }

        void addMethodCallLinkInfos(unsigned n) { ASSERT(m_globalData->canUseJIT()); m_methodCallLinkInfos.grow(n); }
        MethodCallLinkInfo& methodCallLinkInfo(int index) { return m_methodCallLinkInfos[index]; }
        size_t numberOfMethodCallLinkInfos() { return m_methodCallLinkInfos.size(); }
#endif
        
#if ENABLE(VALUE_PROFILER)
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
        
        ValueProfile* addValueProfile(int bytecodeOffset)
        {
            ASSERT(bytecodeOffset != -1);
            ASSERT(m_valueProfiles.isEmpty() || m_valueProfiles.last().m_bytecodeOffset < bytecodeOffset);
            m_valueProfiles.append(ValueProfile(bytecodeOffset));
            return &m_valueProfiles.last();
        }
        unsigned numberOfValueProfiles() { return m_valueProfiles.size(); }
        ValueProfile* valueProfile(int index)
        {
            ValueProfile* result = &m_valueProfiles[index];
            ASSERT(result->m_bytecodeOffset != -1);
            return result;
        }
        ValueProfile* valueProfileForBytecodeOffset(int bytecodeOffset)
        {
            ValueProfile* result = WTF::genericBinarySearch<ValueProfile, int, getValueProfileBytecodeOffset>(m_valueProfiles, m_valueProfiles.size(), bytecodeOffset);
            ASSERT(result->m_bytecodeOffset != -1);
            ASSERT(instructions()[bytecodeOffset + opcodeLength(
                       m_globalData->interpreter->getOpcodeID(
                           instructions()[
                               bytecodeOffset].u.opcode)) - 1].u.profile == result);
            return result;
        }
        SpeculatedType valueProfilePredictionForBytecodeOffset(int bytecodeOffset)
        {
            return valueProfileForBytecodeOffset(bytecodeOffset)->computeUpdatedPrediction();
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
        RareCaseProfile* rareCaseProfileForBytecodeOffset(int bytecodeOffset)
        {
            return WTF::genericBinarySearch<RareCaseProfile, int, getRareCaseProfileBytecodeOffset>(m_rareCaseProfiles, m_rareCaseProfiles.size(), bytecodeOffset);
        }
        
        bool likelyToTakeSlowCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned value = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            return value >= Options::likelyToTakeSlowCaseMinimumCount() && static_cast<double>(value) / m_executionEntryCount >= Options::likelyToTakeSlowCaseThreshold();
        }
        
        bool couldTakeSlowCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned value = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            return value >= Options::couldTakeSlowCaseMinimumCount() && static_cast<double>(value) / m_executionEntryCount >= Options::couldTakeSlowCaseThreshold();
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
            return WTF::genericBinarySearch<RareCaseProfile, int, getRareCaseProfileBytecodeOffset>(m_specialFastCaseProfiles, m_specialFastCaseProfiles.size(), bytecodeOffset);
        }
        
        bool likelyToTakeSpecialFastCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            return specialFastCaseCount >= Options::likelyToTakeSlowCaseMinimumCount() && static_cast<double>(specialFastCaseCount) / m_executionEntryCount >= Options::likelyToTakeSlowCaseThreshold();
        }
        
        bool couldTakeSpecialFastCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            return specialFastCaseCount >= Options::couldTakeSlowCaseMinimumCount() && static_cast<double>(specialFastCaseCount) / m_executionEntryCount >= Options::couldTakeSlowCaseThreshold();
        }
        
        bool likelyToTakeDeepestSlowCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned slowCaseCount = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            unsigned value = slowCaseCount - specialFastCaseCount;
            return value >= Options::likelyToTakeSlowCaseMinimumCount() && static_cast<double>(value) / m_executionEntryCount >= Options::likelyToTakeSlowCaseThreshold();
        }
        
        bool likelyToTakeAnySlowCase(int bytecodeOffset)
        {
            if (!numberOfRareCaseProfiles())
                return false;
            unsigned slowCaseCount = rareCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            unsigned specialFastCaseCount = specialFastCaseProfileForBytecodeOffset(bytecodeOffset)->m_counter;
            unsigned value = slowCaseCount + specialFastCaseCount;
            return value >= Options::likelyToTakeSlowCaseMinimumCount() && static_cast<double>(value) / m_executionEntryCount >= Options::likelyToTakeSlowCaseThreshold();
        }
        
        unsigned executionEntryCount() const { return m_executionEntryCount; }

        unsigned numberOfArrayProfiles() const { return m_arrayProfiles.size(); }
        const ArrayProfileVector& arrayProfiles() { return m_arrayProfiles; }
        ArrayProfile* addArrayProfile(unsigned bytecodeOffset)
        {
            m_arrayProfiles.append(ArrayProfile(bytecodeOffset));
            return &m_arrayProfiles.last();
        }
        ArrayProfile* getArrayProfile(unsigned bytecodeOffset);
        ArrayProfile* getOrAddArrayProfile(unsigned bytecodeOffset);
#endif
        
        unsigned globalResolveInfoCount() const
        {
#if ENABLE(JIT)    
            if (m_globalData->canUseJIT())
                return m_globalResolveInfos.size();
#endif
            return 0;
        }

        // Exception handling support

        size_t numberOfExceptionHandlers() const { return m_rareData ? m_rareData->m_exceptionHandlers.size() : 0; }
        void addExceptionHandler(const HandlerInfo& hanler) { createRareDataIfNecessary(); return m_rareData->m_exceptionHandlers.append(hanler); }
        HandlerInfo& exceptionHandler(int index) { ASSERT(m_rareData); return m_rareData->m_exceptionHandlers[index]; }

        void addExpressionInfo(const ExpressionRangeInfo& expressionInfo)
        {
            createRareDataIfNecessary();
            m_rareData->m_expressionInfo.append(expressionInfo);
        }

        void addLineInfo(unsigned bytecodeOffset, int lineNo)
        {
            Vector<LineInfo>& lineInfo = m_lineInfo;
            if (!lineInfo.size() || lineInfo.last().lineNumber != lineNo) {
                LineInfo info = { bytecodeOffset, lineNo };
                lineInfo.append(info);
            }
        }

        bool hasExpressionInfo() { return m_rareData && m_rareData->m_expressionInfo.size(); }

#if ENABLE(JIT)
        Vector<CallReturnOffsetToBytecodeOffset>& callReturnIndexVector()
        {
            createRareDataIfNecessary();
            return m_rareData->m_callReturnIndexVector;
        }
#endif

#if ENABLE(DFG_JIT)
        SegmentedVector<InlineCallFrame, 4>& inlineCallFrames()
        {
            createRareDataIfNecessary();
            return m_rareData->m_inlineCallFrames;
        }
        
        Vector<CodeOriginAtCallReturnOffset>& codeOrigins()
        {
            createRareDataIfNecessary();
            return m_rareData->m_codeOrigins;
        }
        
        // Having code origins implies that there has been some inlining.
        bool hasCodeOrigins()
        {
            return m_rareData && !!m_rareData->m_codeOrigins.size();
        }
        
        bool codeOriginForReturn(ReturnAddressPtr returnAddress, CodeOrigin& codeOrigin)
        {
            if (!hasCodeOrigins())
                return false;
            unsigned offset = getJITCode().offsetOf(returnAddress.value());
            CodeOriginAtCallReturnOffset* entry = binarySearch<CodeOriginAtCallReturnOffset, unsigned, getCallReturnOffsetForCodeOrigin>(codeOrigins().begin(), codeOrigins().size(), offset, WTF::KeyMustNotBePresentInArray);
            if (entry->callReturnOffset != offset)
                return false;
            codeOrigin = entry->codeOrigin;
            return true;
        }
        
        CodeOrigin codeOrigin(unsigned index)
        {
            ASSERT(m_rareData);
            return m_rareData->m_codeOrigins[index].codeOrigin;
        }
        
        bool addFrequentExitSite(const DFG::FrequentExitSite& site)
        {
            ASSERT(JITCode::isBaselineCode(getJITType()));
            return m_exitProfile.add(site);
        }

        DFG::ExitProfile& exitProfile() { return m_exitProfile; }
        
        CompressedLazyOperandValueProfileHolder& lazyOperandValueProfiles()
        {
            return m_lazyOperandValueProfiles;
        }
#endif

        // Constant Pool

        size_t numberOfIdentifiers() const { return m_identifiers.size(); }
        void addIdentifier(const Identifier& i) { return m_identifiers.append(i); }
        Identifier& identifier(int index) { return m_identifiers[index]; }

        size_t numberOfConstantRegisters() const { return m_constantRegisters.size(); }
        unsigned addConstant(JSValue v)
        {
            unsigned result = m_constantRegisters.size();
            m_constantRegisters.append(WriteBarrier<Unknown>());
            m_constantRegisters.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), v);
            return result;
        }
        unsigned addOrFindConstant(JSValue);
        WriteBarrier<Unknown>& constantRegister(int index) { return m_constantRegisters[index - FirstConstantRegisterIndex]; }
        ALWAYS_INLINE bool isConstantRegisterIndex(int index) const { return index >= FirstConstantRegisterIndex; }
        ALWAYS_INLINE JSValue getConstant(int index) const { return m_constantRegisters[index - FirstConstantRegisterIndex].get(); }

        unsigned addFunctionDecl(FunctionExecutable* n)
        {
            unsigned size = m_functionDecls.size();
            m_functionDecls.append(WriteBarrier<FunctionExecutable>());
            m_functionDecls.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), n);
            return size;
        }
        FunctionExecutable* functionDecl(int index) { return m_functionDecls[index].get(); }
        int numberOfFunctionDecls() { return m_functionDecls.size(); }
        unsigned addFunctionExpr(FunctionExecutable* n)
        {
            unsigned size = m_functionExprs.size();
            m_functionExprs.append(WriteBarrier<FunctionExecutable>());
            m_functionExprs.last().set(m_globalObject->globalData(), m_ownerExecutable.get(), n);
            return size;
        }
        FunctionExecutable* functionExpr(int index) { return m_functionExprs[index].get(); }

        unsigned addRegExp(RegExp* r)
        {
            createRareDataIfNecessary();
            unsigned size = m_rareData->m_regexps.size();
            m_rareData->m_regexps.append(WriteBarrier<RegExp>(*m_globalData, ownerExecutable(), r));
            return size;
        }
        unsigned numberOfRegExps() const
        {
            if (!m_rareData)
                return 0;
            return m_rareData->m_regexps.size();
        }
        RegExp* regexp(int index) const { ASSERT(m_rareData); return m_rareData->m_regexps[index].get(); }

        unsigned addConstantBuffer(unsigned length)
        {
            createRareDataIfNecessary();
            unsigned size = m_rareData->m_constantBuffers.size();
            m_rareData->m_constantBuffers.append(Vector<JSValue>(length));
            return size;
        }

        JSValue* constantBuffer(unsigned index)
        {
            ASSERT(m_rareData);
            return m_rareData->m_constantBuffers[index].data();
        }

        JSGlobalObject* globalObject() { return m_globalObject.get(); }
        
        JSGlobalObject* globalObjectFor(CodeOrigin codeOrigin)
        {
            if (!codeOrigin.inlineCallFrame)
                return globalObject();
            // FIXME: if we ever inline based on executable not function, this code will need to change.
            return codeOrigin.inlineCallFrame->callee->scope()->globalObject();
        }

        // Jump Tables

        size_t numberOfImmediateSwitchJumpTables() const { return m_rareData ? m_rareData->m_immediateSwitchJumpTables.size() : 0; }
        SimpleJumpTable& addImmediateSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_immediateSwitchJumpTables.append(SimpleJumpTable()); return m_rareData->m_immediateSwitchJumpTables.last(); }
        SimpleJumpTable& immediateSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_immediateSwitchJumpTables[tableIndex]; }

        size_t numberOfCharacterSwitchJumpTables() const { return m_rareData ? m_rareData->m_characterSwitchJumpTables.size() : 0; }
        SimpleJumpTable& addCharacterSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_characterSwitchJumpTables.append(SimpleJumpTable()); return m_rareData->m_characterSwitchJumpTables.last(); }
        SimpleJumpTable& characterSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_characterSwitchJumpTables[tableIndex]; }

        size_t numberOfStringSwitchJumpTables() const { return m_rareData ? m_rareData->m_stringSwitchJumpTables.size() : 0; }
        StringJumpTable& addStringSwitchJumpTable() { createRareDataIfNecessary(); m_rareData->m_stringSwitchJumpTables.append(StringJumpTable()); return m_rareData->m_stringSwitchJumpTables.last(); }
        StringJumpTable& stringSwitchJumpTable(int tableIndex) { ASSERT(m_rareData); return m_rareData->m_stringSwitchJumpTables[tableIndex]; }


        SharedSymbolTable* symbolTable() { return m_symbolTable.get(); }

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
        
        void copyPostParseDataFrom(CodeBlock* alternative);
        void copyPostParseDataFromAlternative();
        
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
        
        const ExecutionCounter& llintExecuteCounter() const
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
        unsigned reoptimizationRetryCounter() const
        {
            ASSERT(m_reoptimizationRetryCounter <= Options::reoptimizationRetryCounterMax());
            return m_reoptimizationRetryCounter;
        }
        
        void countReoptimization()
        {
            m_reoptimizationRetryCounter++;
            if (m_reoptimizationRetryCounter > Options::reoptimizationRetryCounterMax())
                m_reoptimizationRetryCounter = Options::reoptimizationRetryCounterMax();
        }
        
        int32_t counterValueForOptimizeAfterWarmUp()
        {
            return Options::thresholdForOptimizeAfterWarmUp() << reoptimizationRetryCounter();
        }
        
        int32_t counterValueForOptimizeAfterLongWarmUp()
        {
            return Options::thresholdForOptimizeAfterLongWarmUp() << reoptimizationRetryCounter();
        }
        
        int32_t* addressOfJITExecuteCounter()
        {
            return &m_jitExecuteCounter.m_counter;
        }
        
        static ptrdiff_t offsetOfJITExecuteCounter() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(ExecutionCounter, m_counter); }
        static ptrdiff_t offsetOfJITExecutionActiveThreshold() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(ExecutionCounter, m_activeThreshold); }
        static ptrdiff_t offsetOfJITExecutionTotalCount() { return OBJECT_OFFSETOF(CodeBlock, m_jitExecuteCounter) + OBJECT_OFFSETOF(ExecutionCounter, m_totalCount); }

        const ExecutionCounter& jitExecuteCounter() const { return m_jitExecuteCounter; }
        
        unsigned optimizationDelayCounter() const { return m_optimizationDelayCounter; }
        
        // Check if the optimization threshold has been reached, and if not,
        // adjust the heuristics accordingly. Returns true if the threshold has
        // been reached.
        bool checkIfOptimizationThresholdReached()
        {
            return m_jitExecuteCounter.checkIfThresholdCrossedAndSet(this);
        }
        
        // Call this to force the next optimization trigger to fire. This is
        // rarely wise, since optimization triggers are typically more
        // expensive than executing baseline code.
        void optimizeNextInvocation()
        {
            m_jitExecuteCounter.setNewThreshold(0, this);
        }
        
        // Call this to prevent optimization from happening again. Note that
        // optimization will still happen after roughly 2^29 invocations,
        // so this is really meant to delay that as much as possible. This
        // is called if optimization failed, and we expect it to fail in
        // the future as well.
        void dontOptimizeAnytimeSoon()
        {
            m_jitExecuteCounter.deferIndefinitely();
        }
        
        // Call this to reinitialize the counter to its starting state,
        // forcing a warm-up to happen before the next optimization trigger
        // fires. This is called in the CodeBlock constructor. It also
        // makes sense to call this if an OSR exit occurred. Note that
        // OSR exit code is code generated, so the value of the execute
        // counter that this corresponds to is also available directly.
        void optimizeAfterWarmUp()
        {
            m_jitExecuteCounter.setNewThreshold(counterValueForOptimizeAfterWarmUp(), this);
        }
        
        // Call this to force an optimization trigger to fire only after
        // a lot of warm-up.
        void optimizeAfterLongWarmUp()
        {
            m_jitExecuteCounter.setNewThreshold(counterValueForOptimizeAfterLongWarmUp(), this);
        }
        
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
        void optimizeSoon()
        {
            m_jitExecuteCounter.setNewThreshold(Options::thresholdForOptimizeSoon() << reoptimizationRetryCounter(), this);
        }
        
        uint32_t osrExitCounter() const { return m_osrExitCounter; }
        
        void countOSRExit() { m_osrExitCounter++; }
        
        uint32_t* addressOfOSRExitCounter() { return &m_osrExitCounter; }
        
        static ptrdiff_t offsetOfOSRExitCounter() { return OBJECT_OFFSETOF(CodeBlock, m_osrExitCounter); }

#if ENABLE(JIT)
        uint32_t adjustedExitCountThreshold(uint32_t desiredThreshold)
        {
            ASSERT(getJITType() == JITCode::DFGJIT);
            // Compute this the lame way so we don't saturate. This is called infrequently
            // enough that this loop won't hurt us.
            unsigned result = desiredThreshold;
            for (unsigned n = baselineVersion()->reoptimizationRetryCounter(); n--;) {
                unsigned newResult = result << 1;
                if (newResult < result)
                    return std::numeric_limits<uint32_t>::max();
                result = newResult;
            }
            return result;
        }
        
        uint32_t exitCountThresholdForReoptimization()
        {
            return adjustedExitCountThreshold(Options::osrExitCountForReoptimization());
        }
        
        uint32_t exitCountThresholdForReoptimizationFromLoop()
        {
            return adjustedExitCountThreshold(Options::osrExitCountForReoptimizationFromLoop());
        }

        bool shouldReoptimizeNow()
        {
            return osrExitCounter() >= exitCountThresholdForReoptimization();
        }
        
        bool shouldReoptimizeFromLoopNow()
        {
            return osrExitCounter() >= exitCountThresholdForReoptimizationFromLoop();
        }
#endif

#if ENABLE(VALUE_PROFILER)
        bool shouldOptimizeNow();
        void updateAllPredictions(OperationInProgress = NoOperation);
#else
        bool shouldOptimizeNow() { return false; }
        void updateAllPredictions(OperationInProgress = NoOperation) { }
#endif
        
#if ENABLE(JIT)
        void reoptimize();
#endif

#if ENABLE(VERBOSE_VALUE_PROFILE)
        void dumpValueProfiles();
#endif
        
        // FIXME: Make these remaining members private.

        int m_numCalleeRegisters;
        int m_numVars;
        int m_numCapturedVars;
        bool m_isConstructor;

    protected:
#if ENABLE(JIT)
        virtual bool jitCompileImpl(ExecState*) = 0;
#endif
        virtual void visitWeakReferences(SlotVisitor&);
        virtual void finalizeUnconditionally();
        
    private:
        friend class DFGCodeBlocks;
        
#if ENABLE(DFG_JIT)
        void tallyFrequentExitSites();
#else
        void tallyFrequentExitSites() { }
#endif
#if ENABLE(VALUE_PROFILER)
        void updateAllPredictionsAndCountLiveness(OperationInProgress, unsigned& numberOfLiveNonArgumentValueProfiles, unsigned& numberOfSamplesInProfiles);
#endif
        
        void dump(ExecState*, const Vector<Instruction>::const_iterator& begin, Vector<Instruction>::const_iterator&);

        CString registerName(ExecState*, int r) const;
        void printUnaryOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op);
        void printBinaryOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op);
        void printConditionalJump(ExecState*, const Vector<Instruction>::const_iterator&, Vector<Instruction>::const_iterator&, int location, const char* op);
        void printGetByIdOp(ExecState*, int location, Vector<Instruction>::const_iterator&);
        void printGetByIdCacheStatus(ExecState*, int location);
        enum CacheDumpMode { DumpCaches, DontDumpCaches };
        void printCallOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op, CacheDumpMode);
        void printPutByIdOp(ExecState*, int location, Vector<Instruction>::const_iterator&, const char* op);
        void visitStructures(SlotVisitor&, Instruction* vPC);
        
#if ENABLE(DFG_JIT)
        bool shouldImmediatelyAssumeLivenessDuringScan()
        {
            // Null m_dfgData means that this is a baseline JIT CodeBlock. Baseline JIT
            // CodeBlocks don't need to be jettisoned when their weak references go
            // stale. So if a basline JIT CodeBlock gets scanned, we can assume that
            // this means that it's live.
            if (!m_dfgData)
                return true;
            
            // For simplicity, we don't attempt to jettison code blocks during GC if
            // they are executing. Instead we strongly mark their weak references to
            // allow them to continue to execute soundly.
            if (m_dfgData->mayBeExecuting)
                return true;

            return false;
        }
#else
        bool shouldImmediatelyAssumeLivenessDuringScan() { return true; }
#endif
        
        void performTracingFixpointIteration(SlotVisitor&);
        
        void stronglyVisitStrongReferences(SlotVisitor&);
        void stronglyVisitWeakReferences(SlotVisitor&);

        void createRareDataIfNecessary()
        {
            if (!m_rareData)
                m_rareData = adoptPtr(new RareData);
        }

#if ENABLE(JIT)
        void resetStubInternal(RepatchBuffer&, StructureStubInfo&);
#endif
        
        int m_numParameters;

        WriteBarrier<ScriptExecutable> m_ownerExecutable;
        JSGlobalData* m_globalData;

        RefCountedArray<Instruction> m_instructions;

        int m_thisRegister;
        int m_argumentsRegister;
        int m_activationRegister;

        bool m_needsFullScopeChain;
        bool m_usesEval;
        bool m_isNumericCompareFunction;
        bool m_isStrictMode;

        CodeType m_codeType;

        RefPtr<SourceProvider> m_source;
        unsigned m_sourceOffset;

        Vector<unsigned> m_propertyAccessInstructions;
        Vector<unsigned> m_globalResolveInstructions;
#if ENABLE(LLINT)
        SegmentedVector<LLIntCallLinkInfo, 8> m_llintCallLinkInfos;
        SentinelLinkedList<LLIntCallLinkInfo, BasicRawSentinelNode<LLIntCallLinkInfo> > m_incomingLLIntCalls;
#endif
#if ENABLE(JIT)
        Vector<StructureStubInfo> m_structureStubInfos;
        Vector<GlobalResolveInfo> m_globalResolveInfos;
        Vector<CallLinkInfo> m_callLinkInfos;
        Vector<MethodCallLinkInfo> m_methodCallLinkInfos;
        JITCode m_jitCode;
        MacroAssemblerCodePtr m_jitCodeWithArityCheck;
        SentinelLinkedList<CallLinkInfo, BasicRawSentinelNode<CallLinkInfo> > m_incomingCalls;
#endif
#if ENABLE(DFG_JIT) || ENABLE(LLINT)
        OwnPtr<CompactJITCodeMap> m_jitCodeMap;
#endif
#if ENABLE(DFG_JIT)
        struct WeakReferenceTransition {
            WeakReferenceTransition() { }
            
            WeakReferenceTransition(JSGlobalData& globalData, JSCell* owner, JSCell* codeOrigin, JSCell* from, JSCell* to)
                : m_from(globalData, owner, from)
                , m_to(globalData, owner, to)
            {
                if (!!codeOrigin)
                    m_codeOrigin.set(globalData, owner, codeOrigin);
            }

            WriteBarrier<JSCell> m_codeOrigin;
            WriteBarrier<JSCell> m_from;
            WriteBarrier<JSCell> m_to;
        };
        
        struct DFGData {
            DFGData()
                : mayBeExecuting(false)
                , isJettisoned(false)
            {
            }
            
            Vector<DFG::OSREntryData> osrEntry;
            SegmentedVector<DFG::OSRExit, 8> osrExit;
            Vector<DFG::SpeculationRecovery> speculationRecovery;
            SegmentedVector<JumpReplacementWatchpoint, 1, 0> watchpoints;
            Vector<WeakReferenceTransition> transitions;
            Vector<WriteBarrier<JSCell> > weakReferences;
            DFG::VariableEventStream variableEventStream;
            DFG::MinifiedGraph minifiedDFG;
            bool mayBeExecuting;
            bool isJettisoned;
            bool livenessHasBeenProved; // Initialized and used on every GC.
            bool allTransitionsHaveBeenMarked; // Initialized and used on every GC.
            unsigned visitAggregateHasBeenCalled; // Unsigned to make it work seamlessly with the broadest set of CAS implementations.
        };
        
        OwnPtr<DFGData> m_dfgData;
        
        // This is relevant to non-DFG code blocks that serve as the profiled code block
        // for DFG code blocks.
        DFG::ExitProfile m_exitProfile;
        CompressedLazyOperandValueProfileHolder m_lazyOperandValueProfiles;
#endif
#if ENABLE(VALUE_PROFILER)
        Vector<ValueProfile> m_argumentValueProfiles;
        SegmentedVector<ValueProfile, 8> m_valueProfiles;
        SegmentedVector<RareCaseProfile, 8> m_rareCaseProfiles;
        SegmentedVector<RareCaseProfile, 8> m_specialFastCaseProfiles;
        ArrayProfileVector m_arrayProfiles;
        unsigned m_executionEntryCount;
#endif

        Vector<unsigned> m_jumpTargets;
        Vector<unsigned> m_loopTargets;

        // Constant Pool
        Vector<Identifier> m_identifiers;
        COMPILE_ASSERT(sizeof(Register) == sizeof(WriteBarrier<Unknown>), Register_must_be_same_size_as_WriteBarrier_Unknown);
        Vector<WriteBarrier<Unknown> > m_constantRegisters;
        Vector<WriteBarrier<FunctionExecutable> > m_functionDecls;
        Vector<WriteBarrier<FunctionExecutable> > m_functionExprs;

        WriteBarrier<SharedSymbolTable> m_symbolTable;

        OwnPtr<CodeBlock> m_alternative;
        
        ExecutionCounter m_llintExecuteCounter;
        
        ExecutionCounter m_jitExecuteCounter;
        int32_t m_totalJITExecutions;
        uint32_t m_osrExitCounter;
        uint16_t m_optimizationDelayCounter;
        uint16_t m_reoptimizationRetryCounter;

        Vector<LineInfo> m_lineInfo;
#if ENABLE(BYTECODE_COMMENTS)
        Vector<Comment>  m_bytecodeComments;
        size_t m_bytecodeCommentIterator;
#endif

        struct RareData {
           WTF_MAKE_FAST_ALLOCATED;
        public:
            Vector<HandlerInfo> m_exceptionHandlers;

            // Rare Constants
            Vector<WriteBarrier<RegExp> > m_regexps;

            // Buffers used for large array literals
            Vector<Vector<JSValue> > m_constantBuffers;
            
            // Jump Tables
            Vector<SimpleJumpTable> m_immediateSwitchJumpTables;
            Vector<SimpleJumpTable> m_characterSwitchJumpTables;
            Vector<StringJumpTable> m_stringSwitchJumpTables;

            EvalCodeCache m_evalCodeCache;

            // Expression info - present if debugging.
            Vector<ExpressionRangeInfo> m_expressionInfo;
            // Line info - present if profiling or debugging.
#if ENABLE(JIT)
            Vector<CallReturnOffsetToBytecodeOffset> m_callReturnIndexVector;
#endif
#if ENABLE(DFG_JIT)
            SegmentedVector<InlineCallFrame, 4> m_inlineCallFrames;
            Vector<CodeOriginAtCallReturnOffset> m_codeOrigins;
#endif
        };
#if COMPILER(MSVC)
        friend void WTF::deleteOwnedPtr<RareData>(RareData*);
#endif
        OwnPtr<RareData> m_rareData;
#if ENABLE(JIT)
        DFG::CapabilityLevel m_canCompileWithDFGState;
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
        
        GlobalCodeBlock(ScriptExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, PassOwnPtr<CodeBlock> alternative)
            : CodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, sourceOffset, false, alternative)
        {
        }
    };

    class ProgramCodeBlock : public GlobalCodeBlock {
    public:
        ProgramCodeBlock(CopyParsedBlockTag, ProgramCodeBlock& other)
            : GlobalCodeBlock(CopyParsedBlock, other)
        {
        }

        ProgramCodeBlock(ProgramExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, PassOwnPtr<CodeBlock> alternative)
            : GlobalCodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, 0, alternative)
        {
        }
        
#if ENABLE(JIT)
    protected:
        virtual JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex);
        virtual void jettison();
        virtual bool jitCompileImpl(ExecState*);
        virtual CodeBlock* replacement();
        virtual DFG::CapabilityLevel canCompileWithDFGInternal();
#endif
    };

    class EvalCodeBlock : public GlobalCodeBlock {
    public:
        EvalCodeBlock(CopyParsedBlockTag, EvalCodeBlock& other)
            : GlobalCodeBlock(CopyParsedBlock, other)
            , m_baseScopeDepth(other.m_baseScopeDepth)
            , m_variables(other.m_variables)
        {
        }
        
        EvalCodeBlock(EvalExecutable* ownerExecutable, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, int baseScopeDepth, PassOwnPtr<CodeBlock> alternative)
            : GlobalCodeBlock(ownerExecutable, EvalCode, globalObject, sourceProvider, 0, alternative)
            , m_baseScopeDepth(baseScopeDepth)
        {
        }

        int baseScopeDepth() const { return m_baseScopeDepth; }

        const Identifier& variable(unsigned index) { return m_variables[index]; }
        unsigned numVariables() { return m_variables.size(); }
        void adoptVariables(Vector<Identifier>& variables)
        {
            ASSERT(m_variables.isEmpty());
            m_variables.swap(variables);
        }
        
#if ENABLE(JIT)
    protected:
        virtual JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex);
        virtual void jettison();
        virtual bool jitCompileImpl(ExecState*);
        virtual CodeBlock* replacement();
        virtual DFG::CapabilityLevel canCompileWithDFGInternal();
#endif

    private:
        int m_baseScopeDepth;
        Vector<Identifier> m_variables;
    };

    class FunctionCodeBlock : public CodeBlock {
    public:
        FunctionCodeBlock(CopyParsedBlockTag, FunctionCodeBlock& other)
            : CodeBlock(CopyParsedBlock, other)
        {
        }

        FunctionCodeBlock(FunctionExecutable* ownerExecutable, CodeType codeType, JSGlobalObject* globalObject, PassRefPtr<SourceProvider> sourceProvider, unsigned sourceOffset, bool isConstructor, PassOwnPtr<CodeBlock> alternative = nullptr)
            : CodeBlock(ownerExecutable, codeType, globalObject, sourceProvider, sourceOffset, isConstructor, alternative)
        {
        }
        
#if ENABLE(JIT)
    protected:
        virtual JSObject* compileOptimized(ExecState*, JSScope*, unsigned bytecodeIndex);
        virtual void jettison();
        virtual bool jitCompileImpl(ExecState*);
        virtual CodeBlock* replacement();
        virtual DFG::CapabilityLevel canCompileWithDFGInternal();
#endif
    };

    inline CodeBlock* baselineCodeBlockForInlineCallFrame(InlineCallFrame* inlineCallFrame)
    {
        ASSERT(inlineCallFrame);
        ExecutableBase* executable = inlineCallFrame->executable.get();
        ASSERT(executable->structure()->classInfo() == &FunctionExecutable::s_info);
        return static_cast<FunctionExecutable*>(executable)->baselineCodeBlockFor(inlineCallFrame->isCall ? CodeForCall : CodeForConstruct);
    }
    
    inline CodeBlock* baselineCodeBlockForOriginAndBaselineCodeBlock(const CodeOrigin& codeOrigin, CodeBlock* baselineCodeBlock)
    {
        if (codeOrigin.inlineCallFrame)
            return baselineCodeBlockForInlineCallFrame(codeOrigin.inlineCallFrame);
        return baselineCodeBlock;
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
        ASSERT(index < FirstConstantRegisterIndex);
        return this[index];
    }

#if ENABLE(DFG_JIT)
    inline bool ExecState::isInlineCallFrame()
    {
        if (LIKELY(!codeBlock() || codeBlock()->getJITType() != JITCode::DFGJIT))
            return false;
        return isInlineCallFrameSlow();
    }
#endif

    inline JSValue ExecState::argumentAfterCapture(size_t argument)
    {
        if (argument >= argumentCount())
             return jsUndefined();

        if (!codeBlock())
            return this[argumentOffset(argument)].jsValue();

        if (argument >= static_cast<size_t>(codeBlock()->symbolTable()->parameterCount()))
            return this[argumentOffset(argument)].jsValue();

        const SlowArgument* slowArguments = codeBlock()->symbolTable()->slowArguments();
        if (!slowArguments || slowArguments[argument].status == SlowArgument::Normal)
            return this[argumentOffset(argument)].jsValue();

        ASSERT(slowArguments[argument].status == SlowArgument::Captured);
        return this[slowArguments[argument].indexIfCaptured].jsValue();
    }

#if ENABLE(DFG_JIT)
    inline void DFGCodeBlocks::mark(void* candidateCodeBlock)
    {
        // We have to check for 0 and -1 because those are used by the HashMap as markers.
        uintptr_t value = reinterpret_cast<uintptr_t>(candidateCodeBlock);
        
        // This checks for both of those nasty cases in one go.
        // 0 + 1 = 1
        // -1 + 1 = 0
        if (value + 1 <= 1)
            return;
        
        HashSet<CodeBlock*>::iterator iter = m_set.find(static_cast<CodeBlock*>(candidateCodeBlock));
        if (iter == m_set.end())
            return;
        
        (*iter)->m_dfgData->mayBeExecuting = true;
    }
#endif
    
    inline JSValue Structure::prototypeForLookup(CodeBlock* codeBlock) const
    {
        return prototypeForLookup(codeBlock->globalObject());
    }

} // namespace JSC

#endif // CodeBlock_h
