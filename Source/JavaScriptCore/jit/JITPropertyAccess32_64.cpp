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
#include "JITInlineMethods.h"
#include "JITStubCall.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSPropertyNameIterator.h"
#include "Interpreter.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"
#include "ResultType.h"
#include "SamplingTool.h"

#ifndef NDEBUG
#include <stdio.h>
#endif

using namespace std;

namespace JSC {
    
void JIT::emit_op_put_by_index(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;
    
    JITStubCall stubCall(this, cti_op_put_by_index);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImm32(property));
    stubCall.addArgument(value);
    stubCall.call();
}

void JIT::emit_op_put_getter_setter(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned getter = currentInstruction[3].u.operand;
    unsigned setter = currentInstruction[4].u.operand;
    
    JITStubCall stubCall(this, cti_op_put_getter_setter);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(property)));
    stubCall.addArgument(getter);
    stubCall.addArgument(setter);
    stubCall.call();
}

void JIT::emit_op_del_by_id(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    JITStubCall stubCall(this, cti_op_del_by_id);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&m_codeBlock->identifier(property)));
    stubCall.call(dst);
}

void JIT::emit_op_method_check(Instruction* currentInstruction)
{
    // Assert that the following instruction is a get_by_id.
    ASSERT(m_interpreter->getOpcodeID((currentInstruction + OPCODE_LENGTH(op_method_check))->u.opcode) == op_get_by_id);
    
    currentInstruction += OPCODE_LENGTH(op_method_check);
    
    // Do the method check - check the object & its prototype's structure inline (this is the common case).
    m_methodCallCompilationInfo.append(MethodCallCompilationInfo(m_bytecodeOffset, m_propertyAccessCompilationInfo.size()));
    MethodCallCompilationInfo& info = m_methodCallCompilationInfo.last();
    
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    
    emitLoad(base, regT1, regT0);
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceMethodCheck);
    
    Jump structureCheck = branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), info.structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    DataLabelPtr protoStructureToCompare, protoObj = moveWithPatch(TrustedImmPtr(0), regT2);
    Jump protoStructureCheck = branchPtrWithPatch(NotEqual, Address(regT2, JSCell::structureOffset()), protoStructureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    
    // This will be relinked to load the function without doing a load.
    DataLabelPtr putFunction = moveWithPatch(TrustedImmPtr(0), regT0);
    
    END_UNINTERRUPTED_SEQUENCE(sequenceMethodCheck);
    
    move(TrustedImm32(JSValue::CellTag), regT1);
    Jump match = jump();
    
    // Link the failure cases here.
    structureCheck.link(this);
    protoStructureCheck.link(this);
    
    // Do a regular(ish) get_by_id (the slow case will be link to
    // cti_op_get_by_id_method_check instead of cti_op_get_by_id.
    compileGetByIdHotPath();
    
    match.link(this);
    emitValueProfilingSite(m_bytecodeOffset + OPCODE_LENGTH(op_method_check));
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_method_check) + OPCODE_LENGTH(op_get_by_id), dst, regT1, regT0);
    
    // We've already generated the following get_by_id, so make sure it's skipped over.
    m_bytecodeOffset += OPCODE_LENGTH(op_get_by_id);

    m_propertyAccessCompilationInfo.last().addMethodCheckInfo(info.structureToCompare, protoObj, protoStructureToCompare, putFunction);
}

void JIT::emitSlow_op_method_check(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    currentInstruction += OPCODE_LENGTH(op_method_check);
    
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int ident = currentInstruction[3].u.operand;
    
    compileGetByIdSlowCase(dst, base, &(m_codeBlock->identifier(ident)), iter, true);
    emitValueProfilingSite(m_bytecodeOffset + OPCODE_LENGTH(op_method_check));
    
    // We've already generated the following get_by_id, so make sure it's skipped over.
    m_bytecodeOffset += OPCODE_LENGTH(op_get_by_id);
}

JIT::CodeRef JIT::stringGetByValStubGenerator(JSGlobalData* globalData)
{
    JSInterfaceJIT jit;
    JumpList failures;
    failures.append(jit.branchPtr(NotEqual, Address(regT0, JSCell::classInfoOffset()), TrustedImmPtr(&JSString::s_info)));
    
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
    jit.loadPtr(Address(regT0, ThunkHelpers::stringImplFlagsOffset()), regT1);
    jit.loadPtr(Address(regT0, ThunkHelpers::stringImplDataOffset()), regT0);
    is16Bit.append(jit.branchTest32(Zero, regT1, TrustedImm32(ThunkHelpers::stringImpl8BitFlag())));
    jit.load8(BaseIndex(regT0, regT2, TimesOne, 0), regT0);
    cont8Bit.append(jit.jump());
    is16Bit.link(&jit);
    jit.load16(BaseIndex(regT0, regT2, TimesTwo, 0), regT0);

    cont8Bit.link(&jit);
    
    failures.append(jit.branch32(AboveOrEqual, regT0, TrustedImm32(0x100)));
    jit.move(TrustedImmPtr(globalData->smallStrings.singleCharacterStrings()), regT1);
    jit.loadPtr(BaseIndex(regT1, regT0, ScalePtr, 0), regT0);
    jit.move(TrustedImm32(JSValue::CellTag), regT1); // We null check regT0 on return so this is safe
    jit.ret();

    failures.link(&jit);
    jit.move(TrustedImm32(0), regT0);
    jit.ret();
    
    LinkBuffer patchBuffer(*globalData, &jit, GLOBAL_THUNK_ID);
    return patchBuffer.finalizeCode();
}

void JIT::emit_op_get_by_val(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::classInfoOffset()), TrustedImmPtr(&JSArray::s_info)));
    
    loadPtr(Address(regT0, JSArray::storageOffset()), regT3);
    addSlowCase(branch32(AboveOrEqual, regT2, Address(regT0, JSArray::vectorLengthOffset())));
    
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1); // tag
    load32(BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0); // payload
    addSlowCase(branch32(Equal, regT1, TrustedImm32(JSValue::EmptyValueTag)));
    
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_val), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check

    Jump nonCell = jump();
    linkSlowCase(iter); // base array check
    Jump notString = branchPtr(NotEqual, Address(regT0, JSCell::classInfoOffset()), TrustedImmPtr(&JSString::s_info));
    emitNakedCall(m_globalData->getCTIStub(stringGetByValStubGenerator).code());
    Jump failed = branchTestPtr(Zero, regT0);
    emitStore(dst, regT1, regT0);
    emitJumpSlowToHot(jump(), OPCODE_LENGTH(op_get_by_val));
    failed.link(this);
    notString.link(this);
    nonCell.link(this);

    linkSlowCase(iter); // vector length check
    linkSlowCase(iter); // empty value
    
    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(base);
    stubCall.addArgument(property);
    stubCall.call(dst);

    emitValueProfilingSite();
}

void JIT::emit_op_put_by_val(Instruction* currentInstruction)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;
    
    emitLoad2(base, regT1, regT0, property, regT3, regT2);
    
    addSlowCase(branch32(NotEqual, regT3, TrustedImm32(JSValue::Int32Tag)));
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    addSlowCase(branchPtr(NotEqual, Address(regT0, JSCell::classInfoOffset()), TrustedImmPtr(&JSArray::s_info)));
    addSlowCase(branch32(AboveOrEqual, regT2, Address(regT0, JSArray::vectorLengthOffset())));

    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);
    loadPtr(Address(regT0, JSArray::storageOffset()), regT3);
    
    Jump empty = branch32(Equal, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
    
    Label storeResult(this);
    emitLoad(value, regT1, regT0);
    store32(regT0, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload))); // payload
    store32(regT1, BaseIndex(regT3, regT2, TimesEight, OBJECT_OFFSETOF(ArrayStorage, m_vector[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag))); // tag
    Jump end = jump();
    
    empty.link(this);
    add32(TrustedImm32(1), Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_numValuesInVector)));
    branch32(Below, regT2, Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_length))).linkTo(storeResult, this);
    
    add32(TrustedImm32(1), regT2, regT0);
    store32(regT0, Address(regT3, OBJECT_OFFSETOF(ArrayStorage, m_length)));
    jump().linkTo(storeResult, this);
    
    end.link(this);
}

void JIT::emitSlow_op_put_by_val(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned base = currentInstruction[1].u.operand;
    unsigned property = currentInstruction[2].u.operand;
    unsigned value = currentInstruction[3].u.operand;
    
    linkSlowCase(iter); // property int32 check
    linkSlowCaseIfNotJSCell(iter, base); // base cell check
    linkSlowCase(iter); // base not array check
    linkSlowCase(iter); // in vector check
    
    JITStubCall stubPutByValCall(this, cti_op_put_by_val);
    stubPutByValCall.addArgument(base);
    stubPutByValCall.addArgument(property);
    stubPutByValCall.addArgument(value);
    stubPutByValCall.call();
}

void JIT::emit_op_get_by_id(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    
    emitLoad(base, regT1, regT0);
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    compileGetByIdHotPath();
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_id), dst, regT1, regT0);
}

void JIT::compileGetByIdHotPath()
{
    // As for put_by_id, get_by_id requires the offset of the Structure and the offset of the access to be patched.
    // Additionally, for get_by_id we need patch the offset of the branch to the slow case (we patch this to jump
    // to array-length / prototype access tranpolines, and finally we also the the property-map access offset as a label
    // to jump back to if one of these trampolies finds a match.
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);
    
    Label hotPathBegin(this);
    
    DataLabelPtr structureToCompare;
    PatchableJump structureCheck = patchableBranchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure)));
    addSlowCase(structureCheck);
    
    loadPtr(Address(regT0, JSObject::offsetOfPropertyStorage()), regT2);
    DataLabelCompact displacementLabel1 = loadPtrWithCompactAddressOffsetPatch(Address(regT2, patchGetByIdDefaultOffset), regT0); // payload
    DataLabelCompact displacementLabel2 = loadPtrWithCompactAddressOffsetPatch(Address(regT2, patchGetByIdDefaultOffset), regT1); // tag
    
    Label putResult(this);
    
    END_UNINTERRUPTED_SEQUENCE(sequenceGetByIdHotPath);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubGetById, m_bytecodeOffset, hotPathBegin, structureToCompare, structureCheck, displacementLabel1, displacementLabel2, putResult));
}

void JIT::emitSlow_op_get_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int dst = currentInstruction[1].u.operand;
    int base = currentInstruction[2].u.operand;
    int ident = currentInstruction[3].u.operand;
    
    compileGetByIdSlowCase(dst, base, &(m_codeBlock->identifier(ident)), iter);
    emitValueProfilingSite();
}

void JIT::compileGetByIdSlowCase(int dst, int base, Identifier* ident, Vector<SlowCaseEntry>::iterator& iter, bool isMethodCheck)
{
    // As for the hot path of get_by_id, above, we ensure that we can use an architecture specific offset
    // so that we only need track one pointer into the slow case code - we track a pointer to the location
    // of the call (which we can use to look up the patch information), but should a array-length or
    // prototype access trampoline fail we want to bail out back to here.  To do so we can subtract back
    // the distance from the call to the head of the slow case.
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequenceGetByIdSlowCase);
    
    Label coldPathBegin(this);
    JITStubCall stubCall(this, isMethodCheck ? cti_op_get_by_id_method_check : cti_op_get_by_id);
    stubCall.addArgument(regT1, regT0);
    stubCall.addArgument(TrustedImmPtr(ident));
    Call call = stubCall.call(dst);
    
    END_UNINTERRUPTED_SEQUENCE_FOR_PUT(sequenceGetByIdSlowCase, dst);
    
    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubGetById, coldPathBegin, call);
}

void JIT::emit_op_put_by_id(Instruction* currentInstruction)
{
    // In order to be able to patch both the Structure, and the object offset, we store one pointer,
    // to just after the arguments have been loaded into registers 'hotPathBegin', and we generate code
    // such that the Structure & offset are always at the same distance from this.
    
    int base = currentInstruction[1].u.operand;
    int value = currentInstruction[3].u.operand;
    
    emitLoad2(base, regT1, regT0, value, regT3, regT2);
    
    emitJumpSlowCaseIfNotJSCell(base, regT1);
    
    BEGIN_UNINTERRUPTED_SEQUENCE(sequencePutById);
    
    Label hotPathBegin(this);
    
    // It is important that the following instruction plants a 32bit immediate, in order that it can be patched over.
    DataLabelPtr structureToCompare;
    addSlowCase(branchPtrWithPatch(NotEqual, Address(regT0, JSCell::structureOffset()), structureToCompare, TrustedImmPtr(reinterpret_cast<void*>(patchGetByIdDefaultStructure))));
    
    loadPtr(Address(regT0, JSObject::offsetOfPropertyStorage()), regT1);
    DataLabel32 displacementLabel1 = storePtrWithAddressOffsetPatch(regT2, Address(regT1, patchPutByIdDefaultOffset)); // payload
    DataLabel32 displacementLabel2 = storePtrWithAddressOffsetPatch(regT3, Address(regT1, patchPutByIdDefaultOffset)); // tag
    
    END_UNINTERRUPTED_SEQUENCE(sequencePutById);

    emitWriteBarrier(regT0, regT2, regT1, regT2, ShouldFilterImmediates, WriteBarrierForPropertyAccess);

    m_propertyAccessCompilationInfo.append(PropertyStubCompilationInfo(PropertyStubPutById, m_bytecodeOffset, hotPathBegin, structureToCompare, displacementLabel1, displacementLabel2));
}

void JIT::emitSlow_op_put_by_id(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    int base = currentInstruction[1].u.operand;
    int ident = currentInstruction[2].u.operand;
    int direct = currentInstruction[8].u.operand;

    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    
    JITStubCall stubCall(this, direct ? cti_op_put_by_id_direct : cti_op_put_by_id);
    stubCall.addArgument(base);
    stubCall.addArgument(TrustedImmPtr(&(m_codeBlock->identifier(ident))));
    stubCall.addArgument(regT3, regT2); 
    Call call = stubCall.call();
    
    // Track the location of the call; this will be used to recover patch information.
    m_propertyAccessCompilationInfo[m_propertyAccessInstructionIndex++].slowCaseInfo(PropertyStubPutById, call);
}

// Compile a store into an object's property storage.  May overwrite base.
void JIT::compilePutDirectOffset(RegisterID base, RegisterID valueTag, RegisterID valuePayload, size_t cachedOffset)
{
    int offset = cachedOffset;
    loadPtr(Address(base, JSObject::offsetOfPropertyStorage()), base);
    emitStore(offset, valueTag, valuePayload, base);
}

// Compile a load from an object's property storage.  May overwrite base.
void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, size_t cachedOffset)
{
    int offset = cachedOffset;
    RegisterID temp = resultPayload;
    loadPtr(Address(base, JSObject::offsetOfPropertyStorage()), temp);
    emitLoad(offset, resultTag, resultPayload, temp);
}

void JIT::compileGetDirectOffset(JSObject* base, RegisterID resultTag, RegisterID resultPayload, size_t cachedOffset)
{
    loadPtr(base->addressOfPropertyStorage(), resultTag);
    load32(Address(resultTag, cachedOffset * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
    load32(Address(resultTag, cachedOffset * sizeof(WriteBarrier<Unknown>) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
}

void JIT::privateCompilePutByIdTransition(StructureStubInfo* stubInfo, Structure* oldStructure, Structure* newStructure, size_t cachedOffset, StructureChain* chain, ReturnAddressPtr returnAddress, bool direct)
{
    // The code below assumes that regT0 contains the basePayload and regT1 contains the baseTag. Restore them from the stack.
#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#else
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#endif

    JumpList failureCases;
    failureCases.append(branch32(NotEqual, regT1, TrustedImm32(JSValue::CellTag)));
    failureCases.append(branchPtr(NotEqual, Address(regT0, JSCell::structureOffset()), TrustedImmPtr(oldStructure)));
    testPrototype(oldStructure->storedPrototype(), failureCases);
    
    if (!direct) {
        // Verify that nothing in the prototype chain has a setter for this property. 
        for (WriteBarrier<Structure>* it = chain->head(); *it; ++it)
            testPrototype((*it)->storedPrototype(), failureCases);
    }

    // If we succeed in all of our checks, and the code was optimizable, then make sure we
    // decrement the rare case counter.
#if ENABLE(VALUE_PROFILER)
    if (m_codeBlock->canCompileWithDFG()) {
        sub32(
            TrustedImm32(1),
            AbsoluteAddress(&m_codeBlock->rareCaseProfileForBytecodeOffset(stubInfo->bytecodeIndex)->m_counter));
    }
#endif
    
    // Reallocate property storage if needed.
    Call callTarget;
    bool willNeedStorageRealloc = oldStructure->propertyStorageCapacity() != newStructure->propertyStorageCapacity();
    if (willNeedStorageRealloc) {
        // This trampoline was called to like a JIT stub; before we can can call again we need to
        // remove the return address from the stack, to prevent the stack from becoming misaligned.
        preserveReturnAddressAfterCall(regT3);
        
        JITStubCall stubCall(this, cti_op_put_by_id_transition_realloc);
        stubCall.skipArgument(); // base
        stubCall.skipArgument(); // ident
        stubCall.skipArgument(); // value
        stubCall.addArgument(TrustedImm32(oldStructure->propertyStorageCapacity()));
        stubCall.addArgument(TrustedImmPtr(newStructure));
        stubCall.call(regT0);

        restoreReturnAddressBeforeReturn(regT3);

#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
        // For MIPS, we don't add sizeof(void*) to the stack offset.
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
        // For MIPS, we don't add sizeof(void*) to the stack offset.
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#else
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT0);
        load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[0]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT1);
#endif
    }

    emitWriteBarrier(regT0, regT1, regT1, regT3, UnconditionalWriteBarrier, WriteBarrierForPropertyAccess);

    storePtr(TrustedImmPtr(newStructure), Address(regT0, JSCell::structureOffset()));
#if CPU(MIPS) || CPU(SH4) || CPU(ARM)
    // For MIPS, we don't add sizeof(void*) to the stack offset.
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT3);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT2);
#else
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.payload)), regT3);
    load32(Address(stackPointerRegister, OBJECT_OFFSETOF(JITStackFrame, args[2]) + sizeof(void*) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), regT2);
#endif
    compilePutDirectOffset(regT0, regT2, regT3, cachedOffset);
    
    ret();
    
    ASSERT(!failureCases.empty());
    failureCases.link(this);
    restoreArgumentReferenceForTrampoline();
    Call failureCall = tailRecursiveCall();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    
    patchBuffer.link(failureCall, FunctionPtr(direct ? cti_op_put_by_id_direct_fail : cti_op_put_by_id_fail));
    
    if (willNeedStorageRealloc) {
        ASSERT(m_calls.size() == 1);
        patchBuffer.link(m_calls[0].from, FunctionPtr(cti_op_put_by_id_transition_realloc));
    }
    
    stubInfo->stubRoutine = patchBuffer.finalizeCode();
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relinkCallerToTrampoline(returnAddress, CodeLocationLabel(stubInfo->stubRoutine.code()));
}

void JIT::patchGetByIdSelf(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, size_t cachedOffset, ReturnAddressPtr returnAddress)
{
    RepatchBuffer repatchBuffer(codeBlock);
    
    // We don't want to patch more than once - in future go to cti_op_get_by_id_generic.
    // Should probably go to JITStubs::cti_op_get_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_self_fail));
    
    int offset = sizeof(JSValue) * cachedOffset;

    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), structure);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel1), offset + OBJECT_OFFSETOF(JSValue, u.asBits.payload)); // payload
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel2), offset + OBJECT_OFFSETOF(JSValue, u.asBits.tag)); // tag
}

void JIT::patchPutByIdReplace(CodeBlock* codeBlock, StructureStubInfo* stubInfo, Structure* structure, size_t cachedOffset, ReturnAddressPtr returnAddress, bool direct)
{
    RepatchBuffer repatchBuffer(codeBlock);
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    // Should probably go to cti_op_put_by_id_fail, but that doesn't do anything interesting right now.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(direct ? cti_op_put_by_id_direct_generic : cti_op_put_by_id_generic));
    
    int offset = sizeof(JSValue) * cachedOffset;

    // Patch the offset into the propoerty map to load from, then patch the Structure to look for.
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.put.structureToCompare), structure);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel1), offset + OBJECT_OFFSETOF(JSValue, u.asBits.payload)); // payload
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel2), offset + OBJECT_OFFSETOF(JSValue, u.asBits.tag)); // tag
}

void JIT::privateCompilePatchGetArrayLength(ReturnAddressPtr returnAddress)
{
    StructureStubInfo* stubInfo = &m_codeBlock->getStubInfo(returnAddress);
    
    // regT0 holds a JSCell*
    
    // Check for array
    Jump failureCases1 = branchPtr(NotEqual, Address(regT0, JSCell::classInfoOffset()), TrustedImmPtr(&JSArray::s_info));
    
    // Checks out okay! - get the length from the storage
    loadPtr(Address(regT0, JSArray::storageOffset()), regT2);
    load32(Address(regT2, OBJECT_OFFSETOF(ArrayStorage, m_length)), regT2);
    
    Jump failureCases2 = branch32(Above, regT2, TrustedImm32(INT_MAX));
    move(regT2, regT0);
    move(TrustedImm32(JSValue::Int32Tag), regT1);
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    patchBuffer.link(failureCases1, slowCaseBegin);
    patchBuffer.link(failureCases2, slowCaseBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    // Track the stub we have created so that it will be deleted later.
    stubInfo->stubRoutine = patchBuffer.finalizeCode();
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubInfo->stubRoutine.code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_array_fail));
}

void JIT::privateCompileGetByIdProto(StructureStubInfo* stubInfo, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));
    
    Jump failureCases1 = checkStructure(regT0, structure);
    
    // Check the prototype object's Structure had not changed.
    move(TrustedImmPtr(protoObject), regT3);
    Jump failureCases2 = branchPtr(NotEqual, Address(regT3, JSCell::structureOffset()), TrustedImmPtr(prototypeStructure));

    bool needsStubLink = false;
    // Checks out okay!
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel slowCaseBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    patchBuffer.link(failureCases1, slowCaseBegin);
    patchBuffer.link(failureCases2, slowCaseBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));

    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }

    // Track the stub we have created so that it will be deleted later.
    stubInfo->stubRoutine = patchBuffer.finalizeCode();
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubInfo->stubRoutine.code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}


void JIT::privateCompileGetByIdSelfList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* polymorphicStructures, int currentIndex, Structure* structure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset)
{
    // regT0 holds a JSCell*
    Jump failureCase = checkStructure(regT0, structure);
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(regT0, regT2, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else {
        isDirect = true;
        compileGetDirectOffset(regT0, regT1, regT0, cachedOffset);
    }

    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }    
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(polymorphicStructures->list[currentIndex - 1].stubRoutine.code());
    if (!lastProtoBegin)
        lastProtoBegin = stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin);
    
    patchBuffer.link(failureCase, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));

    CodeRef stubRoutine = patchBuffer.finalizeCode();

    polymorphicStructures->list[currentIndex].set(*m_globalData, m_codeBlock->ownerExecutable(), stubRoutine, structure, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine.code()));
}

void JIT::privateCompileGetByIdProtoList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, Structure* prototypeStructure, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    
    // The prototype object definitely exists (if this stub exists the CodeBlock is referencing a Structure that is
    // referencing the prototype object - let's speculatively load it's table nice and early!)
    JSObject* protoObject = asObject(structure->prototypeForLookup(callFrame));
    
    // Check eax is an object of the right Structure.
    Jump failureCases1 = checkStructure(regT0, structure);
    
    // Check the prototype object's Structure had not changed.
    move(TrustedImmPtr(protoObject), regT3);
    Jump failureCases2 = branchPtr(NotEqual, Address(regT3, JSCell::structureOffset()), TrustedImmPtr(prototypeStructure));
    
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    }
    
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(prototypeStructures->list[currentIndex - 1].stubRoutine.code());
    patchBuffer.link(failureCases1, lastProtoBegin);
    patchBuffer.link(failureCases2, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    CodeRef stubRoutine = patchBuffer.finalizeCode();

    prototypeStructures->list[currentIndex].set(callFrame->globalData(), m_codeBlock->ownerExecutable(), stubRoutine, structure, prototypeStructure, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine.code()));
}

void JIT::privateCompileGetByIdChainList(StructureStubInfo* stubInfo, PolymorphicAccessStructureList* prototypeStructures, int currentIndex, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    ASSERT(count);
    
    JumpList bucketsOfFail;
    
    // Check eax is an object of the right Structure.
    bucketsOfFail.append(checkStructure(regT0, structure));
    
    Structure* currStructure = structure;
    WriteBarrier<Structure>* it = chain->head();
    JSObject* protoObject = 0;
    for (unsigned i = 0; i < count; ++i, ++it) {
        protoObject = asObject(currStructure->prototypeForLookup(callFrame));
        currStructure = it->get();
        testPrototype(protoObject, bucketsOfFail);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    bool isDirect = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else {
        isDirect = true;
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    }

    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    CodeLocationLabel lastProtoBegin = CodeLocationLabel(prototypeStructures->list[currentIndex - 1].stubRoutine.code());
    
    patchBuffer.link(bucketsOfFail, lastProtoBegin);
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    CodeRef stubRoutine = patchBuffer.finalizeCode();
    
    // Track the stub we have created so that it will be deleted later.
    prototypeStructures->list[currentIndex].set(callFrame->globalData(), m_codeBlock->ownerExecutable(), stubRoutine, structure, chain, isDirect);
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine.code()));
}

void JIT::privateCompileGetByIdChain(StructureStubInfo* stubInfo, Structure* structure, StructureChain* chain, size_t count, const Identifier& ident, const PropertySlot& slot, size_t cachedOffset, ReturnAddressPtr returnAddress, CallFrame* callFrame)
{
    // regT0 holds a JSCell*
    ASSERT(count);
    
    JumpList bucketsOfFail;
    
    // Check eax is an object of the right Structure.
    bucketsOfFail.append(checkStructure(regT0, structure));
    
    Structure* currStructure = structure;
    WriteBarrier<Structure>* it = chain->head();
    JSObject* protoObject = 0;
    for (unsigned i = 0; i < count; ++i, ++it) {
        protoObject = asObject(currStructure->prototypeForLookup(callFrame));
        currStructure = it->get();
        testPrototype(protoObject, bucketsOfFail);
    }
    ASSERT(protoObject);
    
    bool needsStubLink = false;
    if (slot.cachedPropertyType() == PropertySlot::Getter) {
        needsStubLink = true;
        compileGetDirectOffset(protoObject, regT2, regT1, cachedOffset);
        JITStubCall stubCall(this, cti_op_get_by_id_getter_stub);
        stubCall.addArgument(regT1);
        stubCall.addArgument(regT0);
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else if (slot.cachedPropertyType() == PropertySlot::Custom) {
        needsStubLink = true;
        JITStubCall stubCall(this, cti_op_get_by_id_custom_stub);
        stubCall.addArgument(TrustedImmPtr(protoObject));
        stubCall.addArgument(TrustedImmPtr(FunctionPtr(slot.customGetter()).executableAddress()));
        stubCall.addArgument(TrustedImmPtr(const_cast<Identifier*>(&ident)));
        stubCall.addArgument(TrustedImmPtr(stubInfo->callReturnLocation.executableAddress()));
        stubCall.call();
    } else
        compileGetDirectOffset(protoObject, regT1, regT0, cachedOffset);
    Jump success = jump();
    
    LinkBuffer patchBuffer(*m_globalData, this, m_codeBlock);
    if (needsStubLink) {
        for (Vector<CallRecord>::iterator iter = m_calls.begin(); iter != m_calls.end(); ++iter) {
            if (iter->to)
                patchBuffer.link(iter->from, FunctionPtr(iter->to));
        }
    }
    // Use the patch information to link the failure cases back to the original slow case routine.
    patchBuffer.link(bucketsOfFail, stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin));
    
    // On success return back to the hot patch code, at a point it will perform the store to dest for us.
    patchBuffer.link(success, stubInfo->hotPathBegin.labelAtOffset(stubInfo->patch.baseline.u.get.putResult));
    
    // Track the stub we have created so that it will be deleted later.
    CodeRef stubRoutine = patchBuffer.finalizeCode();
    stubInfo->stubRoutine = stubRoutine;
    
    // Finally patch the jump to slow case back in the hot path to jump here instead.
    CodeLocationJump jumpLocation = stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck);
    RepatchBuffer repatchBuffer(m_codeBlock);
    repatchBuffer.relink(jumpLocation, CodeLocationLabel(stubRoutine.code()));
    
    // We don't want to patch more than once - in future go to cti_op_put_by_id_generic.
    repatchBuffer.relinkCallerToFunction(returnAddress, FunctionPtr(cti_op_get_by_id_proto_list));
}

void JIT::compileGetDirectOffset(RegisterID base, RegisterID resultTag, RegisterID resultPayload, RegisterID offset)
{
    ASSERT(sizeof(JSValue) == 8);
    
    loadPtr(Address(base, JSObject::offsetOfPropertyStorage()), base);
    loadPtr(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.payload)), resultPayload);
    loadPtr(BaseIndex(base, offset, TimesEight, OBJECT_OFFSETOF(JSValue, u.asBits.tag)), resultTag);
}

void JIT::emit_op_get_by_pname(Instruction* currentInstruction)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    unsigned expected = currentInstruction[4].u.operand;
    unsigned iter = currentInstruction[5].u.operand;
    unsigned i = currentInstruction[6].u.operand;
    
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
    compileGetDirectOffset(regT2, regT1, regT0, regT3);    
    
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_by_pname), dst, regT1, regT0);
}

void JIT::emitSlow_op_get_by_pname(Instruction* currentInstruction, Vector<SlowCaseEntry>::iterator& iter)
{
    unsigned dst = currentInstruction[1].u.operand;
    unsigned base = currentInstruction[2].u.operand;
    unsigned property = currentInstruction[3].u.operand;
    
    linkSlowCaseIfNotJSCell(iter, property);
    linkSlowCase(iter);
    linkSlowCaseIfNotJSCell(iter, base);
    linkSlowCase(iter);
    linkSlowCase(iter);
    
    JITStubCall stubCall(this, cti_op_get_by_val);
    stubCall.addArgument(base);
    stubCall.addArgument(property);
    stubCall.call(dst);
}

void JIT::emit_op_get_scoped_var(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    int index = currentInstruction[2].u.operand;
    int skip = currentInstruction[3].u.operand;

    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT2);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branch32(Equal, tagFor(m_codeBlock->activationRegister()), TrustedImm32(JSValue::EmptyValueTag));
        loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, next)), regT2);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, next)), regT2);

    loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, object)), regT2);
    loadPtr(Address(regT2, JSVariableObject::offsetOfRegisters()), regT2);

    emitLoad(index, regT1, regT0, regT2);
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_scoped_var), dst, regT1, regT0);
}

void JIT::emit_op_put_scoped_var(Instruction* currentInstruction)
{
    int index = currentInstruction[1].u.operand;
    int skip = currentInstruction[2].u.operand;
    int value = currentInstruction[3].u.operand;

    emitLoad(value, regT1, regT0);

    emitGetFromCallFrameHeaderPtr(RegisterFile::ScopeChain, regT2);
    bool checkTopLevel = m_codeBlock->codeType() == FunctionCode && m_codeBlock->needsFullScopeChain();
    ASSERT(skip || !checkTopLevel);
    if (checkTopLevel && skip--) {
        Jump activationNotCreated;
        if (checkTopLevel)
            activationNotCreated = branch32(Equal, tagFor(m_codeBlock->activationRegister()), TrustedImm32(JSValue::EmptyValueTag));
        loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, next)), regT2);
        activationNotCreated.link(this);
    }
    while (skip--)
        loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, next)), regT2);
    loadPtr(Address(regT2, OBJECT_OFFSETOF(ScopeChainNode, object)), regT2);

    loadPtr(Address(regT2, JSVariableObject::offsetOfRegisters()), regT3);
    emitStore(index, regT1, regT0, regT3);
    emitWriteBarrier(regT2, regT1, regT0, regT1, ShouldFilterImmediates, WriteBarrierForVariableAccess);
}

void JIT::emit_op_get_global_var(Instruction* currentInstruction)
{
    int dst = currentInstruction[1].u.operand;
    JSGlobalObject* globalObject = m_codeBlock->globalObject();
    ASSERT(globalObject->isGlobalObject());
    int index = currentInstruction[2].u.operand;

    loadPtr(&globalObject->m_registers, regT2);

    emitLoad(index, regT1, regT0, regT2);
    emitValueProfilingSite();
    emitStore(dst, regT1, regT0);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_get_global_var), dst, regT1, regT0);
}

void JIT::emit_op_put_global_var(Instruction* currentInstruction)
{
    int index = currentInstruction[1].u.operand;
    int value = currentInstruction[2].u.operand;

    JSGlobalObject* globalObject = m_codeBlock->globalObject();

    emitLoad(value, regT1, regT0);
    move(TrustedImmPtr(globalObject), regT2);

    emitWriteBarrier(globalObject, regT1, regT3, ShouldFilterImmediates, WriteBarrierForVariableAccess);

    loadPtr(Address(regT2, JSVariableObject::offsetOfRegisters()), regT2);
    emitStore(index, regT1, regT0, regT2);
    map(m_bytecodeOffset + OPCODE_LENGTH(op_put_global_var), value, regT1, regT0);
}

void JIT::resetPatchGetById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_get_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.get.structureToCompare), reinterpret_cast<void*>(-1));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel1), 0);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelCompactAtOffset(stubInfo->patch.baseline.u.get.displacementLabel2), 0);
    repatchBuffer.relink(stubInfo->hotPathBegin.jumpAtOffset(stubInfo->patch.baseline.u.get.structureCheck), stubInfo->callReturnLocation.labelAtOffset(-stubInfo->patch.baseline.u.get.coldPathBegin));
}

void JIT::resetPatchPutById(RepatchBuffer& repatchBuffer, StructureStubInfo* stubInfo)
{
    if (isDirectPutById(stubInfo))
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id_direct);
    else
        repatchBuffer.relink(stubInfo->callReturnLocation, cti_op_put_by_id);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabelPtrAtOffset(stubInfo->patch.baseline.u.put.structureToCompare), reinterpret_cast<void*>(-1));
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel1), 0);
    repatchBuffer.repatch(stubInfo->hotPathBegin.dataLabel32AtOffset(stubInfo->patch.baseline.u.put.displacementLabel2), 0);
}

} // namespace JSC

#endif // USE(JSVALUE32_64)
#endif // ENABLE(JIT)
