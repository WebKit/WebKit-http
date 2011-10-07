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
#include "DFGJITCodeGenerator.h"

#if ENABLE(DFG_JIT)

#include "DFGJITCompilerInlineMethods.h"
#include "DFGSpeculativeJIT.h"
#include "LinkBuffer.h"

namespace JSC { namespace DFG {

#if USE(JSVALUE32_64)

GPRReg JITCodeGenerator::fillInteger(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (node.hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(nodeIndex))
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
            else if (isNumberConstant(nodeIndex))
                ASSERT_NOT_REACHED();
            else {
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::Imm32(jsValue.payload()), gpr);
            }
        } else {
            ASSERT(info.spillFormat() == DataFormatJS || info.spillFormat() == DataFormatJSInteger);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.load32(JITCompiler::payloadFor(virtualRegister), gpr);
        }

        info.fillInteger(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }

    switch (info.registerFormat()) {
    case DataFormatNone:
        // Should have filled, above.
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJS:
    case DataFormatCell:
    case DataFormatJSCell:
    case DataFormatJSInteger:
    case DataFormatBoolean:
    case DataFormatJSBoolean:
    case DataFormatStorage:
        // Should only be calling this function if we know this operand to be integer.
        ASSERT_NOT_REACHED();

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsInt32(gpr);
        returnFormat = DataFormatInteger;
        return gpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

FPRReg JITCodeGenerator::fillDouble(NodeIndex nodeIndex)
{
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {

        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex)) {
                // FIXME: should not be reachable?
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
                // FIXME: should not be reachable?
                ASSERT_NOT_REACHED();
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
        // Should have filled, above.
    case DataFormatCell:
    case DataFormatJSCell:
    case DataFormatBoolean:
    case DataFormatJSBoolean:
    case DataFormatStorage:
        // Should only be calling this function if we know this operand to be numeric.
        ASSERT_NOT_REACHED();

    case DataFormatJSInteger:
    case DataFormatJS: {
        GPRReg tagGPR = info.tagGPR();
        GPRReg payloadGPR = info.payloadGPR();
        FPRReg fpr = fprAllocate();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);

        JITCompiler::Jump hasUnboxedDouble;

        if (info.registerFormat() != DataFormatJSInteger) {
            JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));
            m_jit.jitAssertIsJSDouble(tagGPR);
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

bool JITCodeGenerator::fillJSValue(NodeIndex nodeIndex, GPRReg& tagGPR, GPRReg& payloadGPR, FPRReg& fpr)
{
    // FIXME: For double we could fill with a FPR.
    UNUSED_PARAM(fpr);

    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {

        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex)) {
                tagGPR = allocate();
                payloadGPR = allocate();
                m_jit.emitLoad(nodeIndex, tagGPR, payloadGPR);
                info.fillJSValue(tagGPR, payloadGPR, DataFormatJSInteger);
            } else {
                tagGPR = allocate();
                payloadGPR = allocate();
                m_jit.emitLoad(nodeIndex, tagGPR, payloadGPR);
                info.fillJSValue(tagGPR, payloadGPR, DataFormatJS);
            }

            m_gprs.retain(tagGPR, virtualRegister, SpillOrderConstant);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderConstant);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            tagGPR = allocate();
            payloadGPR = allocate();
            m_jit.emitLoad(nodeIndex, tagGPR, payloadGPR);
            m_gprs.retain(tagGPR, virtualRegister, SpillOrderSpilled);
            m_gprs.retain(payloadGPR, virtualRegister, SpillOrderSpilled);
            info.fillJSValue(tagGPR, payloadGPR, spillFormat);
        }

        return true;
    }

    case DataFormatCell:
    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInteger, not DataFormatJSInteger.
        if (m_gprs.isLocked(gpr)) {
            payloadGPR = allocate();
            m_jit.move(gpr, payloadGPR);
        } else {
            payloadGPR = gpr;
            m_gprs.lock(gpr);
        }
        tagGPR = allocate();
        m_jit.move(info.registerFormat() == DataFormatInteger ? JITCompiler::TrustedImm32(JSValue::Int32Tag) : JITCompiler::TrustedImm32(JSValue::CellTag), tagGPR);
        m_gprs.release(gpr);
        m_gprs.retain(tagGPR, virtualRegister, SpillOrderJS);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderJS);
        info.fillJSValue(tagGPR, payloadGPR, info.registerFormat() == DataFormatCell ? DataFormatJSCell : DataFormatJSInteger);
        return true;
    }

    case DataFormatJSDouble:
    case DataFormatDouble: {
        FPRReg oldFPR = info.fpr();
        m_fprs.lock(oldFPR);
        tagGPR = allocate();
        payloadGPR = allocate();
        boxDouble(oldFPR, tagGPR, payloadGPR, virtualRegister);
        m_fprs.unlock(oldFPR);
        m_fprs.release(oldFPR);
        m_gprs.retain(tagGPR, virtualRegister, SpillOrderJS);
        m_gprs.retain(payloadGPR, virtualRegister, SpillOrderJS);
        info.fillJSValue(tagGPR, payloadGPR, DataFormatJS);
        return true;
    }

    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        tagGPR = info.tagGPR();
        payloadGPR = info.payloadGPR();
        m_gprs.lock(tagGPR);
        m_gprs.lock(payloadGPR);
        return true;
    }
        
    case DataFormatBoolean:
    case DataFormatStorage:
        // this type currently never occurs
        ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
    return true;
}

void JITCodeGenerator::nonSpeculativeValueToNumber(Node& node)
{
    if (isKnownNumeric(node.child1())) {
        JSValueOperand op1(this, node.child1());
        op1.fill();
        if (op1.isDouble()) {
            FPRTemporary result(this, op1);
            m_jit.moveDouble(op1.fpr(), result.fpr());
            doubleResult(result.fpr(), m_compileIndex);
        } else {
            GPRTemporary resultTag(this, op1);
            GPRTemporary resultPayload(this, op1, false);
            m_jit.move(op1.tagGPR(), resultTag.gpr());
            m_jit.move(op1.payloadGPR(), resultPayload.gpr());
            jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
        }
        return;
    }

    JSValueOperand op1(this, node.child1());
    GPRTemporary resultTag(this, op1);
    GPRTemporary resultPayload(this, op1, false);

    ASSERT(!isInt32Constant(node.child1()));
    ASSERT(!isNumberConstant(node.child1()));

    GPRReg tagGPR = op1.tagGPR();
    GPRReg payloadGPR = op1.payloadGPR();
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    op1.use();

    JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));
    JITCompiler::Jump nonNumeric = m_jit.branch32(MacroAssembler::AboveOrEqual, tagGPR, TrustedImm32(JSValue::LowestTag));

    // First, if we get here we have a double encoded as a JSValue
    JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

    // Next handle cells (& other JS immediates)
    nonNumeric.link(&m_jit);
    silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
    m_jit.push(tagGPR);
    m_jit.push(payloadGPR);
    m_jit.push(GPRInfo::callFrameRegister);
    appendCallWithExceptionCheck(dfgConvertJSValueToNumber);
    boxDouble(FPRInfo::returnValueFPR, resultTagGPR, resultPayloadGPR, at(m_compileIndex).virtualRegister());
    silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
    JITCompiler::Jump hasCalledToNumber = m_jit.jump();
    
    // Finally, handle integers.
    isInteger.link(&m_jit);
    hasUnboxedDouble.link(&m_jit);
    m_jit.move(tagGPR, resultTagGPR);
    m_jit.move(payloadGPR, resultPayloadGPR);
    hasCalledToNumber.link(&m_jit);
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeValueToInt32(Node& node)
{
    ASSERT(!isInt32Constant(node.child1()));

    if (isKnownInteger(node.child1())) {
        IntegerOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        integerResult(result.gpr(), m_compileIndex);
        return;
    }

    GenerationInfo& childInfo = m_generationInfo[at(node.child1()).virtualRegister()];
    if (isJSDouble(childInfo.registerFormat())) {
        DoubleOperand op1(this, node.child1());
        GPRTemporary result(this);
        FPRReg fpr = op1.fpr();
        GPRReg gpr = result.gpr();
        op1.use();
        JITCompiler::Jump truncatedToInteger = m_jit.branchTruncateDoubleToInt32(fpr, gpr, JITCompiler::BranchIfTruncateSuccessful);

        silentSpillAllRegisters(gpr);

        m_jit.subPtr(TrustedImm32(sizeof(double)), JITCompiler::stackPointerRegister);
        m_jit.storeDouble(fpr, JITCompiler::stackPointerRegister);
        appendCallWithExceptionCheck(toInt32);
        m_jit.move(GPRInfo::returnValueGPR, gpr);
        m_jit.addPtr(TrustedImm32(sizeof(double)), JITCompiler::stackPointerRegister);

        silentFillAllRegisters(gpr);

        truncatedToInteger.link(&m_jit);
        integerResult(gpr, m_compileIndex, UseChildrenCalledExplicitly);
        return;
    }

    JSValueOperand op1(this, node.child1());
    GPRTemporary result(this);
    GPRReg tagGPR = op1.tagGPR();
    GPRReg payloadGPR = op1.payloadGPR();
    GPRReg resultGPR = result.gpr();
    op1.use();

    JITCompiler::Jump isInteger = m_jit.branch32(MacroAssembler::Equal, tagGPR, TrustedImm32(JSValue::Int32Tag));

    // First handle non-integers
    silentSpillAllRegisters(resultGPR);
    m_jit.push(tagGPR);
    m_jit.push(payloadGPR);
    m_jit.push(GPRInfo::callFrameRegister);
    appendCallWithExceptionCheck(dfgConvertJSValueToInt32);
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    silentFillAllRegisters(resultGPR);
    JITCompiler::Jump hasCalledToInt32 = m_jit.jump();

    // Then handle integers.
    isInteger.link(&m_jit);
    m_jit.move(payloadGPR, resultGPR);
    hasCalledToInt32.link(&m_jit);
    integerResult(resultGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeUInt32ToNumber(Node& node)
{
    IntegerOperand op1(this, node.child1());
    FPRTemporary boxer(this);
    GPRTemporary resultTag(this, op1);
    GPRTemporary resultPayload(this);
        
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, op1.gpr(), TrustedImm32(0));
        
    m_jit.convertInt32ToDouble(op1.gpr(), boxer.fpr());
    m_jit.move(JITCompiler::TrustedImmPtr(&twoToThe32), resultPayload.gpr()); // reuse resultPayload register here.
    m_jit.addDouble(JITCompiler::Address(resultPayload.gpr(), 0), boxer.fpr());
        
    boxDouble(boxer.fpr(), resultTag.gpr(), resultPayload.gpr(), at(m_compileIndex).virtualRegister());
        
    JITCompiler::Jump done = m_jit.jump();
        
    positive.link(&m_jit);
        
    m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTag.gpr());
    m_jit.move(op1.gpr(), resultPayload.gpr());
        
    done.link(&m_jit);

    jsValueResult(resultTag.gpr(), resultPayload.gpr(), m_compileIndex);
}

void JITCodeGenerator::nonSpeculativeKnownConstantArithOp(NodeType op, NodeIndex regChild, NodeIndex immChild, bool commute)
{
    JSValueOperand regArg(this, regChild);
    regArg.fill();
    
    if (regArg.isDouble()) {
        FPRReg regArgFPR = regArg.fpr();
        FPRTemporary imm(this);
        FPRTemporary result(this, regArg);
        GPRTemporary scratch(this);
        FPRReg immFPR = imm.fpr();
        FPRReg resultFPR = result.fpr();
        GPRReg scratchGPR = scratch.gpr();
        use(regChild);
        use(immChild);

        int32_t imm32 = valueOfInt32Constant(immChild);
        m_jit.move(TrustedImm32(imm32), scratchGPR);
        m_jit.convertInt32ToDouble(scratchGPR, immFPR);

        switch (op) {
        case ValueAdd:
        case ArithAdd:
            m_jit.addDouble(regArgFPR, immFPR, resultFPR);
            break;
        
        case ArithSub:
            m_jit.subDouble(regArgFPR, immFPR, resultFPR);
            break;
            
        default:
            ASSERT_NOT_REACHED();
        }

        doubleResult(resultFPR, m_compileIndex, UseChildrenCalledExplicitly);
        return;
    }

    GPRReg regArgTagGPR = regArg.tagGPR();
    GPRReg regArgPayloadGPR = regArg.payloadGPR();
    GPRTemporary resultTag(this, regArg);
    GPRTemporary resultPayload(this, regArg, false);
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    FPRTemporary tmp1(this);
    FPRTemporary tmp2(this);
    FPRReg tmp1FPR = tmp1.fpr();
    FPRReg tmp2FPR = tmp2.fpr();
    use(regChild);
    use(immChild);

    JITCompiler::Jump notInt;
    int32_t imm = valueOfInt32Constant(immChild);

    if (!isKnownNumeric(regChild))
        notInt = m_jit.branch32(MacroAssembler::NotEqual, regArgTagGPR, TrustedImm32(JSValue::Int32Tag));
    
    JITCompiler::Jump overflow;

    switch (op) {
    case ValueAdd:
    case ArithAdd:
        overflow = m_jit.branchAdd32(MacroAssembler::Overflow, regArgPayloadGPR, Imm32(imm), resultPayloadGPR);
        break;
        
    case ArithSub:
        overflow = m_jit.branchSub32(MacroAssembler::Overflow, regArgPayloadGPR, Imm32(imm), resultPayloadGPR);
        break;
        
    default:
        ASSERT_NOT_REACHED();
    }

    m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
    JITCompiler::Jump done = m_jit.jump();
    
    overflow.link(&m_jit);
    // first deal with overflow case
    m_jit.convertInt32ToDouble(regArgPayloadGPR, tmp2FPR);
    m_jit.move(TrustedImm32(imm), resultPayloadGPR);
    m_jit.convertInt32ToDouble(resultPayloadGPR, tmp1FPR);
    switch (op) {
    case ValueAdd:
    case ArithAdd:
        m_jit.addDouble(tmp1FPR, tmp2FPR);
        break;
        
    case ArithSub:
        m_jit.subDouble(tmp1FPR, tmp2FPR);
        break;
            
    default:
        ASSERT_NOT_REACHED();
    }
    
    JITCompiler::Jump doneCaseConvertedToInt;
    
    if (op == ValueAdd) {
        JITCompiler::JumpList failureCases;
        m_jit.branchConvertDoubleToInt32(tmp2FPR, resultPayloadGPR, failureCases, tmp1FPR);
        m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
        doneCaseConvertedToInt = m_jit.jump();
        
        failureCases.link(&m_jit);
    }
    
    boxDouble(tmp2FPR, resultTagGPR, resultPayloadGPR, at(m_compileIndex).virtualRegister());
        
    if (!isKnownNumeric(regChild)) {
        ASSERT(notInt.isSet());
        ASSERT(op == ValueAdd);
            
        JITCompiler::Jump doneCaseWasNumber = m_jit.jump();
            
        notInt.link(&m_jit);
            
        silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
        if (commute) {
            m_jit.push(regArgTagGPR);
            m_jit.push(regArgPayloadGPR);
            m_jit.push(MacroAssembler::Imm32(imm));
        } else {
            m_jit.push(MacroAssembler::Imm32(imm));
            m_jit.push(regArgTagGPR);
            m_jit.push(regArgPayloadGPR);
        }
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationValueAddNotNumber);
        setupResults(resultTagGPR, resultPayloadGPR);
        silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
            
        doneCaseWasNumber.link(&m_jit);
    }
    
    done.link(&m_jit);
    if (doneCaseConvertedToInt.isSet())
        doneCaseConvertedToInt.link(&m_jit);

    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeBasicArithOp(NodeType op, Node &node)
{
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    arg1.fill();
    arg2.fill();

    if (arg1.isDouble() && arg2.isDouble()) {
        FPRReg arg1FPR = arg1.fpr();
        FPRReg arg2FPR = arg2.fpr();
        FPRTemporary result(this, arg1);
        arg1.use();
        arg2.use();
        switch (op) {
        case ValueAdd:
        case ArithAdd:
            m_jit.addDouble(arg1FPR, arg2FPR, result.fpr());
            break;
            
        case ArithSub:
            m_jit.subDouble(arg1FPR, arg2FPR, result.fpr());
            break;
            
        case ArithMul:
            m_jit.mulDouble(arg1FPR, arg2FPR, result.fpr());
            break;
            
        default:
            ASSERT_NOT_REACHED();
        }

        doubleResult(result.fpr(), m_compileIndex, UseChildrenCalledExplicitly);
        return;
    }

    FPRTemporary tmp1(this);
    FPRTemporary tmp2(this);
    FPRReg tmp1FPR = tmp1.fpr();
    FPRReg tmp2FPR = tmp2.fpr();
    
    GPRTemporary resultTag(this, arg1.isDouble() ? arg2 : arg1);
    GPRTemporary resultPayload(this, arg1.isDouble() ? arg2 : arg1, false);
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();

    GPRReg arg1TagGPR = InvalidGPRReg;
    GPRReg arg1PayloadGPR = InvalidGPRReg;
    GPRReg arg2TagGPR = InvalidGPRReg;
    GPRReg arg2PayloadGPR = InvalidGPRReg;
    GPRTemporary tmpTag(this);
    GPRTemporary tmpPayload(this);

    if (arg1.isDouble()) {
        arg1TagGPR = tmpTag.gpr();
        arg1PayloadGPR = tmpPayload.gpr();
        boxDouble(arg1.fpr(), arg1TagGPR, arg1PayloadGPR, at(arg1.index()).virtualRegister());
        arg2TagGPR = arg2.tagGPR();
        arg2PayloadGPR = arg2.payloadGPR();
    } else if (arg2.isDouble()) {
        arg1TagGPR = arg1.tagGPR();
        arg1PayloadGPR = arg1.payloadGPR();
        arg2TagGPR = tmpTag.gpr();
        arg2PayloadGPR = tmpPayload.gpr();
        boxDouble(arg2.fpr(), arg2TagGPR, arg2PayloadGPR, at(arg2.index()).virtualRegister());
    } else {
        arg1TagGPR = arg1.tagGPR();
        arg1PayloadGPR = arg1.payloadGPR();
        arg2TagGPR = arg2.tagGPR();
        arg2PayloadGPR = arg2.payloadGPR();
    }

    arg1.use();
    arg2.use();
    
    JITCompiler::Jump child1NotInt;
    JITCompiler::Jump child2NotInt;
    JITCompiler::JumpList overflow;
    
    if (!isKnownInteger(node.child1()))
        child1NotInt = m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, TrustedImm32(JSValue::Int32Tag));

    if (!isKnownInteger(node.child2()))
        child2NotInt = m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, TrustedImm32(JSValue::Int32Tag));
    
    switch (op) {
    case ValueAdd:
    case ArithAdd: {
        overflow.append(m_jit.branchAdd32(MacroAssembler::Overflow, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR));
        break;
    }
        
    case ArithSub: {
        overflow.append(m_jit.branchSub32(MacroAssembler::Overflow, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR));
        break;
    }
        
    case ArithMul: {
        overflow.append(m_jit.branchMul32(MacroAssembler::Overflow, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR));
        overflow.append(m_jit.branchTest32(MacroAssembler::Zero, resultPayloadGPR));
        break;
    }
        
    default:
        ASSERT_NOT_REACHED();
    }
    
    m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
        
    JITCompiler::Jump done = m_jit.jump();
    
    JITCompiler::JumpList haveFPRArguments;

    overflow.link(&m_jit);
        
    // both arguments are integers
    m_jit.convertInt32ToDouble(arg1PayloadGPR, tmp1FPR);
    m_jit.convertInt32ToDouble(arg2PayloadGPR, tmp2FPR);
        
    haveFPRArguments.append(m_jit.jump());
        
    JITCompiler::JumpList notNumbers;
        
    JITCompiler::Jump child2NotInt2;
        
    if (!isKnownInteger(node.child1())) {
        child1NotInt.link(&m_jit);
            
        if (!isKnownNumeric(node.child1())) {
            ASSERT(op == ValueAdd);
            notNumbers.append(m_jit.branch32(MacroAssembler::AboveOrEqual, arg1TagGPR, TrustedImm32(JSValue::LowestTag)));
        }
    
        if (arg1.isDouble())
            m_jit.moveDouble(arg1.fpr(), tmp1FPR);
        else
            unboxDouble(arg1TagGPR, arg1PayloadGPR, tmp1FPR, at(arg1.index()).virtualRegister());
            
        // child1 is converted to a double; child2 may either be an int or
        // a boxed double
            
        if (!isKnownInteger(node.child2())) {
            if (isKnownNumeric(node.child2()))
                child2NotInt2 = m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, TrustedImm32(JSValue::Int32Tag));
            else {
                ASSERT(op == ValueAdd);
                JITCompiler::Jump child2IsInt = m_jit.branch32(MacroAssembler::Equal, arg2TagGPR, TrustedImm32(JSValue::Int32Tag));
                notNumbers.append(m_jit.branch32(MacroAssembler::AboveOrEqual, arg2TagGPR, TrustedImm32(JSValue::LowestTag)));
                child2NotInt2 = m_jit.jump();
                child2IsInt.link(&m_jit);
            }
        }
            
        // child 2 is definitely an integer
        m_jit.convertInt32ToDouble(arg2PayloadGPR, tmp2FPR);
            
        haveFPRArguments.append(m_jit.jump());
    }
        
    if (!isKnownInteger(node.child2())) {
        child2NotInt.link(&m_jit);
            
        if (!isKnownNumeric(node.child2())) {
            ASSERT(op == ValueAdd);
            notNumbers.append(m_jit.branch32(MacroAssembler::AboveOrEqual, arg2TagGPR, TrustedImm32(JSValue::LowestTag)));
        }
            
        // child1 is definitely an integer, and child 2 is definitely not
        m_jit.convertInt32ToDouble(arg1PayloadGPR, tmp1FPR);
            
        if (child2NotInt2.isSet())
            child2NotInt2.link(&m_jit);
            
        if (arg2.isDouble())
            m_jit.moveDouble(arg2.fpr(), tmp2FPR);
        else
            unboxDouble(arg2TagGPR, arg2PayloadGPR, tmp2FPR, at(arg2.index()).virtualRegister());
    }
        
    haveFPRArguments.link(&m_jit);
        
    switch (op) {
    case ValueAdd:
    case ArithAdd:
        m_jit.addDouble(tmp2FPR, tmp1FPR);
        break;
            
    case ArithSub:
        m_jit.subDouble(tmp2FPR, tmp1FPR);
        break;
            
    case ArithMul:
        m_jit.mulDouble(tmp2FPR, tmp1FPR);
        break;
            
    default:
        ASSERT_NOT_REACHED();
    }
    
    JITCompiler::Jump doneCaseConvertedToInt;
    
    if (op == ValueAdd) {
        JITCompiler::JumpList failureCases;
        m_jit.branchConvertDoubleToInt32(tmp1FPR, resultPayloadGPR, failureCases, tmp2FPR);
        m_jit.move(TrustedImm32(JSValue::Int32Tag), resultTagGPR);
        
        doneCaseConvertedToInt = m_jit.jump();
        
        failureCases.link(&m_jit);
    }
        
    boxDouble(tmp1FPR, resultTagGPR, resultPayloadGPR, at(m_compileIndex).virtualRegister());
        
    if (!notNumbers.empty()) {
        ASSERT(op == ValueAdd);
            
        JITCompiler::Jump doneCaseWasNumber = m_jit.jump();
            
        notNumbers.link(&m_jit);
            
        silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
        m_jit.push(arg2TagGPR);
        m_jit.push(arg2PayloadGPR);
        m_jit.push(arg1TagGPR);
        m_jit.push(arg1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationValueAddNotNumber);
        setupResults(resultTagGPR, resultPayloadGPR);
        silentFillAllRegisters(resultTagGPR, resultPayloadGPR);

        doneCaseWasNumber.link(&m_jit);
    }
    
    done.link(&m_jit);
    if (doneCaseConvertedToInt.isSet())
        doneCaseConvertedToInt.link(&m_jit);
        
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeArithMod(Node& node)
{
    JSValueOperand op1(this, node.child1());
    JSValueOperand op2(this, node.child2());
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);

    FPRTemporary op1Double(this);
    FPRTemporary op2Double(this);
    
    GPRReg op1TagGPR = op1.tagGPR();
    GPRReg op1PayloadGPR = op1.payloadGPR();
    GPRReg op2TagGPR = op2.tagGPR();
    GPRReg op2PayloadGPR = op2.payloadGPR();
    
    op1.use();
    op2.use();
    
    GPRReg temp2 = InvalidGPRReg;
    if (op2PayloadGPR == X86Registers::eax || op2PayloadGPR == X86Registers::edx) {
        temp2 = allocate();
        m_jit.move(op2PayloadGPR, temp2);
        op2PayloadGPR = temp2;
    }

    JITCompiler::JumpList done;
    JITCompiler::JumpList slow;
    JITCompiler::Jump modByZero;
    
    if (!isKnownInteger(node.child1()))
        slow.append(m_jit.branch32(MacroAssembler::NotEqual, op1TagGPR, TrustedImm32(JSValue::Int32Tag)));
    if (!isKnownInteger(node.child2()))
        slow.append(m_jit.branch32(MacroAssembler::NotEqual, op2TagGPR, TrustedImm32(JSValue::Int32Tag)));
    
    modByZero = m_jit.branchTest32(MacroAssembler::Zero, op2PayloadGPR);
    
    m_jit.move(op1PayloadGPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2PayloadGPR);

    m_jit.move(TrustedImm32(JSValue::Int32Tag), X86Registers::eax);
    done.append(m_jit.jump());
        
    modByZero.link(&m_jit);
    m_jit.move(MacroAssembler::TrustedImm32(jsNumber(std::numeric_limits<double>::quiet_NaN()).tag()), X86Registers::eax);
    m_jit.move(MacroAssembler::TrustedImm32(jsNumber(std::numeric_limits<double>::quiet_NaN()).payload()), X86Registers::edx);
    done.append(m_jit.jump());
    
    if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
        slow.link(&m_jit);
        silentSpillAllRegisters(X86Registers::eax, X86Registers::edx);
        m_jit.push(op2TagGPR);
        m_jit.push(op2PayloadGPR);
        m_jit.push(op1TagGPR);
        m_jit.push(op1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationArithMod);
        setupResults(X86Registers::eax, X86Registers::edx);
        silentFillAllRegisters(X86Registers::eax, X86Registers::edx);
    }
        
    done.link(&m_jit);
    
    if (temp2 != InvalidGPRReg)
        unlock(temp2);
    
    jsValueResult(X86Registers::eax, X86Registers::edx, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeCheckHasInstance(Node& node)
{
    JSValueOperand base(this, node.child1());
    GPRTemporary structure(this);
    GPRReg baseTagReg = base.tagGPR();
    GPRReg basePayloadReg = base.payloadGPR();
    GPRReg structureReg = structure.gpr();

    // Check that base is a cell.
    MacroAssembler::Jump baseNotCell = m_jit.branch32(MacroAssembler::NotEqual, baseTagReg, TrustedImm32(JSValue::CellTag));

    // Check that base 'ImplementsHasInstance'.
    m_jit.loadPtr(MacroAssembler::Address(basePayloadReg, JSCell::structureOffset()), structureReg);
    MacroAssembler::Jump implementsHasInstance = m_jit.branchTest8(MacroAssembler::NonZero, MacroAssembler::Address(structureReg, Structure::typeInfoFlagsOffset()), MacroAssembler::TrustedImm32(ImplementsHasInstance));

    // At this point we always throw, so no need to preserve registers.
    baseNotCell.link(&m_jit);
    m_jit.push(baseTagReg); // tag
    m_jit.push(basePayloadReg); // payload
    m_jit.push(GPRInfo::callFrameRegister);
    // At some point we could optimize this to plant a direct jump, rather then checking
    // for an exception (operationThrowHasInstanceError always throws). Probably not worth
    // adding the extra interface to do this now, but we may also want this for op_throw.
    appendCallWithExceptionCheck(operationThrowHasInstanceError);

    implementsHasInstance.link(&m_jit);
    noResult(m_compileIndex);
}

void JITCodeGenerator::nonSpeculativeInstanceOf(Node& node)
{
    // FIXME: Currently we flush all registers as the number of available registers
    // does not meet our requirement.
    flushRegisters();
    GPRTemporary value(this);
    GPRTemporary base(this);
    GPRTemporary prototype(this);
    GPRTemporary scratch(this);

    GPRReg valueReg = value.gpr();
    GPRReg baseReg = base.gpr();
    GPRReg prototypeReg = prototype.gpr();
    GPRReg scratchReg = scratch.gpr();
        
    use(node.child3());
    use(node.child1());
    use(node.child2());

    // Check that operands are cells (base is checked by CheckHasInstance, so we can just assert).
    m_jit.emitLoadTag(node.child3(), valueReg);
    MacroAssembler::Jump valueNotCell = m_jit.branch32(MacroAssembler::NotEqual, valueReg, TrustedImm32(JSValue::CellTag));
    m_jit.emitLoadTag(node.child1(), baseReg);
    m_jit.jitAssertIsCell(baseReg);
    m_jit.emitLoadTag(node.child2(), prototypeReg);
    MacroAssembler::Jump prototypeNotCell = m_jit.branch32(MacroAssembler::NotEqual, prototypeReg, TrustedImm32(JSValue::CellTag));

    // Check that baseVal 'ImplementsDefaultHasInstance'.
    m_jit.emitLoadPayload(node.child1(), baseReg);
    m_jit.loadPtr(MacroAssembler::Address(baseReg, JSCell::structureOffset()), scratchReg);
    MacroAssembler::Jump notDefaultHasInstance = m_jit.branchTest8(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, Structure::typeInfoFlagsOffset()), TrustedImm32(ImplementsDefaultHasInstance));

    // Check that prototype is an object
    m_jit.emitLoadPayload(node.child2(), prototypeReg);
    m_jit.loadPtr(MacroAssembler::Address(prototypeReg, JSCell::structureOffset()), scratchReg);
    // MacroAssembler::Jump protoNotObject = m_jit.branch8(MacroAssembler::NotEqual, MacroAssembler::Address(scratchReg, Structure::typeInfoTypeOffset()), MacroAssembler::TrustedImm32(ObjectType));
    MacroAssembler::Jump protoNotObject = m_jit.branchIfNotObject(scratchReg);

    // Initialize scratchReg with the value being checked.
    m_jit.emitLoadPayload(node.child3(), valueReg);
    m_jit.move(valueReg, scratchReg);

    // Walk up the prototype chain of the value (in scratchReg), comparing to prototypeReg.
    MacroAssembler::Label loop(&m_jit);
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, JSCell::structureOffset()), scratchReg);
    m_jit.load32(MacroAssembler::Address(scratchReg, Structure::prototypeOffset() + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), scratchReg);
    MacroAssembler::Jump isInstance = m_jit.branch32(MacroAssembler::Equal, scratchReg, prototypeReg);
    m_jit.branchTest32(MacroAssembler::NonZero, scratchReg).linkTo(loop, &m_jit);

    // No match - result is false.
    m_jit.move(TrustedImm32(0), GPRInfo::returnValueGPR);
    MacroAssembler::Jump wasNotInstance = m_jit.jump();

    // Link to here if any checks fail that require us to try calling out to an operation to help,
    // e.g. for an API overridden HasInstance.
    valueNotCell.link(&m_jit);
    prototypeNotCell.link(&m_jit);
    notDefaultHasInstance.link(&m_jit);
    protoNotObject.link(&m_jit);

    // FIXME: ld/st should be reduced if carefully arranged.
    m_jit.emitLoadTag(node.child2(), prototypeReg);
    m_jit.push(prototypeReg);
    m_jit.emitLoadPayload(node.child2(), prototypeReg);
    m_jit.push(prototypeReg);
    m_jit.emitLoadTag(node.child1(), baseReg);
    m_jit.push(baseReg);
    m_jit.emitLoadPayload(node.child1(), baseReg);
    m_jit.push(baseReg);
    m_jit.emitLoadTag(node.child3(), valueReg);
    m_jit.push(valueReg);
    m_jit.emitLoadPayload(node.child3(), valueReg);
    m_jit.push(valueReg);
    m_jit.push(GPRInfo::callFrameRegister);
    appendCallWithExceptionCheck(operationInstanceOf);
    MacroAssembler::Jump wasNotDefaultHasInstance = m_jit.jump();

    isInstance.link(&m_jit);
    m_jit.move(TrustedImm32(1), GPRInfo::returnValueGPR);

    wasNotInstance.link(&m_jit);
    m_jit.move(TrustedImm32(JSValue::BooleanTag), GPRInfo::returnValueGPR2);
    
    wasNotDefaultHasInstance.link(&m_jit);
    jsValueResult(GPRInfo::returnValueGPR2, GPRInfo::returnValueGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

JITCompiler::Call JITCodeGenerator::cachedGetById(GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, NodeType nodeType)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(basePayloadGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    m_jit.loadPtr(JITCompiler::Address(basePayloadGPR, JSObject::offsetOfPropertyStorage()), resultPayloadGPR);
    JITCompiler::DataLabelCompact tagLoadWithPatch = m_jit.load32WithCompactAddressOffsetPatch(JITCompiler::Address(resultPayloadGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), resultTagGPR);
    JITCompiler::DataLabelCompact payloadLoadWithPatch = m_jit.load32WithCompactAddressOffsetPatch(JITCompiler::Address(resultPayloadGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), resultPayloadGPR);
    
    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);
    
    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);
    
    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
    m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(identifier(identifierNumber))));
    m_jit.push(TrustedImm32(JSValue::CellTag));
    m_jit.push(basePayloadGPR);
    m_jit.push(GPRInfo::callFrameRegister);
    JITCompiler::Call functionCall;
    switch (nodeType) {
    case GetById:
        functionCall = appendCallWithExceptionCheck(operationGetByIdOptimize);
        break;
        
    case GetMethod:
        functionCall = appendCallWithExceptionCheck(operationGetMethodOptimize);
        break;
        
    default:
        ASSERT_NOT_REACHED();
        return JITCompiler::Call();
    }
    setupResults(resultTagGPR, resultPayloadGPR);
    silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
    
    done.link(&m_jit);
    
    JITCompiler::Label doneLabel = m_jit.label();

    int16_t checkImmToCall = safeCast<int16_t>(m_jit.differenceBetween(structureToCompare, functionCall));
    int16_t callToCheck = safeCast<int16_t>(m_jit.differenceBetween(functionCall, structureCheck));
    int16_t callToTagLoad = safeCast<int16_t>(m_jit.differenceBetween(functionCall, tagLoadWithPatch));
    int16_t callToPayloadLoad = safeCast<int16_t>(m_jit.differenceBetween(functionCall, payloadLoadWithPatch));
    int16_t callToSlowCase = safeCast<int16_t>(m_jit.differenceBetween(functionCall, slowCase));
    int16_t callToDone = safeCast<int16_t>(m_jit.differenceBetween(functionCall, doneLabel));
    
    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToTagLoad, callToPayloadLoad, callToSlowCase, callToDone, safeCast<int8_t>(basePayloadGPR), safeCast<int8_t>(resultTagGPR), safeCast<int8_t>(resultPayloadGPR), safeCast<int8_t>(scratchGPR));
    
    return functionCall;
}

void JITCodeGenerator::cachedPutById(GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, NodeIndex valueIndex, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(basePayloadGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));

    writeBarrier(basePayloadGPR, valueTagGPR, valueIndex, WriteBarrierForPropertyAccess, scratchGPR);

    m_jit.loadPtr(JITCompiler::Address(basePayloadGPR, JSObject::offsetOfPropertyStorage()), scratchGPR);
    JITCompiler::DataLabel32 tagStoreWithPatch = m_jit.store32WithAddressOffsetPatch(valueTagGPR, JITCompiler::Address(scratchGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    JITCompiler::DataLabel32 payloadStoreWithPatch = m_jit.store32WithAddressOffsetPatch(valuePayloadGPR, JITCompiler::Address(scratchGPR, OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));

    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);

    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);

    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(InvalidGPRReg);
    m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(identifier(identifierNumber))));
    m_jit.push(TrustedImm32(JSValue::CellTag));
    m_jit.push(basePayloadGPR);
    m_jit.push(valueTagGPR);
    m_jit.push(valuePayloadGPR);
    m_jit.push(GPRInfo::callFrameRegister);
    V_DFGOperation_EJJI optimizedCall;
    if (m_jit.codeBlock()->isStrictMode()) {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectStrictOptimize;
        else
            optimizedCall = operationPutByIdStrictOptimize;
    } else {
        if (putKind == Direct)
            optimizedCall = operationPutByIdDirectNonStrictOptimize;
        else
            optimizedCall = operationPutByIdNonStrictOptimize;
    }
    JITCompiler::Call functionCall = appendCallWithExceptionCheck(optimizedCall);
    silentFillAllRegisters(InvalidGPRReg);

    done.link(&m_jit);
    JITCompiler::Label doneLabel = m_jit.label();

    int16_t checkImmToCall = safeCast<int16_t>(m_jit.differenceBetween(structureToCompare, functionCall));
    int16_t callToCheck = safeCast<int16_t>(m_jit.differenceBetween(functionCall, structureCheck));
    int16_t callToTagStore = safeCast<int16_t>(m_jit.differenceBetween(functionCall, tagStoreWithPatch));
    int16_t callToPayloadStore = safeCast<int16_t>(m_jit.differenceBetween(functionCall, payloadStoreWithPatch));
    int16_t callToSlowCase = safeCast<int16_t>(m_jit.differenceBetween(functionCall, slowCase));
    int16_t callToDone = safeCast<int16_t>(m_jit.differenceBetween(functionCall, doneLabel));

    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToTagStore, callToPayloadStore, callToSlowCase, callToDone, safeCast<int8_t>(basePayloadGPR), safeCast<int8_t>(valueTagGPR), safeCast<int8_t>(valuePayloadGPR), safeCast<int8_t>(scratchGPR));
}

void JITCodeGenerator::cachedGetMethod(GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::Call slowCall;
    JITCompiler::DataLabelPtr structToCompare, protoObj, protoStructToCompare, putFunction;
    
    // m_jit.emitLoadPayload(baseIndex, scratchGPR);
    JITCompiler::Jump wrongStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(basePayloadGPR, JSCell::structureOffset()), structToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    protoObj = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultPayloadGPR);
    JITCompiler::Jump wrongProtoStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(resultPayloadGPR, JSCell::structureOffset()), protoStructToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    putFunction = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultPayloadGPR);
    
    JITCompiler::Jump done = m_jit.jump();
    
    wrongStructure.link(&m_jit);
    wrongProtoStructure.link(&m_jit);
    
    slowCall = cachedGetById(basePayloadGPR, resultTagGPR, resultPayloadGPR, scratchGPR, identifierNumber, slowPathTarget, GetMethod);
    
    done.link(&m_jit);
    
    m_jit.addMethodGet(slowCall, structToCompare, protoObj, protoStructToCompare, putFunction);
}

void JITCodeGenerator::nonSpeculativeNonPeepholeCompareNull(NodeIndex operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();

    GPRTemporary resultTag(this, arg);
    GPRTemporary resultPayload(this, arg, false);
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();

    JITCompiler::Jump notCell;
    if (!isKnownCell(operand))
        notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));
    
    m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureOffset()), resultPayloadGPR);
    m_jit.test8(invert ? JITCompiler::Zero : JITCompiler::NonZero, JITCompiler::Address(resultPayloadGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined), resultPayloadGPR);
    
    if (!isKnownCell(operand)) {
        JITCompiler::JumpList done;
        done.append(m_jit.jump());
        
        notCell.link(&m_jit);
        // null or undefined?
        JITCompiler::Jump checkUndefined = m_jit.branch32(invert ? JITCompiler::Equal: JITCompiler::NotEqual, argTagGPR, JITCompiler::TrustedImm32(JSValue::NullTag));
        m_jit.move(JITCompiler::TrustedImm32(1), resultPayloadGPR);
        done.append(m_jit.jump());

        checkUndefined.link(&m_jit);
        m_jit.compare32(invert ? JITCompiler::NotEqual: JITCompiler::Equal, argTagGPR, JITCompiler::TrustedImm32(JSValue::UndefinedTag), resultPayloadGPR);

        done.link(&m_jit);
    }
    
    m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTagGPR);
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean);
}

void JITCodeGenerator::nonSpeculativePeepholeBranchNull(NodeIndex operand, NodeIndex branchNodeIndex, bool invert)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.takenBytecodeOffset());
    BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.notTakenBytecodeOffset());
    
    if (taken == (m_block + 1)) {
        invert = !invert;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg(this, operand);
    GPRReg argTagGPR = arg.tagGPR();
    GPRReg argPayloadGPR = arg.payloadGPR();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    if (!isKnownCell(operand))
        notCell = m_jit.branch32(MacroAssembler::NotEqual, argTagGPR, TrustedImm32(JSValue::CellTag));
    
    m_jit.loadPtr(JITCompiler::Address(argPayloadGPR, JSCell::structureOffset()), resultGPR);
    addBranch(m_jit.branchTest8(invert ? JITCompiler::Zero : JITCompiler::NonZero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined)), taken);
    
    if (!isKnownCell(operand)) {
        addBranch(m_jit.jump(), notTaken);
        
        notCell.link(&m_jit);
        // null or undefined?
        addBranch(m_jit.branch32(invert ? JITCompiler::NotEqual: JITCompiler::Equal, argTagGPR, JITCompiler::TrustedImm32(JSValue::NullTag)), taken);
        addBranch(m_jit.branch32(invert ? JITCompiler::NotEqual: JITCompiler::Equal, argTagGPR, JITCompiler::TrustedImm32(JSValue::UndefinedTag)), taken);
    }
    
    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
}

bool JITCodeGenerator::nonSpeculativeCompareNull(Node& node, NodeIndex operand, bool invert)
{
    NodeIndex branchNodeIndex = detectPeepHoleBranch();
    if (branchNodeIndex != NoNode) {
        ASSERT(node.adjustedRefCount() == 1);
        
        nonSpeculativePeepholeBranchNull(operand, branchNodeIndex, invert);
    
        use(node.child1());
        use(node.child2());
        m_compileIndex = branchNodeIndex;
        
        return true;
    }
    
    nonSpeculativeNonPeepholeCompareNull(operand, invert);
    
    return false;
}

void JITCodeGenerator::nonSpeculativePeepholeBranch(Node& node, NodeIndex branchNodeIndex, MacroAssembler::RelationalCondition cond, Z_DFGOperation_EJJ helperFunction)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.takenBytecodeOffset());
    BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.notTakenBytecodeOffset());

    JITCompiler::ResultCondition callResultCondition = JITCompiler::NonZero;

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == (m_block + 1)) {
        cond = JITCompiler::invert(cond);
        callResultCondition = JITCompiler::Zero;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }

    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node.child1()) || isKnownNotInteger(node.child2())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();

        flushRegisters();

        callOperation(helperFunction, resultGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        addBranch(m_jit.branchTest8(callResultCondition, resultGPR), taken);
    } else {
        GPRTemporary result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();

        if (!isKnownInteger(node.child1()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node.child2()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
    
        addBranch(m_jit.branch32(cond, arg1PayloadGPR, arg2PayloadGPR), taken);
    
        if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
            addBranch(m_jit.jump(), notTaken);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            m_jit.push(arg2TagGPR);
            m_jit.push(arg2PayloadGPR);
            m_jit.push(arg1TagGPR);
            m_jit.push(arg1PayloadGPR);
            m_jit.push(GPRInfo::callFrameRegister);
            appendCallWithExceptionCheck(helperFunction);
            m_jit.move(GPRInfo::returnValueGPR, resultGPR);
            silentFillAllRegisters(resultGPR);
        
            addBranch(m_jit.branchTest8(callResultCondition, resultGPR), taken);
        }
    }

    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
}

void JITCodeGenerator::nonSpeculativeNonPeepholeCompare(Node& node, MacroAssembler::RelationalCondition cond, Z_DFGOperation_EJJ helperFunction)
{
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node.child1()) || isKnownNotInteger(node.child2())) {
        GPRResult result(this);
        GPRResult2 result2(this);
        GPRReg resultPayloadGPR = result.gpr();
        GPRReg resultTagGPR = result2.gpr();
    
        arg1.use();
        arg2.use();

        flushRegisters();
        
        callOperation(helperFunction, resultPayloadGPR, arg1TagGPR, arg1PayloadGPR, arg2TagGPR, arg2PayloadGPR);
        
        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTagGPR);
        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary resultTag(this, arg1);
        GPRTemporary resultPayload(this, arg1, false);
        GPRReg resultTagGPR = resultTag.gpr();
        GPRReg resultPayloadGPR = resultPayload.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node.child1()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg1TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));
        if (!isKnownInteger(node.child2()))
            slowPath.append(m_jit.branch32(MacroAssembler::NotEqual, arg2TagGPR, JITCompiler::TrustedImm32(JSValue::Int32Tag)));

        m_jit.compare32(cond, arg1PayloadGPR, arg2PayloadGPR, resultPayloadGPR);
    
        if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
            JITCompiler::Jump haveResult = m_jit.jump();
    
            slowPath.link(&m_jit);
        
            silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
            m_jit.push(arg2TagGPR);
            m_jit.push(arg2PayloadGPR);
            m_jit.push(arg1TagGPR);
            m_jit.push(arg1PayloadGPR);
            m_jit.push(GPRInfo::callFrameRegister);
            appendCallWithExceptionCheck(helperFunction);
            m_jit.move(GPRInfo::returnValueGPR, resultPayloadGPR);
            silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
        
            m_jit.andPtr(TrustedImm32(1), resultPayloadGPR);
        
            haveResult.link(&m_jit);
        }
        
        m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTagGPR);
        jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    }
}

void JITCodeGenerator::nonSpeculativePeepholeStrictEq(Node& node, NodeIndex branchNodeIndex, bool invert)
{
    Node& branchNode = at(branchNodeIndex);
    BlockIndex taken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.takenBytecodeOffset());
    BlockIndex notTaken = m_jit.graph().blockIndexForBytecodeOffset(branchNode.notTakenBytecodeOffset());

    // The branch instruction will branch to the taken block.
    // If taken is next, switch taken with notTaken & invert the branch condition so we can fall through.
    if (taken == (m_block + 1)) {
        invert = !invert;
        BlockIndex tmp = taken;
        taken = notTaken;
        notTaken = tmp;
    }
    
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultPayload(this, arg1, false);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node.child1()) && isKnownCell(node.child2())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        addBranch(m_jit.branchPtr(JITCompiler::Equal, arg1PayloadGPR, arg2PayloadGPR), invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultPayloadGPR);
        m_jit.push(arg2TagGPR);
        m_jit.push(arg2PayloadGPR);
        m_jit.push(arg1TagGPR);
        m_jit.push(arg1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationCompareStrictEqCell);
        m_jit.move(GPRInfo::returnValueGPR, resultPayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        addBranch(m_jit.branchTest8(invert ? JITCompiler::NonZero : JITCompiler::Zero, resultPayloadGPR), taken);
    } else {
        // FIXME: Add fast paths for twoCells, number etc.

        silentSpillAllRegisters(resultPayloadGPR);
        m_jit.push(arg2TagGPR);
        m_jit.push(arg2PayloadGPR);
        m_jit.push(arg1TagGPR);
        m_jit.push(arg1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationCompareStrictEq);
        m_jit.move(GPRInfo::returnValueGPR, resultPayloadGPR);
        silentFillAllRegisters(resultPayloadGPR);
        
        addBranch(m_jit.branchTest8(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultPayloadGPR), taken);
    }
    
    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
}

void JITCodeGenerator::nonSpeculativeNonPeepholeStrictEq(Node& node, bool invert)
{
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    GPRReg arg1TagGPR = arg1.tagGPR();
    GPRReg arg1PayloadGPR = arg1.payloadGPR();
    GPRReg arg2TagGPR = arg2.tagGPR();
    GPRReg arg2PayloadGPR = arg2.payloadGPR();
    
    GPRTemporary resultTag(this, arg1);
    GPRTemporary resultPayload(this, arg1, false);
    GPRReg resultTagGPR = resultTag.gpr();
    GPRReg resultPayloadGPR = resultPayload.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node.child1()) && isKnownCell(node.child2())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        JITCompiler::Jump notEqualCase = m_jit.branchPtr(JITCompiler::NotEqual, arg1PayloadGPR, arg2PayloadGPR);
        
        m_jit.move(JITCompiler::TrustedImm32(!invert), resultPayloadGPR);
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
        m_jit.push(arg2TagGPR);
        m_jit.push(arg2PayloadGPR);
        m_jit.push(arg1TagGPR);
        m_jit.push(arg1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationCompareStrictEqCell);
        m_jit.move(GPRInfo::returnValueGPR, resultPayloadGPR);
        silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
        
        done.link(&m_jit);
    } else {
        // FIXME: Add fast paths.

        silentSpillAllRegisters(resultTagGPR, resultPayloadGPR);
        m_jit.push(arg2TagGPR);
        m_jit.push(arg2PayloadGPR);
        m_jit.push(arg1TagGPR);
        m_jit.push(arg1PayloadGPR);
        m_jit.push(GPRInfo::callFrameRegister);
        appendCallWithExceptionCheck(operationCompareStrictEq);
        m_jit.move(GPRInfo::returnValueGPR, resultPayloadGPR);
        silentFillAllRegisters(resultTagGPR, resultPayloadGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultPayloadGPR);
    }

    m_jit.move(TrustedImm32(JSValue::BooleanTag), resultTagGPR);
    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::emitCall(Node& node)
{
    P_DFGOperation_E slowCallFunction;
    bool isCall;

    if (node.op == Call) {
        slowCallFunction = operationLinkCall;
        isCall = true;
    } else {
        ASSERT(node.op == Construct);
        slowCallFunction = operationLinkConstruct;
        isCall = false;
    }

    NodeIndex calleeNodeIndex = m_jit.graph().m_varArgChildren[node.firstChild()];
    JSValueOperand callee(this, calleeNodeIndex);
    GPRReg calleeTagGPR = callee.tagGPR();
    GPRReg calleePayloadGPR = callee.payloadGPR();
    use(calleeNodeIndex);

    // the call instruction's first child is either the function (normal call) or the
    // receiver (method call). subsequent children are the arguments.
    int numArgs = node.numChildren() - 1;

    // For constructors, the this argument is not passed but we have to make space
    // for it.
    int numPassedArgs = numArgs + (isCall ? 0 : 1);

    // amount of stuff (in units of sizeof(Register)) that we need to place at the
    // top of the JS stack.
    int callDataSize = 0;

    // first there are the arguments
    callDataSize += numPassedArgs;

    // and then there is the call frame header
    callDataSize += RegisterFile::CallFrameHeaderSize;

    m_jit.store32(MacroAssembler::TrustedImm32(numPassedArgs), payloadOfCallData(RegisterFile::ArgumentCount));
    m_jit.store32(MacroAssembler::TrustedImm32(JSValue::Int32Tag), tagOfCallData(RegisterFile::ArgumentCount));
    m_jit.storePtr(GPRInfo::callFrameRegister, payloadOfCallData(RegisterFile::CallerFrame));
    m_jit.store32(MacroAssembler::TrustedImm32(JSValue::CellTag), tagOfCallData(RegisterFile::CallerFrame));

    for (int argIdx = 0; argIdx < numArgs; argIdx++) {
        NodeIndex argNodeIndex = m_jit.graph().m_varArgChildren[node.firstChild() + 1 + argIdx];
        JSValueOperand arg(this, argNodeIndex);
        GPRReg argTagGPR = arg.tagGPR();
        GPRReg argPayloadGPR = arg.payloadGPR();
        use(argNodeIndex);

        m_jit.store32(argTagGPR, tagOfCallData(-callDataSize + argIdx + (isCall ? 0 : 1)));
        m_jit.store32(argPayloadGPR, payloadOfCallData(-callDataSize + argIdx + (isCall ? 0 : 1)));
    }

    m_jit.store32(calleeTagGPR, tagOfCallData(RegisterFile::Callee));
    m_jit.store32(calleePayloadGPR, payloadOfCallData(RegisterFile::Callee));

    flushRegisters();

    GPRResult resultPayload(this);
    GPRResult2 resultTag(this);
    GPRReg resultPayloadGPR = resultPayload.gpr();
    GPRReg resultTagGPR = resultTag.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::Jump slowPath;

    slowPath = m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleePayloadGPR, targetToCheck);
    m_jit.loadPtr(MacroAssembler::Address(calleePayloadGPR, OBJECT_OFFSETOF(JSFunction, m_scopeChain)), resultPayloadGPR);
    m_jit.storePtr(resultPayloadGPR, payloadOfCallData(RegisterFile::ScopeChain));
    m_jit.store32(MacroAssembler::TrustedImm32(JSValue::CellTag), tagOfCallData(RegisterFile::ScopeChain));

    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);

    JITCompiler::Call fastCall = m_jit.nearCall();
    m_jit.notifyCall(fastCall, at(m_compileIndex).codeOrigin);

    JITCompiler::Jump done = m_jit.jump();

    slowPath.link(&m_jit);

    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    m_jit.push(GPRInfo::argumentGPR0);
    JITCompiler::Call slowCall = m_jit.appendCallWithFastExceptionCheck(slowCallFunction, at(m_compileIndex).codeOrigin);
    m_jit.move(Imm32(numPassedArgs), GPRInfo::regT1);
    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    m_jit.notifyCall(m_jit.call(GPRInfo::returnValueGPR), at(m_compileIndex).codeOrigin);

    done.link(&m_jit);

    setupResults(resultTagGPR, resultPayloadGPR);

    jsValueResult(resultTagGPR, resultPayloadGPR, m_compileIndex, DataFormatJS, UseChildrenCalledExplicitly);

    m_jit.addJSCall(fastCall, slowCall, targetToCheck, isCall, at(m_compileIndex).codeOrigin);
}

#endif

} } // namespace JSC::DFG

#endif
