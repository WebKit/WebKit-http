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
#include "LLIntData.h"

#if ENABLE(LLINT)

#include "BytecodeConventions.h"
#include "CodeType.h"
#include "Instruction.h"
#include "JSScope.h"
#include "LLIntCLoop.h"
#include "Opcode.h"
#include "PropertyOffset.h"

namespace JSC { namespace LLInt {

Instruction* Data::s_exceptionInstructions = 0;
Opcode* Data::s_opcodeMap = 0;

void initialize()
{
    Data::s_exceptionInstructions = new Instruction[maxOpcodeLength + 1];
    Data::s_opcodeMap = new Opcode[numOpcodeIDs];

    #if ENABLE(LLINT_C_LOOP)
    CLoop::initialize();

    #else // !ENABLE(LLINT_C_LOOP)
    for (int i = 0; i < maxOpcodeLength + 1; ++i)
        Data::s_exceptionInstructions[i].u.pointer =
            LLInt::getCodePtr(llint_throw_from_slow_path_trampoline);
    #define OPCODE_ENTRY(opcode, length) \
        Data::s_opcodeMap[opcode] = LLInt::getCodePtr(llint_##opcode);
    FOR_EACH_OPCODE_ID(OPCODE_ENTRY);
    #undef OPCODE_ENTRY
    #endif // !ENABLE(LLINT_C_LOOP)
}

#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wmissing-noreturn"
#endif
void Data::performAssertions(VM& vm)
{
    UNUSED_PARAM(vm);
    
    // Assertions to match LowLevelInterpreter.asm.  If you change any of this code, be
    // prepared to change LowLevelInterpreter.asm as well!!

#ifndef NDEBUG
#if USE(JSVALUE64)
    const ptrdiff_t PtrSize = 8;
    const ptrdiff_t CallFrameHeaderSlots = 6;
#else // USE(JSVALUE64) // i.e. 32-bit version
    const ptrdiff_t PtrSize = 4;
    const ptrdiff_t CallFrameHeaderSlots = 5;
#endif
    const ptrdiff_t SlotSize = 8;
#endif

    ASSERT(sizeof(void*) == PtrSize);
    ASSERT(sizeof(Register) == SlotSize);
    ASSERT(JSStack::CallFrameHeaderSize == CallFrameHeaderSlots);

    ASSERT(!CallFrame::callerFrameOffset());
    ASSERT(CallFrame::returnPCOffset() == CallFrame::callerFrameOffset() + PtrSize);
    ASSERT(JSStack::CodeBlock * sizeof(Register) == CallFrame::returnPCOffset() + PtrSize);
    ASSERT(JSStack::ScopeChain * sizeof(Register) == JSStack::CodeBlock * sizeof(Register) + SlotSize);
    ASSERT(JSStack::Callee * sizeof(Register) == JSStack::ScopeChain * sizeof(Register) + SlotSize);
    ASSERT(JSStack::ArgumentCount * sizeof(Register) == JSStack::Callee * sizeof(Register) + SlotSize);
    ASSERT(JSStack::ThisArgument * sizeof(Register) == JSStack::ArgumentCount * sizeof(Register) + SlotSize);
    ASSERT(JSStack::CallFrameHeaderSize == JSStack::ThisArgument);

    ASSERT(CallFrame::argumentOffsetIncludingThis(0) == JSStack::ThisArgument);

#if CPU(BIG_ENDIAN)
    ASSERT(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag) == 0);
    ASSERT(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload) == 4);
#else
    ASSERT(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag) == 4);
    ASSERT(OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload) == 0);
#endif
#if USE(JSVALUE32_64)
    ASSERT(JSValue::Int32Tag == static_cast<unsigned>(-1));
    ASSERT(JSValue::BooleanTag == static_cast<unsigned>(-2));
    ASSERT(JSValue::NullTag == static_cast<unsigned>(-3));
    ASSERT(JSValue::UndefinedTag == static_cast<unsigned>(-4));
    ASSERT(JSValue::CellTag == static_cast<unsigned>(-5));
    ASSERT(JSValue::EmptyValueTag == static_cast<unsigned>(-6));
    ASSERT(JSValue::DeletedValueTag == static_cast<unsigned>(-7));
    ASSERT(JSValue::LowestTag == static_cast<unsigned>(-7));
#else
    ASSERT(TagBitTypeOther == 0x2);
    ASSERT(TagBitBool == 0x4);
    ASSERT(TagBitUndefined == 0x8);
    ASSERT(ValueEmpty == 0x0);
    ASSERT(ValueFalse == (TagBitTypeOther | TagBitBool));
    ASSERT(ValueTrue == (TagBitTypeOther | TagBitBool | 1));
    ASSERT(ValueUndefined == (TagBitTypeOther | TagBitUndefined));
    ASSERT(ValueNull == TagBitTypeOther);
#endif
    ASSERT(StringType == 5);
    ASSERT(ObjectType == 17);
    ASSERT(FinalObjectType == 18);
    ASSERT(MasqueradesAsUndefined == 1);
    ASSERT(ImplementsHasInstance == 2);
    ASSERT(ImplementsDefaultHasInstance == 8);
    ASSERT(FirstConstantRegisterIndex == 0x40000000);
    ASSERT(GlobalCode == 0);
    ASSERT(EvalCode == 1);
    ASSERT(FunctionCode == 2);

    ASSERT(GlobalProperty == 0);
    ASSERT(GlobalVar == 1);
    ASSERT(ClosureVar == 2);
    ASSERT(GlobalPropertyWithVarInjectionChecks == 3);
    ASSERT(GlobalVarWithVarInjectionChecks == 4);
    ASSERT(ClosureVarWithVarInjectionChecks == 5);
    ASSERT(Dynamic == 6);
    
    ASSERT(ResolveModeAndType::mask == 0xffff);

    ASSERT(MarkedBlock::blockMask == ~static_cast<decltype(MarkedBlock::blockMask)>(0xffff));

    // FIXME: make these assertions less horrible.
#if !ASSERT_DISABLED
    Vector<int> testVector;
    testVector.resize(42);
    ASSERT(bitwise_cast<uint32_t*>(&testVector)[sizeof(void*)/sizeof(uint32_t) + 1] == 42);
    ASSERT(bitwise_cast<int**>(&testVector)[0] == testVector.begin());
#endif

    ASSERT(StringImpl::s_hashFlag8BitBuffer == 32);
}
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif

} } // namespace JSC::LLInt

#endif // ENABLE(LLINT)
