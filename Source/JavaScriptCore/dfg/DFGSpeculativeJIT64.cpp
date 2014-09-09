/*
 * Copyright (C) 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
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
#include "ArrayPrototype.h"
#include "DFGAbstractInterpreterInlines.h"
#include "DFGCallArrayAllocatorSlowPathGenerator.h"
#include "DFGOperations.h"
#include "DFGSlowPathGenerator.h"
#include "Debugger.h"
#include "GetterSetter.h"
#include "JSCInlines.h"
#include "ObjectPrototype.h"
#include "SpillRegistersMode.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE64)

void SpeculativeJIT::boxInt52(GPRReg sourceGPR, GPRReg targetGPR, DataFormat format)
{
    GPRReg tempGPR;
    if (sourceGPR == targetGPR)
        tempGPR = allocate();
    else
        tempGPR = targetGPR;
    
    FPRReg fpr = fprAllocate();

    if (format == DataFormatInt52)
        m_jit.rshift64(TrustedImm32(JSValue::int52ShiftAmount), sourceGPR);
    else
        ASSERT(format == DataFormatStrictInt52);
    
    m_jit.boxInt52(sourceGPR, targetGPR, tempGPR, fpr);
    
    if (format == DataFormatInt52 && sourceGPR != targetGPR)
        m_jit.lshift64(TrustedImm32(JSValue::int52ShiftAmount), sourceGPR);
    
    if (tempGPR != targetGPR)
        unlock(tempGPR);
    
    unlock(fpr);
}

GPRReg SpeculativeJIT::fillJSValue(Edge edge)
{
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
            info.fillJSValue(*m_stream, gpr, DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
        } else {
            DataFormat spillFormat = info.spillFormat();
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            switch (spillFormat) {
            case DataFormatInt32: {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
                spillFormat = DataFormatJSInt32;
                break;
            }
                
            default:
                m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
                DFG_ASSERT(m_jit.graph(), m_currentNode, spillFormat & DataFormatJS);
                break;
            }
            info.fillJSValue(*m_stream, gpr, spillFormat);
        }
        return gpr;
    }

    case DataFormatInt32: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInt32, not DataFormatJSInt32.
        if (m_gprs.isLocked(gpr)) {
            GPRReg result = allocate();
            m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr, result);
            return result;
        }
        m_gprs.lock(gpr);
        m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
        info.fillJSValue(*m_stream, gpr, DataFormatJSInt32);
        return gpr;
    }

    case DataFormatCell:
        // No retag required on JSVALUE64!
    case DataFormatJS:
    case DataFormatJSInt32:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
        
    case DataFormatBoolean:
    case DataFormatStorage:
    case DataFormatDouble:
    case DataFormatInt52:
        // this type currently never occurs
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad data format");
        
    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Corrupt data format");
        return InvalidGPRReg;
    }
}

void SpeculativeJIT::cachedGetById(CodeOrigin codeOrigin, GPRReg baseGPR, GPRReg resultGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    JITGetByIdGenerator gen(
        m_jit.codeBlock(), codeOrigin, usedRegisters(), JSValueRegs(baseGPR),
        JSValueRegs(resultGPR), spillMode);
    gen.generateFastPath(m_jit);
    
    JITCompiler::JumpList slowCases;
    if (slowPathTarget.isSet())
        slowCases.append(slowPathTarget);
    slowCases.append(gen.slowPathJump());
    
    OwnPtr<SlowPathGenerator> slowPath = slowPathCall(
        slowCases, this, operationGetByIdOptimize, resultGPR, gen.stubInfo(), baseGPR,
        identifierUID(identifierNumber), spillMode);
    
    m_jit.addGetById(gen, slowPath.get());
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::cachedPutById(CodeOrigin codeOrigin, GPRReg baseGPR, GPRReg valueGPR, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    JITPutByIdGenerator gen(
        m_jit.codeBlock(), codeOrigin, usedRegisters(), JSValueRegs(baseGPR),
        JSValueRegs(valueGPR), scratchGPR, spillMode, m_jit.ecmaModeFor(codeOrigin), putKind);

    gen.generateFastPath(m_jit);
    
    JITCompiler::JumpList slowCases;
    if (slowPathTarget.isSet())
        slowCases.append(slowPathTarget);
    slowCases.append(gen.slowPathJump());
    
    OwnPtr<SlowPathGenerator> slowPath = slowPathCall(
        slowCases, this, gen.slowPathFunction(), NoResult, gen.stubInfo(), valueGPR, baseGPR,
        identifierUID(identifierNumber));

    m_jit.addPutById(gen, slowPath.get());
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, Reuse, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    JITCompiler::Jump notMasqueradesAsUndefined;
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(JSValueRegs(argGPR));

        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultGPR);
        notMasqueradesAsUndefined = m_jit.jump();
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);
        GPRTemporary scratch(this);

        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(JSValueRegs(argGPR));
        
        JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(
            JITCompiler::NonZero, 
            JITCompiler::Address(argGPR, JSCell::typeInfoFlagsOffset()), 
            JITCompiler::TrustedImm32(MasqueradesAsUndefined));

        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultGPR);
        notMasqueradesAsUndefined = m_jit.jump();

        isMasqueradesAsUndefined.link(&m_jit);
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic)), localGlobalObjectGPR);
        m_jit.emitLoadStructure(argGPR, resultGPR, scratch.gpr());
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        m_jit.comparePtr(invert ? JITCompiler::NotEqual : JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, resultGPR);
    }
 
    if (!isKnownCell(operand.node())) {
        JITCompiler::Jump done = m_jit.jump();
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.and64(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        m_jit.compare64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm32(ValueNull), resultGPR);
        
        done.link(&m_jit);
    }
   
    notMasqueradesAsUndefined.link(&m_jit);
 
    m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::nonSpeculativePeepholeBranchNull(Edge operand, Node* branchNode, bool invert)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;
    
    if (taken == nextBlock()) {
        invert = !invert;
        BasicBlock* tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg(this, operand);
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, Reuse, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(JSValueRegs(argGPR));
        
        jump(invert ? taken : notTaken, ForceJump);
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);
        GPRTemporary scratch(this);

        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(JSValueRegs(argGPR));
        
        branchTest8(JITCompiler::Zero, 
            JITCompiler::Address(argGPR, JSCell::typeInfoFlagsOffset()), 
            JITCompiler::TrustedImm32(MasqueradesAsUndefined), 
            invert ? taken : notTaken);

        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic)), localGlobalObjectGPR);
        m_jit.emitLoadStructure(argGPR, resultGPR, scratch.gpr());
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        branchPtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, invert ? notTaken : taken);
    }
 
    if (!isKnownCell(operand.node())) {
        jump(notTaken, ForceJump);
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.and64(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        branch64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm64(ValueNull), taken);
    }
    
    jump(notTaken);
}

bool SpeculativeJIT::nonSpeculativeCompareNull(Node* node, Edge operand, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_block->at(branchIndexInBlock);

        DFG_ASSERT(m_jit.graph(), node, node->adjustedRefCount() == 1);
        
        nonSpeculativePeepholeBranchNull(operand, branchNode, invert);
    
        use(node->child1());
        use(node->child2());
        m_indexInBlock = branchIndexInBlock;
        m_currentNode = branchNode;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeCompareNull(operand, invert);
    
    return false;
}

void SpeculativeJIT::nonSpeculativePeepholeBranch(Node* node, Node* branchNode, MacroAssembler::RelationalCondition cond, S_JITOperation_EJJ helperFunction)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;

    JITCompiler::ResultCondition callResultCondition = JITCompiler::NonZero;

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        cond = JITCompiler::invert(cond);
        callResultCondition = JITCompiler::Zero;
        BasicBlock* tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);

        branchTest32(callResultCondition, resultGPR, taken);
    } else {
        GPRTemporary result(this, Reuse, arg2);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        branch32(cond, arg1GPR, arg2GPR, taken);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            jump(notTaken, ForceJump);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
            silentFillAllRegisters(resultGPR);
        
            branchTest32(callResultCondition, resultGPR, taken);
        }
    }

    jump(notTaken);

    m_indexInBlock = m_block->size() - 1;
    m_currentNode = branchNode;
}

template<typename JumpType>
class CompareAndBoxBooleanSlowPathGenerator
    : public CallSlowPathGenerator<JumpType, S_JITOperation_EJJ, GPRReg> {
public:
    CompareAndBoxBooleanSlowPathGenerator(
        JumpType from, SpeculativeJIT* jit,
        S_JITOperation_EJJ function, GPRReg result, GPRReg arg1, GPRReg arg2)
        : CallSlowPathGenerator<JumpType, S_JITOperation_EJJ, GPRReg>(
            from, jit, function, NeedToSpill, result)
        , m_arg1(arg1)
        , m_arg2(arg2)
    {
    }
    
protected:
    virtual void generateInternal(SpeculativeJIT* jit) override
    {
        this->setUp(jit);
        this->recordCall(jit->callOperation(this->m_function, this->m_result, m_arg1, m_arg2));
        jit->m_jit.and32(JITCompiler::TrustedImm32(1), this->m_result);
        jit->m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), this->m_result);
        this->tearDown(jit);
    }
   
private:
    GPRReg m_arg1;
    GPRReg m_arg2;
};

void SpeculativeJIT::nonSpeculativeNonPeepholeCompare(Node* node, MacroAssembler::RelationalCondition cond, S_JITOperation_EJJ helperFunction)
{
    ASSERT(node->isBinaryUseKind(UntypedUse));
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
        
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary result(this, Reuse, arg2);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch64(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        m_jit.compare32(cond, arg1GPR, arg2GPR, resultGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            addSlowPathGenerator(adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<JITCompiler::JumpList>(
                    slowPath, this, helperFunction, resultGPR, arg1GPR, arg2GPR)));
        }

        jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    }
}

void SpeculativeJIT::nonSpeculativePeepholeStrictEq(Node* node, Node* branchNode, bool invert)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        invert = !invert;
        BasicBlock* tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }
    
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        branch64(JITCompiler::Equal, arg1GPR, arg2GPR, invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEqCell, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultGPR, taken);
    } else {
        m_jit.or64(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTest64(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump leftOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg1GPR, GPRInfo::tagTypeNumberRegister);
        JITCompiler::Jump leftDouble = m_jit.branchTest64(JITCompiler::NonZero, arg1GPR, GPRInfo::tagTypeNumberRegister);
        leftOK.link(&m_jit);
        JITCompiler::Jump rightOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg2GPR, GPRInfo::tagTypeNumberRegister);
        JITCompiler::Jump rightDouble = m_jit.branchTest64(JITCompiler::NonZero, arg2GPR, GPRInfo::tagTypeNumberRegister);
        rightOK.link(&m_jit);
        
        branch64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR, taken);
        jump(notTaken, ForceJump);
        
        twoCellsCase.link(&m_jit);
        branch64(JITCompiler::Equal, arg1GPR, arg2GPR, invert ? notTaken : taken);
        
        leftDouble.link(&m_jit);
        rightDouble.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEq, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultGPR, taken);
    }
    
    jump(notTaken);
}

void SpeculativeJIT::nonSpeculativeNonPeepholeStrictEq(Node* node, bool invert)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        // FIXME: this should flush registers instead of silent spill/fill.
        JITCompiler::Jump notEqualCase = m_jit.branch64(JITCompiler::NotEqual, arg1GPR, arg2GPR);
        
        m_jit.move(JITCompiler::TrustedImm64(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCompareStrictEqCell, resultGPR, arg1GPR, arg2GPR);
        silentFillAllRegisters(resultGPR);
        
        m_jit.and64(JITCompiler::TrustedImm32(1), resultGPR);
        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        done.link(&m_jit);
    } else {
        m_jit.or64(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::JumpList slowPathCases;
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTest64(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump leftOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg1GPR, GPRInfo::tagTypeNumberRegister);
        slowPathCases.append(m_jit.branchTest64(JITCompiler::NonZero, arg1GPR, GPRInfo::tagTypeNumberRegister));
        leftOK.link(&m_jit);
        JITCompiler::Jump rightOK = m_jit.branch64(JITCompiler::AboveOrEqual, arg2GPR, GPRInfo::tagTypeNumberRegister);
        slowPathCases.append(m_jit.branchTest64(JITCompiler::NonZero, arg2GPR, GPRInfo::tagTypeNumberRegister));
        rightOK.link(&m_jit);
        
        m_jit.compare64(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR, resultGPR);
        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();
        
        twoCellsCase.link(&m_jit);
        slowPathCases.append(m_jit.branch64(JITCompiler::NotEqual, arg1GPR, arg2GPR));
        
        m_jit.move(JITCompiler::TrustedImm64(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        addSlowPathGenerator(
            adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<MacroAssembler::JumpList>(
                    slowPathCases, this, operationCompareStrictEq, resultGPR, arg1GPR,
                    arg2GPR)));
        
        done.link(&m_jit);
    }
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean, UseChildrenCalledExplicitly);
}

void SpeculativeJIT::compileMiscStrictEq(Node* node)
{
    JSValueOperand op1(this, node->child1(), ManualOperandSpeculation);
    JSValueOperand op2(this, node->child2(), ManualOperandSpeculation);
    GPRTemporary result(this);
    
    if (node->child1().useKind() == MiscUse)
        speculateMisc(node->child1(), op1.jsValueRegs());
    if (node->child2().useKind() == MiscUse)
        speculateMisc(node->child2(), op2.jsValueRegs());
    
    m_jit.compare64(JITCompiler::Equal, op1.gpr(), op2.gpr(), result.gpr());
    m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
    jsValueResult(result.gpr(), node, DataFormatJSBoolean);
}

void SpeculativeJIT::emitCall(Node* node)
{

    bool isCall = node->op() == Call;
    if (!isCall)
        DFG_ASSERT(m_jit.graph(), node, node->op() == Construct);
    
    // For constructors, the this argument is not passed but we have to make space
    // for it.
    int dummyThisArgument = isCall ? 0 : 1;
    
    CallLinkInfo::CallType callType = isCall ? CallLinkInfo::Call : CallLinkInfo::Construct;
    
    Edge calleeEdge = m_jit.graph().m_varArgChildren[node->firstChild()];
    JSValueOperand callee(this, calleeEdge);
    GPRReg calleeGPR = callee.gpr();
    use(calleeEdge);
    
    // The call instruction's first child is the function; the subsequent children are the
    // arguments.
    int numPassedArgs = node->numChildren() - 1;
    
    int numArgs = numPassedArgs + dummyThisArgument;
    
    m_jit.store32(MacroAssembler::TrustedImm32(numArgs), calleeFramePayloadSlot(JSStack::ArgumentCount));
    m_jit.store64(calleeGPR, calleeFrameSlot(JSStack::Callee));
    
    for (int i = 0; i < numPassedArgs; i++) {
        Edge argEdge = m_jit.graph().m_varArgChildren[node->firstChild() + 1 + i];
        JSValueOperand arg(this, argEdge);
        GPRReg argGPR = arg.gpr();
        use(argEdge);
        
        m_jit.store64(argGPR, calleeArgumentSlot(i + dummyThisArgument));
    }

    flushRegisters();

    GPRResult result(this);
    GPRReg resultGPR = result.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::Jump slowPath;

    m_jit.emitStoreCodeOrigin(node->origin.semantic);
    
    slowPath = m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleeGPR, targetToCheck, MacroAssembler::TrustedImmPtr(0));

    m_jit.loadPtr(MacroAssembler::Address(calleeGPR, OBJECT_OFFSETOF(JSFunction, m_scope)), resultGPR);
    m_jit.store64(resultGPR, calleeFrameSlot(JSStack::ScopeChain));

    JITCompiler::Call fastCall = m_jit.nearCall();

    JITCompiler::Jump done = m_jit.jump();
    
    slowPath.link(&m_jit);
    
    m_jit.move(calleeGPR, GPRInfo::regT0); // Callee needs to be in regT0
    CallLinkInfo* callLinkInfo = m_jit.codeBlock()->addCallLinkInfo();
    m_jit.move(MacroAssembler::TrustedImmPtr(callLinkInfo), GPRInfo::regT2); // Link info needs to be in regT2
    JITCompiler::Call slowCall = m_jit.nearCall();
    
    done.link(&m_jit);
    
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJS, UseChildrenCalledExplicitly);
    
    callLinkInfo->callType = callType;
    callLinkInfo->codeOrigin = m_currentNode->origin.semantic;
    callLinkInfo->calleeGPR = calleeGPR;
    
    m_jit.addJSCall(fastCall, slowCall, targetToCheck, callLinkInfo);
}

// Clang should allow unreachable [[clang::fallthrough]] in template functions if any template expansion uses it
// http://llvm.org/bugs/show_bug.cgi?id=18619
#if COMPILER(CLANG) && defined(__has_warning)
#pragma clang diagnostic push
#if __has_warning("-Wimplicit-fallthrough")
#pragma clang diagnostic ignored "-Wimplicit-fallthrough"
#endif
#endif
template<bool strict>
GPRReg SpeculativeJIT::fillSpeculateInt32Internal(Edge edge, DataFormat& returnFormat)
{
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    ASSERT(edge.useKind() != KnownInt32Use || !(value.m_type & ~SpecInt32));
    m_interpreter.filter(value, SpecInt32);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    if (edge->hasConstant() && !edge->isInt32Constant()) {
        // Protect the silent spill/fill logic by failing early. If we "speculate" on
        // the constant then the silent filler may think that we have an int32 and a
        // constant, so it will try to fill this as an int32 constant. Bad things will
        // happen.
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInt32;
        return allocate();
    }
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            ASSERT(edge->isInt32Constant());
            m_jit.move(MacroAssembler::Imm32(edge->asInt32()), gpr);
            info.fillInt32(*m_stream, gpr);
            returnFormat = DataFormatInt32;
            return gpr;
        }
        
        DataFormat spillFormat = info.spillFormat();
        
        DFG_ASSERT(m_jit.graph(), m_currentNode, (spillFormat & DataFormatJS) || spillFormat == DataFormatInt32);
        
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        
        if (spillFormat == DataFormatJSInt32 || spillFormat == DataFormatInt32) {
            // If we know this was spilled as an integer we can fill without checking.
            if (strict) {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                info.fillInt32(*m_stream, gpr);
                returnFormat = DataFormatInt32;
                return gpr;
            }
            if (spillFormat == DataFormatInt32) {
                m_jit.load32(JITCompiler::addressFor(virtualRegister), gpr);
                m_jit.or64(GPRInfo::tagTypeNumberRegister, gpr);
            } else
                m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(*m_stream, gpr, DataFormatJSInt32);
            returnFormat = DataFormatJSInt32;
            return gpr;
        }
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        // Fill as JSValue, and fall through.
        info.fillJSValue(*m_stream, gpr, DataFormatJSInt32);
        m_gprs.unlock(gpr);
        FALLTHROUGH;
    }

    case DataFormatJS: {
        DFG_ASSERT(m_jit.graph(), m_currentNode, !(type & SpecInt52));
        // Check the value is an integer.
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branch64(MacroAssembler::Below, gpr, GPRInfo::tagTypeNumberRegister));
        info.fillJSValue(*m_stream, gpr, DataFormatJSInt32);
        // If !strict we're done, return.
        if (!strict) {
            returnFormat = DataFormatJSInt32;
            return gpr;
        }
        // else fall through & handle as DataFormatJSInt32.
        m_gprs.unlock(gpr);
        FALLTHROUGH;
    }

    case DataFormatJSInt32: {
        // In a strict fill we need to strip off the value tag.
        if (strict) {
            GPRReg gpr = info.gpr();
            GPRReg result;
            // If the register has already been locked we need to take a copy.
            // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInt32, not DataFormatJSInt32.
            if (m_gprs.isLocked(gpr))
                result = allocate();
            else {
                m_gprs.lock(gpr);
                info.fillInt32(*m_stream, gpr);
                result = gpr;
            }
            m_jit.zeroExtend32ToPtr(gpr, result);
            returnFormat = DataFormatInt32;
            return result;
        }

        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatJSInt32;
        return gpr;
    }

    case DataFormatInt32: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatInt32;
        return gpr;
    }
        
    case DataFormatJSDouble:
    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInt32;
        return allocate();
    }

    case DataFormatDouble:
    case DataFormatStorage:
    case DataFormatInt52:
    case DataFormatStrictInt52:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad data format");
        
    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Corrupt data format");
        return InvalidGPRReg;
    }
}
#if COMPILER(CLANG) && defined(__has_warning)
#pragma clang diagnostic pop
#endif

GPRReg SpeculativeJIT::fillSpeculateInt32(Edge edge, DataFormat& returnFormat)
{
    return fillSpeculateInt32Internal<false>(edge, returnFormat);
}

GPRReg SpeculativeJIT::fillSpeculateInt32Strict(Edge edge)
{
    DataFormat mustBeDataFormatInt32;
    GPRReg result = fillSpeculateInt32Internal<true>(edge, mustBeDataFormatInt32);
    DFG_ASSERT(m_jit.graph(), m_currentNode, mustBeDataFormatInt32 == DataFormatInt32);
    return result;
}

GPRReg SpeculativeJIT::fillSpeculateInt52(Edge edge, DataFormat desiredFormat)
{
    ASSERT(desiredFormat == DataFormatInt52 || desiredFormat == DataFormatStrictInt52);
    AbstractValue& value = m_state.forNode(edge);
    m_interpreter.filter(value, SpecMachineInt);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (edge->hasConstant() && !edge->isMachineIntConstant()) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return allocate();
        }
        
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            ASSERT(jsValue.isMachineInt());
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            int64_t value = jsValue.asMachineInt();
            if (desiredFormat == DataFormatInt52)
                value = value << JSValue::int52ShiftAmount;
            m_jit.move(MacroAssembler::Imm64(value), gpr);
            info.fillGPR(*m_stream, gpr, desiredFormat);
            return gpr;
        }
        
        DataFormat spillFormat = info.spillFormat();
        
        DFG_ASSERT(m_jit.graph(), m_currentNode, spillFormat == DataFormatInt52 || spillFormat == DataFormatStrictInt52);
        
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);
        if (desiredFormat == DataFormatStrictInt52) {
            if (spillFormat == DataFormatInt52)
                m_jit.rshift64(TrustedImm32(JSValue::int52ShiftAmount), gpr);
            info.fillStrictInt52(*m_stream, gpr);
            return gpr;
        }
        if (spillFormat == DataFormatStrictInt52)
            m_jit.lshift64(TrustedImm32(JSValue::int52ShiftAmount), gpr);
        info.fillInt52(*m_stream, gpr);
        return gpr;
    }

    case DataFormatStrictInt52: {
        GPRReg gpr = info.gpr();
        bool wasLocked = m_gprs.isLocked(gpr);
        lock(gpr);
        if (desiredFormat == DataFormatStrictInt52)
            return gpr;
        if (wasLocked) {
            GPRReg result = allocate();
            m_jit.move(gpr, result);
            unlock(gpr);
            gpr = result;
        } else
            info.fillInt52(*m_stream, gpr);
        m_jit.lshift64(TrustedImm32(JSValue::int52ShiftAmount), gpr);
        return gpr;
    }
        
    case DataFormatInt52: {
        GPRReg gpr = info.gpr();
        bool wasLocked = m_gprs.isLocked(gpr);
        lock(gpr);
        if (desiredFormat == DataFormatInt52)
            return gpr;
        if (wasLocked) {
            GPRReg result = allocate();
            m_jit.move(gpr, result);
            unlock(gpr);
            gpr = result;
        } else
            info.fillStrictInt52(*m_stream, gpr);
        m_jit.rshift64(TrustedImm32(JSValue::int52ShiftAmount), gpr);
        return gpr;
    }

    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad data format");
        return InvalidGPRReg;
    }
}

FPRReg SpeculativeJIT::fillSpeculateDouble(Edge edge)
{
    ASSERT(edge.useKind() == DoubleRepUse || edge.useKind() == DoubleRepRealUse || edge.useKind() == DoubleRepMachineIntUse);
    ASSERT(edge->hasDoubleResult());
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    if (info.registerFormat() == DataFormatNone) {
        if (edge->hasConstant()) {
            GPRReg gpr = allocate();

            if (edge->isNumberConstant()) {
                FPRReg fpr = fprAllocate();
                m_jit.move(MacroAssembler::Imm64(reinterpretDoubleToInt64(edge->asNumber())), gpr);
                m_jit.move64ToDouble(gpr, fpr);
                unlock(gpr);

                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(*m_stream, fpr);
                return fpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return fprAllocate();
        }
        
        DataFormat spillFormat = info.spillFormat();
        DFG_ASSERT(m_jit.graph(), m_currentNode, spillFormat == DataFormatDouble);
        FPRReg fpr = fprAllocate();
        m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(*m_stream, fpr);
        return fpr;
    }

    DFG_ASSERT(m_jit.graph(), m_currentNode, info.registerFormat() == DataFormatDouble);
    FPRReg fpr = info.fpr();
    m_fprs.lock(fpr);
    return fpr;
}

GPRReg SpeculativeJIT::fillSpeculateCell(Edge edge)
{
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    ASSERT((edge.useKind() != KnownCellUse && edge.useKind() != KnownStringUse) || !(value.m_type & ~SpecCell));
    m_interpreter.filter(value, SpecCell);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    if (edge->hasConstant() && !edge->isCellConstant()) {
        // Better to fail early on constants.
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
    }

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
            info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
            return gpr;
        }
        
        if (!(info.spillFormat() & DataFormatJS)) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }
        
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        info.fillJSValue(*m_stream, gpr, DataFormatJS);
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueRegs(gpr), edge, branchNotCell(JSValueRegs(gpr)));
        info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
        return gpr;
    }

    case DataFormatCell:
    case DataFormatJSCell: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (!ASSERT_DISABLED) {
            MacroAssembler::Jump checkCell = branchIsCell(JSValueRegs(gpr));
            m_jit.abortWithReason(DFGIsNotCell);
            checkCell.link(&m_jit);
        }
        return gpr;
    }

    case DataFormatJS: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (type & ~SpecCell)
            speculationCheck(BadType, JSValueRegs(gpr), edge, branchNotCell(JSValueRegs(gpr)));
        info.fillJSValue(*m_stream, gpr, DataFormatJSCell);
        return gpr;
    }

    case DataFormatJSInt32:
    case DataFormatInt32:
    case DataFormatJSDouble:
    case DataFormatJSBoolean:
    case DataFormatBoolean: {
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
    }

    case DataFormatDouble:
    case DataFormatStorage:
    case DataFormatInt52:
    case DataFormatStrictInt52:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad data format");
        
    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Corrupt data format");
        return InvalidGPRReg;
    }
}

GPRReg SpeculativeJIT::fillSpeculateBoolean(Edge edge)
{
    AbstractValue& value = m_state.forNode(edge);
    SpeculatedType type = value.m_type;
    m_interpreter.filter(value, SpecBoolean);
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatInt32) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return allocate();
        }
        
        GPRReg gpr = allocate();

        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            if (jsValue.isBoolean()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImm64(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }
        DFG_ASSERT(m_jit.graph(), m_currentNode, info.spillFormat() & DataFormatJS);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        m_jit.load64(JITCompiler::addressFor(virtualRegister), gpr);

        info.fillJSValue(*m_stream, gpr, DataFormatJS);
        if (type & ~SpecBoolean) {
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, TrustedImm32(static_cast<int32_t>(~1))), SpeculationRecovery(BooleanSpeculationCheck, gpr, InvalidGPRReg));
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
        }
        info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
        return gpr;
    }

    case DataFormatBoolean:
    case DataFormatJSBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJS: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        if (type & ~SpecBoolean) {
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
            speculationCheck(BadType, JSValueRegs(gpr), edge, m_jit.branchTest64(MacroAssembler::NonZero, gpr, TrustedImm32(static_cast<int32_t>(~1))), SpeculationRecovery(BooleanSpeculationCheck, gpr, InvalidGPRReg));
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), gpr);
        }
        info.fillJSValue(*m_stream, gpr, DataFormatJSBoolean);
        return gpr;
    }

    case DataFormatJSInt32:
    case DataFormatInt32:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatCell:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
        
    case DataFormatDouble:
    case DataFormatStorage:
    case DataFormatInt52:
    case DataFormatStrictInt52:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad data format");
        
    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Corrupt data format");
        return InvalidGPRReg;
    }
}

void SpeculativeJIT::compileBaseValueStoreBarrier(Edge& baseEdge, Edge& valueEdge)
{
#if ENABLE(GGC)
    ASSERT(!isKnownNotCell(valueEdge.node()));

    SpeculateCellOperand base(this, baseEdge);
    JSValueOperand value(this, valueEdge);
    GPRTemporary scratch1(this);
    GPRTemporary scratch2(this);

    writeBarrier(base.gpr(), value.gpr(), valueEdge, scratch1.gpr(), scratch2.gpr());
#else
    UNUSED_PARAM(baseEdge);
    UNUSED_PARAM(valueEdge);
#endif
}

void SpeculativeJIT::compileObjectEquality(Node* node)
{
    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    GPRTemporary result(this, Reuse, op1);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();
   
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node->child1(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));

        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node->child2(),
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    MacroAssembler::Jump falseCase = m_jit.branch64(MacroAssembler::NotEqual, op1GPR, op2GPR);
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    done.link(&m_jit);

    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild)
{
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();

    bool masqueradesAsUndefinedWatchpointValid =
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell = branchNotCell(JSValueRegs(op2GPR));
    
    // We know that within this branch, rightChild must be a cell. 
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueRegs(op2GPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    MacroAssembler::Jump falseCase = m_jit.branch64(MacroAssembler::NotEqual, op1GPR, op2GPR);
    MacroAssembler::Jump trueCase = m_jit.jump();
    
    rightNotCell.link(&m_jit);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (needsTypeCheck(rightChild, SpecCell | SpecOther)) {
        m_jit.move(op2GPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        
        typeCheck(
            JSValueRegs(op2GPR), rightChild, SpecCell | SpecOther,
            m_jit.branch64(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    trueCase.link(&m_jit);
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    done.link(&m_jit);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compilePeepHoleObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild, Node* branchNode)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;
    
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultGPR = result.gpr();
    
    bool masqueradesAsUndefinedWatchpointValid = 
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild, 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }

    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell = branchNotCell(JSValueRegs(op2GPR));
    
    // We know that within this branch, rightChild must be a cell. 
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(op2GPR), rightChild, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));
        speculationCheck(BadType, JSValueRegs(op2GPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    branch64(MacroAssembler::Equal, op1GPR, op2GPR, taken);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (!needsTypeCheck(rightChild, SpecCell | SpecOther))
        rightNotCell.link(&m_jit);
    else {
        jump(notTaken, ForceJump);
        
        rightNotCell.link(&m_jit);
        m_jit.move(op2GPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        
        typeCheck(
            JSValueRegs(op2GPR), rightChild, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    
    jump(notTaken);
}

void SpeculativeJIT::compileInt32Compare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateInt32Operand op1(this, node->child1());
    SpeculateInt32Operand op2(this, node->child2());
    GPRTemporary result(this, Reuse, op1, op2);
    
    m_jit.compare32(condition, op1.gpr(), op2.gpr(), result.gpr());
    
    // If we add a DataFormatBool, we should use it here.
    m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
    jsValueResult(result.gpr(), m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileInt52Compare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateWhicheverInt52Operand op1(this, node->child1());
    SpeculateWhicheverInt52Operand op2(this, node->child2(), op1);
    GPRTemporary result(this, Reuse, op1, op2);
    
    m_jit.compare64(condition, op1.gpr(), op2.gpr(), result.gpr());
    
    // If we add a DataFormatBool, we should use it here.
    m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
    jsValueResult(result.gpr(), m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compilePeepHoleInt52Branch(Node* node, Node* branchNode, JITCompiler::RelationalCondition condition)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == nextBlock()) {
        condition = JITCompiler::invert(condition);
        BasicBlock* tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }
    
    SpeculateWhicheverInt52Operand op1(this, node->child1());
    SpeculateWhicheverInt52Operand op2(this, node->child2(), op1);
    
    branch64(condition, op1.gpr(), op2.gpr(), taken);
    jump(notTaken);
}

void SpeculativeJIT::compileDoubleCompare(Node* node, MacroAssembler::DoubleCondition condition)
{
    SpeculateDoubleOperand op1(this, node->child1());
    SpeculateDoubleOperand op2(this, node->child2());
    GPRTemporary result(this);
    
    m_jit.move(TrustedImm32(ValueTrue), result.gpr());
    MacroAssembler::Jump trueCase = m_jit.branchDouble(condition, op1.fpr(), op2.fpr());
    m_jit.xor64(TrustedImm32(true), result.gpr());
    trueCase.link(&m_jit);
    
    jsValueResult(result.gpr(), node, DataFormatJSBoolean);
}

void SpeculativeJIT::compileObjectOrOtherLogicalNot(Edge nodeUse)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary result(this);
    GPRReg valueGPR = value.gpr();
    GPRReg resultGPR = result.gpr();
    GPRTemporary structure;
    GPRReg structureGPR = InvalidGPRReg;
    GPRTemporary scratch;
    GPRReg scratchGPR = InvalidGPRReg;

    bool masqueradesAsUndefinedWatchpointValid =
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (!masqueradesAsUndefinedWatchpointValid) {
        // The masquerades as undefined case will use the structure register, so allocate it here.
        // Do this at the top of the function to avoid branching around a register allocation.
        GPRTemporary realStructure(this);
        GPRTemporary realScratch(this);
        structure.adopt(realStructure);
        scratch.adopt(realScratch);
        structureGPR = structure.gpr();
        scratchGPR = scratch.gpr();
    }

    MacroAssembler::Jump notCell = branchNotCell(JSValueRegs(valueGPR));
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(valueGPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(valueGPR, JSCell::structureIDOffset()), 
                m_jit.vm()->stringStructure.get()));

        MacroAssembler::Jump isNotMasqueradesAsUndefined = 
            m_jit.branchTest8(
                MacroAssembler::Zero, 
                MacroAssembler::Address(valueGPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined));

        m_jit.emitLoadStructure(valueGPR, structureGPR, scratchGPR);
        speculationCheck(BadType, JSValueRegs(valueGPR), nodeUse, 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(structureGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    m_jit.move(TrustedImm32(ValueFalse), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    
    notCell.link(&m_jit);

    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueGPR, resultGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), resultGPR);
        typeCheck(
            JSValueRegs(valueGPR), nodeUse, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, 
                resultGPR, 
                MacroAssembler::TrustedImm64(ValueNull)));
    }
    m_jit.move(TrustedImm32(ValueTrue), resultGPR);
    
    done.link(&m_jit);
    
    jsValueResult(resultGPR, m_currentNode, DataFormatJSBoolean);
}

void SpeculativeJIT::compileLogicalNot(Node* node)
{
    switch (node->child1().useKind()) {
    case ObjectOrOtherUse: {
        compileObjectOrOtherLogicalNot(node->child1());
        return;
    }
        
    case Int32Use: {
        SpeculateInt32Operand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        m_jit.compare32(MacroAssembler::Equal, value.gpr(), MacroAssembler::TrustedImm32(0), result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
        
    case DoubleRepUse: {
        SpeculateDoubleOperand value(this, node->child1());
        FPRTemporary scratch(this);
        GPRTemporary result(this);
        m_jit.move(TrustedImm32(ValueFalse), result.gpr());
        MacroAssembler::Jump nonZero = m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr());
        m_jit.xor32(TrustedImm32(true), result.gpr());
        nonZero.link(&m_jit);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
    
    case BooleanUse: {
        if (!needsTypeCheck(node->child1(), SpecBoolean)) {
            SpeculateBooleanOperand value(this, node->child1());
            GPRTemporary result(this, Reuse, value);
            
            m_jit.move(value.gpr(), result.gpr());
            m_jit.xor64(TrustedImm32(true), result.gpr());
            
            jsValueResult(result.gpr(), node, DataFormatJSBoolean);
            return;
        }
        
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRTemporary result(this); // FIXME: We could reuse, but on speculation fail would need recovery to restore tag (akin to add).
        
        m_jit.move(value.gpr(), result.gpr());
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), result.gpr());
        typeCheck(
            JSValueRegs(value.gpr()), node->child1(), SpecBoolean, m_jit.branchTest64(
                JITCompiler::NonZero, result.gpr(), TrustedImm32(static_cast<int32_t>(~1))));
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueTrue)), result.gpr());
        
        // If we add a DataFormatBool, we should use it here.
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        return;
    }
        
    case UntypedUse: {
        JSValueOperand arg1(this, node->child1());
        GPRTemporary result(this);
    
        GPRReg arg1GPR = arg1.gpr();
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
    
        m_jit.move(arg1GPR, resultGPR);
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), resultGPR);
        JITCompiler::Jump slowCase = m_jit.branchTest64(JITCompiler::NonZero, resultGPR, TrustedImm32(static_cast<int32_t>(~1)));
    
        addSlowPathGenerator(
            slowPathCall(slowCase, this, operationConvertJSValueToBoolean, resultGPR, arg1GPR));
    
        m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueTrue)), resultGPR);
        jsValueResult(resultGPR, node, DataFormatJSBoolean, UseChildrenCalledExplicitly);
        return;
    }
    case StringUse:
        return compileStringZeroLength(node);

    default:
        DFG_CRASH(m_jit.graph(), node, "Bad use kind");
        break;
    }
}

void SpeculativeJIT::emitObjectOrOtherBranch(Edge nodeUse, BasicBlock* taken, BasicBlock* notTaken)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary scratch(this);
    GPRTemporary structure;
    GPRReg valueGPR = value.gpr();
    GPRReg scratchGPR = scratch.gpr();
    GPRReg structureGPR = InvalidGPRReg;

    if (!masqueradesAsUndefinedWatchpointIsStillValid()) {
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }

    MacroAssembler::Jump notCell = branchNotCell(JSValueRegs(valueGPR));
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(valueGPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));                
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(valueGPR), nodeUse, (~SpecCell) | SpecObject, m_jit.branchStructurePtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(valueGPR, JSCell::structureIDOffset()),
                m_jit.vm()->stringStructure.get()));

        JITCompiler::Jump isNotMasqueradesAsUndefined = m_jit.branchTest8(
            JITCompiler::Zero, 
            MacroAssembler::Address(valueGPR, JSCell::typeInfoFlagsOffset()), 
            TrustedImm32(MasqueradesAsUndefined));

        m_jit.emitLoadStructure(valueGPR, structureGPR, scratchGPR);
        speculationCheck(BadType, JSValueRegs(valueGPR), nodeUse,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(structureGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    jump(taken, ForceJump);
    
    notCell.link(&m_jit);
    
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.move(valueGPR, scratchGPR);
        m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), scratchGPR);
        typeCheck(
            JSValueRegs(valueGPR), nodeUse, SpecCell | SpecOther, m_jit.branch64(
                MacroAssembler::NotEqual, scratchGPR, MacroAssembler::TrustedImm64(ValueNull)));
    }
    jump(notTaken);
    
    noResult(m_currentNode);
}

void SpeculativeJIT::emitBranch(Node* node)
{
    BasicBlock* taken = node->branchData()->taken.block;
    BasicBlock* notTaken = node->branchData()->notTaken.block;
    
    switch (node->child1().useKind()) {
    case ObjectOrOtherUse: {
        emitObjectOrOtherBranch(node->child1(), taken, notTaken);
        return;
    }
        
    case Int32Use:
    case DoubleRepUse: {
        if (node->child1().useKind() == Int32Use) {
            bool invert = false;
            
            if (taken == nextBlock()) {
                invert = true;
                BasicBlock* tmp = taken;
                taken = notTaken;
                notTaken = tmp;
            }

            SpeculateInt32Operand value(this, node->child1());
            branchTest32(invert ? MacroAssembler::Zero : MacroAssembler::NonZero, value.gpr(), taken);
        } else {
            SpeculateDoubleOperand value(this, node->child1());
            FPRTemporary scratch(this);
            branchDoubleNonZero(value.fpr(), scratch.fpr(), taken);
        }
        
        jump(notTaken);
        
        noResult(node);
        return;
    }

    case UntypedUse:
    case BooleanUse: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg valueGPR = value.gpr();
        
        if (node->child1().useKind() == BooleanUse) {
            if (!needsTypeCheck(node->child1(), SpecBoolean)) {
                MacroAssembler::ResultCondition condition = MacroAssembler::NonZero;
                
                if (taken == nextBlock()) {
                    condition = MacroAssembler::Zero;
                    BasicBlock* tmp = taken;
                    taken = notTaken;
                    notTaken = tmp;
                }
                
                branchTest32(condition, valueGPR, TrustedImm32(true), taken);
                jump(notTaken);
            } else {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), notTaken);
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(true))), taken);
                
                typeCheck(JSValueRegs(valueGPR), node->child1(), SpecBoolean, m_jit.jump());
            }
            value.use();
        } else {
            GPRTemporary result(this);
            GPRReg resultGPR = result.gpr();
            
            if (node->child1()->prediction() & SpecInt32) {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsNumber(0))), notTaken);
                branch64(MacroAssembler::AboveOrEqual, valueGPR, GPRInfo::tagTypeNumberRegister, taken);
            }
    
            if (node->child1()->prediction() & SpecBoolean) {
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(false))), notTaken);
                branch64(MacroAssembler::Equal, valueGPR, MacroAssembler::TrustedImm64(JSValue::encode(jsBoolean(true))), taken);
            }
    
            value.use();
    
            silentSpillAllRegisters(resultGPR);
            callOperation(operationConvertJSValueToBoolean, resultGPR, valueGPR);
            silentFillAllRegisters(resultGPR);
    
            branchTest32(MacroAssembler::NonZero, resultGPR, taken);
            jump(notTaken);
        }
        
        noResult(node, UseChildrenCalledExplicitly);
        return;
    }
        
    default:
        DFG_CRASH(m_jit.graph(), m_currentNode, "Bad use kind");
    }
}

void SpeculativeJIT::compile(Node* node)
{
    NodeType op = node->op();
    
#if ENABLE(DFG_REGISTER_ALLOCATION_VALIDATION)
    m_jit.clearRegisterAllocationOffsets();
#endif

    switch (op) {
    case JSConstant:
    case DoubleConstant:
    case Int52Constant:
        initConstantInfo(node);
        break;

    case PhantomArguments:
        initConstantInfo(node);
        break;

    case Identity: {
        // CSE should always eliminate this.
        DFG_CRASH(m_jit.graph(), node, "Unexpected Identity node");
        break;
    }

    case GetLocal: {
        AbstractValue& value = m_state.variables().operand(node->local());

        // If the CFA is tracking this variable and it found that the variable
        // cannot have been assigned, then don't attempt to proceed.
        if (value.isClear()) {
            m_compileOkay = false;
            break;
        }
        
        switch (node->variableAccessData()->flushFormat()) {
        case FlushedDouble: {
            FPRTemporary result(this);
            m_jit.loadDouble(JITCompiler::addressFor(node->machineLocal()), result.fpr());
            VirtualRegister virtualRegister = node->virtualRegister();
            m_fprs.retain(result.fpr(), virtualRegister, SpillOrderDouble);
            generationInfoFromVirtualRegister(virtualRegister).initDouble(node, node->refCount(), result.fpr());
            break;
        }
        
        case FlushedInt32: {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->machineLocal()), result.gpr());
            
            // Like int32Result, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderInteger);
            generationInfoFromVirtualRegister(virtualRegister).initInt32(node, node->refCount(), result.gpr());
            break;
        }
            
        case FlushedInt52: {
            GPRTemporary result(this);
            m_jit.load64(JITCompiler::addressFor(node->machineLocal()), result.gpr());
            
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
            generationInfoFromVirtualRegister(virtualRegister).initInt52(node, node->refCount(), result.gpr());
            break;
        }
            
        default:
            GPRTemporary result(this);
            m_jit.load64(JITCompiler::addressFor(node->machineLocal()), result.gpr());
            
            // Like jsValueResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
            
            DataFormat format;
            if (isCellSpeculation(value.m_type))
                format = DataFormatJSCell;
            else if (isBooleanSpeculation(value.m_type))
                format = DataFormatJSBoolean;
            else
                format = DataFormatJS;
            
            generationInfoFromVirtualRegister(virtualRegister).initJSValue(node, node->refCount(), result.gpr(), format);
            break;
        }
        break;
    }

    case GetLocalUnlinked: {
        GPRTemporary result(this);
        
        m_jit.load64(JITCompiler::addressFor(node->unlinkedMachineLocal()), result.gpr());
        
        jsValueResult(result.gpr(), node);
        break;
    }
        
    case MovHint:
    case ZombieHint:
    case Check: {
        DFG_CRASH(m_jit.graph(), node, "Unexpected node");
        break;
    }

    case SetLocal: {
        switch (node->variableAccessData()->flushFormat()) {
        case FlushedDouble: {
            SpeculateDoubleOperand value(this, node->child1());
            m_jit.storeDouble(value.fpr(), JITCompiler::addressFor(node->machineLocal()));
            noResult(node);
            // Indicate that it's no longer necessary to retrieve the value of
            // this bytecode variable from registers or other locations in the stack,
            // but that it is stored as a double.
            recordSetLocal(DataFormatDouble);
            break;
        }
            
        case FlushedInt32: {
            SpeculateInt32Operand value(this, node->child1());
            m_jit.store32(value.gpr(), JITCompiler::payloadFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatInt32);
            break;
        }
            
        case FlushedInt52: {
            SpeculateInt52Operand value(this, node->child1());
            m_jit.store64(value.gpr(), JITCompiler::addressFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatInt52);
            break;
        }
            
        case FlushedCell: {
            SpeculateCellOperand cell(this, node->child1());
            GPRReg cellGPR = cell.gpr();
            m_jit.store64(cellGPR, JITCompiler::addressFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatCell);
            break;
        }
            
        case FlushedBoolean: {
            SpeculateBooleanOperand boolean(this, node->child1());
            m_jit.store64(boolean.gpr(), JITCompiler::addressFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatBoolean);
            break;
        }
            
        case FlushedJSValue:
        case FlushedArguments: {
            JSValueOperand value(this, node->child1());
            m_jit.store64(value.gpr(), JITCompiler::addressFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(dataFormatFor(node->variableAccessData()->flushFormat()));
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad flush format");
            break;
        }

        break;
    }

    case SetArgument:
        // This is a no-op; it just marks the fact that the argument is being used.
        // But it may be profitable to use this as a hook to run speculation checks
        // on arguments, thereby allowing us to trivially eliminate such checks if
        // the argument is not used.
        recordSetLocal(dataFormatFor(node->variableAccessData()->flushFormat()));
        break;

    case BitAnd:
    case BitOr:
    case BitXor:
        if (node->child1()->isInt32Constant()) {
            SpeculateInt32Operand op2(this, node->child2());
            GPRTemporary result(this, Reuse, op2);

            bitOp(op, node->child1()->asInt32(), op2.gpr(), result.gpr());

            int32Result(result.gpr(), node);
        } else if (node->child2()->isInt32Constant()) {
            SpeculateInt32Operand op1(this, node->child1());
            GPRTemporary result(this, Reuse, op1);

            bitOp(op, node->child2()->asInt32(), op1.gpr(), result.gpr());

            int32Result(result.gpr(), node);
        } else {
            SpeculateInt32Operand op1(this, node->child1());
            SpeculateInt32Operand op2(this, node->child2());
            GPRTemporary result(this, Reuse, op1, op2);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            bitOp(op, reg1, reg2, result.gpr());

            int32Result(result.gpr(), node);
        }
        break;

    case BitRShift:
    case BitLShift:
    case BitURShift:
        if (node->child2()->isInt32Constant()) {
            SpeculateInt32Operand op1(this, node->child1());
            GPRTemporary result(this, Reuse, op1);

            shiftOp(op, op1.gpr(), node->child2()->asInt32() & 0x1f, result.gpr());

            int32Result(result.gpr(), node);
        } else {
            // Do not allow shift amount to be used as the result, MacroAssembler does not permit this.
            SpeculateInt32Operand op1(this, node->child1());
            SpeculateInt32Operand op2(this, node->child2());
            GPRTemporary result(this, Reuse, op1);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            shiftOp(op, reg1, reg2, result.gpr());

            int32Result(result.gpr(), node);
        }
        break;

    case UInt32ToNumber: {
        compileUInt32ToNumber(node);
        break;
    }

    case DoubleAsInt32: {
        compileDoubleAsInt32(node);
        break;
    }

    case ValueToInt32: {
        compileValueToInt32(node);
        break;
    }
        
    case DoubleRep: {
        compileDoubleRep(node);
        break;
    }
        
    case ValueRep: {
        compileValueRep(node);
        break;
    }
        
    case Int52Rep: {
        switch (node->child1().useKind()) {
        case Int32Use: {
            SpeculateInt32Operand operand(this, node->child1());
            GPRTemporary result(this, Reuse, operand);
            
            m_jit.signExtend32ToPtr(operand.gpr(), result.gpr());
            
            strictInt52Result(result.gpr(), node);
            break;
        }
            
        case MachineIntUse: {
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            convertMachineInt(node->child1(), resultGPR);
            
            strictInt52Result(resultGPR, node);
            break;
        }
            
        case DoubleRepMachineIntUse: {
            SpeculateDoubleOperand value(this, node->child1());
            FPRReg valueFPR = value.fpr();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            flushRegisters();
            
            callOperation(operationConvertDoubleToInt52, resultGPR, valueFPR);
            
            DFG_TYPE_CHECK(
                JSValueRegs(), node->child1(), SpecInt52AsDouble,
                m_jit.branch64(
                    JITCompiler::Equal, resultGPR,
                    JITCompiler::TrustedImm64(JSValue::notInt52)));
            
            strictInt52Result(resultGPR, node);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
        }
        break;
    }
        
    case ValueAdd: {
        JSValueOperand op1(this, node->child1());
        JSValueOperand op2(this, node->child2());
        
        GPRReg op1GPR = op1.gpr();
        GPRReg op2GPR = op2.gpr();
        
        flushRegisters();
        
        GPRResult result(this);
        if (isKnownNotNumber(node->child1().node()) || isKnownNotNumber(node->child2().node()))
            callOperation(operationValueAddNotNumber, result.gpr(), op1GPR, op2GPR);
        else
            callOperation(operationValueAdd, result.gpr(), op1GPR, op2GPR);
        
        jsValueResult(result.gpr(), node);
        break;
    }
        
    case ArithAdd:
        compileAdd(node);
        break;
        
    case MakeRope:
        compileMakeRope(node);
        break;

    case ArithSub:
        compileArithSub(node);
        break;

    case ArithNegate:
        compileArithNegate(node);
        break;

    case ArithMul:
        compileArithMul(node);
        break;

    case ArithDiv: {
        compileArithDiv(node);
        break;
    }

    case ArithMod: {
        compileArithMod(node);
        break;
    }

    case ArithAbs: {
        switch (node->child1().useKind()) {
        case Int32Use: {
            SpeculateStrictInt32Operand op1(this, node->child1());
            GPRTemporary result(this);
            GPRTemporary scratch(this);
            
            m_jit.move(op1.gpr(), result.gpr());
            m_jit.rshift32(result.gpr(), MacroAssembler::TrustedImm32(31), scratch.gpr());
            m_jit.add32(scratch.gpr(), result.gpr());
            m_jit.xor32(scratch.gpr(), result.gpr());
            speculationCheck(Overflow, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, result.gpr(), MacroAssembler::TrustedImm32(1 << 31)));
            int32Result(result.gpr(), node);
            break;
        }
        
        case DoubleRepUse: {
            SpeculateDoubleOperand op1(this, node->child1());
            FPRTemporary result(this);
            
            m_jit.absDouble(op1.fpr(), result.fpr());
            doubleResult(result.fpr(), node);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }
        
    case ArithMin:
    case ArithMax: {
        switch (node->binaryUseKind()) {
        case Int32Use: {
            SpeculateStrictInt32Operand op1(this, node->child1());
            SpeculateStrictInt32Operand op2(this, node->child2());
            GPRTemporary result(this, Reuse, op1);
            
            MacroAssembler::Jump op1Less = m_jit.branch32(op == ArithMin ? MacroAssembler::LessThan : MacroAssembler::GreaterThan, op1.gpr(), op2.gpr());
            m_jit.move(op2.gpr(), result.gpr());
            if (op1.gpr() != result.gpr()) {
                MacroAssembler::Jump done = m_jit.jump();
                op1Less.link(&m_jit);
                m_jit.move(op1.gpr(), result.gpr());
                done.link(&m_jit);
            } else
                op1Less.link(&m_jit);
            
            int32Result(result.gpr(), node);
            break;
        }
        
        case DoubleRepUse: {
            SpeculateDoubleOperand op1(this, node->child1());
            SpeculateDoubleOperand op2(this, node->child2());
            FPRTemporary result(this, op1);
            
            FPRReg op1FPR = op1.fpr();
            FPRReg op2FPR = op2.fpr();
            FPRReg resultFPR = result.fpr();
        
            MacroAssembler::JumpList done;
        
            MacroAssembler::Jump op1Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleLessThan : MacroAssembler::DoubleGreaterThan, op1FPR, op2FPR);
        
            // op2 is eather the lesser one or one of then is NaN
            MacroAssembler::Jump op2Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleGreaterThanOrEqual : MacroAssembler::DoubleLessThanOrEqual, op1FPR, op2FPR);
        
            // Unordered case. We don't know which of op1, op2 is NaN. Manufacture NaN by adding 
            // op1 + op2 and putting it into result.
            m_jit.addDouble(op1FPR, op2FPR, resultFPR);
            done.append(m_jit.jump());
        
            op2Less.link(&m_jit);
            m_jit.moveDouble(op2FPR, resultFPR);
        
            if (op1FPR != resultFPR) {
                done.append(m_jit.jump());
            
                op1Less.link(&m_jit);
                m_jit.moveDouble(op1FPR, resultFPR);
            } else
                op1Less.link(&m_jit);
        
            done.link(&m_jit);
        
            doubleResult(resultFPR, node);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }
        
    case ArithSqrt: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRTemporary result(this, op1);
        
        m_jit.sqrtDouble(op1.fpr(), result.fpr());
        
        doubleResult(result.fpr(), node);
        break;
    }
        
    case ArithFRound: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRTemporary result(this, op1);
        
        m_jit.convertDoubleToFloat(op1.fpr(), result.fpr());
        m_jit.convertFloatToDouble(result.fpr(), result.fpr());
        
        doubleResult(result.fpr(), node);
        break;
    }

    case ArithSin: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRReg op1FPR = op1.fpr();

        flushRegisters();
        
        FPRResult result(this);
        callOperation(sin, result.fpr(), op1FPR);
        doubleResult(result.fpr(), node);
        break;
    }

    case ArithCos: {
        SpeculateDoubleOperand op1(this, node->child1());
        FPRReg op1FPR = op1.fpr();

        flushRegisters();
        
        FPRResult result(this);
        callOperation(cos, result.fpr(), op1FPR);
        doubleResult(result.fpr(), node);
        break;
    }

    case LogicalNot:
        compileLogicalNot(node);
        break;

    case CompareLess:
        if (compare(node, JITCompiler::LessThan, JITCompiler::DoubleLessThan, operationCompareLess))
            return;
        break;

    case CompareLessEq:
        if (compare(node, JITCompiler::LessThanOrEqual, JITCompiler::DoubleLessThanOrEqual, operationCompareLessEq))
            return;
        break;

    case CompareGreater:
        if (compare(node, JITCompiler::GreaterThan, JITCompiler::DoubleGreaterThan, operationCompareGreater))
            return;
        break;

    case CompareGreaterEq:
        if (compare(node, JITCompiler::GreaterThanOrEqual, JITCompiler::DoubleGreaterThanOrEqual, operationCompareGreaterEq))
            return;
        break;
        
    case CompareEqConstant:
        ASSERT(node->child2()->asJSValue().isNull());
        if (nonSpeculativeCompareNull(node, node->child1()))
            return;
        break;

    case CompareEq:
        if (compare(node, JITCompiler::Equal, JITCompiler::DoubleEqual, operationCompareEq))
            return;
        break;

    case CompareStrictEq:
        if (compileStrictEq(node))
            return;
        break;

    case StringCharCodeAt: {
        compileGetCharCodeAt(node);
        break;
    }

    case StringCharAt: {
        // Relies on StringCharAt node having same basic layout as GetByVal
        compileGetByValOnString(node);
        break;
    }

    case StringFromCharCode: {
        compileFromCharCode(node);
        break;
    }
        
    case CheckArray: {
        checkArray(node);
        break;
    }
        
    case Arrayify:
    case ArrayifyToStructure: {
        arrayify(node);
        break;
    }

    case GetByVal: {
        switch (node->arrayMode().type()) {
        case Array::SelectUsingPredictions:
        case Array::ForceExit:
            DFG_CRASH(m_jit.graph(), node, "Bad array mode type");
            break;
        case Array::Generic: {
            JSValueOperand base(this, node->child1());
            JSValueOperand property(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg propertyGPR = property.gpr();
            
            flushRegisters();
            GPRResult result(this);
            callOperation(operationGetByVal, result.gpr(), baseGPR, propertyGPR);
            
            jsValueResult(result.gpr(), node);
            break;
        }
        case Array::Int32:
        case Array::Contiguous: {
            if (node->arrayMode().isInBounds()) {
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
                
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
                
                if (!m_compileOkay)
                    return;
                
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
                
                GPRTemporary result(this);
                m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), result.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchTest64(MacroAssembler::Zero, result.gpr()));
                jsValueResult(result.gpr(), node, node->arrayMode().type() == Array::Int32 ? DataFormatJSInt32 : DataFormatJS);
                break;
            }
            
            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            GPRReg resultReg = result.gpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), resultReg);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, resultReg));
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }

        case Array::Double: {
            if (node->arrayMode().isInBounds()) {
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
            
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
            
                if (!m_compileOkay)
                    return;
            
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
                FPRTemporary result(this);
                m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), result.fpr());
                if (!node->arrayMode().isSaneChain())
                    speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, result.fpr(), result.fpr()));
                doubleResult(result.fpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            FPRTemporary temp(this);
            GPRReg resultReg = result.gpr();
            FPRReg tempReg = temp.fpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), tempReg);
            slowCases.append(m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempReg, tempReg));
            boxDouble(tempReg, resultReg);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }

        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            if (node->arrayMode().isInBounds()) {
                SpeculateStrictInt32Operand property(this, node->child2());
                StorageOperand storage(this, node->child3());
            
                GPRReg propertyReg = property.gpr();
                GPRReg storageReg = storage.gpr();
            
                if (!m_compileOkay)
                    return;
            
                speculationCheck(OutOfBounds, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset())));
            
                GPRTemporary result(this);
                m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), result.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branchTest64(MacroAssembler::Zero, result.gpr()));
            
                jsValueResult(result.gpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            
            GPRReg baseReg = base.gpr();
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            
            if (!m_compileOkay)
                return;
            
            GPRTemporary result(this);
            GPRReg resultReg = result.gpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset())));
    
            m_jit.load64(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), resultReg);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, resultReg));
    
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    result.gpr(), baseReg, propertyReg));
            
            jsValueResult(resultReg, node);
            break;
        }
        case Array::String:
            compileGetByValOnString(node);
            break;
        case Array::Arguments:
            compileGetByValOnArguments(node);
            break;
        default: {
            TypedArrayType type = node->arrayMode().typedArrayType();
            if (isInt(type))
                compileGetByValOnIntTypedArray(node, type);
            else
                compileGetByValOnFloatTypedArray(node, type);
        } }
        break;
    }

    case PutByValDirect:
    case PutByVal:
    case PutByValAlias: {
        Edge child1 = m_jit.graph().varArgChild(node, 0);
        Edge child2 = m_jit.graph().varArgChild(node, 1);
        Edge child3 = m_jit.graph().varArgChild(node, 2);
        Edge child4 = m_jit.graph().varArgChild(node, 3);
        
        ArrayMode arrayMode = node->arrayMode().modeForPut();
        bool alreadyHandled = false;
        
        switch (arrayMode.type()) {
        case Array::SelectUsingPredictions:
        case Array::ForceExit:
            DFG_CRASH(m_jit.graph(), node, "Bad array mode type");
            break;
        case Array::Generic: {
            DFG_ASSERT(m_jit.graph(), node, node->op() == PutByVal);
            
            JSValueOperand arg1(this, child1);
            JSValueOperand arg2(this, child2);
            JSValueOperand arg3(this, child3);
            GPRReg arg1GPR = arg1.gpr();
            GPRReg arg2GPR = arg2.gpr();
            GPRReg arg3GPR = arg3.gpr();
            flushRegisters();
            if (node->op() == PutByValDirect)
                callOperation(m_jit.isStrictModeFor(node->origin.semantic) ? operationPutByValDirectStrict : operationPutByValDirectNonStrict, arg1GPR, arg2GPR, arg3GPR);
            else
                callOperation(m_jit.isStrictModeFor(node->origin.semantic) ? operationPutByValStrict : operationPutByValNonStrict, arg1GPR, arg2GPR, arg3GPR);
            
            noResult(node);
            alreadyHandled = true;
            break;
        }
        default:
            break;
        }
        
        if (alreadyHandled)
            break;
        
        // FIXME: the base may not be necessary for some array access modes. But we have to
        // keep it alive to this point, so it's likely to be in a register anyway. Likely
        // no harm in locking it here.
        SpeculateCellOperand base(this, child1);
        SpeculateStrictInt32Operand property(this, child2);
        
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();

        switch (arrayMode.type()) {
        case Array::Int32:
        case Array::Contiguous: {
            JSValueOperand value(this, child3, ManualOperandSpeculation);

            GPRReg valueReg = value.gpr();
        
            if (!m_compileOkay)
                return;
            
            if (arrayMode.type() == Array::Int32) {
                DFG_TYPE_CHECK(
                    JSValueRegs(valueReg), child3, SpecInt32,
                    m_jit.branch64(
                        MacroAssembler::Below, valueReg, GPRInfo::tagTypeNumberRegister));
            }

            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                GPRReg valueReg = value.gpr();
                m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));
                
                noResult(node);
                break;
            }
            
            GPRTemporary temporary;
            GPRReg temporaryReg = temporaryRegisterForPutByVal(temporary, node);

            MacroAssembler::Jump slowCase;
            
            if (arrayMode.isInBounds()) {
                speculationCheck(
                    OutOfBounds, JSValueRegs(), 0,
                    m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            } else {
                MacroAssembler::Jump inBounds = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
                
                slowCase = m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfVectorLength()));
                
                if (!arrayMode.isOutOfBounds())
                    speculationCheck(OutOfBounds, JSValueRegs(), 0, slowCase);
                
                m_jit.add32(TrustedImm32(1), propertyReg, temporaryReg);
                m_jit.store32(temporaryReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
                
                inBounds.link(&m_jit);
            }
            
            m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (arrayMode.isOutOfBounds()) {
                if (node->op() == PutByValDirect) {
                    addSlowPathGenerator(slowPathCall(
                        slowCase, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValDirectBeyondArrayBoundsStrict : operationPutByValDirectBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
                } else {
                    addSlowPathGenerator(slowPathCall(
                        slowCase, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
                }
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Double: {
            compileDoublePutByVal(node, base, property);
            break;
        }
            
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            JSValueOperand value(this, child3);

            GPRReg valueReg = value.gpr();
        
            if (!m_compileOkay)
                return;

            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                GPRReg valueReg = value.gpr();
                m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
                
                noResult(node);
                break;
            }
            
            GPRTemporary temporary;
            GPRReg temporaryReg = temporaryRegisterForPutByVal(temporary, node);

            MacroAssembler::JumpList slowCases;

            MacroAssembler::Jump beyondArrayBounds = m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset()));
            if (!arrayMode.isOutOfBounds())
                speculationCheck(OutOfBounds, JSValueRegs(), 0, beyondArrayBounds);
            else
                slowCases.append(beyondArrayBounds);

            // Check if we're writing to a hole; if so increment m_numValuesInVector.
            if (arrayMode.isInBounds()) {
                speculationCheck(
                    StoreToHole, JSValueRegs(), 0,
                    m_jit.branchTest64(MacroAssembler::Zero, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]))));
            } else {
                MacroAssembler::Jump notHoleValue = m_jit.branchTest64(MacroAssembler::NonZero, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
                if (arrayMode.isSlowPut()) {
                    // This is sort of strange. If we wanted to optimize this code path, we would invert
                    // the above branch. But it's simply not worth it since this only happens if we're
                    // already having a bad time.
                    slowCases.append(m_jit.jump());
                } else {
                    m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, ArrayStorage::numValuesInVectorOffset()));
                
                    // If we're writing to a hole we might be growing the array; 
                    MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.add32(TrustedImm32(1), propertyReg, temporaryReg);
                    m_jit.store32(temporaryReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                
                    lengthDoesNotNeedUpdate.link(&m_jit);
                }
                notHoleValue.link(&m_jit);
            }
    
            // Store the value to the array.
            m_jit.store64(valueReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (!slowCases.empty()) {
                if (node->op() == PutByValDirect) {
                    addSlowPathGenerator(slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValDirectBeyondArrayBoundsStrict : operationPutByValDirectBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
                } else {
                    addSlowPathGenerator(slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueReg));
                }
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Arguments: {
            JSValueOperand value(this, child3);
            GPRTemporary scratch(this);
            GPRTemporary scratch2(this);
            
            GPRReg valueReg = value.gpr();
            GPRReg scratchReg = scratch.gpr();
            GPRReg scratch2Reg = scratch2.gpr();
            
            if (!m_compileOkay)
                return;

            // Two really lame checks.
            speculationCheck(
                Uncountable, JSValueSource(), 0,
                m_jit.branch32(
                    MacroAssembler::AboveOrEqual, propertyReg,
                    MacroAssembler::Address(baseReg, Arguments::offsetOfNumArguments())));
            speculationCheck(
                Uncountable, JSValueSource(), 0,
                m_jit.branchTestPtr(
                    MacroAssembler::NonZero,
                    MacroAssembler::Address(
                        baseReg, Arguments::offsetOfSlowArgumentData())));

            m_jit.move(propertyReg, scratch2Reg);
            m_jit.signExtend32ToPtr(scratch2Reg, scratch2Reg);
            m_jit.loadPtr(
                MacroAssembler::Address(baseReg, Arguments::offsetOfRegisters()),
                scratchReg);
            
            m_jit.store64(
                valueReg,
                MacroAssembler::BaseIndex(
                    scratchReg, scratch2Reg, MacroAssembler::TimesEight,
                    CallFrame::thisArgumentOffset() * sizeof(Register) + sizeof(Register)));
            
            noResult(node);
            break;
        }
            
        default: {
            TypedArrayType type = arrayMode.typedArrayType();
            if (isInt(type))
                compilePutByValForIntTypedArray(base.gpr(), property.gpr(), node, type);
            else
                compilePutByValForFloatTypedArray(base.gpr(), property.gpr(), node, type);
        } }

        break;
    }

    case RegExpExec: {
        if (compileRegExpExec(node))
            return;
        if (!node->adjustedRefCount()) {
            SpeculateCellOperand base(this, node->child1());
            SpeculateCellOperand argument(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg argumentGPR = argument.gpr();
            
            flushRegisters();
            GPRResult result(this);
            callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);
            
            // Must use jsValueResult because otherwise we screw up register
            // allocation, which thinks that this node has a result.
            jsValueResult(result.gpr(), node);
            break;
        }

        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult result(this);
        callOperation(operationRegExpExec, result.gpr(), baseGPR, argumentGPR);
        
        jsValueResult(result.gpr(), node);
        break;
    }

    case RegExpTest: {
        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult result(this);
        callOperation(operationRegExpTest, result.gpr(), baseGPR, argumentGPR);
        
        // If we add a DataFormatBool, we should use it here.
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case ArrayPush: {
        ASSERT(node->arrayMode().isJSArray());
        
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary storageLength(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        
        StorageOperand storage(this, node->child3());
        GPRReg storageGPR = storage.gpr();

        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            JSValueOperand value(this, node->child2(), ManualOperandSpeculation);
            GPRReg valueGPR = value.gpr();

            if (node->arrayMode().type() == Array::Int32) {
                DFG_TYPE_CHECK(
                    JSValueRegs(valueGPR), node->child2(), SpecInt32,
                    m_jit.branch64(
                        MacroAssembler::Below, valueGPR, GPRInfo::tagTypeNumberRegister));
            }

            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store64(valueGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush, storageLengthGPR,
                    valueGPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        case Array::Double: {
            SpeculateDoubleOperand value(this, node->child2());
            FPRReg valueFPR = value.fpr();

            DFG_TYPE_CHECK(
                JSValueRegs(), node->child2(), SpecDoubleReal,
                m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, valueFPR, valueFPR));
            
            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.storeDouble(valueFPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPushDouble, storageLengthGPR,
                    valueFPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            JSValueOperand value(this, node->child2());
            GPRReg valueGPR = value.gpr();

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            // Refuse to handle bizarre lengths.
            speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Above, storageLengthGPR, TrustedImm32(0x7ffffffe)));
        
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.store64(valueGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
        
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
            m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
            m_jit.or64(GPRInfo::tagTypeNumberRegister, storageLengthGPR);
        
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush, NoResult, storageLengthGPR,
                    valueGPR, baseGPR));
        
            jsValueResult(storageLengthGPR, node);
            break;
        }
            
        default:
            CRASH();
            break;
        }
        break;
    }
        
    case ArrayPop: {
        ASSERT(node->arrayMode().isJSArray());

        SpeculateCellOperand base(this, node->child1());
        StorageOperand storage(this, node->child2());
        GPRTemporary value(this);
        GPRTemporary storageLength(this);
        FPRTemporary temp(this); // This is kind of lame, since we don't always need it. I'm relying on the fact that we don't have FPR pressure, especially in code that uses pop().
        
        GPRReg baseGPR = base.gpr();
        GPRReg storageGPR = storage.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        FPRReg tempFPR = temp.fpr();
        
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Double:
        case Array::Contiguous: {
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR);
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(
                storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            MacroAssembler::Jump slowCase;
            if (node->arrayMode().type() == Array::Double) {
                m_jit.loadDouble(
                    MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight),
                    tempFPR);
                // FIXME: This would not have to be here if changing the publicLength also zeroed the values between the old
                // length and the new length.
                m_jit.store64(
                    MacroAssembler::TrustedImm64(bitwise_cast<int64_t>(PNaN)), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
                slowCase = m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempFPR, tempFPR);
                boxDouble(tempFPR, valueGPR);
            } else {
                m_jit.load64(
                    MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight),
                    valueGPR);
                // FIXME: This would not have to be here if changing the publicLength also zeroed the values between the old
                // length and the new length.
                m_jit.store64(
                MacroAssembler::TrustedImm64((int64_t)0), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight));
                slowCase = m_jit.branchTest64(MacroAssembler::Zero, valueGPR);
            }

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm64(JSValue::encode(jsUndefined())), valueGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength, valueGPR, baseGPR));
            
            // We can't know for sure that the result is an int because of the slow paths. :-/
            jsValueResult(valueGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            JITCompiler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR);
        
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
        
            JITCompiler::JumpList slowCases;
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset())));
        
            m_jit.load64(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])), valueGPR);
            slowCases.append(m_jit.branchTest64(MacroAssembler::Zero, valueGPR));
        
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
        
            m_jit.store64(MacroAssembler::TrustedImm64((int64_t)0), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
            m_jit.sub32(MacroAssembler::TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        
            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm64(JSValue::encode(jsUndefined())), valueGPR));
        
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationArrayPop, valueGPR, baseGPR));

            jsValueResult(valueGPR, node);
            break;
        }
            
        default:
            CRASH();
            break;
        }
        break;
    }

    case DFG::Jump: {
        jump(node->targetBlock());
        noResult(node);
        break;
    }

    case Branch:
        emitBranch(node);
        break;
        
    case Switch:
        emitSwitch(node);
        break;

    case Return: {
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT1);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node->child1());
        m_jit.move(op1.gpr(), GPRInfo::returnValueGPR);

        m_jit.emitFunctionEpilogue();
        m_jit.ret();
        
        noResult(node);
        break;
    }
        
    case Throw:
    case ThrowReferenceError: {
        // We expect that throw statements are rare and are intended to exit the code block
        // anyway, so we just OSR back to the old JIT for now.
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        break;
    }
        
    case BooleanToNumber: {
        switch (node->child1().useKind()) {
        case BooleanUse: {
            JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
            GPRTemporary result(this); // FIXME: We could reuse, but on speculation fail would need recovery to restore tag (akin to add).
            
            m_jit.move(value.gpr(), result.gpr());
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), result.gpr());
            DFG_TYPE_CHECK(
                JSValueRegs(value.gpr()), node->child1(), SpecBoolean, m_jit.branchTest64(
                    JITCompiler::NonZero, result.gpr(), TrustedImm32(static_cast<int32_t>(~1))));

            int32Result(result.gpr(), node);
            break;
        }
            
        case UntypedUse: {
            JSValueOperand value(this, node->child1());
            GPRTemporary result(this);
            
            m_jit.move(value.gpr(), result.gpr());
            m_jit.xor64(TrustedImm32(static_cast<int32_t>(ValueFalse)), result.gpr());
            JITCompiler::Jump isBoolean = m_jit.branchTest64(
                JITCompiler::Zero, result.gpr(), TrustedImm32(static_cast<int32_t>(~1)));
            m_jit.move(value.gpr(), result.gpr());
            JITCompiler::Jump done = m_jit.jump();
            isBoolean.link(&m_jit);
            m_jit.or64(GPRInfo::tagTypeNumberRegister, result.gpr());
            done.link(&m_jit);
            
            jsValueResult(result.gpr(), node);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }
        
    case ToPrimitive: {
        DFG_ASSERT(m_jit.graph(), node, node->child1().useKind() == UntypedUse);
        JSValueOperand op1(this, node->child1());
        GPRTemporary result(this, Reuse, op1);
        
        GPRReg op1GPR = op1.gpr();
        GPRReg resultGPR = result.gpr();
        
        op1.use();
        
        MacroAssembler::Jump alreadyPrimitive = branchNotCell(JSValueRegs(op1GPR));
        MacroAssembler::Jump notPrimitive = m_jit.branchStructurePtr(
            MacroAssembler::NotEqual, 
            MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
            m_jit.vm()->stringStructure.get());
        
        alreadyPrimitive.link(&m_jit);
        m_jit.move(op1GPR, resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(notPrimitive, this, operationToPrimitive, resultGPR, op1GPR));
        
        jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
        break;
    }
        
    case ToString: {
        if (node->child1().useKind() == UntypedUse) {
            JSValueOperand op1(this, node->child1());
            GPRReg op1GPR = op1.gpr();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            flushRegisters();
            
            JITCompiler::Jump done;
            if (node->child1()->prediction() & SpecString) {
                JITCompiler::Jump slowPath1 = branchNotCell(JSValueRegs(op1GPR));
                JITCompiler::Jump slowPath2 = m_jit.branchStructurePtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(op1GPR, JSCell::structureIDOffset()),
                    m_jit.vm()->stringStructure.get());
                m_jit.move(op1GPR, resultGPR);
                done = m_jit.jump();
                slowPath1.link(&m_jit);
                slowPath2.link(&m_jit);
            }
            callOperation(operationToString, resultGPR, op1GPR);
            if (done.isSet())
                done.link(&m_jit);
            cellResult(resultGPR, node);
            break;
        }
        
        compileToStringOnCell(node);
        break;
    }
        
    case NewStringObject: {
        compileNewStringObject(node);
        break;
    }
        
    case NewArray: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->origin.semantic);
        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(node->indexingType())) {
            Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType());
            DFG_ASSERT(m_jit.graph(), node, structure->indexingType() == node->indexingType());
            ASSERT(
                hasUndecided(structure->indexingType())
                || hasInt32(structure->indexingType())
                || hasDouble(structure->indexingType())
                || hasContiguous(structure->indexingType()));
            
            unsigned numElements = node->numChildren();
            
            GPRTemporary result(this);
            GPRTemporary storage(this);
            
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();

            emitAllocateJSArray(resultGPR, structure, storageGPR, numElements);
            
            // At this point, one way or another, resultGPR and storageGPR have pointers to
            // the JSArray and the Butterfly, respectively.
            
            ASSERT(!hasUndecided(structure->indexingType()) || !node->numChildren());
            
            for (unsigned operandIdx = 0; operandIdx < node->numChildren(); ++operandIdx) {
                Edge use = m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx];
                switch (node->indexingType()) {
                case ALL_BLANK_INDEXING_TYPES:
                case ALL_UNDECIDED_INDEXING_TYPES:
                    CRASH();
                    break;
                case ALL_DOUBLE_INDEXING_TYPES: {
                    SpeculateDoubleOperand operand(this, use);
                    FPRReg opFPR = operand.fpr();
                    DFG_TYPE_CHECK(
                        JSValueRegs(), use, SpecDoubleReal,
                        m_jit.branchDouble(
                            MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                    m_jit.storeDouble(opFPR, MacroAssembler::Address(storageGPR, sizeof(double) * operandIdx));
                    break;
                }
                case ALL_INT32_INDEXING_TYPES:
                case ALL_CONTIGUOUS_INDEXING_TYPES: {
                    JSValueOperand operand(this, use, ManualOperandSpeculation);
                    GPRReg opGPR = operand.gpr();
                    if (hasInt32(node->indexingType())) {
                        DFG_TYPE_CHECK(
                            JSValueRegs(opGPR), use, SpecInt32,
                            m_jit.branch64(
                                MacroAssembler::Below, opGPR, GPRInfo::tagTypeNumberRegister));
                    }
                    m_jit.store64(opGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx));
                    break;
                }
                default:
                    CRASH();
                    break;
                }
            }
            
            // Yuck, we should *really* have a way of also returning the storageGPR. But
            // that's the least of what's wrong with this code. We really shouldn't be
            // allocating the array after having computed - and probably spilled to the
            // stack - all of the things that will go into the array. The solution to that
            // bigger problem will also likely fix the redundancy in reloading the storage
            // pointer that we currently have.
            
            cellResult(resultGPR, node);
            break;
        }
        
        if (!node->numChildren()) {
            flushRegisters();
            GPRResult result(this);
            callOperation(operationNewEmptyArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()));
            cellResult(result.gpr(), node);
            break;
        }
        
        size_t scratchSize = sizeof(EncodedJSValue) * node->numChildren();
        ScratchBuffer* scratchBuffer = m_jit.vm()->scratchBufferForSize(scratchSize);
        EncodedJSValue* buffer = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;
        
        for (unsigned operandIdx = 0; operandIdx < node->numChildren(); ++operandIdx) {
            // Need to perform the speculations that this node promises to perform. If we're
            // emitting code here and the indexing type is not array storage then there is
            // probably something hilarious going on and we're already failing at all the
            // things, but at least we're going to be sound.
            Edge use = m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx];
            switch (node->indexingType()) {
            case ALL_BLANK_INDEXING_TYPES:
            case ALL_UNDECIDED_INDEXING_TYPES:
                CRASH();
                break;
            case ALL_DOUBLE_INDEXING_TYPES: {
                SpeculateDoubleOperand operand(this, use);
                GPRTemporary scratch(this);
                FPRReg opFPR = operand.fpr();
                GPRReg scratchGPR = scratch.gpr();
                DFG_TYPE_CHECK(
                    JSValueRegs(), use, SpecDoubleReal,
                    m_jit.branchDouble(
                        MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                m_jit.boxDouble(opFPR, scratchGPR);
                m_jit.store64(scratchGPR, buffer + operandIdx);
                break;
            }
            case ALL_INT32_INDEXING_TYPES: {
                JSValueOperand operand(this, use, ManualOperandSpeculation);
                GPRReg opGPR = operand.gpr();
                if (hasInt32(node->indexingType())) {
                    DFG_TYPE_CHECK(
                        JSValueRegs(opGPR), use, SpecInt32,
                        m_jit.branch64(
                            MacroAssembler::Below, opGPR, GPRInfo::tagTypeNumberRegister));
                }
                m_jit.store64(opGPR, buffer + operandIdx);
                break;
            }
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                JSValueOperand operand(this, use);
                GPRReg opGPR = operand.gpr();
                m_jit.store64(opGPR, buffer + operandIdx);
                operand.use();
                break;
            }
            default:
                CRASH();
                break;
            }
        }
        
        switch (node->indexingType()) {
        case ALL_DOUBLE_INDEXING_TYPES:
        case ALL_INT32_INDEXING_TYPES:
            useChildren(node);
            break;
        default:
            break;
        }
        
        flushRegisters();

        if (scratchSize) {
            GPRTemporary scratch(this);

            // Tell GC mark phase how much of the scratch buffer is active during call.
            m_jit.move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), scratch.gpr());
            m_jit.storePtr(TrustedImmPtr(scratchSize), scratch.gpr());
        }

        GPRResult result(this);
        
        callOperation(
            operationNewArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()),
            static_cast<void*>(buffer), node->numChildren());

        if (scratchSize) {
            GPRTemporary scratch(this);

            m_jit.move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), scratch.gpr());
            m_jit.storePtr(TrustedImmPtr(0), scratch.gpr());
        }

        cellResult(result.gpr(), node, UseChildrenCalledExplicitly);
        break;
    }
        
    case NewArrayWithSize: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->origin.semantic);
        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(node->indexingType())) {
            SpeculateStrictInt32Operand size(this, node->child1());
            GPRTemporary result(this);
            GPRTemporary storage(this);
            GPRTemporary scratch(this);
            GPRTemporary scratch2(this);
            
            GPRReg sizeGPR = size.gpr();
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();
            GPRReg scratchGPR = scratch.gpr();
            GPRReg scratch2GPR = scratch2.gpr();
            
            MacroAssembler::JumpList slowCases;
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, sizeGPR, TrustedImm32(MIN_SPARSE_ARRAY_INDEX)));
            
            ASSERT((1 << 3) == sizeof(JSValue));
            m_jit.move(sizeGPR, scratchGPR);
            m_jit.lshift32(TrustedImm32(3), scratchGPR);
            m_jit.add32(TrustedImm32(sizeof(IndexingHeader)), scratchGPR, resultGPR);
            slowCases.append(
                emitAllocateBasicStorage(resultGPR, storageGPR));
            m_jit.subPtr(scratchGPR, storageGPR);
            Structure* structure = globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType());
            emitAllocateJSObject<JSArray>(resultGPR, TrustedImmPtr(structure), storageGPR, scratchGPR, scratch2GPR, slowCases);
            
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.store32(sizeGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            
            if (hasDouble(node->indexingType())) {
                m_jit.move(TrustedImm64(bitwise_cast<int64_t>(PNaN)), scratchGPR);
                m_jit.move(sizeGPR, scratch2GPR);
                MacroAssembler::Jump done = m_jit.branchTest32(MacroAssembler::Zero, scratch2GPR);
                MacroAssembler::Label loop = m_jit.label();
                m_jit.sub32(TrustedImm32(1), scratch2GPR);
                m_jit.store64(scratchGPR, MacroAssembler::BaseIndex(storageGPR, scratch2GPR, MacroAssembler::TimesEight));
                m_jit.branchTest32(MacroAssembler::NonZero, scratch2GPR).linkTo(loop, &m_jit);
                done.link(&m_jit);
            }
            
            addSlowPathGenerator(adoptPtr(
                new CallArrayAllocatorWithVariableSizeSlowPathGenerator(
                    slowCases, this, operationNewArrayWithSize, resultGPR,
                    globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()),
                    globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage),
                    sizeGPR)));
            
            cellResult(resultGPR, node);
            break;
        }
        
        SpeculateStrictInt32Operand size(this, node->child1());
        GPRReg sizeGPR = size.gpr();
        flushRegisters();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        GPRReg structureGPR = selectScratchGPR(sizeGPR);
        MacroAssembler::Jump bigLength = m_jit.branch32(MacroAssembler::AboveOrEqual, sizeGPR, TrustedImm32(MIN_SPARSE_ARRAY_INDEX));
        m_jit.move(TrustedImmPtr(globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType())), structureGPR);
        MacroAssembler::Jump done = m_jit.jump();
        bigLength.link(&m_jit);
        m_jit.move(TrustedImmPtr(globalObject->arrayStructureForIndexingTypeDuringAllocation(ArrayWithArrayStorage)), structureGPR);
        done.link(&m_jit);
        callOperation(operationNewArrayWithSize, resultGPR, structureGPR, sizeGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case NewArrayBuffer: {
        JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->origin.semantic);
        IndexingType indexingType = node->indexingType();
        if (!globalObject->isHavingABadTime() && !hasAnyArrayStorage(indexingType)) {
            unsigned numElements = node->numConstants();
            
            GPRTemporary result(this);
            GPRTemporary storage(this);
            
            GPRReg resultGPR = result.gpr();
            GPRReg storageGPR = storage.gpr();

            emitAllocateJSArray(resultGPR, globalObject->arrayStructureForIndexingTypeDuringAllocation(indexingType), storageGPR, numElements);
            
            DFG_ASSERT(m_jit.graph(), node, indexingType & IsArray);
            JSValue* data = m_jit.codeBlock()->constantBuffer(node->startConstant());
            if (indexingType == ArrayWithDouble) {
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    double value = data[index].asNumber();
                    m_jit.store64(
                        Imm64(bitwise_cast<int64_t>(value)),
                        MacroAssembler::Address(storageGPR, sizeof(double) * index));
                }
            } else {
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    m_jit.store64(
                        Imm64(JSValue::encode(data[index])),
                        MacroAssembler::Address(storageGPR, sizeof(JSValue) * index));
                }
            }
            
            cellResult(resultGPR, node);
            break;
        }
        
        flushRegisters();
        GPRResult result(this);
        
        callOperation(operationNewArrayBuffer, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()), node->startConstant(), node->numConstants());
        
        cellResult(result.gpr(), node);
        break;
    }
        
    case NewTypedArray: {
        switch (node->child1().useKind()) {
        case Int32Use:
            compileNewTypedArray(node);
            break;
        case UntypedUse: {
            JSValueOperand argument(this, node->child1());
            GPRReg argumentGPR = argument.gpr();
            
            flushRegisters();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->origin.semantic);
            callOperation(
                operationNewTypedArrayWithOneArgumentForType(node->typedArrayType()),
                resultGPR, globalObject->typedArrayStructure(node->typedArrayType()),
                argumentGPR);
            
            cellResult(resultGPR, node);
            break;
        }
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }
        
    case NewRegexp: {
        flushRegisters();
        GPRResult result(this);
        
        callOperation(operationNewRegexp, result.gpr(), m_jit.codeBlock()->regexp(node->regexpIndex()));
        
        cellResult(result.gpr(), node);
        break;
    }
        
    case ToThis: {
        ASSERT(node->child1().useKind() == UntypedUse);
        JSValueOperand thisValue(this, node->child1());
        GPRTemporary temp(this);
        GPRReg thisValueGPR = thisValue.gpr();
        GPRReg tempGPR = temp.gpr();
        
        MacroAssembler::JumpList slowCases;
        slowCases.append(branchNotCell(JSValueRegs(thisValueGPR)));
        slowCases.append(m_jit.branch8(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(thisValueGPR, JSCell::typeInfoTypeOffset()),
            TrustedImm32(FinalObjectType)));
        m_jit.move(thisValueGPR, tempGPR);
        J_JITOperation_EJ function;
        if (m_jit.graph().executableFor(node->origin.semantic)->isStrictMode())
            function = operationToThisStrict;
        else
            function = operationToThis;
        addSlowPathGenerator(
            slowPathCall(slowCases, this, function, tempGPR, thisValueGPR));

        jsValueResult(tempGPR, node);
        break;
    }

    case CreateThis: {
        // Note that there is not so much profit to speculate here. The only things we
        // speculate on are (1) that it's a cell, since that eliminates cell checks
        // later if the proto is reused, and (2) if we have a FinalObject prediction
        // then we speculate because we want to get recompiled if it isn't (since
        // otherwise we'd start taking slow path a lot).
        
        SpeculateCellOperand callee(this, node->child1());
        GPRTemporary result(this);
        GPRTemporary allocator(this);
        GPRTemporary structure(this);
        GPRTemporary scratch(this);
        
        GPRReg calleeGPR = callee.gpr();
        GPRReg resultGPR = result.gpr();
        GPRReg allocatorGPR = allocator.gpr();
        GPRReg structureGPR = structure.gpr();
        GPRReg scratchGPR = scratch.gpr();

        MacroAssembler::JumpList slowPath;

        m_jit.loadPtr(JITCompiler::Address(calleeGPR, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfAllocator()), allocatorGPR);
        m_jit.loadPtr(JITCompiler::Address(calleeGPR, JSFunction::offsetOfAllocationProfile() + ObjectAllocationProfile::offsetOfStructure()), structureGPR);
        slowPath.append(m_jit.branchTestPtr(MacroAssembler::Zero, allocatorGPR));
        emitAllocateJSObject(resultGPR, allocatorGPR, structureGPR, TrustedImmPtr(0), scratchGPR, slowPath);

        addSlowPathGenerator(slowPathCall(slowPath, this, operationCreateThis, resultGPR, calleeGPR, node->inlineCapacity()));
        
        cellResult(resultGPR, node);
        break;
    }
        
    case AllocationProfileWatchpoint:
    case TypedArrayWatchpoint: {
        noResult(node);
        break;
    }

    case NewObject: {
        GPRTemporary result(this);
        GPRTemporary allocator(this);
        GPRTemporary scratch(this);
        
        GPRReg resultGPR = result.gpr();
        GPRReg allocatorGPR = allocator.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        MacroAssembler::JumpList slowPath;

        Structure* structure = node->structure();
        size_t allocationSize = JSFinalObject::allocationSize(structure->inlineCapacity());
        MarkedAllocator* allocatorPtr = &m_jit.vm()->heap.allocatorForObjectWithoutDestructor(allocationSize);

        m_jit.move(TrustedImmPtr(allocatorPtr), allocatorGPR);
        emitAllocateJSObject(resultGPR, allocatorGPR, TrustedImmPtr(structure), TrustedImmPtr(0), scratchGPR, slowPath);

        addSlowPathGenerator(slowPathCall(slowPath, this, operationNewObject, resultGPR, structure));
        
        cellResult(resultGPR, node);
        break;
    }

    case GetCallee: {
        GPRTemporary result(this);
        m_jit.loadPtr(JITCompiler::addressFor(JSStack::Callee), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case GetScope: {
        SpeculateCellOperand function(this, node->child1());
        GPRTemporary result(this, Reuse, function);
        m_jit.loadPtr(JITCompiler::Address(function.gpr(), JSFunction::offsetOfScopeChain()), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case GetMyScope: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();

        m_jit.loadPtr(JITCompiler::addressFor(JSStack::ScopeChain), resultGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case SkipTopScope: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this, Reuse, scope);
        GPRReg resultGPR = result.gpr();
        m_jit.move(scope.gpr(), resultGPR);
        JITCompiler::Jump activationNotCreated =
            m_jit.branchTest64(
                JITCompiler::Zero,
                JITCompiler::addressFor(
                    static_cast<VirtualRegister>(m_jit.graph().machineActivationRegister())));
        m_jit.loadPtr(JITCompiler::Address(resultGPR, JSScope::offsetOfNext()), resultGPR);
        activationNotCreated.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }
        
    case SkipScope: {
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this, Reuse, scope);
        m_jit.loadPtr(JITCompiler::Address(scope.gpr(), JSScope::offsetOfNext()), result.gpr());
        cellResult(result.gpr(), node);
        break;
    }
        
    case GetClosureRegisters: {
        if (WriteBarrierBase<Unknown>* registers = m_jit.graph().tryGetRegisters(node->child1().node())) {
            GPRTemporary result(this);
            GPRReg resultGPR = result.gpr();
            m_jit.move(TrustedImmPtr(registers), resultGPR);
            storageResult(resultGPR, node);
            break;
        }
        
        SpeculateCellOperand scope(this, node->child1());
        GPRTemporary result(this);
        GPRReg scopeGPR = scope.gpr();
        GPRReg resultGPR = result.gpr();

        m_jit.loadPtr(JITCompiler::Address(scopeGPR, JSVariableObject::offsetOfRegisters()), resultGPR);
        storageResult(resultGPR, node);
        break;
    }
    case GetClosureVar: {
        StorageOperand registers(this, node->child1());
        GPRTemporary result(this);
        GPRReg registersGPR = registers.gpr();
        GPRReg resultGPR = result.gpr();

        m_jit.load64(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register)), resultGPR);
        jsValueResult(resultGPR, node);
        break;
    }
    case PutClosureVar: {
        StorageOperand registers(this, node->child2());
        JSValueOperand value(this, node->child3());

        GPRReg registersGPR = registers.gpr();
        GPRReg valueGPR = value.gpr();

        speculate(node, node->child1());

        m_jit.store64(valueGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register)));
        noResult(node);
        break;
    }
    case GetById: {
        ASSERT(node->prediction());

        switch (node->child1().useKind()) {
        case CellUse: {
            SpeculateCellOperand base(this, node->child1());
            GPRTemporary result(this, Reuse, base);
            
            GPRReg baseGPR = base.gpr();
            GPRReg resultGPR = result.gpr();
            
            base.use();
            
            cachedGetById(node->origin.semantic, baseGPR, resultGPR, node->identifierNumber());
            
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRTemporary result(this, Reuse, base);
        
            GPRReg baseGPR = base.gpr();
            GPRReg resultGPR = result.gpr();
        
            base.use();
        
            JITCompiler::Jump notCell = branchNotCell(JSValueRegs(baseGPR));
        
            cachedGetById(node->origin.semantic, baseGPR, resultGPR, node->identifierNumber(), notCell);
        
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }

    case GetByIdFlush: {
        if (!node->prediction()) {
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            break;
        }

        switch (node->child1().useKind()) {
        case CellUse: {
            SpeculateCellOperand base(this, node->child1());
            GPRReg baseGPR = base.gpr();

            GPRResult result(this);
            
            GPRReg resultGPR = result.gpr();
            
            base.use();
            
            flushRegisters();
            
            cachedGetById(node->origin.semantic, baseGPR, resultGPR, node->identifierNumber(), JITCompiler::Jump(), DontSpill);
            
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRReg baseGPR = base.gpr();

            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
        
            base.use();
            flushRegisters();
        
            JITCompiler::Jump notCell = branchNotCell(JSValueRegs(baseGPR));
        
            cachedGetById(node->origin.semantic, baseGPR, resultGPR, node->identifierNumber(), notCell, DontSpill);
        
            jsValueResult(resultGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            DFG_CRASH(m_jit.graph(), node, "Bad use kind");
            break;
        }
        break;
    }

    case GetArrayLength:
        compileGetArrayLength(node);
        break;
        
    case CheckFunction: {
        SpeculateCellOperand function(this, node->child1());
        speculationCheck(BadFunction, JSValueSource::unboxedCell(function.gpr()), node->child1(), m_jit.branchWeakPtr(JITCompiler::NotEqual, function.gpr(), node->function()->value().asCell()));
        noResult(node);
        break;
    }
        
    case CheckExecutable: {
        SpeculateCellOperand function(this, node->child1());
        speculationCheck(BadExecutable, JSValueSource::unboxedCell(function.gpr()), node->child1(), m_jit.branchWeakPtr(JITCompiler::NotEqual, JITCompiler::Address(function.gpr(), JSFunction::offsetOfExecutable()), node->executable()));
        noResult(node);
        break;
    }
        
    case CheckStructure: {
        SpeculateCellOperand base(this, node->child1());
        
        ASSERT(node->structureSet().size());
        
        ExitKind exitKind;
        if (node->child1()->hasConstant())
            exitKind = BadConstantCache;
        else
            exitKind = BadCache;
        
        if (node->structureSet().size() == 1) {
            speculationCheck(
                exitKind, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakStructure(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(base.gpr(), JSCell::structureIDOffset()),
                    node->structureSet()[0]));
        } else {
            JITCompiler::JumpList done;
            
            for (size_t i = 0; i < node->structureSet().size() - 1; ++i)
                done.append(m_jit.branchWeakStructure(JITCompiler::Equal, MacroAssembler::Address(base.gpr(), JSCell::structureIDOffset()), node->structureSet()[i]));
            
            speculationCheck(
                exitKind, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakStructure(
                    JITCompiler::NotEqual, MacroAssembler::Address(base.gpr(), JSCell::structureIDOffset()), node->structureSet().last()));
            
            done.link(&m_jit);
        }
        
        noResult(node);
        break;
    }
        
    case PutStructure: {
        Structure* oldStructure = node->transition()->previous;
        Structure* newStructure = node->transition()->next;

        m_jit.jitCode()->common.notifyCompilingStructureTransition(m_jit.graph().m_plan, m_jit.codeBlock(), node);

        SpeculateCellOperand base(this, node->child1());
        GPRReg baseGPR = base.gpr();
        
        ASSERT_UNUSED(oldStructure, oldStructure->indexingType() == newStructure->indexingType());
        ASSERT(oldStructure->typeInfo().type() == newStructure->typeInfo().type());
        ASSERT(oldStructure->typeInfo().inlineTypeFlags() == newStructure->typeInfo().inlineTypeFlags());
        m_jit.store32(MacroAssembler::TrustedImm32(newStructure->id()), MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()));
        
        noResult(node);
        break;
    }
        
    case AllocatePropertyStorage:
        compileAllocatePropertyStorage(node);
        break;
        
    case ReallocatePropertyStorage:
        compileReallocatePropertyStorage(node);
        break;
        
    case GetButterfly: {
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary result(this, Reuse, base);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::butterflyOffset()), resultGPR);
        
        storageResult(resultGPR, node);
        break;
    }

    case GetIndexedPropertyStorage: {
        compileGetIndexedPropertyStorage(node);
        break;
    }
        
    case ConstantStoragePointer: {
        compileConstantStoragePointer(node);
        break;
    }
        
    case GetTypedArrayByteOffset: {
        compileGetTypedArrayByteOffset(node);
        break;
    }
        
    case GetByOffset:
    case GetGetterSetterByOffset: {
        StorageOperand storage(this, node->child1());
        GPRTemporary result(this, Reuse, storage);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultGPR = result.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.load64(JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset)), resultGPR);
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case GetGetter: {
        SpeculateCellOperand op1(this, node->child1());
        GPRTemporary result(this, Reuse, op1);
        
        GPRReg op1GPR = op1.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(op1GPR, GetterSetter::offsetOfGetter()), resultGPR);
        
        cellResult(resultGPR, node);
        break;
    }
        
    case GetSetter: {
        SpeculateCellOperand op1(this, node->child1());
        GPRTemporary result(this, Reuse, op1);
        
        GPRReg op1GPR = op1.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(op1GPR, GetterSetter::offsetOfSetter()), resultGPR);
        
        cellResult(resultGPR, node);
        break;
    }
        
    case PutByOffset: {
        StorageOperand storage(this, node->child1());
        JSValueOperand value(this, node->child3());
        GPRTemporary scratch1(this);
        GPRTemporary scratch2(this);

        GPRReg storageGPR = storage.gpr();
        GPRReg valueGPR = value.gpr();

        speculate(node, node->child2());

        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.store64(valueGPR, JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset)));

        noResult(node);
        break;
    }

    case PutByIdFlush: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);

        GPRReg baseGPR = base.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratch.gpr();
        flushRegisters();

        cachedPutById(node->origin.semantic, baseGPR, valueGPR, scratchGPR, node->identifierNumber(), NotDirect, MacroAssembler::Jump(), DontSpill);

        noResult(node);
        break;
    }
        
    case PutById: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        cachedPutById(node->origin.semantic, baseGPR, valueGPR, scratchGPR, node->identifierNumber(), NotDirect);

        noResult(node);
        break;
    }

    case PutByIdDirect: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        cachedPutById(node->origin.semantic, baseGPR, valueGPR, scratchGPR, node->identifierNumber(), Direct);

        noResult(node);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary result(this);

        m_jit.load64(node->registerPointer(), result.gpr());

        jsValueResult(result.gpr(), node);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node->child1());

        m_jit.store64(value.gpr(), node->registerPointer());

        noResult(node);
        break;
    }

    case NotifyWrite: {
        VariableWatchpointSet* set = node->variableWatchpointSet();
    
        JSValueOperand value(this, node->child1());
        GPRReg valueGPR = value.gpr();
    
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
    
        m_jit.load8(set->addressOfState(), tempGPR);
    
        JITCompiler::Jump isDone =
            m_jit.branch32(JITCompiler::Equal, tempGPR, TrustedImm32(IsInvalidated));
        JITCompiler::Jump slowCase = m_jit.branch64(JITCompiler::NotEqual,
            JITCompiler::AbsoluteAddress(set->addressOfInferredValue()), valueGPR);
        isDone.link(&m_jit);
    
        addSlowPathGenerator(
            slowPathCall(slowCase, this, operationNotifyWrite, NoResult, set, valueGPR));

        noResult(node);
        break;
    }

    case VarInjectionWatchpoint:
    case VariableWatchpoint: {
        noResult(node);
        break;
    }

    case CheckHasInstance: {
        SpeculateCellOperand base(this, node->child1());
        GPRTemporary structure(this);

        // Speculate that base 'ImplementsDefaultHasInstance'.
        speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branchTest8(
            MacroAssembler::Zero, 
            MacroAssembler::Address(base.gpr(), JSCell::typeInfoFlagsOffset()), 
            MacroAssembler::TrustedImm32(ImplementsDefaultHasInstance)));

        noResult(node);
        break;
    }

    case InstanceOf: {
        compileInstanceOf(node);
        break;
    }
        
    case IsUndefined: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this);
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);
        GPRTemporary scratch(this);

        JITCompiler::Jump isCell = branchIsCell(value.jsValueRegs());

        m_jit.compare64(JITCompiler::Equal, value.gpr(), TrustedImm32(ValueUndefined), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        JITCompiler::Jump notMasqueradesAsUndefined;
        if (masqueradesAsUndefinedWatchpointIsStillValid()) {
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
        } else {
            JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(
                JITCompiler::NonZero, 
                JITCompiler::Address(value.gpr(), JSCell::typeInfoFlagsOffset()), 
                TrustedImm32(MasqueradesAsUndefined));
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();

            isMasqueradesAsUndefined.link(&m_jit);
            GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
            GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
            m_jit.move(TrustedImmPtr(m_jit.globalObjectFor(node->origin.semantic)), localGlobalObjectGPR);
            m_jit.emitLoadStructure(value.gpr(), result.gpr(), scratch.gpr());
            m_jit.loadPtr(JITCompiler::Address(result.gpr(), Structure::globalObjectOffset()), remoteGlobalObjectGPR); 
            m_jit.comparePtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, result.gpr());
        }

        notMasqueradesAsUndefined.link(&m_jit);
        done.link(&m_jit);
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsBoolean: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        
        m_jit.move(value.gpr(), result.gpr());
        m_jit.xor64(JITCompiler::TrustedImm32(ValueFalse), result.gpr());
        m_jit.test64(JITCompiler::Zero, result.gpr(), JITCompiler::TrustedImm32(static_cast<int32_t>(~1)), result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsNumber: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        
        m_jit.test64(JITCompiler::NonZero, value.gpr(), GPRInfo::tagTypeNumberRegister, result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsString: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        
        JITCompiler::Jump isNotCell = branchNotCell(value.jsValueRegs());
        
        m_jit.compare8(JITCompiler::Equal, 
            JITCompiler::Address(value.gpr(), JSCell::typeInfoTypeOffset()), 
            TrustedImm32(StringType), 
            result.gpr());
        m_jit.or32(TrustedImm32(ValueFalse), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isNotCell.link(&m_jit);
        m_jit.move(TrustedImm32(ValueFalse), result.gpr());
        
        done.link(&m_jit);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }
        
    case IsObject: {
        JSValueOperand value(this, node->child1());
        GPRReg valueGPR = value.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsObject, resultGPR, valueGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }

    case IsFunction: {
        JSValueOperand value(this, node->child1());
        GPRReg valueGPR = value.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsFunction, resultGPR, valueGPR);
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(result.gpr(), node, DataFormatJSBoolean);
        break;
    }

    case TypeOf: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg valueGPR = value.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        JITCompiler::JumpList doneJumps;

        flushRegisters();
        
        ASSERT(node->child1().useKind() == UntypedUse || node->child1().useKind() == CellUse || node->child1().useKind() == StringUse);

        JITCompiler::Jump isNotCell = branchNotCell(JSValueRegs(valueGPR));
        if (node->child1().useKind() != UntypedUse)
            DFG_TYPE_CHECK(JSValueSource(valueGPR), node->child1(), SpecCell, isNotCell);

        if (!node->child1()->shouldSpeculateObject() || node->child1().useKind() == StringUse) {
            JITCompiler::Jump notString = m_jit.branch8(
                JITCompiler::NotEqual, 
                JITCompiler::Address(valueGPR, JSCell::typeInfoTypeOffset()), 
                TrustedImm32(StringType));
            if (node->child1().useKind() == StringUse)
                DFG_TYPE_CHECK(JSValueSource(valueGPR), node->child1(), SpecString, notString);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.stringString()), resultGPR);
            doneJumps.append(m_jit.jump());
            if (node->child1().useKind() != StringUse) {
                notString.link(&m_jit);
                callOperation(operationTypeOf, resultGPR, valueGPR);
                doneJumps.append(m_jit.jump());
            }
        } else {
            callOperation(operationTypeOf, resultGPR, valueGPR);
            doneJumps.append(m_jit.jump());
        }

        if (node->child1().useKind() == UntypedUse) {
            isNotCell.link(&m_jit);
            JITCompiler::Jump notNumber = m_jit.branchTest64(JITCompiler::Zero, valueGPR, GPRInfo::tagTypeNumberRegister);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.numberString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNumber.link(&m_jit);

            JITCompiler::Jump notUndefined = m_jit.branch64(JITCompiler::NotEqual, valueGPR, JITCompiler::TrustedImm64(ValueUndefined));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.undefinedString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notUndefined.link(&m_jit);

            JITCompiler::Jump notNull = m_jit.branch64(JITCompiler::NotEqual, valueGPR, JITCompiler::TrustedImm64(ValueNull));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.objectString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNull.link(&m_jit);

            // Only boolean left
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.booleanString()), resultGPR);
        }
        doneJumps.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }

    case Flush:
        break;

    case Call:
    case Construct:
        emitCall(node);
        break;

    case CreateActivation: {
        DFG_ASSERT(m_jit.graph(), node, !node->origin.semantic.inlineCallFrame);
        
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(
                notCreated, this, operationCreateActivation, resultGPR,
                framePointerOffsetToGetActivationRegisters()));
        
        cellResult(resultGPR, node);
        break;
    }
        
    case FunctionReentryWatchpoint: {
        noResult(node);
        break;
    }
        
    case CreateArguments: {
        JSValueOperand value(this, node->child1());
        GPRTemporary scratch1(this);
        GPRTemporary scratch2(this);
        GPRTemporary result(this, Reuse, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg scratchGPR1 = scratch1.gpr();
        GPRReg scratchGPR2 = scratch2.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        if (node->origin.semantic.inlineCallFrame) {
            JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
            addSlowPathGenerator(
                slowPathCall(
                    notCreated, this, operationCreateInlinedArguments, resultGPR,
                    node->origin.semantic.inlineCallFrame));
            cellResult(resultGPR, node);
            break;
        } 

        FunctionExecutable* executable = jsCast<FunctionExecutable*>(m_jit.graph().executableFor(node->origin.semantic));
        if (m_jit.codeBlock()->hasSlowArguments()
            || executable->isStrictMode() 
            || !executable->parameterCount()) {
            JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
            addSlowPathGenerator(
                slowPathCall(notCreated, this, operationCreateArguments, resultGPR));
            cellResult(resultGPR, node);
            break;
        }

        JITCompiler::Jump alreadyCreated = m_jit.branchTest64(JITCompiler::NonZero, resultGPR);

        MacroAssembler::JumpList slowPaths;
        emitAllocateArguments(resultGPR, scratchGPR1, scratchGPR2, slowPaths);
        addSlowPathGenerator(
            slowPathCall(slowPaths, this, operationCreateArguments, resultGPR));

        alreadyCreated.link(&m_jit);
        cellResult(resultGPR, node);
        break;
    }

    case TearOffActivation: {
        DFG_ASSERT(m_jit.graph(), node, !node->origin.semantic.inlineCallFrame);

        JSValueOperand activationValue(this, node->child1());
        GPRTemporary scratch(this);
        GPRReg activationValueGPR = activationValue.gpr();
        GPRReg scratchGPR = scratch.gpr();

        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, activationValueGPR);

        SymbolTable* symbolTable = m_jit.symbolTableFor(node->origin.semantic);
        int registersOffset = JSActivation::registersOffset(symbolTable);

        int bytecodeCaptureStart = symbolTable->captureStart();
        int machineCaptureStart = m_jit.graph().m_machineCaptureStart;
        for (int i = symbolTable->captureCount(); i--;) {
            m_jit.load64(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister,
                    (machineCaptureStart - i) * sizeof(Register)),
                scratchGPR);
            m_jit.store64(
                scratchGPR,
                JITCompiler::Address(
                    activationValueGPR,
                    registersOffset + (bytecodeCaptureStart - i) * sizeof(Register)));
        }
        m_jit.addPtr(TrustedImm32(registersOffset), activationValueGPR, scratchGPR);
        m_jit.storePtr(scratchGPR, JITCompiler::Address(activationValueGPR, JSActivation::offsetOfRegisters()));

        notCreated.link(&m_jit);
        noResult(node);
        break;
    }

    case TearOffArguments: {
        JSValueOperand unmodifiedArgumentsValue(this, node->child1());
        JSValueOperand activationValue(this, node->child2());
        GPRReg unmodifiedArgumentsValueGPR = unmodifiedArgumentsValue.gpr();
        GPRReg activationValueGPR = activationValue.gpr();

        JITCompiler::Jump created = m_jit.branchTest64(JITCompiler::NonZero, unmodifiedArgumentsValueGPR);

        if (node->origin.semantic.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffInlinedArguments, NoResult,
                    unmodifiedArgumentsValueGPR, activationValueGPR, node->origin.semantic.inlineCallFrame));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffArguments, NoResult, unmodifiedArgumentsValueGPR, activationValueGPR));
        }
        
        noResult(node);
        break;
    }
        
    case GetMyArgumentsLength: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->origin.semantic)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branchTest64(
                    JITCompiler::NonZero,
                    JITCompiler::addressFor(
                        m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic))));
        }
        
        DFG_ASSERT(m_jit.graph(), node, !node->origin.semantic.inlineCallFrame);
        m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
        m_jit.sub32(TrustedImm32(1), resultGPR);
        int32Result(resultGPR, node);
        break;
    }
        
    case GetMyArgumentsLengthSafe: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
        
        JITCompiler::Jump created = m_jit.branchTest64(
            JITCompiler::NonZero,
            JITCompiler::addressFor(
                m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)));
        
        if (node->origin.semantic.inlineCallFrame) {
            m_jit.move(
                Imm64(JSValue::encode(jsNumber(node->origin.semantic.inlineCallFrame->arguments.size() - 1))),
                resultGPR);
        } else {
            m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
            m_jit.sub32(TrustedImm32(1), resultGPR);
            m_jit.or64(GPRInfo::tagTypeNumberRegister, resultGPR);
        }
        
        // FIXME: the slow path generator should perform a forward speculation that the
        // result is an integer. For now we postpone the speculation by having this return
        // a JSValue.
        
        addSlowPathGenerator(
            slowPathCall(
                created, this, operationGetArgumentsLength, resultGPR,
                m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset()));
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentByVal: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary result(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultGPR = result.gpr();

        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->origin.semantic)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branchTest64(
                    JITCompiler::NonZero,
                    JITCompiler::addressFor(
                        m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic))));
        }

        m_jit.add32(TrustedImm32(1), indexGPR, resultGPR);
        if (node->origin.semantic.inlineCallFrame) {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    Imm32(node->origin.semantic.inlineCallFrame->arguments.size())));
        } else {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }

        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (m_jit.symbolTableFor(node->origin.semantic)->slowArguments()) {
            DFG_ASSERT(m_jit.graph(), node, !node->origin.semantic.inlineCallFrame);
            const SlowArgument* slowArguments = m_jit.graph().m_slowArguments.get();
            
            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->origin.semantic)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)),
                resultGPR);
            m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            m_jit.load64(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight),
                resultGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            
        m_jit.load64(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic)),
            resultGPR);

        slowArgument.link(&m_jit);
        jsValueResult(resultGPR, node);
        break;
    }
        
    case GetMyArgumentByValSafe: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary result(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultGPR = result.gpr();
        
        JITCompiler::JumpList slowPath;
        slowPath.append(
            m_jit.branchTest64(
                JITCompiler::NonZero,
                JITCompiler::addressFor(
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic))));
        
        m_jit.add32(TrustedImm32(1), indexGPR, resultGPR);
        if (node->origin.semantic.inlineCallFrame) {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    Imm32(node->origin.semantic.inlineCallFrame->arguments.size())));
        } else {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }
        
        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (m_jit.symbolTableFor(node->origin.semantic)->slowArguments()) {
            DFG_ASSERT(m_jit.graph(), node, !node->origin.semantic.inlineCallFrame);
            const SlowArgument* slowArguments = m_jit.graph().m_slowArguments.get();

            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->origin.semantic)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultGPR);
            m_jit.signExtend32ToPtr(resultGPR, resultGPR);
            m_jit.load64(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight),
                resultGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.signExtend32ToPtr(resultGPR, resultGPR);
        
        m_jit.load64(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultGPR, JITCompiler::TimesEight, m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic)),
            resultGPR);
        
        if (node->origin.semantic.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetInlinedArgumentByVal, resultGPR, 
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset(),
                    node->origin.semantic.inlineCallFrame,
                    indexGPR));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetArgumentByVal, resultGPR, 
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset(),
                    indexGPR));
        }
        
        slowArgument.link(&m_jit);
        jsValueResult(resultGPR, node);
        break;
    }
        
    case CheckArgumentsNotCreated: {
        ASSERT(!isEmptySpeculation(
            m_state.variables().operand(
                m_jit.graph().argumentsRegisterFor(node->origin.semantic)).m_type));
        speculationCheck(
            ArgumentsEscaped, JSValueRegs(), 0,
            m_jit.branchTest64(
                JITCompiler::NonZero,
                JITCompiler::addressFor(
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic))));
        noResult(node);
        break;
    }
        
    case NewFunctionNoCheck:
        compileNewFunctionNoCheck(node);
        break;
        
    case NewFunction: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        
        GPRReg valueGPR = value.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valueGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branchTest64(JITCompiler::Zero, resultGPR);
        
        addSlowPathGenerator(
            slowPathCall(
                notCreated, this, operationNewFunction,
                resultGPR, m_jit.codeBlock()->functionDecl(node->functionDeclIndex())));
        
        jsValueResult(resultGPR, node);
        break;
    }
        
    case NewFunctionExpression:
        compileNewFunctionExpression(node);
        break;
        
    case In:
        compileIn(node);
        break;
        
    case CountExecution:
        m_jit.add64(TrustedImm32(1), MacroAssembler::AbsoluteAddress(node->executionCounter()->address()));
        break;

    case ForceOSRExit: {
        terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
        break;
    }
        
    case InvalidationPoint:
        emitInvalidationPoint(node);
        break;

    case CheckWatchdogTimer:
        ASSERT(m_jit.vm()->watchdog);
        speculationCheck(
            WatchdogTimerFired, JSValueRegs(), 0,
            m_jit.branchTest8(
                JITCompiler::NonZero,
                JITCompiler::AbsoluteAddress(m_jit.vm()->watchdog->timerDidFireAddress())));
        break;

    case Phantom:
    case HardPhantom:
        DFG_NODE_DO_TO_CHILDREN(m_jit.graph(), node, speculate);
        noResult(node);
        break;
        
    case Breakpoint:
    case ProfileWillCall:
    case ProfileDidCall:
    case PhantomLocal:
    case LoopHint:
        // This is a no-op.
        noResult(node);
        break;

    case Unreachable:
        DFG_CRASH(m_jit.graph(), node, "Unexpected Unreachable node");
        break;

    case StoreBarrier:
    case StoreBarrierWithNullCheck: {
        compileStoreBarrier(node);
        break;
    }

#if ENABLE(FTL_JIT)        
    case CheckTierUpInLoop: {
        MacroAssembler::Jump done = m_jit.branchAdd32(
            MacroAssembler::Signed,
            TrustedImm32(Options::ftlTierUpCounterIncrementForLoop()),
            MacroAssembler::AbsoluteAddress(&m_jit.jitCode()->tierUpCounter.m_counter));
        
        silentSpillAllRegisters(InvalidGPRReg);
        m_jit.setupArgumentsExecState();
        appendCall(triggerTierUpNow);
        silentFillAllRegisters(InvalidGPRReg);
        
        done.link(&m_jit);
        break;
    }
        
    case CheckTierUpAtReturn: {
        MacroAssembler::Jump done = m_jit.branchAdd32(
            MacroAssembler::Signed,
            TrustedImm32(Options::ftlTierUpCounterIncrementForReturn()),
            MacroAssembler::AbsoluteAddress(&m_jit.jitCode()->tierUpCounter.m_counter));
        
        silentSpillAllRegisters(InvalidGPRReg);
        m_jit.setupArgumentsExecState();
        appendCall(triggerTierUpNow);
        silentFillAllRegisters(InvalidGPRReg);
        
        done.link(&m_jit);
        break;
    }
        
    case CheckTierUpAndOSREnter: {
        ASSERT(!node->origin.semantic.inlineCallFrame);
        
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        
        MacroAssembler::Jump done = m_jit.branchAdd32(
            MacroAssembler::Signed,
            TrustedImm32(Options::ftlTierUpCounterIncrementForLoop()),
            MacroAssembler::AbsoluteAddress(&m_jit.jitCode()->tierUpCounter.m_counter));
        
        silentSpillAllRegisters(tempGPR);
        m_jit.setupArgumentsWithExecState(
            TrustedImm32(node->origin.semantic.bytecodeIndex),
            TrustedImm32(m_stream->size()));
        appendCallSetResult(triggerOSREntryNow, tempGPR);
        MacroAssembler::Jump dontEnter = m_jit.branchTestPtr(MacroAssembler::Zero, tempGPR);
        m_jit.jump(tempGPR);
        dontEnter.link(&m_jit);
        silentFillAllRegisters(tempGPR);
        
        done.link(&m_jit);
        break;
    }
#else // ENABLE(FTL_JIT)
    case CheckTierUpInLoop:
    case CheckTierUpAtReturn:
    case CheckTierUpAndOSREnter:
        DFG_CRASH(m_jit.graph(), node, "Unexpected tier-up node");
        break;
#endif // ENABLE(FTL_JIT)

    case NativeCall:
    case NativeConstruct:    
    case LastNodeType:
    case Phi:
    case Upsilon:
    case GetArgument:
    case ExtractOSREntryLocal:
    case CheckInBounds:
    case ArithIMul:
    case MultiGetByOffset:
    case MultiPutByOffset:
    case FiatInt52:
        DFG_CRASH(m_jit.graph(), node, "Unexpected FTL node");
        break;
    }

    if (!m_compileOkay)
        return;
    
    if (node->hasResult() && node->mustGenerate())
        use(node);
}

#if ENABLE(GGC)
void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, Edge valueUse, GPRReg scratch1, GPRReg scratch2)
{
    JITCompiler::Jump isNotCell;
    if (!isKnownCell(valueUse.node()))
        isNotCell = branchNotCell(JSValueRegs(valueGPR));
    
    JITCompiler::Jump ownerNotMarkedOrAlreadyRemembered = m_jit.checkMarkByte(ownerGPR);
    storeToWriteBarrierBuffer(ownerGPR, scratch1, scratch2);
    ownerNotMarkedOrAlreadyRemembered.link(&m_jit);

    if (!isKnownCell(valueUse.node()))
        isNotCell.link(&m_jit);
}

void SpeculativeJIT::writeBarrier(JSCell* owner, GPRReg valueGPR, Edge valueUse, GPRReg scratch1, GPRReg scratch2)
{
    JITCompiler::Jump isNotCell;
    if (!isKnownCell(valueUse.node()))
        isNotCell = branchNotCell(JSValueRegs(valueGPR));
    
    JITCompiler::Jump ownerNotMarkedOrAlreadyRemembered = m_jit.checkMarkByte(owner);
    storeToWriteBarrierBuffer(owner, scratch1, scratch2);
    ownerNotMarkedOrAlreadyRemembered.link(&m_jit);

    if (!isKnownCell(valueUse.node()))
        isNotCell.link(&m_jit);
}
#endif // ENABLE(GGC)

JITCompiler::Jump SpeculativeJIT::branchIsCell(JSValueRegs regs)
{
    return m_jit.branchTest64(MacroAssembler::Zero, regs.gpr(), GPRInfo::tagMaskRegister);
}

JITCompiler::Jump SpeculativeJIT::branchNotCell(JSValueRegs regs)
{
    return m_jit.branchTest64(MacroAssembler::NonZero, regs.gpr(), GPRInfo::tagMaskRegister);
}

JITCompiler::Jump SpeculativeJIT::branchIsOther(JSValueRegs regs, GPRReg tempGPR)
{
    m_jit.move(regs.gpr(), tempGPR);
    m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), tempGPR);
    return m_jit.branch64(
        MacroAssembler::Equal, tempGPR,
        MacroAssembler::TrustedImm64(ValueNull));
}

JITCompiler::Jump SpeculativeJIT::branchNotOther(JSValueRegs regs, GPRReg tempGPR)
{
    m_jit.move(regs.gpr(), tempGPR);
    m_jit.and64(MacroAssembler::TrustedImm32(~TagBitUndefined), tempGPR);
    return m_jit.branch64(
        MacroAssembler::NotEqual, tempGPR,
        MacroAssembler::TrustedImm64(ValueNull));
}

void SpeculativeJIT::moveTrueTo(GPRReg gpr)
{
    m_jit.move(TrustedImm32(ValueTrue), gpr);
}

void SpeculativeJIT::moveFalseTo(GPRReg gpr)
{
    m_jit.move(TrustedImm32(ValueFalse), gpr);
}

void SpeculativeJIT::blessBoolean(GPRReg gpr)
{
    m_jit.or32(TrustedImm32(ValueFalse), gpr);
}

void SpeculativeJIT::convertMachineInt(Edge valueEdge, GPRReg resultGPR)
{
    JSValueOperand value(this, valueEdge, ManualOperandSpeculation);
    GPRReg valueGPR = value.gpr();
    
    JITCompiler::Jump notInt32 =
        m_jit.branch64(JITCompiler::Below, valueGPR, GPRInfo::tagTypeNumberRegister);
    
    m_jit.signExtend32ToPtr(valueGPR, resultGPR);
    JITCompiler::Jump done = m_jit.jump();
    
    notInt32.link(&m_jit);
    silentSpillAllRegisters(resultGPR);
    callOperation(operationConvertBoxedDoubleToInt52, resultGPR, valueGPR);
    silentFillAllRegisters(resultGPR);

    DFG_TYPE_CHECK(
        JSValueRegs(valueGPR), valueEdge, SpecInt32 | SpecInt52AsDouble,
        m_jit.branch64(
            JITCompiler::Equal, resultGPR,
            JITCompiler::TrustedImm64(JSValue::notInt52)));
    done.link(&m_jit);
}

void SpeculativeJIT::speculateMachineInt(Edge edge)
{
    if (!needsTypeCheck(edge, SpecInt32 | SpecInt52AsDouble))
        return;
    
    GPRTemporary temp(this);
    convertMachineInt(edge, temp.gpr());
}

void SpeculativeJIT::speculateDoubleRepMachineInt(Edge edge)
{
    if (!needsTypeCheck(edge, SpecInt52AsDouble))
        return;
    
    SpeculateDoubleOperand value(this, edge);
    FPRReg valueFPR = value.fpr();
    
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();
    
    flushRegisters();
    
    callOperation(operationConvertDoubleToInt52, resultGPR, valueFPR);
    
    DFG_TYPE_CHECK(
        JSValueRegs(), edge, SpecInt52AsDouble,
        m_jit.branch64(
            JITCompiler::Equal, resultGPR,
            JITCompiler::TrustedImm64(JSValue::notInt52)));
}

#endif

} } // namespace JSC::DFG

#endif
