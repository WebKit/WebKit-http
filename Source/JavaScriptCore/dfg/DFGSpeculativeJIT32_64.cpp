/*
 * Copyright (C) 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Intel Corporation. All rights reserved.
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

#include "ArrayPrototype.h"
#include "DFGAbstractInterpreterInlines.h"
#include "DFGCallArrayAllocatorSlowPathGenerator.h"
#include "DFGOperations.h"
#include "DFGSlowPathGenerator.h"
#include "Debugger.h"
#include "GetterSetter.h"
#include "JSActivation.h"
#include "JSPropertyNameEnumerator.h"
#include "ObjectPrototype.h"
#include "JSCInlines.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE32_64)

bool SpeculativeJIT::fillJSValue(Edge edge, GPRReg& tagGPR, GPRReg& payloadGPR, FPRReg& fpr)
{
    // FIXME: For double we could fill with a FPR.
    UNUSED_PARAM(fpr);

    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    switch (info.registerFormat()) {
    case DataFormatNone: {

        if (edge->hasConstant()) {
            tagGPR = allocate();
            payloadGPR = allocate();
            JSValue value = edge->asJSValue();
            m_jit.move(Imm32(value.tag()), tagGPR);
            m_jit.move(Imm32(value.payload()), payloadGPR);
            m_gprs.retain(tagGPR, virtualRegister, SpillOrderConstant);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderConstant);
            info.fillJSValue(*m_stream, tagGPR, payloadGPR, DataFormatJS);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat != DataFormatNone && spillFormat != DataFormatStorage);
            tagGPR = allocate();
            payloadGPR = allocate();
            switch (spillFormat) {
            case DataFormatInt32:
                m_jit.move(TrustedImm32(JSValue::Int32Tag), tagGPR);
                spillFormat = DataFormatJSInt32; // This will be used as the new register format.
                break;
            case DataFormatCell:
                m_jit.move(TrustedImm32(JSValue::CellTag), tagGPR);
                spillFormat = DataFormatJSCell; // This will be used as the new register format.
                break;
            case DataFormatBoolean:
                m_jit.move(TrustedImm32(JSValue::BooleanTag), tagGPR);
                spillFormat = DataFormatJSBoolean; // This will be used as the new register format.
                break;
            default:
                m_jit.load32(JITCompiler::tagFor(virtualRegister), tagGPR);
                break;
            }
            m_jit.load32(JITCompiler::payloadFor(virtualRegister), payloadGPR);
            m_gprs.retain(tagGPR, virtualRegister, SpillOrderSpilled);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderSpilled);
            info.fillJSValue(*m_stream, tagGPR, payloadGPR, spillFormat == DataFormatJSDouble ? DataFormatJS : spillFormat);
        }

        return true;
    }

    case DataFormatInt32:
    case DataFormatCell:
    case DataFormatBoolean: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        if (m_gprs.isLocked(gpr)) {
            payloadGPR = allocate();
            m_jit.move(gpr, payloadGPR);
        } else {
            payloadGPR = gpr;
            m_gprs.lock(gpr);
        }
        tagGPR = allocate();
        uint32_t tag = JSValue::EmptyValueTag;
        DataFormat fillFormat = DataFormatJS;
        switch (info.registerFormat()) {
        case DataFormatInt32:
            tag = JSValue::Int32Tag;
            fillFormat = DataFormatJSInt32;
            break;
        case DataFormatCell:
            tag = JSValue::CellTag;
            fillFormat = DataFormatJSCell;
            break;
        case DataFormatBoolean:
            tag = JSValue::BooleanTag;
            fillFormat = DataFormatJSBoolean;
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        m_jit.move(TrustedImm32(tag), tagGPR);
        m_gprs.release(gpr);
        m_gprs.retain(tagGPR, virtualRegister, SpillOrderJS);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderJS);
        info.fillJSValue(*m_stream, tagGPR, payloadGPR, fillFormat);
        return true;
    }

    case DataFormatJSDouble:
    case DataFormatJS:
    case DataFormatJSInt32:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        tagGPR = info.tagGPR();
        payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        return true;
    }
        
    case DataFormatStorage:
    case DataFormatDouble:
        // this type currently never occurs
        RELEASE_ASSERT_NOT_REACHED();

    default:
        RELEASE_ASSERT_NOT_REACHED();
        return true;
    }
}

void SpeculativeJIT::cachedGetById(
    CodeOrigin codeOrigin, GPRReg baseTagGPROrNone, GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR,
    unsigned identifierNumber, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    // This is a hacky fix for when the register allocator decides to alias the base payload with the result tag. This only happens
    // in the case of GetByIdFlush, which has a relatively expensive register allocation story already so we probably don't need to
    // trip over one move instruction.
    if (basePayloadGPR == resultTagGPR) {
        RELEASE_ASSERT(basePayloadGPR != resultPayloadGPR);
        
        if (baseTagGPROrNone == resultPayloadGPR) {
            m_jit.swap(basePayloadGPR, baseTagGPROrNone);
            baseTagGPROrNone = resultTagGPR;
        } else
            m_jit.move(basePayloadGPR, resultPayloadGPR);
        basePayloadGPR = resultPayloadGPR;
    }
    
    JITGetByIdGenerator gen(
        m_jit.codeBlock(), codeOrigin, usedRegisters(),
        JSValueRegs(baseTagGPROrNone, basePayloadGPR),
        JSValueRegs(resultTagGPR, resultPayloadGPR), spillMode);
    
    gen.generateFastPath(m_jit);
    
    JITCompiler::JumpList slowCases;
    if (slowPathTarget.isSet())
        slowCases.append(slowPathTarget);
    slowCases.append(gen.slowPathJump());
    
    OwnPtr<SlowPathGenerator> slowPath;
    if (baseTagGPROrNone == InvalidGPRReg) {
        slowPath = slowPathCall(
            slowCases, this, operationGetByIdOptimize,
            JSValueRegs(resultTagGPR, resultPayloadGPR), gen.stubInfo(),
            static_cast<int32_t>(JSValue::CellTag), basePayloadGPR,
            identifierUID(identifierNumber));
    } else {
        slowPath = slowPathCall(
            slowCases, this, operationGetByIdOptimize,
            JSValueRegs(resultTagGPR, resultPayloadGPR), gen.stubInfo(), baseTagGPROrNone,
            basePayloadGPR, identifierUID(identifierNumber));
    }
    
    m_jit.addGetById(gen, slowPath.get());
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::cachedPutById(CodeOrigin codeOrigin, GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget, SpillRegistersMode spillMode)
{
    JITPutByIdGenerator gen(
        m_jit.codeBlock(), codeOrigin, usedRegisters(),
        JSValueRegs::payloadOnly(basePayloadGPR), JSValueRegs(valueTagGPR, valuePayloadGPR),
        scratchGPR, spillMode, m_jit.ecmaModeFor(codeOrigin), putKind);
    
    gen.generateFastPath(m_jit);
    
    JITCompiler::JumpList slowCases;
    if (slowPathTarget.isSet())
        slowCases.append(slowPathTarget);
    slowCases.append(gen.slowPathJump());

    OwnPtr<SlowPathGenerator> slowPath = slowPathCall(
        slowCases, this, gen.slowPathFunction(), NoResult, gen.stubInfo(), valueTagGPR,
        valuePayloadGPR, basePayloadGPR, identifierUID(identifierNumber));

    m_jit.addPutById(gen, slowPath.get());
    addSlowPathGenerator(slowPath.release());
}

void SpeculativeJIT::nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();

    GPRTemporary resultPayload(this, Reuse, arg, PayloadWord);
    GPRReg resultPayloadGPR = resultPayload.gpr();

    JITCompiler::Jump notCell;
    JITCompiler::Jump notMasqueradesAsUndefined;   
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(arg.jsValueRegs());
        
        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultPayloadGPR);
        notMasqueradesAsUndefined = m_jit.jump();
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(arg.jsValueRegs());
        
        JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(
            JITCompiler::NonZero, 
            JITCompiler::Address(argPayloadGPR, JSCell::typeInfoFlagsOffset()), 
            JITCompiler::TrustedImm32(MasqueradesAsUndefined));
        
        m_jit.move(invert ? TrustedImm32(1) : TrustedImm32(0), resultPayloadGPR);
        notMasqueradesAsUndefined = m_jit.jump();

        isMasqueradesAsUndefined.link(&m_jit);
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureIDOffset()), resultPayloadGPR);
        m_jit.loadPtr(JITCompiler::Address(resultPayloadGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        m_jit.compare32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, resultPayloadGPR);
    }
 
    if (!isKnownCell(operand.node())) {
        JITCompiler::Jump done = m_jit.jump();
        
        notCell.link(&m_jit);
        // null or undefined?
        COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
        m_jit.or32(TrustedImm32(1), argTagGPR, resultPayloadGPR);
        m_jit.compare32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultPayloadGPR, TrustedImm32(JSValue::NullTag), resultPayloadGPR);

        done.link(&m_jit);
    }
    
    notMasqueradesAsUndefined.link(&m_jit);
 
    booleanResult(resultPayloadGPR, m_currentNode);
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
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();
    
    GPRTemporary result(this, Reuse, arg, TagWord);
    GPRReg resultGPR = result.gpr();

    JITCompiler::Jump notCell;

    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(arg.jsValueRegs());
        
        jump(invert ? taken : notTaken, ForceJump);
    } else {
        GPRTemporary localGlobalObject(this);
        GPRTemporary remoteGlobalObject(this);

        if (!isKnownCell(operand.node()))
            notCell = branchNotCell(arg.jsValueRegs());
        
        branchTest8(JITCompiler::Zero, 
            JITCompiler::Address(argPayloadGPR, JSCell::typeInfoFlagsOffset()), 
            JITCompiler::TrustedImm32(MasqueradesAsUndefined), 
            invert ? taken : notTaken);
   
        GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
        GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
        m_jit.move(TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic)), localGlobalObjectGPR);
        m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureIDOffset()), resultGPR);
        m_jit.loadPtr(JITCompiler::Address(resultGPR, Structure::globalObjectOffset()), remoteGlobalObjectGPR);
        branchPtr(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, invert ? notTaken : taken);
    }
 
    if (!isKnownCell(operand.node())) {
        jump(notTaken, ForceJump);
        
        notCell.link(&m_jit);
        // null or undefined?
        COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
        m_jit.or32(TrustedImm32(1), argTagGPR, resultGPR);
        branch32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm32(JSValue::NullTag), taken);
    }
    
    jump(notTaken);
}

bool SpeculativeJIT::nonSpeculativeCompareNull(Node* node, Edge operand, bool invert)
{
    unsigned branchIndexInBlock = detectPeepHoleBranch();
    if (branchIndexInBlock != UINT_MAX) {
        Node* branchNode = m_block->at(branchIndexInBlock);

        ASSERT(node->adjustedRefCount() == 1);
        
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
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();

        flushRegisters();
        callOperation(helperFunction, resultGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);

        branchTest32(callResultCondition, resultGPR, taken);
    } else {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();

        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
    
        branch32(cond, arg1PayloadGPR, arg2PayloadGPR, taken);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            jump(notTaken, ForceJump);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            callOperation(helperFunction, resultGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
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
        S_JITOperation_EJJ function, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload,
        GPRReg arg2Tag, GPRReg arg2Payload)
        : CallSlowPathGenerator<JumpType, S_JITOperation_EJJ, GPRReg>(
            from, jit, function, NeedToSpill, result)
        , m_arg1Tag(arg1Tag)
        , m_arg1Payload(arg1Payload)
        , m_arg2Tag(arg2Tag)
        , m_arg2Payload(arg2Payload)
    {
    }
    
protected:
    virtual void generateInternal(SpeculativeJIT* jit)
    {
        this->setUp(jit);
        this->recordCall(
            jit->callOperation(
                this->m_function, this->m_result, m_arg1Tag, m_arg1Payload, m_arg2Tag,
                m_arg2Payload));
        jit->m_jit.and32(JITCompiler::TrustedImm32(1), this->m_result);
        this->tearDown(jit);
    }
   
private:
    GPRReg m_arg1Tag;
    GPRReg m_arg1Payload;
    GPRReg m_arg2Tag;
    GPRReg m_arg2Payload;
};

void SpeculativeJIT::nonSpeculativeNonPeepholeCompare(Node* node, MacroAssembler::RelationalCondition cond, S_JITOperation_EJJ helperFunction)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node->child1().node()) || isKnownNotInteger(node->child2().node())) {
        GPRResult result(this);
        GPRReg resultPayloadGPR = result.gpr();
    
        arg1.use();
        arg2.use();

        flushRegisters();
        callOperation(helperFunction, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary resultPayload(this, Reuse, arg1, PayloadWord);
        GPRReg resultPayloadGPR = resultPayload.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node->child1().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node->child2().node()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));

        m_jit.compare32(cond, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR);
    
        if (!isKnownInteger(node->child1().node()) || !isKnownInteger(node->child2().node())) {
            addSlowPathGenerator(adoptPtr(
                new CompareAndBoxBooleanSlowPathGenerator<JITCompiler::JumpList>(
                    slowPath, this, helperFunction, resultPayloadGPR, arg1TagGPR,
                    arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR)));
        }
        
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
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
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultPayload(this, Reuse, arg1, PayloadWord);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        branchPtr(JITCompiler::Equal, arg1PayloadGPR, arg2PayloadGPR, invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEqCell, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultPayloadGPR, taken);
    } else {
        // FIXME: Add fast paths for twoCells, number etc.

        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEq, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        branchTest32(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultPayloadGPR, taken);
    }
    
    jump(notTaken);
}

void SpeculativeJIT::nonSpeculativeNonPeepholeStrictEq(Node* node, bool invert)
{
    JSValueOperand arg1(this, node->child1());
    JSValueOperand arg2(this, node->child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultPayload(this, Reuse, arg1, PayloadWord);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node->child1().node()) && isKnownCell(node->child2().node())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        // FIXME: this should flush registers instead of silent spill/fill.
        JITCompiler::Jump notEqualCase = m_jit.branchPtr(JITCompiler::NotEqual, arg1PayloadGPR, arg2PayloadGPR);
        
        m_jit.move(JITCompiler::TrustedImm32(!invert), resultPayloadGPR);
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEqCell, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
        
        done.link(&m_jit);
    } else {
        // FIXME: Add fast paths.

        silentSpillAllRegisters(resultPayloadGPR);
        callOperation(operationCompareStrictEq, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
    }

    booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
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
    
    m_jit.move(TrustedImm32(0), result.gpr());
    JITCompiler::Jump notEqual = m_jit.branch32(JITCompiler::NotEqual, op1.tagGPR(), op2.tagGPR());
    m_jit.compare32(JITCompiler::Equal, op1.payloadGPR(), op2.payloadGPR(), result.gpr());
    notEqual.link(&m_jit);
    booleanResult(result.gpr(), node);
}

void SpeculativeJIT::emitCall(Node* node)
{
    bool isCall = node->op() == Call || node->op() == ProfiledCall;
    if (!isCall)
        ASSERT(node->op() == Construct || node->op() == ProfiledConstruct);

    // For constructors, the this argument is not passed but we have to make space
    // for it.
    int dummyThisArgument = isCall ? 0 : 1;

    CallLinkInfo::CallType callType = isCall ? CallLinkInfo::Call : CallLinkInfo::Construct;

    Edge calleeEdge = m_jit.graph().m_varArgChildren[node->firstChild()];
    JSValueOperand callee(this, calleeEdge);
    GPRReg calleeTagGPR = callee.tagGPR();
    GPRReg calleePayloadGPR = callee.payloadGPR();
    use(calleeEdge);

    // The call instruction's first child is either the function (normal call) or the
    // receiver (method call). subsequent children are the arguments.
    int numPassedArgs = node->numChildren() - 1;
    
    int numArgs = numPassedArgs + dummyThisArgument;

    m_jit.store32(MacroAssembler::TrustedImm32(numArgs), calleeFramePayloadSlot(JSStack::ArgumentCount));
    m_jit.store32(calleePayloadGPR, calleeFramePayloadSlot(JSStack::Callee));
    m_jit.store32(calleeTagGPR, calleeFrameTagSlot(JSStack::Callee));

    for (int i = 0; i < numPassedArgs; i++) {
        Edge argEdge = m_jit.graph().m_varArgChildren[node->firstChild() + 1 + i];
        JSValueOperand arg(this, argEdge);
        GPRReg argTagGPR = arg.tagGPR();
        GPRReg argPayloadGPR = arg.payloadGPR();
        use(argEdge);

        m_jit.store32(argTagGPR, calleeArgumentTagSlot(i + dummyThisArgument));
        m_jit.store32(argPayloadGPR, calleeArgumentPayloadSlot(i + dummyThisArgument));
    }

    flushRegisters();

    GPRResult resultPayload(this);
    GPRResult2 resultTag(this);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    GPRReg resultTagGPR = resultTag.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::JumpList slowPath;

    m_jit.emitStoreCodeOrigin(node->origin.semantic);
    
    CallLinkInfo* info = m_jit.codeBlock()->addCallLinkInfo();

    if (node->op() == ProfiledCall || node->op() == ProfiledConstruct) {
        m_jit.vm()->callEdgeLog->emitLogCode(
            m_jit, info->callEdgeProfile, callee.jsValueRegs());
    }
    
    slowPath.append(branchNotCell(callee.jsValueRegs()));
    slowPath.append(m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleePayloadGPR, targetToCheck));
    m_jit.loadPtr(MacroAssembler::Address(calleePayloadGPR, OBJECT_OFFSETOF(JSFunction, m_scope)), resultPayloadGPR);
    m_jit.storePtr(resultPayloadGPR, calleeFramePayloadSlot(JSStack::ScopeChain));
    m_jit.storePtr(MacroAssembler::TrustedImm32(JSValue::CellTag), calleeFrameTagSlot(JSStack::ScopeChain));

    JITCompiler::Call fastCall = m_jit.nearCall();

    JITCompiler::Jump done = m_jit.jump();

    slowPath.link(&m_jit);

    // Callee payload needs to be in regT0, tag in regT1
    if (calleeTagGPR == GPRInfo::regT0) {
        if (calleePayloadGPR == GPRInfo::regT1)
            m_jit.swap(GPRInfo::regT1, GPRInfo::regT0);
        else {
            m_jit.move(calleeTagGPR, GPRInfo::regT1);
            m_jit.move(calleePayloadGPR, GPRInfo::regT0);
        }
    } else {
        m_jit.move(calleePayloadGPR, GPRInfo::regT0);
        m_jit.move(calleeTagGPR, GPRInfo::regT1);
    }
    m_jit.move(MacroAssembler::TrustedImmPtr(info), GPRInfo::regT2);
    JITCompiler::Call slowCall = m_jit.nearCall();

    done.link(&m_jit);

    m_jit.setupResults(resultPayloadGPR, resultTagGPR);

    jsValueResult(resultTagGPR, resultPayloadGPR, node, DataFormatJS, UseChildrenCalledExplicitly);

    info->callType = callType;
    info->codeOrigin = node->origin.semantic;
    info->calleeGPR = calleePayloadGPR;
    m_jit.addJSCall(fastCall, slowCall, targetToCheck, info);
}

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
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInt32;
        return allocate();
    }
    
    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (edge->hasConstant()) {
            ASSERT(edge->isInt32Constant());
            GPRReg gpr = allocate();
            m_jit.move(MacroAssembler::Imm32(edge->asInt32()), gpr);
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            info.fillInt32(*m_stream, gpr);
            returnFormat = DataFormatInt32;
            return gpr;
        }

        DataFormat spillFormat = info.spillFormat();
        ASSERT_UNUSED(spillFormat, (spillFormat & DataFormatJS) || spillFormat == DataFormatInt32);

        // If we know this was spilled as an integer we can fill without checking.
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag)));

        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillInt32(*m_stream, gpr);
        returnFormat = DataFormatInt32;
        return gpr;
    }

    case DataFormatJSInt32:
    case DataFormatJS: {
        // Check the value is an integer.
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecInt32)
            speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::Int32Tag)));
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderInteger);
        info.fillInt32(*m_stream, payloadGPR);
        // If !strict we're done, return.
        returnFormat = DataFormatInt32;
        return payloadGPR;
    }

    case DataFormatInt32: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatInt32;
        return gpr;
    }

    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        returnFormat = DataFormatInt32;
        return allocate();

    case DataFormatDouble:
    case DataFormatStorage:
    default:
        RELEASE_ASSERT_NOT_REACHED();
        return InvalidGPRReg;
    }
}

GPRReg SpeculativeJIT::fillSpeculateInt32(Edge edge, DataFormat& returnFormat)
{
    return fillSpeculateInt32Internal<false>(edge, returnFormat);
}

GPRReg SpeculativeJIT::fillSpeculateInt32Strict(Edge edge)
{
    DataFormat mustBeDataFormatInt32;
    GPRReg result = fillSpeculateInt32Internal<true>(edge, mustBeDataFormatInt32);
    ASSERT(mustBeDataFormatInt32 == DataFormatInt32);
    return result;
}

FPRReg SpeculativeJIT::fillSpeculateDouble(Edge edge)
{
    ASSERT(isDouble(edge.useKind()));
    ASSERT(edge->hasDoubleResult());
    VirtualRegister virtualRegister = edge->virtualRegister();
    GenerationInfo& info = generationInfoFromVirtualRegister(virtualRegister);

    if (info.registerFormat() == DataFormatNone) {

        if (edge->hasConstant()) {
            RELEASE_ASSERT(edge->isNumberConstant());
            FPRReg fpr = fprAllocate();
            m_jit.loadDouble(TrustedImmPtr(m_jit.addressOfDoubleConstant(edge.node())), fpr);
            m_fprs.retain(fpr, virtualRegister, SpillOrderConstant);
            info.fillDouble(*m_stream, fpr);
            return fpr;
        }
        
        RELEASE_ASSERT(info.spillFormat() == DataFormatDouble);
        FPRReg fpr = fprAllocate();
        m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderSpilled);
        info.fillDouble(*m_stream, fpr);
        return fpr;
    }

    RELEASE_ASSERT(info.registerFormat() == DataFormatDouble);
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
        // Protect the silent spill/fill logic by failing early. If we "speculate" on
        // the constant then the silent filler may think that we have a cell and a
        // constant, so it will try to fill this as an cell constant. Bad things will
        // happen.
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();
    }

    switch (info.registerFormat()) {
    case DataFormatNone: {
        if (info.spillFormat() == DataFormatInt32) {
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return allocate();
        }

        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            GPRReg gpr = allocate();
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            m_jit.move(MacroAssembler::TrustedImmPtr(jsValue.asCell()), gpr);
            info.fillCell(*m_stream, gpr);
            return gpr;
        }

        ASSERT((info.spillFormat() & DataFormatJS) || info.spillFormat() == DataFormatCell);
        if (type & ~SpecCell) {
            speculationCheck(
                BadType,
                JSValueSource(JITCompiler::addressFor(virtualRegister)),
                edge,
                m_jit.branch32(
                    MacroAssembler::NotEqual,
                    JITCompiler::tagFor(virtualRegister),
                    TrustedImm32(JSValue::CellTag)));
        }
        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillCell(*m_stream, gpr);
        return gpr;
    }

    case DataFormatCell: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJSCell:
    case DataFormatJS: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecCell) {
            speculationCheck(
                BadType, JSValueRegs(tagGPR, payloadGPR), edge,
                branchNotCell(info.jsValueRegs()));
        }
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderCell);
        info.fillCell(*m_stream, payloadGPR);
        return payloadGPR;
    }

    case DataFormatJSInt32:
    case DataFormatInt32:
    case DataFormatJSDouble:
    case DataFormatJSBoolean:
    case DataFormatBoolean:
        terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
        return allocate();

    case DataFormatDouble:
    case DataFormatStorage:
        RELEASE_ASSERT_NOT_REACHED();

    default:
        RELEASE_ASSERT_NOT_REACHED();
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
        
        if (edge->hasConstant()) {
            JSValue jsValue = edge->asJSValue();
            GPRReg gpr = allocate();
            if (jsValue.isBoolean()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImm32(jsValue.asBoolean()), gpr);
                info.fillBoolean(*m_stream, gpr);
                return gpr;
            }
            terminateSpeculativeExecution(Uncountable, JSValueRegs(), 0);
            return gpr;
        }

        ASSERT((info.spillFormat() & DataFormatJS) || info.spillFormat() == DataFormatBoolean);

        if (type & ~SpecBoolean)
            speculationCheck(BadType, JSValueSource(JITCompiler::addressFor(virtualRegister)), edge, m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::BooleanTag)));

        GPRReg gpr = allocate();
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillBoolean(*m_stream, gpr);
        return gpr;
    }

    case DataFormatBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }

    case DataFormatJSBoolean:
    case DataFormatJS: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (type & ~SpecBoolean)
            speculationCheck(BadType, JSValueRegs(tagGPR, payloadGPR), edge, m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::BooleanTag)));

        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderBoolean);
        info.fillBoolean(*m_stream, payloadGPR);
        return payloadGPR;
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
        RELEASE_ASSERT_NOT_REACHED();

    default:
        RELEASE_ASSERT_NOT_REACHED();
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

    writeBarrier(base.gpr(), value.tagGPR(), valueEdge, scratch1.gpr(), scratch2.gpr());
#else
    UNUSED_PARAM(baseEdge);
    UNUSED_PARAM(valueEdge);
#endif
}

void SpeculativeJIT::compileObjectEquality(Node* node)
{
    SpeculateCellOperand op1(this, node->child1());
    SpeculateCellOperand op2(this, node->child2());
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), node->child1(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), node->child1(), 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));

        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op2GPR), node->child2(), SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op2GPR), node->child2(), 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    GPRTemporary resultPayload(this, Reuse, op2);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    MacroAssembler::Jump falseCase = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, op2GPR);
    m_jit.move(TrustedImm32(1), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(0), resultPayloadGPR);
    done.link(&m_jit);

    booleanResult(resultPayloadGPR, node);
}

void SpeculativeJIT::compileObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild)
{
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();
    GPRReg resultGPR = result.gpr();

    bool masqueradesAsUndefinedWatchpointValid =
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild, 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    
    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell = branchNotCell(op2.jsValueRegs());
    
    // We know that within this branch, rightChild must be a cell.
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, 
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    MacroAssembler::Jump falseCase = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, op2PayloadGPR);
    MacroAssembler::Jump trueCase = m_jit.jump();
    
    rightNotCell.link(&m_jit);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (needsTypeCheck(rightChild, SpecCell | SpecOther)) {
        m_jit.or32(TrustedImm32(1), op2TagGPR, resultGPR);
        
        typeCheck(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm32(JSValue::NullTag)));
    }
    
    falseCase.link(&m_jit);
    m_jit.move(TrustedImm32(0), resultGPR);
    MacroAssembler::Jump done = m_jit.jump();
    trueCase.link(&m_jit);
    m_jit.move(TrustedImm32(1), resultGPR);
    done.link(&m_jit);
    
    booleanResult(resultGPR, m_currentNode);
}

void SpeculativeJIT::compilePeepHoleObjectToObjectOrOtherEquality(Edge leftChild, Edge rightChild, Node* branchNode)
{
    BasicBlock* taken = branchNode->branchData()->taken.block;
    BasicBlock* notTaken = branchNode->branchData()->notTaken.block;
    
    SpeculateCellOperand op1(this, leftChild);
    JSValueOperand op2(this, rightChild, ManualOperandSpeculation);
    GPRTemporary result(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();
    GPRReg resultGPR = result.gpr();

    bool masqueradesAsUndefinedWatchpointValid =
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        DFG_TYPE_CHECK(
            JSValueSource::unboxedCell(op1GPR), leftChild, SpecObject, m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op1GPR, JSCell::structureIDOffset()),
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueSource::unboxedCell(op1GPR), leftChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op1GPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // It seems that most of the time when programs do a == b where b may be either null/undefined
    // or an object, b is usually an object. Balance the branches to make that case fast.
    MacroAssembler::Jump rightNotCell = branchNotCell(op2.jsValueRegs());
    
    // We know that within this branch, rightChild must be a cell.
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        DFG_TYPE_CHECK(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
        speculationCheck(BadType, JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild,
            m_jit.branchTest8(
                MacroAssembler::NonZero, 
                MacroAssembler::Address(op2PayloadGPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined)));
    }
    
    // At this point we know that we can perform a straight-forward equality comparison on pointer
    // values because both left and right are pointers to objects that have no special equality
    // protocols.
    branch32(MacroAssembler::Equal, op1GPR, op2PayloadGPR, taken);
    
    // We know that within this branch, rightChild must not be a cell. Check if that is enough to
    // prove that it is either null or undefined.
    if (!needsTypeCheck(rightChild, SpecCell | SpecOther))
        rightNotCell.link(&m_jit);
    else {
        jump(notTaken, ForceJump);
        
        rightNotCell.link(&m_jit);
        m_jit.or32(TrustedImm32(1), op2TagGPR, resultGPR);
        
        typeCheck(
            JSValueRegs(op2TagGPR, op2PayloadGPR), rightChild, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, resultGPR,
                MacroAssembler::TrustedImm32(JSValue::NullTag)));
    }
    
    jump(notTaken);
}

void SpeculativeJIT::compileInt32Compare(Node* node, MacroAssembler::RelationalCondition condition)
{
    SpeculateInt32Operand op1(this, node->child1());
    SpeculateInt32Operand op2(this, node->child2());
    GPRTemporary resultPayload(this);
    
    m_jit.compare32(condition, op1.gpr(), op2.gpr(), resultPayload.gpr());
    
    // If we add a DataFormatBool, we should use it here.
    booleanResult(resultPayload.gpr(), node);
}

void SpeculativeJIT::compileDoubleCompare(Node* node, MacroAssembler::DoubleCondition condition)
{
    SpeculateDoubleOperand op1(this, node->child1());
    SpeculateDoubleOperand op2(this, node->child2());
    GPRTemporary resultPayload(this);
    
    m_jit.move(TrustedImm32(1), resultPayload.gpr());
    MacroAssembler::Jump trueCase = m_jit.branchDouble(condition, op1.fpr(), op2.fpr());
    m_jit.move(TrustedImm32(0), resultPayload.gpr());
    trueCase.link(&m_jit);
    
    booleanResult(resultPayload.gpr(), node);
}

void SpeculativeJIT::compileObjectOrOtherLogicalNot(Edge nodeUse)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary resultPayload(this);
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    GPRTemporary structure;
    GPRReg structureGPR = InvalidGPRReg;

    bool masqueradesAsUndefinedWatchpointValid =
        masqueradesAsUndefinedWatchpointIsStillValid();

    if (!masqueradesAsUndefinedWatchpointValid) {
        // The masquerades as undefined case will use the structure register, so allocate it here.
        // Do this at the top of the function to avoid branching around a register allocation.
        GPRTemporary realStructure(this);
        structure.adopt(realStructure);
        structureGPR = structure.gpr();
    }

    MacroAssembler::Jump notCell = branchNotCell(value.jsValueRegs());
    if (masqueradesAsUndefinedWatchpointValid) {
        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                MacroAssembler::Address(valuePayloadGPR, JSCell::structureIDOffset()),
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valuePayloadGPR, JSCell::structureIDOffset()), structureGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal,
                structureGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        MacroAssembler::Jump isNotMasqueradesAsUndefined = 
            m_jit.branchTest8(
                MacroAssembler::Zero, 
                MacroAssembler::Address(valuePayloadGPR, JSCell::typeInfoFlagsOffset()), 
                MacroAssembler::TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, 
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(structureGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    m_jit.move(TrustedImm32(0), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    
    notCell.link(&m_jit);
 
    COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.or32(TrustedImm32(1), valueTagGPR, resultPayloadGPR);
        typeCheck(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, SpecCell | SpecOther,
            m_jit.branch32(
                MacroAssembler::NotEqual, 
                resultPayloadGPR, 
                TrustedImm32(JSValue::NullTag)));
    }
    m_jit.move(TrustedImm32(1), resultPayloadGPR);
    
    done.link(&m_jit);
    
    booleanResult(resultPayloadGPR, m_currentNode);
}

void SpeculativeJIT::compileLogicalNot(Node* node)
{
    switch (node->child1().useKind()) {
    case BooleanUse: {
        SpeculateBooleanOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value);
        m_jit.xor32(TrustedImm32(1), value.gpr(), result.gpr());
        booleanResult(result.gpr(), node);
        return;
    }
        
    case ObjectOrOtherUse: {
        compileObjectOrOtherLogicalNot(node->child1());
        return;
    }
        
    case Int32Use: {
        SpeculateInt32Operand value(this, node->child1());
        GPRTemporary resultPayload(this, Reuse, value);
        m_jit.compare32(MacroAssembler::Equal, value.gpr(), MacroAssembler::TrustedImm32(0), resultPayload.gpr());
        booleanResult(resultPayload.gpr(), node);
        return;
    }
        
    case DoubleRepUse: {
        SpeculateDoubleOperand value(this, node->child1());
        FPRTemporary scratch(this);
        GPRTemporary resultPayload(this);
        m_jit.move(TrustedImm32(0), resultPayload.gpr());
        MacroAssembler::Jump nonZero = m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr());
        m_jit.move(TrustedImm32(1), resultPayload.gpr());
        nonZero.link(&m_jit);
        booleanResult(resultPayload.gpr(), node);
        return;
    }

    case UntypedUse: {
        JSValueOperand arg1(this, node->child1());
        GPRTemporary resultPayload(this, Reuse, arg1, PayloadWord);
        GPRReg arg1TagGPR = arg1.tagGPR();
        GPRReg arg1PayloadGPR = arg1.payloadGPR();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        arg1.use();

        JITCompiler::Jump slowCase = m_jit.branch32(JITCompiler::NotEqual, arg1TagGPR, TrustedImm32(JSValue::BooleanTag));
    
        m_jit.move(arg1PayloadGPR, resultPayloadGPR);

        addSlowPathGenerator(
            slowPathCall(
                slowCase, this, operationConvertJSValueToBoolean, resultPayloadGPR, arg1TagGPR,
                arg1PayloadGPR));
    
        m_jit.xor32(TrustedImm32(1), resultPayloadGPR);
        booleanResult(resultPayloadGPR, node, UseChildrenCalledExplicitly);
        return;
    }
    case StringUse:
        return compileStringZeroLength(node);

    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

void SpeculativeJIT::emitObjectOrOtherBranch(Edge nodeUse, BasicBlock* taken, BasicBlock* notTaken)
{
    JSValueOperand value(this, nodeUse, ManualOperandSpeculation);
    GPRTemporary scratch(this);
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    GPRReg scratchGPR = scratch.gpr();
    
    MacroAssembler::Jump notCell = branchNotCell(value.jsValueRegs());
    if (masqueradesAsUndefinedWatchpointIsStillValid()) {
        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(valuePayloadGPR, JSCell::structureIDOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));
    } else {
        m_jit.loadPtr(MacroAssembler::Address(valuePayloadGPR, JSCell::structureIDOffset()), scratchGPR);

        DFG_TYPE_CHECK(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, (~SpecCell) | SpecObject,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                scratchGPR,
                MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get())));

        JITCompiler::Jump isNotMasqueradesAsUndefined = m_jit.branchTest8(
            JITCompiler::Zero, 
            MacroAssembler::Address(valuePayloadGPR, JSCell::typeInfoFlagsOffset()), 
            TrustedImm32(MasqueradesAsUndefined));

        speculationCheck(BadType, JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse,
            m_jit.branchPtr(
                MacroAssembler::Equal, 
                MacroAssembler::Address(scratchGPR, Structure::globalObjectOffset()), 
                MacroAssembler::TrustedImmPtr(m_jit.graph().globalObjectFor(m_currentNode->origin.semantic))));

        isNotMasqueradesAsUndefined.link(&m_jit);
    }
    jump(taken, ForceJump);
    
    notCell.link(&m_jit);
    
    COMPILE_ASSERT((JSValue::UndefinedTag | 1) == JSValue::NullTag, UndefinedTag_OR_1_EQUALS_NullTag);
    if (needsTypeCheck(nodeUse, SpecCell | SpecOther)) {
        m_jit.or32(TrustedImm32(1), valueTagGPR, scratchGPR);
        typeCheck(
            JSValueRegs(valueTagGPR, valuePayloadGPR), nodeUse, SpecCell | SpecOther,
            m_jit.branch32(MacroAssembler::NotEqual, scratchGPR, TrustedImm32(JSValue::NullTag)));
    }

    jump(notTaken);
    
    noResult(m_currentNode);
}

void SpeculativeJIT::emitBranch(Node* node)
{
    BasicBlock* taken = node->branchData()->taken.block;
    BasicBlock* notTaken = node->branchData()->notTaken.block;

    switch (node->child1().useKind()) {
    case BooleanUse: {
        SpeculateBooleanOperand value(this, node->child1());
        MacroAssembler::ResultCondition condition = MacroAssembler::NonZero;

        if (taken == nextBlock()) {
            condition = MacroAssembler::Zero;
            BasicBlock* tmp = taken;
            taken = notTaken;
            notTaken = tmp;
        }

        branchTest32(condition, value.gpr(), TrustedImm32(1), taken);
        jump(notTaken);

        noResult(node);
        return;
    }
    
    case ObjectOrOtherUse: {
        emitObjectOrOtherBranch(node->child1(), taken, notTaken);
        return;
    }
    
    case DoubleRepUse:
    case Int32Use: {
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
    
    case UntypedUse: {
        JSValueOperand value(this, node->child1());
        value.fill();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();

        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        use(node->child1());
    
        JITCompiler::Jump fastPath = m_jit.branch32(JITCompiler::Equal, valueTagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag));
        JITCompiler::Jump slowPath = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, JITCompiler::TrustedImm32(JSValue::BooleanTag));

        fastPath.link(&m_jit);
        branchTest32(JITCompiler::Zero, valuePayloadGPR, notTaken);
        jump(taken, ForceJump);

        slowPath.link(&m_jit);
        silentSpillAllRegisters(resultGPR);
        callOperation(operationConvertJSValueToBoolean, resultGPR, valueTagGPR, valuePayloadGPR);
        silentFillAllRegisters(resultGPR);
    
        branchTest32(JITCompiler::NonZero, resultGPR, taken);
        jump(notTaken);
    
        noResult(node, UseChildrenCalledExplicitly);
        return;
    }
        
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }
}

template<typename BaseOperandType, typename PropertyOperandType, typename ValueOperandType, typename TagType>
void SpeculativeJIT::compileContiguousPutByVal(Node* node, BaseOperandType& base, PropertyOperandType& property, ValueOperandType& value, GPRReg valuePayloadReg, TagType valueTag)
{
    Edge child4 = m_jit.graph().varArgChild(node, 3);

    ArrayMode arrayMode = node->arrayMode();
    
    GPRReg baseReg = base.gpr();
    GPRReg propertyReg = property.gpr();
    
    StorageOperand storage(this, child4);
    GPRReg storageReg = storage.gpr();

    if (node->op() == PutByValAlias) {
        // Store the value to the array.
        GPRReg propertyReg = property.gpr();
        m_jit.store32(valueTag, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        
        noResult(node);
        return;
    }
    
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
        
        m_jit.add32(TrustedImm32(1), propertyReg);
        m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength()));
        m_jit.sub32(TrustedImm32(1), propertyReg);
        
        inBounds.link(&m_jit);
    }
    
    m_jit.store32(valueTag, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
    m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
    
    base.use();
    property.use();
    value.use();
    storage.use();
    
    if (arrayMode.isOutOfBounds()) {
        if (node->op() == PutByValDirect) {
            addSlowPathGenerator(slowPathCall(
                slowCase, this,
                m_jit.codeBlock()->isStrictMode() ? operationPutByValDirectBeyondArrayBoundsStrict : operationPutByValDirectBeyondArrayBoundsNonStrict,
                NoResult, baseReg, propertyReg, valueTag, valuePayloadReg));
        } else {
            addSlowPathGenerator(slowPathCall(
                slowCase, this,
                m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                NoResult, baseReg, propertyReg, valueTag, valuePayloadReg));
        }
    }

    noResult(node, UseChildrenCalledExplicitly);    
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
        initConstantInfo(node);
        break;

    case PhantomArguments:
        initConstantInfo(node);
        break;

    case Identity: {
        RELEASE_ASSERT_NOT_REACHED();
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
        
        case FlushedCell: {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->machineLocal()), result.gpr());
            
            // Like cellResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderCell);
            generationInfoFromVirtualRegister(virtualRegister).initCell(node, node->refCount(), result.gpr());
            break;
        }
            
        case FlushedBoolean: {
            GPRTemporary result(this);
            m_jit.load32(JITCompiler::payloadFor(node->machineLocal()), result.gpr());
            
            // Like booleanResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderBoolean);
            generationInfoFromVirtualRegister(virtualRegister).initBoolean(node, node->refCount(), result.gpr());
            break;
        }
            
        case FlushedJSValue:
        case FlushedArguments: {
            GPRTemporary result(this);
            GPRTemporary tag(this);
            m_jit.load32(JITCompiler::payloadFor(node->machineLocal()), result.gpr());
            m_jit.load32(JITCompiler::tagFor(node->machineLocal()), tag.gpr());
            
            // Like jsValueResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node->virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
            m_gprs.retain(tag.gpr(), virtualRegister, SpillOrderJS);
            
            generationInfoFromVirtualRegister(virtualRegister).initJSValue(node, node->refCount(), tag.gpr(), result.gpr(), DataFormatJS);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
        break;
    }
        
    case GetLocalUnlinked: {
        GPRTemporary payload(this);
        GPRTemporary tag(this);
        m_jit.load32(JITCompiler::payloadFor(node->unlinkedMachineLocal()), payload.gpr());
        m_jit.load32(JITCompiler::tagFor(node->unlinkedMachineLocal()), tag.gpr());
        jsValueResult(tag.gpr(), payload.gpr(), node);
        break;
    }

    case MovHint:
    case ZombieHint: {
        RELEASE_ASSERT_NOT_REACHED();
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
            
        case FlushedCell: {
            SpeculateCellOperand cell(this, node->child1());
            GPRReg cellGPR = cell.gpr();
            m_jit.storePtr(cellGPR, JITCompiler::payloadFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatCell);
            break;
        }
            
        case FlushedBoolean: {
            SpeculateBooleanOperand value(this, node->child1());
            m_jit.store32(value.gpr(), JITCompiler::payloadFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(DataFormatBoolean);
            break;
        }
            
        case FlushedJSValue:
        case FlushedArguments: {
            JSValueOperand value(this, node->child1());
            m_jit.store32(value.payloadGPR(), JITCompiler::payloadFor(node->machineLocal()));
            m_jit.store32(value.tagGPR(), JITCompiler::tagFor(node->machineLocal()));
            noResult(node);
            recordSetLocal(dataFormatFor(node->variableAccessData()->flushFormat()));
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
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
        
    case ValueAdd: {
        JSValueOperand op1(this, node->child1());
        JSValueOperand op2(this, node->child2());
        
        GPRReg op1TagGPR = op1.tagGPR();
        GPRReg op1PayloadGPR = op1.payloadGPR();
        GPRReg op2TagGPR = op2.tagGPR();
        GPRReg op2PayloadGPR = op2.payloadGPR();
        
        flushRegisters();
        
        GPRResult2 resultTag(this);
        GPRResult resultPayload(this);
        if (isKnownNotNumber(node->child1().node()) || isKnownNotNumber(node->child2().node()))
            callOperation(operationValueAddNotNumber, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
        else
            callOperation(operationValueAdd, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
        
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
            GPRTemporary result(this, Reuse, op1);
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
            RELEASE_ASSERT_NOT_REACHED();
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

            GPRReg op1GPR = op1.gpr();
            GPRReg op2GPR = op2.gpr();
            GPRReg resultGPR = result.gpr();

            MacroAssembler::Jump op1Less = m_jit.branch32(op == ArithMin ? MacroAssembler::LessThan : MacroAssembler::GreaterThan, op1GPR, op2GPR);
            m_jit.move(op2GPR, resultGPR);
            if (op1GPR != resultGPR) {
                MacroAssembler::Jump done = m_jit.jump();
                op1Less.link(&m_jit);
                m_jit.move(op1GPR, resultGPR);
                done.link(&m_jit);
            } else
                op1Less.link(&m_jit);
            
            int32Result(resultGPR, node);
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
            RELEASE_ASSERT_NOT_REACHED();
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
            RELEASE_ASSERT_NOT_REACHED();
#if COMPILER_QUIRK(CONSIDERS_UNREACHABLE_CODE)
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
#endif
            break;
        case Array::Generic: {
            SpeculateCellOperand base(this, node->child1()); // Save a register, speculate cell. We'll probably be right.
            JSValueOperand property(this, node->child2());
            GPRReg baseGPR = base.gpr();
            GPRReg propertyTagGPR = property.tagGPR();
            GPRReg propertyPayloadGPR = property.payloadGPR();
            
            flushRegisters();
            GPRResult2 resultTag(this);
            GPRResult resultPayload(this);
            callOperation(operationGetByValCell, resultTag.gpr(), resultPayload.gpr(), baseGPR, propertyTagGPR, propertyPayloadGPR);
            
            jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
            
                GPRTemporary resultPayload(this);
                if (node->arrayMode().type() == Array::Int32) {
                    speculationCheck(
                        OutOfBounds, JSValueRegs(), 0,
                        m_jit.branch32(
                            MacroAssembler::Equal,
                            MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
                            TrustedImm32(JSValue::EmptyValueTag)));
                    m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
                    int32Result(resultPayload.gpr(), node);
                    break;
                }
                
                GPRTemporary resultTag(this);
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag)));
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
                jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
            
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();
            
            MacroAssembler::JumpList slowCases;

            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagReg);
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadReg);
            slowCases.append(m_jit.branch32(MacroAssembler::Equal, resultTagReg, TrustedImm32(JSValue::EmptyValueTag)));
            
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg), baseReg, propertyReg));
            
            jsValueResult(resultTagReg, resultPayloadReg, node);
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
            
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            FPRTemporary temp(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();
            FPRReg tempReg = temp.fpr();
            
            MacroAssembler::JumpList slowCases;
            
            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(storageReg, Butterfly::offsetOfPublicLength())));
            
            m_jit.loadDouble(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight), tempReg);
            slowCases.append(m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempReg, tempReg));
            boxDouble(tempReg, resultTagReg, resultPayloadReg);

            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg), baseReg, propertyReg));
            
            jsValueResult(resultTagReg, resultPayloadReg, node);
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

                GPRTemporary resultTag(this);
                GPRTemporary resultPayload(this);

                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag.gpr());
                speculationCheck(LoadFromHole, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag)));
                m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload.gpr());
            
                jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
                break;
            }

            SpeculateCellOperand base(this, node->child1());
            SpeculateStrictInt32Operand property(this, node->child2());
            StorageOperand storage(this, node->child3());
            GPRReg propertyReg = property.gpr();
            GPRReg storageReg = storage.gpr();
            GPRReg baseReg = base.gpr();

            if (!m_compileOkay)
                return;

            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            GPRReg resultTagReg = resultTag.gpr();
            GPRReg resultPayloadReg = resultPayload.gpr();

            JITCompiler::Jump outOfBounds = m_jit.branch32(
                MacroAssembler::AboveOrEqual, propertyReg,
                MacroAssembler::Address(storageReg, ArrayStorage::vectorLengthOffset()));

            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagReg);
            JITCompiler::Jump hole = m_jit.branch32(
                MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag));
            m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadReg);
            
            JITCompiler::JumpList slowCases;
            slowCases.append(outOfBounds);
            slowCases.append(hole);
            addSlowPathGenerator(
                slowPathCall(
                    slowCases, this, operationGetByValArrayInt,
                    JSValueRegs(resultTagReg, resultPayloadReg),
                    baseReg, propertyReg));

            jsValueResult(resultTagReg, resultPayloadReg, node);
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
            RELEASE_ASSERT_NOT_REACHED();
#if COMPILER_QUIRK(CONSIDERS_UNREACHABLE_CODE)
            terminateSpeculativeExecution(InadequateCoverage, JSValueRegs(), 0);
            alreadyHandled = true;
#endif
            break;
        case Array::Generic: {
            ASSERT(node->op() == PutByVal || node->op() == PutByValDirect);
            
            SpeculateCellOperand base(this, child1); // Save a register, speculate cell. We'll probably be right.
            JSValueOperand property(this, child2);
            JSValueOperand value(this, child3);
            GPRReg baseGPR = base.gpr();
            GPRReg propertyTagGPR = property.tagGPR();
            GPRReg propertyPayloadGPR = property.payloadGPR();
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();
            
            flushRegisters();
            if (node->op() == PutByValDirect)
                callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByValDirectCellStrict : operationPutByValDirectCellNonStrict, baseGPR, propertyTagGPR, propertyPayloadGPR, valueTagGPR, valuePayloadGPR);
            else
                callOperation(m_jit.codeBlock()->isStrictMode() ? operationPutByValCellStrict : operationPutByValCellNonStrict, baseGPR, propertyTagGPR, propertyPayloadGPR, valueTagGPR, valuePayloadGPR);
            
            noResult(node);
            alreadyHandled = true;
            break;
        }
        default:
            break;
        }
        
        if (alreadyHandled)
            break;
        
        SpeculateCellOperand base(this, child1);
        SpeculateStrictInt32Operand property(this, child2);
        
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();

        switch (arrayMode.type()) {
        case Array::Int32: {
            SpeculateInt32Operand value(this, child3);

            GPRReg valuePayloadReg = value.gpr();
        
            if (!m_compileOkay)
                return;
            
            compileContiguousPutByVal(node, base, property, value, valuePayloadReg, TrustedImm32(JSValue::Int32Tag));
            break;
        }
        case Array::Contiguous: {
            JSValueOperand value(this, child3);

            GPRReg valueTagReg = value.tagGPR();
            GPRReg valuePayloadReg = value.payloadGPR();
        
            if (!m_compileOkay)
                return;

            compileContiguousPutByVal(node, base, property, value, valuePayloadReg, valueTagReg);
            break;
        }
        case Array::Double: {
            compileDoublePutByVal(node, base, property);
            break;
        }
        case Array::ArrayStorage:
        case Array::SlowPutArrayStorage: {
            JSValueOperand value(this, child3);

            GPRReg valueTagReg = value.tagGPR();
            GPRReg valuePayloadReg = value.payloadGPR();
            
            if (!m_compileOkay)
                return;

            StorageOperand storage(this, child4);
            GPRReg storageReg = storage.gpr();

            if (node->op() == PutByValAlias) {
                // Store the value to the array.
                GPRReg propertyReg = property.gpr();
                m_jit.store32(value.tagGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                m_jit.store32(value.payloadGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                
                noResult(node);
                break;
            }

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
                    m_jit.branch32(MacroAssembler::Equal, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag)));
            } else {
                MacroAssembler::Jump notHoleValue = m_jit.branch32(MacroAssembler::NotEqual, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
                if (arrayMode.isSlowPut()) {
                    // This is sort of strange. If we wanted to optimize this code path, we would invert
                    // the above branch. But it's simply not worth it since this only happens if we're
                    // already having a bad time.
                    slowCases.append(m_jit.jump());
                } else {
                    m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, ArrayStorage::numValuesInVectorOffset()));
                
                    // If we're writing to a hole we might be growing the array; 
                    MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.add32(TrustedImm32(1), propertyReg);
                    m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, ArrayStorage::lengthOffset()));
                    m_jit.sub32(TrustedImm32(1), propertyReg);
                
                    lengthDoesNotNeedUpdate.link(&m_jit);
                }
                notHoleValue.link(&m_jit);
            }
    
            // Store the value to the array.
            m_jit.store32(valueTagReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));

            base.use();
            property.use();
            value.use();
            storage.use();
            
            if (!slowCases.empty()) {
                if (node->op() == PutByValDirect) {
                    addSlowPathGenerator(slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValDirectBeyondArrayBoundsStrict : operationPutByValDirectBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueTagReg, valuePayloadReg));
                } else {
                    addSlowPathGenerator(slowPathCall(
                        slowCases, this,
                        m_jit.codeBlock()->isStrictMode() ? operationPutByValBeyondArrayBoundsStrict : operationPutByValBeyondArrayBoundsNonStrict,
                        NoResult, baseReg, propertyReg, valueTagReg, valuePayloadReg));
                }
            }

            noResult(node, UseChildrenCalledExplicitly);
            break;
        }
            
        case Array::Arguments:
            // FIXME: we could at some point make this work. Right now we're assuming that the register
            // pressure would be too great.
            RELEASE_ASSERT_NOT_REACHED();
            break;
            
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
            booleanResult(result.gpr(), node);
            break;
        }

        SpeculateCellOperand base(this, node->child1());
        SpeculateCellOperand argument(this, node->child2());
        GPRReg baseGPR = base.gpr();
        GPRReg argumentGPR = argument.gpr();
        
        flushRegisters();
        GPRResult2 resultTag(this);
        GPRResult resultPayload(this);
        callOperation(operationRegExpExec, resultTag.gpr(), resultPayload.gpr(), baseGPR, argumentGPR);
        
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
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
        booleanResult(result.gpr(), node);
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
        case Array::Int32: {
            SpeculateInt32Operand value(this, node->child2());
            GPRReg valuePayloadGPR = value.gpr();
            
            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store32(TrustedImm32(JSValue::Int32Tag), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    TrustedImm32(JSValue::Int32Tag), valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
            break;
        }
            
        case Array::Contiguous: {
            JSValueOperand value(this, node->child2());
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();

            m_jit.load32(MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), storageLengthGPR);
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfVectorLength()));
            m_jit.store32(valueTagGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPush,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    valueTagGPR, valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
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
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationArrayPushDouble,
                    JSValueRegs(storageGPR, storageLengthGPR),
                    valueFPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
            break;
        }
            
        case Array::ArrayStorage: {
            JSValueOperand value(this, node->child2());
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            // Refuse to handle bizarre lengths.
            speculationCheck(Uncountable, JSValueRegs(), 0, m_jit.branch32(MacroAssembler::Above, storageLengthGPR, TrustedImm32(0x7ffffffe)));
        
            MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.store32(valueTagGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        
            m_jit.add32(TrustedImm32(1), storageLengthGPR);
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));
            m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
            m_jit.move(TrustedImm32(JSValue::Int32Tag), storageGPR);
        
            addSlowPathGenerator(slowPathCall(slowPath, this, operationArrayPush, JSValueRegs(storageGPR, storageLengthGPR), valueTagGPR, valuePayloadGPR, baseGPR));
        
            jsValueResult(storageGPR, storageLengthGPR, node);
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
        GPRTemporary valueTag(this);
        GPRTemporary valuePayload(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = valueTag.gpr();
        GPRReg valuePayloadGPR = valuePayload.gpr();
        GPRReg storageGPR = storage.gpr();
        
        switch (node->arrayMode().type()) {
        case Array::Int32:
        case Array::Contiguous: {
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), valuePayloadGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, valuePayloadGPR);
            m_jit.sub32(TrustedImm32(1), valuePayloadGPR);
            m_jit.store32(
                valuePayloadGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.load32(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)),
                valueTagGPR);
            MacroAssembler::Jump slowCase = m_jit.branch32(MacroAssembler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
            m_jit.store32(
                MacroAssembler::TrustedImm32(JSValue::EmptyValueTag),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.load32(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)),
                valuePayloadGPR);

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));
            
            jsValueResult(valueTagGPR, valuePayloadGPR, node);
            break;
        }
            
        case Array::Double: {
            FPRTemporary temp(this);
            FPRReg tempFPR = temp.fpr();
            
            m_jit.load32(
                MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()), valuePayloadGPR);
            MacroAssembler::Jump undefinedCase =
                m_jit.branchTest32(MacroAssembler::Zero, valuePayloadGPR);
            m_jit.sub32(TrustedImm32(1), valuePayloadGPR);
            m_jit.store32(
                valuePayloadGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength()));
            m_jit.loadDouble(
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight),
                tempFPR);
            MacroAssembler::Jump slowCase = m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, tempFPR, tempFPR);
            JSValue nan = JSValue(JSValue::EncodeAsDouble, PNaN);
            m_jit.store32(
                MacroAssembler::TrustedImm32(nan.u.asBits.tag),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
            m_jit.store32(
                MacroAssembler::TrustedImm32(nan.u.asBits.payload),
                MacroAssembler::BaseIndex(storageGPR, valuePayloadGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
            boxDouble(tempFPR, valueTagGPR, valuePayloadGPR);

            addSlowPathGenerator(
                slowPathMove(
                    undefinedCase, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPopAndRecoverLength,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));
            
            jsValueResult(valueTagGPR, valuePayloadGPR, node);
            break;
        }

        case Array::ArrayStorage: {
            GPRTemporary storageLength(this);
            GPRReg storageLengthGPR = storageLength.gpr();

            m_jit.load32(MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()), storageLengthGPR);
        
            JITCompiler::JumpList setUndefinedCases;
            setUndefinedCases.append(m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR));
        
            m_jit.sub32(TrustedImm32(1), storageLengthGPR);
        
            MacroAssembler::Jump slowCase = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset()));
        
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), valueTagGPR);
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), valuePayloadGPR);
        
            m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, ArrayStorage::lengthOffset()));

            setUndefinedCases.append(m_jit.branch32(MacroAssembler::Equal, TrustedImm32(JSValue::EmptyValueTag), valueTagGPR));
        
            m_jit.store32(TrustedImm32(JSValue::EmptyValueTag), MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));

            m_jit.sub32(TrustedImm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        
            addSlowPathGenerator(
                slowPathMove(
                    setUndefinedCases, this,
                    MacroAssembler::TrustedImm32(jsUndefined().tag()), valueTagGPR,
                    MacroAssembler::TrustedImm32(jsUndefined().payload()), valuePayloadGPR));
        
            addSlowPathGenerator(
                slowPathCall(
                    slowCase, this, operationArrayPop,
                    JSValueRegs(valueTagGPR, valuePayloadGPR), baseGPR));

            jsValueResult(valueTagGPR, valuePayloadGPR, node);
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
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT2);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node->child1());
        op1.fill();
        if (op1.isDouble())
            boxDouble(op1.fpr(), GPRInfo::returnValueGPR2, GPRInfo::returnValueGPR);
        else {
            if (op1.payloadGPR() == GPRInfo::returnValueGPR2 && op1.tagGPR() == GPRInfo::returnValueGPR)
                m_jit.swap(GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
            else if (op1.payloadGPR() == GPRInfo::returnValueGPR2) {
                m_jit.move(op1.payloadGPR(), GPRInfo::returnValueGPR);
                m_jit.move(op1.tagGPR(), GPRInfo::returnValueGPR2);
            } else {
                m_jit.move(op1.tagGPR(), GPRInfo::returnValueGPR2);
                m_jit.move(op1.payloadGPR(), GPRInfo::returnValueGPR);
            }
        }

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
            SpeculateBooleanOperand value(this, node->child1());
            GPRTemporary result(this); // FIXME: We could reuse, but on speculation fail would need recovery to restore tag (akin to add).
            
            m_jit.move(value.gpr(), result.gpr());

            int32Result(result.gpr(), node);
            break;
        }
            
        case UntypedUse: {
            JSValueOperand value(this, node->child1());
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this);
            
            GPRReg valueTagGPR = value.tagGPR();
            GPRReg valuePayloadGPR = value.payloadGPR();
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();
            
            m_jit.move(valuePayloadGPR, resultPayloadGPR);
            JITCompiler::Jump isBoolean = m_jit.branch32(
                JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::BooleanTag));
            m_jit.move(valueTagGPR, resultTagGPR);
            JITCompiler::Jump done = m_jit.jump();
            isBoolean.link(&m_jit);
            m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
            done.link(&m_jit);
            
            jsValueResult(resultTagGPR, resultPayloadGPR, node);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }
        
    case ToPrimitive: {
        RELEASE_ASSERT(node->child1().useKind() == UntypedUse);
        JSValueOperand op1(this, node->child1());
        GPRTemporary resultTag(this, Reuse, op1, TagWord);
        GPRTemporary resultPayload(this, Reuse, op1, PayloadWord);
        
        GPRReg op1TagGPR = op1.tagGPR();
        GPRReg op1PayloadGPR = op1.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        op1.use();
        
        if (!(m_state.forNode(node->child1()).m_type & ~(SpecFullNumber | SpecBoolean))) {
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
        } else {
            MacroAssembler::Jump alreadyPrimitive = branchNotCell(op1.jsValueRegs());
            MacroAssembler::Jump notPrimitive = m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(op1PayloadGPR, JSCell::structureIDOffset()), MacroAssembler::TrustedImmPtr(m_jit.vm()->stringStructure.get()));
            
            alreadyPrimitive.link(&m_jit);
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
            
            addSlowPathGenerator(
                slowPathCall(
                    notPrimitive, this, operationToPrimitive,
                    JSValueRegs(resultTagGPR, resultPayloadGPR), op1TagGPR, op1PayloadGPR));
        }
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
        break;
    }
        
    case ToString: {
        if (node->child1().useKind() == UntypedUse) {
            JSValueOperand op1(this, node->child1());
            GPRReg op1PayloadGPR = op1.payloadGPR();
            GPRReg op1TagGPR = op1.tagGPR();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            flushRegisters();
            
            JITCompiler::Jump done;
            if (node->child1()->prediction() & SpecString) {
                JITCompiler::Jump slowPath1 = branchNotCell(op1.jsValueRegs());
                JITCompiler::Jump slowPath2 = m_jit.branchPtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(op1PayloadGPR, JSCell::structureIDOffset()),
                    TrustedImmPtr(m_jit.vm()->stringStructure.get()));
                m_jit.move(op1PayloadGPR, resultGPR);
                done = m_jit.jump();
                slowPath1.link(&m_jit);
                slowPath2.link(&m_jit);
            }
            callOperation(operationToString, resultGPR, op1TagGPR, op1PayloadGPR);
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
            ASSERT(structure->indexingType() == node->indexingType());
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
                        m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
        
                    m_jit.storeDouble(opFPR, MacroAssembler::Address(storageGPR, sizeof(double) * operandIdx));
                    break;
                }
                case ALL_INT32_INDEXING_TYPES: {
                    SpeculateInt32Operand operand(this, use);
                    m_jit.store32(TrustedImm32(JSValue::Int32Tag), MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                    m_jit.store32(operand.gpr(), MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                    break;
                }
                case ALL_CONTIGUOUS_INDEXING_TYPES: {
                    JSValueOperand operand(this, m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx]);
                    GPRReg opTagGPR = operand.tagGPR();
                    GPRReg opPayloadGPR = operand.payloadGPR();
                    m_jit.store32(opTagGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                    m_jit.store32(opPayloadGPR, MacroAssembler::Address(storageGPR, sizeof(JSValue) * operandIdx + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
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
            callOperation(
                operationNewEmptyArray, result.gpr(), globalObject->arrayStructureForIndexingTypeDuringAllocation(node->indexingType()));
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
                FPRReg opFPR = operand.fpr();
                DFG_TYPE_CHECK(
                    JSValueRegs(), use, SpecFullRealNumber,
                    m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, opFPR, opFPR));
                
                m_jit.storeDouble(opFPR, TrustedImmPtr(reinterpret_cast<char*>(buffer + operandIdx)));
                break;
            }
            case ALL_INT32_INDEXING_TYPES: {
                SpeculateInt32Operand operand(this, use);
                GPRReg opGPR = operand.gpr();
                m_jit.store32(TrustedImm32(JSValue::Int32Tag), reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                m_jit.store32(opGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
                break;
            }
            case ALL_CONTIGUOUS_INDEXING_TYPES:
            case ALL_ARRAY_STORAGE_INDEXING_TYPES: {
                JSValueOperand operand(this, m_jit.graph().m_varArgChildren[node->firstChild() + operandIdx]);
                GPRReg opTagGPR = operand.tagGPR();
                GPRReg opPayloadGPR = operand.payloadGPR();
                
                m_jit.store32(opTagGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
                m_jit.store32(opPayloadGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
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
                JSValue nan = JSValue(JSValue::EncodeAsDouble, PNaN);
                
                m_jit.move(sizeGPR, scratchGPR);
                MacroAssembler::Jump done = m_jit.branchTest32(MacroAssembler::Zero, scratchGPR);
                MacroAssembler::Label loop = m_jit.label();
                m_jit.sub32(TrustedImm32(1), scratchGPR);
                m_jit.store32(TrustedImm32(nan.u.asBits.tag), MacroAssembler::BaseIndex(storageGPR, scratchGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
                m_jit.store32(TrustedImm32(nan.u.asBits.payload), MacroAssembler::BaseIndex(storageGPR, scratchGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
                m_jit.branchTest32(MacroAssembler::NonZero, scratchGPR).linkTo(loop, &m_jit);
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
        callOperation(
            operationNewArrayWithSize, resultGPR, structureGPR, sizeGPR);
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
            
            if (node->indexingType() == ArrayWithDouble) {
                JSValue* data = m_jit.codeBlock()->constantBuffer(node->startConstant());
                for (unsigned index = 0; index < node->numConstants(); ++index) {
                    union {
                        int32_t halves[2];
                        double value;
                    } u;
                    u.value = data[index].asNumber();
                    m_jit.store32(Imm32(u.halves[0]), MacroAssembler::Address(storageGPR, sizeof(double) * index));
                    m_jit.store32(Imm32(u.halves[1]), MacroAssembler::Address(storageGPR, sizeof(double) * index + sizeof(int32_t)));
                }
            } else {
                int32_t* data = bitwise_cast<int32_t*>(m_jit.codeBlock()->constantBuffer(node->startConstant()));
                for (unsigned index = 0; index < node->numConstants() * 2; ++index) {
                    m_jit.store32(
                        Imm32(data[index]), MacroAssembler::Address(storageGPR, sizeof(int32_t) * index));
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
            GPRReg argumentTagGPR = argument.tagGPR();
            GPRReg argumentPayloadGPR = argument.payloadGPR();
            
            flushRegisters();
            
            GPRResult result(this);
            GPRReg resultGPR = result.gpr();
            
            JSGlobalObject* globalObject = m_jit.graph().globalObjectFor(node->origin.semantic);
            callOperation(
                operationNewTypedArrayWithOneArgumentForType(node->typedArrayType()),
                resultGPR, globalObject->typedArrayStructure(node->typedArrayType()),
                argumentTagGPR, argumentPayloadGPR);
            
            cellResult(resultGPR, node);
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }
        
    case NewRegexp: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        
        callOperation(operationNewRegexp, resultTag.gpr(), resultPayload.gpr(), m_jit.codeBlock()->regexp(node->regexpIndex()));
        
        // FIXME: make the callOperation above explicitly return a cell result, or jitAssert the tag is a cell tag.
        cellResult(resultPayload.gpr(), node);
        break;
    }
        
    case ToThis: {
        ASSERT(node->child1().useKind() == UntypedUse);
        JSValueOperand thisValue(this, node->child1());
        GPRTemporary temp(this);
        GPRTemporary tempTag(this);
        GPRReg thisValuePayloadGPR = thisValue.payloadGPR();
        GPRReg thisValueTagGPR = thisValue.tagGPR();
        GPRReg tempGPR = temp.gpr();
        GPRReg tempTagGPR = tempTag.gpr();
        
        MacroAssembler::JumpList slowCases;
        slowCases.append(branchNotCell(thisValue.jsValueRegs()));
        slowCases.append(m_jit.branch8(
            MacroAssembler::NotEqual,
            MacroAssembler::Address(thisValuePayloadGPR, JSCell::typeInfoTypeOffset()),
            TrustedImm32(FinalObjectType)));
        m_jit.move(thisValuePayloadGPR, tempGPR);
        m_jit.move(thisValueTagGPR, tempTagGPR);
        J_JITOperation_EJ function;
        if (m_jit.graph().executableFor(node->origin.semantic)->isStrictMode())
            function = operationToThisStrict;
        else
            function = operationToThis;
        addSlowPathGenerator(
            slowPathCall(
                slowCases, this, function,
                JSValueRegs(tempTagGPR, tempGPR), thisValueTagGPR, thisValuePayloadGPR));

        jsValueResult(tempTagGPR, tempGPR, node);
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
        m_jit.loadPtr(JITCompiler::payloadFor(JSStack::Callee), result.gpr());
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

        m_jit.loadPtr(JITCompiler::payloadFor(JSStack::ScopeChain), resultGPR);
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
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);
        GPRReg registersGPR = registers.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        m_jit.load32(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        m_jit.load32(JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case PutClosureVar: {
        StorageOperand registers(this, node->child2());
        JSValueOperand value(this, node->child3());
        GPRTemporary scratchRegister(this);

        GPRReg registersGPR = registers.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();

        speculate(node, node->child1());

        m_jit.store32(valueTagGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.store32(valuePayloadGPR, JITCompiler::Address(registersGPR, node->varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        noResult(node);
        break;
    }
        
    case GetById: {
        ASSERT(node->prediction());
        
        switch (node->child1().useKind()) {
        case CellUse: {
            SpeculateCellOperand base(this, node->child1());
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this, Reuse, base);
            
            GPRReg baseGPR = base.gpr();
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();

            base.use();
            
            cachedGetById(node->origin.semantic, InvalidGPRReg, baseGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber());
            
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRTemporary resultTag(this);
            GPRTemporary resultPayload(this, Reuse, base, TagWord);
        
            GPRReg baseTagGPR = base.tagGPR();
            GPRReg basePayloadGPR = base.payloadGPR();
            GPRReg resultTagGPR = resultTag.gpr();
            GPRReg resultPayloadGPR = resultPayload.gpr();
        
            base.use();
        
            JITCompiler::Jump notCell = branchNotCell(base.jsValueRegs());
        
            cachedGetById(node->origin.semantic, baseTagGPR, basePayloadGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), notCell);
        
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
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

            GPRResult resultPayload(this);
            GPRResult2 resultTag(this);
            GPRReg resultPayloadGPR = resultPayload.gpr();
            GPRReg resultTagGPR = resultTag.gpr();

            base.use();
            
            flushRegisters();
            
            cachedGetById(node->origin.semantic, InvalidGPRReg, baseGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), JITCompiler::Jump(), DontSpill);
            
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
        
        case UntypedUse: {
            JSValueOperand base(this, node->child1());
            GPRReg baseTagGPR = base.tagGPR();
            GPRReg basePayloadGPR = base.payloadGPR();

            GPRResult resultPayload(this);
            GPRResult2 resultTag(this);
            GPRReg resultPayloadGPR = resultPayload.gpr();
            GPRReg resultTagGPR = resultTag.gpr();

            base.use();
        
            flushRegisters();
        
            JITCompiler::Jump notCell = branchNotCell(base.jsValueRegs());
        
            cachedGetById(node->origin.semantic, baseTagGPR, basePayloadGPR, resultTagGPR, resultPayloadGPR, node->identifierNumber(), notCell, DontSpill);
        
            jsValueResult(resultTagGPR, resultPayloadGPR, node, UseChildrenCalledExplicitly);
            break;
        }
            
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
        break;
    }

    case GetArrayLength:
        compileGetArrayLength(node);
        break;
        
    case CheckCell: {
        SpeculateCellOperand cell(this, node->child1());
        speculationCheck(BadCell, JSValueSource::unboxedCell(cell.gpr()), node->child1(), m_jit.branchWeakPtr(JITCompiler::NotEqual, cell.gpr(), node->cellOperand()->value().asCell()));
        noResult(node);
        break;
    }

    case GetExecutable: {
        SpeculateCellOperand function(this, node->child1());
        GPRTemporary result(this, Reuse, function);
        GPRReg functionGPR = function.gpr();
        GPRReg resultGPR = result.gpr();
        speculateCellType(node->child1(), functionGPR, SpecFunction, JSFunctionType);
        m_jit.loadPtr(JITCompiler::Address(functionGPR, JSFunction::offsetOfExecutable()), resultGPR);
        cellResult(resultGPR, node);
        break;
    }
        
    case CheckStructure: {
        SpeculateCellOperand base(this, node->child1());
        
        ASSERT(node->structureSet().size());
        
        if (node->structureSet().size() == 1) {
            speculationCheck(
                BadCache, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakPtr(
                    JITCompiler::NotEqual,
                    JITCompiler::Address(base.gpr(), JSCell::structureIDOffset()),
                    node->structureSet()[0]));
        } else {
            GPRTemporary structure(this);
            
            m_jit.loadPtr(JITCompiler::Address(base.gpr(), JSCell::structureIDOffset()), structure.gpr());
            
            JITCompiler::JumpList done;
            
            for (size_t i = 0; i < node->structureSet().size() - 1; ++i)
                done.append(m_jit.branchWeakPtr(JITCompiler::Equal, structure.gpr(), node->structureSet()[i]));
            
            speculationCheck(
                BadCache, JSValueSource::unboxedCell(base.gpr()), 0,
                m_jit.branchWeakPtr(
                    JITCompiler::NotEqual, structure.gpr(), node->structureSet().last()));
            
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
        m_jit.storePtr(MacroAssembler::TrustedImmPtr(newStructure), MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()));
        
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
        
    case GetByOffset: {
        StorageOperand storage(this, node->child1());
        GPRTemporary resultTag(this, Reuse, storage);
        GPRTemporary resultPayload(this);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.load32(JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case GetGetterSetterByOffset: {
        StorageOperand storage(this, node->child1());
        GPRTemporary resultPayload(this);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.load32(JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        
        cellResult(resultPayloadGPR, node);
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

        GPRReg storageGPR = storage.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();

        speculate(node, node->child2());

        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node->storageAccessDataIndex()];
        
        m_jit.storePtr(valueTagGPR, JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.storePtr(valuePayloadGPR, JITCompiler::Address(storageGPR, offsetRelativeToBase(storageAccessData.offset) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        
        noResult(node);
        break;
    }

    case PutByIdFlush: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);

        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        flushRegisters();

        cachedPutById(node->origin.semantic, baseGPR, valueTagGPR, valuePayloadGPR, scratchGPR, node->identifierNumber(), NotDirect, MacroAssembler::Jump(), DontSpill);

        noResult(node);
        break;
    }
        
    case PutById: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        cachedPutById(node->origin.semantic, baseGPR, valueTagGPR, valuePayloadGPR, scratchGPR, node->identifierNumber(), NotDirect);
        
        noResult(node);
        break;
    }

    case PutByIdDirect: {
        SpeculateCellOperand base(this, node->child1());
        JSValueOperand value(this, node->child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        cachedPutById(node->origin.semantic, baseGPR, valueTagGPR, valuePayloadGPR, scratchGPR, node->identifierNumber(), Direct);

        noResult(node);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);

        m_jit.move(TrustedImmPtr(node->registerPointer()), resultPayload.gpr());
        m_jit.load32(JITCompiler::Address(resultPayload.gpr(), OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTag.gpr());
        m_jit.load32(JITCompiler::Address(resultPayload.gpr(), OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayload.gpr());

        jsValueResult(resultTag.gpr(), resultPayload.gpr(), node);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node->child1());

        // FIXME: if we happen to have a spare register - and _ONLY_ if we happen to have
        // a spare register - a good optimization would be to put the register pointer into
        // a register and then do a zero offset store followed by a four-offset store (or
        // vice-versa depending on endianness).
        m_jit.store32(value.tagGPR(), node->registerPointer()->tagPointer());
        m_jit.store32(value.payloadGPR(), node->registerPointer()->payloadPointer());

        noResult(node);
        break;
    }

    case NotifyWrite: {
        VariableWatchpointSet* set = node->variableWatchpointSet();
    
        JSValueOperand value(this, node->child1());
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
    
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
    
        m_jit.load8(set->addressOfState(), tempGPR);
    
        JITCompiler::Jump isDone = m_jit.branch32(JITCompiler::Equal, tempGPR, TrustedImm32(IsInvalidated));
        JITCompiler::JumpList notifySlow;
        notifySlow.append(m_jit.branch32(
            JITCompiler::NotEqual,
            JITCompiler::AbsoluteAddress(set->addressOfInferredValue()->payloadPointer()),
            valuePayloadGPR));
        notifySlow.append(m_jit.branch32(
            JITCompiler::NotEqual, 
            JITCompiler::AbsoluteAddress(set->addressOfInferredValue()->tagPointer()),
            valueTagGPR));
        addSlowPathGenerator(
            slowPathCall(notifySlow, this, operationNotifyWrite, NoResult, set, valueTagGPR, valuePayloadGPR));
        isDone.link(&m_jit);
    
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

        JITCompiler::Jump isCell = branchIsCell(value.jsValueRegs());
        
        m_jit.compare32(JITCompiler::Equal, value.tagGPR(), TrustedImm32(JSValue::UndefinedTag), result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isCell.link(&m_jit);
        JITCompiler::Jump notMasqueradesAsUndefined;
        if (masqueradesAsUndefinedWatchpointIsStillValid()) {
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
        } else {
            JITCompiler::Jump isMasqueradesAsUndefined = m_jit.branchTest8(
                JITCompiler::NonZero, 
                JITCompiler::Address(value.payloadGPR(), JSCell::typeInfoFlagsOffset()), 
                TrustedImm32(MasqueradesAsUndefined));
            m_jit.move(TrustedImm32(0), result.gpr());
            notMasqueradesAsUndefined = m_jit.jump();
            
            isMasqueradesAsUndefined.link(&m_jit);
            GPRReg localGlobalObjectGPR = localGlobalObject.gpr();
            GPRReg remoteGlobalObjectGPR = remoteGlobalObject.gpr();
            m_jit.move(TrustedImmPtr(m_jit.globalObjectFor(node->origin.semantic)), localGlobalObjectGPR);
            m_jit.loadPtr(JITCompiler::Address(value.payloadGPR(), JSCell::structureIDOffset()), result.gpr());
            m_jit.loadPtr(JITCompiler::Address(result.gpr(), Structure::globalObjectOffset()), remoteGlobalObjectGPR); 
            m_jit.compare32(JITCompiler::Equal, localGlobalObjectGPR, remoteGlobalObjectGPR, result.gpr());
        }

        notMasqueradesAsUndefined.link(&m_jit);
        done.link(&m_jit);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsBoolean: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value, TagWord);
        
        m_jit.compare32(JITCompiler::Equal, value.tagGPR(), JITCompiler::TrustedImm32(JSValue::BooleanTag), result.gpr());
        booleanResult(result.gpr(), node);
        break;
    }

    case IsNumber: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value, TagWord);
        
        m_jit.add32(TrustedImm32(1), value.tagGPR(), result.gpr());
        m_jit.compare32(JITCompiler::Below, result.gpr(), JITCompiler::TrustedImm32(JSValue::LowestTag + 1), result.gpr());
        booleanResult(result.gpr(), node);
        break;
    }

    case IsString: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value, TagWord);
        
        JITCompiler::Jump isNotCell = branchNotCell(value.jsValueRegs());
        
        m_jit.compare8(JITCompiler::Equal, 
            JITCompiler::Address(value.payloadGPR(), JSCell::typeInfoTypeOffset()), 
            TrustedImm32(StringType), 
            result.gpr());
        JITCompiler::Jump done = m_jit.jump();
        
        isNotCell.link(&m_jit);
        m_jit.move(TrustedImm32(0), result.gpr());
        
        done.link(&m_jit);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsObject: {
        JSValueOperand value(this, node->child1());
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsObject, resultGPR, valueTagGPR, valuePayloadGPR);
        booleanResult(result.gpr(), node);
        break;
    }

    case IsFunction: {
        JSValueOperand value(this, node->child1());
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        flushRegisters();
        callOperation(operationIsFunction, resultGPR, valueTagGPR, valuePayloadGPR);
        booleanResult(result.gpr(), node);
        break;
    }
    case TypeOf: {
        JSValueOperand value(this, node->child1(), ManualOperandSpeculation);
        GPRReg tagGPR = value.tagGPR();
        GPRReg payloadGPR = value.payloadGPR();
        GPRTemporary temp(this);
        GPRReg tempGPR = temp.gpr();
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
        JITCompiler::JumpList doneJumps;

        flushRegisters();

        ASSERT(node->child1().useKind() == UntypedUse || node->child1().useKind() == CellUse || node->child1().useKind() == StringUse);

        JITCompiler::Jump isNotCell = branchNotCell(value.jsValueRegs());
        if (node->child1().useKind() != UntypedUse)
            DFG_TYPE_CHECK(JSValueRegs(tagGPR, payloadGPR), node->child1(), SpecCell, isNotCell);

        if (!node->child1()->shouldSpeculateObject() || node->child1().useKind() == StringUse) {
            JITCompiler::Jump notString = m_jit.branch8(
                JITCompiler::NotEqual, 
                JITCompiler::Address(payloadGPR, JSCell::typeInfoTypeOffset()), 
                TrustedImm32(StringType));
            if (node->child1().useKind() == StringUse)
                DFG_TYPE_CHECK(JSValueRegs(tagGPR, payloadGPR), node->child1(), SpecString, notString);
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.stringString()), resultGPR);
            doneJumps.append(m_jit.jump());
            if (node->child1().useKind() != StringUse) {
                notString.link(&m_jit);
                callOperation(operationTypeOf, resultGPR, payloadGPR);
                doneJumps.append(m_jit.jump());
            }
        } else {
            callOperation(operationTypeOf, resultGPR, payloadGPR);
            doneJumps.append(m_jit.jump());
        }

        if (node->child1().useKind() == UntypedUse) {
            isNotCell.link(&m_jit);

            m_jit.add32(TrustedImm32(1), tagGPR, tempGPR);
            JITCompiler::Jump notNumber = m_jit.branch32(JITCompiler::AboveOrEqual, tempGPR, JITCompiler::TrustedImm32(JSValue::LowestTag + 1));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.numberString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notNumber.link(&m_jit);

            JITCompiler::Jump notUndefined = m_jit.branch32(JITCompiler::NotEqual, tagGPR, TrustedImm32(JSValue::UndefinedTag));
            m_jit.move(TrustedImmPtr(m_jit.vm()->smallStrings.undefinedString()), resultGPR);
            doneJumps.append(m_jit.jump());
            notUndefined.link(&m_jit);

            JITCompiler::Jump notNull = m_jit.branch32(JITCompiler::NotEqual, tagGPR, TrustedImm32(JSValue::NullTag));
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
    case ProfiledCall:
    case ProfiledConstruct:
        emitCall(node);
        break;

    case CreateActivation: {
        JSValueOperand value(this, node->child1());
        GPRTemporary result(this, Reuse, value, PayloadWord);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valuePayloadGPR, resultGPR);
        
        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
        
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
        GPRTemporary result(this, Reuse, value, PayloadWord);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratch1GPR = scratch1.gpr();
        GPRReg scratch2GPR = scratch2.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.move(valuePayloadGPR, resultGPR);
        
        if (node->origin.semantic.inlineCallFrame) {
            JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
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
            JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
            addSlowPathGenerator(
                slowPathCall(notCreated, this, operationCreateArguments, resultGPR));
            cellResult(resultGPR, node);
            break;
        }

        JITCompiler::Jump alreadyCreated = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));

        MacroAssembler::JumpList slowPaths;
        emitAllocateArguments(resultGPR, scratch1GPR, scratch2GPR, slowPaths);
            addSlowPathGenerator(
                slowPathCall(slowPaths, this, operationCreateArguments, resultGPR));

        alreadyCreated.link(&m_jit); 
        cellResult(resultGPR, node);
        break;
    }
        
    case TearOffActivation: {
        JSValueOperand activationValue(this, node->child1());
        GPRTemporary scratch(this);
        
        GPRReg activationValueTagGPR = activationValue.tagGPR();
        GPRReg activationValuePayloadGPR = activationValue.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();

        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, activationValueTagGPR, TrustedImm32(JSValue::EmptyValueTag));

        SymbolTable* symbolTable = m_jit.symbolTableFor(node->origin.semantic);
        int registersOffset = JSActivation::registersOffset(symbolTable);

        int bytecodeCaptureStart = symbolTable->captureStart();
        int machineCaptureStart = m_jit.graph().m_machineCaptureStart;
        for (int i = symbolTable->captureCount(); i--;) {
            m_jit.loadPtr(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister, (machineCaptureStart - i) * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                scratchGPR);
            m_jit.storePtr(
                scratchGPR, JITCompiler::Address(
                    activationValuePayloadGPR, registersOffset + (bytecodeCaptureStart - i) * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
            m_jit.loadPtr(
                JITCompiler::Address(
                    GPRInfo::callFrameRegister, (machineCaptureStart - i) * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                scratchGPR);
            m_jit.storePtr(
                scratchGPR, JITCompiler::Address(
                    activationValuePayloadGPR, registersOffset + (bytecodeCaptureStart - i) * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        }
        m_jit.addPtr(TrustedImm32(registersOffset), activationValuePayloadGPR, scratchGPR);
        m_jit.storePtr(scratchGPR, JITCompiler::Address(activationValuePayloadGPR, JSActivation::offsetOfRegisters()));
        
        notCreated.link(&m_jit);
        noResult(node);
        break;
    }
        
    case TearOffArguments: {
        JSValueOperand unmodifiedArgumentsValue(this, node->child1());
        JSValueOperand activationValue(this, node->child2());
        GPRReg unmodifiedArgumentsValuePayloadGPR = unmodifiedArgumentsValue.payloadGPR();
        GPRReg activationValuePayloadGPR = activationValue.payloadGPR();
        
        JITCompiler::Jump created = m_jit.branchTest32(
            JITCompiler::NonZero, unmodifiedArgumentsValuePayloadGPR);
        
        if (node->origin.semantic.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffInlinedArguments, NoResult,
                    unmodifiedArgumentsValuePayloadGPR, activationValuePayloadGPR, node->origin.semantic.inlineCallFrame));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    created, this, operationTearOffArguments, NoResult,
                    unmodifiedArgumentsValuePayloadGPR, activationValuePayloadGPR));
        }
        
        noResult(node);
        break;
    }
        
    case CheckArgumentsNotCreated: {
        ASSERT(!isEmptySpeculation(
            m_state.variables().operand(
                m_jit.graph().argumentsRegisterFor(node->origin.semantic)).m_type));
        speculationCheck(
            Uncountable, JSValueRegs(), 0,
            m_jit.branch32(
                JITCompiler::NotEqual,
                JITCompiler::tagFor(m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)),
                TrustedImm32(JSValue::EmptyValueTag)));
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
                m_jit.branch32(
                    JITCompiler::NotEqual,
                    JITCompiler::tagFor(m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)),
                    TrustedImm32(JSValue::EmptyValueTag)));
        }
        
        ASSERT(!node->origin.semantic.inlineCallFrame);
        m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultGPR);
        m_jit.sub32(TrustedImm32(1), resultGPR);
        int32Result(resultGPR, node);
        break;
    }
        
    case GetMyArgumentsLengthSafe: {
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        JITCompiler::Jump created = m_jit.branch32(
            JITCompiler::NotEqual,
            JITCompiler::tagFor(m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)),
            TrustedImm32(JSValue::EmptyValueTag));
        
        if (node->origin.semantic.inlineCallFrame) {
            m_jit.move(
                Imm32(node->origin.semantic.inlineCallFrame->arguments.size() - 1),
                resultPayloadGPR);
        } else {
            m_jit.load32(JITCompiler::payloadFor(JSStack::ArgumentCount), resultPayloadGPR);
            m_jit.sub32(TrustedImm32(1), resultPayloadGPR);
        }
        m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
        
        // FIXME: the slow path generator should perform a forward speculation that the
        // result is an integer. For now we postpone the speculation by having this return
        // a JSValue.
        
        addSlowPathGenerator(
            slowPathCall(
                created, this, operationGetArgumentsLength,
                JSValueRegs(resultTagGPR, resultPayloadGPR),
                m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset()));
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case GetMyArgumentByVal: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        if (!isEmptySpeculation(
                m_state.variables().operand(
                    m_jit.graph().argumentsRegisterFor(node->origin.semantic)).m_type)) {
            speculationCheck(
                ArgumentsEscaped, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::NotEqual,
                    JITCompiler::tagFor(m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)),
                    TrustedImm32(JSValue::EmptyValueTag)));
        }
            
        m_jit.add32(TrustedImm32(1), indexGPR, resultPayloadGPR);
            
        if (node->origin.semantic.inlineCallFrame) {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    Imm32(node->origin.semantic.inlineCallFrame->arguments.size())));
        } else {
            speculationCheck(
                Uncountable, JSValueRegs(), 0,
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }
        
        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (m_jit.symbolTableFor(node->origin.semantic)->slowArguments()) {
            RELEASE_ASSERT(!node->origin.semantic.inlineCallFrame);
            const SlowArgument* slowArguments = m_jit.graph().m_slowArguments.get();
            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->origin.semantic)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultPayloadGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultPayloadGPR);

            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                resultTagGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                resultPayloadGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
            resultTagGPR);
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
            resultPayloadGPR);
            
        slowArgument.link(&m_jit);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case GetMyArgumentByValSafe: {
        SpeculateStrictInt32Operand index(this, node->child1());
        GPRTemporary resultPayload(this);
        GPRTemporary resultTag(this);
        GPRReg indexGPR = index.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        
        JITCompiler::JumpList slowPath;
        slowPath.append(
            m_jit.branch32(
                JITCompiler::NotEqual,
                JITCompiler::tagFor(m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic)),
                TrustedImm32(JSValue::EmptyValueTag)));
        
        m_jit.add32(TrustedImm32(1), indexGPR, resultPayloadGPR);
        if (node->origin.semantic.inlineCallFrame) {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    Imm32(node->origin.semantic.inlineCallFrame->arguments.size())));
        } else {
            slowPath.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual,
                    resultPayloadGPR,
                    JITCompiler::payloadFor(JSStack::ArgumentCount)));
        }
        
        JITCompiler::JumpList slowArgument;
        JITCompiler::JumpList slowArgumentOutOfBounds;
        if (m_jit.symbolTableFor(node->origin.semantic)->slowArguments()) {
            RELEASE_ASSERT(!node->origin.semantic.inlineCallFrame);
            const SlowArgument* slowArguments = m_jit.graph().m_slowArguments.get();
            slowArgumentOutOfBounds.append(
                m_jit.branch32(
                    JITCompiler::AboveOrEqual, indexGPR,
                    Imm32(m_jit.symbolTableFor(node->origin.semantic)->parameterCount())));

            COMPILE_ASSERT(sizeof(SlowArgument) == 8, SlowArgument_size_is_eight_bytes);
            m_jit.move(ImmPtr(slowArguments), resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    resultPayloadGPR, indexGPR, JITCompiler::TimesEight, 
                    OBJECT_OFFSETOF(SlowArgument, index)), 
                resultPayloadGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
                resultTagGPR);
            m_jit.load32(
                JITCompiler::BaseIndex(
                    GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                    OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
                resultPayloadGPR);
            slowArgument.append(m_jit.jump());
        }
        slowArgumentOutOfBounds.link(&m_jit);

        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)),
            resultTagGPR);
        m_jit.load32(
            JITCompiler::BaseIndex(
                GPRInfo::callFrameRegister, resultPayloadGPR, JITCompiler::TimesEight,
                m_jit.offsetOfArgumentsIncludingThis(node->origin.semantic) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)),
            resultPayloadGPR);
        
        if (node->origin.semantic.inlineCallFrame) {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetInlinedArgumentByVal,
                    JSValueRegs(resultTagGPR, resultPayloadGPR),
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset(),
                    node->origin.semantic.inlineCallFrame, indexGPR));
        } else {
            addSlowPathGenerator(
                slowPathCall(
                    slowPath, this, operationGetArgumentByVal,
                    JSValueRegs(resultTagGPR, resultPayloadGPR),
                    m_jit.graph().machineArgumentsRegisterFor(node->origin.semantic).offset(),
                    indexGPR));
        }
        
        slowArgument.link(&m_jit);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case NewFunctionNoCheck:
        compileNewFunctionNoCheck(node);
        break;
        
    case NewFunction: {
        JSValueOperand value(this, node->child1());
        GPRTemporary resultTag(this, Reuse, value, TagWord);
        GPRTemporary resultPayload(this, Reuse, value, PayloadWord);
        
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        m_jit.move(valuePayloadGPR, resultPayloadGPR);
        m_jit.move(valueTagGPR, resultTagGPR);
        
        JITCompiler::Jump notCreated = m_jit.branch32(JITCompiler::Equal, valueTagGPR, TrustedImm32(JSValue::EmptyValueTag));
        
        addSlowPathGenerator(
            slowPathCall(
                notCreated, this, operationNewFunction, JSValueRegs(resultTagGPR, resultPayloadGPR),
                m_jit.codeBlock()->functionDecl(node->functionDeclIndex())));
        
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
        
    case NewFunctionExpression:
        compileNewFunctionExpression(node);
        break;
        
    case In:
        compileIn(node);
        break;

    case StoreBarrier:
    case StoreBarrierWithNullCheck: {
        compileStoreBarrier(node);
        break;
    }

    case GetEnumerableLength: {
        SpeculateCellOperand base(this, node->child1());
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        flushRegisters();
        callOperation(operationGetEnumerableLength, resultGPR, base.gpr());
        int32Result(resultGPR, node);
        break;
    }
    case HasGenericProperty: {
        JSValueOperand base(this, node->child1());
        SpeculateCellOperand property(this, node->child2());
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        GPRReg basePayloadGPR = base.payloadGPR();
        GPRReg baseTagGPR = base.tagGPR();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();

        flushRegisters();
        callOperation(operationHasGenericProperty, resultTagGPR, resultPayloadGPR, baseTagGPR, basePayloadGPR, property.gpr());
        booleanResult(resultPayloadGPR, node);
        break;
    }
    case HasStructureProperty: {
        JSValueOperand base(this, node->child1());
        SpeculateCellOperand property(this, node->child2());
        SpeculateCellOperand enumerator(this, node->child3());
        GPRTemporary scratch(this);
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);

        GPRReg baseTagGPR = base.tagGPR();
        GPRReg basePayloadGPR = base.payloadGPR();
        GPRReg propertyGPR = property.gpr();
        GPRReg scratchGPR = scratch.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();

        m_jit.load32(MacroAssembler::Address(basePayloadGPR, JSCell::structureIDOffset()), scratchGPR);
        MacroAssembler::Jump wrongStructure = m_jit.branch32(MacroAssembler::NotEqual, 
            scratchGPR, 
            MacroAssembler::Address(enumerator.gpr(), JSPropertyNameEnumerator::cachedStructureIDOffset()));

        moveTrueTo(resultPayloadGPR);
        MacroAssembler::Jump done = m_jit.jump();

        done.link(&m_jit);

        addSlowPathGenerator(slowPathCall(wrongStructure, this, operationHasGenericProperty, resultTagGPR, resultPayloadGPR, baseTagGPR, basePayloadGPR, propertyGPR));
        booleanResult(resultPayloadGPR, node);
        break;
    }
    case HasIndexedProperty: {
        SpeculateCellOperand base(this, node->child1());
        SpeculateInt32Operand index(this, node->child2());
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);

        GPRReg baseGPR = base.gpr();
        GPRReg indexGPR = index.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg resultTagGPR = resultTag.gpr();

        MacroAssembler::JumpList slowCases;
        ArrayMode mode = node->arrayMode();
        switch (mode.type()) {
        case Array::Int32:
        case Array::Contiguous: {
            ASSERT(!!node->child3());
            StorageOperand storage(this, node->child3());
            GPRTemporary scratch(this);
            
            GPRReg storageGPR = storage.gpr();
            GPRReg scratchGPR = scratch.gpr();

            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, indexGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength())));
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, indexGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), scratchGPR);
            slowCases.append(m_jit.branch32(MacroAssembler::Equal, scratchGPR, TrustedImm32(JSValue::EmptyValueTag)));
            break;
        }
        case Array::Double: {
            ASSERT(!!node->child3());
            StorageOperand storage(this, node->child3());
            FPRTemporary scratch(this);
            FPRReg scratchFPR = scratch.fpr();
            GPRReg storageGPR = storage.gpr();

            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, indexGPR, MacroAssembler::Address(storageGPR, Butterfly::offsetOfPublicLength())));
            m_jit.loadDouble(MacroAssembler::BaseIndex(storageGPR, indexGPR, MacroAssembler::TimesEight), scratchFPR);
            slowCases.append(m_jit.branchDouble(MacroAssembler::DoubleNotEqualOrUnordered, scratchFPR, scratchFPR));
            break;
        }
        case Array::ArrayStorage: {
            ASSERT(!!node->child3());
            StorageOperand storage(this, node->child3());
            GPRTemporary scratch(this);

            GPRReg storageGPR = storage.gpr();
            GPRReg scratchGPR = scratch.gpr();

            slowCases.append(m_jit.branch32(MacroAssembler::AboveOrEqual, indexGPR, MacroAssembler::Address(storageGPR, ArrayStorage::vectorLengthOffset())));
            m_jit.load32(MacroAssembler::BaseIndex(storageGPR, indexGPR, MacroAssembler::TimesEight, ArrayStorage::vectorOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), scratchGPR);
            slowCases.append(m_jit.branch32(MacroAssembler::Equal, scratchGPR, TrustedImm32(JSValue::EmptyValueTag)));
            break;
        }
        default: {
            slowCases.append(m_jit.jump());
            break;
        }
        }

        moveTrueTo(resultPayloadGPR);
        MacroAssembler::Jump done = m_jit.jump();

        addSlowPathGenerator(slowPathCall(slowCases, this, operationHasIndexedProperty, resultTagGPR, resultPayloadGPR, baseGPR, indexGPR));
        
        done.link(&m_jit);
        booleanResult(resultPayloadGPR, node);
        break;
    }
    case GetDirectPname: {
        Edge& baseEdge = m_jit.graph().varArgChild(node, 0);
        Edge& propertyEdge = m_jit.graph().varArgChild(node, 1);

        SpeculateCellOperand base(this, baseEdge);
        SpeculateCellOperand property(this, propertyEdge);
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        GPRTemporary scratch(this);

        GPRReg baseGPR = base.gpr();
        GPRReg propertyGPR = property.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg scratchGPR = scratch.gpr();

#if CPU(X86)
        // Not enough registers on X86 for this code, so always use the slow path.
        flushRegisters();
        m_jit.move(MacroAssembler::TrustedImm32(JSValue::CellTag), scratchGPR);
        callOperation(operationGetByValCell, resultTagGPR, resultPayloadGPR, baseGPR, scratchGPR, propertyGPR);
#else
        Edge& indexEdge = m_jit.graph().varArgChild(node, 2);
        Edge& enumeratorEdge = m_jit.graph().varArgChild(node, 3);

        SpeculateInt32Operand index(this, indexEdge);
        SpeculateCellOperand enumerator(this, enumeratorEdge);

        GPRReg indexGPR = index.gpr();
        GPRReg enumeratorGPR = enumerator.gpr();

        // Check the structure
        m_jit.load32(MacroAssembler::Address(baseGPR, JSCell::structureIDOffset()), scratchGPR);
        MacroAssembler::Jump wrongStructure = m_jit.branch32(MacroAssembler::NotEqual, 
            scratchGPR, MacroAssembler::Address(enumeratorGPR, JSPropertyNameEnumerator::cachedStructureIDOffset()));
        
        // Compute the offset
        // If index is less than the enumerator's cached inline storage, then it's an inline access
        MacroAssembler::Jump outOfLineAccess = m_jit.branch32(MacroAssembler::AboveOrEqual, 
            indexGPR, MacroAssembler::Address(enumeratorGPR, JSPropertyNameEnumerator::cachedInlineCapacityOffset()));

        m_jit.move(indexGPR, scratchGPR);
        m_jit.signExtend32ToPtr(scratchGPR, scratchGPR);
        m_jit.load32(MacroAssembler::BaseIndex(baseGPR, scratchGPR, MacroAssembler::TimesEight, JSObject::offsetOfInlineStorage() + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagGPR);
        m_jit.load32(MacroAssembler::BaseIndex(baseGPR, scratchGPR, MacroAssembler::TimesEight, JSObject::offsetOfInlineStorage() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadGPR);

        MacroAssembler::Jump done = m_jit.jump();
        
        // Otherwise it's out of line
        outOfLineAccess.link(&m_jit);
        m_jit.move(indexGPR, scratchGPR);
        m_jit.sub32(MacroAssembler::Address(enumeratorGPR, JSPropertyNameEnumerator::cachedInlineCapacityOffset()), scratchGPR);
        m_jit.neg32(scratchGPR);
        m_jit.signExtend32ToPtr(scratchGPR, scratchGPR);
        // We use resultPayloadGPR as a temporary here. We have to make sure clobber it after getting the 
        // value out of indexGPR and enumeratorGPR because resultPayloadGPR could reuse either of those registers.
        m_jit.loadPtr(MacroAssembler::Address(baseGPR, JSObject::butterflyOffset()), resultPayloadGPR); 
        int32_t offsetOfFirstProperty = static_cast<int32_t>(offsetInButterfly(firstOutOfLineOffset)) * sizeof(EncodedJSValue);
        m_jit.load32(MacroAssembler::BaseIndex(resultPayloadGPR, scratchGPR, MacroAssembler::TimesEight, offsetOfFirstProperty + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTagGPR);
        m_jit.load32(MacroAssembler::BaseIndex(resultPayloadGPR, scratchGPR, MacroAssembler::TimesEight, offsetOfFirstProperty + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayloadGPR);

        done.link(&m_jit);

        addSlowPathGenerator(slowPathCall(wrongStructure, this, operationGetByValCell, resultTagGPR, resultPayloadGPR, baseGPR, propertyGPR));
#endif

        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case GetStructurePropertyEnumerator: {
        SpeculateCellOperand base(this, node->child1());
        SpeculateInt32Operand length(this, node->child2());
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        flushRegisters();
        callOperation(operationGetStructurePropertyEnumerator, resultGPR, base.gpr(), length.gpr());
        cellResult(resultGPR, node);
        break;
    }
    case GetGenericPropertyEnumerator: {
        SpeculateCellOperand base(this, node->child1());
        SpeculateInt32Operand length(this, node->child2());
        SpeculateCellOperand enumerator(this, node->child3());
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        flushRegisters();
        callOperation(operationGetGenericPropertyEnumerator, resultGPR, base.gpr(), length.gpr(), enumerator.gpr());
        cellResult(resultGPR, node);
        break;
    }
    case GetEnumeratorPname: {
        SpeculateCellOperand enumerator(this, node->child1());
        SpeculateInt32Operand index(this, node->child2());
        GPRTemporary scratch(this);
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);

        GPRReg enumeratorGPR = enumerator.gpr();
        GPRReg indexGPR = index.gpr();
        GPRReg scratchGPR = scratch.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();

        MacroAssembler::Jump inBounds = m_jit.branch32(MacroAssembler::Below, 
            indexGPR, MacroAssembler::Address(enumeratorGPR, JSPropertyNameEnumerator::cachedPropertyNamesLengthOffset()));

        m_jit.move(MacroAssembler::TrustedImm32(JSValue::NullTag), resultTagGPR);
        m_jit.move(MacroAssembler::TrustedImm32(0), resultPayloadGPR);

        MacroAssembler::Jump done = m_jit.jump();
        inBounds.link(&m_jit);

        m_jit.loadPtr(MacroAssembler::Address(enumeratorGPR, JSPropertyNameEnumerator::cachedPropertyNamesVectorOffset()), scratchGPR);
        m_jit.loadPtr(MacroAssembler::BaseIndex(scratchGPR, indexGPR, MacroAssembler::ScalePtr), resultPayloadGPR);
        m_jit.move(MacroAssembler::TrustedImm32(JSValue::CellTag), resultTagGPR);

        done.link(&m_jit);
        jsValueResult(resultTagGPR, resultPayloadGPR, node);
        break;
    }
    case ToIndexString: {
        SpeculateInt32Operand index(this, node->child1());
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        flushRegisters();
        callOperation(operationToIndexString, resultGPR, index.gpr());
        cellResult(resultGPR, node);
        break;
    }

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

    case CountExecution:
        m_jit.add64(TrustedImm32(1), MacroAssembler::AbsoluteAddress(node->executionCounter()->address()));
        break;

    case Phantom:
    case HardPhantom:
    case Check:
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
        RELEASE_ASSERT_NOT_REACHED();
        break;

    case LastNodeType:
    case Phi:
    case Upsilon:
    case GetArgument:
    case ExtractOSREntryLocal:
    case CheckTierUpInLoop:
    case CheckTierUpAtReturn:
    case CheckTierUpAndOSREnter:
    case Int52Rep:
    case FiatInt52:
    case Int52Constant:
    case CheckInBounds:
    case ArithIMul:
    case MultiGetByOffset:
    case MultiPutByOffset:
    case NativeCall:
    case NativeConstruct:
    case CheckBadCell:
    case BottomValue:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    if (!m_compileOkay)
        return;
    
    if (node->hasResult() && node->mustGenerate())
        use(node);
}

#if ENABLE(GGC)
void SpeculativeJIT::writeBarrier(GPRReg ownerGPR, GPRReg valueTagGPR, Edge valueUse, GPRReg scratch1, GPRReg scratch2)
{
    JITCompiler::Jump isNotCell;
    if (!isKnownCell(valueUse.node()))
        isNotCell = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, JITCompiler::TrustedImm32(JSValue::CellTag));

    JITCompiler::Jump ownerNotMarkedOrAlreadyRemembered = m_jit.checkMarkByte(ownerGPR);
    storeToWriteBarrierBuffer(ownerGPR, scratch1, scratch2);
    ownerNotMarkedOrAlreadyRemembered.link(&m_jit);

    if (!isKnownCell(valueUse.node()))
        isNotCell.link(&m_jit);
}

void SpeculativeJIT::writeBarrier(JSCell* owner, GPRReg valueTagGPR, Edge valueUse, GPRReg scratch1, GPRReg scratch2)
{
    JITCompiler::Jump isNotCell;
    if (!isKnownCell(valueUse.node()))
        isNotCell = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, JITCompiler::TrustedImm32(JSValue::CellTag));

    JITCompiler::Jump ownerNotMarkedOrAlreadyRemembered = m_jit.checkMarkByte(owner);
    storeToWriteBarrierBuffer(owner, scratch1, scratch2);
    ownerNotMarkedOrAlreadyRemembered.link(&m_jit);

    if (!isKnownCell(valueUse.node()))
        isNotCell.link(&m_jit);
}
#endif // ENABLE(GGC)

JITCompiler::Jump SpeculativeJIT::branchIsCell(JSValueRegs regs)
{
    return m_jit.branch32(MacroAssembler::Equal, regs.tagGPR(), TrustedImm32(JSValue::CellTag));
}

JITCompiler::Jump SpeculativeJIT::branchNotCell(JSValueRegs regs)
{
    return m_jit.branch32(MacroAssembler::NotEqual, regs.tagGPR(), TrustedImm32(JSValue::CellTag));
}

JITCompiler::Jump SpeculativeJIT::branchIsOther(JSValueRegs regs, GPRReg tempGPR)
{
    m_jit.or32(TrustedImm32(1), regs.tagGPR(), tempGPR);
    return m_jit.branch32(
        MacroAssembler::Equal, tempGPR,
        MacroAssembler::TrustedImm32(JSValue::NullTag));
}

JITCompiler::Jump SpeculativeJIT::branchNotOther(JSValueRegs regs, GPRReg tempGPR)
{
    m_jit.or32(TrustedImm32(1), regs.tagGPR(), tempGPR);
    return m_jit.branch32(
        MacroAssembler::NotEqual, tempGPR,
        MacroAssembler::TrustedImm32(JSValue::NullTag));
}

void SpeculativeJIT::moveTrueTo(GPRReg gpr)
{
    m_jit.move(TrustedImm32(1), gpr);
}

void SpeculativeJIT::moveFalseTo(GPRReg gpr)
{
    m_jit.move(TrustedImm32(0), gpr);
}

void SpeculativeJIT::blessBoolean(GPRReg)
{
}

#endif

} } // namespace JSC::DFG

#endif
