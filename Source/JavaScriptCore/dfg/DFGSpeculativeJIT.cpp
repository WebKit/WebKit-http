/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#include "config.h"
#include "DFGSpeculativeJIT.h"

#if ENABLE(DFG_JIT)

#include "Arguments.h"
#include "DFGSlowPathGenerator.h"
#include "LinkBuffer.h"

namespace JSC { namespace DFG {

SpeculativeJIT::SpeculativeJIT(JITCompiler& jit)
    : m_compileOkay(true)
    , m_jit(jit)
    , m_compileIndex(0)
    , m_indexInBlock(0)
    , m_generationInfo(m_jit.codeBlock()->m_numCalleeRegisters)
    , m_blockHeads(jit.graph().m_blocks.size())
    , m_arguments(jit.codeBlock()->numParameters())
    , m_variables(jit.graph().m_localVars)
    , m_lastSetOperand(std::numeric_limits<int>::max())
    , m_state(m_jit.graph())
    , m_stream(&jit.codeBlock()->variableEventStream())
    , m_minifiedGraph(&jit.codeBlock()->minifiedDFG())
    , m_isCheckingArgumentTypes(false)
{
}

SpeculativeJIT::~SpeculativeJIT()
{
    WTF::deleteAllValues(m_slowPathGenerators);
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail)
{
    if (!m_compileOkay)
        return;
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(nodeIndex), jumpToFail, this, m_stream->size()));
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    speculationCheck(kind, jsValueSource, nodeUse.index(), jumpToFail);
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::JumpList& jumpsToFail)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    Vector<MacroAssembler::Jump, 16> jumpVector = jumpsToFail.jumps();
    for (unsigned i = 0; i < jumpVector.size(); ++i)
        speculationCheck(kind, jsValueSource, nodeIndex, jumpVector[i]);
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::JumpList& jumpsToFail)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    speculationCheck(kind, jsValueSource, nodeUse.index(), jumpsToFail);
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    if (!m_compileOkay)
        return;
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    m_jit.codeBlock()->appendSpeculationRecovery(recovery);
    m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(nodeIndex), jumpToFail, this, m_stream->size(), m_jit.codeBlock()->numberOfSpeculationRecoveries()));
}

void SpeculativeJIT::speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    speculationCheck(kind, jsValueSource, nodeUse.index(), jumpToFail, recovery);
}

JumpReplacementWatchpoint* SpeculativeJIT::speculationWatchpoint(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex)
{
    if (!m_compileOkay)
        return 0;
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    OSRExit& exit = m_jit.codeBlock()->osrExit(
        m_jit.codeBlock()->appendOSRExit(
            OSRExit(kind, jsValueSource,
                    m_jit.graph().methodOfGettingAValueProfileFor(nodeIndex),
                    JITCompiler::Jump(), this, m_stream->size())));
    exit.m_watchpointIndex = m_jit.codeBlock()->appendWatchpoint(
        JumpReplacementWatchpoint(m_jit.watchpointLabel()));
    return &m_jit.codeBlock()->watchpoint(exit.m_watchpointIndex);
}

JumpReplacementWatchpoint* SpeculativeJIT::speculationWatchpoint(ExitKind kind)
{
    return speculationWatchpoint(kind, JSValueSource(), NoNode);
}

void SpeculativeJIT::convertLastOSRExitToForward(const ValueRecovery& valueRecovery)
{
#if !ASSERT_DISABLED
    if (!valueRecovery) {
        // Check that the preceding node was a SetLocal with the same code origin.
        Node* setLocal = &at(m_jit.graph().m_blocks[m_block]->at(m_indexInBlock - 1));
        ASSERT(setLocal->op() == SetLocal);
        ASSERT(setLocal->codeOrigin == at(m_compileIndex).codeOrigin);
    }
#endif
    
    unsigned setLocalIndexInBlock = m_indexInBlock + 1;
    
    Node* setLocal = &at(m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock));
    bool hadInt32ToDouble = false;
    
    if (setLocal->op() == Int32ToDouble) {
        setLocal = &at(m_jit.graph().m_blocks[m_block]->at(++setLocalIndexInBlock));
        hadInt32ToDouble = true;
    }
    if (setLocal->op() == Flush || setLocal->op() == Phantom)
        setLocal = &at(m_jit.graph().m_blocks[m_block]->at(++setLocalIndexInBlock));
        
    if (!!valueRecovery) {
        if (hadInt32ToDouble)
            ASSERT(at(setLocal->child1()).child1() == m_compileIndex);
        else
            ASSERT(setLocal->child1() == m_compileIndex);
    }
    ASSERT(setLocal->op() == SetLocal);
    ASSERT(setLocal->codeOrigin == at(m_compileIndex).codeOrigin);

    Node* nextNode = &at(m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock + 1));
    if (nextNode->op() == Jump && nextNode->codeOrigin == at(m_compileIndex).codeOrigin) {
        // We're at an inlined return. Use a backward speculation instead.
        return;
    }
    ASSERT(nextNode->codeOrigin != at(m_compileIndex).codeOrigin);
        
    OSRExit& exit = m_jit.codeBlock()->lastOSRExit();
    exit.m_codeOrigin = nextNode->codeOrigin;
        
    if (!valueRecovery)
        return;
    exit.m_lastSetOperand = setLocal->local();
    exit.m_valueRecoveryOverride = adoptRef(
        new ValueRecoveryOverride(setLocal->local(), valueRecovery));
}

JumpReplacementWatchpoint* SpeculativeJIT::forwardSpeculationWatchpoint(ExitKind kind)
{
    JumpReplacementWatchpoint* result = speculationWatchpoint(kind);
    convertLastOSRExitToForward();
    return result;
}

JumpReplacementWatchpoint* SpeculativeJIT::speculationWatchpointWithConditionalDirection(ExitKind kind, bool isForward)
{
    JumpReplacementWatchpoint* result = speculationWatchpoint(kind);
    if (isForward)
        convertLastOSRExitToForward();
    return result;
}

void SpeculativeJIT::forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail, const ValueRecovery& valueRecovery)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    speculationCheck(kind, jsValueSource, nodeIndex, jumpToFail);
    convertLastOSRExitToForward(valueRecovery);
}

void SpeculativeJIT::forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::JumpList& jumpsToFail, const ValueRecovery& valueRecovery)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    Vector<MacroAssembler::Jump, 16> jumpVector = jumpsToFail.jumps();
    for (unsigned i = 0; i < jumpVector.size(); ++i)
        forwardSpeculationCheck(kind, jsValueSource, nodeIndex, jumpVector[i], valueRecovery);
}

void SpeculativeJIT::speculationCheckWithConditionalDirection(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail, bool isForward)
{
    if (isForward)
        forwardSpeculationCheck(kind, jsValueSource, nodeIndex, jumpToFail);
    else
        speculationCheck(kind, jsValueSource, nodeIndex, jumpToFail);
}

void SpeculativeJIT::terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, NodeIndex nodeIndex)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("SpeculativeJIT was terminated.\n");
#endif
    if (!m_compileOkay)
        return;
    speculationCheck(kind, jsValueRegs, nodeIndex, m_jit.jump());
    m_compileOkay = false;
}

void SpeculativeJIT::terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, Edge nodeUse)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
    terminateSpeculativeExecution(kind, jsValueRegs, nodeUse.index());
}

void SpeculativeJIT::terminateSpeculativeExecutionWithConditionalDirection(ExitKind kind, JSValueRegs jsValueRegs, NodeIndex nodeIndex, bool isForward)
{
    ASSERT(at(m_compileIndex).canExit() || m_isCheckingArgumentTypes);
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("SpeculativeJIT was terminated.\n");
#endif
    if (!m_compileOkay)
        return;
    speculationCheckWithConditionalDirection(kind, jsValueRegs, nodeIndex, m_jit.jump(), isForward);
    m_compileOkay = false;
}

void SpeculativeJIT::addSlowPathGenerator(PassOwnPtr<SlowPathGenerator> slowPathGenerator)
{
    m_slowPathGenerators.append(slowPathGenerator.leakPtr());
}

void SpeculativeJIT::runSlowPathGenerators()
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Running %lu slow path generators.\n", m_slowPathGenerators.size());
#endif
    for (unsigned i = 0; i < m_slowPathGenerators.size(); ++i)
        m_slowPathGenerators[i]->generate(this);
}

// On Windows we need to wrap fmod; on other platforms we can call it directly.
// On ARMv7 we assert that all function pointers have to low bit set (point to thumb code).
#if CALLING_CONVENTION_IS_STDCALL || CPU(ARM_THUMB2)
static double DFG_OPERATION fmodAsDFGOperation(double x, double y)
{
    return fmod(x, y);
}
#else
#define fmodAsDFGOperation fmod
#endif

void SpeculativeJIT::clearGenerationInfo()
{
    for (unsigned i = 0; i < m_generationInfo.size(); ++i)
        m_generationInfo[i] = GenerationInfo();
    m_gprs = RegisterBank<GPRInfo>();
    m_fprs = RegisterBank<FPRInfo>();
}

const TypedArrayDescriptor* SpeculativeJIT::typedArrayDescriptor(Array::Mode arrayMode)
{
    switch (arrayMode) {
    case Array::Int8Array:
        return &m_jit.globalData()->int8ArrayDescriptor();
    case Array::Int16Array:
        return &m_jit.globalData()->int16ArrayDescriptor();
    case Array::Int32Array:
        return &m_jit.globalData()->int32ArrayDescriptor();
    case Array::Uint8Array:
        return &m_jit.globalData()->uint8ArrayDescriptor();
    case Array::Uint8ClampedArray:
        return &m_jit.globalData()->uint8ClampedArrayDescriptor();
    case Array::Uint16Array:
        return &m_jit.globalData()->uint16ArrayDescriptor();
    case Array::Uint32Array:
        return &m_jit.globalData()->uint32ArrayDescriptor();
    case Array::Float32Array:
        return &m_jit.globalData()->float32ArrayDescriptor();
    case Array::Float64Array:
        return &m_jit.globalData()->float32ArrayDescriptor();
    default:
        return 0;
    }
}

void SpeculativeJIT::checkArray(Node& node)
{
    ASSERT(modeIsSpecific(node.arrayMode()));
    
    SpeculateCellOperand base(this, node.child1());
    GPRReg baseReg = base.gpr();
    
    const TypedArrayDescriptor* result = typedArrayDescriptor(node.arrayMode());
    
    if (modeAlreadyChecked(m_state.forNode(node.child1()), node.arrayMode())) {
        noResult(m_compileIndex);
        return;
    }
    
    const ClassInfo* expectedClassInfo = 0;
    
    switch (node.arrayMode()) {
    case Array::String:
        expectedClassInfo = &JSString::s_info;
        break;
    case NON_ARRAY_ARRAY_STORAGE_MODES: {
        GPRTemporary temp(this);
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), temp.gpr());
        speculationCheck(
            Uncountable, JSValueRegs(), NoNode,
            m_jit.branchTest8(
                MacroAssembler::Zero,
                MacroAssembler::Address(temp.gpr(), Structure::indexingTypeOffset()),
                MacroAssembler::TrustedImm32(
                    isSlowPutAccess(node.arrayMode()) ? (HasArrayStorage | HasSlowPutArrayStorage) : HasArrayStorage)));
        
        noResult(m_compileIndex);
        return;
    }
    case ARRAY_WITH_ARRAY_STORAGE_MODES: {
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), tempGPR);
        m_jit.load8(MacroAssembler::Address(tempGPR, Structure::indexingTypeOffset()), tempGPR);
        // FIXME: This can be turned into a single branch. But we currently have no evidence
        // that doing so would be profitable, nor do I feel comfortable with the present test
        // coverage for this code path.
        speculationCheck(
            Uncountable, JSValueRegs(), NoNode,
            m_jit.branchTest32(
                MacroAssembler::Zero, tempGPR, MacroAssembler::TrustedImm32(IsArray)));
        speculationCheck(
            Uncountable, JSValueRegs(), NoNode,
            m_jit.branchTest32(
                MacroAssembler::Zero, tempGPR, MacroAssembler::TrustedImm32(
                    isSlowPutAccess(node.arrayMode()) ? (HasArrayStorage | HasSlowPutArrayStorage) : HasArrayStorage)));
        
        noResult(m_compileIndex);
        return;
    }
    case Array::Arguments:
        expectedClassInfo = &Arguments::s_info;
        break;
    case Array::Int8Array:
    case Array::Int16Array:
    case Array::Int32Array:
    case Array::Uint8Array:
    case Array::Uint8ClampedArray:
    case Array::Uint16Array:
    case Array::Uint32Array:
    case Array::Float32Array:
    case Array::Float64Array:
        expectedClassInfo = result->m_classInfo;
        break;
    default:
        ASSERT_NOT_REACHED();
        break;
    }
    
    GPRTemporary temp(this);
    m_jit.loadPtr(
        MacroAssembler::Address(baseReg, JSCell::structureOffset()), temp.gpr());
    speculationCheck(
        Uncountable, JSValueRegs(), NoNode,
        m_jit.branchPtr(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(temp.gpr(), Structure::classInfoOffset()),
            MacroAssembler::TrustedImmPtr(expectedClassInfo)));
    
    noResult(m_compileIndex);
}

void SpeculativeJIT::arrayify(Node& node)
{
    ASSERT(modeIsSpecific(node.arrayMode()));
    ASSERT(!modeAlreadyChecked(m_state.forNode(node.child1()), node.arrayMode()));
    
    SpeculateCellOperand base(this, node.child1());
    GPRReg baseReg = base.gpr();
    
    switch (node.arrayMode()) {
    case EFFECTFUL_NON_ARRAY_ARRAY_STORAGE_MODES: {
        GPRTemporary structure(this);
        GPRTemporary temp(this);
        GPRReg structureGPR = structure.gpr();
        GPRReg tempGPR = temp.gpr();
        
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), structureGPR);
        
        // We can skip all that comes next if we already have array storage.
        IndexingType desiredIndexingTypeMask =
            isSlowPutAccess(node.arrayMode()) ? (HasArrayStorage | HasSlowPutArrayStorage) : HasArrayStorage;
        MacroAssembler::Jump slowCase = m_jit.branchTest8(
            MacroAssembler::Zero,
            MacroAssembler::Address(structureGPR, Structure::indexingTypeOffset()),
            MacroAssembler::TrustedImm32(desiredIndexingTypeMask));
        
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSObject::butterflyOffset()), tempGPR);
        
        MacroAssembler::Jump done = m_jit.jump();
        
        slowCase.link(&m_jit);
        
        // Next check that the object does not intercept indexed accesses. If it does,
        // then this mode won't work.
        speculationCheck(
            Uncountable, JSValueRegs(), NoNode,
            m_jit.branchTest8(
                MacroAssembler::NonZero,
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()),
                MacroAssembler::TrustedImm32(InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero)));
        
        // Now call out to create the array storage.
        silentSpillAllRegisters(tempGPR);
        callOperation(operationEnsureArrayStorage, tempGPR, baseReg);
        silentFillAllRegisters(tempGPR);

        // Alas, we need to reload the structure because silent spilling does not save
        // temporaries. Nor would it be useful for it to do so. Either way we're talking
        // about a load.
        m_jit.loadPtr(
            MacroAssembler::Address(baseReg, JSCell::structureOffset()), structureGPR);
        
        // Finally, check that we have the kind of array storage that we wanted to get.
        // Note that this is a backwards speculation check, which will result in the 
        // bytecode operation corresponding to this arrayification being reexecuted.
        // That's fine, since arrayification is not user-visible.
        speculationCheck(
            Uncountable, JSValueRegs(), NoNode,
            m_jit.branchTest8(
                MacroAssembler::Zero,
                MacroAssembler::Address(structureGPR, Structure::indexingTypeOffset()),
                MacroAssembler::TrustedImm32(desiredIndexingTypeMask)));
        
        done.link(&m_jit);
        storageResult(tempGPR, m_compileIndex);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
        break;
    }
}

GPRReg SpeculativeJIT::fillStorage(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatStorage) {
            GPRReg gpr = allocate();
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillStorage(*m_stream, gpr);
            return gpr;
        }
        
        // Must be a cell; fill it as a cell and then return the pointer.
        return fillSpeculateCell(nodeIndex);
    }
        
    case DataFormatStorage: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
        
    default:
        return fillSpeculateCell(nodeIndex);
    }
}

void SpeculativeJIT::useChildren(Node& node)
{
    if (node.flags() & NodeHasVarArgs) {
        for (unsigned childIdx = node.firstChild(); childIdx < node.firstChild() + node.numChildren(); childIdx++) {
            if (!!m_jit.graph().m_varArgChildren[childIdx])
                use(m_jit.graph().m_varArgChildren[childIdx]);
        }
    } else {
        Edge child1 = node.child1();
        if (!child1) {
            ASSERT(!node.child2() && !node.child3());
            return;
        }
        use(child1);
        
        Edge child2 = node.child2();
        if (!child2) {
            ASSERT(!node.child3());
            return;
        }
        use(child2);
        
        Edge child3 = node.child3();
        if (!child3)
            return;
        use(child3);
    }
}

bool SpeculativeJIT::isStrictInt32(NodeIndex nodeIndex)
{
    if (isInt32Constant(nodeIndex))
        return true;
    
    Node& node = m_jit.graph()[nodeIndex];
    GenerationInfo& info = m_generationInfo[node.virtualRegister()];
    
    return info.registerFormat() == DataFormatInteger;
}

bool SpeculativeJIT::isKnownInteger(NodeIndex nodeIndex)
{
    if (isInt32Constant(nodeIndex))
        return true;

    Node& node = m_jit.graph()[nodeIndex];
    
    if (node.hasInt32Result())
        return true;
    
    GenerationInfo& info = m_generationInfo[node.virtualRegister()];

    return info.isJSInteger();
}

bool SpeculativeJIT::isKnownNumeric(NodeIndex nodeIndex)
{
    if (isInt32Constant(nodeIndex) || isNumberConstant(nodeIndex))
        return true;

    Node& node = m_jit.graph()[nodeIndex];
    
    if (node.hasNumberResult())
        return true;
    
    GenerationInfo& info = m_generationInfo[node.virtualRegister()];

    return info.isJSInteger() || info.isJSDouble();
}

bool SpeculativeJIT::isKnownCell(NodeIndex nodeIndex)
{
    return m_generationInfo[m_jit.graph()[nodeIndex].virtualRegister()].isJSCell();
}

bool SpeculativeJIT::isKnownNotCell(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    if (node.hasConstant() && !valueOfJSConstant(nodeIndex).isCell())
        return true;
    return !(info.isJSCell() || info.isUnknownJS());
}

bool SpeculativeJIT::isKnownNotInteger(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    return info.isJSDouble() || info.isJSCell() || info.isJSBoolean()
        || (node.hasConstant() && !valueOfJSConstant(nodeIndex).isInt32());
}

bool SpeculativeJIT::isKnownNotNumber(NodeIndex nodeIndex)
{
    Node& node = m_jit.graph()[nodeIndex];
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    return (!info.isJSDouble() && !info.isJSInteger() && !info.isUnknownJS())
        || (node.hasConstant() && !isNumberConstant(nodeIndex));
}

void SpeculativeJIT::writeBarrier(MacroAssembler& jit, GPRReg owner, GPRReg scratch1, GPRReg scratch2, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(jit);
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    ASSERT(owner != scratch1);
    ASSERT(owner != scratch2);
    ASSERT(scratch1 != scratch2);

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif
    markCellCard(jit, owner, scratch1, scratch2);
}

void SpeculativeJIT::markCellCard(MacroAssembler& jit, GPRReg owner, GPRReg scratch1, GPRReg scratch2)
{
    UNUSED_PARAM(jit);
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    
#if ENABLE(GGC)
    jit.move(owner, scratch1);
    jit.andPtr(TrustedImm32(static_cast<int32_t>(MarkedBlock::blockMask)), scratch1);
    jit.move(owner, scratch2);
    // consume additional 8 bits as we're using an approximate filter
    jit.rshift32(TrustedImm32(MarkedBlock::atomShift + 8), scratch2);
    jit.andPtr(TrustedImm32(MarkedBlock::atomMask >> 8), scratch2);
    MacroAssembler::Jump filter = jit.branchTest8(MacroAssembler::Zero, MacroAssembler::BaseIndex(scratch1, scratch2, MacroAssembler::TimesOne, MarkedBlock::offsetOfMarks()));
    jit.move(owner, scratch2);
    jit.rshift32(TrustedImm32(MarkedBlock::cardShift), scratch2);
    jit.andPtr(TrustedImm32(MarkedBlock::cardMask), scratch2);
    jit.store8(TrustedImm32(1), MacroAssembler::BaseIndex(scratch1, scratch2, MacroAssembler::TimesOne, MarkedBlock::offsetOfCards()));
    filter.link(&jit);
#endif
}

void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind useKind, GPRReg scratch1, GPRReg scratch2)
{
    UNUSED_PARAM(ownerGPR);
    UNUSED_PARAM(valueGPR);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);

    if (isKnownNotCell(valueUse.index()))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif

#if ENABLE(GGC)
    GPRTemporary temp1;
    GPRTemporary temp2;
    if (scratch1 == InvalidGPRReg) {
        GPRTemporary scratchGPR(this);
        temp1.adopt(scratchGPR);
        scratch1 = temp1.gpr();
    }
    if (scratch2 == InvalidGPRReg) {
        GPRTemporary scratchGPR(this);
        temp2.adopt(scratchGPR);
        scratch2 = temp2.gpr();
    }
    
    JITCompiler::Jump rhsNotCell;
    bool hadCellCheck = false;
    if (!isKnownCell(valueUse.index()) && !isCellSpeculation(m_jit.getSpeculation(valueUse.index()))) {
        hadCellCheck = true;
        rhsNotCell = m_jit.branchIfNotCell(valueGPR);
    }

    markCellCard(m_jit, ownerGPR, scratch1, scratch2);

    if (hadCellCheck)
        rhsNotCell.link(&m_jit);
#endif
}

void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, JSCell* value, WriteBarrierUseKind useKind, GPRReg scratch1, GPRReg scratch2)
{
    UNUSED_PARAM(ownerGPR);
    UNUSED_PARAM(value);
    UNUSED_PARAM(scratch1);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    
    if (Heap::isMarked(value))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif

#if ENABLE(GGC)
    GPRTemporary temp1;
    GPRTemporary temp2;
    if (scratch1 == InvalidGPRReg) {
        GPRTemporary scratchGPR(this);
        temp1.adopt(scratchGPR);
        scratch1 = temp1.gpr();
    }
    if (scratch2 == InvalidGPRReg) {
        GPRTemporary scratchGPR(this);
        temp2.adopt(scratchGPR);
        scratch2 = temp2.gpr();
    }

    markCellCard(m_jit, ownerGPR, scratch1, scratch2);
#endif
}

void SpeculativeJIT::writeBarrier(JSCell* owner, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind useKind, GPRReg scratch)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(valueGPR);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(useKind);

    if (isKnownNotCell(valueUse.index()))
        return;

#if ENABLE(WRITE_BARRIER_PROFILING)
    JITCompiler::emitCount(m_jit, WriteBarrierCounters::jitCounterFor(useKind));
#endif

#if ENABLE(GGC)
    JITCompiler::Jump rhsNotCell;
    bool hadCellCheck = false;
    if (!isKnownCell(valueUse.index()) && !isCellSpeculation(m_jit.getSpeculation(valueUse.index()))) {
        hadCellCheck = true;
        rhsNotCell = m_jit.branchIfNotCell(valueGPR);
    }
    
    GPRTemporary temp;
    if (scratch == InvalidGPRReg) {
        GPRTemporary scratchGPR(this);
        temp.adopt(scratchGPR);
        scratch = temp.gpr();
    }

    uint8_t* cardAddress = Heap::addressOfCardFor(owner);
    m_jit.move(JITCompiler::TrustedImmPtr(cardAddress), scratch);
    m_jit.store8(JITCompiler::TrustedImm32(1), JITCompiler::Address(scratch));

    if (hadCellCheck)
        rhsNotCell.link(&m_jit);
#endif
}

bool SpeculativeJIT::nonSpeculativeCompare(Node& node, MacroAssembler::RelationalCondition cond, S_DFGOperation_EJJ helperFunction)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        ASSERT(node.adjustedRefCount() == 1);
        
        nonSpeculativePeepholeBranch(node, branchNodeIndex, cond, helperFunction);
    
        m_indexInBlock = branchIndexInBlock;
        m_compileIndex = branchNodeIndex;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeCompare(node, cond, helperFunction);
    
    return false;
}

bool SpeculativeJIT::nonSpeculativeStrictEq(Node& node, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        ASSERT(node.adjustedRefCount() == 1);
        
        nonSpeculativePeepholeStrictEq(node, branchNodeIndex, invert);
    
        m_indexInBlock = branchIndexInBlock;
        m_compileIndex = branchNodeIndex;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeStrictEq(node, invert);
    
    return false;
}

#ifndef NDEBUG
static const char* dataFormatString(DataFormat format)
{
    // These values correspond to the DataFormat enum.
    const char* strings[] = {
        "[  ]",
        "[ i]",
        "[ d]",
        "[ c]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
        "[J ]",
        "[Ji]",
        "[Jd]",
        "[Jc]",
        "Err!",
        "Err!",
        "Err!",
        "Err!",
    };
    return strings[format];
}

void SpeculativeJIT::dump(const char* label)
{
    if (label)
        dataLog("<%s>\n", label);

    dataLog("  gprs:\n");
    m_gprs.dump();
    dataLog("  fprs:\n");
    m_fprs.dump();
    dataLog("  VirtualRegisters:\n");
    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        GenerationInfo& info = m_generationInfo[i];
        if (info.alive())
            dataLog("    % 3d:%s%s", i, dataFormatString(info.registerFormat()), dataFormatString(info.spillFormat()));
        else
            dataLog("    % 3d:[__][__]", i);
        if (info.registerFormat() == DataFormatDouble)
            dataLog(":fpr%d\n", info.fpr());
        else if (info.registerFormat() != DataFormatNone
#if USE(JSVALUE32_64)
            && !(info.registerFormat() & DataFormatJS)
#endif
            ) {
            ASSERT(info.gpr() != InvalidGPRReg);
            dataLog(":%s\n", GPRInfo::debugName(info.gpr()));
        } else
            dataLog("\n");
    }
    if (label)
        dataLog("</%s>\n", label);
}
#endif


#if DFG_ENABLE(CONSISTENCY_CHECK)
void SpeculativeJIT::checkConsistency()
{
    bool failed = false;

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        if (iter.isLocked()) {
            dataLog("DFG_CONSISTENCY_CHECK failed: gpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }
    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        if (iter.isLocked()) {
            dataLog("DFG_CONSISTENCY_CHECK failed: fpr %s is locked.\n", iter.debugName());
            failed = true;
        }
    }

    for (unsigned i = 0; i < m_generationInfo.size(); ++i) {
        VirtualRegister virtualRegister = (VirtualRegister)i;
        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (!info.alive())
            continue;
        switch (info.registerFormat()) {
        case DataFormatNone:
            break;
        case DataFormatJS:
        case DataFormatJSInteger:
        case DataFormatJSDouble:
        case DataFormatJSCell:
        case DataFormatJSBoolean:
#if USE(JSVALUE32_64)
            break;
#endif
        case DataFormatInteger:
        case DataFormatCell:
        case DataFormatBoolean:
        case DataFormatStorage: {
            GPRReg gpr = info.gpr();
            ASSERT(gpr != InvalidGPRReg);
            if (m_gprs.name(gpr) != virtualRegister) {
                dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (gpr %s).\n", virtualRegister, GPRInfo::debugName(gpr));
                failed = true;
            }
            break;
        }
        case DataFormatDouble: {
            FPRReg fpr = info.fpr();
            ASSERT(fpr != InvalidFPRReg);
            if (m_fprs.name(fpr) != virtualRegister) {
                dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for virtual register %d (fpr %s).\n", virtualRegister, FPRInfo::debugName(fpr));
                failed = true;
            }
            break;
        }
        }
    }

    for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
#if USE(JSVALUE64)
        if (iter.regID() != info.gpr()) {
            dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
#else
        if (!(info.registerFormat() & DataFormatJS)) {
            if (iter.regID() != info.gpr()) {
                dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
                failed = true;
            }
        } else {
            if (iter.regID() != info.tagGPR() && iter.regID() != info.payloadGPR()) {
                dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for gpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
                failed = true;
            }
        }
#endif
    }

    for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
        VirtualRegister virtualRegister = iter.name();
        if (virtualRegister == InvalidVirtualRegister)
            continue;

        GenerationInfo& info = m_generationInfo[virtualRegister];
        if (iter.regID() != info.fpr()) {
            dataLog("DFG_CONSISTENCY_CHECK failed: name mismatch for fpr %s (virtual register %d).\n", iter.debugName(), virtualRegister);
            failed = true;
        }
    }

    if (failed) {
        dump();
        CRASH();
    }
}
#endif

GPRTemporary::GPRTemporary()
    : m_jit(0)
    , m_gpr(InvalidGPRReg)
{
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, GPRReg specific)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    m_gpr = m_jit->allocate(specific);
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateIntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateIntegerOperand& op1, SpeculateIntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.index()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateStrictInt32Operand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, IntegerOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, IntegerOperand& op1, IntegerOperand& op2)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else if (m_jit->canReuse(op2.index()))
        m_gpr = m_jit->reuse(op2.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateCellOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, SpeculateBooleanOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

#if USE(JSVALUE64)
GPRTemporary::GPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}
#else
GPRTemporary::GPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1, bool tag)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (!op1.isDouble() && m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(tag ? op1.tagGPR() : op1.payloadGPR());
    else
        m_gpr = m_jit->allocate();
}
#endif

GPRTemporary::GPRTemporary(SpeculativeJIT* jit, StorageOperand& op1)
    : m_jit(jit)
    , m_gpr(InvalidGPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_gpr = m_jit->reuse(op1.gpr());
    else
        m_gpr = m_jit->allocate();
}

void GPRTemporary::adopt(GPRTemporary& other)
{
    ASSERT(!m_jit);
    ASSERT(m_gpr == InvalidGPRReg);
    ASSERT(other.m_jit);
    ASSERT(other.m_gpr != InvalidGPRReg);
    m_jit = other.m_jit;
    m_gpr = other.m_gpr;
    other.m_jit = 0;
    other.m_gpr = InvalidGPRReg;
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, DoubleOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, DoubleOperand& op1, DoubleOperand& op2)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else if (m_jit->canReuse(op2.index()))
        m_fpr = m_jit->reuse(op2.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, SpeculateDoubleOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

FPRTemporary::FPRTemporary(SpeculativeJIT* jit, SpeculateDoubleOperand& op1, SpeculateDoubleOperand& op2)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else if (m_jit->canReuse(op2.index()))
        m_fpr = m_jit->reuse(op2.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}

#if USE(JSVALUE32_64)
FPRTemporary::FPRTemporary(SpeculativeJIT* jit, JSValueOperand& op1)
    : m_jit(jit)
    , m_fpr(InvalidFPRReg)
{
    if (op1.isDouble() && m_jit->canReuse(op1.index()))
        m_fpr = m_jit->reuse(op1.fpr());
    else
        m_fpr = m_jit->fprAllocate();
}
#endif

void SpeculativeJIT::compilePeepHoleDoubleBranch(Node& node, NodeIndex branchNodeIndex, JITCompiler::DoubleCondition condition)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = branchNode.takenBlockIndex();
    BlockIndex notTaken = branchNode.notTakenBlockIndex();
    
    SpeculateDoubleOperand op1(this, node.child1());
    SpeculateDoubleOperand op2(this, node.child2());
    
    branchDouble(condition, op1.fpr(), op2.fpr(), taken);
    jump(notTaken);
}

void SpeculativeJIT::compilePeepHoleObjectEquality(Node& node, NodeIndex branchNodeIndex)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = branchNode.takenBlockIndex();
    BlockIndex notTaken = branchNode.notTakenBlockIndex();

    MacroAssembler::RelationalCondition condition = MacroAssembler::Equal;
    
    if (taken == nextBlock()) {
        condition = MacroAssembler::NotEqual;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    SpeculateCellOperand op1(this, node.child1());
    SpeculateCellOperand op2(this, node.child2());
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    if (m_jit.graph().globalObjectFor(node.codeOrigin)->masqueradesAsUndefinedWatchpoint()->isStillValid()) {
        m_jit.graph().globalObjectFor(node.codeOrigin)->masqueradesAsUndefinedWatchpoint()->add(speculationWatchpoint());
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node.child1().index(), 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.globalData()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node.child2().index(), 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.globalData()->stringStructure.get())));
    } else {
        GPRTemporary structure(this);
        GPRReg structureGPR = structure.gpr();

        m_jit.loadPtr(MacroAssembler::Address(op1GPR, JSCell::structureOffset()), structureGPR);
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node.child1().index(), 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.globalData()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node.child1().index(), 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));

        m_jit.loadPtr(MacroAssembler::Address(op2GPR, JSCell::structureOffset()), structureGPR);
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node.child2().index(), 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                structureGPR, 
                MacroAssembler::TrustedImmPtr(m_jit.globalData()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node.child2().index(), 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(structureGPR, Structure::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }

    branchPtr(condition, op1GPR, op2GPR, taken);
    jump(notTaken);
}

void SpeculativeJIT::compilePeepHoleIntegerBranch(Node& node, NodeIndex branchNodeIndex, JITCompiler::RelationalCondition condition)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = branchNode.takenBlockIndex();
    BlockIndex notTaken = branchNode.notTakenBlockIndex();

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        condition = JITCompiler::invert(condition);
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    if (isInt32Constant(node.child1().index())) {
        int32_t imm = valueOfInt32Constant(node.child1().index());
        SpeculateIntegerOperand op2(this, node.child2());
        branch32(condition, JITCompiler::Imm32(imm), op2.gpr(), taken);
    } else if (isInt32Constant(node.child2().index())) {
        SpeculateIntegerOperand op1(this, node.child1());
        int32_t imm = valueOfInt32Constant(node.child2().index());
        branch32(condition, op1.gpr(), JITCompiler::Imm32(imm), taken);
    } else {
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        branch32(condition, op1.gpr(), op2.gpr(), taken);
    }

    jump(notTaken);
}

// Returns true if the compare is fused with a subsequent branch.
bool SpeculativeJIT::compilePeepHoleBranch(Node& node, MacroAssembler::RelationalCondition condition, MacroAssembler::DoubleCondition doubleCondition, S_DFGOperation_EJJ operation)
{
    // Fused compare & branch.
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);

        // detectPeepHoleBranch currently only permits the branch to be the very next node,
        // so can be no intervening nodes to also reference the compare. 
        ASSERT(node.adjustedRefCount() == 1);

        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())))
            compilePeepHoleIntegerBranch(node, branchNodeIndex, condition);
        else if (Node::shouldSpeculateNumber(at(node.child1()), at(node.child2())))
            compilePeepHoleDoubleBranch(node, branchNodeIndex, doubleCondition);
        else if (node.op() == CompareEq) {
            if (at(node.child1()).shouldSpeculateString() || at(node.child2()).shouldSpeculateString()) {
                nonSpeculativePeepholeBranch(node, branchNodeIndex, condition, operation);
                return true;
            }
            if (at(node.child1()).shouldSpeculateNonStringCell() && at(node.child2()).shouldSpeculateNonStringCellOrOther())
                compilePeepHoleObjectToObjectOrOtherEquality(node.child1(), node.child2(), branchNodeIndex);
            else if (at(node.child1()).shouldSpeculateNonStringCellOrOther() && at(node.child2()).shouldSpeculateNonStringCell())
                compilePeepHoleObjectToObjectOrOtherEquality(node.child2(), node.child1(), branchNodeIndex);
            else if (at(node.child1()).shouldSpeculateNonStringCell() && at(node.child2()).shouldSpeculateNonStringCell())
                compilePeepHoleObjectEquality(node, branchNodeIndex);
            else {
                nonSpeculativePeepholeBranch(node, branchNodeIndex, condition, operation);
                return true;
            }
        } else {
            nonSpeculativePeepholeBranch(node, branchNodeIndex, condition, operation);
            return true;
        }

        use(node.child1());
        use(node.child2());
        m_indexInBlock = branchIndexInBlock;
        m_compileIndex = branchNodeIndex;
        return true;
    }
    return false;
}

void SpeculativeJIT::noticeOSRBirth(NodeIndex nodeIndex, Node& node)
{
    if (!node.hasVirtualRegister())
        return;
    
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];
    
    info.noticeOSRBirth(*m_stream, nodeIndex, virtualRegister);
}

void SpeculativeJIT::compileMovHint(Node& node)
{
    ASSERT(node.op() == SetLocal);
    
    m_lastSetOperand = node.local();
    
    Node& child = at(node.child1());
    noticeOSRBirth(node.child1().index(), child);
    
    if (child.op() == UInt32ToNumber)
        noticeOSRBirth(child.child1().index(), at(child.child1()));
    
    m_stream->appendAndLog(VariableEvent::movHint(node.child1().index(), node.local()));
}

void SpeculativeJIT::compile(BasicBlock& block)
{
    ASSERT(m_compileOkay);
    
    if (!block.isReachable)
        return;
    
    if (!block.cfaHasVisited) {
        // Don't generate code for basic blocks that are unreachable according to CFA.
        // But to be sure that nobody has generated a jump to this block, drop in a
        // breakpoint here.
#if !ASSERT_DISABLED
        m_jit.breakpoint();
#endif
        return;
    }

    m_blockHeads[m_block] = m_jit.label();
#if DFG_ENABLE(JIT_BREAK_ON_EVERY_BLOCK)
    m_jit.breakpoint();
#endif
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Setting up state for block #%u: ", m_block);
#endif
    
    m_stream->appendAndLog(VariableEvent::reset());
    
    m_jit.jitAssertHasValidCallFrame();

    ASSERT(m_arguments.size() == block.variablesAtHead.numberOfArguments());
    for (size_t i = 0; i < m_arguments.size(); ++i) {
        ValueSource valueSource = ValueSource(ValueInRegisterFile);
        m_arguments[i] = valueSource;
        m_stream->appendAndLog(VariableEvent::setLocal(argumentToOperand(i), valueSource.dataFormat()));
    }
    
    m_state.reset();
    m_state.beginBasicBlock(&block);
    
    ASSERT(m_variables.size() == block.variablesAtHead.numberOfLocals());
    for (size_t i = 0; i < m_variables.size(); ++i) {
        NodeIndex nodeIndex = block.variablesAtHead.local(i);
        ValueSource valueSource;
        if (nodeIndex == NoNode)
            valueSource = ValueSource(SourceIsDead);
        else if (at(nodeIndex).variableAccessData()->isArgumentsAlias())
            valueSource = ValueSource(ArgumentsSource);
        else if (at(nodeIndex).variableAccessData()->isCaptured())
            valueSource = ValueSource(ValueInRegisterFile);
        else if (!at(nodeIndex).refCount())
            valueSource = ValueSource(SourceIsDead);
        else if (at(nodeIndex).variableAccessData()->shouldUseDoubleFormat())
            valueSource = ValueSource(DoubleInRegisterFile);
        else
            valueSource = ValueSource::forSpeculation(at(nodeIndex).variableAccessData()->argumentAwarePrediction());
        m_variables[i] = valueSource;
        m_stream->appendAndLog(VariableEvent::setLocal(i, valueSource.dataFormat()));
    }
    
    m_lastSetOperand = std::numeric_limits<int>::max();
    m_codeOriginForOSR = CodeOrigin();
    
    if (DFG_ENABLE_EDGE_CODE_VERIFICATION) {
        JITCompiler::Jump verificationSucceeded =
            m_jit.branch32(JITCompiler::Equal, GPRInfo::regT0, TrustedImm32(m_block));
        m_jit.breakpoint();
        verificationSucceeded.link(&m_jit);
    }

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("\n");
#endif

    for (m_indexInBlock = 0; m_indexInBlock < block.size(); ++m_indexInBlock) {
        m_compileIndex = block[m_indexInBlock];
        m_jit.setForNode(m_compileIndex);
        Node& node = at(m_compileIndex);
        m_codeOriginForOSR = node.codeOrigin;
        if (!node.shouldGenerate()) {
#if DFG_ENABLE(DEBUG_VERBOSE)
            dataLog("SpeculativeJIT skipping Node @%d (bc#%u) at JIT offset 0x%x     ", (int)m_compileIndex, node.codeOrigin.bytecodeIndex, m_jit.debugOffset());
#endif
            switch (node.op()) {
            case JSConstant:
                m_minifiedGraph->append(MinifiedNode::fromNode(m_compileIndex, node));
                break;
                
            case WeakJSConstant:
                m_jit.addWeakReference(node.weakConstant());
                m_minifiedGraph->append(MinifiedNode::fromNode(m_compileIndex, node));
                break;
                
            case SetLocal:
                compileMovHint(node);
                break;

            case InlineStart: {
                InlineCallFrame* inlineCallFrame = node.codeOrigin.inlineCallFrame;
                int argumentCountIncludingThis = inlineCallFrame->arguments.size();
                unsigned argumentPositionStart = node.argumentPositionStart();
                CodeBlock* codeBlock = baselineCodeBlockForInlineCallFrame(inlineCallFrame);
                for (int i = 0; i < argumentCountIncludingThis; ++i) {
                    ValueRecovery recovery;
                    if (codeBlock->isCaptured(argumentToOperand(i)))
                        recovery = ValueRecovery::alreadyInRegisterFile();
                    else {
                        ArgumentPosition& argumentPosition =
                            m_jit.graph().m_argumentPositions[argumentPositionStart + i];
                        ValueSource valueSource;
                        if (argumentPosition.shouldUseDoubleFormat())
                            valueSource = ValueSource(DoubleInRegisterFile);
                        else if (isInt32Speculation(argumentPosition.prediction()))
                            valueSource = ValueSource(Int32InRegisterFile);
                        else if (isCellSpeculation(argumentPosition.prediction()))
                            valueSource = ValueSource(CellInRegisterFile);
                        else if (isBooleanSpeculation(argumentPosition.prediction()))
                            valueSource = ValueSource(BooleanInRegisterFile);
                        else
                            valueSource = ValueSource(ValueInRegisterFile);
                        recovery = computeValueRecoveryFor(valueSource);
                    }
                    // The recovery should refer either to something that has already been
                    // stored into the register file at the right place, or to a constant,
                    // since the Arguments code isn't smart enough to handle anything else.
                    // The exception is the this argument, which we don't really need to be
                    // able to recover.
#if DFG_ENABLE(DEBUG_VERBOSE)
                    dataLog("\nRecovery for argument %d: ", i);
                    recovery.dump(WTF::dataFile());
#endif
                    inlineCallFrame->arguments[i] = recovery;
                }
                break;
            }
                
            default:
                if (belongsInMinifiedGraph(node.op()))
                    m_minifiedGraph->append(MinifiedNode::fromNode(m_compileIndex, node));
                break;
            }
        } else {
            
#if DFG_ENABLE(DEBUG_VERBOSE)
            dataLog("SpeculativeJIT generating Node @%d (bc#%u) at JIT offset 0x%x   ", (int)m_compileIndex, node.codeOrigin.bytecodeIndex, m_jit.debugOffset());
#endif
#if DFG_ENABLE(JIT_BREAK_ON_EVERY_NODE)
            m_jit.breakpoint();
#endif
#if DFG_ENABLE(XOR_DEBUG_AID)
            m_jit.xorPtr(JITCompiler::TrustedImm32(m_compileIndex), GPRInfo::regT0);
            m_jit.xorPtr(JITCompiler::TrustedImm32(m_compileIndex), GPRInfo::regT0);
#endif
            checkConsistency();
            compile(node);
            if (!m_compileOkay) {
                m_compileOkay = true;
                clearGenerationInfo();
                return;
            }
            
            if (belongsInMinifiedGraph(node.op())) {
                m_minifiedGraph->append(MinifiedNode::fromNode(m_compileIndex, node));
                noticeOSRBirth(m_compileIndex, node);
            }
            
#if DFG_ENABLE(DEBUG_VERBOSE)
            if (node.hasResult()) {
                GenerationInfo& info = m_generationInfo[node.virtualRegister()];
                dataLog("-> %s, vr#%d", dataFormatToString(info.registerFormat()), (int)node.virtualRegister());
                if (info.registerFormat() != DataFormatNone) {
                    if (info.registerFormat() == DataFormatDouble)
                        dataLog(", %s", FPRInfo::debugName(info.fpr()));
#if USE(JSVALUE32_64)
                    else if (info.registerFormat() & DataFormatJS)
                        dataLog(", %s %s", GPRInfo::debugName(info.tagGPR()), GPRInfo::debugName(info.payloadGPR()));
#endif
                    else
                        dataLog(", %s", GPRInfo::debugName(info.gpr()));
                }
                dataLog("    ");
            } else
                dataLog("    ");
#endif
        }
        
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("\n");
#endif
        
        // Make sure that the abstract state is rematerialized for the next node.
        m_state.execute(m_indexInBlock);
        
        if (node.shouldGenerate())
            checkConsistency();
    }
    
    // Perform the most basic verification that children have been used correctly.
#if !ASSERT_DISABLED
    for (unsigned index = 0; index < m_generationInfo.size(); ++index) {
        GenerationInfo& info = m_generationInfo[index];
        ASSERT(!info.alive());
    }
#endif
}

// If we are making type predictions about our arguments then
// we need to check that they are correct on function entry.
void SpeculativeJIT::checkArgumentTypes()
{
    ASSERT(!m_compileIndex);
    m_isCheckingArgumentTypes = true;
    m_codeOriginForOSR = CodeOrigin(0);

    for (size_t i = 0; i < m_arguments.size(); ++i)
        m_arguments[i] = ValueSource(ValueInRegisterFile);
    for (size_t i = 0; i < m_variables.size(); ++i)
        m_variables[i] = ValueSource(ValueInRegisterFile);
    
    for (int i = 0; i < m_jit.codeBlock()->numParameters(); ++i) {
        NodeIndex nodeIndex = m_jit.graph().m_arguments[i];
        Node& node = at(nodeIndex);
        ASSERT(node.op() == SetArgument);
        if (!node.shouldGenerate()) {
            // The argument is dead. We don't do any checks for such arguments.
            continue;
        }
        
        VariableAccessData* variableAccessData = node.variableAccessData();
        VirtualRegister virtualRegister = variableAccessData->local();
        SpeculatedType predictedType = variableAccessData->prediction();

        JSValueSource valueSource = JSValueSource(JITCompiler::addressFor(virtualRegister));
        
#if USE(JSVALUE64)
        if (isInt32Speculation(predictedType))
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branchPtr(MacroAssembler::Below, JITCompiler::addressFor(virtualRegister), GPRInfo::tagTypeNumberRegister));
        else if (isBooleanSpeculation(predictedType)) {
            GPRTemporary temp(this);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), temp.gpr());
            m_jit.xorPtr(TrustedImm32(static_cast<int32_t>(ValueFalse)), temp.gpr());
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branchTestPtr(MacroAssembler::NonZero, temp.gpr(), TrustedImm32(static_cast<int32_t>(~1))));
        } else if (isCellSpeculation(predictedType))
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branchTestPtr(MacroAssembler::NonZero, JITCompiler::addressFor(virtualRegister), GPRInfo::tagMaskRegister));
#else
        if (isInt32Speculation(predictedType))
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag)));
        else if (isBooleanSpeculation(predictedType))
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::BooleanTag)));
        else if (isCellSpeculation(predictedType))
            speculationCheck(BadType, valueSource, nodeIndex, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::CellTag)));
#endif
    }
    m_isCheckingArgumentTypes = false;
}

bool SpeculativeJIT::compile()
{
    checkArgumentTypes();

    if (DFG_ENABLE_EDGE_CODE_VERIFICATION)
        m_jit.move(TrustedImm32(0), GPRInfo::regT0);

    ASSERT(!m_compileIndex);
    for (m_block = 0; m_block < m_jit.graph().m_blocks.size(); ++m_block) {
        m_jit.setForBlock(m_block);
        BasicBlock* block = m_jit.graph().m_blocks[m_block].get();
        if (block)
            compile(*block);
    }
    linkBranches();
    return true;
}

void SpeculativeJIT::createOSREntries()
{
    for (BlockIndex blockIndex = 0; blockIndex < m_jit.graph().m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_jit.graph().m_blocks[blockIndex].get();
        if (!block)
            continue;
        if (!block->isOSRTarget)
            continue;

        // Currently we only need to create OSR entry trampolines when using edge code
        // verification. But in the future, we'll need this for other things as well (like
        // when we have global reg alloc).
        // If we don't need OSR entry trampolin
        if (!DFG_ENABLE_EDGE_CODE_VERIFICATION) {
            m_osrEntryHeads.append(m_blockHeads[blockIndex]);
            continue;
        }
        
        m_osrEntryHeads.append(m_jit.label());
        m_jit.move(TrustedImm32(blockIndex), GPRInfo::regT0);
        m_jit.jump().linkTo(m_blockHeads[blockIndex], &m_jit);
    }
}

void SpeculativeJIT::linkOSREntries(LinkBuffer& linkBuffer)
{
    unsigned osrEntryIndex = 0;
    for (BlockIndex blockIndex = 0; blockIndex < m_jit.graph().m_blocks.size(); ++blockIndex) {
        BasicBlock* block = m_jit.graph().m_blocks[blockIndex].get();
        if (!block)
            continue;
        if (!block->isOSRTarget)
            continue;
        m_jit.noticeOSREntry(*block, m_osrEntryHeads[osrEntryIndex++], linkBuffer);
    }
    ASSERT(osrEntryIndex == m_osrEntryHeads.size());
}

ValueRecovery SpeculativeJIT::computeValueRecoveryFor(const ValueSource& valueSource)
{
    if (valueSource.isInRegisterFile())
        return valueSource.valueRecovery();
        
    ASSERT(valueSource.kind() == HaveNode);
    if (isConstant(valueSource.nodeIndex()))
        return ValueRecovery::constant(valueOfJSConstant(valueSource.nodeIndex()));
    
    return ValueRecovery();
}

void SpeculativeJIT::compileGetCharCodeAt(Node& node)
{
    ASSERT(node.child3() == NoNode);
    SpeculateCellOperand string(this, node.child1());
    SpeculateStrictInt32Operand index(this, node.child2());
    StorageOperand storage(this, node.child3());

    GPRReg stringReg = string.gpr();
    GPRReg indexReg = index.gpr();
    GPRReg storageReg = storage.gpr();
    
    if (!isStringSpeculation(m_state.forNode(node.child1()).m_type)) {
        ASSERT(!(at(node.child1()).prediction() & SpecString));
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), NoNode);
        noResult(m_compileIndex);
        return;
    }

    // unsigned comparison so we can filter out negative indices and indices that are too large
    speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::AboveOrEqual, indexReg, MacroAssembler::Address(stringReg, JSString::offsetOfLength())));

    GPRTemporary scratch(this);
    GPRReg scratchReg = scratch.gpr();

    m_jit.loadPtr(MacroAssembler::Address(stringReg, JSString::offsetOfValue()), scratchReg);

    // Load the character into scratchReg
    JITCompiler::Jump is16Bit = m_jit.branchTest32(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, StringImpl::flagsOffset()), TrustedImm32(StringImpl::flagIs8Bit()));

    m_jit.load8(MacroAssembler::BaseIndex(storageReg, indexReg, MacroAssembler::TimesOne, 0), scratchReg);
    JITCompiler::Jump cont8Bit = m_jit.jump();

    is16Bit.link(&m_jit);

    m_jit.load16(MacroAssembler::BaseIndex(storageReg, indexReg, MacroAssembler::TimesTwo, 0), scratchReg);

    cont8Bit.link(&m_jit);

    integerResult(scratchReg, m_compileIndex);
}

void SpeculativeJIT::compileGetByValOnString(Node& node)
{
    SpeculateCellOperand base(this, node.child1());
    SpeculateStrictInt32Operand property(this, node.child2());
    StorageOperand storage(this, node.child3());
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    ASSERT(modeAlreadyChecked(m_state.forNode(node.child1()), Array::String));

    // unsigned comparison so we can filter out negative indices and indices that are too large
    speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, JSString::offsetOfLength())));

    GPRTemporary scratch(this);
    GPRReg scratchReg = scratch.gpr();

    m_jit.loadPtr(MacroAssembler::Address(baseReg, JSString::offsetOfValue()), scratchReg);

    // Load the character into scratchReg
    JITCompiler::Jump is16Bit = m_jit.branchTest32(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, StringImpl::flagsOffset()), TrustedImm32(StringImpl::flagIs8Bit()));

    m_jit.load8(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne, 0), scratchReg);
    JITCompiler::Jump cont8Bit = m_jit.jump();

    is16Bit.link(&m_jit);

    m_jit.load16(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo, 0), scratchReg);

    // We only support ascii characters
    speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::AboveOrEqual, scratchReg, TrustedImm32(0x100)));

    // 8 bit string values don't need the isASCII check.
    cont8Bit.link(&m_jit);

    GPRTemporary smallStrings(this);
    GPRReg smallStringsReg = smallStrings.gpr();
    m_jit.move(MacroAssembler::TrustedImmPtr(m_jit.globalData()->smallStrings.singleCharacterStrings()), smallStringsReg);
    m_jit.loadPtr(MacroAssembler::BaseIndex(smallStringsReg, scratchReg, MacroAssembler::ScalePtr, 0), scratchReg);
    speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branchTest32(MacroAssembler::Zero, scratchReg));
    cellResult(scratchReg, m_compileIndex);
}

GeneratedOperandType SpeculativeJIT::checkGeneratedTypeForToInt32(NodeIndex nodeIndex)
{
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("checkGeneratedTypeForToInt32@%d   ", nodeIndex);
#endif
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex))
                return GeneratedOperandInteger;

            if (isNumberConstant(nodeIndex))
                return GeneratedOperandDouble;

            terminateSpeculativeExecution(Uncountable, JSValueRegs(), NoNode);
            return GeneratedOperandTypeUnknown;
        }

        if (info.spillFormat() == DataFormatDouble)
            return GeneratedOperandDouble;
    }

    switch (info.registerFormat()) {
    case DataFormatBoolean: // This type never occurs.
    case DataFormatStorage:
        ASSERT_NOT_REACHED();

    case DataFormatCell:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), NoNode);
        return GeneratedOperandTypeUnknown;

    case DataFormatNone:
    case DataFormatJSCell:
    case DataFormatJS:
    case DataFormatJSBoolean:
        return GeneratedOperandJSValue;

    case DataFormatJSInteger:
    case DataFormatInteger:
        return GeneratedOperandInteger;

    case DataFormatJSDouble:
    case DataFormatDouble:
        return GeneratedOperandDouble;
        
    default:
        ASSERT_NOT_REACHED();
        return GeneratedOperandTypeUnknown;
    }
}

void SpeculativeJIT::compileValueToInt32(Node& node)
{
    if (at(node.child1()).shouldSpeculateInteger()) {
        SpeculateIntegerOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        return;
    }
    
    if (at(node.child1()).shouldSpeculateNumber()) {
        switch (checkGeneratedTypeForToInt32(node.child1().index())) {
        case GeneratedOperandInteger: {
            SpeculateIntegerOperand op1(this, node.child1());
            GPRTemporary result(this, op1);
            m_jit.move(op1.gpr(), result.gpr());
            integerResult(result.gpr(), m_compileIndex, op1.format());
            return;
        }
        case GeneratedOperandDouble: {
            GPRTemporary result(this);
            DoubleOperand op1(this, node.child1());
            FPRReg fpr = op1.fpr();
            GPRReg gpr = result.gpr();
            JITCompiler::Jump notTruncatedToInteger = m_jit.branchTruncateDoubleToInt32(fpr, gpr, JITCompiler::BranchIfTruncateFailed);
            
            addSlowPathGenerator(slowPathCall(notTruncatedToInteger, this, toInt32, gpr, fpr));

            integerResult(gpr, m_compileIndex);
            return;
        }
        case GeneratedOperandJSValue: {
            GPRTemporary result(this);
#if USE(JSVALUE64)
            JSValueOperand op1(this, node.child1());

            GPRReg gpr = op1.gpr();
            GPRReg resultGpr = result.gpr();
            FPRTemporary tempFpr(this);
            FPRReg fpr = tempFpr.fpr();

            JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, gpr, GPRInfo::tagTypeNumberRegister);

            if (!isNumberSpeculation(m_state.forNode(node.child1()).m_type))
                speculationCheck(BadType, JSValueRegs(gpr), node.child1().index(), m_jit.branchTestPtr(MacroAssembler::Zero, gpr, GPRInfo::tagTypeNumberRegister));

            // First, if we get here we have a double encoded as a JSValue
            m_jit.move(gpr, resultGpr);
            unboxDouble(resultGpr, fpr);

            silentSpillAllRegisters(resultGpr);
            callOperation(toInt32, resultGpr, fpr);
            silentFillAllRegisters(resultGpr);

            JITCompiler::Jump converted = m_jit.jump();

            isInteger.link(&m_jit);
            m_jit.zeroExtend32ToPtr(gpr, resultGpr);

            converted.link(&m_jit);
#else
            Node& childNode = at(node.child1().index());
            VirtualRegister virtualRegister = childNode.virtualRegister();
            GenerationInfo& info = m_generationInfo[virtualRegister];

            JSValueOperand op1(this, node.child1());

            GPRReg payloadGPR = op1.payloadGPR();
            GPRReg resultGpr = result.gpr();

            if (info.registerFormat() == DataFormatJSInteger)
                m_jit.move(payloadGPR, resultGpr);
            else {
                GPRReg tagGPR = op1.tagGPR();
                FPRTemporary tempFpr(this);
                FPRReg fpr = tempFpr.fpr();
                FPRTemporary scratch(this);

                JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));

                if (!isNumberSpeculation(m_state.forNode(node.child1()).m_type))
                    speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), node.child1().index(), m_jit.branch32(MacroAssembler::AboveOrEqual, tagGPR, TrustedImm32(JSValue::LowestTag)));

                unboxDouble(tagGPR, payloadGPR, fpr, scratch.fpr());

                silentSpillAllRegisters(resultGpr);
                callOperation(toInt32, resultGpr, fpr);
                silentFillAllRegisters(resultGpr);

                JITCompiler::Jump converted = m_jit.jump();

                isInteger.link(&m_jit);
                m_jit.move(payloadGPR, resultGpr);

                converted.link(&m_jit);
            }
#endif
            integerResult(resultGpr, m_compileIndex);
            return;
        }
        case GeneratedOperandTypeUnknown:
            ASSERT_NOT_REACHED();
            break;
        }
    }
    
    if (at(node.child1()).shouldSpeculateBoolean()) {
        SpeculateBooleanOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        
        m_jit.and32(JITCompiler::TrustedImm32(1), op1.gpr());
        
        integerResult(op1.gpr(), m_compileIndex);
        return;
    }
    
    // Do it the safe way.
    nonSpeculativeValueToInt32(node);
    return;
}

void SpeculativeJIT::compileUInt32ToNumber(Node& node)
{
    if (!nodeCanSpeculateInteger(node.arithNodeFlags())) {
        // We know that this sometimes produces doubles. So produce a double every
        // time. This at least allows subsequent code to not have weird conditionals.
            
        IntegerOperand op1(this, node.child1());
        FPRTemporary result(this);
            
        GPRReg inputGPR = op1.gpr();
        FPRReg outputFPR = result.fpr();
            
        m_jit.convertInt32ToDouble(inputGPR, outputFPR);
            
        JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, inputGPR, TrustedImm32(0));
        m_jit.addDouble(JITCompiler::AbsoluteAddress(&AssemblyHelpers::twoToThe32), outputFPR);
        positive.link(&m_jit);
            
        doubleResult(outputFPR, m_compileIndex);
        return;
    }

    IntegerOperand op1(this, node.child1());
    GPRTemporary result(this, op1);

    // Test the operand is positive. This is a very special speculation check - we actually
    // use roll-forward speculation here, where if this fails, we jump to the baseline
    // instruction that follows us, rather than the one we're executing right now. We have
    // to do this because by this point, the original values necessary to compile whatever
    // operation the UInt32ToNumber originated from might be dead.
    forwardSpeculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, op1.gpr(), TrustedImm32(0)), ValueRecovery::uint32InGPR(op1.gpr()));

    m_jit.move(op1.gpr(), result.gpr());
    integerResult(result.gpr(), m_compileIndex, op1.format());
}

void SpeculativeJIT::compileDoubleAsInt32(Node& node)
{
    SpeculateDoubleOperand op1(this, node.child1());
    FPRTemporary scratch(this);
    GPRTemporary result(this);
    
    FPRReg valueFPR = op1.fpr();
    FPRReg scratchFPR = scratch.fpr();
    GPRReg resultGPR = result.gpr();

    JITCompiler::JumpList failureCases;
    m_jit.branchConvertDoubleToInt32(valueFPR, resultGPR, failureCases, scratchFPR);
    forwardSpeculationCheck(Overflow, JSValueRegs(), NoNode, failureCases, ValueRecovery::inFPR(valueFPR));

    integerResult(resultGPR, m_compileIndex);
}

void SpeculativeJIT::compileInt32ToDouble(Node& node)
{
#if USE(JSVALUE64)
    // On JSVALUE64 we have a way of loading double constants in a more direct manner
    // than a int->double conversion. On 32_64, unfortunately, we currently don't have
    // any such mechanism - though we could have it, if we just provisioned some memory
    // in CodeBlock for the double form of integer constants.
    if (at(node.child1()).hasConstant()) {
        ASSERT(isInt32Constant(node.child1().index()));
        FPRTemporary result(this);
        GPRTemporary temp(this);
        m_jit.move(MacroAssembler::ImmPtr(reinterpret_cast<void*>(reinterpretDoubleToIntptr(valueOfNumberConstant(node.child1().index())))), temp.gpr());
        m_jit.movePtrToDouble(temp.gpr(), result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        return;
    }
#endif
    
    if (isInt32Speculation(m_state.forNode(node.child1()).m_type)) {
        SpeculateIntegerOperand op1(this, node.child1());
        FPRTemporary result(this);
        m_jit.convertInt32ToDouble(op1.gpr(), result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        return;
    }
    
    JSValueOperand op1(this, node.child1());
    FPRTemporary result(this);
    
#if USE(JSVALUE64)
    GPRTemporary temp(this);

    GPRReg op1GPR = op1.gpr();
    GPRReg tempGPR = temp.gpr();
    FPRReg resultFPR = result.fpr();
    
    JITCompiler::Jump isInteger = m_jit.branchPtr(
        MacroAssembler::AboveOrEqual, op1GPR, GPRInfo::tagTypeNumberRegister);
    
    if (!isNumberSpeculation(m_state.forNode(node.child1()).m_type)) {
        speculationCheck(
            BadType, JSValueRegs(op1GPR), node.child1(),
            m_jit.branchTestPtr(MacroAssembler::Zero, op1GPR, GPRInfo::tagTypeNumberRegister));
    }
    
    m_jit.move(op1GPR, tempGPR);
    unboxDouble(tempGPR, resultFPR);
    JITCompiler::Jump done = m_jit.jump();
    
    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(op1GPR, resultFPR);
    done.link(&m_jit);
#else
    FPRTemporary temp(this);
    
    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg op1PayloadGPR = op1.payloadGPR();
    FPRReg tempFPR = temp.fpr();
    FPRReg resultFPR = result.fpr();
    
    JITCompiler::Jump isInteger = m_jit.branch32(
        MacroAssembler::Equal, op1TagGPR, TrustedImm32(JSValue::Int32Tag));
    
    if (!isNumberSpeculation(m_state.forNode(node.child1()).m_type)) {
        speculationCheck(
            BadType, JSValueRegs(op1TagGPR, op1PayloadGPR), node.child1(),
            m_jit.branch32(MacroAssembler::AboveOrEqual, op1TagGPR, TrustedImm32(JSValue::LowestTag)));
    }
    
    unboxDouble(op1TagGPR, op1PayloadGPR, resultFPR, tempFPR);
    JITCompiler::Jump done = m_jit.jump();
    
    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(op1PayloadGPR, resultFPR);
    done.link(&m_jit);
#endif
    
    doubleResult(resultFPR, m_compileIndex);
}

static double clampDoubleToByte(double d)
{
    d += 0.5;
    if (!(d > 0))
        d = 0;
    else if (d > 255)
        d = 255;
    return d;
}

static void compileClampIntegerToByte(JITCompiler& jit, GPRReg result)
{
    MacroAssembler::Jump inBounds = jit.branch32(MacroAssembler::BelowOrEqual, result, JITCompiler::TrustedImm32(0xff));
    MacroAssembler::Jump tooBig = jit.branch32(MacroAssembler::GreaterThan, result, JITCompiler::TrustedImm32(0xff));
    jit.xorPtr(result, result);
    MacroAssembler::Jump clamped = jit.jump();
    tooBig.link(&jit);
    jit.move(JITCompiler::TrustedImm32(255), result);
    clamped.link(&jit);
    inBounds.link(&jit);
}

static void compileClampDoubleToByte(JITCompiler& jit, GPRReg result, FPRReg source, FPRReg scratch)
{
    // Unordered compare so we pick up NaN
    static const double zero = 0;
    static const double byteMax = 255;
    static const double half = 0.5;
    jit.loadDouble(&zero, scratch);
    MacroAssembler::Jump tooSmall = jit.branchDouble(MacroAssembler::DoubleLessThanOrEqualOrUnordered, source, scratch);
    jit.loadDouble(&byteMax, scratch);
    MacroAssembler::Jump tooBig = jit.branchDouble(MacroAssembler::DoubleGreaterThan, source, scratch);
    
    jit.loadDouble(&half, scratch);
    // FIXME: This should probably just use a floating point round!
    // https://bugs.webkit.org/show_bug.cgi?id=72054
    jit.addDouble(source, scratch);
    jit.truncateDoubleToInt32(scratch, result);   
    MacroAssembler::Jump truncatedInt = jit.jump();
    
    tooSmall.link(&jit);
    jit.xorPtr(result, result);
    MacroAssembler::Jump zeroed = jit.jump();
    
    tooBig.link(&jit);
    jit.move(JITCompiler::TrustedImm32(255), result);
    
    truncatedInt.link(&jit);
    zeroed.link(&jit);

}

void SpeculativeJIT::compileGetByValOnIntTypedArray(const TypedArrayDescriptor& descriptor, Node& node, size_t elementSize, TypedArraySignedness signedness)
{
    SpeculateCellOperand base(this, node.child1());
    SpeculateStrictInt32Operand property(this, node.child2());
    StorageOperand storage(this, node.child3());

    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    GPRTemporary result(this);
    GPRReg resultReg = result.gpr();

    ASSERT(modeAlreadyChecked(m_state.forNode(node.child1()), node.arrayMode()));

    speculationCheck(
        Uncountable, JSValueRegs(), NoNode,
        m_jit.branch32(
            MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, descriptor.m_lengthOffset)));
    switch (elementSize) {
    case 1:
        if (signedness == SignedTypedArray)
            m_jit.load8Signed(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne), resultReg);
        else
            m_jit.load8(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesOne), resultReg);
        break;
    case 2:
        if (signedness == SignedTypedArray)
            m_jit.load16Signed(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo), resultReg);
        else
            m_jit.load16(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesTwo), resultReg);
        break;
    case 4:
        m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesFour), resultReg);
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    if (elementSize < 4 || signedness == SignedTypedArray) {
        integerResult(resultReg, m_compileIndex);
        return;
    }
    
    ASSERT(elementSize == 4 && signedness == UnsignedTypedArray);
    if (node.shouldSpeculateInteger()) {
        forwardSpeculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, resultReg, TrustedImm32(0)), ValueRecovery::uint32InGPR(resultReg));
        integerResult(resultReg, m_compileIndex);
        return;
    }
    
    FPRTemporary fresult(this);
    m_jit.convertInt32ToDouble(resultReg, fresult.fpr());
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, resultReg, TrustedImm32(0));
    m_jit.addDouble(JITCompiler::AbsoluteAddress(&AssemblyHelpers::twoToThe32), fresult.fpr());
    positive.link(&m_jit);
    doubleResult(fresult.fpr(), m_compileIndex);
}

void SpeculativeJIT::compilePutByValForIntTypedArray(const TypedArrayDescriptor& descriptor, GPRReg base, GPRReg property, Node& node, size_t elementSize, TypedArraySignedness signedness, TypedArrayRounding rounding)
{
    StorageOperand storage(this, m_jit.graph().varArgChild(node, 3));
    GPRReg storageReg = storage.gpr();
    
    Edge valueUse = m_jit.graph().varArgChild(node, 2);
    
    GPRTemporary value;
    GPRReg valueGPR;
    
    if (at(valueUse).isConstant()) {
        JSValue jsValue = valueOfJSConstant(valueUse.index());
        if (!jsValue.isNumber()) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), NoNode);
            noResult(m_compileIndex);
            return;
        }
        double d = jsValue.asNumber();
        if (rounding == ClampRounding) {
            ASSERT(elementSize == 1);
            d = clampDoubleToByte(d);
        }
        GPRTemporary scratch(this);
        GPRReg scratchReg = scratch.gpr();
        m_jit.move(Imm32(toInt32(d)), scratchReg);
        value.adopt(scratch);
        valueGPR = scratchReg;
    } else if (at(valueUse).shouldSpeculateInteger()) {
        SpeculateIntegerOperand valueOp(this, valueUse);
        GPRTemporary scratch(this);
        GPRReg scratchReg = scratch.gpr();
        m_jit.move(valueOp.gpr(), scratchReg);
        if (rounding == ClampRounding) {
            ASSERT(elementSize == 1);
            compileClampIntegerToByte(m_jit, scratchReg);
        }
        value.adopt(scratch);
        valueGPR = scratchReg;
    } else if (rounding == ClampRounding) {
        ASSERT(elementSize == 1);
        SpeculateDoubleOperand valueOp(this, valueUse);
        GPRTemporary result(this);
        FPRTemporary floatScratch(this);
        FPRReg fpr = valueOp.fpr();
        GPRReg gpr = result.gpr();
        compileClampDoubleToByte(m_jit, gpr, fpr, floatScratch.fpr());
        value.adopt(result);
        valueGPR = gpr;
    } else {
        SpeculateDoubleOperand valueOp(this, valueUse);
        GPRTemporary result(this);
        FPRReg fpr = valueOp.fpr();
        GPRReg gpr = result.gpr();
        MacroAssembler::Jump notNaN = m_jit.branchDouble(MacroAssembler::DoubleEqual, fpr, fpr);
        m_jit.xorPtr(gpr, gpr);
        MacroAssembler::Jump fixed = m_jit.jump();
        notNaN.link(&m_jit);

        MacroAssembler::Jump failed;
        if (signedness == SignedTypedArray)
            failed = m_jit.branchTruncateDoubleToInt32(fpr, gpr, MacroAssembler::BranchIfTruncateFailed);
        else
            failed = m_jit.branchTruncateDoubleToUint32(fpr, gpr, MacroAssembler::BranchIfTruncateFailed);
        
        addSlowPathGenerator(slowPathCall(failed, this, toInt32, gpr, fpr));

        fixed.link(&m_jit);
        value.adopt(result);
        valueGPR = gpr;
    }
    ASSERT_UNUSED(valueGPR, valueGPR != property);
    ASSERT(valueGPR != base);
    ASSERT(valueGPR != storageReg);
    MacroAssembler::Jump outOfBounds;
    if (node.op() == PutByVal)
        outOfBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, property, MacroAssembler::Address(base, descriptor.m_lengthOffset));

    switch (elementSize) {
    case 1:
        m_jit.store8(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesOne));
        break;
    case 2:
        m_jit.store16(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesTwo));
        break;
    case 4:
        m_jit.store32(value.gpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesFour));
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    if (node.op() == PutByVal)
        outOfBounds.link(&m_jit);
    noResult(m_compileIndex);
}

void SpeculativeJIT::compileGetByValOnFloatTypedArray(const TypedArrayDescriptor& descriptor, Node& node, size_t elementSize)
{
    SpeculateCellOperand base(this, node.child1());
    SpeculateStrictInt32Operand property(this, node.child2());
    StorageOperand storage(this, node.child3());

    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg storageReg = storage.gpr();

    ASSERT(modeAlreadyChecked(m_state.forNode(node.child1()), node.arrayMode()));

    FPRTemporary result(this);
    FPRReg resultReg = result.fpr();
    speculationCheck(
        Uncountable, JSValueRegs(), NoNode,
        m_jit.branch32(
            MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, descriptor.m_lengthOffset)));
    switch (elementSize) {
    case 4:
        m_jit.loadFloat(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesFour), resultReg);
        m_jit.convertFloatToDouble(resultReg, resultReg);
        break;
    case 8: {
        m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), resultReg);
        MacroAssembler::Jump notNaN = m_jit.branchDouble(MacroAssembler::DoubleEqual, resultReg, resultReg);
        static const double NaN = std::numeric_limits<double>::quiet_NaN();
        m_jit.loadDouble(&NaN, resultReg);
        notNaN.link(&m_jit);
        break;
    }
    default:
        ASSERT_NOT_REACHED();
    }
    doubleResult(resultReg, m_compileIndex);
}

void SpeculativeJIT::compilePutByValForFloatTypedArray(const TypedArrayDescriptor& descriptor, GPRReg base, GPRReg property, Node& node, size_t elementSize)
{
    StorageOperand storage(this, m_jit.graph().varArgChild(node, 3));
    GPRReg storageReg = storage.gpr();
    
    Edge baseUse = m_jit.graph().varArgChild(node, 0);
    Edge valueUse = m_jit.graph().varArgChild(node, 2);
    
    SpeculateDoubleOperand valueOp(this, valueUse);
    
    ASSERT_UNUSED(baseUse, modeAlreadyChecked(m_state.forNode(baseUse), node.arrayMode()));
    
    GPRTemporary result(this);
    
    MacroAssembler::Jump outOfBounds;
    if (node.op() == PutByVal)
        outOfBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, property, MacroAssembler::Address(base, descriptor.m_lengthOffset));
    
    switch (elementSize) {
    case 4: {
        FPRTemporary scratch(this);
        m_jit.moveDouble(valueOp.fpr(), scratch.fpr());
        m_jit.convertDoubleToFloat(valueOp.fpr(), scratch.fpr());
        m_jit.storeFloat(scratch.fpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesFour));
        break;
    }
    case 8:
        m_jit.storeDouble(valueOp.fpr(), MacroAssembler::BaseIndex(storageReg, property, MacroAssembler::TimesEight));
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    if (node.op() == PutByVal)
        outOfBounds.link(&m_jit);
    noResult(m_compileIndex);
}

void SpeculativeJIT::compileInstanceOfForObject(Node&, GPRReg valueReg, GPRReg prototypeReg, GPRReg scratchReg)
{
    // Check that prototype is an object.
    m_jit.loadPtr(MacroAssembler::Address(prototypeReg, JSCell::structureOffset()), scratchReg);
    speculationCheck(BadType, JSValueRegs(), NoNode, m_jit.branchIfNotObject(scratchReg));
    
    // Initialize scratchReg with the value being checked.
    m_jit.move(valueReg, scratchReg);
    
    // Walk up the prototype chain of the value (in scratchReg), comparing to prototypeReg.
    MacroAssembler::Label loop(&m_jit);
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, JSCell::structureOffset()), scratchReg);
#if USE(JSVALUE64)
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, Structure::prototypeOffset()), scratchReg);
#else
    m_jit.load32(MacroAssembler::Address(scratchReg, Structure::prototypeOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), scratchReg);
#endif
    MacroAssembler::Jump isInstance = m_jit.branchPtr(MacroAssembler::Equal, scratchReg, prototypeReg);
#if USE(JSVALUE64)
    m_jit.branchTestPtr(MacroAssembler::Zero, scratchReg, GPRInfo::tagMaskRegister).linkTo(loop, &m_jit);
#else
    m_jit.branchTest32(MacroAssembler::NonZero, scratchReg).linkTo(loop, &m_jit);
#endif
    
    // No match - result is false.
#if USE(JSVALUE64)
    m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsBoolean(false))), scratchReg);
#else
    m_jit.move(MacroAssembler::TrustedImm32(0), scratchReg);
#endif
    MacroAssembler::Jump putResult = m_jit.jump();
    
    isInstance.link(&m_jit);
#if USE(JSVALUE64)
    m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsBoolean(true))), scratchReg);
#else
    m_jit.move(MacroAssembler::TrustedImm32(1), scratchReg);
#endif
    
    putResult.link(&m_jit);
}

void SpeculativeJIT::compileInstanceOf(Node& node)
{
    if ((!!(at(node.child1()).prediction() & ~SpecCell)
         && !!(m_state.forNode(node.child1()).m_type & ~SpecCell))
        || at(node.child1()).adjustedRefCount() == 1) {
        // It might not be a cell. Speculate less aggressively.
        // Or: it might only be used once (i.e. by us), so we get zero benefit
        // from speculating any more aggressively than we absolutely need to.
        
        JSValueOperand value(this, node.child1());
        SpeculateCellOperand prototype(this, node.child3());
        GPRTemporary scratch(this);
        
        GPRReg prototypeReg = prototype.gpr();
        GPRReg scratchReg = scratch.gpr();
        
#if USE(JSVALUE64)
        GPRReg valueReg = value.gpr();
        MacroAssembler::Jump isCell = m_jit.branchTestPtr(MacroAssembler::Zero, valueReg, GPRInfo::tagMaskRegister);
        m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsBoolean(false))), scratchReg);
#else
        GPRReg valueTagReg = value.tagGPR();
        GPRReg valueReg = value.payloadGPR();
        MacroAssembler::Jump isCell = m_jit.branch32(MacroAssembler::Equal, valueTagReg, TrustedImm32(JSValue::CellTag));
        m_jit.move(MacroAssembler::TrustedImm32(0), scratchReg);
#endif

        MacroAssembler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        
        compileInstanceOfForObject(node, valueReg, prototypeReg, scratchReg);
        
        done.link(&m_jit);

#if USE(JSVALUE64)
        jsValueResult(scratchReg, m_compileIndex, DataFormatJSBoolean);
#else
        booleanResult(scratchReg, m_compileIndex);
#endif
        return;
    }
    
    SpeculateCellOperand value(this, node.child1());
    // Base unused since we speculate default InstanceOf behaviour in CheckHasInstance.
    SpeculateCellOperand prototype(this, node.child3());
    
    GPRTemporary scratch(this);
    
    GPRReg valueReg = value.gpr();
    GPRReg prototypeReg = prototype.gpr();
    GPRReg scratchReg = scratch.gpr();
    
    compileInstanceOfForObject(node, valueReg, prototypeReg, scratchReg);

#if USE(JSVALUE64)
    jsValueResult(scratchReg, m_compileIndex, DataFormatJSBoolean);
#else
    booleanResult(scratchReg, m_compileIndex);
#endif
}

void SpeculativeJIT::compileSoftModulo(Node& node)
{
    // In the fast path, the dividend value could be the final result
    // (in case of |dividend| < |divisor|), so we speculate it as strict int32.
    SpeculateStrictInt32Operand op1(this, node.child1());
#if CPU(X86) || CPU(X86_64)
    if (isInt32Constant(node.child2().index())) {
        int32_t divisor = valueOfInt32Constant(node.child2().index());
        if (divisor) {
            GPRReg op1Gpr = op1.gpr();

            GPRTemporary eax(this, X86Registers::eax);
            GPRTemporary edx(this, X86Registers::edx);
            GPRTemporary scratch(this);
            GPRReg scratchGPR = scratch.gpr();

            GPRReg op1SaveGPR;
            if (op1Gpr == X86Registers::eax || op1Gpr == X86Registers::edx) {
                op1SaveGPR = allocate();
                ASSERT(op1Gpr != op1SaveGPR);
                m_jit.move(op1Gpr, op1SaveGPR);
            } else
                op1SaveGPR = op1Gpr;
            ASSERT(op1SaveGPR != X86Registers::eax);
            ASSERT(op1SaveGPR != X86Registers::edx);

            m_jit.move(op1Gpr, eax.gpr());
            m_jit.move(TrustedImm32(divisor), scratchGPR);
            if (divisor == -1)
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branch32(JITCompiler::Equal, eax.gpr(), TrustedImm32(-2147483647-1)));
            m_jit.assembler().cdq();
            m_jit.assembler().idivl_r(scratchGPR);
            // Check that we're not about to create negative zero.
            // FIXME: if the node use doesn't care about neg zero, we can do this more easily.
            JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, op1SaveGPR, TrustedImm32(0));
            speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(JITCompiler::Zero, edx.gpr()));
            numeratorPositive.link(&m_jit);
            
            if (op1SaveGPR != op1Gpr)
                unlock(op1SaveGPR);

            integerResult(edx.gpr(), m_compileIndex);
            return;
        }
    }
#endif

    SpeculateIntegerOperand op2(this, node.child2());
#if CPU(X86) || CPU(X86_64)
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    GPRReg op2TempGPR;
    GPRReg temp;
    GPRReg op1SaveGPR;
    
    if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
        op2TempGPR = allocate();
        temp = op2TempGPR;
    } else {
        op2TempGPR = InvalidGPRReg;
        if (op1GPR == X86Registers::eax)
            temp = X86Registers::edx;
        else
            temp = X86Registers::eax;
    }
    
    if (op1GPR == X86Registers::eax || op1GPR == X86Registers::edx) {
        op1SaveGPR = allocate();
        ASSERT(op1GPR != op1SaveGPR);
        m_jit.move(op1GPR, op1SaveGPR);
    } else
        op1SaveGPR = op1GPR;
    
    ASSERT(temp != op1GPR);
    ASSERT(temp != op2GPR);
    ASSERT(op1SaveGPR != X86Registers::eax);
    ASSERT(op1SaveGPR != X86Registers::edx);
    
    m_jit.add32(JITCompiler::TrustedImm32(1), op2GPR, temp);
    
    JITCompiler::Jump safeDenominator = m_jit.branch32(JITCompiler::Above, temp, JITCompiler::TrustedImm32(1));
    
    JITCompiler::Jump done;
    // FIXME: if the node is not used as number then we can do this more easily.
    speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(JITCompiler::Zero, op2GPR));
    speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branch32(JITCompiler::Equal, op1GPR, TrustedImm32(-2147483647-1)));
    
    safeDenominator.link(&m_jit);
            
    if (op2TempGPR != InvalidGPRReg) {
        m_jit.move(op2GPR, op2TempGPR);
        op2GPR = op2TempGPR;
    }
            
    m_jit.move(op1GPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2GPR);
            
    if (op2TempGPR != InvalidGPRReg)
        unlock(op2TempGPR);

    // Check that we're not about to create negative zero.
    // FIXME: if the node use doesn't care about neg zero, we can do this more easily.
    JITCompiler::Jump numeratorPositive = m_jit.branch32(JITCompiler::GreaterThanOrEqual, op1SaveGPR, TrustedImm32(0));
    speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(JITCompiler::Zero, edx.gpr()));
    numeratorPositive.link(&m_jit);
    
    if (op1SaveGPR != op1GPR)
        unlock(op1SaveGPR);
            
    integerResult(edx.gpr(), m_compileIndex);
#else // CPU(X86) || CPU(X86_64) --> so not X86
    // Do this the *safest* way possible: call out to a C function that will do the modulo,
    // and then attempt to convert back.
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    FPRResult result(this);
    
    flushRegisters();
    callOperation(operationFModOnInts, result.fpr(), op1GPR, op2GPR);
    
    FPRTemporary scratch(this);
    GPRTemporary intResult(this);
    JITCompiler::JumpList failureCases;
    m_jit.branchConvertDoubleToInt32(result.fpr(), intResult.gpr(), failureCases, scratch.fpr());
    speculationCheck(Overflow, JSValueRegs(), NoNode, failureCases);
    
    integerResult(intResult.gpr(), m_compileIndex);
#endif // CPU(X86) || CPU(X86_64)
}

void SpeculativeJIT::compileAdd(Node& node)
{
    if (m_jit.graph().addShouldSpeculateInteger(node)) {
        if (isNumberConstant(node.child1().index())) {
            int32_t imm1 = valueOfNumberConstantAsInt32(node.child1().index());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this);

            if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                m_jit.move(op2.gpr(), result.gpr());
                m_jit.add32(Imm32(imm1), result.gpr());
            } else
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchAdd32(MacroAssembler::Overflow, op2.gpr(), Imm32(imm1), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            return;
        }
                
        if (isNumberConstant(node.child2().index())) {
            SpeculateIntegerOperand op1(this, node.child1());
            int32_t imm2 = valueOfNumberConstantAsInt32(node.child2().index());
            GPRTemporary result(this);
                
            if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                m_jit.move(op1.gpr(), result.gpr());
                m_jit.add32(Imm32(imm2), result.gpr());
            } else
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchAdd32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            return;
        }
                
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        GPRTemporary result(this, op1, op2);

        GPRReg gpr1 = op1.gpr();
        GPRReg gpr2 = op2.gpr();
        GPRReg gprResult = result.gpr();

        if (nodeCanTruncateInteger(node.arithNodeFlags())) {
            if (gpr1 == gprResult)
                m_jit.add32(gpr2, gprResult);
            else {
                m_jit.move(gpr2, gprResult);
                m_jit.add32(gpr1, gprResult);
            }
        } else {
            MacroAssembler::Jump check = m_jit.branchAdd32(MacroAssembler::Overflow, gpr1, gpr2, gprResult);
                
            if (gpr1 == gprResult)
                speculationCheck(Overflow, JSValueRegs(), NoNode, check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr2));
            else if (gpr2 == gprResult)
                speculationCheck(Overflow, JSValueRegs(), NoNode, check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr1));
            else
                speculationCheck(Overflow, JSValueRegs(), NoNode, check);
        }

        integerResult(gprResult, m_compileIndex);
        return;
    }
        
    if (Node::shouldSpeculateNumber(at(node.child1()), at(node.child2()))) {
        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        FPRTemporary result(this, op1, op2);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.addDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        return;
    }

    if (node.op() == ValueAdd) {
        compileValueAdd(node);
        return;
    }
    
    // We don't handle this yet. :-(
    terminateSpeculativeExecution(Uncountable, JSValueRegs(), NoNode);
}

void SpeculativeJIT::compileArithSub(Node& node)
{
    if (m_jit.graph().addShouldSpeculateInteger(node)) {
        if (isNumberConstant(node.child2().index())) {
            SpeculateIntegerOperand op1(this, node.child1());
            int32_t imm2 = valueOfNumberConstantAsInt32(node.child2().index());
            GPRTemporary result(this);

            if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                m_jit.move(op1.gpr(), result.gpr());
                m_jit.sub32(Imm32(imm2), result.gpr());
            } else {
#if ENABLE(JIT_CONSTANT_BLINDING)
                GPRTemporary scratch(this);
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr(), scratch.gpr()));
#else
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));
#endif
            }

            integerResult(result.gpr(), m_compileIndex);
            return;
        }
            
        if (isNumberConstant(node.child1().index())) {
            int32_t imm1 = valueOfNumberConstantAsInt32(node.child1().index());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this);
                
            m_jit.move(Imm32(imm1), result.gpr());
            if (nodeCanTruncateInteger(node.arithNodeFlags()))
                m_jit.sub32(op2.gpr(), result.gpr());
            else
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchSub32(MacroAssembler::Overflow, op2.gpr(), result.gpr()));
                
            integerResult(result.gpr(), m_compileIndex);
            return;
        }
            
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        GPRTemporary result(this);

        if (nodeCanTruncateInteger(node.arithNodeFlags())) {
            m_jit.move(op1.gpr(), result.gpr());
            m_jit.sub32(op2.gpr(), result.gpr());
        } else
            speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), op2.gpr(), result.gpr()));

        integerResult(result.gpr(), m_compileIndex);
        return;
    }
        
    SpeculateDoubleOperand op1(this, node.child1());
    SpeculateDoubleOperand op2(this, node.child2());
    FPRTemporary result(this, op1);

    FPRReg reg1 = op1.fpr();
    FPRReg reg2 = op2.fpr();
    m_jit.subDouble(reg1, reg2, result.fpr());

    doubleResult(result.fpr(), m_compileIndex);
}

void SpeculativeJIT::compileArithNegate(Node& node)
{
    if (m_jit.graph().negateShouldSpeculateInteger(node)) {
        SpeculateIntegerOperand op1(this, node.child1());
        GPRTemporary result(this);

        m_jit.move(op1.gpr(), result.gpr());

        if (nodeCanTruncateInteger(node.arithNodeFlags()))
            m_jit.neg32(result.gpr());
        else {
            speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchNeg32(MacroAssembler::Overflow, result.gpr()));
            if (!nodeCanIgnoreNegativeZero(node.arithNodeFlags()))
                speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(MacroAssembler::Zero, result.gpr()));
        }

        integerResult(result.gpr(), m_compileIndex);
        return;
    }
        
    SpeculateDoubleOperand op1(this, node.child1());
    FPRTemporary result(this);

    m_jit.negateDouble(op1.fpr(), result.fpr());

    doubleResult(result.fpr(), m_compileIndex);
}

void SpeculativeJIT::compileArithMul(Node& node)
{
    if (m_jit.graph().mulShouldSpeculateInteger(node)) {
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        GPRTemporary result(this);

        GPRReg reg1 = op1.gpr();
        GPRReg reg2 = op2.gpr();

        // We can perform truncated multiplications if we get to this point, because if the
        // fixup phase could not prove that it would be safe, it would have turned us into
        // a double multiplication.
        if (nodeCanTruncateInteger(node.arithNodeFlags())) {
            m_jit.move(reg1, result.gpr());
            m_jit.mul32(reg2, result.gpr());
        } else {
            speculationCheck(
                Overflow, JSValueRegs(), NoNode,
                m_jit.branchMul32(MacroAssembler::Overflow, reg1, reg2, result.gpr()));
        }
            
        // Check for negative zero, if the users of this node care about such things.
        if (!nodeCanIgnoreNegativeZero(node.arithNodeFlags())) {
            MacroAssembler::Jump resultNonZero = m_jit.branchTest32(MacroAssembler::NonZero, result.gpr());
            speculationCheck(NegativeZero, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, reg1, TrustedImm32(0)));
            speculationCheck(NegativeZero, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, reg2, TrustedImm32(0)));
            resultNonZero.link(&m_jit);
        }

        integerResult(result.gpr(), m_compileIndex);
        return;
    }

    SpeculateDoubleOperand op1(this, node.child1());
    SpeculateDoubleOperand op2(this, node.child2());
    FPRTemporary result(this, op1, op2);

    FPRReg reg1 = op1.fpr();
    FPRReg reg2 = op2.fpr();
        
    m_jit.mulDouble(reg1, reg2, result.fpr());
        
    doubleResult(result.fpr(), m_compileIndex);
}

#if CPU(X86) || CPU(X86_64)
void SpeculativeJIT::compileIntegerArithDivForX86(Node& node)
{
    SpeculateIntegerOperand op1(this, node.child1());
    SpeculateIntegerOperand op2(this, node.child2());
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    GPRReg op2TempGPR;
    GPRReg temp;
    if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
        op2TempGPR = allocate();
        temp = op2TempGPR;
    } else {
        op2TempGPR = InvalidGPRReg;
        if (op1GPR == X86Registers::eax)
            temp = X86Registers::edx;
        else
            temp = X86Registers::eax;
    }
    
    ASSERT(temp != op1GPR);
    ASSERT(temp != op2GPR);
    
    m_jit.add32(JITCompiler::TrustedImm32(1), op2GPR, temp);
    
    JITCompiler::Jump safeDenominator = m_jit.branch32(JITCompiler::Above, temp, JITCompiler::TrustedImm32(1));
    
    JITCompiler::Jump done;
    if (nodeUsedAsNumber(node.arithNodeFlags())) {
        speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(JITCompiler::Zero, op2GPR));
        speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branch32(JITCompiler::Equal, op1GPR, TrustedImm32(-2147483647-1)));
    } else {
        JITCompiler::Jump zero = m_jit.branchTest32(JITCompiler::Zero, op2GPR);
        JITCompiler::Jump notNeg2ToThe31 = m_jit.branch32(JITCompiler::Equal, op1GPR, TrustedImm32(-2147483647-1));
        zero.link(&m_jit);
        m_jit.move(TrustedImm32(0), eax.gpr());
        done = m_jit.jump();
        notNeg2ToThe31.link(&m_jit);
    }
    
    safeDenominator.link(&m_jit);
            
    // If the user cares about negative zero, then speculate that we're not about
    // to produce negative zero.
    if (!nodeCanIgnoreNegativeZero(node.arithNodeFlags())) {
        MacroAssembler::Jump numeratorNonZero = m_jit.branchTest32(MacroAssembler::NonZero, op1GPR);
        speculationCheck(NegativeZero, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, op2GPR, TrustedImm32(0)));
        numeratorNonZero.link(&m_jit);
    }
    
    if (op2TempGPR != InvalidGPRReg) {
        m_jit.move(op2GPR, op2TempGPR);
        op2GPR = op2TempGPR;
    }
            
    m_jit.move(op1GPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2GPR);
            
    if (op2TempGPR != InvalidGPRReg)
        unlock(op2TempGPR);

    // Check that there was no remainder. If there had been, then we'd be obligated to
    // produce a double result instead.
    if (nodeUsedAsNumber(node.arithNodeFlags()))
        speculationCheck(Overflow, JSValueRegs(), NoNode, m_jit.branchTest32(JITCompiler::NonZero, edx.gpr()));
    else
        done.link(&m_jit);
            
    integerResult(eax.gpr(), m_compileIndex);
}
#endif // CPU(X86) || CPU(X86_64)

void SpeculativeJIT::compileArithMod(Node& node)
{
    if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2()))
        && node.canSpeculateInteger()) {
        compileSoftModulo(node);
        return;
    }
        
    SpeculateDoubleOperand op1(this, node.child1());
    SpeculateDoubleOperand op2(this, node.child2());
        
    FPRReg op1FPR = op1.fpr();
    FPRReg op2FPR = op2.fpr();
        
    flushRegisters();
        
    FPRResult result(this);

    callOperation(fmodAsDFGOperation, result.fpr(), op1FPR, op2FPR);
        
    doubleResult(result.fpr(), m_compileIndex);
}

// Returns true if the compare is fused with a subsequent branch.
bool SpeculativeJIT::compare(Node& node, MacroAssembler::RelationalCondition condition, MacroAssembler::DoubleCondition doubleCondition, S_DFGOperation_EJJ operation)
{
    if (compilePeepHoleBranch(node, condition, doubleCondition, operation))
        return true;

    if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2()))) {
        compileIntegerCompare(node, condition);
        return false;
    }
    
    if (Node::shouldSpeculateNumber(at(node.child1()), at(node.child2()))) {
        compileDoubleCompare(node, doubleCondition);
        return false;
    }
    
    if (node.op() == CompareEq) {
        if (at(node.child1()).shouldSpeculateString() || at(node.child2()).shouldSpeculateString()) {
            nonSpeculativeNonPeepholeCompare(node, condition, operation);
            return false;
        }
        
        if (at(node.child1()).shouldSpeculateNonStringCell() && at(node.child2()).shouldSpeculateNonStringCellOrOther()) {
            compileObjectToObjectOrOtherEquality(node.child1(), node.child2());
            return false;
        }
        
        if (at(node.child1()).shouldSpeculateNonStringCellOrOther() && at(node.child2()).shouldSpeculateNonStringCell()) {
            compileObjectToObjectOrOtherEquality(node.child2(), node.child1());
            return false;
        }

        if (at(node.child1()).shouldSpeculateNonStringCell() && at(node.child2()).shouldSpeculateNonStringCell()) {
            compileObjectEquality(node);
            return false;
        }
    }
    
    nonSpeculativeNonPeepholeCompare(node, condition, operation);
    return false;
}

bool SpeculativeJIT::compileStrictEqForConstant(Node& node, Edge value, JSValue constant)
{
    JSValueOperand op1(this, value);
    
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
        Node& branchNode = at(branchNodeIndex);
        BlockIndex taken = branchNode.takenBlockIndex();
        BlockIndex notTaken = branchNode.notTakenBlockIndex();
        MacroAssembler::RelationalCondition condition = MacroAssembler::Equal;
        
        // The branch instruction will branch to the taken block.
        // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
        if (taken == nextBlock()) {
            condition = MacroAssembler::NotEqual;
            BlockIndex tmp = taken;
            taken = notTaken;
            notTaken = tmp;
        }

#if USE(JSVALUE64)
        branchPtr(condition, op1.gpr(), MacroAssembler::TrustedImmPtr(bitwise_cast<void*>(JSValue::encode(constant))), taken);
#else
        GPRReg payloadGPR = op1.payloadGPR();
        GPRReg tagGPR = op1.tagGPR();
        if (condition == MacroAssembler::Equal) {
            // Drop down if not equal, go elsewhere if equal.
            MacroAssembler::Jump notEqual = m_jit.branch32(MacroAssembler::NotEqual, tagGPR, MacroAssembler::Imm32(constant.tag()));
            branch32(MacroAssembler::Equal, payloadGPR, MacroAssembler::Imm32(constant.payload()), taken);
            notEqual.link(&m_jit);
        } else {
            // Drop down if equal, go elsehwere if not equal.
            branch32(MacroAssembler::NotEqual, tagGPR, MacroAssembler::Imm32(constant.tag()), taken);
            branch32(MacroAssembler::NotEqual, payloadGPR, MacroAssembler::Imm32(constant.payload()), taken);
        }
#endif
        
        jump(notTaken);
        
        use(node.child1());
        use(node.child2());
        m_indexInBlock = branchIndexInBlock;
        m_compileIndex = branchNodeIndex;
        return true;
    }
    
    GPRTemporary result(this);
    
#if USE(JSVALUE64)
    GPRReg op1GPR = op1.gpr();
    GPRReg resultGPR = result.gpr();
    m_jit.move(MacroAssembler::TrustedImmPtr(bitwise_cast<void*>(ValueFalse)), resultGPR);
    MacroAssembler::Jump notEqual = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, MacroAssembler::TrustedImmPtr(bitwise_cast<void*>(JSValue::encode(constant))));
    m_jit.or32(MacroAssembler::TrustedImm32(1), resultGPR);
    notEqual.link(&m_jit);
    jsValueResult(resultGPR, m_compileIndex, DataFormatJSBoolean);
#else
    GPRReg op1PayloadGPR = op1.payloadGPR();
    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg resultGPR = result.gpr();
    m_jit.move(TrustedImm32(0), resultGPR);
    MacroAssembler::JumpList notEqual;
    notEqual.append(m_jit.branch32(MacroAssembler::NotEqual, op1TagGPR, MacroAssembler::Imm32(constant.tag())));
    notEqual.append(m_jit.branch32(MacroAssembler::NotEqual, op1PayloadGPR, MacroAssembler::Imm32(constant.payload())));
    m_jit.move(TrustedImm32(1), resultGPR);
    notEqual.link(&m_jit);
    booleanResult(resultGPR, m_compileIndex);
#endif
    
    return false;
}

bool SpeculativeJIT::compileStrictEq(Node& node)
{
    // 1) If either operand is a constant and that constant is not a double, integer,
    //    or string, then do a JSValue comparison.
    
    if (isJSConstant(node.child1().index())) {
        JSValue value = valueOfJSConstant(node.child1().index());
        if (!value.isNumber() && !value.isString())
            return compileStrictEqForConstant(node, node.child2(), value);
    }
    
    if (isJSConstant(node.child2().index())) {
        JSValue value = valueOfJSConstant(node.child2().index());
        if (!value.isNumber() && !value.isString())
            return compileStrictEqForConstant(node, node.child1(), value);
    }
    
    // 2) If the operands are predicted integer, do an integer comparison.
    
    if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2()))) {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleIntegerBranch(node, branchNodeIndex, MacroAssembler::Equal);
            use(node.child1());
            use(node.child2());
            m_indexInBlock = branchIndexInBlock;
            m_compileIndex = branchNodeIndex;
            return true;
        }
        compileIntegerCompare(node, MacroAssembler::Equal);
        return false;
    }
    
    // 3) If the operands are predicted double, do a double comparison.
    
    if (Node::shouldSpeculateNumber(at(node.child1()), at(node.child2()))) {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleDoubleBranch(node, branchNodeIndex, MacroAssembler::DoubleEqual);
            use(node.child1());
            use(node.child2());
            m_indexInBlock = branchIndexInBlock;
            m_compileIndex = branchNodeIndex;
            return true;
        }
        compileDoubleCompare(node, MacroAssembler::DoubleEqual);
        return false;
    }
    
    if (at(node.child1()).shouldSpeculateString() || at(node.child2()).shouldSpeculateString())
        return nonSpeculativeStrictEq(node);
    if (at(node.child1()).shouldSpeculateNonStringCell() && at(node.child2()).shouldSpeculateNonStringCell()) {
        unsigned branchIndexInBlock = detectPeepHoleBranch();
        if (branchIndexInBlock != UINT_MAX) {
            NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
            compilePeepHoleObjectEquality(node, branchNodeIndex);
            use(node.child1());
            use(node.child2());
            m_indexInBlock = branchIndexInBlock;
            m_compileIndex = branchNodeIndex;
            return true;
        }
        compileObjectEquality(node);
        return false;
    }
    
    // 5) Fall back to non-speculative strict equality.
    
    return nonSpeculativeStrictEq(node);
}

void SpeculativeJIT::compileGetIndexedPropertyStorage(Node& node)
{
    SpeculateCellOperand base(this, node.child1());
    GPRReg baseReg = base.gpr();
    
    GPRTemporary storage(this);
    GPRReg storageReg = storage.gpr();
    
    const TypedArrayDescriptor* descriptor = typedArrayDescriptor(node.arrayMode());
    
    switch (node.arrayMode()) {
    case Array::String:
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSString::offsetOfValue()), storageReg);
        
        // Speculate that we're not accessing a rope
        speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branchTest32(MacroAssembler::Zero, storageReg));

        m_jit.loadPtr(MacroAssembler::Address(storageReg, StringImpl::dataOffset()), storageReg);
        break;
        
    default:
        ASSERT(descriptor);
        m_jit.loadPtr(MacroAssembler::Address(baseReg, descriptor->m_storageOffset), storageReg);
        break;
    }
    
    storageResult(storageReg, m_compileIndex);
}

void SpeculativeJIT::compileGetByValOnArguments(Node& node)
{
    SpeculateCellOperand base(this, node.child1());
    SpeculateStrictInt32Operand property(this, node.child2());
    GPRTemporary result(this);
#if USE(JSVALUE32_64)
    GPRTemporary resultTag(this);
#endif
    GPRTemporary scratch(this);
    
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    GPRReg resultReg = result.gpr();
#if USE(JSVALUE32_64)
    GPRReg resultTagReg = resultTag.gpr();
#endif
    GPRReg scratchReg = scratch.gpr();
    
    if (!m_compileOkay)
        return;
  
    ASSERT(modeAlreadyChecked(m_state.forNode(node.child1()), Array::Arguments));
    
    // Two really lame checks.
    speculationCheck(
        Uncountable, JSValueSource(), NoNode,
        m_jit.branchPtr(
            MacroAssembler::AboveOrEqual, propertyReg,
            MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_numArguments))));
    speculationCheck(
        Uncountable, JSValueSource(), NoNode,
        m_jit.branchTestPtr(
            MacroAssembler::NonZero,
            MacroAssembler::Address(
                baseReg, OBJECT_OFFSETOF(Arguments, m_slowArguments))));
    
    m_jit.move(propertyReg, resultReg);
    m_jit.neg32(resultReg);
    m_jit.signExtend32ToPtr(resultReg, resultReg);
    m_jit.loadPtr(
        MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_registers)),
        scratchReg);
    
#if USE(JSVALUE32_64)
    m_jit.load32(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register) +
            OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
        resultTagReg);
    m_jit.load32(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register) +
            OBJECT_OFFSETOF(JSValue, u.asBits.payload)),
        resultReg);
    jsValueResult(resultTagReg, resultReg, m_compileIndex);
#else
    m_jit.loadPtr(
        MacroAssembler::BaseIndex(
            scratchReg, resultReg, MacroAssembler::TimesEight,
            CallFrame::thisArgumentOffset() * sizeof(Register) - sizeof(Register)),
        resultReg);
    jsValueResult(resultReg, m_compileIndex);
#endif
}

void SpeculativeJIT::compileGetArgumentsLength(Node& node)
{
    SpeculateCellOperand base(this, node.child1());
    GPRTemporary result(this, base);
    
    GPRReg baseReg = base.gpr();
    GPRReg resultReg = result.gpr();
    
    if (!m_compileOkay)
        return;
    
    ASSERT(modeAlreadyChecked(m_state.forNode(node.child1()), Array::Arguments));
    
    speculationCheck(
        Uncountable, JSValueSource(), NoNode,
        m_jit.branchTest8(
            MacroAssembler::NonZero,
            MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_overrodeLength))));
    
    m_jit.load32(
        MacroAssembler::Address(baseReg, OBJECT_OFFSETOF(Arguments, m_numArguments)),
        resultReg);
    integerResult(resultReg, m_compileIndex);
}

void SpeculativeJIT::compileGetArrayLength(Node& node)
{
    const TypedArrayDescriptor* descriptor = typedArrayDescriptor(node.arrayMode());

    switch (node.arrayMode()) {
    case ARRAY_WITH_ARRAY_STORAGE_MODES: {
        StorageOperand storage(this, node.child2());
        GPRTemporary result(this, storage);
        GPRReg storageReg = storage.gpr();
        GPRReg resultReg = result.gpr();
        m_jit.load32(MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()), resultReg);
            
        speculationCheck(Uncountable, JSValueRegs(), NoNode, m_jit.branch32(MacroAssembler::LessThan, resultReg, MacroAssembler::TrustedImm32(0)));
            
        integerResult(resultReg, m_compileIndex);
        break;
    }
    case Array::String: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary result(this, base);
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        m_jit.load32(MacroAssembler::Address(baseGPR, JSString::offsetOfLength()), resultGPR);
        integerResult(resultGPR, m_compileIndex);
        break;
    }
    case Array::Arguments: {
        compileGetArgumentsLength(node);
        break;
    }
    default:
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary result(this, base);
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        ASSERT(descriptor);
        m_jit.load32(MacroAssembler::Address(baseGPR, descriptor->m_lengthOffset), resultGPR);
        integerResult(resultGPR, m_compileIndex);
        break;
    }
}

void SpeculativeJIT::compileNewFunctionNoCheck(Node& node)
{
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();
    flushRegisters();
    callOperation(
        operationNewFunction, resultGPR, m_jit.codeBlock()->functionDecl(node.functionDeclIndex()));
    cellResult(resultGPR, m_compileIndex);
}

void SpeculativeJIT::compileNewFunctionExpression(Node& node)
{
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();
    flushRegisters();
    callOperation(
        operationNewFunctionExpression,
        resultGPR,
        m_jit.codeBlock()->functionExpr(node.functionExprIndex()));
    cellResult(resultGPR, m_compileIndex);
}

bool SpeculativeJIT::compileRegExpExec(Node& node)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock == UINT_MAX)
        return false;
    NodeIndex branchNodeIndex = m_jit.graph().m_blocks[m_block]->at(branchIndexInBlock);
    ASSERT(node.adjustedRefCount() == 1);

    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = branchNode.takenBlockIndex();
    BlockIndex notTaken = branchNode.notTakenBlockIndex();
    
    bool invert = false;
    if (taken == nextBlock()) {
        invert = true;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    SpeculateCellOperand base(this, node.child1());
    SpeculateCellOperand argument(this, node.child2());
    GPRReg baseGPR = base.gpr();
    GPRReg argumentGPR = argument.gpr();
    
    flushRegisters();
    GPRResult result(this);
    callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);

    branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, result.gpr(), taken);
    jump(notTaken);

    use(node.child1());
    use(node.child2());
    m_indexInBlock = branchIndexInBlock;
    m_compileIndex = branchNodeIndex;

    return true;
}

void SpeculativeJIT::compileAllocatePropertyStorage(Node& node)
{
    if (hasIndexingHeader(node.structureTransitionData().previousStructure->indexingType())) {
        SpeculateCellOperand base(this, node.child1());
        
        GPRReg baseGPR = base.gpr();
        
        flushRegisters();

        GPRResult result(this);
        callOperation(operationReallocateButterflyToHavePropertyStorageWithInitialCapacity, result.gpr(), baseGPR);
        
        storageResult(result.gpr(), m_compileIndex);
        return;
    }
    
    SpeculateCellOperand base(this, node.child1());
    GPRTemporary scratch(this);
        
    GPRReg baseGPR = base.gpr();
    GPRReg scratchGPR = scratch.gpr();
        
    ASSERT(!node.structureTransitionData().previousStructure->outOfLineCapacity());
    ASSERT(initialOutOfLineCapacity == node.structureTransitionData().newStructure->outOfLineCapacity());
    size_t newSize = initialOutOfLineCapacity * sizeof(JSValue);
    CopiedAllocator* copiedAllocator = &m_jit.globalData()->heap.storageAllocator();

    m_jit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR);
    JITCompiler::Jump slowPath = m_jit.branchSubPtr(JITCompiler::Signed, JITCompiler::TrustedImm32(newSize), scratchGPR);
    m_jit.storePtr(scratchGPR, &copiedAllocator->m_currentRemaining);
    m_jit.negPtr(scratchGPR);
    m_jit.addPtr(JITCompiler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR);
    m_jit.addPtr(JITCompiler::TrustedImm32(sizeof(JSValue)), scratchGPR);
        
    addSlowPathGenerator(
        slowPathCall(slowPath, this, operationAllocatePropertyStorageWithInitialCapacity, scratchGPR));
        
    m_jit.storePtr(scratchGPR, JITCompiler::Address(baseGPR, JSObject::butterflyOffset()));
        
    storageResult(scratchGPR, m_compileIndex);
}

void SpeculativeJIT::compileReallocatePropertyStorage(Node& node)
{
    size_t oldSize = node.structureTransitionData().previousStructure->outOfLineCapacity() * sizeof(JSValue);
    size_t newSize = oldSize * outOfLineGrowthFactor;
    ASSERT(newSize == node.structureTransitionData().newStructure->outOfLineCapacity() * sizeof(JSValue));

    if (hasIndexingHeader(node.structureTransitionData().previousStructure->indexingType())) {
        SpeculateCellOperand base(this, node.child1());
        
        GPRReg baseGPR = base.gpr();
        
        flushRegisters();

        GPRResult result(this);
        callOperation(operationReallocateButterflyToGrowPropertyStorage, result.gpr(), baseGPR, newSize / sizeof(JSValue));
        
        storageResult(result.gpr(), m_compileIndex);
        return;
    }
    
    SpeculateCellOperand base(this, node.child1());
    StorageOperand oldStorage(this, node.child2());
    GPRTemporary scratch1(this);
    GPRTemporary scratch2(this);
        
    GPRReg baseGPR = base.gpr();
    GPRReg oldStorageGPR = oldStorage.gpr();
    GPRReg scratchGPR1 = scratch1.gpr();
    GPRReg scratchGPR2 = scratch2.gpr();
        
    JITCompiler::Jump slowPath;
        
    CopiedAllocator* copiedAllocator = &m_jit.globalData()->heap.storageAllocator();
        
    m_jit.loadPtr(&copiedAllocator->m_currentRemaining, scratchGPR2);
    slowPath = m_jit.branchSubPtr(JITCompiler::Signed, JITCompiler::TrustedImm32(newSize), scratchGPR2);
    m_jit.storePtr(scratchGPR2, &copiedAllocator->m_currentRemaining);
    m_jit.negPtr(scratchGPR2);
    m_jit.addPtr(JITCompiler::AbsoluteAddress(&copiedAllocator->m_currentPayloadEnd), scratchGPR2);
    m_jit.addPtr(JITCompiler::TrustedImm32(sizeof(JSValue)), scratchGPR2);
        
    addSlowPathGenerator(
        slowPathCall(slowPath, this, operationAllocatePropertyStorage, scratchGPR2, newSize / sizeof(JSValue)));
    // We have scratchGPR2 = new storage, scratchGPR1 = scratch
    for (ptrdiff_t offset = 0; offset < static_cast<ptrdiff_t>(oldSize); offset += sizeof(void*)) {
        m_jit.loadPtr(JITCompiler::Address(oldStorageGPR, -(offset + sizeof(JSValue) + sizeof(void*))), scratchGPR1);
        m_jit.storePtr(scratchGPR1, JITCompiler::Address(scratchGPR2, -(offset + sizeof(JSValue) + sizeof(void*))));
    }
    m_jit.storePtr(scratchGPR2, JITCompiler::Address(baseGPR, JSObject::butterflyOffset()));
    
    storageResult(scratchGPR2, m_compileIndex);
}

} } // namespace JSC::DFG

#endif
