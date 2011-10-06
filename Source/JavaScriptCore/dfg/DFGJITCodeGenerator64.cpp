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

#if USE(JSVALUE64)

GPRReg JITCodeGenerator::fillInteger(NodeIndex nodeIndex, DataFormat& returnFormat)
{
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    if (info.registerFormat() == DataFormatNone) {
        GPRReg gpr = allocate();

        if (node.hasConstant()) {
            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
            if (isInt32Constant(nodeIndex)) {
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                info.fillInteger(gpr);
                returnFormat = DataFormatInteger;
                return gpr;
            }
            if (isNumberConstant(nodeIndex)) {
                JSValue jsValue = jsNumber(valueOfNumberConstant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            }
        } else {
            ASSERT(info.spillFormat() == DataFormatJS || info.spillFormat() == DataFormatJSInteger);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
        }

        // Since we statically know that we're filling an integer, and values
        // in the RegisterFile are boxed, this must be DataFormatJSInteger.
        // We will check this with a jitAssert below.
        info.fillJSValue(gpr, DataFormatJSInteger);
        unlock(gpr);
    }

    switch (info.registerFormat()) {
    case DataFormatNone:
        // Should have filled, above.
    case DataFormatJSDouble:
    case DataFormatDouble:
    case DataFormatJS:
    case DataFormatCell:
    case DataFormatJSCell:
    case DataFormatBoolean:
    case DataFormatJSBoolean:
    case DataFormatStorage:
        // Should only be calling this function if we know this operand to be integer.
        ASSERT_NOT_REACHED();

    case DataFormatJSInteger: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.jitAssertIsJSInt32(gpr);
        returnFormat = DataFormatJSInteger;
        return gpr;
    }

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
        GPRReg gpr = allocate();

        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex)) {
                // FIXME: should not be reachable?
                m_jit.move(MacroAssembler::Imm32(valueOfInt32Constant(nodeIndex)), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillInteger(gpr);
                unlock(gpr);
            } else if (isNumberConstant(nodeIndex)) {
                FPRReg fpr = fprAllocate();
                m_jit.move(MacroAssembler::ImmPtr(reinterpret_cast<void*>(reinterpretDoubleToIntptr(valueOfNumberConstant(nodeIndex)))), gpr);
                m_jit.movePtrToDouble(gpr, fpr);
                unlock(gpr);

                m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
                info.fillDouble(fpr);
                return fpr;
            } else {
                // FIXME: should not be reachable?
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
                m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
                info.fillJSValue(gpr, DataFormatJS);
                unlock(gpr);
            }
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(gpr, spillFormat);
            unlock(gpr);
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

    case DataFormatJS: {
        GPRReg jsValueGpr = info.gpr();
        m_gprs.lock(jsValueGpr);
        FPRReg fpr = fprAllocate();
        GPRReg tempGpr = allocate(); // FIXME: can we skip this allocation on the last use of the virtual register?

        JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);

        m_jit.jitAssertIsJSDouble(jsValueGpr);

        // First, if we get here we have a double encoded as a JSValue
        m_jit.move(jsValueGpr, tempGpr);
        unboxDouble(tempGpr, fpr);
        JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

        // Finally, handle integers.
        isInteger.link(&m_jit);
        m_jit.convertInt32ToDouble(jsValueGpr, fpr);
        hasUnboxedDouble.link(&m_jit);

        m_gprs.release(jsValueGpr);
        m_gprs.unlock(jsValueGpr);
        m_gprs.unlock(tempGpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);
        info.fillDouble(fpr);
        info.killSpilled();
        return fpr;
    }

    case DataFormatJSInteger:
    case DataFormatInteger: {
        FPRReg fpr = fprAllocate();
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        m_jit.convertInt32ToDouble(gpr, fpr);
        m_gprs.unlock(gpr);
        return fpr;
    }

    // Unbox the double
    case DataFormatJSDouble: {
        GPRReg gpr = info.gpr();
        FPRReg fpr = fprAllocate();
        if (m_gprs.isLocked(gpr)) {
            // Make sure we don't trample gpr if it is in use.
            GPRReg temp = allocate();
            m_jit.move(gpr, temp);
            unboxDouble(temp, fpr);
            unlock(temp);
        } else
            unboxDouble(gpr, fpr);

        m_gprs.release(gpr);
        m_fprs.retain(fpr, virtualRegister, SpillOrderDouble);

        info.fillDouble(fpr);
        return fpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        m_fprs.lock(fpr);
        return fpr;
    }
    }

    ASSERT_NOT_REACHED();
    return InvalidFPRReg;
}

GPRReg JITCodeGenerator::fillJSValue(NodeIndex nodeIndex)
{
    Node& node = at(nodeIndex);
    VirtualRegister virtualRegister = node.virtualRegister();
    GenerationInfo& info = m_generationInfo[virtualRegister];

    switch (info.registerFormat()) {
    case DataFormatNone: {
        GPRReg gpr = allocate();

        if (node.hasConstant()) {
            if (isInt32Constant(nodeIndex)) {
                info.fillJSValue(gpr, DataFormatJSInteger);
                JSValue jsValue = jsNumber(valueOfInt32Constant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else if (isNumberConstant(nodeIndex)) {
                info.fillJSValue(gpr, DataFormatJSDouble);
                JSValue jsValue(JSValue::EncodeAsDouble, valueOfNumberConstant(nodeIndex));
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
            } else {
                ASSERT(isJSConstant(nodeIndex));
                JSValue jsValue = valueOfJSConstant(nodeIndex);
                m_jit.move(MacroAssembler::ImmPtr(JSValue::encode(jsValue)), gpr);
                info.fillJSValue(gpr, DataFormatJS);
            }

            m_gprs.retain(gpr, virtualRegister, SpillOrderConstant);
        } else {
            DataFormat spillFormat = info.spillFormat();
            ASSERT(spillFormat & DataFormatJS);
            m_gprs.retain(gpr, virtualRegister, SpillOrderSpilled);
            m_jit.loadPtr(JITCompiler::addressFor(virtualRegister), gpr);
            info.fillJSValue(gpr, spillFormat);
        }
        return gpr;
    }

    case DataFormatInteger: {
        GPRReg gpr = info.gpr();
        // If the register has already been locked we need to take a copy.
        // If not, we'll zero extend in place, so mark on the info that this is now type DataFormatInteger, not DataFormatJSInteger.
        if (m_gprs.isLocked(gpr)) {
            GPRReg result = allocate();
            m_jit.orPtr(GPRInfo::tagTypeNumberRegister, gpr, result);
            return result;
        }
        m_gprs.lock(gpr);
        m_jit.orPtr(GPRInfo::tagTypeNumberRegister, gpr);
        info.fillJSValue(gpr, DataFormatJSInteger);
        return gpr;
    }

    case DataFormatDouble: {
        FPRReg fpr = info.fpr();
        GPRReg gpr = boxDouble(fpr);

        // Update all info
        info.fillJSValue(gpr, DataFormatJSDouble);
        m_fprs.release(fpr);
        m_gprs.retain(gpr, virtualRegister, SpillOrderJS);

        return gpr;
    }

    case DataFormatCell:
        // No retag required on JSVALUE64!
    case DataFormatJS:
    case DataFormatJSInteger:
    case DataFormatJSDouble:
    case DataFormatJSCell:
    case DataFormatJSBoolean: {
        GPRReg gpr = info.gpr();
        m_gprs.lock(gpr);
        return gpr;
    }
        
    case DataFormatBoolean:
    case DataFormatStorage:
        // this type currently never occurs
        ASSERT_NOT_REACHED();
    }

    ASSERT_NOT_REACHED();
    return InvalidGPRReg;
}

void JITCodeGenerator::nonSpeculativeValueToNumber(Node& node)
{
    if (isKnownNumeric(node.child1())) {
        JSValueOperand op1(this, node.child1());
        GPRTemporary result(this, op1);
        m_jit.move(op1.gpr(), result.gpr());
        jsValueResult(result.gpr(), m_compileIndex);
        return;
    }

    JSValueOperand op1(this, node.child1());
    GPRTemporary result(this);
    
    ASSERT(!isInt32Constant(node.child1()));
    ASSERT(!isNumberConstant(node.child1()));
    
    GPRReg jsValueGpr = op1.gpr();
    GPRReg gpr = result.gpr();
    op1.use();

    JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);
    JITCompiler::Jump nonNumeric = m_jit.branchTestPtr(MacroAssembler::Zero, jsValueGpr, GPRInfo::tagTypeNumberRegister);

    // First, if we get here we have a double encoded as a JSValue
    m_jit.move(jsValueGpr, gpr);
    JITCompiler::Jump hasUnboxedDouble = m_jit.jump();

    // Next handle cells (& other JS immediates)
    nonNumeric.link(&m_jit);
    silentSpillAllRegisters(gpr);
    m_jit.move(jsValueGpr, GPRInfo::argumentGPR1);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    appendCallWithExceptionCheck(dfgConvertJSValueToNumber);
    boxDouble(FPRInfo::returnValueFPR, gpr);
    silentFillAllRegisters(gpr);
    JITCompiler::Jump hasCalledToNumber = m_jit.jump();
    
    // Finally, handle integers.
    isInteger.link(&m_jit);
    m_jit.orPtr(GPRInfo::tagTypeNumberRegister, jsValueGpr, gpr);
    hasUnboxedDouble.link(&m_jit);
    hasCalledToNumber.link(&m_jit);
    
    jsValueResult(result.gpr(), m_compileIndex, UseChildrenCalledExplicitly);
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
        
        m_jit.moveDouble(fpr, FPRInfo::argumentFPR0);
        appendCallWithExceptionCheck(toInt32);
        m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, gpr);
        
        silentFillAllRegisters(gpr);
        
        truncatedToInteger.link(&m_jit);
        integerResult(gpr, m_compileIndex, UseChildrenCalledExplicitly);
        return;
    }
    
    JSValueOperand op1(this, node.child1());
    GPRTemporary result(this, op1);
    GPRReg jsValueGpr = op1.gpr();
    GPRReg resultGPR = result.gpr();
    op1.use();

    JITCompiler::Jump isInteger = m_jit.branchPtr(MacroAssembler::AboveOrEqual, jsValueGpr, GPRInfo::tagTypeNumberRegister);

    // First handle non-integers
    silentSpillAllRegisters(resultGPR);
    m_jit.move(jsValueGpr, GPRInfo::argumentGPR1);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    appendCallWithExceptionCheck(dfgConvertJSValueToInt32);
    m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, resultGPR);
    silentFillAllRegisters(resultGPR);
    JITCompiler::Jump hasCalledToInt32 = m_jit.jump();

    // Then handle integers.
    isInteger.link(&m_jit);
    m_jit.zeroExtend32ToPtr(jsValueGpr, resultGPR);
    hasCalledToInt32.link(&m_jit);
    integerResult(resultGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeUInt32ToNumber(Node& node)
{
    IntegerOperand op1(this, node.child1());
    FPRTemporary boxer(this);
    GPRTemporary result(this, op1);
    
    JITCompiler::Jump positive = m_jit.branch32(MacroAssembler::GreaterThanOrEqual, op1.gpr(), TrustedImm32(0));
    
    m_jit.convertInt32ToDouble(op1.gpr(), boxer.fpr());
    m_jit.addDouble(JITCompiler::AbsoluteAddress(&twoToThe32), boxer.fpr());
    
    boxDouble(boxer.fpr(), result.gpr());
    
    JITCompiler::Jump done = m_jit.jump();
    
    positive.link(&m_jit);
    
    m_jit.orPtr(GPRInfo::tagTypeNumberRegister, op1.gpr(), result.gpr());
    
    done.link(&m_jit);
    
    jsValueResult(result.gpr(), m_compileIndex);
}

void JITCodeGenerator::nonSpeculativeKnownConstantArithOp(NodeType op, NodeIndex regChild, NodeIndex immChild, bool commute)
{
    JSValueOperand regArg(this, regChild);
    GPRReg regArgGPR = regArg.gpr();
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    FPRTemporary tmp1(this);
    FPRTemporary tmp2(this);
    FPRReg tmp1FPR = tmp1.fpr();
    FPRReg tmp2FPR = tmp2.fpr();
    
    regArg.use();
    use(immChild);

    JITCompiler::Jump notInt;
    
    int32_t imm = valueOfInt32Constant(immChild);
        
    if (!isKnownInteger(regChild))
        notInt = m_jit.branchPtr(MacroAssembler::Below, regArgGPR, GPRInfo::tagTypeNumberRegister);
    
    JITCompiler::Jump overflow;
    
    switch (op) {
    case ValueAdd:
    case ArithAdd:
        overflow = m_jit.branchAdd32(MacroAssembler::Overflow, regArgGPR, Imm32(imm), resultGPR);
        break;
        
    case ArithSub:
        overflow = m_jit.branchSub32(MacroAssembler::Overflow, regArgGPR, Imm32(imm), resultGPR);
        break;
        
    default:
        ASSERT_NOT_REACHED();
    }
    
    m_jit.orPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        
    JITCompiler::Jump done = m_jit.jump();
    
    overflow.link(&m_jit);
    
    JITCompiler::Jump notNumber;
    
    // first deal with overflow case
    m_jit.convertInt32ToDouble(regArgGPR, tmp2FPR);
    
    // now deal with not-int case, if applicable
    if (!isKnownInteger(regChild)) {
        JITCompiler::Jump haveValue = m_jit.jump();
        
        notInt.link(&m_jit);
        
        if (!isKnownNumeric(regChild)) {
            ASSERT(op == ValueAdd);
            notNumber = m_jit.branchTestPtr(MacroAssembler::Zero, regArgGPR, GPRInfo::tagTypeNumberRegister);
        }
        
        m_jit.move(regArgGPR, resultGPR);
        m_jit.addPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        m_jit.movePtrToDouble(resultGPR, tmp2FPR);
        
        haveValue.link(&m_jit);
    }
    
    m_jit.move(MacroAssembler::ImmPtr(reinterpret_cast<void*>(reinterpretDoubleToIntptr(valueOfNumberConstant(immChild)))), resultGPR);
    m_jit.movePtrToDouble(resultGPR, tmp1FPR);
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
        m_jit.branchConvertDoubleToInt32(tmp2FPR, resultGPR, failureCases, tmp1FPR);
        m_jit.orPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        
        doneCaseConvertedToInt = m_jit.jump();
        
        failureCases.link(&m_jit);
    }
    
    m_jit.moveDoubleToPtr(tmp2FPR, resultGPR);
    m_jit.subPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        
    if (!isKnownNumeric(regChild)) {
        ASSERT(notNumber.isSet());
        ASSERT(op == ValueAdd);
            
        JITCompiler::Jump doneCaseWasNumber = m_jit.jump();
            
        notNumber.link(&m_jit);
            
        silentSpillAllRegisters(resultGPR);
        if (commute) {
            m_jit.move(regArgGPR, GPRInfo::argumentGPR2);
            m_jit.move(MacroAssembler::ImmPtr(static_cast<const void*>(JSValue::encode(jsNumber(imm)))), GPRInfo::argumentGPR1);
        } else {
            m_jit.move(regArgGPR, GPRInfo::argumentGPR1);
            m_jit.move(MacroAssembler::ImmPtr(static_cast<const void*>(JSValue::encode(jsNumber(imm)))), GPRInfo::argumentGPR2);
        }
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationValueAddNotNumber);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);
            
        doneCaseWasNumber.link(&m_jit);
    }
    
    done.link(&m_jit);
    if (doneCaseConvertedToInt.isSet())
        doneCaseConvertedToInt.link(&m_jit);
        
    jsValueResult(resultGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeBasicArithOp(NodeType op, Node &node)
{
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    
    FPRTemporary tmp1(this);
    FPRTemporary tmp2(this);
    FPRReg tmp1FPR = tmp1.fpr();
    FPRReg tmp2FPR = tmp2.fpr();
    
    GPRTemporary result(this);

    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    JITCompiler::Jump child1NotInt;
    JITCompiler::Jump child2NotInt;
    JITCompiler::JumpList overflow;
    
    if (!isKnownInteger(node.child1()))
        child1NotInt = m_jit.branchPtr(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister);
    if (!isKnownInteger(node.child2()))
        child2NotInt = m_jit.branchPtr(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister);
    
    switch (op) {
    case ValueAdd:
    case ArithAdd: {
        overflow.append(m_jit.branchAdd32(MacroAssembler::Overflow, arg1GPR, arg2GPR, resultGPR));
        break;
    }
        
    case ArithSub: {
        overflow.append(m_jit.branchSub32(MacroAssembler::Overflow, arg1GPR, arg2GPR, resultGPR));
        break;
    }
        
    case ArithMul: {
        overflow.append(m_jit.branchMul32(MacroAssembler::Overflow, arg1GPR, arg2GPR, resultGPR));
        overflow.append(m_jit.branchTest32(MacroAssembler::Zero, resultGPR));
        break;
    }
        
    default:
        ASSERT_NOT_REACHED();
    }
    
    m_jit.orPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        
    JITCompiler::Jump done = m_jit.jump();
    
    JITCompiler::JumpList haveFPRArguments;

    overflow.link(&m_jit);
        
    // both arguments are integers
    m_jit.convertInt32ToDouble(arg1GPR, tmp1FPR);
    m_jit.convertInt32ToDouble(arg2GPR, tmp2FPR);
        
    haveFPRArguments.append(m_jit.jump());
        
    JITCompiler::JumpList notNumbers;
        
    JITCompiler::Jump child2NotInt2;
        
    if (!isKnownInteger(node.child1())) {
        child1NotInt.link(&m_jit);
            
        if (!isKnownNumeric(node.child1())) {
            ASSERT(op == ValueAdd);
            notNumbers.append(m_jit.branchTestPtr(MacroAssembler::Zero, arg1GPR, GPRInfo::tagTypeNumberRegister));
        }
            
        m_jit.move(arg1GPR, resultGPR);
        unboxDouble(resultGPR, tmp1FPR);
            
        // child1 is converted to a double; child2 may either be an int or
        // a boxed double
            
        if (!isKnownInteger(node.child2())) {
            if (isKnownNumeric(node.child2()))
                child2NotInt2 = m_jit.branchPtr(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister);
            else {
                ASSERT(op == ValueAdd);
                JITCompiler::Jump child2IsInt = m_jit.branchPtr(MacroAssembler::AboveOrEqual, arg2GPR, GPRInfo::tagTypeNumberRegister);
                notNumbers.append(m_jit.branchTestPtr(MacroAssembler::Zero, arg2GPR, GPRInfo::tagTypeNumberRegister));
                child2NotInt2 = m_jit.jump();
                child2IsInt.link(&m_jit);
            }
        }
            
        // child 2 is definitely an integer
        m_jit.convertInt32ToDouble(arg2GPR, tmp2FPR);
            
        haveFPRArguments.append(m_jit.jump());
    }
        
    if (!isKnownInteger(node.child2())) {
        child2NotInt.link(&m_jit);
            
        if (!isKnownNumeric(node.child2())) {
            ASSERT(op == ValueAdd);
            notNumbers.append(m_jit.branchTestPtr(MacroAssembler::Zero, arg2GPR, GPRInfo::tagTypeNumberRegister));
        }
            
        // child1 is definitely an integer, and child 2 is definitely not
            
        m_jit.convertInt32ToDouble(arg1GPR, tmp1FPR);
            
        if (child2NotInt2.isSet())
            child2NotInt2.link(&m_jit);
            
        m_jit.move(arg2GPR, resultGPR);
        unboxDouble(resultGPR, tmp2FPR);
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
        m_jit.branchConvertDoubleToInt32(tmp1FPR, resultGPR, failureCases, tmp2FPR);
        m_jit.orPtr(GPRInfo::tagTypeNumberRegister, resultGPR);
        
        doneCaseConvertedToInt = m_jit.jump();
        
        failureCases.link(&m_jit);
    }
        
    boxDouble(tmp1FPR, resultGPR);
        
    if (!notNumbers.empty()) {
        ASSERT(op == ValueAdd);
            
        JITCompiler::Jump doneCaseWasNumber = m_jit.jump();
            
        notNumbers.link(&m_jit);
            
        silentSpillAllRegisters(resultGPR);
        setupStubArguments(arg1GPR, arg2GPR);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationValueAddNotNumber);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);

        doneCaseWasNumber.link(&m_jit);
    }
    
    done.link(&m_jit);
    if (doneCaseConvertedToInt.isSet())
        doneCaseConvertedToInt.link(&m_jit);
        
    jsValueResult(resultGPR, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeArithMod(Node& node)
{
    JSValueOperand op1(this, node.child1());
    JSValueOperand op2(this, node.child2());
    GPRTemporary eax(this, X86Registers::eax);
    GPRTemporary edx(this, X86Registers::edx);

    FPRTemporary op1Double(this);
    FPRTemporary op2Double(this);
    
    GPRReg op1GPR = op1.gpr();
    GPRReg op2GPR = op2.gpr();
    
    FPRReg op1FPR = op1Double.fpr();
    FPRReg op2FPR = op2Double.fpr();
        
    op1.use();
    op2.use();
    
    GPRReg temp2 = InvalidGPRReg;
    GPRReg unboxGPR;
    if (op2GPR == X86Registers::eax || op2GPR == X86Registers::edx) {
        temp2 = allocate();
        m_jit.move(op2GPR, temp2);
        op2GPR = temp2;
        unboxGPR = temp2;
    } else if (op1GPR == X86Registers::eax)
        unboxGPR = X86Registers::edx;
    else
        unboxGPR = X86Registers::eax;
    ASSERT(unboxGPR != op1.gpr());
    ASSERT(unboxGPR != op2.gpr());

    JITCompiler::Jump firstOpNotInt;
    JITCompiler::Jump secondOpNotInt;
    JITCompiler::JumpList done;
    JITCompiler::Jump modByZero;
    
    if (!isKnownInteger(node.child1()))
        firstOpNotInt = m_jit.branchPtr(MacroAssembler::Below, op1GPR, GPRInfo::tagTypeNumberRegister);
    if (!isKnownInteger(node.child2()))
        secondOpNotInt = m_jit.branchPtr(MacroAssembler::Below, op2GPR, GPRInfo::tagTypeNumberRegister);
    
    modByZero = m_jit.branchTest32(MacroAssembler::Zero, op2GPR);
    
    m_jit.move(op1GPR, eax.gpr());
    m_jit.assembler().cdq();
    m_jit.assembler().idivl_r(op2GPR);
    
    m_jit.orPtr(GPRInfo::tagTypeNumberRegister, X86Registers::edx);
    
    done.append(m_jit.jump());
        
    JITCompiler::Jump gotDoubleArgs;
    
    modByZero.link(&m_jit);
        
    m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsNumber(std::numeric_limits<double>::quiet_NaN()))), X86Registers::edx);
    done.append(m_jit.jump());
    
    if (!isKnownInteger(node.child1())) {
        firstOpNotInt.link(&m_jit);
        
        JITCompiler::Jump secondOpNotInt2;
        
        if (!isKnownInteger(node.child2()))
            secondOpNotInt2 = m_jit.branchPtr(MacroAssembler::Below, op2GPR, GPRInfo::tagTypeNumberRegister);
            
        // first op is a double, second op is an int.
        m_jit.convertInt32ToDouble(op2GPR, op2FPR);

        if (!isKnownInteger(node.child2())) {
            JITCompiler::Jump gotSecondOp = m_jit.jump();
            
            secondOpNotInt2.link(&m_jit);
            
            // first op is a double, second op is a double.
            m_jit.move(op2GPR, unboxGPR);
            unboxDouble(unboxGPR, op2FPR);
            
            gotSecondOp.link(&m_jit);
        }
        
        m_jit.move(op1GPR, unboxGPR);
        unboxDouble(unboxGPR, op1FPR);
        
        gotDoubleArgs = m_jit.jump();
    }
    
    if (!isKnownInteger(node.child2())) {
        secondOpNotInt.link(&m_jit);
        
        // we know that the first op is an int, and the second is a double
        m_jit.convertInt32ToDouble(op1GPR, op1FPR);
        m_jit.move(op2GPR, unboxGPR);
        unboxDouble(unboxGPR, op2FPR);
    }
    
    if (!isKnownInteger(node.child1()))
        gotDoubleArgs.link(&m_jit);
    
    if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
        silentSpillAllRegisters(X86Registers::edx);
        setupTwoStubArgs<FPRInfo::argumentFPR0, FPRInfo::argumentFPR1>(op1FPR, op2FPR);
        m_jit.appendCall(fmod);
        boxDouble(FPRInfo::returnValueFPR, X86Registers::edx);
        silentFillAllRegisters(X86Registers::edx);
    }
        
    done.link(&m_jit);
    
    if (temp2 != InvalidGPRReg)
        unlock(temp2);
    
    jsValueResult(X86Registers::edx, m_compileIndex, UseChildrenCalledExplicitly);
}

void JITCodeGenerator::nonSpeculativeCheckHasInstance(Node& node)
{
    JSValueOperand base(this, node.child1());
    GPRTemporary structure(this);
    
    GPRReg baseReg = base.gpr();
    GPRReg structureReg = structure.gpr();
    
    // Check that base is a cell.
    MacroAssembler::Jump baseNotCell = m_jit.branchTestPtr(MacroAssembler::NonZero, baseReg, GPRInfo::tagMaskRegister);
    
    // Check that base 'ImplementsHasInstance'.
    m_jit.loadPtr(MacroAssembler::Address(baseReg, JSCell::structureOffset()), structureReg);
    MacroAssembler::Jump implementsHasInstance = m_jit.branchTest8(MacroAssembler::NonZero, MacroAssembler::Address(structureReg, Structure::typeInfoFlagsOffset()), MacroAssembler::TrustedImm32(ImplementsHasInstance));
    
    // At this point we always throw, so no need to preserve registers.
    baseNotCell.link(&m_jit);
    m_jit.move(baseReg, GPRInfo::argumentGPR1);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    // At some point we could optimize this to plant a direct jump, rather then checking
    // for an exception (operationThrowHasInstanceError always throws). Probably not worth
    // adding the extra interface to do this now, but we may also want this for op_throw.
    appendCallWithExceptionCheck(operationThrowHasInstanceError);
    
    implementsHasInstance.link(&m_jit);
    noResult(m_compileIndex);
}

void JITCodeGenerator::nonSpeculativeInstanceOf(Node& node)
{
    JSValueOperand value(this, node.child1());
    JSValueOperand base(this, node.child2());
    JSValueOperand prototype(this, node.child3());
    GPRTemporary scratch(this, base);

    GPRReg valueReg = value.gpr();
    GPRReg baseReg = base.gpr();
    GPRReg prototypeReg = prototype.gpr();
    GPRReg scratchReg = scratch.gpr();
        
    value.use();
    base.use();
    prototype.use();

    // Check that operands are cells (base is checked by CheckHasInstance, so we can just assert).
    MacroAssembler::Jump valueNotCell = m_jit.branchTestPtr(MacroAssembler::NonZero, valueReg, GPRInfo::tagMaskRegister);
    m_jit.jitAssertIsCell(baseReg);
    MacroAssembler::Jump prototypeNotCell = m_jit.branchTestPtr(MacroAssembler::NonZero, prototypeReg, GPRInfo::tagMaskRegister);

    // Check that baseVal 'ImplementsDefaultHasInstance'.
    m_jit.loadPtr(MacroAssembler::Address(baseReg, JSCell::structureOffset()), scratchReg);
    MacroAssembler::Jump notDefaultHasInstance = m_jit.branchTest8(MacroAssembler::Zero, MacroAssembler::Address(scratchReg, Structure::typeInfoFlagsOffset()), TrustedImm32(ImplementsDefaultHasInstance));

    // Check that prototype is an object
    m_jit.loadPtr(MacroAssembler::Address(prototypeReg, JSCell::structureOffset()), scratchReg);
    MacroAssembler::Jump protoNotObject = m_jit.branchIfNotObject(scratchReg);

    // Initialize scratchReg with the value being checked.
    m_jit.move(valueReg, scratchReg);

    // Walk up the prototype chain of the value (in scratchReg), comparing to prototypeReg.
    MacroAssembler::Label loop(&m_jit);
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, JSCell::structureOffset()), scratchReg);
    m_jit.loadPtr(MacroAssembler::Address(scratchReg, Structure::prototypeOffset()), scratchReg);
    MacroAssembler::Jump isInstance = m_jit.branchPtr(MacroAssembler::Equal, scratchReg, prototypeReg);
    m_jit.branchTestPtr(MacroAssembler::Zero, scratchReg, GPRInfo::tagMaskRegister).linkTo(loop, &m_jit);

    // No match - result is false.
    m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsBoolean(false))), scratchReg);
    MacroAssembler::Jump wasNotInstance = m_jit.jump();

    // Link to here if any checks fail that require us to try calling out to an operation to help,
    // e.g. for an API overridden HasInstance.
    valueNotCell.link(&m_jit);
    prototypeNotCell.link(&m_jit);
    notDefaultHasInstance.link(&m_jit);
    protoNotObject.link(&m_jit);

    silentSpillAllRegisters(scratchReg);
    setupStubArguments(valueReg, baseReg, prototypeReg);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    appendCallWithExceptionCheck(operationInstanceOf);
    m_jit.move(GPRInfo::returnValueGPR, scratchReg);
    silentFillAllRegisters(scratchReg);

    MacroAssembler::Jump wasNotDefaultHasInstance = m_jit.jump();

    isInstance.link(&m_jit);
    m_jit.move(MacroAssembler::TrustedImmPtr(JSValue::encode(jsBoolean(true))), scratchReg);

    wasNotInstance.link(&m_jit);
    wasNotDefaultHasInstance.link(&m_jit);
    jsValueResult(scratchReg, m_compileIndex, UseChildrenCalledExplicitly);
}

JITCompiler::Call JITCodeGenerator::cachedGetById(GPRReg baseGPR, GPRReg resultGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget, NodeType nodeType)
{
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), resultGPR);
    JITCompiler::DataLabelCompact loadWithPatch = m_jit.loadPtrWithCompactAddressOffsetPatch(JITCompiler::Address(resultGPR, 0), resultGPR);
    
    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);
    
    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);
    
    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(resultGPR);
    m_jit.move(baseGPR, GPRInfo::argumentGPR1);
    m_jit.move(JITCompiler::ImmPtr(identifier(identifierNumber)), GPRInfo::argumentGPR2);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
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
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    silentFillAllRegisters(resultGPR);
    
    done.link(&m_jit);
    
    JITCompiler::Label doneLabel = m_jit.label();

    int16_t checkImmToCall = safeCast<int16_t>(m_jit.differenceBetween(structureToCompare, functionCall));
    int16_t callToCheck = safeCast<int16_t>(m_jit.differenceBetween(functionCall, structureCheck));
    int16_t callToLoad = safeCast<int16_t>(m_jit.differenceBetween(functionCall, loadWithPatch));
    int16_t callToSlowCase = safeCast<int16_t>(m_jit.differenceBetween(functionCall, slowCase));
    int16_t callToDone = safeCast<int16_t>(m_jit.differenceBetween(functionCall, doneLabel));
    
    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToLoad, callToSlowCase, callToDone, safeCast<int8_t>(baseGPR), safeCast<int8_t>(resultGPR), safeCast<int8_t>(scratchGPR));
    
    if (scratchGPR != resultGPR && scratchGPR != InvalidGPRReg)
        unlock(scratchGPR);
    
    return functionCall;
}

void JITCodeGenerator::cachedPutById(GPRReg baseGPR, GPRReg valueGPR, NodeIndex valueIndex, GPRReg scratchGPR, unsigned identifierNumber, PutKind putKind, JITCompiler::Jump slowPathTarget)
{
    
    JITCompiler::DataLabelPtr structureToCompare;
    JITCompiler::Jump structureCheck = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structureToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));

    writeBarrier(baseGPR, valueGPR, valueIndex, WriteBarrierForPropertyAccess, scratchGPR);

    m_jit.loadPtr(JITCompiler::Address(baseGPR, JSObject::offsetOfPropertyStorage()), scratchGPR);
    JITCompiler::DataLabel32 storeWithPatch = m_jit.storePtrWithAddressOffsetPatch(valueGPR, JITCompiler::Address(scratchGPR, 0));

    JITCompiler::Jump done = m_jit.jump();

    structureCheck.link(&m_jit);

    if (slowPathTarget.isSet())
        slowPathTarget.link(&m_jit);

    JITCompiler::Label slowCase = m_jit.label();

    silentSpillAllRegisters(InvalidGPRReg);
    setupTwoStubArgs<GPRInfo::argumentGPR1, GPRInfo::argumentGPR2>(valueGPR, baseGPR);
    m_jit.move(JITCompiler::ImmPtr(identifier(identifierNumber)), GPRInfo::argumentGPR3);
    m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
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
    int16_t callToStore = safeCast<int16_t>(m_jit.differenceBetween(functionCall, storeWithPatch));
    int16_t callToSlowCase = safeCast<int16_t>(m_jit.differenceBetween(functionCall, slowCase));
    int16_t callToDone = safeCast<int16_t>(m_jit.differenceBetween(functionCall, doneLabel));

    m_jit.addPropertyAccess(functionCall, checkImmToCall, callToCheck, callToStore, callToSlowCase, callToDone, safeCast<int8_t>(baseGPR), safeCast<int8_t>(valueGPR), safeCast<int8_t>(scratchGPR));
}

void JITCodeGenerator::cachedGetMethod(GPRReg baseGPR, GPRReg resultGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget)
{
    JITCompiler::Call slowCall;
    JITCompiler::DataLabelPtr structToCompare, protoObj, protoStructToCompare, putFunction;
    
    JITCompiler::Jump wrongStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(baseGPR, JSCell::structureOffset()), structToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    protoObj = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultGPR);
    JITCompiler::Jump wrongProtoStructure = m_jit.branchPtrWithPatch(JITCompiler::NotEqual, JITCompiler::Address(resultGPR, JSCell::structureOffset()), protoStructToCompare, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(-1)));
    
    putFunction = m_jit.moveWithPatch(JITCompiler::TrustedImmPtr(0), resultGPR);
    
    JITCompiler::Jump done = m_jit.jump();
    
    wrongStructure.link(&m_jit);
    wrongProtoStructure.link(&m_jit);
    
    slowCall = cachedGetById(baseGPR, resultGPR, scratchGPR, identifierNumber, slowPathTarget, GetMethod);
    
    done.link(&m_jit);
    
    m_jit.addMethodGet(slowCall, structToCompare, protoObj, protoStructToCompare, putFunction);
}

void JITCodeGenerator::nonSpeculativeNonPeepholeCompareNull(NodeIndex operand, bool invert)
{
    JSValueOperand arg(this, operand);
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    if (!isKnownCell(operand))
        notCell = m_jit.branchTestPtr(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);
    
    m_jit.loadPtr(JITCompiler::Address(argGPR, JSCell::structureOffset()), resultGPR);
    m_jit.test8(invert ? JITCompiler::Zero : JITCompiler::NonZero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined), resultGPR);
    
    if (!isKnownCell(operand)) {
        JITCompiler::Jump done = m_jit.jump();
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.andPtr(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        m_jit.comparePtr(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImm32(ValueNull), resultGPR);
        
        done.link(&m_jit);
    }
    
    m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
    jsValueResult(resultGPR, m_compileIndex, DataFormatJSBoolean);
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
    GPRReg argGPR = arg.gpr();
    
    GPRTemporary result(this, arg);
    GPRReg resultGPR = result.gpr();
    
    JITCompiler::Jump notCell;
    
    if (!isKnownCell(operand))
        notCell = m_jit.branchTestPtr(MacroAssembler::NonZero, argGPR, GPRInfo::tagMaskRegister);
    
    m_jit.loadPtr(JITCompiler::Address(argGPR, JSCell::structureOffset()), resultGPR);
    addBranch(m_jit.branchTest8(invert ? JITCompiler::Zero : JITCompiler::NonZero, JITCompiler::Address(resultGPR, Structure::typeInfoFlagsOffset()), JITCompiler::TrustedImm32(MasqueradesAsUndefined)), taken);
    
    if (!isKnownCell(operand)) {
        addBranch(m_jit.jump(), notTaken);
        
        notCell.link(&m_jit);
        
        m_jit.move(argGPR, resultGPR);
        m_jit.andPtr(JITCompiler::TrustedImm32(~TagBitUndefined), resultGPR);
        addBranch(m_jit.branchPtr(invert ? JITCompiler::NotEqual : JITCompiler::Equal, resultGPR, JITCompiler::TrustedImmPtr(reinterpret_cast<void*>(ValueNull))), taken);
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
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node.child1()) || isKnownNotInteger(node.child2())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();

        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
        addBranch(m_jit.branchTest8(callResultCondition, resultGPR), taken);
    } else {
        GPRTemporary result(this, arg2);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node.child1()))
            slowPath.append(m_jit.branchPtr(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node.child2()))
            slowPath.append(m_jit.branchPtr(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        addBranch(m_jit.branch32(cond, arg1GPR, arg2GPR), taken);
    
        if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
            addBranch(m_jit.jump(), notTaken);
    
            slowPath.link(&m_jit);
    
            silentSpillAllRegisters(resultGPR);
            setupStubArguments(arg1GPR, arg2GPR);
            m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
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
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    JITCompiler::JumpList slowPath;
    
    if (isKnownNotInteger(node.child1()) || isKnownNotInteger(node.child2())) {
        GPRResult result(this);
        GPRReg resultGPR = result.gpr();
    
        arg1.use();
        arg2.use();
    
        flushRegisters();
        
        callOperation(helperFunction, resultGPR, arg1GPR, arg2GPR);
        
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        jsValueResult(resultGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
    } else {
        GPRTemporary result(this, arg2);
        GPRReg resultGPR = result.gpr();

        arg1.use();
        arg2.use();
    
        if (!isKnownInteger(node.child1()))
            slowPath.append(m_jit.branchPtr(MacroAssembler::Below, arg1GPR, GPRInfo::tagTypeNumberRegister));
        if (!isKnownInteger(node.child2()))
            slowPath.append(m_jit.branchPtr(MacroAssembler::Below, arg2GPR, GPRInfo::tagTypeNumberRegister));
    
        m_jit.compare32(cond, arg1GPR, arg2GPR, resultGPR);
    
        if (!isKnownInteger(node.child1()) || !isKnownInteger(node.child2())) {
            JITCompiler::Jump haveResult = m_jit.jump();
    
            slowPath.link(&m_jit);
        
            silentSpillAllRegisters(resultGPR);
            setupStubArguments(arg1GPR, arg2GPR);
            m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
            appendCallWithExceptionCheck(helperFunction);
            m_jit.move(GPRInfo::returnValueGPR, resultGPR);
            silentFillAllRegisters(resultGPR);
        
            m_jit.andPtr(TrustedImm32(1), resultGPR);
        
            haveResult.link(&m_jit);
        }
        
        m_jit.or32(TrustedImm32(ValueFalse), resultGPR);
        
        jsValueResult(resultGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
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
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node.child1()) && isKnownCell(node.child2())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        addBranch(m_jit.branchPtr(JITCompiler::Equal, arg1GPR, arg2GPR), invert ? notTaken : taken);
        
        silentSpillAllRegisters(resultGPR);
        setupStubArguments(arg1GPR, arg2GPR);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationCompareStrictEqCell);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);
        
        addBranch(m_jit.branchTest8(invert ? JITCompiler::NonZero : JITCompiler::Zero, resultGPR), taken);
    } else {
        m_jit.orPtr(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTestPtr(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump numberCase = m_jit.branchTestPtr(JITCompiler::NonZero, resultGPR, GPRInfo::tagTypeNumberRegister);
        
        addBranch(m_jit.branch32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR), taken);
        addBranch(m_jit.jump(), notTaken);
        
        twoCellsCase.link(&m_jit);
        addBranch(m_jit.branchPtr(JITCompiler::Equal, arg1GPR, arg2GPR), invert ? notTaken : taken);
        
        numberCase.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        setupStubArguments(arg1GPR, arg2GPR);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationCompareStrictEq);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);
        
        addBranch(m_jit.branchTest8(invert ? JITCompiler::Zero : JITCompiler::NonZero, resultGPR), taken);
    }
    
    if (notTaken != (m_block + 1))
        addBranch(m_jit.jump(), notTaken);
}

void JITCodeGenerator::nonSpeculativeNonPeepholeStrictEq(Node& node, bool invert)
{
    JSValueOperand arg1(this, node.child1());
    JSValueOperand arg2(this, node.child2());
    GPRReg arg1GPR = arg1.gpr();
    GPRReg arg2GPR = arg2.gpr();
    
    GPRTemporary result(this);
    GPRReg resultGPR = result.gpr();
    
    arg1.use();
    arg2.use();
    
    if (isKnownCell(node.child1()) && isKnownCell(node.child2())) {
        // see if we get lucky: if the arguments are cells and they reference the same
        // cell, then they must be strictly equal.
        JITCompiler::Jump notEqualCase = m_jit.branchPtr(JITCompiler::NotEqual, arg1GPR, arg2GPR);
        
        m_jit.move(JITCompiler::TrustedImmPtr(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        JITCompiler::Jump done = m_jit.jump();

        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        setupStubArguments(arg1GPR, arg2GPR);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationCompareStrictEqCell);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultGPR);
        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        done.link(&m_jit);
    } else {
        m_jit.orPtr(arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::Jump twoCellsCase = m_jit.branchTestPtr(JITCompiler::Zero, resultGPR, GPRInfo::tagMaskRegister);
        
        JITCompiler::Jump numberCase = m_jit.branchTestPtr(JITCompiler::NonZero, resultGPR, GPRInfo::tagTypeNumberRegister);
        
        m_jit.compare32(invert ? JITCompiler::NotEqual : JITCompiler::Equal, arg1GPR, arg2GPR, resultGPR);
        
        JITCompiler::Jump done1 = m_jit.jump();
        
        twoCellsCase.link(&m_jit);
        JITCompiler::Jump notEqualCase = m_jit.branchPtr(JITCompiler::NotEqual, arg1GPR, arg2GPR);
        
        m_jit.move(JITCompiler::TrustedImmPtr(JSValue::encode(jsBoolean(!invert))), resultGPR);
        
        JITCompiler::Jump done2 = m_jit.jump();
        
        numberCase.link(&m_jit);
        notEqualCase.link(&m_jit);
        
        silentSpillAllRegisters(resultGPR);
        setupStubArguments(arg1GPR, arg2GPR);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        appendCallWithExceptionCheck(operationCompareStrictEq);
        m_jit.move(GPRInfo::returnValueGPR, resultGPR);
        silentFillAllRegisters(resultGPR);
        
        m_jit.andPtr(JITCompiler::TrustedImm32(1), resultGPR);

        done1.link(&m_jit);

        m_jit.or32(JITCompiler::TrustedImm32(ValueFalse), resultGPR);
        
        done2.link(&m_jit);
    }
    
    jsValueResult(resultGPR, m_compileIndex, DataFormatJSBoolean, UseChildrenCalledExplicitly);
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
    GPRReg calleeGPR = callee.gpr();
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
    
    m_jit.storePtr(MacroAssembler::TrustedImmPtr(JSValue::encode(jsNumber(numPassedArgs))), addressOfCallData(RegisterFile::ArgumentCount));
    m_jit.storePtr(GPRInfo::callFrameRegister, addressOfCallData(RegisterFile::CallerFrame));
    
    for (int argIdx = 0; argIdx < numArgs; argIdx++) {
        NodeIndex argNodeIndex = m_jit.graph().m_varArgChildren[node.firstChild() + 1 + argIdx];
        JSValueOperand arg(this, argNodeIndex);
        GPRReg argGPR = arg.gpr();
        use(argNodeIndex);
        
        m_jit.storePtr(argGPR, addressOfCallData(-callDataSize + argIdx + (isCall ? 0 : 1)));
    }
    
    m_jit.storePtr(calleeGPR, addressOfCallData(RegisterFile::Callee));
    
    flushRegisters();
    
    GPRResult result(this);
    GPRReg resultGPR = result.gpr();

    JITCompiler::DataLabelPtr targetToCheck;
    JITCompiler::Jump slowPath;
    
    slowPath = m_jit.branchPtrWithPatch(MacroAssembler::NotEqual, calleeGPR, targetToCheck, MacroAssembler::TrustedImmPtr(JSValue::encode(JSValue())));
    m_jit.loadPtr(MacroAssembler::Address(calleeGPR, OBJECT_OFFSETOF(JSFunction, m_scopeChain)), resultGPR);
    m_jit.storePtr(resultGPR, addressOfCallData(RegisterFile::ScopeChain));

    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    
    JITCompiler::Call fastCall = m_jit.nearCall();
    m_jit.notifyCall(fastCall, at(m_compileIndex).codeOrigin);
    
    JITCompiler::Jump done = m_jit.jump();
    
    slowPath.link(&m_jit);
    
    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
    JITCompiler::Call slowCall = m_jit.appendCallWithFastExceptionCheck(slowCallFunction, at(m_compileIndex).codeOrigin);
    m_jit.move(Imm32(numPassedArgs), GPRInfo::regT1);
    m_jit.addPtr(Imm32(m_jit.codeBlock()->m_numCalleeRegisters * sizeof(Register)), GPRInfo::callFrameRegister);
    m_jit.notifyCall(m_jit.call(GPRInfo::returnValueGPR), at(m_compileIndex).codeOrigin);
    
    done.link(&m_jit);
    
    m_jit.move(GPRInfo::returnValueGPR, resultGPR);
    
    jsValueResult(resultGPR, m_compileIndex, DataFormatJS, UseChildrenCalledExplicitly);
    
    m_jit.addJSCall(fastCall, slowCall, targetToCheck, isCall, at(m_compileIndex).codeOrigin);
}

#endif

} } // namespace JSC::DFG

#endif
