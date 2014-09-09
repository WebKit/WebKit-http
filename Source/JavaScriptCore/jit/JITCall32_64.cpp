/*
 * Copyright (C) 2008, 2013, 2014 Apple Inc. All rights reserved.
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
#if USE(JSVALUE32_64)
#include "JIT.h"

#include "Arguments.h"
#include "CodeBlock.h"
#include "Interpreter.h"
#include "JITInlines.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSCInlines.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include "StackAlignment.h"
#include <wtf/StringPrintStream.h>


namespace JSC {

void JIT::emitPutCallResult(Instruction* instruction)
{
    int dst = instruction[1].u.operand;
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
}

void JIT::emit_op_ret(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;

    emitLoad(dst, regT1, regT0);

    checkStackPointerAlignment();
    emitFunctionEpilogue();
    ret();
}

void JIT::emit_op_ret_object_or_this(Instruction* currentInstruction)
{
    unsigned result = currentInstruction[1].u.operand;
    unsigned thisReg = currentInstruction[2].u.operand;

    emitLoad(result, regT1, regT0);
    Jump notJSCell = branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag));
    Jump notObject = emitJumpIfCellNotObject(regT0);

    checkStackPointerAlignment();
    emitFunctionEpilogue();
    ret();

    notJSCell.link(this);
    notObject.link(this);
    emitLoad(thisReg, regT1, regT0);

    checkStackPointerAlignment();
    emitFunctionEpilogue();
    ret();
}

void JIT::emitSlow_op_call(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emitSlow_op_call_eval(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_eval, currentInstruction, iter, m_callLinkInfoIndex);
}
 
void JIT::emitSlow_op_call_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_call_varargs, currentInstruction, iter, m_callLinkInfoIndex++);
}
    
void JIT::emitSlow_op_construct_varargs(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_construct_varargs, currentInstruction, iter, m_callLinkInfoIndex++);
}
    
void JIT::emitSlow_op_construct(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    compileOpCallSlowCase(op_construct, currentInstruction, iter, m_callLinkInfoIndex++);
}

void JIT::emit_op_call(Instruction* currentInstruction)
{
    compileOpCall(op_call, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::emit_op_call_eval(Instruction* currentInstruction)
{
    compileOpCall(op_call_eval, currentInstruction, m_callLinkInfoIndex);
}

void JIT::emit_op_call_varargs(Instruction* currentInstruction)
{
    compileOpCall(op_call_varargs, currentInstruction, m_callLinkInfoIndex++);
}
    
void JIT::emit_op_construct_varargs(Instruction* currentInstruction)
{
    compileOpCall(op_construct_varargs, currentInstruction, m_callLinkInfoIndex++);
}
    
void JIT::emit_op_construct(Instruction* currentInstruction)
{
    compileOpCall(op_construct, currentInstruction, m_callLinkInfoIndex++);
}

void JIT::compileLoadVarargs(Instruction* instruction)
{
    int thisValue = instruction[3].u.operand;
    int arguments = instruction[4].u.operand;
    int firstFreeRegister = instruction[5].u.operand;
    int firstVarArgOffset = instruction[6].u.operand;

    JumpList slowCase;
    JumpList end;
    bool canOptimize = m_codeBlock->usesArguments()
        && VirtualRegister(arguments) == m_codeBlock->argumentsRegister()
        && !m_codeBlock->symbolTable()->slowArguments();

    if (canOptimize) {
        emitLoadTag(arguments, regT1);
        slowCase.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::EmptyValueTag)));

        load32(payloadFor(JSStack::ArgumentCount), regT2);
        if (firstVarArgOffset) {
            Jump sufficientArguments = branch32(GreaterThan, regT2, TrustedImm32(firstVarArgOffset + 1));
            move(TrustedImm32(1), regT2);
            Jump endVarArgs = jump();
            sufficientArguments.link(this);
            sub32(TrustedImm32(firstVarArgOffset), regT2);
            endVarArgs.link(this);
        }
        slowCase.append(branch32(Above, regT2, TrustedImm32(Arguments::MaxArguments + 1)));
        // regT2: argumentCountIncludingThis

        move(regT2, regT3);
        addPtr(TrustedImm32(-firstFreeRegister + JSStack::CallFrameHeaderSize), regT3);
        // regT1 now has the required frame size in Register units
        // Round regT1 to next multiple of stackAlignmentRegisters()
        addPtr(TrustedImm32(stackAlignmentRegisters() - 1), regT3);
        andPtr(TrustedImm32(~(stackAlignmentRegisters() - 1)), regT3);
        neg32(regT3);
        lshift32(TrustedImm32(3), regT3);
        addPtr(callFrameRegister, regT3);
        // regT3: newCallFrame

        slowCase.append(branchPtr(Above, AbsoluteAddress(m_vm->addressOfStackLimit()), regT3));

        // Initialize ArgumentCount.
        store32(regT2, payloadFor(JSStack::ArgumentCount, regT3));

        // Initialize 'this'.
        emitLoad(thisValue, regT1, regT0);
        store32(regT0, Address(regT3, OBJECT_OFFSETOF(JSValue, u.asBits.payload) + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        store32(regT1, Address(regT3, OBJECT_OFFSETOF(JSValue, u.asBits.tag) + (CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));

        // Copy arguments.
        end.append(branchSub32(Zero, TrustedImm32(1), regT2));
        // regT2: argumentCount;

        Label copyLoop = label();
        load32(BaseIndex(callFrameRegister, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) +((CallFrame::thisArgumentOffset() + firstVarArgOffset) * static_cast<int>(sizeof(Register)))), regT0);
        load32(BaseIndex(callFrameRegister, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) +((CallFrame::thisArgumentOffset() + firstVarArgOffset) * static_cast<int>(sizeof(Register)))), regT1);
        store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) +(CallFrame::thisArgumentOffset() * static_cast<int>(sizeof(Register)))));
        branchSub32(NonZero, TrustedImm32(1), regT2).linkTo(copyLoop, this);

        end.append(jump());
    }

    if (canOptimize)
        slowCase.link(this);

    emitLoad(arguments, regT1, regT0);
    callOperation(operationSizeFrameForVarargs, regT1, regT0, firstFreeRegister, firstVarArgOffset);
    addPtr(TrustedImm32(-sizeof(CallerFrameAndPC)), returnValueGPR, stackPointerRegister);
    emitLoad(thisValue, regT1, regT4);
    emitLoad(arguments, regT3, regT2);
    callOperation(operationLoadVarargs, returnValueGPR, regT1, regT4, regT3, regT2, firstVarArgOffset);
    move(returnValueGPR, regT3);

    if (canOptimize)
        end.link(this);

    addPtr(TrustedImm32(sizeof(CallerFrameAndPC)), regT3, stackPointerRegister);
}

void JIT::compileCallEval(Instruction* instruction)
{
    addPtr(TrustedImm32(-static_cast<ptrdiff_t>(sizeof(CallerFrameAndPC))), stackPointerRegister, regT1);

    callOperationNoExceptionCheck(operationCallEval, regT1);

    Jump noException = emitExceptionCheck(InvertedExceptionCheck);
    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    exceptionCheck(jump());

    noException.link(this);
    addSlowCase(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    
    emitPutCallResult(instruction);
}

void JIT::compileCallEvalSlowCase(Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter)
{
    linkSlowCase(iter);

    loadPtr(Address(stackPointerRegister, sizeof(Register) * JSStack::Callee - sizeof(CallerFrameAndPC)), regT0);
    loadPtr(Address(stackPointerRegister, sizeof(Register) * JSStack::Callee - sizeof(CallerFrameAndPC)), regT1);
    move(TrustedImmPtr(&CallLinkInfo::dummy()), regT2);

    emitLoad(JSStack::Callee, regT1, regT0);
    emitNakedCall(m_vm->getCTIStub(virtualCallThunkGenerator).code());
    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    
    emitPutCallResult(instruction);
}

void JIT::compileOpCall(OpcodeID opcodeID, Instruction* instruction, unsigned callLinkInfoIndex)
{
    int callee = instruction[2].u.operand;

    /* Caller always:
        - Updates callFrameRegister to callee callFrame.
        - Initializes ArgumentCount; CallerFrame; Callee.

       For a JS call:
        - Caller initializes ScopeChain.
        - Callee initializes ReturnPC; CodeBlock.
        - Callee restores callFrameRegister before return.

       For a non-JS call:
        - Caller initializes ScopeChain; ReturnPC; CodeBlock.
        - Caller restores callFrameRegister after return.
    */
    
    if (opcodeID == op_call_varargs || opcodeID == op_construct_varargs)
        compileLoadVarargs(instruction);
    else {
        int argCount = instruction[3].u.operand;
        int registerOffset = -instruction[4].u.operand;
        
        if (opcodeID == op_call && shouldEmitProfiling()) {
            emitLoad(registerOffset + CallFrame::argumentOffsetIncludingThis(0), regT0, regT1);
            Jump done = branch32(NotEqual, regT0, TrustedImm32(JSValue::CellTag));
            loadPtr(Address(regT1, JSCell::structureIDOffset()), regT1);
            storePtr(regT1, instruction[OPCODE_LENGTH(op_call) - 2].u.arrayProfile->addressOfLastSeenStructureID());
            done.link(this);
        }
    
        addPtr(TrustedImm32(registerOffset * sizeof(Register) + sizeof(CallerFrameAndPC)), callFrameRegister, stackPointerRegister);

        store32(TrustedImm32(argCount), Address(stackPointerRegister, JSStack::ArgumentCount * static_cast<int>(sizeof(Register)) + PayloadOffset - sizeof(CallerFrameAndPC)));
    } // SP holds newCallFrame + sizeof(CallerFrameAndPC), with ArgumentCount initialized.
    
    uint32_t locationBits = CallFrame::Location::encodeAsBytecodeInstruction(instruction);
    store32(TrustedImm32(locationBits), tagFor(JSStack::ArgumentCount, callFrameRegister));
    emitLoad(callee, regT1, regT0); // regT1, regT0 holds callee.

    store32(regT0, Address(stackPointerRegister, JSStack::Callee * static_cast<int>(sizeof(Register)) + PayloadOffset - sizeof(CallerFrameAndPC)));
    store32(regT1, Address(stackPointerRegister, JSStack::Callee * static_cast<int>(sizeof(Register)) + TagOffset - sizeof(CallerFrameAndPC)));

    if (opcodeID == op_call_eval) {
        compileCallEval(instruction);
        return;
    }

    addSlowCase(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));

    DataLabelPtr addressOfLinkedFunctionCheck;
    Jump slowCase = branchPtrWithPatch(NotEqual, regT0, addressOfLinkedFunctionCheck, TrustedImmPtr(0));

    addSlowCase(slowCase);

    ASSERT(m_callCompilationInfo.size() == callLinkInfoIndex);
    CallLinkInfo* info = m_codeBlock->addCallLinkInfo();
    info->callType = CallLinkInfo::callTypeFor(opcodeID);
    info->codeOrigin = CodeOrigin(m_bytecodeOffset);
    info->calleeGPR = regT0;
    m_callCompilationInfo.append(CallCompilationInfo());
    m_callCompilationInfo[callLinkInfoIndex].hotPathBegin = addressOfLinkedFunctionCheck;
    m_callCompilationInfo[callLinkInfoIndex].callLinkInfo = info;

    loadPtr(Address(regT0, OBJECT_OFFSETOF(JSFunction, m_scope)), regT2);
    store32(regT2, Address(MacroAssembler::stackPointerRegister, JSStack::ScopeChain * sizeof(Register) + PayloadOffset - sizeof(CallerFrameAndPC)));
    store32(TrustedImm32(JSValue::CellTag), Address(stackPointerRegister, JSStack::ScopeChain * sizeof(Register) + TagOffset - sizeof(CallerFrameAndPC)));

    checkStackPointerAlignment();
    m_callCompilationInfo[callLinkInfoIndex].hotPathOther = emitNakedCall();

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    emitPutCallResult(instruction);
}

void JIT::compileOpCallSlowCase(OpcodeID opcodeID, Instruction* instruction, Vector<SlowCaseEntry>::iterator& iter, unsigned callLinkInfoIndex)
{
    if (opcodeID == op_call_eval) {
        compileCallEvalSlowCase(instruction, iter);
        return;
    }

    linkSlowCase(iter);
    linkSlowCase(iter);

    ThunkGenerator generator = linkThunkGeneratorFor(
        (opcodeID == op_construct || opcodeID == op_construct_varargs) ? CodeForConstruct : CodeForCall,
        RegisterPreservationNotRequired);
    
    move(TrustedImmPtr(m_callCompilationInfo[callLinkInfoIndex].callLinkInfo), regT2);
    m_callCompilationInfo[callLinkInfoIndex].callReturnLocation = emitNakedCall(m_vm->getCTIStub(generator).code());

    addPtr(TrustedImm32(stackPointerOffsetFor(m_codeBlock) * sizeof(Register)), callFrameRegister, stackPointerRegister);
    checkStackPointerAlignment();

    sampleCodeBlock(m_codeBlock);
    emitPutCallResult(instruction);
}

void JIT::privateCompileClosureCall(CallLinkInfo* callLinkInfo, CodeBlock* calleeCodeBlock, Structure* expectedStructure, ExecutableBase* expectedExecutable, MacroAssemblerCodePtr codePtr)
{
    JumpList slowCases;

    slowCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureIDOffset()), TrustedImmPtr(expectedStructure)));
    slowCases.append(branchPtr(NotEqual, Address(regT0, JSFunction::offsetOfExecutable()), TrustedImmPtr(expectedExecutable)));
    
    loadPtr(Address(regT0, JSFunction::offsetOfScopeChain()), regT1);
    emitPutCellToCallFrameHeader(regT1, JSStack::ScopeChain);
    
    Call call = nearCall();
    Jump done = jump();
    
    slowCases.link(this);
    move(TrustedImmPtr(callLinkInfo->callReturnLocation.executableAddress()), regT2);
    restoreReturnAddressBeforeReturn(regT2);
    Jump slow = jump();
    
    LinkBuffer patchBuffer(*m_vm, *this, m_codeBlock);
    
    patchBuffer.link(call, FunctionPtr(codePtr.executableAddress()));
    patchBuffer.link(done, callLinkInfo->hotPathOther.labelAtOffset(0));
    patchBuffer.link(slow, CodeLocationLabel(m_vm->getCTIStub(virtualCallThunkGenerator).code()));
    
    RefPtr<ClosureCallStubRoutine> stubRoutine = adoptRef(new ClosureCallStubRoutine(
        FINALIZE_CODE(
            patchBuffer,
            ("Baseline closure call stub for %s, return point %p, target %p (%s)",
                toCString(*m_codeBlock).data(),
                callLinkInfo->hotPathOther.labelAtOffset(0).executableAddress(),
                codePtr.executableAddress(),
                toCString(pointerDump(calleeCodeBlock)).data())),
        *m_vm, m_codeBlock->ownerExecutable(), expectedStructure, expectedExecutable,
        callLinkInfo->codeOrigin));
    
    RepatchBuffer repatchBuffer(m_codeBlock);
    
    repatchBuffer.replaceWithJump(
        RepatchBuffer::startOfBranchPtrWithPatchOnRegister(callLinkInfo->hotPathBegin),
        CodeLocationLabel(stubRoutine->code().code()));
    repatchBuffer.relink(callLinkInfo->callReturnLocation, m_vm->getCTIStub(virtualCallThunkGenerator).code());
    
    callLinkInfo->stub = stubRoutine.release();
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
