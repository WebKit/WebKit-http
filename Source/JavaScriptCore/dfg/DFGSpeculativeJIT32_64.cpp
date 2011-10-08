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

#include "DFGJITCompilerInlineMethods.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE32_64)

template<bool strict>
GPRReg SpeculativeJIT::fillSpeculateIntInternal(NodeIndex nodeIndex, DataFormat& returnFormat)
{
#if ENABLE(DFG_DEBUG_VERBOSE)
    fprintf(stderr, "SpecInt@%d   ", nodeIndex);
#endif
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (node.hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(nodeIndex)) {
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                info.fillInteger(gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            terminateSpeculativeExecution();
            returnFormat = DataFormatInteger;
            return allocate();
        }

        DataFormat spillFormat = info.spillFormat();
        ASSERT(spillFormat & DataFormatJS);

        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);

        // If we know this was spilled as an integer we can fill without checking.
        if (spillFormat != DataFormatJSInteger)
            speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, JITCompiler::tagFor(virtualRegister), TrustedImm32(JSValue::Int32Tag)));

        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        info.fillInteger(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    case DataFormatJSInteger:
    case DataFormatJS: {
        // Check the value is an integer.
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        if (info.registerFormat() != DataFormatJSInteger) 
            speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::Int32Tag)));
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        info.fillInteger(payloadGPR);
        // If !strict we're done, return.
        returnFormat = DataFormatInteger;
        return payloadGPR;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    case DataFormatDouble:
    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        terminateSpeculativeExecution();
        returnFormat = DataFormatInteger;
        return allocate();
    }

    case DataFormatStorage:
        ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

GPRReg SpeculativeJIT::fillSpeculateInt(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    return fillSpeculateIntInternal<false>(nodeIndex, returnFormat);
}

GPRReg SpeculativeJIT::fillSpeculateIntStrict(NodeIndex nodeIndex)
{
    DataFormat mustBeDataFormatInteger;
    GPRReg result = fillSpeculateIntInternal<true>(nodeIndex, mustBeDataFormatInteger);
    ASSERT(mustBeDataFormatInteger == DataFormatInteger);
    return result;
}

FPRReg SpeculativeJIT::fillSpeculateDouble(NodeIndex nodeIndex)
{
#if ENABLE(DFG_DEBUG_VERBOSE)
    fprintf(stderr, "SpecDouble@%d   ", nodeIndex);
#endif
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {

        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex)) {
                GPRReg gpr = allocate();
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillInteger(gpr);
                unlock(gpr);
            } else if (isNumberConstant(nodeIndex)) {
                FPRReg fpr = fprAllocate();
                m_jit.loadDouble(addressOfDoubleConstant(nodeIndex), fpr);
                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(fpr);
                return fpr;
            } else {
                terminateSpeculativeExecution();
                return fprAllocate();
            }
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            if (spillFormat == DataFormatJSDouble) {
                FPRReg fpr = fprAllocate();
                m_jit.loadDouble(JITCompiler::addressFor(virtualRegister), fpr);
                m_fprs.retain(fpr, virtualRegister, SpillOrderSpilled);
                info.fillDouble(fpr);
                return fpr;
            } 
            GPRReg tag = allocate();
            GPRReg payload = allocate();
            m_jit.emitLoad(nodeIndex, tag, payload);
            m_gprs.retain(tag, virtualRegister, SpillOrderSpilled);
            m_gprs.retain(payload, virtualRegister, SpillOrderSpilled);
            info.fillJSValue(tag, payload, spillFormat);
            unlock(tag);
            unlock(payload);
        }
    }

    switch (info.registerFormat()) {
    case DataFormatNone:
    case DataFormatCell:
    case DataFormatBoolean:
    case DataFormatStorage:
        // Should have filled, above.
        ASSERT_NOT_REACHED();
        
    case DataFormatJSCell:
    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSBoolean: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        FPRReg fpr = fprAllocate();

        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);

        JITCompiler::Jump hasUnboxedDouble;

        if (info.registerFormat() != DataFormatJSInteger) {
            JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));
            speculationCheck(m_jit.branch32(MacroAssembler::AboveOrEqual, tagGPR, TrustedImm32(JSValue::LowestTag)));
            unboxDouble(tagGPR, payloadGPR, fpr, virtualRegister);
            hasUnboxedDouble = m_jit.jump();
            isInteger.link(&m_jit);
        }

        m_jit.convertInt32ToDouble(payloadGPR, fpr);

        if (info.registerFormat() != DataFormatJSInteger)
            hasUnboxedDouble.link(&m_jit);

        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.unlock(tagGPR);
        m_gprs.unlock(payloadGPR);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(fpr);
        info.killSpilled();
        return fpr;
    }

    case DataFormatInteger: {
        FPRReg fpr = fprAllocate();
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.convertInt32ToDouble(gpr, fpr);
        m_gprs.unlock(gpr);
        return fpr;
    }

    case DataFormatJSDouble:
    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        m_fprs.lock(fpr);
        return fpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidFPRReg;
}

GPRReg SpeculativeJIT::fillSpeculateCell(NodeIndex nodeIndex)
{
#if ENABLE(DFG_DEBUG_VERBOSE)
    fprintf(stderr, "SpecCell@%d   ", nodeIndex);
#endif
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {

        GPRReg gpr = allocate();
        if (node.hasConstant()) {
            JSValue jsValue = valueOfJSConstant(nodeIndex);
            if (jsValue.isCell()) {
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                m_jit.move(MacroAssembler::TrustedImmPtr(jsValue.asCell()), gpr);
                info.fillCell(gpr);
                return gpr;
            }
            terminateSpeculativeExecution();
            return gpr;
        }
        ASSERT(info.spillFormat() & DataFormatJS);
        m_jit.load32(JITCompiler::tagFor(virtualRegister), gpr);
        if (info.spillFormat() != DataFormatJSCell)
            speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, gpr, TrustedImm32(JSValue::CellTag)));
        m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
        info.fillCell(gpr);
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
        if (info.spillFormat() != DataFormatJSCell)
            speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, tagGPR, TrustedImm32(JSValue::CellTag)));
        m_gprs.unlock(tagGPR);
        m_gprs.release(tagGPR);
        m_gprs.release(payloadGPR);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderCell);
        info.fillCell(payloadGPR);
        return payloadGPR;
    }

    case DataFormatJSInteger:
    case DataFormatInteger:
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJSBoolean:
    case DataFormatBoolean: {
        terminateSpeculativeExecution();
        return allocate();
    }

    case DataFormatStorage:
        ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

GPRReg SpeculativeJIT::fillSpeculateBoolean(NodeIndex nodeIndex)
{
    ASSERT_NOT_REACHED();
    UNUSED_PARAM(nodeIndex);

#if ENABLE(DFG_DEBUG_VERBOSE)
     fprintf(stderr, "SpecBool@%d   ", nodeIndex);
#endif
    return InvalidGPRReg;
}

JITCompiler::Jump SpeculativeJIT::convertToDouble(JSValueOperand& op, FPRReg result)
{
    JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, op.tagGPR(), TrustedImm32(JSValue::Int32Tag));
    JITCompiler::Jump notNumber = m_jit.branch32(MacroAssembler::AboveOrEqual, op.payloadGPR(), TrustedImm32(JSValue::LowestTag));

    unboxDouble(op.tagGPR(), op.payloadGPR(), result, at(op.index()).virtualRegister());
    JITCompiler::Jump done = m_jit.jump();

    isInteger.link(&m_jit);
    m_jit.convertInt32ToDouble(op.payloadGPR(), result);

    done.link(&m_jit);

    return notNumber;
}

void SpeculativeJIT::compileObjectEquality(Node& node, void* vptr)
{
    SpeculateCellOperand op1(this, node.child1());
    SpeculateCellOperand op2(this, node.child2());
    GPRTemporary resultTag(this, op1);
    GPRTemporary resultPayload(this, op1);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(op1GPR), MacroAssembler::TrustedImmPtr(vptr)));
    speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(op2GPR), MacroAssembler::TrustedImmPtr(vptr)));
    
    MacroAssembler::Jump falseCase = m_jit.branchPtr(MacroAssembler::NotEqual, op1GPR, op2GPR);
    m_jit.move(Imm32(1), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    falseCase.link(&m_jit);
    m_jit.move(Imm32(0), resultPayloadGPR);
    done.link(&m_jit);

    m_jit.move(Imm32(JSValue::BooleanTag), resultTagGPR);
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean);
}

// Returns true if the compare is fused with a subsequent branch.
bool SpeculativeJIT::compare(Node& node, MacroAssembler::RelationalCondition condition, MacroAssembler::DoubleCondition doubleCondition, Z_DFGOperation_EJJ operation)
{
    if (compilePeepHoleBranch(node, condition, doubleCondition, operation))
        return true;

    if (Node::shouldSpeculateFinalObject(at(node.child1()), at(node.child2())))
        compileObjectEquality(node, m_jit.globalData()->jsFinalObjectVPtr);
    else if (Node::shouldSpeculateArray(at(node.child1()), at(node.child2())))
        compileObjectEquality(node, m_jit.globalData()->jsArrayVPtr);
    else if (!at(node.child1()).shouldSpeculateNumber() && !at(node.child2()).shouldSpeculateNumber())
        nonSpeculativeNonPeepholeCompare(node, condition, operation);
    else if ((at(node.child1()).shouldSpeculateNumber() || at(node.child2()).shouldSpeculateNumber()) && !Node::shouldSpeculateInteger(at(node.child1()), at(node.child2()))) {
        // Normal case, not fused to branch.
        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);
        
        m_jit.move(Imm32(1), resultPayload.gpr());
        MacroAssembler::Jump trueCase = m_jit.branchDouble(doubleCondition, op1.fpr(), op2.fpr());
        m_jit.move(Imm32(0), resultPayload.gpr());
        trueCase.link(&m_jit);

        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex, DataFormatJSBoolean);
    } else {
        // Normal case, not fused to branch.
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        GPRTemporary resultTag(this, op1, op2);
        GPRTemporary resultPayload(this);
        
        m_jit.compare32(condition, op1.gpr(), op2.gpr(), resultPayload.gpr());
        
        // If we add a DataFormatBool, we should use it here.
        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex, DataFormatJSBoolean);
    }
    
    return false;
}

void SpeculativeJIT::compileValueAdd(Node& node)
{
    JSValueOperand op1(this, node.child1());
    JSValueOperand op2(this, node.child2());

    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg op1PayloadGPR = op1.payloadGPR();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();

    flushRegisters();
    
    GPRResult2 resultTag(this);
    GPRResult resultPayload(this);
    if (isKnownNotNumber(node.child1()) || isKnownNotNumber(node.child2()))
        callOperation(operationValueAddNotNumber, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
    else
        callOperation(operationValueAdd, resultTag.gpr(), resultPayload.gpr(), op1TagGPR, op1PayloadGPR, op2TagGPR, op2PayloadGPR);
    
    jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
}

void SpeculativeJIT::compileObjectOrOtherLogicalNot(NodeIndex nodeIndex, void* vptr)
{
    JSValueOperand value(this, nodeIndex);
    GPRTemporary resultTag(this);
    GPRTemporary resultPayload(this);
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());
    
    MacroAssembler::Jump notCell = m_jit.branch32(MacroAssembler::NotEqual, valueTagGPR, TrustedImm32(JSValue::CellTag));
    speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(valuePayloadGPR), MacroAssembler::TrustedImmPtr(vptr)));
    m_jit.move(TrustedImm32(1), resultPayloadGPR);
    MacroAssembler::Jump done = m_jit.jump();
    
    notCell.link(&m_jit);
    
    m_jit.move(valueTagGPR, resultPayloadGPR);
    m_jit.and32(MacroAssembler::TrustedImm32(JSValue::UndefinedTag), resultPayloadGPR);
    speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, resultPayloadGPR, TrustedImm32(JSValue::UndefinedTag)));
    m_jit.move(TrustedImm32(0), resultPayloadGPR);
    
    done.link(&m_jit);
    
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean);
}

void SpeculativeJIT::compileLogicalNot(Node& node)
{
    // FIXME: Need to add fast paths for known booleans.

    if (at(node.child1()).shouldSpeculateFinalObjectOrOther()) {
        compileObjectOrOtherLogicalNot(node.child1(), m_jit.globalData()->jsFinalObjectVPtr);
        return;
    }
    if (at(node.child1()).shouldSpeculateArrayOrOther()) {
        compileObjectOrOtherLogicalNot(node.child1(), m_jit.globalData()->jsArrayVPtr);
        return;
    }
    if (at(node.child1()).shouldSpeculateInteger()) {
        SpeculateIntegerOperand value(this, node.child1());
        GPRTemporary resultPayload(this, value);
        GPRTemporary resultTag(this);
        m_jit.compare32(MacroAssembler::Equal, value.gpr(), MacroAssembler::TrustedImm32(0), resultPayload.gpr());
        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex, DataFormatJSBoolean);
        return;
    }
    if (at(node.child1()).shouldSpeculateNumber()) {
        SpeculateDoubleOperand value(this, node.child1());
        FPRTemporary scratch(this);
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);
        m_jit.move(TrustedImm32(0), resultPayload.gpr());
        MacroAssembler::Jump nonZero = m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr());
        m_jit.move(TrustedImm32(1), resultPayload.gpr());
        nonZero.link(&m_jit);
        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex, DataFormatJSBoolean);
        return;
    }
    
    JSValueOperand value(this, node.child1());
    GPRTemporary resultTag(this, value);
    GPRTemporary resultPayload(this, value, false);
    speculationCheck(m_jit.branch32(JITCompiler::NotEqual, value.tagGPR(), TrustedImm32(JSValue::BooleanTag)));
    m_jit.move(value.payloadGPR(), resultPayload.gpr());
    m_jit.xor32(TrustedImm32(1), resultPayload.gpr());
    m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTag.gpr());

    // If we add a DataFormatBool, we should use it here.
    jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex, DataFormatJSBoolean);

    // This code is moved from nonSpeculativeLogicalNot, currently unused!
#if 0
    JSValueOperand arg1(this, node.child1());
    GPRTemporary resultTag(this, arg1);
    GPRTemporary resultPayload(this, arg1, false);
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
        
    arg1.use();

    JITCompiler::Jump fastCase = m_jit.branch32(JITCompiler::Equal, arg1TagGPR, TrustedImm32(JSValue::BooleanTag));
        
    silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
    callOperation(dfgConvertJSValueToBoolean, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR);
    silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
    JITCompiler::Jump doNot = m_jit.jump();
        
    fastCase.link(&m_jit);
    m_jit.move(arg1PayloadGPR, resultPayloadGPR);

    doNot.link(&m_jit);
    m_jit.xor32(TrustedImm32(1), resultPayloadGPR);
    m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTagGPR);
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
#endif
}

void SpeculativeJIT::emitObjectOrOtherBranch(NodeIndex nodeIndex, BlockIndex taken, BlockIndex notTaken, void *vptr)
{
    JSValueOperand value(this, nodeIndex);
    GPRReg valueTagGPR = value.tagGPR();
    GPRReg valuePayloadGPR = value.payloadGPR();
    
    MacroAssembler::Jump notCell = m_jit.branch32(MacroAssembler::NotEqual, valueTagGPR, TrustedImm32(JSValue::CellTag));
    speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(valuePayloadGPR), MacroAssembler::TrustedImmPtr(vptr)));
    addBranch(m_jit.jump(), taken);
    
    notCell.link(&m_jit);
    
    m_jit.and32(MacroAssembler::TrustedImm32(JSValue::UndefinedTag), valueTagGPR);
    speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, valueTagGPR, TrustedImm32(JSValue::UndefinedTag)));
    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
    
    noResult(m_compileIndex);
}

void SpeculativeJIT::emitBranch(Node& node)
{
    BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
    BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(node.notTakenBytecodeOffset());

    // FIXME: Add fast cases for known Boolean!

    if (at(node.child1()).shouldSpeculateFinalObjectOrOther()) {
        emitObjectOrOtherBranch(node.child1(), taken, notTaken, m_jit.globalData()->jsFinalObjectVPtr);
    } else if (at(node.child1()).shouldSpeculateArrayOrOther()) {
        emitObjectOrOtherBranch(node.child1(), taken, notTaken, m_jit.globalData()->jsArrayVPtr);
    } else if (at(node.child1()).shouldSpeculateNumber()) {
        if (at(node.child1()).shouldSpeculateInteger()) {
            bool invert = false;
            
            if (taken == (m_block + 1)) {
                invert = true;
                BlockIndex tmp = taken;
                taken = notTaken;
                notTaken = tmp;
            }

            SpeculateIntegerOperand value(this, node.child1());
            addBranch(m_jit.branchTest32(invert ? MacroAssembler::Zero : MacroAssembler::NonZero, value.gpr()), taken);
        } else {
            SpeculateDoubleOperand value(this, node.child1());
            FPRTemporary scratch(this);
            addBranch(m_jit.branchDoubleNonZero(value.fpr(), scratch.fpr()), taken);
        }
        
        if (notTaken != (m_block + 1))
            addBranch(m_jit.jump(), notTaken);
        
        noResult(m_compileIndex);
    } else {
        JSValueOperand value(this, node.child1());
        value.fill();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();

        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        use(node.child1());
    
        JITCompiler::Jump fastPath = m_jit.branch32(JITCompiler::Equal, valueTagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag));
        JITCompiler::Jump slowPath = m_jit.branch32(JITCompiler::NotEqual, valueTagGPR, JITCompiler::TrustedImm32(JSValue::BooleanTag));

        fastPath.link(&m_jit);
        addBranch(m_jit.branchTest32(JITCompiler::Zero, valuePayloadGPR), notTaken);
        addBranch(m_jit.jump(), taken);

        slowPath.link(&m_jit);
        silentSpillAllRegisters(resultGPR);
        callOperation(dfgConvertJSValueToBoolean, resultGPR, valueTagGPR, valuePayloadGPR);
        silentFillAllRegisters(resultGPR);
    
        addBranch(m_jit.branchTest8(JITCompiler::NonZero, resultGPR), taken);
        if (notTaken != (m_block + 1))
            addBranch(m_jit.jump(), notTaken);
    
        noResult(m_compileIndex, UseChildrenCalledExplicitly);
    }
}

void SpeculativeJIT::compile(Node& node)
{
    NodeType op = node.op;

    switch (op) {
    case JSConstant:
        initConstantInfo(m_compileIndex);
        break;

    case GetLocal: {
        PredictedType prediction = node.variableAccessData()->prediction();

        // If we have no prediction for this local, then don't attempt to compile.
        if (prediction == PredictNone) {
            terminateSpeculativeExecution();
            break;
        }
        
        GPRTemporary result(this);
        if (isInt32Prediction(prediction)) {
            m_jit.load32(JITCompiler::payloadFor(node.local()), result.gpr());

            // Like integerResult, but don't useChildren - our children are phi nodes,
            // and don't represent values within this dataflow with virtual registers.
            VirtualRegister virtualRegister = node.virtualRegister();
            m_gprs.retain(result.gpr(), virtualRegister, SpillOrderInteger);
            m_generationInfo[virtualRegister].initInteger(m_compileIndex, node.refCount(), result.gpr());
            break;
        }

        GPRTemporary tag(this);
        m_jit.load32(JITCompiler::payloadFor(node.local()), result.gpr());
        m_jit.load32(JITCompiler::tagFor(node.local()), tag.gpr());

        // Like jsValueResult, but don't useChildren - our children are phi nodes,
        // and don't represent values within this dataflow with virtual registers.
        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(result.gpr(), virtualRegister, SpillOrderJS);
        m_gprs.retain(tag.gpr(), virtualRegister, SpillOrderJS);

        DataFormat format;
        if (isArrayPrediction(prediction))
            format = DataFormatJSCell;
        else if (isBooleanPrediction(prediction))
            format = DataFormatJSBoolean;
        else
            format = DataFormatJS;

        m_generationInfo[virtualRegister].initJSValue(m_compileIndex, node.refCount(), tag.gpr(), result.gpr(), format);
        break;
    }

    case SetLocal: {
        // SetLocal doubles as a hint as to where a node will be stored and
        // as a speculation point. So before we speculate make sure that we
        // know where the child of this node needs to go in the virtual
        // register file.
        compileMovHint(node);
        
        // As far as OSR is concerned, we're on the bytecode index corresponding
        // to the *next* instruction, since we've already "executed" the
        // SetLocal and whatever other DFG Nodes are associated with the same
        // bytecode index as the SetLocal.
        ASSERT(m_bytecodeIndexForOSR == node.codeOrigin.bytecodeIndex());
        Node& nextNode = at(m_compileIndex + 1);
        
        m_bytecodeIndexForOSR = nextNode.codeOrigin.bytecodeIndex();
        
        PredictedType predictedType = node.variableAccessData()->prediction();
        if (isInt32Prediction(predictedType)) {
            SpeculateIntegerOperand value(this, node.child1());
            m_jit.store32(value.gpr(), JITCompiler::payloadFor(node.local()));
            noResult(m_compileIndex);
        } else if (isArrayPrediction(predictedType)) {
            SpeculateCellOperand cell(this, node.child1());
            GPRReg cellGPR = cell.gpr();
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(cellGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
            m_jit.storePtr(cellGPR, JITCompiler::payloadFor(node.local()));
            noResult(m_compileIndex);
        } else { // FIXME: Add BooleanPrediction handling
            JSValueOperand value(this, node.child1());
            m_jit.store32(value.payloadGPR(), JITCompiler::payloadFor(node.local()));
            m_jit.store32(value.tagGPR(), JITCompiler::tagFor(node.local()));
            noResult(m_compileIndex);
        }

        // Indicate that it's no longer necessary to retrieve the value of
        // this bytecode variable from registers or other locations in the register file.
        valueSourceReferenceForOperand(node.local()) = ValueSource::forPrediction(predictedType);
        break;
    }

    case SetArgument:
        // This is a no-op; it just marks the fact that the argument is being used.
        // But it may be profitable to use this as a hook to run speculation checks
        // on arguments, thereby allowing us to trivially eliminate such checks if
        // the argument is not used.
        break;

    case BitAnd:
    case BitOr:
    case BitXor:
        if (isInt32Constant(node.child1())) {
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this, op2);

            bitOp(op, valueOfInt32Constant(node.child1()), op2.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else if (isInt32Constant(node.child2())) {
            SpeculateIntegerOperand op1(this, node.child1());
            GPRTemporary result(this, op1);

            bitOp(op, valueOfInt32Constant(node.child2()), op1.gpr(), result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            SpeculateIntegerOperand op1(this, node.child1());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this, op1, op2);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            bitOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        }
        break;

    case BitRShift:
    case BitLShift:
    case BitURShift:
        if (isInt32Constant(node.child2())) {
            SpeculateIntegerOperand op1(this, node.child1());
            GPRTemporary result(this, op1);

            shiftOp(op, op1.gpr(), valueOfInt32Constant(node.child2()) & 0x1f, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        } else {
            // Do not allow shift amount to be used as the result, MacroAssembler does not permit this.
            SpeculateIntegerOperand op1(this, node.child1());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this, op1);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();
            shiftOp(op, reg1, reg2, result.gpr());

            integerResult(result.gpr(), m_compileIndex);
        }
        break;

    case UInt32ToNumber: {
        if (!nodeCanSpeculateInteger(node.arithNodeFlags())) {
            // We know that this sometimes produces doubles. So produce a double every
            // time. This at least allows subsequent code to not have weird conditionals.
            
            IntegerOperand op1(this, node.child1());
            FPRTemporary result(this);
            
            GPRReg inputGPR = op1.gpr();
            FPRReg outputFPR = result.fpr();
            
            m_jit.convertInt32ToDouble(inputGPR, outputFPR);
            
            JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, inputGPR, TrustedImm32(0));
            m_jit.addDouble(JITCompiler::AbsoluteAddress(&twoToThe32), outputFPR);
            positive.link(&m_jit);
            
            doubleResult(outputFPR, m_compileIndex);
            break;
        }

        IntegerOperand op1(this, node.child1());
        GPRTemporary result(this, op1);

        // Test the operand is positive.
        speculationCheck(m_jit.branch32(MacroAssembler::LessThan, op1.gpr(), TrustedImm32(0)));

        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueToInt32: {
        if (at(node.child1()).shouldNotSpeculateInteger()) {
            // Do it the safe way.
            nonSpeculativeValueToInt32(node);
            break;
        }
        
        SpeculateIntegerOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueToNumber: {
        if (at(node.child1()).shouldNotSpeculateInteger()) {
            SpeculateDoubleOperand op1(this, node.child1());
            FPRTemporary result(this, op1);
            m_jit.moveDouble(op1.fpr(), result.fpr());
            doubleResult(result.fpr(), m_compileIndex);
            break;
        }
        
        SpeculateIntegerOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex, op1.format());
        break;
    }

    case ValueToDouble: {
        SpeculateDoubleOperand op1(this, node.child1());
        FPRTemporary result(this, op1);
        m_jit.moveDouble(op1.fpr(), result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ValueAdd:
    case ArithAdd: {
        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())) && node.canSpeculateInteger()) {
            if (isInt32Constant(node.child1())) {
                int32_t imm1 = valueOfInt32Constant(node.child1());
                SpeculateIntegerOperand op2(this, node.child2());
                GPRTemporary result(this);

                if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                    m_jit.move(op2.gpr(), result.gpr());
                    m_jit.add32(Imm32(imm1), result.gpr());
                } else
                    speculationCheck(m_jit.branchAdd32(MacroAssembler::Overflow, op2.gpr(), Imm32(imm1), result.gpr()));

                integerResult(result.gpr(), m_compileIndex);
                break;
            }
                
            if (isInt32Constant(node.child2())) {
                SpeculateIntegerOperand op1(this, node.child1());
                int32_t imm2 = valueOfInt32Constant(node.child2());
                GPRTemporary result(this);
                
                if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                    m_jit.move(op1.gpr(), result.gpr());
                    m_jit.add32(Imm32(imm2), result.gpr());
                } else
                    speculationCheck(m_jit.branchAdd32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

                integerResult(result.gpr(), m_compileIndex);
                break;
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
                    speculationCheck(check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr2));
                else if (gpr2 == gprResult)
                    speculationCheck(check, SpeculationRecovery(SpeculativeAdd, gprResult, gpr1));
                else
                    speculationCheck(check);
            }

            integerResult(gprResult, m_compileIndex);
            break;
        }
        
        if (Node::shouldSpeculateNumber(at(node.child1()), at(node.child2()))) {
            SpeculateDoubleOperand op1(this, node.child1());
            SpeculateDoubleOperand op2(this, node.child2());
            FPRTemporary result(this, op1, op2);

            FPRReg reg1 = op1.fpr();
            FPRReg reg2 = op2.fpr();
            m_jit.addDouble(reg1, reg2, result.fpr());

            doubleResult(result.fpr(), m_compileIndex);
            break;
        }

        ASSERT(op == ValueAdd);
        compileValueAdd(node);
        break;
    }

    case ArithSub: {
        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())) && node.canSpeculateInteger()) {
            if (isInt32Constant(node.child2())) {
                SpeculateIntegerOperand op1(this, node.child1());
                int32_t imm2 = valueOfInt32Constant(node.child2());
                GPRTemporary result(this);

                if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                    m_jit.move(op1.gpr(), result.gpr());
                    m_jit.sub32(Imm32(imm2), result.gpr());
                } else
                    speculationCheck(m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), Imm32(imm2), result.gpr()));

                integerResult(result.gpr(), m_compileIndex);
                break;
            }
                
            SpeculateIntegerOperand op1(this, node.child1());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this);

            if (nodeCanTruncateInteger(node.arithNodeFlags())) {
                m_jit.move(op1.gpr(), result.gpr());
                m_jit.sub32(op2.gpr(), result.gpr());
            } else
                speculationCheck(m_jit.branchSub32(MacroAssembler::Overflow, op1.gpr(), op2.gpr(), result.gpr()));

            integerResult(result.gpr(), m_compileIndex);
            break;
        }
        
        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        FPRTemporary result(this, op1);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.subDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithMul: {
        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())) && node.canSpeculateInteger()) {
            SpeculateIntegerOperand op1(this, node.child1());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary result(this);

            GPRReg reg1 = op1.gpr();
            GPRReg reg2 = op2.gpr();

            // What is unfortunate is that we cannot take advantage of nodeCanTruncateInteger()
            // here. A multiply on integers performed in the double domain and then truncated to
            // an integer will give a different result than a multiply performed in the integer
            // domain and then truncated, if the integer domain result would have resulted in
            // something bigger than what a 32-bit integer can hold. JavaScript mandates that
            // the semantics are always as if the multiply had been performed in the double
            // domain.
            
            speculationCheck(m_jit.branchMul32(MacroAssembler::Overflow, reg1, reg2, result.gpr()));
            
            // Check for negative zero, if the users of this node care about such things.
            if (!nodeCanIgnoreNegativeZero(node.arithNodeFlags())) {
                MacroAssembler::Jump resultNonZero = m_jit.branchTest32(MacroAssembler::NonZero, result.gpr());
                speculationCheck(m_jit.branch32(MacroAssembler::LessThan, reg1, TrustedImm32(0)));
                speculationCheck(m_jit.branch32(MacroAssembler::LessThan, reg2, TrustedImm32(0)));
                resultNonZero.link(&m_jit);
            }

            integerResult(result.gpr(), m_compileIndex);
            break;
        }

        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        FPRTemporary result(this, op1, op2);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        
        m_jit.mulDouble(reg1, reg2, result.fpr());
        
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithDiv: {
        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())) && node.canSpeculateInteger()) {
            SpeculateIntegerOperand op1(this, node.child1());
            SpeculateIntegerOperand op2(this, node.child2());
            GPRTemporary eax(this, X86Registers::eax);
            GPRTemporary edx(this, X86Registers::edx);
            GPRReg op1GPR = op1.gpr();
            GPRReg op2GPR = op2.gpr();
            
            speculationCheck(m_jit.branchTest32(JITCompiler::Zero, op2GPR));
            
            // If the user cares about negative zero, then speculate that we're not about
            // to produce negative zero.
            if (!nodeCanIgnoreNegativeZero(node.arithNodeFlags())) {
                MacroAssembler::Jump numeratorNonZero = m_jit.branchTest32(MacroAssembler::NonZero, op1GPR);
                speculationCheck(m_jit.branch32(MacroAssembler::LessThan, op2GPR, TrustedImm32(0)));
                numeratorNonZero.link(&m_jit);
            }
            
            GPRReg temp2 = InvalidGPRReg;
            if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
                temp2 = allocate();
                m_jit.move(op2GPR, temp2);
                op2GPR = temp2;
            }
            
            m_jit.move(op1GPR, eax.gpr());
            m_jit.assembler().cdq();
            m_jit.assembler().idivl_r(op2GPR);
            
            if (temp2 != InvalidGPRReg)
                unlock(temp2);

            // Check that there was no remainder. If there had been, then we'd be obligated to
            // produce a double result instead.
            speculationCheck(m_jit.branchTest32(JITCompiler::NonZero, edx.gpr()));
            
            integerResult(eax.gpr(), m_compileIndex);
            break;
        }
        
        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        FPRTemporary result(this, op1);

        FPRReg reg1 = op1.fpr();
        FPRReg reg2 = op2.fpr();
        m_jit.divDouble(reg1, reg2, result.fpr());

        doubleResult(result.fpr(), m_compileIndex);
        break;
    }

    case ArithMod: {
        if (at(node.child1()).shouldNotSpeculateInteger() || at(node.child2()).shouldNotSpeculateInteger()
            || !node.canSpeculateInteger()) {
            SpeculateDoubleOperand op1(this, node.child1());
            SpeculateDoubleOperand op2(this, node.child2());
            
            FPRReg op1FPR = op1.fpr();
            FPRReg op2FPR = op2.fpr();
            
            flushRegisters();
            
            FPRResult result(this);

            callOperation(fmod, result.fpr(), op1FPR, op2FPR);
            
            doubleResult(result.fpr(), m_compileIndex);
            break;
        }
        
        SpeculateIntegerOperand op1(this, node.child1());
        SpeculateIntegerOperand op2(this, node.child2());
        GPRTemporary eax(this, X86Registers::eax);
        GPRTemporary edx(this, X86Registers::edx);
        GPRReg op1Gpr = op1.gpr();
        GPRReg op2Gpr = op2.gpr();

        speculationCheck(m_jit.branchTest32(JITCompiler::Zero, op2Gpr));

        GPRReg temp2 = InvalidGPRReg;
        if (op2Gpr == X86Registers::eax || op2Gpr == X86Registers::edx) {
            temp2 = allocate();
            m_jit.move(op2Gpr, temp2);
            op2Gpr = temp2;
        }

        m_jit.move(op1Gpr, eax.gpr());
        m_jit.assembler().cdq();
        m_jit.assembler().idivl_r(op2Gpr);

        if (temp2 != InvalidGPRReg)
            unlock(temp2);

        integerResult(edx.gpr(), m_compileIndex);
        break;
    }

    case ArithAbs: {
        if (at(node.child1()).shouldSpeculateInteger() && node.canSpeculateInteger()) {
            SpeculateIntegerOperand op1(this, node.child1());
            GPRTemporary result(this, op1);
            GPRTemporary scratch(this);
            
            m_jit.zeroExtend32ToPtr(op1.gpr(), result.gpr());
            m_jit.rshift32(result.gpr(), MacroAssembler::TrustedImm32(31), scratch.gpr());
            m_jit.add32(scratch.gpr(), result.gpr());
            m_jit.xor32(scratch.gpr(), result.gpr());
            speculationCheck(m_jit.branch32(MacroAssembler::Equal, result.gpr(), MacroAssembler::TrustedImm32(1 << 31)));
            integerResult(result.gpr(), m_compileIndex);
            break;
        }
        
        SpeculateDoubleOperand op1(this, node.child1());
        FPRTemporary result(this);
        
        static const double negativeZeroConstant = -0.0;
        
        m_jit.loadDouble(&negativeZeroConstant, result.fpr());
        m_jit.andnotDouble(op1.fpr(), result.fpr());
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }
        
    case ArithMin:
    case ArithMax: {
        if (Node::shouldSpeculateInteger(at(node.child1()), at(node.child2())) && node.canSpeculateInteger()) {
            SpeculateStrictInt32Operand op1(this, node.child1());
            SpeculateStrictInt32Operand op2(this, node.child2());
            GPRTemporary result(this, op1);
            
            MacroAssembler::Jump op1Less = m_jit.branch32(op == ArithMin ? MacroAssembler::LessThan : MacroAssembler::GreaterThan, op1.gpr(), op2.gpr());
            m_jit.move(op2.gpr(), result.gpr());
            if (op1.gpr() != result.gpr()) {
                MacroAssembler::Jump done = m_jit.jump();
                op1Less.link(&m_jit);
                m_jit.move(op1.gpr(), result.gpr());
                done.link(&m_jit);
            } else
                op1Less.link(&m_jit);
            
            integerResult(result.gpr(), m_compileIndex);
            break;
        }
        
        SpeculateDoubleOperand op1(this, node.child1());
        SpeculateDoubleOperand op2(this, node.child2());
        FPRTemporary result(this, op1);
        
        MacroAssembler::JumpList done;
        
        MacroAssembler::Jump op1Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleLessThan : MacroAssembler::DoubleGreaterThan, op1.fpr(), op2.fpr());
        
        // op2 is eather the lesser one or one of then is NaN
        MacroAssembler::Jump op2Less = m_jit.branchDouble(op == ArithMin ? MacroAssembler::DoubleGreaterThan : MacroAssembler::DoubleLessThan, op1.fpr(), op2.fpr());
        
        // Unordered case. We don't know which of op1, op2 is NaN. Manufacture NaN by adding 
        // op1 + op2 and putting it into result.
        m_jit.addDouble(op1.fpr(), op2.fpr(), result.fpr());
        done.append(m_jit.jump());
        
        op2Less.link(&m_jit);
        m_jit.moveDouble(op2.fpr(), result.fpr());
        
        if (op1.fpr() != result.fpr()) {
            done.append(m_jit.jump());
            
            op1Less.link(&m_jit);
            m_jit.moveDouble(op1.fpr(), result.fpr());
        } else
            op1Less.link(&m_jit);
        
        done.link(&m_jit);
        
        doubleResult(result.fpr(), m_compileIndex);
        break;
    }
        
    case ArithSqrt: {
        SpeculateDoubleOperand op1(this, node.child1());
        FPRTemporary result(this, op1);
        
        m_jit.sqrtDouble(op1.fpr(), result.fpr());
        
        doubleResult(result.fpr(), m_compileIndex);
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

    case CompareEq:
        if (isNullConstant(node.child1())) {
            if (nonSpeculativeCompareNull(node, node.child2()))
                return;
            break;
        }
        if (isNullConstant(node.child2())) {
            if (nonSpeculativeCompareNull(node, node.child1()))
                return;
            break;
        }
        if (compare(node, JITCompiler::Equal, JITCompiler::DoubleEqual, operationCompareEq))
            return;
        break;

    case CompareStrictEq:
        if (nonSpeculativeStrictEq(node))
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

    case GetByVal: {
        if (at(node.child1()).prediction() == PredictString) {
            compileGetByValOnString(node);
            if (!m_compileOkay)
                return;
            break;
        }

        ASSERT(node.child3() == NoNode);
        SpeculateCellOperand base(this, node.child1());
        SpeculateStrictInt32Operand property(this, node.child2());
        GPRTemporary storage(this);
        GPRTemporary resultTag(this, base);

        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();
        GPRReg storageReg = storage.gpr();
        
        if (!m_compileOkay)
            return;

        // Get the array storage. We haven't yet checked this is a JSArray, so this is only safe if
        // an access with offset JSArray::storageOffset() is valid for all JSCells!
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSArray::storageOffset()), storageReg);

        // Check that base is an array, and that property is contained within m_vector (< m_vectorLength).
        // If we have predicted the base to be type array, we can skip the check.
        if (!isKnownArray(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseReg), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        speculationCheck(m_jit.branch32(MacroAssembler::AboveOrEqual, propertyReg, MacroAssembler::Address(baseReg, JSArray::vectorLengthOffset())));

        // FIXME: In cases where there are subsequent by_val accesses to the same base it might help to cache
        // the storage pointer - especially if there happens to be another register free right now. If we do so,
        // then we'll need to allocate a new temporary for result.
        m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag.gpr());
        speculationCheck(m_jit.branch32(MacroAssembler::Equal, resultTag.gpr(), TrustedImm32(JSValue::EmptyValueTag)));
        m_jit.load32(MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), storageReg);

        jsValueResult(resultTag.gpr(), storageReg, m_compileIndex);
        break;
    }

    case PutByVal: {
        SpeculateCellOperand base(this, node.child1());
        SpeculateStrictInt32Operand property(this, node.child2());
        JSValueOperand value(this, node.child3());
        GPRTemporary scratch(this);

        // Map base, property & value into registers, allocate a scratch register.
        GPRReg baseReg = base.gpr();
        GPRReg propertyReg = property.gpr();
        GPRReg valueTagReg = value.tagGPR();
        GPRReg valuePayloadReg = value.payloadGPR();
        GPRReg scratchReg = scratch.gpr();
        
        if (!m_compileOkay)
            return;
        
        writeBarrier(baseReg, valueTagReg, node.child3(), WriteBarrierForPropertyAccess, scratchReg);

        // Check that base is an array, and that property is contained within m_vector (< m_vectorLength).
        // If we have predicted the base to be type array, we can skip the check.
        if (!isKnownArray(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseReg), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));

        base.use();
        property.use();
        value.use();
        
        MacroAssembler::Jump withinArrayBounds = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(baseReg, JSArray::vectorLengthOffset()));

        // Code to handle put beyond array bounds.
        silentSpillAllRegisters(scratchReg);
        m_jit.push(valueTagReg);
        m_jit.push(valuePayloadReg);
        m_jit.push(propertyReg);
        m_jit.push(baseReg);
        m_jit.push(GPRInfo::callFrameRegister);
        JITCompiler::Call functionCall = appendCallWithExceptionCheck(operationPutByValBeyondArrayBounds);
        silentFillAllRegisters(scratchReg);
        JITCompiler::Jump wasBeyondArrayBounds = m_jit.jump();

        withinArrayBounds.link(&m_jit);

        // Get the array storage.
        GPRReg storageReg = scratchReg;
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSArray::storageOffset()), storageReg);

        // Check if we're writing to a hole; if so increment m_numValuesInVector.
        MacroAssembler::Jump notHoleValue = m_jit.branch32(MacroAssembler::NotEqual, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
        m_jit.add32(TrustedImm32(1), MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));

        // If we're writing to a hole we might be growing the array; 
        MacroAssembler::Jump lengthDoesNotNeedUpdate = m_jit.branch32(MacroAssembler::Below, propertyReg, MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_length)));
        m_jit.add32(TrustedImm32(1), propertyReg);
        m_jit.store32(propertyReg, MacroAssembler::Address(storageReg, OBJECT_OFFSETOF(ArrayStorage, m_length)));
        m_jit.sub32(TrustedImm32(1), propertyReg);

        lengthDoesNotNeedUpdate.link(&m_jit);
        notHoleValue.link(&m_jit);

        // Store the value to the array.
        m_jit.store32(valueTagReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        m_jit.store32(valuePayloadReg, MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));

        wasBeyondArrayBounds.link(&m_jit);

        noResult(m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case PutByValAlias: {
        SpeculateCellOperand base(this, node.child1());
        SpeculateStrictInt32Operand property(this, node.child2());
        JSValueOperand value(this, node.child3());
        GPRTemporary scratch(this, base);
        
        GPRReg baseReg = base.gpr();
        GPRReg scratchReg = scratch.gpr();

        writeBarrier(baseReg, value.tagGPR(), node.child3(), WriteBarrierForPropertyAccess, scratchReg);

        // Get the array storage.
        GPRReg storageReg = scratchReg;
        m_jit.loadPtr(MacroAssembler::Address(baseReg, JSArray::storageOffset()), storageReg);

        // Store the value to the array.
        GPRReg propertyReg = property.gpr();
        m_jit.store32(value.tagGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        m_jit.store32(value.payloadGPR(), MacroAssembler::BaseIndex(storageReg, propertyReg, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));

        noResult(m_compileIndex);
        break;
    }

    case ArrayPush: {
        SpeculateCellOperand base(this, node.child1());
        JSValueOperand value(this, node.child2());
        GPRTemporary storage(this);
        GPRTemporary storageLength(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg storageGPR = storage.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        
        writeBarrier(baseGPR, valueTagGPR, node.child2(), WriteBarrierForPropertyAccess, storageGPR, storageLengthGPR);

        if (!isKnownArray(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        
        m_jit.loadPtr(MacroAssembler::Address(baseGPR, JSArray::storageOffset()), storageGPR);
        m_jit.load32(MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)), storageLengthGPR);
        
        // Refuse to handle bizarre lengths.
        speculationCheck(m_jit.branch32(MacroAssembler::Above, storageLengthGPR, TrustedImm32(0x7ffffffe)));
        
        MacroAssembler::Jump slowPath = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(baseGPR, JSArray::vectorLengthOffset()));
        
        m_jit.store32(valueTagGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        m_jit.store32(valuePayloadGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        
        m_jit.add32(Imm32(1), storageLengthGPR);
        m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)));
        m_jit.add32(Imm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        m_jit.move(Imm32(JSValue::Int32Tag), storageGPR);
        
        MacroAssembler::Jump done = m_jit.jump();
        
        slowPath.link(&m_jit);
        
        silentSpillAllRegisters(storageGPR, storageLengthGPR);
        callOperation(operationArrayPush, storageGPR, storageLengthGPR, valueTagGPR, valuePayloadGPR, baseGPR);
        silentFillAllRegisters(storageGPR, storageLengthGPR);
        
        done.link(&m_jit);
        
        jsValueResult(storageGPR, storageLengthGPR, m_compileIndex);
        break;
    }
        
    case ArrayPop: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary valueTag(this);
        GPRTemporary valuePayload(this);
        GPRTemporary storage(this);
        GPRTemporary storageLength(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = valueTag.gpr();
        GPRReg valuePayloadGPR = valuePayload.gpr();
        GPRReg storageGPR = storage.gpr();
        GPRReg storageLengthGPR = storageLength.gpr();
        
        if (!isKnownArray(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        
        m_jit.loadPtr(MacroAssembler::Address(baseGPR, JSArray::storageOffset()), storageGPR);
        m_jit.load32(MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)), storageLengthGPR);
        
        MacroAssembler::Jump emptyArrayCase = m_jit.branchTest32(MacroAssembler::Zero, storageLengthGPR);
        
        m_jit.sub32(Imm32(1), storageLengthGPR);
        
        MacroAssembler::Jump slowCase = m_jit.branch32(MacroAssembler::AboveOrEqual, storageLengthGPR, MacroAssembler::Address(baseGPR, JSArray::vectorLengthOffset()));
        
        m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), valueTagGPR);
        m_jit.load32(MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), valuePayloadGPR);
        
        m_jit.store32(storageLengthGPR, MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)));

        MacroAssembler::Jump holeCase = m_jit.branch32(MacroAssembler::Equal, Imm32(JSValue::EmptyValueTag), valueTagGPR);
        
        m_jit.move(Imm32(JSValue::EmptyValueTag), storageLengthGPR);
        m_jit.store32(storageLengthGPR, MacroAssembler::BaseIndex(storageGPR, storageLengthGPR, MacroAssembler::TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)));

        m_jit.sub32(MacroAssembler::Imm32(1), MacroAssembler::Address(storageGPR, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
        
        MacroAssembler::JumpList done;
        
        done.append(m_jit.jump());
        
        holeCase.link(&m_jit);
        emptyArrayCase.link(&m_jit);
        m_jit.move(MacroAssembler::Imm32(jsUndefined().tag()), valueTagGPR);
        m_jit.move(MacroAssembler::Imm32(jsUndefined().payload()), valuePayloadGPR);
        done.append(m_jit.jump());
        
        slowCase.link(&m_jit);
        
        silentSpillAllRegisters(valueTagGPR, valuePayloadGPR);
        callOperation(operationArrayPop, valueTagGPR, valuePayloadGPR, baseGPR);
        silentFillAllRegisters(valueTagGPR, valuePayloadGPR);
        
        done.link(&m_jit);
        
        jsValueResult(valueTagGPR, valuePayloadGPR, m_compileIndex);
        break;
    }

    case DFG::Jump: {
        BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
        if (taken != (m_block + 1))
            addBranch(m_jit.jump(), taken);
        noResult(m_compileIndex);
        break;
    }

    case Branch:
        if (isStrictInt32(node.child1()) || at(node.child1()).shouldSpeculateInteger()) {
            SpeculateIntegerOperand op(this, node.child1());
            
            BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(node.takenBytecodeOffset());
            BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(node.notTakenBytecodeOffset());
            
            MacroAssembler::ResultCondition condition = MacroAssembler::NonZero;
            
            if (taken == (m_block + 1)) {
                condition = MacroAssembler::Zero;
                BlockIndex tmp = taken;
                taken = notTaken;
                notTaken = tmp;
            }
            
            addBranch(m_jit.branchTest32(condition, op.gpr()), taken);
            if (notTaken != (m_block + 1))
                addBranch(m_jit.jump(), notTaken);
            
            noResult(m_compileIndex);
            break;
        }
        emitBranch(node);
        break;

    case Return: {
        ASSERT(GPRInfo::callFrameRegister != GPRInfo::regT2);
        ASSERT(GPRInfo::regT1 != GPRInfo::returnValueGPR);
        ASSERT(GPRInfo::returnValueGPR != GPRInfo::callFrameRegister);

#if ENABLE(DFG_SUCCESS_STATS)
        static SamplingCounter counter("SpeculativeJIT");
        m_jit.emitCount(counter);
#endif

        // Return the result in returnValueGPR.
        JSValueOperand op1(this, node.child1());
        op1.fill();
        if (op1.isDouble())
            boxDouble(op1.fpr(), GPRInfo::returnValueGPR2, GPRInfo::returnValueGPR, at(op1.index()).virtualRegister());
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

        // Grab the return address.
        m_jit.emitGetFromCallFrameHeaderPtr(RegisterFile::ReturnPC, GPRInfo::regT2);
        // Restore our caller's "r".
        m_jit.emitGetFromCallFrameHeaderPtr(RegisterFile::CallerFrame, GPRInfo::callFrameRegister);
        // Return.
        m_jit.restoreReturnAddressBeforeReturn(GPRInfo::regT2);
        m_jit.ret();
        
        noResult(m_compileIndex);
        break;
    }
        
    case Throw:
    case ThrowReferenceError: {
        // We expect that throw statements are rare and are intended to exit the code block
        // anyway, so we just OSR back to the old JIT for now.
        terminateSpeculativeExecution();
        break;
    }
        
    case ToPrimitive: {
        if (at(node.child1()).shouldSpeculateInteger()) {
            // It's really profitable to speculate integer, since it's really cheap,
            // it means we don't have to do any real work, and we emit a lot less code.
            
            SpeculateIntegerOperand op1(this, node.child1());
            GPRTemporary result(this, op1);
            
            ASSERT(op1.format() == DataFormatInteger);
            m_jit.move(op1.gpr(), result.gpr());
            
            integerResult(result.gpr(), m_compileIndex);
            break;
        }
        
        // FIXME: Add string speculation here.
        
        bool wasPrimitive = isKnownNumeric(node.child1()) || isKnownBoolean(node.child1());
        
        JSValueOperand op1(this, node.child1());
        GPRTemporary resultTag(this, op1);
        GPRTemporary resultPayload(this, op1, false);
        
        GPRReg op1TagGPR = op1.tagGPR();
        GPRReg op1PayloadGPR = op1.payloadGPR();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        op1.use();
        
        if (wasPrimitive) {
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
        } else {
            MacroAssembler::JumpList alreadyPrimitive;
            
            alreadyPrimitive.append(m_jit.branch32(MacroAssembler::NotEqual, op1TagGPR, TrustedImm32(JSValue::CellTag)));
            alreadyPrimitive.append(m_jit.branchPtr(MacroAssembler::Equal, MacroAssembler::Address(op1PayloadGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsStringVPtr)));
            
            silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
            callOperation(operationToPrimitive, resultTagGPR, resultPayloadGPR, op1TagGPR, op1PayloadGPR);
            silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
            
            MacroAssembler::Jump done = m_jit.jump();
            
            alreadyPrimitive.link(&m_jit);
            m_jit.move(op1TagGPR, resultTagGPR);
            m_jit.move(op1PayloadGPR, resultPayloadGPR);
            
            done.link(&m_jit);
        }
        
        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }
        
    case StrCat:
    case NewArray: {
        // We really don't want to grow the register file just to do a StrCat or NewArray.
        // Say we have 50 functions on the stack that all have a StrCat in them that has
        // upwards of 10 operands. In the DFG this would mean that each one gets
        // some random virtual register, and then to do the StrCat we'd need a second
        // span of 10 operands just to have somewhere to copy the 10 operands to, where
        // they'd be contiguous and we could easily tell the C code how to find them.
        // Ugly! So instead we use the scratchBuffer infrastructure in JSGlobalData. That
        // way, those 50 functions will share the same scratchBuffer for offloading their
        // StrCat operands. It's about as good as we can do, unless we start doing
        // virtual register coalescing to ensure that operands to StrCat get spilled
        // in exactly the place where StrCat wants them, or else have the StrCat
        // refer to those operands' SetLocal instructions to force them to spill in
        // the right place. Basically, any way you cut it, the current approach
        // probably has the best balance of performance and sensibility in the sense
        // that it does not increase the complexity of the DFG JIT just to make StrCat
        // fast and pretty.
        
        EncodedJSValue* buffer = static_cast<EncodedJSValue*>(m_jit.globalData()->scratchBufferForSize(sizeof(EncodedJSValue) * node.numChildren()));
        
        for (unsigned operandIdx = 0; operandIdx < node.numChildren(); ++operandIdx) {
            JSValueOperand operand(this, m_jit.graph().m_varArgChildren[node.firstChild() + operandIdx]);
            GPRReg opTagGPR = operand.tagGPR();
            GPRReg opPayloadGPR = operand.payloadGPR();
            operand.use();
            
            m_jit.store32(opTagGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
            m_jit.store32(opPayloadGPR, reinterpret_cast<char*>(buffer + operandIdx) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
        }
        
        flushRegisters();
        
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        
        callOperation(op == StrCat ? operationStrCat : operationNewArray, resultTag.gpr(), resultPayload.gpr(), buffer, node.numChildren());

        // FIXME: make the callOperation above explicitly return a cell result, or jitAssert the tag is a cell tag.
        cellResult(resultPayload.gpr(), m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case NewArrayBuffer: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        
        callOperation(operationNewArrayBuffer, resultTag.gpr(), resultPayload.gpr(), node.startConstant(), node.numConstants());
        
        // FIXME: make the callOperation above explicitly return a cell result, or jitAssert the tag is a cell tag.
        cellResult(resultPayload.gpr(), m_compileIndex);
        break;
    }
        
    case NewRegexp: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        
        callOperation(operationNewRegexp, resultTag.gpr(), resultPayload.gpr(), m_jit.codeBlock()->regexp(node.regexpIndex()));
        
        // FIXME: make the callOperation above explicitly return a cell result, or jitAssert the tag is a cell tag.
        cellResult(resultPayload.gpr(), m_compileIndex);
        break;
    }
        
    case ConvertThis: {
        if (isOtherPrediction(at(node.child1()).prediction())) {
            JSValueOperand thisValue(this, node.child1());
            GPRTemporary scratch(this, thisValue);
            
            GPRReg thisValueTagGPR = thisValue.tagGPR();
            GPRReg scratchGPR = scratch.gpr();
            
            m_jit.move(thisValueTagGPR, scratchGPR);
            m_jit.and32(MacroAssembler::TrustedImm32(JSValue::UndefinedTag), scratchGPR);
            speculationCheck(m_jit.branch32(MacroAssembler::NotEqual, scratchGPR, TrustedImm32(JSValue::UndefinedTag)));
            
            m_jit.move(MacroAssembler::TrustedImmPtr(m_jit.codeBlock()->globalObject()), scratchGPR);
            cellResult(scratchGPR, m_compileIndex);
            break;
        }
        
        if (isObjectPrediction(at(node.child1()).prediction())) {
            SpeculateCellOperand thisValue(this, node.child1());
            
            speculationCheck(m_jit.branchPtr(JITCompiler::Equal, JITCompiler::Address(thisValue.gpr()), JITCompiler::TrustedImmPtr(m_jit.globalData()->jsStringVPtr)));
            
            cellResult(thisValue.gpr(), m_compileIndex);
            break;
        }
        
        JSValueOperand thisValue(this, node.child1());
        GPRReg thisValueTagGPR = thisValue.tagGPR();
        GPRReg thisValuePayloadGPR = thisValue.payloadGPR();
        
        flushRegisters();
        
        GPRResult2 resultTag(this);
        GPRResult resultPayload(this);
        callOperation(operationConvertThis, resultTag.gpr(), resultPayload.gpr(), thisValueTagGPR, thisValuePayloadGPR);
        
        cellResult(resultPayload.gpr(), m_compileIndex);
        break;
    }

    case CreateThis: {
        // Note that there is not so much profit to speculate here. The only things we
        // speculate on are (1) that it's a cell, since that eliminates cell checks
        // later if the proto is reused, and (2) if we have a FinalObject prediction
        // then we speculate because we want to get recompiled if it isn't (since
        // otherwise we'd start taking slow path a lot).
        
        SpeculateCellOperand proto(this, node.child1());
        GPRTemporary result(this);
        GPRTemporary scratch(this);
        
        GPRReg protoGPR = proto.gpr();
        GPRReg resultGPR = result.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        proto.use();
        
        MacroAssembler::JumpList slowPath;
        
        // Need to verify that the prototype is an object. If we have reason to believe
        // that it's a FinalObject then we speculate on that directly. Otherwise we
        // do the slow (structure-based) check.
        if (at(node.child1()).shouldSpeculateFinalObject())
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(protoGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsFinalObjectVPtr)));
        else {
            m_jit.loadPtr(MacroAssembler::Address(protoGPR, JSCell::structureOffset()), scratchGPR);
            slowPath.append(m_jit.branch8(MacroAssembler::Below, MacroAssembler::Address(scratchGPR, Structure::typeInfoTypeOffset()), MacroAssembler::TrustedImm32(ObjectType)));
        }
        
        // Load the inheritorID (the Structure that objects who have protoGPR as the prototype
        // use to refer to that prototype). If the inheritorID is not set, go to slow path.
        m_jit.loadPtr(MacroAssembler::Address(protoGPR, JSObject::offsetOfInheritorID()), scratchGPR);
        slowPath.append(m_jit.branchTestPtr(MacroAssembler::Zero, scratchGPR));
        
        emitAllocateJSFinalObject(scratchGPR, resultGPR, scratchGPR, slowPath);
        
        MacroAssembler::Jump done = m_jit.jump();
        
        slowPath.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationCreateThis, resultGPR, protoGPR);
        silentFillAllRegisters(resultGPR);
        
        done.link(&m_jit);
        
        cellResult(resultGPR, m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case NewObject: {
        GPRTemporary result(this);
        GPRTemporary scratch(this);
        
        GPRReg resultGPR = result.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        MacroAssembler::JumpList slowPath;
        
        emitAllocateJSFinalObject(MacroAssembler::TrustedImmPtr(m_jit.codeBlock()->globalObject()->emptyObjectStructure()), resultGPR, scratchGPR, slowPath);
        
        MacroAssembler::Jump done = m_jit.jump();
        
        slowPath.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        callOperation(operationNewObject, resultGPR);
        silentFillAllRegisters(resultGPR);
        
        done.link(&m_jit);
        
        cellResult(resultGPR, m_compileIndex);
        break;
    }

    case GetCallee: {
        GPRTemporary result(this);
        m_jit.loadPtr(JITCompiler::addressFor(static_cast<VirtualRegister>(RegisterFile::Callee)), result.gpr());
        cellResult(result.gpr(), m_compileIndex);
        break;
    }

    case GetScopeChain: {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();

        m_jit.loadPtr(JITCompiler::addressFor(static_cast<VirtualRegister>(RegisterFile::ScopeChain)), resultGPR);
        bool checkTopLevel = m_jit.codeBlock()->codeType() == FunctionCode && m_jit.codeBlock()->needsFullScopeChain();
        int skip = node.scopeChainDepth();
        ASSERT(skip || !checkTopLevel);
        if (checkTopLevel && skip--) {
            JITCompiler::Jump activationNotCreated;
            if (checkTopLevel)
                activationNotCreated = m_jit.branchTestPtr(JITCompiler::Zero, JITCompiler::addressFor(static_cast<VirtualRegister>(m_jit.codeBlock()->activationRegister())));
            m_jit.loadPtr(JITCompiler::Address(resultGPR, OBJECT_OFFSETOF(ScopeChainNode, next)), resultGPR);
            activationNotCreated.link(&m_jit);
        }
        while (skip--)
            m_jit.loadPtr(JITCompiler::Address(resultGPR, OBJECT_OFFSETOF(ScopeChainNode, next)), resultGPR);
        
        m_jit.loadPtr(JITCompiler::Address(resultGPR, OBJECT_OFFSETOF(ScopeChainNode, object)), resultGPR);

        cellResult(resultGPR, m_compileIndex);
        break;
    }
    case GetScopedVar: {
        SpeculateCellOperand scopeChain(this, node.child1());
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        m_jit.loadPtr(JITCompiler::Address(scopeChain.gpr(), JSVariableObject::offsetOfRegisters()), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(resultPayloadGPR, node.varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        m_jit.load32(JITCompiler::Address(resultPayloadGPR, node.varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex);
        break;
    }
    case PutScopedVar: {
        SpeculateCellOperand scopeChain(this, node.child1());
        GPRTemporary scratchRegister(this);
        GPRReg scratchGPR = scratchRegister.gpr();
        m_jit.loadPtr(JITCompiler::Address(scopeChain.gpr(), JSVariableObject::offsetOfRegisters()), scratchGPR);
        JSValueOperand value(this, node.child2());
        m_jit.store32(value.tagGPR(), JITCompiler::Address(scratchGPR, node.varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.store32(value.payloadGPR(), JITCompiler::Address(scratchGPR, node.varNumber() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        writeBarrier(scopeChain.gpr(), value.tagGPR(), node.child2(), WriteBarrierForVariableAccess, scratchGPR);
        noResult(m_compileIndex);
        break;
    }
        
    case GetById: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary resultTag(this, base);
        GPRTemporary resultPayload(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg scratchGPR;
        
        if (resultTagGPR == baseGPR)
            scratchGPR = resultPayloadGPR;
        else
            scratchGPR = resultTagGPR;
        
        base.use();

        cachedGetById(baseGPR, resultTagGPR, resultPayloadGPR, scratchGPR, node.identifierNumber());

        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case GetArrayLength: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary result(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        
        if (!isKnownArray(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsArrayVPtr)));
        
        m_jit.loadPtr(MacroAssembler::Address(baseGPR, JSArray::storageOffset()), resultGPR);
        m_jit.load32(MacroAssembler::Address(resultGPR, OBJECT_OFFSETOF(ArrayStorage, m_length)), resultGPR);
        
        speculationCheck(m_jit.branch32(MacroAssembler::LessThan, resultGPR, MacroAssembler::TrustedImm32(0)));
        
        integerResult(resultGPR, m_compileIndex);
        break;
    }

    case GetStringLength: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary result(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        
        if (!isKnownString(node.child1()))
            speculationCheck(m_jit.branchPtr(MacroAssembler::NotEqual, MacroAssembler::Address(baseGPR), MacroAssembler::TrustedImmPtr(m_jit.globalData()->jsStringVPtr)));
        
        m_jit.load32(MacroAssembler::Address(baseGPR, JSString::offsetOfLength()), resultGPR);

        integerResult(resultGPR, m_compileIndex);
        break;
    }

    case CheckFunction: {
        SpeculateCellOperand function(this, node.child1());
        speculationCheck(m_jit.branchPtr(JITCompiler::NotEqual, function.gpr(), JITCompiler::TrustedImmPtr(node.function())));
        noResult(m_compileIndex);
        break;
    }

    case CheckStructure: {
        SpeculateCellOperand base(this, node.child1());
        
        ASSERT(node.structureSet().size());
        
        if (node.structureSet().size() == 1)
            speculationCheck(m_jit.branchPtr(JITCompiler::NotEqual, JITCompiler::Address(base.gpr(), JSCell::structureOffset()), JITCompiler::TrustedImmPtr(node.structureSet()[0])));
        else {
            GPRTemporary structure(this);
            
            m_jit.loadPtr(JITCompiler::Address(base.gpr(), JSCell::structureOffset()), structure.gpr());
            
            JITCompiler::JumpList done;
            
            for (size_t i = 0; i < node.structureSet().size() - 1; ++i)
                done.append(m_jit.branchPtr(JITCompiler::Equal, structure.gpr(), JITCompiler::TrustedImmPtr(node.structureSet()[i])));
            
            speculationCheck(m_jit.branchPtr(JITCompiler::NotEqual, structure.gpr(), JITCompiler::TrustedImmPtr(node.structureSet().last())));
            
            done.link(&m_jit);
        }
        
        noResult(m_compileIndex);
        break;
    }
        
    case PutStructure: {
        SpeculateCellOperand base(this, node.child1());
        GPRReg baseGPR = base.gpr();
        
#if ENABLE(GGC) || ENABLE(WRITE_BARRIER_PROFILING)
        // Must always emit this write barrier as the structure transition itself requires it
        writeBarrier(baseGPR, node.structureTransitionData().newStructure, WriteBarrierForGenericAccess);
#endif
        
        m_jit.storePtr(MacroAssembler::TrustedImmPtr(node.structureTransitionData().newStructure), MacroAssembler::Address(baseGPR, JSCell::structureOffset()));
        
        noResult(m_compileIndex);
        break;
    }
        
    case GetPropertyStorage: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary result(this, base);
        
        GPRReg baseGPR = base.gpr();
        GPRReg resultGPR = result.gpr();
        
        m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), resultGPR);
        
        storageResult(resultGPR, m_compileIndex);
        break;
    }
        
    case GetByOffset: {
        StorageOperand storage(this, node.child1());
        GPRTemporary resultTag(this, storage);
        GPRTemporary resultPayload(this);
        
        GPRReg storageGPR = storage.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        
        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node.storageAccessDataIndex()];
        
        m_jit.load32(JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        
        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex);
        break;
    }
        
    case PutByOffset: {
#if ENABLE(GGC) || ENABLE(WRITE_BARRIER_PROFILING)
        SpeculateCellOperand base(this, node.child1());
#endif
        StorageOperand storage(this, node.child2());
        JSValueOperand value(this, node.child3());

        GPRReg storageGPR = storage.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        
#if ENABLE(GGC) || ENABLE(WRITE_BARRIER_PROFILING)
        writeBarrier(base.gpr(), valueTagGPR, node.child3(), WriteBarrierForPropertyAccess);
#endif

        StorageAccessData& storageAccessData = m_jit.graph().m_storageAccessData[node.storageAccessDataIndex()];
        
        m_jit.storePtr(valueTagGPR, JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
        m_jit.storePtr(valuePayloadGPR, JITCompiler::Address(storageGPR, storageAccessData.offset * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
        
        noResult(m_compileIndex);
        break;
    }
        
    case GetMethod: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary resultTag(this, base);
        GPRTemporary resultPayload(this);

        GPRReg baseGPR = base.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();
        GPRReg scratchGPR;
        
        if (resultTagGPR == baseGPR)
            scratchGPR = resultPayloadGPR;
        else
            scratchGPR = resultTagGPR;
        
        base.use();

        cachedGetMethod(baseGPR, resultTagGPR, resultPayloadGPR, scratchGPR, node.identifierNumber());

        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case CheckMethod: {
        MethodCheckData& methodCheckData = m_jit.graph().m_methodCheckData[node.methodCheckDataIndex()];
        
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary scratch(this); // this needs to be a separate register, unfortunately.
        GPRReg baseGPR = base.gpr();
        GPRReg scratchGPR = scratch.gpr();
        
        speculationCheck(m_jit.branchPtr(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), JITCompiler::TrustedImmPtr(methodCheckData.structure)));
        if (methodCheckData.prototype != m_jit.codeBlock()->globalObject()->methodCallDummy()) {
            m_jit.move(JITCompiler::TrustedImmPtr(methodCheckData.prototype->structureAddress()), scratchGPR);
            speculationCheck(m_jit.branchPtr(JITCompiler::NotEqual, JITCompiler::Address(scratchGPR), JITCompiler::TrustedImmPtr(methodCheckData.prototypeStructure)));
        }
        
        useChildren(node);
        initConstantInfo(m_compileIndex);
        break;
    }

    case PutById: {
        SpeculateCellOperand base(this, node.child1());
        JSValueOperand value(this, node.child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(baseGPR, valueTagGPR, valuePayloadGPR, node.child2(), scratchGPR, node.identifierNumber(), NotDirect);
        
        noResult(m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case PutByIdDirect: {
        SpeculateCellOperand base(this, node.child1());
        JSValueOperand value(this, node.child2());
        GPRTemporary scratch(this);
        
        GPRReg baseGPR = base.gpr();
        GPRReg valueTagGPR = value.tagGPR();
        GPRReg valuePayloadGPR = value.payloadGPR();
        GPRReg scratchGPR = scratch.gpr();
        
        base.use();
        value.use();

        cachedPutById(baseGPR, valueTagGPR, valuePayloadGPR, node.child2(), scratchGPR, node.identifierNumber(), Direct);

        noResult(m_compileIndex, UseChildrenCalledExplicitly);
        break;
    }

    case GetGlobalVar: {
        GPRTemporary result(this);
        GPRTemporary scratch(this);

        JSVariableObject* globalObject = m_jit.codeBlock()->globalObject();
        m_jit.loadPtr(const_cast<WriteBarrier<Unknown>**>(globalObject->addressOfRegisters()), result.gpr());
        m_jit.load32(JITCompiler::tagForGlobalVar(result.gpr(), node.varNumber()), scratch.gpr());
        m_jit.load32(JITCompiler::payloadForGlobalVar(result.gpr(), node.varNumber()), result.gpr());

        jsValueResult(scratch.gpr(), result.gpr(), m_compileIndex);
        break;
    }

    case PutGlobalVar: {
        JSValueOperand value(this, node.child1());
        GPRTemporary globalObject(this);
        GPRTemporary scratch(this);
        
        GPRReg globalObjectReg = globalObject.gpr();
        GPRReg scratchReg = scratch.gpr();

        m_jit.move(MacroAssembler::TrustedImmPtr(m_jit.codeBlock()->globalObject()), globalObjectReg);

        writeBarrier(m_jit.codeBlock()->globalObject(), value.tagGPR(), node.child1(), WriteBarrierForVariableAccess, scratchReg);

        m_jit.loadPtr(MacroAssembler::Address(globalObjectReg, JSVariableObject::offsetOfRegisters()), scratchReg);
        m_jit.store32(value.tagGPR(), JITCompiler::tagForGlobalVar(scratchReg, node.varNumber()));
        m_jit.store32(value.payloadGPR(), JITCompiler::payloadForGlobalVar(scratchReg, node.varNumber()));

        noResult(m_compileIndex);
        break;
    }

    case CheckHasInstance: {
        SpeculateCellOperand base(this, node.child1());
        GPRTemporary structure(this);

        // Speculate that base 'ImplementsDefaultHasInstance'.
        m_jit.loadPtr(MacroAssembler::Address(base.gpr(), JSCell::structureOffset()), structure.gpr());
        speculationCheck(m_jit.branchTest8(MacroAssembler::Zero, MacroAssembler::Address(structure.gpr(), Structure::typeInfoFlagsOffset()), MacroAssembler::TrustedImm32(ImplementsDefaultHasInstance)));

        noResult(m_compileIndex);
        break;
    }

    case InstanceOf: {
        SpeculateCellOperand value(this, node.child1());
        // Base unused since we speculate default InstanceOf behaviour in CheckHasInstance.
        SpeculateCellOperand prototype(this, node.child3());

        GPRTemporary scratch(this);
        GPRTemporary booleanTag(this, value);

        GPRReg valueReg = value.gpr();
        GPRReg prototypeReg = prototype.gpr();
        GPRReg scratchReg = scratch.gpr();

        // Check that prototype is an object.
        m_jit.loadPtr(MacroAssembler::Address(prototypeReg, JSCell::structureOffset()), scratchReg);
        speculationCheck(m_jit.branch8(MacroAssembler::NotEqual, MacroAssembler::Address(scratchReg, Structure::typeInfoTypeOffset()), MacroAssembler::TrustedImm32(ObjectType)));

        // Initialize scratchReg with the value being checked.
        m_jit.move(valueReg, scratchReg);

        // Walk up the prototype chain of the value (in scratchReg), comparing to prototypeReg.
        MacroAssembler::Label loop(&m_jit);
        m_jit.loadPtr(MacroAssembler::Address(scratchReg, JSCell::structureOffset()), scratchReg);
        m_jit.load32(MacroAssembler::Address(scratchReg, Structure::prototypeOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), scratchReg);
        MacroAssembler::Jump isInstance = m_jit.branch32(MacroAssembler::Equal, scratchReg, prototypeReg);
        m_jit.branchTest32(MacroAssembler::NonZero, scratchReg).linkTo(loop, &m_jit);

        // No match - result is false.
        m_jit.move(MacroAssembler::TrustedImm32(0), scratchReg);
        MacroAssembler::Jump putResult = m_jit.jump();

        isInstance.link(&m_jit);
        m_jit.move(MacroAssembler::TrustedImm32(1), scratchReg);

        putResult.link(&m_jit);
        m_jit.move(TrustedImm32(JSValue::BooleanTag), booleanTag.gpr());
        jsValueResult(booleanTag.gpr(), scratchReg, m_compileIndex, DataFormatJSBoolean);
        break;
    }

    case Phi:
        ASSERT_NOT_REACHED();

    case Breakpoint:
#if ENABLE(DEBUG_WITH_BREAKPOINT)
        m_jit.breakpoint();
#else
        ASSERT_NOT_REACHED();
#endif
        break;
        
    case Call:
    case Construct:
        emitCall(node);
        break;

    case Resolve: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        callOperation(operationResolve, resultTag.gpr(), resultPayload.gpr(), identifier(node.identifierNumber()));
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
        break;
    }

    case ResolveBase: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        callOperation(operationResolveBase, resultTag.gpr(), resultPayload.gpr(), identifier(node.identifierNumber()));
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
        break;
    }

    case ResolveBaseStrictPut: {
        flushRegisters();
        GPRResult resultPayload(this);
        GPRResult2 resultTag(this);
        callOperation(operationResolveBaseStrictPut, resultTag.gpr(), resultPayload.gpr(), identifier(node.identifierNumber()));
        jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
        break;
    }

    case ResolveGlobal: {
        GPRTemporary globalObject(this);
        GPRTemporary resolveInfo(this);
        GPRTemporary resultTag(this);
        GPRTemporary resultPayload(this);

        GPRReg globalObjectGPR = globalObject.gpr();
        GPRReg resolveInfoGPR = resolveInfo.gpr();
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();

        ResolveGlobalData& data = m_jit.graph().m_resolveGlobalData[node.resolveGlobalDataIndex()];
        GlobalResolveInfo* resolveInfoAddress = &(m_jit.codeBlock()->globalResolveInfo(data.resolveInfoIndex));

        // Check Structure of global object
        m_jit.move(JITCompiler::TrustedImmPtr(m_jit.codeBlock()->globalObject()), globalObjectGPR);
        m_jit.move(JITCompiler::TrustedImmPtr(resolveInfoAddress), resolveInfoGPR);
        m_jit.load32(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(GlobalResolveInfo, structure) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        m_jit.load32(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(GlobalResolveInfo, structure) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);

        JITCompiler::JumpList structuresNotMatch;
        structuresNotMatch.append(m_jit.branch32(JITCompiler::NotEqual, resultTagGPR, JITCompiler::Address(globalObjectGPR, JSCell::structureOffset() + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag))));
        structuresNotMatch.append(m_jit.branch32(JITCompiler::NotEqual, resultPayloadGPR, JITCompiler::Address(globalObjectGPR, JSCell::structureOffset() + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload))));

        // Fast case
        m_jit.load32(JITCompiler::Address(globalObjectGPR, JSObject::offsetOfPropertyStorage()), resultPayloadGPR);
        m_jit.load32(JITCompiler::Address(resolveInfoGPR, OBJECT_OFFSETOF(GlobalResolveInfo, offset)), resolveInfoGPR);
        m_jit.load32(JITCompiler::BaseIndex(resultPayloadGPR, resolveInfoGPR, JITCompiler::TimesEight, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
        m_jit.load32(JITCompiler::BaseIndex(resultPayloadGPR, resolveInfoGPR, JITCompiler::TimesEight, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);

        JITCompiler::Jump wasFast = m_jit.jump();

        structuresNotMatch.link(&m_jit);
        silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
        m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(&m_jit.codeBlock()->identifier(data.identifierNumber))));
        m_jit.push(resolveInfoGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        JITCompiler::Call functionCall = appendCallWithExceptionCheck(operationResolveGlobal);
        setupResults(resultTagGPR, resultPayloadGPR);
        silentFillAllRegisters(resultTagGPR, resultPayloadGPR);

        wasFast.link(&m_jit);

        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex);
        break;
    }

    case ForceOSRExit: {
        terminateSpeculativeExecution();
        break;
    }

    case Phantom:
        // This is a no-op.
        noResult(m_compileIndex);
        break;
    }

    if (node.hasResult() && node.mustGenerate())
        use(m_compileIndex);
}

#endif

} } // namespace JSC::DFG

#endif
