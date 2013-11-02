/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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

#include "CodeBlock.h"
#include "GCAwareJITStubRoutine.h"
#include "Interpreter.h"
#include "JITInlines.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "JSVariableObject.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"
#include <wtf/StringPrintStream.h>

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {
    
void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    int base = currentInstruction[1].u.operand;
    int property = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;

    emitLoad(base, regT1, regT0);
    emitLoad(value, regT3, regT2);
    callOperation(operationPutByIndex, regT1, regT0, property, regT3, regT2);
}

void JIT::emit_op_put_getter_setter(Instruction* currentInstruction)
{
    int base = currentInstruction[1].u.operand;
    int property = currentInstruction[2].u.operand;
    int getter = currentInstruction[3].u.operand;
    int setter = currentInstruction[4].u.operand;

    emitLoadPayload(base, regT1);
    emitLoadPayload(getter, regT3);
    emitLoadPayload(setter, regT4);
    callOperation(operationPutGetterSetter, regT1, &m_codeBlock->identifier(property), regT3, regT4);
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    emitLoad(base, regT1, regT0);
    callOperation(operationDeleteById, dst, regT1, regT0, &m_codeBlock->identifier(property));
}

JIT::CodeRef JIT::stringGetByValStubGenerator(VM* vm)
{
    JSInterfaceJIT jit(vm);
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(vm->stringStructure.get())));
    
    // Load string length to regT1, and start the process of loading the data pointer into regT0
    jit.load32(Address(regT0, ThunkHelpers::jsStringLengthOffset()), regT1);
    jit.loadPtr(Address(regT0, ThunkHelpers::jsStringValueOffset()), regT0);
    failures.append(jit.branchTest32(Zero, regT0));
    
    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    failures.append(jit.branch32(AboveOrEqual, regT2, regT1));
    
    // Load the character
    JumpList is16Bit;
    JumpList cont8Bit;
    // Load the string flags
    jit.loadPtr(Address(regT0, StringImpl::flagsOffset()), regT1);
    jit.loadPtr(Address(regT0, StringImpl::dataOffset()), regT0);
    is16Bit.append(jit.branchTest32(Zero, regT1, TrustedImm32(StringImpl::flagIs8Bit())));
    jit.load8(BaseIndex(regT0, regT2, TimesOne, 0), regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(BaseIndex(regT0, regT2, TimesTwo, 0), regT0);

    cont8Bit.link(&jit);
    
    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(vm->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
    jit.move(TrustedImm32(JSValue::CellTag), regT1); // We null check regT0 on return so this is safe
    jit.ret();

    failures.link(&jit);
    jit.move(TrustedImm32(0), regT0);
    jit.ret();
    
    LinkBuffer patchBuffer(*vm, &jit, GLOBAL_THUNK_ID);
    return FINALIZE_CODE(patchBuffer, ("String get_by_val stub"));
}

void JIT::emit_op_get_by_val(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    emitArrayProfilingSite(regT1, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT1);

    PatchableJump badType;
    JumpList slowCases;
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
        slowCases = emitInt32GetByVal(currentInstruction, badType);
        break;
    case JITDouble:
        slowCases = emitDoubleGetByVal(currentInstruction, badType);
        break;
    case JITContiguous:
        slowCases = emitContiguousGetByVal(currentInstruction, badType);
        break;
    case JITArrayStorage:
        slowCases = emitArrayStorageGetByVal(currentInstruction, badType);
        break;
    default:
        CRASH();
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();

#if !ASSERT_DISABLED
    Jump resultOK = branch32(NotEqual, regT1, TrustedImm32(JSValue::EmptyValueTag));
    breakpoint();
    resultOK.link(this);
#endif

    emitValueProfilingSite(regT4);
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_val), dst, regT1, regT0);
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitContiguousGetByVal(Instruction*, PatchableJump& badType, IndexingType expectedShape)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(expectedShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength())));
    
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1); // tag
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0); // payload
    slowCases.append(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    
    return slowCases;
}

JIT::JumpList JIT::emitDoubleGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(DoubleShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength())));
    
    loadDouble(BaseIndex(regT3, regT2, TimesEight), fpRegT0);
    slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
    moveDoubleToInts(fpRegT0, regT0, regT1);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStorageGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    add32(TrustedImm32(-ArrayStorageShape), regT1, regT3);
    badType = patchableBranch32(Above, regT3, TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, ArrayStorage::vectorLengthOffset())));
    
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1); // tag
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0); // payload
    slowCases.append(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    
    return slowCases;
}
    
void JIT::emitSlow_op_get_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check

    Jump nonCell = jump();
    linkSlowCase(iter); // base array check
    Jump notString = branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(m_vm->stringStructure.get()));
    emitNakedCall(m_vm->getCTIStub(stringGetByValStubGenerator).code());
    Jump failed = branchTestPtr(Zero, regT0);
    emitStore(dst, regT1, regT0);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_get_by_val));
    failed.link(this);
    notString.link(this);
    nonCell.link(this);
    
    Jump skipProfiling = jump();

    linkSlowCase(iter); // vector length check
    linkSlowCase(iter); // empty value
    
    emitArrayProfileOutOfBoundsSpecialCase(profile);
    
    skipProfiling.link(this);
    
    Label slowPath = label();
    
    emitLoad(base, regT1, regT0);
    emitLoad(property, regT3, regT2);
    Call call = callOperation(operationGetByValDefault, dst, regT1, regT0, regT3, regT2);

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;

    emitValueProfilingSite(regT4);
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    int base = currentInstruction[1].u.operand;
    int property = currentInstruction[2].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
    emitArrayProfilingSite(regT1, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT1);
    
    PatchableJump badType;
    JumpList slowCases;
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
        slowCases = emitInt32PutByVal(currentInstruction, badType);
        break;
    case JITDouble:
        slowCases = emitDoublePutByVal(currentInstruction, badType);
        break;
    case JITContiguous:
        slowCases = emitContiguousPutByVal(currentInstruction, badType);
        break;
    case JITArrayStorage:
        slowCases = emitArrayStoragePutByVal(currentInstruction, badType);
        break;
    default:
        CRASH();
        break;
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitGenericContiguousPutByVal(Instruction* currentInstruction, PatchableJump& badType, IndexingType indexingShape)
{
    int value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(ContiguousShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    Jump outOfBounds = branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfPublicLength()));
    
    Label storeResult = label();
    emitLoad(value, regT1, regT0);
    switch (indexingShape) {
    case Int32Shape:
        slowCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag)));
        // Fall through.
    case ContiguousShape:
        store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)));
        store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)));
        break;
    case DoubleShape: {
        Jump notInt = branch32(NotEqual, regT1, TrustedImm32(JSValue::Int32Tag));
        convertInt32ToDouble(regT0, fpRegT0);
        Jump ready = jump();
        notInt.link(this);
        moveIntsToDouble(regT0, regT1, fpRegT0, fpRegT1);
        slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
        ready.link(this);
        storeDouble(fpRegT0, BaseIndex(regT3, regT2, TimesEight));
        break;
    }
    default:
        CRASH();
        break;
    }
        
    Jump done = jump();
    
    outOfBounds.link(this);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, Butterfly::offsetOfVectorLength())));
    
    emitArrayProfileStoreToHoleSpecialCase(profile);
    
    add32(TrustedImm32(1), regT2, regT1);
    store32(regT1, Address(regT3, Butterfly::offsetOfPublicLength()));
    jump().linkTo(storeResult, this);
    
    done.link(this);
    
    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStoragePutByVal(Instruction* currentInstruction, PatchableJump& badType)
{
    int value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT1, TrustedImm32(ArrayStorageShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT3);
    slowCases.append(branch32(AboveOrEqual, regT2, Address(regT3, ArrayStorage::vectorLengthOffset())));

    Jump empty = branch32(Equal, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
    
    Label storeResult(this);
    emitLoad(value, regT1, regT0);
    store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload))); // payload
    store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag))); // tag
    Jump end = jump();
    
    empty.link(this);
    emitArrayProfileStoreToHoleSpecialCase(profile);
    add32(TrustedImm32(1), Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
    branch32(Below, regT2, Address(regT3, ArrayStorage::lengthOffset())).linkTo(storeResult, this);
    
    add32(TrustedImm32(1), regT2, regT0);
    store32(regT0, Address(regT3, ArrayStorage::lengthOffset()));
    jump().linkTo(storeResult, this);
    
    end.link(this);
    
    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);
    
    return slowCases;
}

void JIT::emitSlow_op_put_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int base = currentInstruction[1].u.operand;
    int property = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check
    linkSlowCase(iter); // base not array check
    
    JITArrayMode mode = chooseArrayMode(profile);
    switch (mode) {
    case JITInt32:
    case JITDouble:
        linkSlowCase(iter); // value type check
        break;
    default:
        break;
    }
    
    Jump skipProfiling = jump();
    linkSlowCase(iter); // out of bounds
    emitArrayProfileOutOfBoundsSpecialCase(profile);
    skipProfiling.link(this);

    Label slowPath = label();
    
    bool isDirect = m_interpreter->getOpcodeID(currentInstruction->u.opcode) == op_put_by_val_direct;

#if CPU(X86)
    // FIXME: We only have 5 temp registers, but need 6 to make this call, therefore we materialize
    // our own call. When we finish moving JSC to the C call stack, we'll get another register so
    // we can use the normal case.
    resetCallArguments();
    addCallArgument(GPRInfo::callFrameRegister);
    emitLoad(base, regT0, regT1);
    addCallArgument(regT1);
    addCallArgument(regT0);
    emitLoad(property, regT0, regT1);
    addCallArgument(regT1);
    addCallArgument(regT0);
    emitLoad(value, regT0, regT1);
    addCallArgument(regT1);
    addCallArgument(regT0);
    Call call = appendCallWithExceptionCheck(isDirect ? operationDirectPutByVal : operationPutByVal);
#else
    // The register selection below is chosen to reduce register swapping on ARM.
    // Swapping shouldn't happen on other platforms.
    emitLoad(base, regT2, regT1);
    emitLoad(property, regT3, regT0);
    emitLoad(value, regT5, regT4);
    Call call = callOperation(isDirect ? operationDirectPutByVal : operationPutByVal, regT2, regT1, regT3, regT0, regT5, regT4);
#endif

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;
}

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    const Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));
    
    emitLoad(base, regT1, regT0);
    emitJumpSlowCaseIfNotJSCell(base, regT1);

    if (*ident == m_vm->propertyNames->length && shouldEmitProfiling()) {
        loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
        emitArrayProfilingSiteForBytecodeIndex(regT2, regT3, m_bytecodeOffset);
    }

    JITGetByIdGenerator gen(
        m_codeBlock, CodeOrigin(m_bytecodeOffset), RegisterSet::specialRegisters(),
        JSValueRegs::payloadOnly(regT0), JSValueRegs(regT1, regT0), true);
    gen.generateFastPath(*this);
    addSlowCase(gen.slowPathJump());
    m_getByIds.append(gen);

    emitValueProfilingSite(regT4);
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_id), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int resultVReg = currentInstruction[1].u.operand;
    int baseVReg = currentInstruction[2].u.operand;
    const Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    JITGetByIdGenerator& gen = m_getByIds[m_getByIdIndex++];
    
    Label coldPathBegin = label();
    
    Call call = callOperation(WithProfile, operationGetByIdOptimize, resultVReg, gen.stubInfo(), regT1, regT0, ident->impl());
    
    gen.reportSlowPathCall(coldPathBegin, call);
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.
    
    int base = currentInstruction[1].u.operand;
    int value = currentInstruction[3].u.operand;
    int direct = currentInstruction[8].u.operand;
    
    emitLoad2(base, regT1, regT0, value, regT3, regT2);
    
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    
    emitWriteBarrier(regT0, regT1, regT2, regT3, ShouldFilterImmediates, WriteBarrierForPropertyAccess);
    
    JITPutByIdGenerator gen(
        m_codeBlock, CodeOrigin(m_bytecodeOffset), RegisterSet::specialRegisters(),
        JSValueRegs::payloadOnly(regT0), JSValueRegs(regT3, regT2), regT1, true,
        m_codeBlock->ecmaMode(), direct ? Direct : NotDirect);
    
    gen.generateFastPath(*this);
    addSlowCase(gen.slowPathJump());
    
    m_putByIds.append(gen);
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int base = currentInstruction[1].u.operand;
    const Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));

    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    
    Label coldPathBegin(this);
    
    JITPutByIdGenerator& gen = m_putByIds[m_putByIdIndex++];
    
    Call call = callOperation(
        gen.slowPathFunction(), gen.stubInfo(), regT3, regT2, regT1, regT0, ident->impl());
    
    gen.reportSlowPathCall(coldPathBegin, call);
}

// Compile a store into an object's property storage.  May overwrite base.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID valueTag, RegisterID valuePayload, PropertyOffset cachedOffset)
{
    if (isOutOfLineOffset(cachedOffset))
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
    emitStore(indexRelativeToBase(cachedOffset), valueTag, valuePayload, base);
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        emitLoad(indexRelativeToBase(cachedOffset), resultTag, resultPayload, base);
        return;
    }
    
    RegisterID temp = resultPayload;
    loadPtr(Address(base, JSObject::butterflyOffset()), temp);
    emitLoad(indexRelativeToBase(cachedOffset), resultTag, resultPayload, temp);
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID resultTag, RegisterID resultPayload, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        move(TrustedImmPtr(base->locationForOffset(cachedOffset)), resultTag);
        load32(Address(resultTag, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
        load32(Address(resultTag, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
        return;
    }
    
    loadPtr(base->butterflyAddress(), resultTag);
    load32(Address(resultTag, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
    load32(Address(resultTag, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, RegisterID offset, FinalObjectMode finalObjectMode)
{
    ASSERT(sizeof(JSValue) == 8);
    
    if (finalObjectMode == MayBeFinal) {
        Jump isInline = branch32(LessThan, offset, TrustedImm32(firstOutOfLineOffset));
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
        neg32(offset);
        Jump done = jump();
        isInline.link(this);
        addPtr(TrustedImmPtr(JSObject::offsetOfInlineStorage() - (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), base);
        done.link(this);
    } else {
#if !ASSERT_DISABLED
        Jump isOutOfLine = branch32(GreaterThanOrEqual, offset, TrustedImm32(firstOutOfLineOffset));
        breakpoint();
        isOutOfLine.link(this);
#endif
        loadPtr(Address(base, JSObject::butterflyOffset()), base);
        neg32(offset);
    }
    load32(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload) + (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), resultPayload);
    load32(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag) + (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), resultTag);
}

void JIT::emit_op_get_by_pname(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    unsigned expected = currentInstruction[4].u.operand;
    int iter = currentInstruction[5].u.operand;
    int i = currentInstruction[6].u.operand;
    
    emitLoad2(property, regT1, regT0, base, regT3, regT2);
    emitJumpSlowCaseIfNotJSCell(property, regT1);
    addSlowCase(branchPtr(NotEqual, regT0, payloadFor(expected)));
    // Property registers are now available as the property is known
    emitJumpSlowCaseIfNotJSCell(base, regT3);
    emitLoadPayload(iter, regT1);
    
    // Test base's structure
    loadPtr(Address(regT2, JSCell::structureOffset()), regT0);
    addSlowCase(branchPtr(NotEqual, regT0, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure))));
    load32(addressFor(i), regT3);
    sub32(TrustedImm32(1), regT3);
    addSlowCase(branch32(AboveOrEqual, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_numCacheableSlots))));
    Jump inlineProperty = branch32(Below, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)));
    add32(TrustedImm32(firstOutOfLineOffset), regT3);
    sub32(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)), regT3);
    inlineProperty.link(this);
    compileGetDirectOffset(regT2, regT1, regT0, regT3);    
    
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_pname), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_by_pname(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    
    linkSlowCaseIfNotJSCell(iter, property);
    linkSlowCase(iter);
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    linkSlowCase(iter);
    
    emitLoad(base, regT1, regT0);
    emitLoad(property, regT3, regT2);
    callOperation(operationGetByValGeneric, dst, regT1, regT0, regT3, regT2);
}

void JIT::emitVarInjectionCheck(bool needsVarInjectionChecks)
{
    if (!needsVarInjectionChecks)
        return;
    addSlowCase(branchTest8(NonZero, AbsoluteAddress(m_codeBlock->globalObject()->varInjectionWatchpoint()->addressOfIsInvalidated())));
}

void JIT::emitResolveClosure(int dst, bool needsVarInjectionChecks, unsigned depth)
{
    emitVarInjectionCheck(needsVarInjectionChecks);
    move(TrustedImm32(JSValue::CellTag), regT1);
    emitLoadPayload(JSStack::ScopeChain, regT0);
    if (m_codeBlock->needsActivation()) {
        emitLoadPayload(m_codeBlock->activationRegister().offset(), regT2);
        Jump noActivation = branchTestPtr(Zero, regT2);
        loadPtr(Address(regT2, JSScope::offsetOfNext()), regT0);
        noActivation.link(this);
    }
    for (unsigned i = 0; i < depth; ++i)
        loadPtr(Address(regT0, JSScope::offsetOfNext()), regT0);
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_resolve_scope), dst, regT1, regT0);
}

void JIT::emit_op_resolve_scope(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    ResolveType resolveType = static_cast<ResolveType>(currentInstruction[3].u.operand);
    unsigned depth = currentInstruction[4].u.operand;

    switch (resolveType) {
    case GlobalProperty:
    case GlobalVar:
    case GlobalPropertyWithVarInjectionChecks:
    case GlobalVarWithVarInjectionChecks:
        emitVarInjectionCheck(needsVarInjectionChecks(resolveType));
        move(TrustedImm32(JSValue::CellTag), regT1);
        move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
        emitStore(dst, regT1, regT0);
        map(m_bytecodeOffset + OPCODE_LENGTH(op_resolve_scope), dst, regT1, regT0);
        break;
    case ClosureVar:
    case ClosureVarWithVarInjectionChecks:
        emitResolveClosure(dst, needsVarInjectionChecks(resolveType), depth);
        break;
    case Dynamic:
        addSlowCase(jump());
        break;
    }
}

void JIT::emitSlow_op_resolve_scope(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    ResolveType resolveType = static_cast<ResolveType>(currentInstruction[3].u.operand);

    if (resolveType == GlobalProperty || resolveType == GlobalVar || resolveType == ClosureVar)
        return;

    linkSlowCase(iter);
    int32_t indentifierIndex = currentInstruction[2].u.operand;
    callOperation(operationResolveScope, dst, indentifierIndex);
}

void JIT::emitLoadWithStructureCheck(int scope, Structure** structureSlot)
{
    emitLoad(scope, regT1, regT0);
    loadPtr(structureSlot, regT2);
    addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), regT2));
}

void JIT::emitGetGlobalProperty(uintptr_t* operandSlot)
{
    move(regT0, regT2);
    load32(operandSlot, regT3);
    compileGetDirectOffset(regT2, regT1, regT0, regT3, KnownNotFinal);
}

void JIT::emitGetGlobalVar(uintptr_t operand)
{
    load32(reinterpret_cast<char*>(operand) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag), regT1);
    load32(reinterpret_cast<char*>(operand) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload), regT0);
}

void JIT::emitGetClosureVar(int scope, uintptr_t operand)
{
    emitLoad(scope, regT1, regT0);
    loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT0);
    load32(Address(regT0, operand * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)), regT1);
    load32(Address(regT0, operand * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)), regT0);
}

void JIT::emit_op_get_from_scope(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int scope = currentInstruction[2].u.operand;
    ResolveType resolveType = ResolveModeAndType(currentInstruction[4].u.operand).type();
    Structure** structureSlot = currentInstruction[5].u.structure.slot();
    uintptr_t* operandSlot = reinterpret_cast<uintptr_t*>(&currentInstruction[6].u.pointer);

    switch (resolveType) {
    case GlobalProperty:
    case GlobalPropertyWithVarInjectionChecks:
        emitLoadWithStructureCheck(scope, structureSlot); // Structure check covers var injection.
        emitGetGlobalProperty(operandSlot);
        break;
    case GlobalVar:
    case GlobalVarWithVarInjectionChecks:
        emitVarInjectionCheck(needsVarInjectionChecks(resolveType));
        emitGetGlobalVar(*operandSlot);
        break;
    case ClosureVar:
    case ClosureVarWithVarInjectionChecks:
        emitVarInjectionCheck(needsVarInjectionChecks(resolveType));
        emitGetClosureVar(scope, *operandSlot);
        break;
    case Dynamic:
        addSlowCase(jump());
        break;
    }
    emitValueProfilingSite(regT4);
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_from_scope), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_from_scope(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    ResolveType resolveType = ResolveModeAndType(currentInstruction[4].u.operand).type();

    if (resolveType == GlobalVar || resolveType == ClosureVar)
        return;

    linkSlowCase(iter);
    callOperation(WithProfile, operationGetFromScope, dst, currentInstruction);
}

void JIT::emitPutGlobalProperty(uintptr_t* operandSlot, int value)
{
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT0);
    loadPtr(operandSlot, regT1);
    negPtr(regT1);
    emitLoad(value, regT3, regT2);
    store32(regT3, BaseIndex(regT0, regT1, TimesEight, (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    store32(regT2, BaseIndex(regT0, regT1, TimesEight, (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
}

void JIT::emitPutGlobalVar(uintptr_t operand, int value)
{
    emitLoad(value, regT1, regT0);
    store32(regT1, reinterpret_cast<char*>(operand) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    store32(regT0, reinterpret_cast<char*>(operand) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
}

void JIT::emitPutClosureVar(int scope, uintptr_t operand, int value)
{
    emitLoad(value, regT3, regT2);
    emitLoad(scope, regT1, regT0);
    loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT0);
    store32(regT3, Address(regT0, operand * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag)));
    store32(regT2, Address(regT0, operand * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload)));
}

void JIT::emit_op_put_to_scope(Instruction* currentInstruction)
{
    int scope = currentInstruction[1].u.operand;
    int value = currentInstruction[3].u.operand;
    ResolveType resolveType = ResolveModeAndType(currentInstruction[4].u.operand).type();
    Structure** structureSlot = currentInstruction[5].u.structure.slot();
    uintptr_t* operandSlot = reinterpret_cast<uintptr_t*>(&currentInstruction[6].u.pointer);

    switch (resolveType) {
    case GlobalProperty:
    case GlobalPropertyWithVarInjectionChecks:
        emitLoadWithStructureCheck(scope, structureSlot); // Structure check covers var injection.
        emitPutGlobalProperty(operandSlot, value);
        break;
    case GlobalVar:
    case GlobalVarWithVarInjectionChecks:
        emitVarInjectionCheck(needsVarInjectionChecks(resolveType));
        emitPutGlobalVar(*operandSlot, value);
        break;
    case ClosureVar:
    case ClosureVarWithVarInjectionChecks:
        emitVarInjectionCheck(needsVarInjectionChecks(resolveType));
        emitPutClosureVar(scope, *operandSlot, value);
        break;
    case Dynamic:
        addSlowCase(jump());
        break;
    }
}

void JIT::emitSlow_op_put_to_scope(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    ResolveType resolveType = ResolveModeAndType(currentInstruction[4].u.operand).type();

    if (resolveType == GlobalVar || resolveType == ClosureVar)
        return;

    linkSlowCase(iter);
    callOperation(operationPutToScope, currentInstruction);
}

void JIT::emit_op_init_global_const(Instruction* currentInstruction)
{
    WriteBarrier<Unknown>* registerPointer = currentInstruction[1].u.registerPointer;
    int value = currentInstruction[2].u.operand;

    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    emitLoad(value, regT1, regT0);
    
    if (Heap::isWriteBarrierEnabled()) {
        move(TrustedImmPtr(globalObject), regT2);
        
        emitWriteBarrier(globalObject, regT1, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);
    }

    store32(regT1, registerPointer->tagPointer());
    store32(regT0, registerPointer->payloadPointer());
    map(m_bytecodeOffset + OPCODE_LENGTH(op_init_global_const), value, regT1, regT0);
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
