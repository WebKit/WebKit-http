/*
 * Copyright (C) 2008, 2012, 2013 Apple Inc. All rights reserved.
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

#ifndef JITInlines_h
#define JITInlines_h


#if ENABLE(JIT)

#include "CallFrameInlines.h"

namespace JSC {

ALWAYS_INLINE bool JIT::isOperandConstantImmediateDouble(int src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isDouble();
}

ALWAYS_INLINE JSValue JIT::getConstantOperand(int src)
{
    ASSERT(m_codeBlock->isConstantRegisterIndex(src));
    return m_codeBlock->getConstant(src);
}

ALWAYS_INLINE void JIT::emitPutIntToCallFrameHeader(RegisterID from, JSStack::CallFrameHeaderEntry entry)
{
#if USE(JSVALUE32_64)
    store32(TrustedImm32(Int32Tag), intTagFor(entry, callFrameRegister));
    store32(from, intPayloadFor(entry, callFrameRegister));
#else
    store64(from, addressFor(entry, callFrameRegister));
#endif
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    loadPtr(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
}

ALWAYS_INLINE void JIT::emitGetFromCallFrameHeader32(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    load32(Address(from, entry * sizeof(Register)), to);
#if USE(JSVALUE64)
    killLastResultRegister();
#endif
}

#if USE(JSVALUE64)
ALWAYS_INLINE void JIT::emitGetFromCallFrameHeader64(JSStack::CallFrameHeaderEntry entry, RegisterID to, RegisterID from)
{
    load64(Address(from, entry * sizeof(Register)), to);
    killLastResultRegister();
}
#endif

ALWAYS_INLINE void JIT::emitLoadCharacterString(RegisterID src, RegisterID dst, JumpList& failures)
{
    failures.append(branchPtr(NotEqual, Address(src, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get())));
    failures.append(branch32(NotEqual, MacroAssembler::Address(src, ThunkHelpers::jsStringLengthOffset()), TrustedImm32(1)));
    loadPtr(MacroAssembler::Address(src, ThunkHelpers::jsStringValueOffset()), dst);
    failures.append(branchTest32(Zero, dst));
    loadPtr(MacroAssembler::Address(dst, StringImpl::flagsOffset()), regT1);
    loadPtr(MacroAssembler::Address(dst, StringImpl::dataOffset()), dst);

    JumpList is16Bit;
    JumpList cont8Bit;
    is16Bit.append(branchTest32(Zero, regT1, TrustedImm32(StringImpl::flagIs8Bit())));
    load8(MacroAssembler::Address(dst, 0), dst);
    cont8Bit.append(jump());
    is16Bit.link(this);
    load16(MacroAssembler::Address(dst, 0), dst);
    cont8Bit.link(this);
}

ALWAYS_INLINE JIT::Call JIT::emitNakedCall(CodePtr function)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    Call nakedCall = nearCall();
    m_calls.append(CallRecord(nakedCall, m_bytecodeOffset, function.executableAddress()));
    return nakedCall;
}

ALWAYS_INLINE bool JIT::atJumpTarget()
{
    while (m_jumpTargetsPosition < m_codeBlock->numberOfJumpTargets() && m_codeBlock->jumpTarget(m_jumpTargetsPosition) <= m_bytecodeOffset) {
        if (m_codeBlock->jumpTarget(m_jumpTargetsPosition) == m_bytecodeOffset)
            return true;
        ++m_jumpTargetsPosition;
    }
    return false;
}

ALWAYS_INLINE void JIT::updateTopCallFrame()
{
    ASSERT(static_cast<int>(m_bytecodeOffset) >= 0);
#if USE(JSVALUE32_64)
    Instruction* instruction = m_codeBlock->instructions().begin() + m_bytecodeOffset + 1; 
    uint32_t locationBits = CallFrame::Location::encodeAsBytecodeInstruction(instruction);
#else
    uint32_t locationBits = CallFrame::Location::encodeAsBytecodeOffset(m_bytecodeOffset + 1);
#endif
    store32(TrustedImm32(locationBits), intTagFor(JSStack::ArgumentCount));
    storePtr(callFrameRegister, &m_vm->topCallFrame);
}

ALWAYS_INLINE MacroAssembler::Call JIT::appendCallWithExceptionCheck(const FunctionPtr& function)
{
    updateTopCallFrame();
    MacroAssembler::Call call = appendCall(function);
    exceptionCheck();
    return call;
}

ALWAYS_INLINE MacroAssembler::Call JIT::appendCallWithCallFrameRollbackOnException(const FunctionPtr& function)
{
    updateTopCallFrame(); // The callee is responsible for setting topCallFrame to their caller
    MacroAssembler::Call call = appendCall(function);
    exceptionCheckWithCallFrameRollback();
    return call;
}

ALWAYS_INLINE MacroAssembler::Call JIT::appendCallWithExceptionCheckSetJSValueResult(const FunctionPtr& function, int dst)
{
    MacroAssembler::Call call = appendCallWithExceptionCheck(function);
#if USE(JSVALUE64)
    emitPutVirtualRegister(dst, returnValueRegister);
#else
    emitStore(dst, returnValue2Register, returnValueRegister);
#endif
    return call;
}

ALWAYS_INLINE MacroAssembler::Call JIT::appendCallWithExceptionCheckSetJSValueResultWithProfile(const FunctionPtr& function, int dst)
{
    MacroAssembler::Call call = appendCallWithExceptionCheck(function);
    emitValueProfilingSite(regT4);
#if USE(JSVALUE64)
    emitPutVirtualRegister(dst, returnValueRegister);
#else
    emitStore(dst, returnValue2Register, returnValueRegister);
#endif
    return call;
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(C_JITOperation_E operation)
{
    setupArgumentsExecState();
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(C_JITOperation_EO operation, GPRReg arg)
{
    setupArgumentsWithExecState(arg);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(C_JITOperation_ESt operation, Structure* structure)
{
    setupArgumentsWithExecState(TrustedImmPtr(structure));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(C_JITOperation_EZ operation, int32_t arg)
{
    setupArgumentsWithExecState(TrustedImm32(arg));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_E operation, int dst)
{
    setupArgumentsExecState();
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EAapJcpZ operation, int dst, ArrayAllocationProfile* arg1, GPRReg arg2, int32_t arg3)
{
    setupArgumentsWithExecState(TrustedImmPtr(arg1), arg2, TrustedImm32(arg3));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EAapJcpZ operation, int dst, ArrayAllocationProfile* arg1, const JSValue* arg2, int32_t arg3)
{
    setupArgumentsWithExecState(TrustedImmPtr(arg1), TrustedImmPtr(arg2), TrustedImm32(arg3));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EC operation, int dst, JSCell* cell)
{
    setupArgumentsWithExecState(TrustedImmPtr(cell));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EP operation, int dst, void* pointer)
{
    setupArgumentsWithExecState(TrustedImmPtr(pointer));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(WithProfileTag, J_JITOperation_EPc operation, int dst, Instruction* bytecodePC)
{
    setupArgumentsWithExecState(TrustedImmPtr(bytecodePC));
    return appendCallWithExceptionCheckSetJSValueResultWithProfile(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EZ operation, int dst, int32_t arg)
{
    setupArgumentsWithExecState(TrustedImm32(arg));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(P_JITOperation_EZ operation, int32_t op)
{
    setupArgumentsWithExecState(TrustedImm32(op));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_ECC operation, RegisterID regOp1, RegisterID regOp2)
{
    setupArgumentsWithExecState(regOp1, regOp2);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_EOJss operation, RegisterID regOp1, RegisterID regOp2)
{
    setupArgumentsWithExecState(regOp1, regOp2);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_E operation)
{
    setupArgumentsExecState();
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EC operation, RegisterID regOp)
{
    setupArgumentsWithExecState(regOp);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_ECC operation, RegisterID regOp1, RegisterID regOp2)
{
    setupArgumentsWithExecState(regOp1, regOp2);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EPc operation, Instruction* bytecodePC)
{
    setupArgumentsWithExecState(TrustedImmPtr(bytecodePC));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EZ operation, int32_t op)
{
    setupArgumentsWithExecState(TrustedImm32(op));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperationWithCallFrameRollbackOnException(J_JITOperation_E operation)
{
    setupArgumentsExecState();
    return appendCallWithCallFrameRollbackOnException(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperationWithCallFrameRollbackOnException(V_JITOperation_ECb operation, CodeBlock* pointer)
{
    setupArgumentsWithExecState(TrustedImmPtr(pointer));
    return appendCallWithCallFrameRollbackOnException(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperationWithCallFrameRollbackOnException(Z_JITOperation_E operation)
{
    setupArgumentsExecState();
    return appendCallWithCallFrameRollbackOnException(operation);
}


#if USE(JSVALUE64)
ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(F_JITOperation_EJJZ operation, GPRReg arg1, GPRReg arg2, int32_t arg3)
{
    setupArgumentsWithExecState(arg1, arg2, TrustedImm32(arg3));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_ESsiJJI operation, StructureStubInfo* stubInfo, RegisterID regOp1, RegisterID regOp2, StringImpl* uid)
{
    setupArgumentsWithExecState(TrustedImmPtr(stubInfo), regOp1, regOp2, TrustedImmPtr(uid));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJJJ operation, RegisterID regOp1, RegisterID regOp2, RegisterID regOp3)
{
    setupArgumentsWithExecState(regOp1, regOp2, regOp3);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(JIT::WithProfileTag, J_JITOperation_ESsiJI operation, int dst, StructureStubInfo* stubInfo, GPRReg arg1, StringImpl* uid)
{
    setupArgumentsWithExecState(TrustedImmPtr(stubInfo), arg1, TrustedImmPtr(uid));
    return appendCallWithExceptionCheckSetJSValueResultWithProfile(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(JIT::WithProfileTag, J_JITOperation_EJJ operation, int dst, GPRReg arg1, GPRReg arg2)
{
    setupArgumentsWithExecState(arg1, arg2);
    return appendCallWithExceptionCheckSetJSValueResultWithProfile(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EAapJ operation, int dst, ArrayAllocationProfile* arg1, GPRReg arg2)
{
    setupArgumentsWithExecState(TrustedImmPtr(arg1), arg2);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJ operation, int dst, GPRReg arg1)
{
    setupArgumentsWithExecState(arg1);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJIdc operation, int dst, GPRReg arg1, const Identifier* arg2)
{
    setupArgumentsWithExecState(arg1, TrustedImmPtr(arg2));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJJ operation, int dst, GPRReg arg1, GPRReg arg2)
{
    setupArgumentsWithExecState(arg1, arg2);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperationNoExceptionCheck(V_JITOperation_EJ operation, GPRReg arg1)
{
    setupArgumentsWithExecState(arg1);
    updateTopCallFrame();
    return appendCall(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(P_JITOperation_EJS operation, GPRReg arg1, size_t arg2)
{
    setupArgumentsWithExecState(arg1, TrustedImmPtr(arg2));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_EJ operation, RegisterID regOp)
{
    setupArgumentsWithExecState(regOp);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_EJJ operation, RegisterID regOp1, RegisterID regOp2)
{
    setupArgumentsWithExecState(regOp1, regOp2);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EIdJZ operation, const Identifier* identOp1, RegisterID regOp2, int32_t op3)
{
    setupArgumentsWithExecState(TrustedImmPtr(identOp1), regOp2, TrustedImm32(op3));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJ operation, RegisterID regOp)
{
    setupArgumentsWithExecState(regOp);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJIdJJ operation, RegisterID regOp1, const Identifier* identOp2, RegisterID regOp3, RegisterID regOp4)
{
    setupArgumentsWithExecState(regOp1, TrustedImmPtr(identOp2), regOp3, regOp4);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJZ operation, RegisterID regOp1, int32_t op2)
{
    setupArgumentsWithExecState(regOp1, TrustedImm32(op2));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJZJ operation, RegisterID regOp1, int32_t op2, RegisterID regOp3)
{
    setupArgumentsWithExecState(regOp1, TrustedImm32(op2), regOp3);
    return appendCallWithExceptionCheck(operation);
}

#else // USE(JSVALUE32_64)

// EncodedJSValue in JSVALUE32_64 is a 64-bit integer. When being compiled in ARM EABI, it must be aligned even-numbered register (r0, r2 or [sp]).
// To avoid assemblies from using wrong registers, let's occupy r1 or r3 with a dummy argument when necessary.
#if (COMPILER_SUPPORTS(EABI) && CPU(ARM)) || CPU(MIPS)
#define EABI_32BIT_DUMMY_ARG      TrustedImm32(0),
#else
#define EABI_32BIT_DUMMY_ARG
#endif

// JSVALUE32_64 is a 64-bit integer that cannot be put half in an argument register and half on stack when using SH4 architecture.
// To avoid this, let's occupy the 4th argument register (r7) with a dummy argument when necessary. This must only be done when there
// is no other 32-bit value argument behind this 64-bit JSValue.
#if CPU(SH4)
#define SH4_32BIT_DUMMY_ARG      TrustedImm32(0),
#else
#define SH4_32BIT_DUMMY_ARG
#endif

ALWAYS_INLINE MacroAssembler::Call JIT::callOperationNoExceptionCheck(V_JITOperation_EJ operation, GPRReg arg1Tag, GPRReg arg1Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
    updateTopCallFrame();
    return appendCall(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(F_JITOperation_EJJZ operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload, int32_t arg3)
{
#if CPU(SH4)
    // We have to put arg3 in the 4th argument register (r7) as 64-bit value arg2 will be put on stack for sh4 architecure.
    setupArgumentsWithExecState(arg1Payload, arg1Tag, TrustedImm32(arg3), arg2Payload, arg2Tag);
#else
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, arg2Payload, arg2Tag, TrustedImm32(arg3));
#endif
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EAapJ operation, int dst, ArrayAllocationProfile* arg1, GPRReg arg2Tag, GPRReg arg2Payload)
{
    setupArgumentsWithExecState(TrustedImmPtr(arg1), arg2Payload, arg2Tag);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJ operation, int dst, GPRReg arg1Tag, GPRReg arg1Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(JIT::WithProfileTag, J_JITOperation_ESsiJI operation, int dst, StructureStubInfo* stubInfo, GPRReg arg1Tag, GPRReg arg1Payload, StringImpl* uid)
{
    setupArgumentsWithExecState(TrustedImmPtr(stubInfo), arg1Payload, arg1Tag, TrustedImmPtr(uid));
    return appendCallWithExceptionCheckSetJSValueResultWithProfile(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJIdc operation, int dst, GPRReg arg1Tag, GPRReg arg1Payload, const Identifier* arg2)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, TrustedImmPtr(arg2));
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(J_JITOperation_EJJ operation, int dst, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
    return appendCallWithExceptionCheckSetJSValueResult(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(JIT::WithProfileTag, J_JITOperation_EJJ operation, int dst, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
    return appendCallWithExceptionCheckSetJSValueResultWithProfile(operation, dst);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(P_JITOperation_EJS operation, GPRReg arg1Tag, GPRReg arg1Payload, size_t arg2)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, TrustedImmPtr(arg2));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_EJ operation, RegisterID argTag, RegisterID argPayload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG argPayload, argTag);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(S_JITOperation_EJJ operation, RegisterID arg1Tag, RegisterID arg1Payload, RegisterID arg2Tag, RegisterID arg2Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG arg1Payload, arg1Tag, SH4_32BIT_DUMMY_ARG arg2Payload, arg2Tag);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_ECICC operation, RegisterID regOp1, const Identifier* identOp2, RegisterID regOp3, RegisterID regOp4)
{
    setupArgumentsWithExecState(regOp1, TrustedImmPtr(identOp2), regOp3, regOp4);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJ operation, RegisterID regOp1Tag, RegisterID regOp1Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG regOp1Payload, regOp1Tag);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EIdJZ operation, const Identifier* identOp1, RegisterID regOp2Tag, RegisterID regOp2Payload, int32_t op3)
{
    setupArgumentsWithExecState(TrustedImmPtr(identOp1), regOp2Payload, regOp2Tag, TrustedImm32(op3));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_ESsiJJI operation, StructureStubInfo* stubInfo, RegisterID regOp1Tag, RegisterID regOp1Payload, RegisterID regOp2Tag, RegisterID regOp2Payload, StringImpl* uid)
{
    setupArgumentsWithExecState(TrustedImmPtr(stubInfo), regOp1Payload, regOp1Tag, regOp2Payload, regOp2Tag, TrustedImmPtr(uid));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJJJ operation, RegisterID regOp1Tag, RegisterID regOp1Payload, RegisterID regOp2Tag, RegisterID regOp2Payload, RegisterID regOp3Tag, RegisterID regOp3Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG regOp1Payload, regOp1Tag, SH4_32BIT_DUMMY_ARG regOp2Payload, regOp2Tag, regOp3Payload, regOp3Tag);
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJZ operation, RegisterID regOp1Tag, RegisterID regOp1Payload, int32_t op2)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG regOp1Payload, regOp1Tag, TrustedImm32(op2));
    return appendCallWithExceptionCheck(operation);
}

ALWAYS_INLINE MacroAssembler::Call JIT::callOperation(V_JITOperation_EJZJ operation, RegisterID regOp1Tag, RegisterID regOp1Payload, int32_t op2, RegisterID regOp3Tag, RegisterID regOp3Payload)
{
    setupArgumentsWithExecState(EABI_32BIT_DUMMY_ARG regOp1Payload, regOp1Tag, TrustedImm32(op2), regOp3Payload, regOp3Tag);
    return appendCallWithExceptionCheck(operation);
}

#undef EABI_32BIT_DUMMY_ARG
#undef SH4_32BIT_DUMMY_ARG

#endif // USE(JSVALUE32_64)

ALWAYS_INLINE JIT::Jump JIT::checkStructure(RegisterID reg, Structure* structure)
{
    return branchPtr(NotEqual, Address(reg, JSCell::structureOffset()), TrustedImmPtr(structure));
}

ALWAYS_INLINE void JIT::linkSlowCaseIfNotJSCell(Vector<SlowCaseEntry>::iterator& iter, int vReg)
{
    if (!m_codeBlock->isKnownNotImmediate(vReg))
        linkSlowCase(iter);
}

ALWAYS_INLINE void JIT::addSlowCase(Jump jump)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    m_slowCases.append(SlowCaseEntry(jump, m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addSlowCase(JumpList jumpList)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    const JumpList::JumpVector& jumpVector = jumpList.jumps();
    size_t size = jumpVector.size();
    for (size_t i = 0; i < size; ++i)
        m_slowCases.append(SlowCaseEntry(jumpVector[i], m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addSlowCase()
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.
    
    Jump emptyJump; // Doing it this way to make Windows happy.
    m_slowCases.append(SlowCaseEntry(emptyJump, m_bytecodeOffset));
}

ALWAYS_INLINE void JIT::addJump(Jump jump, int relativeOffset)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    m_jmpTable.append(JumpTable(jump, m_bytecodeOffset + relativeOffset));
}

ALWAYS_INLINE void JIT::emitJumpSlowToHot(Jump jump, int relativeOffset)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    jump.linkTo(m_labels[m_bytecodeOffset + relativeOffset], this);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotObject(RegisterID structureReg)
{
    return branch8(Below, Address(structureReg, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType));
}

#if ENABLE(SAMPLING_FLAGS)
ALWAYS_INLINE void JIT::setSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    or32(TrustedImm32(1u << (flag - 1)), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}

ALWAYS_INLINE void JIT::clearSamplingFlag(int32_t flag)
{
    ASSERT(flag >= 1);
    ASSERT(flag <= 32);
    and32(TrustedImm32(~(1u << (flag - 1))), AbsoluteAddress(SamplingFlags::addressOfFlags()));
}
#endif

#if ENABLE(SAMPLING_COUNTERS)
ALWAYS_INLINE void JIT::emitCount(AbstractSamplingCounter& counter, int32_t count)
{
    add64(TrustedImm32(count), AbsoluteAddress(counter.addressOfCounter()));
}
#endif

#if ENABLE(OPCODE_SAMPLING)
#if CPU(X86_64)
ALWAYS_INLINE void JIT::sampleInstruction(Instruction* instruction, bool inHostFunction)
{
    move(TrustedImmPtr(m_interpreter->sampler()->sampleSlot()), X86Registers::ecx);
    storePtr(TrustedImmPtr(m_interpreter->sampler()->encodeSample(instruction, inHostFunction)), X86Registers::ecx);
}
#else
ALWAYS_INLINE void JIT::sampleInstruction(Instruction* instruction, bool inHostFunction)
{
    storePtr(TrustedImmPtr(m_interpreter->sampler()->encodeSample(instruction, inHostFunction)), m_interpreter->sampler()->sampleSlot());
}
#endif
#endif

#if ENABLE(CODEBLOCK_SAMPLING)
#if CPU(X86_64)
ALWAYS_INLINE void JIT::sampleCodeBlock(CodeBlock* codeBlock)
{
    move(TrustedImmPtr(m_interpreter->sampler()->codeBlockSlot()), X86Registers::ecx);
    storePtr(TrustedImmPtr(codeBlock), X86Registers::ecx);
}
#else
ALWAYS_INLINE void JIT::sampleCodeBlock(CodeBlock* codeBlock)
{
    storePtr(TrustedImmPtr(codeBlock), m_interpreter->sampler()->codeBlockSlot());
}
#endif
#endif

ALWAYS_INLINE bool JIT::isOperandConstantImmediateChar(int src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isString() && asString(getConstantOperand(src).asCell())->length() == 1;
}

template<typename StructureType>
inline void JIT::emitAllocateJSObject(RegisterID allocator, StructureType structure, RegisterID result, RegisterID scratch)
{
    loadPtr(Address(allocator, MarkedAllocator::offsetOfFreeListHead()), result);
    addSlowCase(branchTestPtr(Zero, result));

    // remove the object from the free list
    loadPtr(Address(result), scratch);
    storePtr(scratch, Address(allocator, MarkedAllocator::offsetOfFreeListHead()));

    // initialize the object's structure
    storePtr(structure, Address(result, JSCell::structureOffset()));

    // initialize the object's property storage pointer
    storePtr(TrustedImmPtr(0), Address(result, JSObject::butterflyOffset()));
}

#if ENABLE(VALUE_PROFILER)
inline void JIT::emitValueProfilingSite(ValueProfile* valueProfile, RegisterID bucketCounterRegister)
{
    ASSERT(shouldEmitProfiling());
    ASSERT(valueProfile);

    const RegisterID value = regT0;
#if USE(JSVALUE32_64)
    const RegisterID valueTag = regT1;
#endif
    const RegisterID scratch = regT3;
    
    if (ValueProfile::numberOfBuckets == 1) {
        // We're in a simple configuration: only one bucket, so we can just do a direct
        // store.
#if USE(JSVALUE64)
        store64(value, valueProfile->m_buckets);
#else
        EncodedValueDescriptor* descriptor = bitwise_cast<EncodedValueDescriptor*>(valueProfile->m_buckets);
        store32(value, &descriptor->asBits.payload);
        store32(valueTag, &descriptor->asBits.tag);
#endif
        return;
    }
    
    if (m_randomGenerator.getUint32() & 1)
        add32(TrustedImm32(1), bucketCounterRegister);
    else
        add32(TrustedImm32(3), bucketCounterRegister);
    and32(TrustedImm32(ValueProfile::bucketIndexMask), bucketCounterRegister);
    move(TrustedImmPtr(valueProfile->m_buckets), scratch);
#if USE(JSVALUE64)
    store64(value, BaseIndex(scratch, bucketCounterRegister, TimesEight));
#elif USE(JSVALUE32_64)
    store32(value, BaseIndex(scratch, bucketCounterRegister, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
    store32(valueTag, BaseIndex(scratch, bucketCounterRegister, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
#endif
}

inline void JIT::emitValueProfilingSite(unsigned bytecodeOffset, RegisterID bucketCounterRegister)
{
    if (!shouldEmitProfiling())
        return;
    emitValueProfilingSite(m_codeBlock->valueProfileForBytecodeOffset(bytecodeOffset), bucketCounterRegister);
}

inline void JIT::emitValueProfilingSite(RegisterID bucketCounterRegister)
{
    emitValueProfilingSite(m_bytecodeOffset, bucketCounterRegister);
}
#endif // ENABLE(VALUE_PROFILER)

inline void JIT::emitArrayProfilingSite(RegisterID structureAndIndexingType, RegisterID scratch, ArrayProfile* arrayProfile)
{
    UNUSED_PARAM(scratch); // We had found this scratch register useful here before, so I will keep it for now.
    
    RegisterID structure = structureAndIndexingType;
    RegisterID indexingType = structureAndIndexingType;
    
    if (shouldEmitProfiling())
        storePtr(structure, arrayProfile->addressOfLastSeenStructure());

    load8(Address(structure, Structure::indexingTypeOffset()), indexingType);
}

inline void JIT::emitArrayProfilingSiteForBytecodeIndex(RegisterID structureAndIndexingType, RegisterID scratch, unsigned bytecodeIndex)
{
#if ENABLE(VALUE_PROFILER)
    emitArrayProfilingSite(structureAndIndexingType, scratch, m_codeBlock->getOrAddArrayProfile(bytecodeIndex));
#else
    UNUSED_PARAM(bytecodeIndex);
    emitArrayProfilingSite(structureAndIndexingType, scratch, 0);
#endif
}

inline void JIT::emitArrayProfileStoreToHoleSpecialCase(ArrayProfile* arrayProfile)
{
#if ENABLE(VALUE_PROFILER)    
    store8(TrustedImm32(1), arrayProfile->addressOfMayStoreToHole());
#else
    UNUSED_PARAM(arrayProfile);
#endif
}

inline void JIT::emitArrayProfileOutOfBoundsSpecialCase(ArrayProfile* arrayProfile)
{
#if ENABLE(VALUE_PROFILER)    
    store8(TrustedImm32(1), arrayProfile->addressOfOutOfBounds());
#else
    UNUSED_PARAM(arrayProfile);
#endif
}

static inline bool arrayProfileSaw(ArrayModes arrayModes, IndexingType capability)
{
#if ENABLE(VALUE_PROFILER)
    return arrayModesInclude(arrayModes, capability);
#else
    UNUSED_PARAM(arrayModes);
    UNUSED_PARAM(capability);
    return false;
#endif
}

inline JITArrayMode JIT::chooseArrayMode(ArrayProfile* profile)
{
#if ENABLE(VALUE_PROFILER)
    ConcurrentJITLocker locker(m_codeBlock->m_lock);
    profile->computeUpdatedPrediction(locker, m_codeBlock);
    ArrayModes arrayModes = profile->observedArrayModes(locker);
    if (arrayProfileSaw(arrayModes, DoubleShape))
        return JITDouble;
    if (arrayProfileSaw(arrayModes, Int32Shape))
        return JITInt32;
    if (arrayProfileSaw(arrayModes, ArrayStorageShape))
        return JITArrayStorage;
    return JITContiguous;
#else
    UNUSED_PARAM(profile);
    return JITContiguous;
#endif
}

#if USE(JSVALUE32_64)

inline void JIT::emitLoadTag(int index, RegisterID tag)
{
    RegisterID mappedTag;
    if (getMappedTag(index, mappedTag)) {
        move(mappedTag, tag);
        unmap(tag);
        return;
    }

    if (m_codeBlock->isConstantRegisterIndex(index)) {
        move(Imm32(getConstantOperand(index).tag()), tag);
        unmap(tag);
        return;
    }

    load32(tagFor(index), tag);
    unmap(tag);
}

inline void JIT::emitLoadPayload(int index, RegisterID payload)
{
    RegisterID mappedPayload;
    if (getMappedPayload(index, mappedPayload)) {
        move(mappedPayload, payload);
        unmap(payload);
        return;
    }

    if (m_codeBlock->isConstantRegisterIndex(index)) {
        move(Imm32(getConstantOperand(index).payload()), payload);
        unmap(payload);
        return;
    }

    load32(payloadFor(index), payload);
    unmap(payload);
}

inline void JIT::emitLoad(const JSValue& v, RegisterID tag, RegisterID payload)
{
    move(Imm32(v.payload()), payload);
    move(Imm32(v.tag()), tag);
}

inline void JIT::emitLoad(int index, RegisterID tag, RegisterID payload, RegisterID base)
{
    RELEASE_ASSERT(tag != payload);

    if (base == callFrameRegister) {
        RELEASE_ASSERT(payload != base);
        emitLoadPayload(index, payload);
        emitLoadTag(index, tag);
        return;
    }

    if (payload == base) { // avoid stomping base
        load32(tagFor(index, base), tag);
        load32(payloadFor(index, base), payload);
        return;
    }

    load32(payloadFor(index, base), payload);
    load32(tagFor(index, base), tag);
}

inline void JIT::emitLoad2(int index1, RegisterID tag1, RegisterID payload1, int index2, RegisterID tag2, RegisterID payload2)
{
    if (isMapped(index1)) {
        emitLoad(index1, tag1, payload1);
        emitLoad(index2, tag2, payload2);
        return;
    }
    emitLoad(index2, tag2, payload2);
    emitLoad(index1, tag1, payload1);
}

inline void JIT::emitLoadDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        char* bytePointer = reinterpret_cast<char*>(&inConstantPool);
        convertInt32ToDouble(AbsoluteAddress(bytePointer + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), value);
    } else
        convertInt32ToDouble(payloadFor(index), value);
}

inline void JIT::emitStore(int index, RegisterID tag, RegisterID payload, RegisterID base)
{
    store32(payload, payloadFor(index, base));
    store32(tag, tagFor(index, base));
}

inline void JIT::emitStoreInt32(int index, RegisterID payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreAndMapInt32(int index, RegisterID tag, RegisterID payload, bool indexIsInt32, size_t opcodeLength)
{
    emitStoreInt32(index, payload, indexIsInt32);
    map(m_bytecodeOffset + opcodeLength, index, tag, payload);
}

inline void JIT::emitStoreInt32(int index, TrustedImm32 payload, bool indexIsInt32)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsInt32)
        store32(TrustedImm32(JSValue::Int32Tag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreCell(int index, RegisterID payload, bool indexIsCell)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsCell)
        store32(TrustedImm32(JSValue::CellTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreBool(int index, RegisterID payload, bool indexIsBool)
{
    store32(payload, payloadFor(index, callFrameRegister));
    if (!indexIsBool)
        store32(TrustedImm32(JSValue::BooleanTag), tagFor(index, callFrameRegister));
}

inline void JIT::emitStoreDouble(int index, FPRegisterID value)
{
    storeDouble(value, addressFor(index));
}

inline void JIT::emitStore(int index, const JSValue constant, RegisterID base)
{
    store32(Imm32(constant.payload()), payloadFor(index, base));
    store32(Imm32(constant.tag()), tagFor(index, base));
}

ALWAYS_INLINE void JIT::emitInitRegister(int dst)
{
    emitStore(dst, jsUndefined());
}

inline bool JIT::isLabeled(unsigned bytecodeOffset)
{
    for (size_t numberOfJumpTargets = m_codeBlock->numberOfJumpTargets(); m_jumpTargetIndex != numberOfJumpTargets; ++m_jumpTargetIndex) {
        unsigned jumpTarget = m_codeBlock->jumpTarget(m_jumpTargetIndex);
        if (jumpTarget == bytecodeOffset)
            return true;
        if (jumpTarget > bytecodeOffset)
            return false;
    }
    return false;
}

inline void JIT::map(unsigned bytecodeOffset, int virtualRegisterIndex, RegisterID tag, RegisterID payload)
{
    if (isLabeled(bytecodeOffset))
        return;

    m_mappedBytecodeOffset = bytecodeOffset;
    m_mappedVirtualRegisterIndex = virtualRegisterIndex;
    m_mappedTag = tag;
    m_mappedPayload = payload;
    
    ASSERT(!canBeOptimizedOrInlined() || m_mappedPayload == regT0);
    ASSERT(!canBeOptimizedOrInlined() || m_mappedTag == regT1);
}

inline void JIT::unmap(RegisterID registerID)
{
    if (m_mappedTag == registerID)
        m_mappedTag = (RegisterID)-1;
    else if (m_mappedPayload == registerID)
        m_mappedPayload = (RegisterID)-1;
}

inline void JIT::unmap()
{
    m_mappedBytecodeOffset = (unsigned)-1;
    m_mappedVirtualRegisterIndex = UINT_MAX;
    m_mappedTag = (RegisterID)-1;
    m_mappedPayload = (RegisterID)-1;
}

inline bool JIT::isMapped(int virtualRegisterIndex)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    return true;
}

inline bool JIT::getMappedPayload(int virtualRegisterIndex, RegisterID& payload)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    if (m_mappedPayload == (RegisterID)-1)
        return false;
    payload = m_mappedPayload;
    return true;
}

inline bool JIT::getMappedTag(int virtualRegisterIndex, RegisterID& tag)
{
    if (m_mappedBytecodeOffset != m_bytecodeOffset)
        return false;
    if (m_mappedVirtualRegisterIndex != virtualRegisterIndex)
        return false;
    if (m_mappedTag == (RegisterID)-1)
        return false;
    tag = m_mappedTag;
    return true;
}

inline void JIT::emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(emitJumpIfNotJSCell(virtualRegisterIndex));
    }
}

inline void JIT::emitJumpSlowCaseIfNotJSCell(int virtualRegisterIndex, RegisterID tag)
{
    if (!m_codeBlock->isKnownNotImmediate(virtualRegisterIndex)) {
        if (m_codeBlock->isConstantRegisterIndex(virtualRegisterIndex))
            addSlowCase(jump());
        else
            addSlowCase(branch32(NotEqual, tag, TrustedImm32(JSValue::CellTag)));
    }
}

ALWAYS_INLINE bool JIT::isOperandConstantImmediateInt(int src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isInt32();
}

ALWAYS_INLINE bool JIT::getOperandConstantImmediateInt(int op1, int op2, int& op, int32_t& constant)
{
    if (isOperandConstantImmediateInt(op1)) {
        constant = getConstantOperand(op1).asInt32();
        op = op2;
        return true;
    }

    if (isOperandConstantImmediateInt(op2)) {
        constant = getConstantOperand(op2).asInt32();
        op = op1;
        return true;
    }
    
    return false;
}

#else // USE(JSVALUE32_64)

ALWAYS_INLINE void JIT::killLastResultRegister()
{
    m_lastResultBytecodeRegister = std::numeric_limits<int>::max();
}

// get arg puts an arg from the SF register array into a h/w register
ALWAYS_INLINE void JIT::emitGetVirtualRegister(int src, RegisterID dst)
{
    ASSERT(m_bytecodeOffset != (unsigned)-1); // This method should only be called during hot/cold path generation, so that m_bytecodeOffset is set.

    // TODO: we want to reuse values that are already in registers if we can - add a register allocator!
    if (m_codeBlock->isConstantRegisterIndex(src)) {
        JSValue value = m_codeBlock->getConstant(src);
        if (!value.isNumber())
            move(TrustedImm64(JSValue::encode(value)), dst);
        else
            move(Imm64(JSValue::encode(value)), dst);
        killLastResultRegister();
        return;
    }

    if (src == m_lastResultBytecodeRegister && operandIsLocal(src) && m_codeBlock->isTemporaryRegisterIndex(VirtualRegister(src).toLocal()) && !atJumpTarget()) {
        // The argument we want is already stored in eax
        if (dst != cachedResultRegister)
            move(cachedResultRegister, dst);
        killLastResultRegister();
        return;
    }

    load64(Address(callFrameRegister, src * sizeof(Register)), dst);
    killLastResultRegister();
}

ALWAYS_INLINE void JIT::emitGetVirtualRegister(VirtualRegister src, RegisterID dst)
{
    emitGetVirtualRegister(src.offset(), dst);
}

ALWAYS_INLINE void JIT::emitGetVirtualRegisters(int src1, RegisterID dst1, int src2, RegisterID dst2)
{
    if (src2 == m_lastResultBytecodeRegister) {
        emitGetVirtualRegister(src2, dst2);
        emitGetVirtualRegister(src1, dst1);
    } else {
        emitGetVirtualRegister(src1, dst1);
        emitGetVirtualRegister(src2, dst2);
    }
}

ALWAYS_INLINE void JIT::emitGetVirtualRegisters(VirtualRegister src1, RegisterID dst1, VirtualRegister src2, RegisterID dst2)
{
    emitGetVirtualRegisters(src1.offset(), dst1, src2.offset(), dst2);
}

ALWAYS_INLINE int32_t JIT::getConstantOperandImmediateInt(int src)
{
    return getConstantOperand(src).asInt32();
}

ALWAYS_INLINE bool JIT::isOperandConstantImmediateInt(int src)
{
    return m_codeBlock->isConstantRegisterIndex(src) && getConstantOperand(src).isInt32();
}

ALWAYS_INLINE void JIT::emitPutVirtualRegister(int dst, RegisterID from)
{
    store64(from, Address(callFrameRegister, dst * sizeof(Register)));
    m_lastResultBytecodeRegister = (from == cachedResultRegister) ? dst : std::numeric_limits<int>::max();
}

ALWAYS_INLINE void JIT::emitPutVirtualRegister(VirtualRegister dst, RegisterID from)
{
    emitPutVirtualRegister(dst.offset(), from);
}

ALWAYS_INLINE void JIT::emitInitRegister(int dst)
{
    store64(TrustedImm64(JSValue::encode(jsUndefined())), Address(callFrameRegister, dst * sizeof(Register)));
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfJSCell(RegisterID reg)
{
    return branchTest64(Zero, reg, tagMaskRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfBothJSCells(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    or64(reg2, scratch);
    return emitJumpIfJSCell(scratch);
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfJSCell(RegisterID reg)
{
    addSlowCase(emitJumpIfJSCell(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotJSCell(RegisterID reg)
{
    addSlowCase(emitJumpIfNotJSCell(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotJSCell(RegisterID reg, int vReg)
{
    if (!m_codeBlock->isKnownNotImmediate(vReg))
        emitJumpSlowCaseIfNotJSCell(reg);
}

inline void JIT::emitLoadDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        WriteBarrier<Unknown>& inConstantPool = m_codeBlock->constantRegister(index);
        loadDouble(&inConstantPool, value);
    } else
        loadDouble(addressFor(index), value);
}

inline void JIT::emitLoadInt32ToDouble(int index, FPRegisterID value)
{
    if (m_codeBlock->isConstantRegisterIndex(index)) {
        ASSERT(isOperandConstantImmediateInt(index));
        convertInt32ToDouble(Imm32(getConstantOperand(index).asInt32()), value);
    } else
        convertInt32ToDouble(addressFor(index), value);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfImmediateInteger(RegisterID reg)
{
    return branch64(AboveOrEqual, reg, tagTypeNumberRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateInteger(RegisterID reg)
{
    return branch64(Below, reg, tagTypeNumberRegister);
}

ALWAYS_INLINE JIT::Jump JIT::emitJumpIfNotImmediateIntegers(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    move(reg1, scratch);
    and64(reg2, scratch);
    return emitJumpIfNotImmediateInteger(scratch);
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateInteger(RegisterID reg)
{
    addSlowCase(emitJumpIfNotImmediateInteger(reg));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateIntegers(RegisterID reg1, RegisterID reg2, RegisterID scratch)
{
    addSlowCase(emitJumpIfNotImmediateIntegers(reg1, reg2, scratch));
}

ALWAYS_INLINE void JIT::emitJumpSlowCaseIfNotImmediateNumber(RegisterID reg)
{
    addSlowCase(emitJumpIfNotImmediateNumber(reg));
}

ALWAYS_INLINE void JIT::emitFastArithReTagImmediate(RegisterID src, RegisterID dest)
{
    emitFastArithIntToImmNoCheck(src, dest);
}

ALWAYS_INLINE void JIT::emitTagAsBoolImmediate(RegisterID reg)
{
    or32(TrustedImm32(static_cast<int32_t>(ValueFalse)), reg);
}

#endif // USE(JSVALUE32_64)

} // namespace JSC

#endif // ENABLE(JIT)

#endif // JITInlines_h

