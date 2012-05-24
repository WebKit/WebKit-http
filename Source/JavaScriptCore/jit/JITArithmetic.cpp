/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#if ENABLE(JIT)
#include "JIT.h"

#include "CodeBlock.h"
#include "JITInlineMethods.h"
#include "JITStubCall.h"
#include "JITStubs.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "Interpreter.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {

void JIT::emit_op_jless(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jless, op1, op2, target, LessThan);
}

void JIT::emit_op_jlesseq(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jlesseq, op1, op2, target, LessThanOrEqual);
}

void JIT::emit_op_jgreater(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jgreater, op1, op2, target, GreaterThan);
}

void JIT::emit_op_jgreatereq(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jgreatereq, op1, op2, target, GreaterThanOrEqual);
}

void JIT::emit_op_jnless(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jnless, op1, op2, target, GreaterThanOrEqual);
}

void JIT::emit_op_jnlesseq(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jnlesseq, op1, op2, target, GreaterThan);
}

void JIT::emit_op_jngreater(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jngreater, op1, op2, target, LessThanOrEqual);
}

void JIT::emit_op_jngreatereq(Instruction* currentInstruction)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJump(op_jngreatereq, op1, op2, target, LessThan);
}

void JIT::emitSlow_op_jless(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleLessThan, cti_op_jless, false, iter);
}

void JIT::emitSlow_op_jlesseq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleLessThanOrEqual, cti_op_jlesseq, false, iter);
}

void JIT::emitSlow_op_jgreater(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleGreaterThan, cti_op_jgreater, false, iter);
}

void JIT::emitSlow_op_jgreatereq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleGreaterThanOrEqual, cti_op_jgreatereq, false, iter);
}

void JIT::emitSlow_op_jnless(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleGreaterThanOrEqualOrUnordered, cti_op_jless, true, iter);
}

void JIT::emitSlow_op_jnlesseq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleGreaterThanOrUnordered, cti_op_jlesseq, true, iter);
}

void JIT::emitSlow_op_jngreater(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleLessThanOrEqualOrUnordered, cti_op_jgreater, true, iter);
}

void JIT::emitSlow_op_jngreatereq(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned op1 = currentInstruction[1].u.operand;
    unsigned op2 = currentInstruction[2].u.operand;
    unsigned target = currentInstruction[3].u.operand;

    emit_compareAndJumpSlow(op1, op2, target, DoubleLessThanOrUnordered, cti_op_jgreatereq, true, iter);
}

#if USE(JSVALUE64)

void JIT::emit_op_negate(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned src = currentInstruction[2].u.operand;

    emitGetVirtualRegister(src, regT0);

    Jump srcNotInt = emitJumpIfNotImmediateInteger(regT0);
    addSlowCase(branchTest32(Zero, regT0, TrustedImm32(0x7fffffff)));
    neg32(regT0);
    emitFastArithReTagImmediate(regT0, regT0);

    Jump end = jump();

    srcNotInt.link(this);
    emitJumpSlowCaseIfNotImmediateNumber(regT0);

    move(TrustedImmPtr(reinterpret_cast<void*>(0x8000000000000000ull)), regT1);
    xorPtr(regT1, regT0);

    end.link(this);
    emitPutVirtualRegister(dst);
}

void JIT::emitSlow_op_negate(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;

    linkSlowCase(iter); // 0x7fffffff check
    linkSlowCase(iter); // double check

    JITStubCall stubCall(this, cti_op_negate);
    stubCall.addArgument(regT1, regT0);
    stubCall.call(dst);
}

void JIT::emit_op_lshift(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    emitGetVirtualRegisters(op1, regT0, op2, regT2);
    // FIXME: would we be better using 'emitJumpSlowCaseIfNotImmediateIntegers'? - we *probably* ought to be consistent.
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT2);
    emitFastArithImmToInt(regT0);
    emitFastArithImmToInt(regT2);
    lshift32(regT2, regT0);
    emitFastArithReTagImmediate(regT0, regT0);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_lshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    UNUSED_PARAM(op1);
    UNUSED_PARAM(op2);
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_lshift);
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT2);
    stubCall.call(result);
}

void JIT::emit_op_rshift(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (isOperandConstantImmediateInt(op2)) {
        // isOperandConstantImmediateInt(op2) => 1 SlowCase
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        // Mask with 0x1f as per ecma-262 11.7.2 step 7.
        rshift32(Imm32(getConstantOperandImmediateInt(op2) & 0x1f), regT0);
    } else {
        emitGetVirtualRegisters(op1, regT0, op2, regT2);
        if (supportsFloatingPointTruncate()) {
            Jump lhsIsInt = emitJumpIfImmediateInteger(regT0);
            // supportsFloatingPoint() && USE(JSVALUE64) => 3 SlowCases
            addSlowCase(emitJumpIfNotImmediateNumber(regT0));
            addPtr(tagTypeNumberRegister, regT0);
            movePtrToDouble(regT0, fpRegT0);
            addSlowCase(branchTruncateDoubleToInt32(fpRegT0, regT0));
            lhsIsInt.link(this);
            emitJumpSlowCaseIfNotImmediateInteger(regT2);
        } else {
            // !supportsFloatingPoint() => 2 SlowCases
            emitJumpSlowCaseIfNotImmediateInteger(regT0);
            emitJumpSlowCaseIfNotImmediateInteger(regT2);
        }
        emitFastArithImmToInt(regT2);
        rshift32(regT2, regT0);
    }
    emitFastArithIntToImmNoCheck(regT0, regT0);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_rshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    JITStubCall stubCall(this, cti_op_rshift);

    if (isOperandConstantImmediateInt(op2)) {
        linkSlowCase(iter);
        stubCall.addArgument(regT0);
        stubCall.addArgument(op2, regT2);
    } else {
        if (supportsFloatingPointTruncate()) {
            linkSlowCase(iter);
            linkSlowCase(iter);
            linkSlowCase(iter);
            // We're reloading op1 to regT0 as we can no longer guarantee that
            // we have not munged the operand.  It may have already been shifted
            // correctly, but it still will not have been tagged.
            stubCall.addArgument(op1, regT0);
            stubCall.addArgument(regT2);
        } else {
            linkSlowCase(iter);
            linkSlowCase(iter);
            stubCall.addArgument(regT0);
            stubCall.addArgument(regT2);
        }
    }

    stubCall.call(result);
}

void JIT::emit_op_urshift(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    // Slow case of urshift makes assumptions about what registers hold the
    // shift arguments, so any changes must be updated there as well.
    if (isOperandConstantImmediateInt(op2)) {
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        emitFastArithImmToInt(regT0);
        int shift = getConstantOperand(op2).asInt32();
        if (shift)
            urshift32(Imm32(shift & 0x1f), regT0);
        // unsigned shift < 0 or shift = k*2^32 may result in (essentially)
        // a toUint conversion, which can result in a value we can represent
        // as an immediate int.
        if (shift < 0 || !(shift & 31))
            addSlowCase(branch32(LessThan, regT0, TrustedImm32(0)));
        emitFastArithReTagImmediate(regT0, regT0);
        emitPutVirtualRegister(dst, regT0);
        return;
    }
    emitGetVirtualRegisters(op1, regT0, op2, regT1);
    if (!isOperandConstantImmediateInt(op1))
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);
    emitFastArithImmToInt(regT0);
    emitFastArithImmToInt(regT1);
    urshift32(regT1, regT0);
    addSlowCase(branch32(LessThan, regT0, TrustedImm32(0)));
    emitFastArithReTagImmediate(regT0, regT0);
    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_urshift(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    if (isOperandConstantImmediateInt(op2)) {
        int shift = getConstantOperand(op2).asInt32();
        // op1 = regT0
        linkSlowCase(iter); // int32 check
        if (supportsFloatingPointTruncate()) {
            JumpList failures;
            failures.append(emitJumpIfNotImmediateNumber(regT0)); // op1 is not a double
            addPtr(tagTypeNumberRegister, regT0);
            movePtrToDouble(regT0, fpRegT0);
            failures.append(branchTruncateDoubleToInt32(fpRegT0, regT0));
            if (shift)
                urshift32(Imm32(shift & 0x1f), regT0);
            if (shift < 0 || !(shift & 31))
                failures.append(branch32(LessThan, regT0, TrustedImm32(0)));
            emitFastArithReTagImmediate(regT0, regT0);
            emitPutVirtualRegister(dst, regT0);
            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_rshift));
            failures.link(this);
        }
        if (shift < 0 || !(shift & 31))
            linkSlowCase(iter); // failed to box in hot path
    } else {
        // op1 = regT0
        // op2 = regT1
        if (!isOperandConstantImmediateInt(op1)) {
            linkSlowCase(iter); // int32 check -- op1 is not an int
            if (supportsFloatingPointTruncate()) {
                JumpList failures;
                failures.append(emitJumpIfNotImmediateNumber(regT0)); // op1 is not a double
                addPtr(tagTypeNumberRegister, regT0);
                movePtrToDouble(regT0, fpRegT0);
                failures.append(branchTruncateDoubleToInt32(fpRegT0, regT0));
                failures.append(emitJumpIfNotImmediateInteger(regT1)); // op2 is not an int
                emitFastArithImmToInt(regT1);
                urshift32(regT1, regT0);
                failures.append(branch32(LessThan, regT0, TrustedImm32(0)));
                emitFastArithReTagImmediate(regT0, regT0);
                emitPutVirtualRegister(dst, regT0);
                emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_rshift));
                failures.link(this);
            }
        }
        
        linkSlowCase(iter); // int32 check - op2 is not an int
        linkSlowCase(iter); // Can't represent unsigned result as an immediate
    }
    
    JITStubCall stubCall(this, cti_op_urshift);
    stubCall.addArgument(op1, regT0);
    stubCall.addArgument(op2, regT1);
    stubCall.call(dst);
}

void JIT::emit_compareAndJump(OpcodeID, unsigned op1, unsigned op2, unsigned target, RelationalCondition condition)
{
    // We generate inline code for the following cases in the fast path:
    // - int immediate to constant int immediate
    // - constant int immediate to int immediate
    // - int immediate to int immediate

    if (isOperandConstantImmediateChar(op1)) {
        emitGetVirtualRegister(op2, regT0);
        addSlowCase(emitJumpIfNotJSCell(regT0));
        JumpList failures;
        emitLoadCharacterString(regT0, regT0, failures);
        addSlowCase(failures);
        addJump(branch32(commute(condition), regT0, Imm32(asString(getConstantOperand(op1))->tryGetValue()[0])), target);
        return;
    }
    if (isOperandConstantImmediateChar(op2)) {
        emitGetVirtualRegister(op1, regT0);
        addSlowCase(emitJumpIfNotJSCell(regT0));
        JumpList failures;
        emitLoadCharacterString(regT0, regT0, failures);
        addSlowCase(failures);
        addJump(branch32(condition, regT0, Imm32(asString(getConstantOperand(op2))->tryGetValue()[0])), target);
        return;
    }
    if (isOperandConstantImmediateInt(op2)) {
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        int32_t op2imm = getConstantOperandImmediateInt(op2);
        addJump(branch32(condition, regT0, Imm32(op2imm)), target);
    } else if (isOperandConstantImmediateInt(op1)) {
        emitGetVirtualRegister(op2, regT1);
        emitJumpSlowCaseIfNotImmediateInteger(regT1);
        int32_t op1imm = getConstantOperandImmediateInt(op1);
        addJump(branch32(commute(condition), regT1, Imm32(op1imm)), target);
    } else {
        emitGetVirtualRegisters(op1, regT0, op2, regT1);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT1);

        addJump(branch32(condition, regT0, regT1), target);
    }
}

void JIT::emit_compareAndJumpSlow(unsigned op1, unsigned op2, unsigned target, DoubleCondition condition, int (JIT_STUB *stub)(STUB_ARGS_DECLARATION), bool invert, Vector<SlowCaseEntry>::iterator& iter)
{
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jlesseq), OPCODE_LENGTH_op_jlesseq_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jnless), OPCODE_LENGTH_op_jnless_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jnlesseq), OPCODE_LENGTH_op_jnlesseq_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jgreater), OPCODE_LENGTH_op_jgreater_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jgreatereq), OPCODE_LENGTH_op_jgreatereq_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jngreater), OPCODE_LENGTH_op_jngreater_equals_op_jless);
    COMPILE_ASSERT(OPCODE_LENGTH(op_jless) == OPCODE_LENGTH(op_jngreatereq), OPCODE_LENGTH_op_jngreatereq_equals_op_jless);
    
    // We generate inline code for the following cases in the slow path:
    // - floating-point number to constant int immediate
    // - constant int immediate to floating-point number
    // - floating-point number to floating-point number.
    if (isOperandConstantImmediateChar(op1) || isOperandConstantImmediateChar(op2)) {
        linkSlowCase(iter);
        linkSlowCase(iter);
        linkSlowCase(iter);
        linkSlowCase(iter);
        JITStubCall stubCall(this, stub);
        stubCall.addArgument(op1, regT0);
        stubCall.addArgument(op2, regT1);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(invert ? Zero : NonZero, regT0), target);
        return;
    }

    if (isOperandConstantImmediateInt(op2)) {
        linkSlowCase(iter);

        if (supportsFloatingPoint()) {
            Jump fail1 = emitJumpIfNotImmediateNumber(regT0);
            addPtr(tagTypeNumberRegister, regT0);
            movePtrToDouble(regT0, fpRegT0);

            int32_t op2imm = getConstantOperand(op2).asInt32();

            move(Imm32(op2imm), regT1);
            convertInt32ToDouble(regT1, fpRegT1);

            emitJumpSlowToHot(branchDouble(condition, fpRegT0, fpRegT1), target);

            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_jless));

            fail1.link(this);
        }

        JITStubCall stubCall(this, stub);
        stubCall.addArgument(regT0);
        stubCall.addArgument(op2, regT2);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(invert ? Zero : NonZero, regT0), target);

    } else if (isOperandConstantImmediateInt(op1)) {
        linkSlowCase(iter);

        if (supportsFloatingPoint()) {
            Jump fail1 = emitJumpIfNotImmediateNumber(regT1);
            addPtr(tagTypeNumberRegister, regT1);
            movePtrToDouble(regT1, fpRegT1);

            int32_t op1imm = getConstantOperand(op1).asInt32();

            move(Imm32(op1imm), regT0);
            convertInt32ToDouble(regT0, fpRegT0);

            emitJumpSlowToHot(branchDouble(condition, fpRegT0, fpRegT1), target);

            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_jless));

            fail1.link(this);
        }

        JITStubCall stubCall(this, stub);
        stubCall.addArgument(op1, regT2);
        stubCall.addArgument(regT1);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(invert ? Zero : NonZero, regT0), target);
    } else {
        linkSlowCase(iter);

        if (supportsFloatingPoint()) {
            Jump fail1 = emitJumpIfNotImmediateNumber(regT0);
            Jump fail2 = emitJumpIfNotImmediateNumber(regT1);
            Jump fail3 = emitJumpIfImmediateInteger(regT1);
            addPtr(tagTypeNumberRegister, regT0);
            addPtr(tagTypeNumberRegister, regT1);
            movePtrToDouble(regT0, fpRegT0);
            movePtrToDouble(regT1, fpRegT1);

            emitJumpSlowToHot(branchDouble(condition, fpRegT0, fpRegT1), target);

            emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_jless));

            fail1.link(this);
            fail2.link(this);
            fail3.link(this);
        }

        linkSlowCase(iter);
        JITStubCall stubCall(this, stub);
        stubCall.addArgument(regT0);
        stubCall.addArgument(regT1);
        stubCall.call();
        emitJumpSlowToHot(branchTest32(invert ? Zero : NonZero, regT0), target);
    }
}

void JIT::emit_op_bitand(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    if (isOperandConstantImmediateInt(op1)) {
        emitGetVirtualRegister(op2, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        int32_t imm = getConstantOperandImmediateInt(op1);
        andPtr(Imm32(imm), regT0);
        if (imm >= 0)
            emitFastArithIntToImmNoCheck(regT0, regT0);
    } else if (isOperandConstantImmediateInt(op2)) {
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        int32_t imm = getConstantOperandImmediateInt(op2);
        andPtr(Imm32(imm), regT0);
        if (imm >= 0)
            emitFastArithIntToImmNoCheck(regT0, regT0);
    } else {
        emitGetVirtualRegisters(op1, regT0, op2, regT1);
        andPtr(regT1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
    }
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_bitand(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    linkSlowCase(iter);
    if (isOperandConstantImmediateInt(op1)) {
        JITStubCall stubCall(this, cti_op_bitand);
        stubCall.addArgument(op1, regT2);
        stubCall.addArgument(regT0);
        stubCall.call(result);
    } else if (isOperandConstantImmediateInt(op2)) {
        JITStubCall stubCall(this, cti_op_bitand);
        stubCall.addArgument(regT0);
        stubCall.addArgument(op2, regT2);
        stubCall.call(result);
    } else {
        JITStubCall stubCall(this, cti_op_bitand);
        stubCall.addArgument(op1, regT2);
        stubCall.addArgument(regT1);
        stubCall.call(result);
    }
}

void JIT::emit_op_post_inc(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned srcDst = currentInstruction[2].u.operand;

    emitGetVirtualRegister(srcDst, regT0);
    move(regT0, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    addSlowCase(branchAdd32(Overflow, TrustedImm32(1), regT1));
    emitFastArithIntToImmNoCheck(regT1, regT1);
    emitPutVirtualRegister(srcDst, regT1);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_post_inc(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned srcDst = currentInstruction[2].u.operand;

    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_post_inc);
    stubCall.addArgument(regT0);
    stubCall.addArgument(Imm32(srcDst));
    stubCall.call(result);
}

void JIT::emit_op_post_dec(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned srcDst = currentInstruction[2].u.operand;

    emitGetVirtualRegister(srcDst, regT0);
    move(regT0, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    addSlowCase(branchSub32(Overflow, TrustedImm32(1), regT1));
    emitFastArithIntToImmNoCheck(regT1, regT1);
    emitPutVirtualRegister(srcDst, regT1);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_post_dec(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned srcDst = currentInstruction[2].u.operand;

    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_post_dec);
    stubCall.addArgument(regT0);
    stubCall.addArgument(Imm32(srcDst));
    stubCall.call(result);
}

void JIT::emit_op_pre_inc(Instruction* currentInstruction)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    emitGetVirtualRegister(srcDst, regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    addSlowCase(branchAdd32(Overflow, TrustedImm32(1), regT0));
    emitFastArithIntToImmNoCheck(regT0, regT0);
    emitPutVirtualRegister(srcDst);
}

void JIT::emitSlow_op_pre_inc(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    Jump notImm = getSlowCase(iter);
    linkSlowCase(iter);
    emitGetVirtualRegister(srcDst, regT0);
    notImm.link(this);
    JITStubCall stubCall(this, cti_op_pre_inc);
    stubCall.addArgument(regT0);
    stubCall.call(srcDst);
}

void JIT::emit_op_pre_dec(Instruction* currentInstruction)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    emitGetVirtualRegister(srcDst, regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    addSlowCase(branchSub32(Overflow, TrustedImm32(1), regT0));
    emitFastArithIntToImmNoCheck(regT0, regT0);
    emitPutVirtualRegister(srcDst);
}

void JIT::emitSlow_op_pre_dec(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned srcDst = currentInstruction[1].u.operand;

    Jump notImm = getSlowCase(iter);
    linkSlowCase(iter);
    emitGetVirtualRegister(srcDst, regT0);
    notImm.link(this);
    JITStubCall stubCall(this, cti_op_pre_dec);
    stubCall.addArgument(regT0);
    stubCall.call(srcDst);
}

/* ------------------------------ BEGIN: OP_MOD ------------------------------ */

#if CPU(X86) || CPU(X86_64)

void JIT::emit_op_mod(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    // Make sure registers are correct for x86 IDIV instructions.
    ASSERT(regT0 == X86Registers::eax);
    ASSERT(regT1 == X86Registers::edx);
    ASSERT(regT2 == X86Registers::ecx);

    emitGetVirtualRegisters(op1, regT3, op2, regT2);
    emitJumpSlowCaseIfNotImmediateInteger(regT3);
    emitJumpSlowCaseIfNotImmediateInteger(regT2);

    move(regT3, regT0);
    addSlowCase(branchTest32(Zero, regT2));
    Jump denominatorNotNeg1 = branch32(NotEqual, regT2, TrustedImm32(-1));
    addSlowCase(branch32(Equal, regT0, TrustedImm32(-2147483647-1)));
    denominatorNotNeg1.link(this);
    m_assembler.cdq();
    m_assembler.idivl_r(regT2);
    Jump numeratorPositive = branch32(GreaterThanOrEqual, regT3, TrustedImm32(0));
    addSlowCase(branchTest32(Zero, regT1));
    numeratorPositive.link(this);
    emitFastArithReTagImmediate(regT1, regT0);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_mod(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;

    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    linkSlowCase(iter);
    JITStubCall stubCall(this, cti_op_mod);
    stubCall.addArgument(regT3);
    stubCall.addArgument(regT2);
    stubCall.call(result);
}

#else // CPU(X86) || CPU(X86_64)

void JIT::emit_op_mod(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;

    JITStubCall stubCall(this, cti_op_mod);
    stubCall.addArgument(op1, regT2);
    stubCall.addArgument(op2, regT2);
    stubCall.call(result);
}

void JIT::emitSlow_op_mod(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ASSERT_NOT_REACHED();
}

#endif // CPU(X86) || CPU(X86_64)

/* ------------------------------ END: OP_MOD ------------------------------ */

/* ------------------------------ BEGIN: USE(JSVALUE64) (OP_ADD, OP_SUB, OP_MUL) ------------------------------ */

void JIT::compileBinaryArithOp(OpcodeID opcodeID, unsigned, unsigned op1, unsigned op2, OperandTypes)
{
    emitGetVirtualRegisters(op1, regT0, op2, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT0);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);
#if ENABLE(VALUE_PROFILER)
    RareCaseProfile* profile = m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset);
#endif
    if (opcodeID == op_add)
        addSlowCase(branchAdd32(Overflow, regT1, regT0));
    else if (opcodeID == op_sub)
        addSlowCase(branchSub32(Overflow, regT1, regT0));
    else {
        ASSERT(opcodeID == op_mul);
#if ENABLE(VALUE_PROFILER)
        if (shouldEmitProfiling()) {
            // We want to be able to measure if this is taking the slow case just
            // because of negative zero. If this produces positive zero, then we
            // don't want the slow case to be taken because that will throw off
            // speculative compilation.
            move(regT0, regT2);
            addSlowCase(branchMul32(Overflow, regT1, regT2));
            JumpList done;
            done.append(branchTest32(NonZero, regT2));
            Jump negativeZero = branch32(LessThan, regT0, TrustedImm32(0));
            done.append(branch32(GreaterThanOrEqual, regT1, TrustedImm32(0)));
            negativeZero.link(this);
            // We only get here if we have a genuine negative zero. Record this,
            // so that the speculative JIT knows that we failed speculation
            // because of a negative zero.
            add32(TrustedImm32(1), AbsoluteAddress(&profile->m_counter));
            addSlowCase(jump());
            done.link(this);
            move(regT2, regT0);
        } else {
            addSlowCase(branchMul32(Overflow, regT1, regT0));
            addSlowCase(branchTest32(Zero, regT0));
        }
#else
        addSlowCase(branchMul32(Overflow, regT1, regT0));
        addSlowCase(branchTest32(Zero, regT0));
#endif
    }
    emitFastArithIntToImmNoCheck(regT0, regT0);
}

void JIT::compileBinaryArithOpSlowCase(OpcodeID opcodeID, Vector<SlowCaseEntry>::iterator& iter, unsigned result, unsigned op1, unsigned op2, OperandTypes types, bool op1HasImmediateIntFastCase, bool op2HasImmediateIntFastCase)
{
    // We assume that subtracting TagTypeNumber is equivalent to adding DoubleEncodeOffset.
    COMPILE_ASSERT(((TagTypeNumber + DoubleEncodeOffset) == 0), TagTypeNumber_PLUS_DoubleEncodeOffset_EQUALS_0);

    Jump notImm1;
    Jump notImm2;
    if (op1HasImmediateIntFastCase) {
        notImm2 = getSlowCase(iter);
    } else if (op2HasImmediateIntFastCase) {
        notImm1 = getSlowCase(iter);
    } else {
        notImm1 = getSlowCase(iter);
        notImm2 = getSlowCase(iter);
    }

    linkSlowCase(iter); // Integer overflow case - we could handle this in JIT code, but this is likely rare.
    if (opcodeID == op_mul && !op1HasImmediateIntFastCase && !op2HasImmediateIntFastCase) // op_mul has an extra slow case to handle 0 * negative number.
        linkSlowCase(iter);
    emitGetVirtualRegister(op1, regT0);

    Label stubFunctionCall(this);
    JITStubCall stubCall(this, opcodeID == op_add ? cti_op_add : opcodeID == op_sub ? cti_op_sub : cti_op_mul);
    if (op1HasImmediateIntFastCase || op2HasImmediateIntFastCase) {
        emitGetVirtualRegister(op1, regT0);
        emitGetVirtualRegister(op2, regT1);
    }
    stubCall.addArgument(regT0);
    stubCall.addArgument(regT1);
    stubCall.call(result);
    Jump end = jump();

    if (op1HasImmediateIntFastCase) {
        notImm2.link(this);
        if (!types.second().definitelyIsNumber())
            emitJumpIfNotImmediateNumber(regT0).linkTo(stubFunctionCall, this);
        emitGetVirtualRegister(op1, regT1);
        convertInt32ToDouble(regT1, fpRegT1);
        addPtr(tagTypeNumberRegister, regT0);
        movePtrToDouble(regT0, fpRegT2);
    } else if (op2HasImmediateIntFastCase) {
        notImm1.link(this);
        if (!types.first().definitelyIsNumber())
            emitJumpIfNotImmediateNumber(regT0).linkTo(stubFunctionCall, this);
        emitGetVirtualRegister(op2, regT1);
        convertInt32ToDouble(regT1, fpRegT1);
        addPtr(tagTypeNumberRegister, regT0);
        movePtrToDouble(regT0, fpRegT2);
    } else {
        // if we get here, eax is not an int32, edx not yet checked.
        notImm1.link(this);
        if (!types.first().definitelyIsNumber())
            emitJumpIfNotImmediateNumber(regT0).linkTo(stubFunctionCall, this);
        if (!types.second().definitelyIsNumber())
            emitJumpIfNotImmediateNumber(regT1).linkTo(stubFunctionCall, this);
        addPtr(tagTypeNumberRegister, regT0);
        movePtrToDouble(regT0, fpRegT1);
        Jump op2isDouble = emitJumpIfNotImmediateInteger(regT1);
        convertInt32ToDouble(regT1, fpRegT2);
        Jump op2wasInteger = jump();

        // if we get here, eax IS an int32, edx is not.
        notImm2.link(this);
        if (!types.second().definitelyIsNumber())
            emitJumpIfNotImmediateNumber(regT1).linkTo(stubFunctionCall, this);
        convertInt32ToDouble(regT0, fpRegT1);
        op2isDouble.link(this);
        addPtr(tagTypeNumberRegister, regT1);
        movePtrToDouble(regT1, fpRegT2);
        op2wasInteger.link(this);
    }

    if (opcodeID == op_add)
        addDouble(fpRegT2, fpRegT1);
    else if (opcodeID == op_sub)
        subDouble(fpRegT2, fpRegT1);
    else if (opcodeID == op_mul)
        mulDouble(fpRegT2, fpRegT1);
    else {
        ASSERT(opcodeID == op_div);
        divDouble(fpRegT2, fpRegT1);
    }
    moveDoubleToPtr(fpRegT1, regT0);
    subPtr(tagTypeNumberRegister, regT0);
    emitPutVirtualRegister(result, regT0);

    end.link(this);
}

void JIT::emit_op_add(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (!types.first().mightBeNumber() || !types.second().mightBeNumber()) {
        addSlowCase();
        JITStubCall stubCall(this, cti_op_add);
        stubCall.addArgument(op1, regT2);
        stubCall.addArgument(op2, regT2);
        stubCall.call(result);
        return;
    }

    if (isOperandConstantImmediateInt(op1)) {
        emitGetVirtualRegister(op2, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        addSlowCase(branchAdd32(Overflow, regT0, Imm32(getConstantOperandImmediateInt(op1)), regT1));
        emitFastArithIntToImmNoCheck(regT1, regT0);
    } else if (isOperandConstantImmediateInt(op2)) {
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        addSlowCase(branchAdd32(Overflow, regT0, Imm32(getConstantOperandImmediateInt(op2)), regT1));
        emitFastArithIntToImmNoCheck(regT1, regT0);
    } else
        compileBinaryArithOp(op_add, result, op1, op2, types);

    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_add(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (!types.first().mightBeNumber() || !types.second().mightBeNumber()) {
        linkDummySlowCase(iter);
        return;
    }

    bool op1HasImmediateIntFastCase = isOperandConstantImmediateInt(op1);
    bool op2HasImmediateIntFastCase = !op1HasImmediateIntFastCase && isOperandConstantImmediateInt(op2);
    compileBinaryArithOpSlowCase(op_add, iter, result, op1, op2, types, op1HasImmediateIntFastCase, op2HasImmediateIntFastCase);
}

void JIT::emit_op_mul(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    // For now, only plant a fast int case if the constant operand is greater than zero.
    int32_t value;
    if (isOperandConstantImmediateInt(op1) && ((value = getConstantOperandImmediateInt(op1)) > 0)) {
#if ENABLE(VALUE_PROFILER)
        // Add a special fast case profile because the DFG JIT will expect one.
        m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset);
#endif
        emitGetVirtualRegister(op2, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        addSlowCase(branchMul32(Overflow, Imm32(value), regT0, regT1));
        emitFastArithReTagImmediate(regT1, regT0);
    } else if (isOperandConstantImmediateInt(op2) && ((value = getConstantOperandImmediateInt(op2)) > 0)) {
#if ENABLE(VALUE_PROFILER)
        // Add a special fast case profile because the DFG JIT will expect one.
        m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset);
#endif
        emitGetVirtualRegister(op1, regT0);
        emitJumpSlowCaseIfNotImmediateInteger(regT0);
        addSlowCase(branchMul32(Overflow, Imm32(value), regT0, regT1));
        emitFastArithReTagImmediate(regT1, regT0);
    } else
        compileBinaryArithOp(op_mul, result, op1, op2, types);

    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_mul(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    bool op1HasImmediateIntFastCase = isOperandConstantImmediateInt(op1) && getConstantOperandImmediateInt(op1) > 0;
    bool op2HasImmediateIntFastCase = !op1HasImmediateIntFastCase && isOperandConstantImmediateInt(op2) && getConstantOperandImmediateInt(op2) > 0;
    compileBinaryArithOpSlowCase(op_mul, iter, result, op1, op2, types, op1HasImmediateIntFastCase, op2HasImmediateIntFastCase);
}

void JIT::emit_op_div(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    if (isOperandConstantImmediateDouble(op1)) {
        emitGetVirtualRegister(op1, regT0);
        addPtr(tagTypeNumberRegister, regT0);
        movePtrToDouble(regT0, fpRegT0);
    } else if (isOperandConstantImmediateInt(op1)) {
        emitLoadInt32ToDouble(op1, fpRegT0);
    } else {
        emitGetVirtualRegister(op1, regT0);
        if (!types.first().definitelyIsNumber())
            emitJumpSlowCaseIfNotImmediateNumber(regT0);
        Jump notInt = emitJumpIfNotImmediateInteger(regT0);
        convertInt32ToDouble(regT0, fpRegT0);
        Jump skipDoubleLoad = jump();
        notInt.link(this);
        addPtr(tagTypeNumberRegister, regT0);
        movePtrToDouble(regT0, fpRegT0);
        skipDoubleLoad.link(this);
    }

    if (isOperandConstantImmediateDouble(op2)) {
        emitGetVirtualRegister(op2, regT1);
        addPtr(tagTypeNumberRegister, regT1);
        movePtrToDouble(regT1, fpRegT1);
    } else if (isOperandConstantImmediateInt(op2)) {
        emitLoadInt32ToDouble(op2, fpRegT1);
    } else {
        emitGetVirtualRegister(op2, regT1);
        if (!types.second().definitelyIsNumber())
            emitJumpSlowCaseIfNotImmediateNumber(regT1);
        Jump notInt = emitJumpIfNotImmediateInteger(regT1);
        convertInt32ToDouble(regT1, fpRegT1);
        Jump skipDoubleLoad = jump();
        notInt.link(this);
        addPtr(tagTypeNumberRegister, regT1);
        movePtrToDouble(regT1, fpRegT1);
        skipDoubleLoad.link(this);
    }
    divDouble(fpRegT1, fpRegT0);
    
#if ENABLE(VALUE_PROFILER)
    // Is the result actually an integer? The DFG JIT would really like to know. If it's
    // not an integer, we increment a count. If this together with the slow case counter
    // are below threshold then the DFG JIT will compile this division with a specualtion
    // that the remainder is zero.
    
    // As well, there are cases where a double result here would cause an important field
    // in the heap to sometimes have doubles in it, resulting in double predictions getting
    // propagated to a use site where it might cause damage (such as the index to an array
    // access). So if we are DFG compiling anything in the program, we want this code to
    // ensure that it produces integers whenever possible.
    
    // FIXME: This will fail to convert to integer if the result is zero. We should
    // distinguish between positive zero and negative zero here.
    
    JumpList notInteger;
    branchConvertDoubleToInt32(fpRegT0, regT0, notInteger, fpRegT1);
    // If we've got an integer, we might as well make that the result of the division.
    emitFastArithReTagImmediate(regT0, regT0);
    Jump isInteger = jump();
    notInteger.link(this);
    add32(TrustedImm32(1), AbsoluteAddress(&m_codeBlock->addSpecialFastCaseProfile(m_bytecodeOffset)->m_counter));
    moveDoubleToPtr(fpRegT0, regT0);
    subPtr(tagTypeNumberRegister, regT0);
    isInteger.link(this);
#else
    // Double result.
    moveDoubleToPtr(fpRegT0, regT0);
    subPtr(tagTypeNumberRegister, regT0);
#endif

    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_div(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);
    if (types.first().definitelyIsNumber() && types.second().definitelyIsNumber()) {
#ifndef NDEBUG
        breakpoint();
#endif
        return;
    }
    if (!isOperandConstantImmediateDouble(op1) && !isOperandConstantImmediateInt(op1)) {
        if (!types.first().definitelyIsNumber())
            linkSlowCase(iter);
    }
    if (!isOperandConstantImmediateDouble(op2) && !isOperandConstantImmediateInt(op2)) {
        if (!types.second().definitelyIsNumber())
            linkSlowCase(iter);
    }
    // There is an extra slow case for (op1 * -N) or (-N * op2), to check for 0 since this should produce a result of -0.
    JITStubCall stubCall(this, cti_op_div);
    stubCall.addArgument(op1, regT2);
    stubCall.addArgument(op2, regT2);
    stubCall.call(result);
}

void JIT::emit_op_sub(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    compileBinaryArithOp(op_sub, result, op1, op2, types);
    emitPutVirtualRegister(result);
}

void JIT::emitSlow_op_sub(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned op1 = currentInstruction[2].u.operand;
    unsigned op2 = currentInstruction[3].u.operand;
    OperandTypes types = OperandTypes::fromInt(currentInstruction[4].u.operand);

    compileBinaryArithOpSlowCase(op_sub, iter, result, op1, op2, types, false, false);
}

/* ------------------------------ END: OP_ADD, OP_SUB, OP_MUL ------------------------------ */

#endif // USE(JSVALUE64)

} // namespace JSC

#endif // ENABLE(JIT)
