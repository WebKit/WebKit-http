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
#include "JIT.h"

#include "CodeBlock.h"
#include "GCAwareJITStubRoutine.h"
#include "GetterSetter.h"
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
#if USE(JSVALUE64)

JIT::CodeRef JIT::stringGetByValStubGenerator(VM* vm)
{
    JSInterfaceJIT jit(vm);
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(vm->stringStructure.get())));

    // Load string length to regT2, and start the process of loading the data pointer into regT0
    jit.load32(Address(regT0, ThunkHelpers::jsStringLengthOffset()), regT2);
    jit.loadPtr(Address(regT0, ThunkHelpers::jsStringValueOffset()), regT0);
    failures.append(jit.branchTest32(Zero, regT0));

    // Do an unsigned compare to simultaneously filter negative indices as well as indices that are too large
    failures.append(jit.branch32(AboveOrEqual, regT1, regT2));
    
    // Load the character
    JumpList is16Bit;
    JumpList cont8Bit;
    // Load the string flags
    jit.loadPtr(Address(regT0, StringImpl::flagsOffset()), regT2);
    jit.loadPtr(Address(regT0, StringImpl::dataOffset()), regT0);
    is16Bit.append(jit.branchTest32(Zero, regT2, TrustedImm32(StringImpl::flagIs8Bit())));
    jit.load8(BaseIndex(regT0, regT1, TimesOne, 0), regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(BaseIndex(regT0, regT1, TimesTwo, 0), regT0);
    cont8Bit.link(&jit);

    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(vm->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
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
    
    emitGetVirtualRegisters(base, regT0, property, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);

    // This is technically incorrect - we're zero-extending an int32.  On the hot path this doesn't matter.
    // We check the value as if it was a uint32 against the m_vectorLength - which will always fail if
    // number was signed since m_vectorLength is always less than intmax (since the total allocation
    // size is always less than 4Gb).  As such zero extending wil have been correct (and extending the value
    // to 64-bits is necessary since it's used in the address calculation.  We zero extend rather than sign
    // extending since it makes it easier to re-tag the value in the slow case.
    zeroExtend32ToPtr(regT1, regT1);

    emitJumpSlowCaseIfNotJSCell(regT0, base);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    emitArrayProfilingSite(regT2, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT2);

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
        break;
    }
    
    addSlowCase(badType);
    addSlowCase(slowCases);
    
    Label done = label();
    
#if !ASSERT_DISABLED
    Jump resultOK = branchTest64(NonZero, regT0);
    breakpoint();
    resultOK.link(this);
#endif

    emitValueProfilingSite(regT4);
    emitPutVirtualRegister(dst);
    
    m_byValCompilationInfo.append(ByValCompilationInfo(m_bytecodeOffset, badType, mode, done));
}

JIT::JumpList JIT::emitDoubleGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(DoubleShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength())));
    loadDouble(BaseIndex(regT2, regT1, TimesEight), fpRegT0);
    slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
    moveDoubleTo64(fpRegT0, regT0);
    sub64(tagTypeNumberRegister, regT0);
    
    return slowCases;
}

JIT::JumpList JIT::emitContiguousGetByVal(Instruction*, PatchableJump& badType, IndexingType expectedShape)
{
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(expectedShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength())));
    load64(BaseIndex(regT2, regT1, TimesEight), regT0);
    slowCases.append(branchTest64(Zero, regT0));
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStorageGetByVal(Instruction*, PatchableJump& badType)
{
    JumpList slowCases;

    add32(TrustedImm32(-ArrayStorageShape), regT2, regT3);
    badType = patchableBranch32(Above, regT3, TrustedImm32(SlowPutArrayStorageShape - ArrayStorageShape));

    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, ArrayStorage::vectorLengthOffset())));

    load64(BaseIndex(regT2, regT1, TimesEight, ArrayStorage::vectorOffset()), regT0);
    slowCases.append(branchTest64(Zero, regT0));
    
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
    emitNakedCall(CodeLocationLabel(m_vm->getCTIStub(stringGetByValStubGenerator).code()));
    Jump failed = branchTest64(Zero, regT0);
    emitPutVirtualRegister(dst, regT0);
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
    
    emitGetVirtualRegister(base, regT0);
    emitGetVirtualRegister(property, regT1);
    Call call = callOperation(operationGetByValDefault, dst, regT0, regT1);

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;

    emitValueProfilingSite(regT4);
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, RegisterID offset, RegisterID scratch, FinalObjectMode finalObjectMode)
{
    ASSERT(sizeof(JSValue) == 8);
    
    if (finalObjectMode == MayBeFinal) {
        Jump isInline = branch32(LessThan, offset, TrustedImm32(firstOutOfLineOffset));
        loadPtr(Address(base, JSObject::butterflyOffset()), scratch);
        neg32(offset);
        Jump done = jump();
        isInline.link(this);
        addPtr(TrustedImm32(JSObject::offsetOfInlineStorage() - (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), base, scratch);
        done.link(this);
    } else {
#if !ASSERT_DISABLED
        Jump isOutOfLine = branch32(GreaterThanOrEqual, offset, TrustedImm32(firstOutOfLineOffset));
        breakpoint();
        isOutOfLine.link(this);
#endif
        loadPtr(Address(base, JSObject::butterflyOffset()), scratch);
        neg32(offset);
    }
    signExtend32ToPtr(offset, offset);
    load64(BaseIndex(scratch, offset, TimesEight, (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)), result);
}

void JIT::emit_op_get_by_pname(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    unsigned expected = currentInstruction[4].u.operand;
    int iter = currentInstruction[5].u.operand;
    int i = currentInstruction[6].u.operand;

    emitGetVirtualRegister(property, regT0);
    addSlowCase(branch64(NotEqual, regT0, addressFor(expected)));
    emitGetVirtualRegisters(base, regT0, iter, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);

    // Test base's structure
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    addSlowCase(branchPtr(NotEqual, regT2, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructure))));
    load32(addressFor(i), regT3);
    sub32(TrustedImm32(1), regT3);
    addSlowCase(branch32(AboveOrEqual, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_numCacheableSlots))));
    Jump inlineProperty = branch32(Below, regT3, Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)));
    add32(TrustedImm32(firstOutOfLineOffset), regT3);
    sub32(Address(regT1, OBJECT_OFFSETOF(JSPropertyNameIterator, m_cachedStructureInlineCapacity)), regT3);
    inlineProperty.link(this);
    compileGetDirectOffset(regT0, regT0, regT3, regT1);

    emitPutVirtualRegister(dst, regT0);
}

void JIT::emitSlow_op_get_by_pname(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;

    linkSlowCase(iter);
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    linkSlowCase(iter);

    emitGetVirtualRegister(base, regT0);
    emitGetVirtualRegister(property, regT1);
    callOperation(operationGetByValGeneric, dst, regT0, regT1);
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    int base = currentInstruction[1].u.operand;
    int property = currentInstruction[2].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;

    emitGetVirtualRegisters(base, regT0, property, regT1);
    emitJumpSlowCaseIfNotImmediateInteger(regT1);
    // See comment in op_get_by_val.
    zeroExtend32ToPtr(regT1, regT1);
    emitJumpSlowCaseIfNotJSCell(regT0, base);
    loadPtr(Address(regT0, JSCell::structureOffset()), regT2);
    emitArrayProfilingSite(regT2, regT3, profile);
    and32(TrustedImm32(IndexingShapeMask), regT2);
    
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

    emitWriteBarrier(regT0, regT3, regT1, regT3, ShouldFilterImmediates, WriteBarrierForPropertyAccess);
}

JIT::JumpList JIT::emitGenericContiguousPutByVal(Instruction* currentInstruction, PatchableJump& badType, IndexingType indexingShape)
{
    int value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;

    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(indexingShape));
    
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    Jump outOfBounds = branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfPublicLength()));

    Label storeResult = label();
    emitGetVirtualRegister(value, regT3);
    switch (indexingShape) {
    case Int32Shape:
        slowCases.append(emitJumpIfNotImmediateInteger(regT3));
        store64(regT3, BaseIndex(regT2, regT1, TimesEight));
        break;
    case DoubleShape: {
        Jump notInt = emitJumpIfNotImmediateInteger(regT3);
        convertInt32ToDouble(regT3, fpRegT0);
        Jump ready = jump();
        notInt.link(this);
        add64(tagTypeNumberRegister, regT3);
        move64ToDouble(regT3, fpRegT0);
        slowCases.append(branchDouble(DoubleNotEqualOrUnordered, fpRegT0, fpRegT0));
        ready.link(this);
        storeDouble(fpRegT0, BaseIndex(regT2, regT1, TimesEight));
        break;
    }
    case ContiguousShape:
        store64(regT3, BaseIndex(regT2, regT1, TimesEight));
        break;
    default:
        CRASH();
        break;
    }
    
    Jump done = jump();
    outOfBounds.link(this);
    
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, Butterfly::offsetOfVectorLength())));
    
    emitArrayProfileStoreToHoleSpecialCase(profile);
    
    add32(TrustedImm32(1), regT1, regT3);
    store32(regT3, Address(regT2, Butterfly::offsetOfPublicLength()));
    jump().linkTo(storeResult, this);
    
    done.link(this);
    
    return slowCases;
}

JIT::JumpList JIT::emitArrayStoragePutByVal(Instruction* currentInstruction, PatchableJump& badType)
{
    int value = currentInstruction[3].u.operand;
    ArrayProfile* profile = currentInstruction[4].u.arrayProfile;
    
    JumpList slowCases;
    
    badType = patchableBranch32(NotEqual, regT2, TrustedImm32(ArrayStorageShape));
    loadPtr(Address(regT0, JSObject::butterflyOffset()), regT2);
    slowCases.append(branch32(AboveOrEqual, regT1, Address(regT2, ArrayStorage::vectorLengthOffset())));

    Jump empty = branchTest64(Zero, BaseIndex(regT2, regT1, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));

    Label storeResult(this);
    emitGetVirtualRegister(value, regT3);
    store64(regT3, BaseIndex(regT2, regT1, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0])));
    Jump end = jump();
    
    empty.link(this);
    emitArrayProfileStoreToHoleSpecialCase(profile);
    add32(TrustedImm32(1), Address(regT2, ArrayStorage::numValuesInVectorOffset()));
    branch32(Below, regT1, Address(regT2, ArrayStorage::lengthOffset())).linkTo(storeResult, this);

    add32(TrustedImm32(1), regT1);
    store32(regT1, Address(regT2, ArrayStorage::lengthOffset()));
    sub32(TrustedImm32(1), regT1);
    jump().linkTo(storeResult, this);

    end.link(this);
    
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

    emitGetVirtualRegister(property, regT1);
    emitGetVirtualRegister(value, regT2);
    bool isDirect = m_interpreter->getOpcodeID(currentInstruction->u.opcode) == op_put_by_val_direct;
    Call call = callOperation(isDirect ? operationDirectPutByVal : operationPutByVal, regT0, regT1, regT2);

    m_byValCompilationInfo[m_byValInstructionIndex].slowPathTarget = slowPath;
    m_byValCompilationInfo[m_byValInstructionIndex].returnAddress = call;
    m_byValInstructionIndex++;
}

void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);
    emitGetVirtualRegister(currentInstruction[3].u.operand, regT1);
    callOperation(operationPutByIndex, regT0, currentInstruction[2].u.operand, regT1);
}

void JIT::emit_op_put_getter_setter(Instruction* currentInstruction)
{
    emitGetVirtualRegister(currentInstruction[1].u.operand, regT0);
    emitGetVirtualRegister(currentInstruction[3].u.operand, regT1);
    emitGetVirtualRegister(currentInstruction[4].u.operand, regT2);
    callOperation(operationPutGetterSetter, regT0, &m_codeBlock->identifier(currentInstruction[2].u.operand), regT1, regT2);
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int property = currentInstruction[3].u.operand;
    emitGetVirtualRegister(base, regT0);
    callOperation(operationDeleteById, dst, regT0, &m_codeBlock->identifier(property));
}

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    int resultVReg = currentInstruction[1].u.operand;
    int baseVReg = currentInstruction[2].u.operand;
    const Identifier* ident = &(m_codeBlock->identifier(currentInstruction[3].u.operand));

    emitGetVirtualRegister(baseVReg, regT0);
    
    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);
    
    if (*ident == m_vm->propertyNames->length && shouldEmitProfiling()) {
        loadPtr(Address(regT0, JSCell::structureOffset()), regT1);
        emitArrayProfilingSiteForBytecodeIndex(regT1, regT2, m_bytecodeOffset);
    }

    JITGetByIdGenerator gen(
        m_codeBlock, CodeOrigin(m_bytecodeOffset), RegisterSet::specialRegisters(),
        JSValueRegs(regT0), JSValueRegs(regT0), true);
    gen.generateFastPath(*this);
    addSlowCase(gen.slowPathJump());
    m_getByIds.append(gen);

    emitValueProfilingSite(regT4);
    emitPutVirtualRegister(resultVReg);
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
    
    Call call = callOperation(WithProfile, operationGetByIdOptimize, resultVReg, gen.stubInfo(), regT0, ident->impl());

    gen.reportSlowPathCall(coldPathBegin, call);
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    int baseVReg = currentInstruction[1].u.operand;
    int valueVReg = currentInstruction[3].u.operand;
    unsigned direct = currentInstruction[8].u.operand;

    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.

    emitGetVirtualRegisters(baseVReg, regT0, valueVReg, regT1);

    // Jump to a slow case if either the base object is an immediate, or if the Structure does not match.
    emitJumpSlowCaseIfNotJSCell(regT0, baseVReg);
    
    emitWriteBarrier(regT0, regT1, regT2, regT3, ShouldFilterImmediates, WriteBarrierForPropertyAccess);

    JITPutByIdGenerator gen(
        m_codeBlock, CodeOrigin(m_bytecodeOffset), RegisterSet::specialRegisters(),
        JSValueRegs(regT0), JSValueRegs(regT1), regT2, true, m_codeBlock->ecmaMode(),
        direct ? Direct : NotDirect);
    
    gen.generateFastPath(*this);
    addSlowCase(gen.slowPathJump());
    
    m_putByIds.append(gen);
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int baseVReg = currentInstruction[1].u.operand;
    const Identifier* ident = &(m_codeBlock->identifier(currentInstruction[2].u.operand));

    linkSlowCaseIfNotJSCell(iter, baseVReg);
    linkSlowCase(iter);

    Label coldPathBegin(this);
    
    JITPutByIdGenerator& gen = m_putByIds[m_putByIdIndex++];

    Call call = callOperation(
        gen.slowPathFunction(), gen.stubInfo(), regT1, regT0, ident->impl());

    gen.reportSlowPathCall(coldPathBegin, call);
}

// Compile a store into an object's property storage.  May overwrite the
// value in objectReg.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID value, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        store64(value, Address(base, JSObject::offsetOfInlineStorage() + sizeof(JSValue) * offsetInInlineStorage(cachedOffset)));
        return;
    }
    
    loadPtr(Address(base, JSObject::butterflyOffset()), base);
    store64(value, Address(base, sizeof(JSValue) * offsetInButterfly(cachedOffset)));
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID result, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        load64(Address(base, JSObject::offsetOfInlineStorage() + sizeof(JSValue) * offsetInInlineStorage(cachedOffset)), result);
        return;
    }
    
    loadPtr(Address(base, JSObject::butterflyOffset()), result);
    load64(Address(result, sizeof(JSValue) * offsetInButterfly(cachedOffset)), result);
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID result, PropertyOffset cachedOffset)
{
    if (isInlineOffset(cachedOffset)) {
        load64(base->locationForOffset(cachedOffset), result);
        return;
    }
    
    loadPtr(base->butterflyAddress(), result);
    load64(Address(result, offsetInButterfly(cachedOffset) * sizeof(WriteBarrier<Unknown>)), result);
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
    emitGetVirtualRegister(JSStack::ScopeChain, regT0);
    if (m_codeBlock->needsActivation()) {
        emitGetVirtualRegister(m_codeBlock->activationRegister(), regT1);
        Jump noActivation = branchTestPtr(Zero, regT1);
        loadPtr(Address(regT0, JSScope::offsetOfNext()), regT0);
        noActivation.link(this);
    }
    for (unsigned i = 0; i < depth; ++i)
        loadPtr(Address(regT0, JSScope::offsetOfNext()), regT0);
    emitPutVirtualRegister(dst);
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
        move(TrustedImmPtr(m_codeBlock->globalObject()), regT0);
        emitPutVirtualRegister(dst);
        break;
    case ClosureVar:
    case ClosureVarWithVarInjectionChecks:
        emitResolveClosure(dst, needsVarInjectionChecks(resolveType), depth);
        break;
    case Dynamic:
        killLastResultRegister();
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
    emitGetVirtualRegister(scope, regT0);
    loadPtr(structureSlot, regT1);
    addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), regT1));
}

void JIT::emitGetGlobalProperty(uintptr_t* operandSlot)
{
    load32(operandSlot, regT1);
    compileGetDirectOffset(regT0, regT0, regT1, regT2, KnownNotFinal);
}

void JIT::emitGetGlobalVar(uintptr_t operand)
{
    loadPtr(reinterpret_cast<void*>(operand), regT0);
}

void JIT::emitGetClosureVar(int scope, uintptr_t operand)
{
    emitGetVirtualRegister(scope, regT0);
    loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT0);
    loadPtr(Address(regT0, operand * sizeof(Register)), regT0);
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
        killLastResultRegister();
        addSlowCase(jump());
        break;
    }
    emitPutVirtualRegister(dst);
    emitValueProfilingSite(regT4);
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
    emitGetVirtualRegister(value, regT2);
    storePtr(regT2, BaseIndex(regT0, regT1, TimesEight, (firstOutOfLineOffset - 2) * sizeof(EncodedJSValue)));
}

void JIT::emitPutGlobalVar(uintptr_t operand, int value)
{
    emitGetVirtualRegister(value, regT0);
    storePtr(regT0, reinterpret_cast<void*>(operand));
}

void JIT::emitPutClosureVar(int scope, uintptr_t operand, int value)
{
    emitGetVirtualRegister(value, regT1);
    emitGetVirtualRegister(scope, regT0);
    loadPtr(Address(regT0, JSVariableObject::offsetOfRegisters()), regT0);
    storePtr(regT1, Address(regT0, operand * sizeof(Register)));
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
        killLastResultRegister();
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
    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    emitGetVirtualRegister(currentInstruction[2].u.operand, regT0);

    store64(regT0, currentInstruction[1].u.registerPointer);
    if (Heap::isWriteBarrierEnabled())
        emitWriteBarrier(globalObject, regT0, regT2, ShouldFilterImmediates, WriteBarrierForVariableAccess);
}

#endif // USE(JSVALUE64)

void JIT::emitWriteBarrier(RegisterID owner, RegisterID value, RegisterID scratch, RegisterID scratch2, WriteBarrierMode mode, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(scratch2);
    UNUSED_PARAM(useKind);
    UNUSED_PARAM(value);
    UNUSED_PARAM(mode);
    ASSERT(owner != scratch);
    ASSERT(owner != scratch2);
    
#if ENABLE(WRITE_BARRIER_PROFILING)
    emitCount(WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

void JIT::emitWriteBarrier(JSCell* owner, RegisterID value, RegisterID scratch, WriteBarrierMode mode, WriteBarrierUseKind useKind)
{
    UNUSED_PARAM(owner);
    UNUSED_PARAM(scratch);
    UNUSED_PARAM(useKind);
    UNUSED_PARAM(value);
    UNUSED_PARAM(mode);
    
#if ENABLE(WRITE_BARRIER_PROFILING)
    emitCount(WriteBarrierCounters::jitCounterFor(useKind));
#endif
}

JIT::Jump JIT::addStructureTransitionCheck(JSCell* object, Structure* structure, StructureStubInfo* stubInfo, RegisterID scratch)
{
    if (object->structure() == structure && structure->transitionWatchpointSetIsStillValid()) {
        structure->addTransitionWatchpoint(stubInfo->addWatchpoint(m_codeBlock));
#if !ASSERT_DISABLED
        move(TrustedImmPtr(object), scratch);
        Jump ok = branchPtr(Equal, Address(scratch, JSCell::structureOffset()), TrustedImmPtr(structure));
        breakpoint();
        ok.link(this);
#endif
        Jump result; // Returning an unset jump this way because otherwise VC++ would complain.
        return result;
    }
    
    move(TrustedImmPtr(object), scratch);
    return branchPtr(NotEqual, Address(scratch, JSCell::structureOffset()), TrustedImmPtr(structure));
}

void JIT::addStructureTransitionCheck(JSCell* object, Structure* structure, StructureStubInfo* stubInfo, JumpList& failureCases, RegisterID scratch)
{
    Jump failureCase = addStructureTransitionCheck(object, structure, stubInfo, scratch);
    if (!failureCase.isSet())
        return;
    
    failureCases.append(failureCase);
}

void JIT::testPrototype(JSValue prototype, JumpList& failureCases, StructureStubInfo* stubInfo)
{
    if (prototype.isNull())
        return;

    ASSERT(prototype.isCell());
    addStructureTransitionCheck(prototype.asCell(), prototype.asCell()->structure(), stubInfo, failureCases, regT3);
}

void JIT::privateCompileGetByVal(ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
{
    Instruction* currentInstruction = m_codeBlock->instructions().begin() + byValInfo->bytecodeIndex;
    
    PatchableJump badType;
    JumpList slowCases;
    
    switch (arrayMode) {
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
        TypedArrayType type = typedArrayTypeForJITArrayMode(arrayMode);
        if (isInt(type))
            slowCases = emitIntTypedArrayGetByVal(currentInstruction, badType, type);
        else 
            slowCases = emitFloatTypedArrayGetByVal(currentInstruction, badType, type);
        break;
    }
    
    Jump done = jump();

    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(badType, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    patchBuffer.link(slowCases, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    
    patchBuffer.link(done, byValInfo->badTypeJump.labelAtOffset(byValInfo->badTypeJumpToDone));
    
    byValInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
        patchBuffer,
        ("Baseline get_by_val stub for %s, return point %p", toCString(*m_codeBlock).data(), returnAddress.value()));
    
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(byValInfo->badTypeJump, CodeLocationLabel(byValInfo->stubRoutine->code().code()));
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(operationGetByValGeneric));
}

void JIT::privateCompilePutByVal(ByValInfo* byValInfo, ReturnAddressPtr returnAddress, JITArrayMode arrayMode)
{
    Instruction* currentInstruction = m_codeBlock->instructions().begin() + byValInfo->bytecodeIndex;
    
    PatchableJump badType;
    JumpList slowCases;
    
    switch (arrayMode) {
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
        TypedArrayType type = typedArrayTypeForJITArrayMode(arrayMode);
        if (isInt(type))
            slowCases = emitIntTypedArrayPutByVal(currentInstruction, badType, type);
        else 
            slowCases = emitFloatTypedArrayPutByVal(currentInstruction, badType, type);
        break;
    }
    
    Jump done = jump();

    LinkBuffer patchBuffer(*m_vm, this, m_codeBlock);
    
    patchBuffer.link(badType, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    patchBuffer.link(slowCases, CodeLocationLabel(MacroAssemblerCodePtr::createFromExecutableAddress(returnAddress.value())).labelAtOffset(byValInfo->returnAddressToSlowPath));
    
    patchBuffer.link(done, byValInfo->badTypeJump.labelAtOffset(byValInfo->badTypeJumpToDone));
    
    bool isDirect = m_interpreter->getOpcodeID(currentInstruction->u.opcode) == op_put_by_val_direct;
    if (!isDirect) {
        byValInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
            patchBuffer,
            ("Baseline put_by_val stub for %s, return point %p", toCString(*m_codeBlock).data(), returnAddress.value()));
        
    } else {
        byValInfo->stubRoutine = FINALIZE_CODE_FOR_STUB(
            patchBuffer,
            ("Baseline put_by_val_direct stub for %s, return point %p", toCString(*m_codeBlock).data(), returnAddress.value()));
    }
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(byValInfo->badTypeJump, CodeLocationLabel(byValInfo->stubRoutine->code().code()));
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(isDirect ? operationDirectPutByValGeneric : operationPutByValGeneric));
}

JIT::JumpList JIT::emitIntTypedArrayGetByVal(Instruction*, PatchableJump& badType, TypedArrayType type)
{
    ASSERT(isInt(type));
    
    // The best way to test the array type is to use the classInfo. We need to do so without
    // clobbering the register that holds the indexing type, base, and property.

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID resultPayload = regT0;
    RegisterID scratch = regT3;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID resultPayload = regT0;
    RegisterID resultTag = regT1;
    RegisterID scratch = regT3;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), scratch);
    badType = patchableBranchPtr(NotEqual, Address(scratch, Structure::classInfoOffset()), TrustedImmPtr(classInfoForType(type)));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, JSArrayBufferView::offsetOfLength())));
    loadPtr(Address(base, JSArrayBufferView::offsetOfVector()), base);
    
    switch (elementSize(type)) {
    case 1:
        if (isSigned(type))
            load8Signed(BaseIndex(base, property, TimesOne), resultPayload);
        else
            load8(BaseIndex(base, property, TimesOne), resultPayload);
        break;
    case 2:
        if (isSigned(type))
            load16Signed(BaseIndex(base, property, TimesTwo), resultPayload);
        else
            load16(BaseIndex(base, property, TimesTwo), resultPayload);
        break;
    case 4:
        load32(BaseIndex(base, property, TimesFour), resultPayload);
        break;
    default:
        CRASH();
    }
    
    Jump done;
    if (type == TypeUint32) {
        Jump canBeInt = branch32(GreaterThanOrEqual, resultPayload, TrustedImm32(0));
        
        convertInt32ToDouble(resultPayload, fpRegT0);
        addDouble(AbsoluteAddress(&twoToThe32), fpRegT0);
#if USE(JSVALUE64)
        moveDoubleTo64(fpRegT0, resultPayload);
        sub64(tagTypeNumberRegister, resultPayload);
#else
        moveDoubleToInts(fpRegT0, resultPayload, resultTag);
#endif
        
        done = jump();
        canBeInt.link(this);
    }

#if USE(JSVALUE64)
    or64(tagTypeNumberRegister, resultPayload);
#else
    move(TrustedImm32(JSValue::Int32Tag), resultTag);
#endif
    if (done.isSet())
        done.link(this);
    return slowCases;
}

JIT::JumpList JIT::emitFloatTypedArrayGetByVal(Instruction*, PatchableJump& badType, TypedArrayType type)
{
    ASSERT(isFloat(type));
    
#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID resultPayload = regT0;
    RegisterID scratch = regT3;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID resultPayload = regT0;
    RegisterID resultTag = regT1;
    RegisterID scratch = regT3;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), scratch);
    badType = patchableBranchPtr(NotEqual, Address(scratch, Structure::classInfoOffset()), TrustedImmPtr(classInfoForType(type)));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, JSArrayBufferView::offsetOfLength())));
    loadPtr(Address(base, JSArrayBufferView::offsetOfVector()), base);
    
    switch (elementSize(type)) {
    case 4:
        loadFloat(BaseIndex(base, property, TimesFour), fpRegT0);
        convertFloatToDouble(fpRegT0, fpRegT0);
        break;
    case 8: {
        loadDouble(BaseIndex(base, property, TimesEight), fpRegT0);
        break;
    }
    default:
        CRASH();
    }
    
    Jump notNaN = branchDouble(DoubleEqual, fpRegT0, fpRegT0);
    static const double NaN = QNaN;
    loadDouble(&NaN, fpRegT0);
    notNaN.link(this);
    
#if USE(JSVALUE64)
    moveDoubleTo64(fpRegT0, resultPayload);
    sub64(tagTypeNumberRegister, resultPayload);
#else
    moveDoubleToInts(fpRegT0, resultPayload, resultTag);
#endif
    return slowCases;    
}

JIT::JumpList JIT::emitIntTypedArrayPutByVal(Instruction* currentInstruction, PatchableJump& badType, TypedArrayType type)
{
    ASSERT(isInt(type));
    
    int value = currentInstruction[3].u.operand;

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT2;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT1;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), earlyScratch);
    badType = patchableBranchPtr(NotEqual, Address(earlyScratch, Structure::classInfoOffset()), TrustedImmPtr(classInfoForType(type)));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, JSArrayBufferView::offsetOfLength())));
    
#if USE(JSVALUE64)
    emitGetVirtualRegister(value, earlyScratch);
    slowCases.append(emitJumpIfNotImmediateInteger(earlyScratch));
#else
    emitLoad(value, lateScratch, earlyScratch);
    slowCases.append(branch32(NotEqual, lateScratch, TrustedImm32(JSValue::Int32Tag)));
#endif
    
    // We would be loading this into base as in get_by_val, except that the slow
    // path expects the base to be unclobbered.
    loadPtr(Address(base, JSArrayBufferView::offsetOfVector()), lateScratch);
    
    if (isClamped(type)) {
        ASSERT(elementSize(type) == 1);
        ASSERT(!isSigned(type));
        Jump inBounds = branch32(BelowOrEqual, earlyScratch, TrustedImm32(0xff));
        Jump tooBig = branch32(GreaterThan, earlyScratch, TrustedImm32(0xff));
        xor32(earlyScratch, earlyScratch);
        Jump clamped = jump();
        tooBig.link(this);
        move(TrustedImm32(0xff), earlyScratch);
        clamped.link(this);
        inBounds.link(this);
    }
    
    switch (elementSize(type)) {
    case 1:
        store8(earlyScratch, BaseIndex(lateScratch, property, TimesOne));
        break;
    case 2:
        store16(earlyScratch, BaseIndex(lateScratch, property, TimesTwo));
        break;
    case 4:
        store32(earlyScratch, BaseIndex(lateScratch, property, TimesFour));
        break;
    default:
        CRASH();
    }
    
    return slowCases;
}

JIT::JumpList JIT::emitFloatTypedArrayPutByVal(Instruction* currentInstruction, PatchableJump& badType, TypedArrayType type)
{
    ASSERT(isFloat(type));
    
    int value = currentInstruction[3].u.operand;

#if USE(JSVALUE64)
    RegisterID base = regT0;
    RegisterID property = regT1;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT2;
#else
    RegisterID base = regT0;
    RegisterID property = regT2;
    RegisterID earlyScratch = regT3;
    RegisterID lateScratch = regT1;
#endif
    
    JumpList slowCases;
    
    loadPtr(Address(base, JSCell::structureOffset()), earlyScratch);
    badType = patchableBranchPtr(NotEqual, Address(earlyScratch, Structure::classInfoOffset()), TrustedImmPtr(classInfoForType(type)));
    slowCases.append(branch32(AboveOrEqual, property, Address(base, JSArrayBufferView::offsetOfLength())));
    
#if USE(JSVALUE64)
    emitGetVirtualRegister(value, earlyScratch);
    Jump doubleCase = emitJumpIfNotImmediateInteger(earlyScratch);
    convertInt32ToDouble(earlyScratch, fpRegT0);
    Jump ready = jump();
    doubleCase.link(this);
    slowCases.append(emitJumpIfNotImmediateNumber(earlyScratch));
    add64(tagTypeNumberRegister, earlyScratch);
    move64ToDouble(earlyScratch, fpRegT0);
    ready.link(this);
#else
    emitLoad(value, lateScratch, earlyScratch);
    Jump doubleCase = branch32(NotEqual, lateScratch, TrustedImm32(JSValue::Int32Tag));
    convertInt32ToDouble(earlyScratch, fpRegT0);
    Jump ready = jump();
    doubleCase.link(this);
    slowCases.append(branch32(Above, lateScratch, TrustedImm32(JSValue::LowestTag)));
    moveIntsToDouble(earlyScratch, lateScratch, fpRegT0, fpRegT1);
    ready.link(this);
#endif
    
    // We would be loading this into base as in get_by_val, except that the slow
    // path expects the base to be unclobbered.
    loadPtr(Address(base, JSArrayBufferView::offsetOfVector()), lateScratch);
    
    switch (elementSize(type)) {
    case 4:
        convertDoubleToFloat(fpRegT0, fpRegT0);
        storeFloat(fpRegT0, BaseIndex(lateScratch, property, TimesFour));
        break;
    case 8:
        storeDouble(fpRegT0, BaseIndex(lateScratch, property, TimesEight));
        break;
    default:
        CRASH();
    }
    
    return slowCases;
}

} // namespace JSC

#endif // ENABLE(JIT)
