/*
 * Copyright (C) 2011, 2013 Apple Inc. All rights reserved.
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

#ifndef AssemblyHelpers_h
#define AssemblyHelpers_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "CodeBlock.h"
#include "FPRInfo.h"
#include "GPRInfo.h"
#include "JITCode.h"
#include "MacroAssembler.h"
#include "VM.h"

namespace JSC {

typedef void (*V_DebugOperation_EPP)(ExecState*, void*, void*);

class AssemblyHelpers : public MacroAssembler {
public:
    AssemblyHelpers(VM* vm, CodeBlock* codeBlock)
        : m_vm(vm)
        , m_codeBlock(codeBlock)
        , m_baselineCodeBlock(codeBlock ? codeBlock->baselineAlternative() : 0)
    {
        if (m_codeBlock) {
            ASSERT(m_baselineCodeBlock);
            ASSERT(!m_baselineCodeBlock->alternative());
            ASSERT(m_baselineCodeBlock->jitType() == JITCode::None || JITCode::isBaselineCode(m_baselineCodeBlock->jitType()));
        }
    }
    
    CodeBlock* codeBlock() { return m_codeBlock; }
    VM* vm() { return m_vm; }
    AssemblerType_T& assembler() { return m_assembler; }
    
#if CPU(X86_64) || CPU(X86)
    void preserveReturnAddressAfterCall(GPRReg reg)
    {
        pop(reg);
    }

    void restoreReturnAddressBeforeReturn(GPRReg reg)
    {
        push(reg);
    }

    void restoreReturnAddressBeforeReturn(Address address)
    {
        push(address);
    }
#endif // CPU(X86_64) || CPU(X86)

#if CPU(ARM) || CPU(ARM64)
    ALWAYS_INLINE void preserveReturnAddressAfterCall(RegisterID reg)
    {
        move(linkRegister, reg);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        move(reg, linkRegister);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtr(address, linkRegister);
    }
#endif

#if CPU(MIPS)
    ALWAYS_INLINE void preserveReturnAddressAfterCall(RegisterID reg)
    {
        move(returnAddressRegister, reg);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        move(reg, returnAddressRegister);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtr(address, returnAddressRegister);
    }
#endif

#if CPU(SH4)
    ALWAYS_INLINE void preserveReturnAddressAfterCall(RegisterID reg)
    {
        m_assembler.stspr(reg);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(RegisterID reg)
    {
        m_assembler.ldspr(reg);
    }

    ALWAYS_INLINE void restoreReturnAddressBeforeReturn(Address address)
    {
        loadPtrLinkReg(address);
    }
#endif

    void emitGetFromCallFrameHeaderPtr(JSStack::CallFrameHeaderEntry entry, GPRReg to)
    {
        loadPtr(Address(GPRInfo::callFrameRegister, entry * sizeof(Register)), to);
    }
    void emitPutToCallFrameHeader(GPRReg from, JSStack::CallFrameHeaderEntry entry)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }

    void emitPutImmediateToCallFrameHeader(void* value, JSStack::CallFrameHeaderEntry entry)
    {
        storePtr(TrustedImmPtr(value), Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }

    void emitGetCallerFrameFromCallFrameHeaderPtr(RegisterID to)
    {
        loadPtr(Address(GPRInfo::callFrameRegister, CallFrame::callerFrameOffset()), to);
    }
    void emitPutCallerFrameToCallFrameHeader(RegisterID from)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, CallFrame::callerFrameOffset()));
    }

    void emitGetReturnPCFromCallFrameHeaderPtr(RegisterID to)
    {
        loadPtr(Address(GPRInfo::callFrameRegister, CallFrame::returnPCOffset()), to);
    }
    void emitPutReturnPCToCallFrameHeader(RegisterID from)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, CallFrame::returnPCOffset()));
    }
    void emitPutReturnPCToCallFrameHeader(TrustedImmPtr from)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, CallFrame::returnPCOffset()));
    }

    Jump branchIfNotCell(GPRReg reg)
    {
#if USE(JSVALUE64)
        return branchTest64(MacroAssembler::NonZero, reg, GPRInfo::tagMaskRegister);
#else
        return branch32(MacroAssembler::NotEqual, reg, TrustedImm32(JSValue::CellTag));
#endif
    }
    
    static Address addressForByteOffset(ptrdiff_t byteOffset)
    {
        return Address(GPRInfo::callFrameRegister, byteOffset);
    }
    static Address addressFor(VirtualRegister virtualRegister)
    {
        ASSERT(virtualRegister.isValid());
        return Address(GPRInfo::callFrameRegister, virtualRegister.offset() * sizeof(Register));
    }
    static Address addressFor(int operand)
    {
        return addressFor(static_cast<VirtualRegister>(operand));
    }

    static Address tagFor(VirtualRegister virtualRegister)
    {
        ASSERT(virtualRegister.isValid());
        return Address(GPRInfo::callFrameRegister, virtualRegister.offset() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }
    static Address tagFor(int operand)
    {
        return tagFor(static_cast<VirtualRegister>(operand));
    }

    static Address payloadFor(VirtualRegister virtualRegister)
    {
        ASSERT(virtualRegister.isValid());
        return Address(GPRInfo::callFrameRegister, virtualRegister.offset() * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
    static Address payloadFor(int operand)
    {
        return payloadFor(static_cast<VirtualRegister>(operand));
    }

    Jump branchIfNotObject(GPRReg structureReg)
    {
        return branch8(Below, Address(structureReg, Structure::typeInfoTypeOffset()), TrustedImm32(ObjectType));
    }

    static GPRReg selectScratchGPR(GPRReg preserve1 = InvalidGPRReg, GPRReg preserve2 = InvalidGPRReg, GPRReg preserve3 = InvalidGPRReg, GPRReg preserve4 = InvalidGPRReg)
    {
        if (preserve1 != GPRInfo::regT0 && preserve2 != GPRInfo::regT0 && preserve3 != GPRInfo::regT0 && preserve4 != GPRInfo::regT0)
            return GPRInfo::regT0;

        if (preserve1 != GPRInfo::regT1 && preserve2 != GPRInfo::regT1 && preserve3 != GPRInfo::regT1 && preserve4 != GPRInfo::regT1)
            return GPRInfo::regT1;

        if (preserve1 != GPRInfo::regT2 && preserve2 != GPRInfo::regT2 && preserve3 != GPRInfo::regT2 && preserve4 != GPRInfo::regT2)
            return GPRInfo::regT2;

        if (preserve1 != GPRInfo::regT3 && preserve2 != GPRInfo::regT3 && preserve3 != GPRInfo::regT3 && preserve4 != GPRInfo::regT3)
            return GPRInfo::regT3;

        return GPRInfo::regT4;
    }

    // Add a debug call. This call has no effect on JIT code execution state.
    void debugCall(V_DebugOperation_EPP function, void* argument)
    {
        size_t scratchSize = sizeof(EncodedJSValue) * (GPRInfo::numberOfRegisters + FPRInfo::numberOfRegisters);
        ScratchBuffer* scratchBuffer = m_vm->scratchBufferForSize(scratchSize);
        EncodedJSValue* buffer = static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer());

        for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
            store64(GPRInfo::toRegister(i), buffer + i);
#else
            store32(GPRInfo::toRegister(i), buffer + i);
#endif
        }

        for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
            move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
            storeDouble(FPRInfo::toRegister(i), GPRInfo::regT0);
        }

        // Tell GC mark phase how much of the scratch buffer is active during call.
        move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
        storePtr(TrustedImmPtr(scratchSize), GPRInfo::regT0);

#if CPU(X86_64) || CPU(ARM) || CPU(ARM64) || CPU(MIPS) || CPU(SH4)
        move(TrustedImmPtr(buffer), GPRInfo::argumentGPR2);
        move(TrustedImmPtr(argument), GPRInfo::argumentGPR1);
        move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        GPRReg scratch = selectScratchGPR(GPRInfo::argumentGPR0, GPRInfo::argumentGPR1, GPRInfo::argumentGPR2);
#elif CPU(X86)
        poke(GPRInfo::callFrameRegister, 0);
        poke(TrustedImmPtr(argument), 1);
        poke(TrustedImmPtr(buffer), 2);
        GPRReg scratch = GPRInfo::regT0;
#else
#error "JIT not supported on this platform."
#endif
        move(TrustedImmPtr(reinterpret_cast<void*>(function)), scratch);
        call(scratch);

        move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
        storePtr(TrustedImmPtr(0), GPRInfo::regT0);

        for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
            move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
            loadDouble(GPRInfo::regT0, FPRInfo::toRegister(i));
        }
        for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i) {
#if USE(JSVALUE64)
            load64(buffer + i, GPRInfo::toRegister(i));
#else
            load32(buffer + i, GPRInfo::toRegister(i));
#endif
        }
    }

    // These methods JIT generate dynamic, debug-only checks - akin to ASSERTs.
#if !ASSERT_DISABLED
    void jitAssertIsInt32(GPRReg);
    void jitAssertIsJSInt32(GPRReg);
    void jitAssertIsJSNumber(GPRReg);
    void jitAssertIsJSDouble(GPRReg);
    void jitAssertIsCell(GPRReg);
    void jitAssertHasValidCallFrame();
    void jitAssertIsNull(GPRReg);
#else
    void jitAssertIsInt32(GPRReg) { }
    void jitAssertIsJSInt32(GPRReg) { }
    void jitAssertIsJSNumber(GPRReg) { }
    void jitAssertIsJSDouble(GPRReg) { }
    void jitAssertIsCell(GPRReg) { }
    void jitAssertHasValidCallFrame() { }
    void jitAssertIsNull(GPRReg) { }
#endif

    // These methods convert between doubles, and doubles boxed and JSValues.
#if USE(JSVALUE64)
    GPRReg boxDouble(FPRReg fpr, GPRReg gpr)
    {
        moveDoubleTo64(fpr, gpr);
        sub64(GPRInfo::tagTypeNumberRegister, gpr);
        jitAssertIsJSDouble(gpr);
        return gpr;
    }
    FPRReg unboxDouble(GPRReg gpr, FPRReg fpr)
    {
        jitAssertIsJSDouble(gpr);
        add64(GPRInfo::tagTypeNumberRegister, gpr);
        move64ToDouble(gpr, fpr);
        return fpr;
    }
    
    void boxInt52(GPRReg source, GPRReg target, GPRReg scratch, FPRReg fpScratch)
    {
        // Is it an int32?
        signExtend32ToPtr(source, scratch);
        Jump isInt32 = branch64(Equal, source, scratch);
        
        // Nope, it's not, but regT0 contains the int64 value.
        convertInt64ToDouble(source, fpScratch);
        boxDouble(fpScratch, target);
        Jump done = jump();
        
        isInt32.link(this);
        zeroExtend32ToPtr(source, target);
        or64(GPRInfo::tagTypeNumberRegister, target);
        
        done.link(this);
    }
#endif

#if USE(JSVALUE32_64)
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR)
    {
        moveDoubleToInts(fpr, payloadGPR, tagGPR);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, FPRReg scratchFPR)
    {
        moveIntsToDouble(payloadGPR, tagGPR, fpr, scratchFPR);
    }
#endif
    
    enum ExceptionCheckKind { NormalExceptionCheck, InvertedExceptionCheck };
    Jump emitExceptionCheck(ExceptionCheckKind kind = NormalExceptionCheck)
    {
#if USE(JSVALUE64)
        return branchTest64(kind == NormalExceptionCheck ? NonZero : Zero, AbsoluteAddress(vm()->addressOfException()));
#elif USE(JSVALUE32_64)
        return branch32(kind == NormalExceptionCheck ? NotEqual : Equal, AbsoluteAddress(reinterpret_cast<char*>(vm()->addressOfException()) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
#endif
    }

#if ENABLE(SAMPLING_COUNTERS)
    static void emitCount(MacroAssembler& jit, AbstractSamplingCounter& counter, int32_t increment = 1)
    {
        jit.add64(TrustedImm32(increment), AbsoluteAddress(counter.addressOfCounter()));
    }
    void emitCount(AbstractSamplingCounter& counter, int32_t increment = 1)
    {
        add64(TrustedImm32(increment), AbsoluteAddress(counter.addressOfCounter()));
    }
#endif

#if ENABLE(SAMPLING_FLAGS)
    void setSamplingFlag(int32_t);
    void clearSamplingFlag(int32_t flag);
#endif

    JSGlobalObject* globalObjectFor(CodeOrigin codeOrigin)
    {
        return codeBlock()->globalObjectFor(codeOrigin);
    }
    
    bool isStrictModeFor(CodeOrigin codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return codeBlock()->isStrictMode();
        return jsCast<FunctionExecutable*>(codeOrigin.inlineCallFrame->executable.get())->isStrictMode();
    }
    
    ECMAMode ecmaModeFor(CodeOrigin codeOrigin)
    {
        return isStrictModeFor(codeOrigin) ? StrictMode : NotStrictMode;
    }
    
    ExecutableBase* executableFor(const CodeOrigin& codeOrigin);
    
    CodeBlock* baselineCodeBlockFor(const CodeOrigin& codeOrigin)
    {
        return baselineCodeBlockForOriginAndBaselineCodeBlock(codeOrigin, baselineCodeBlock());
    }
    
    CodeBlock* baselineCodeBlockFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return baselineCodeBlock();
        return baselineCodeBlockForInlineCallFrame(inlineCallFrame);
    }
    
    CodeBlock* baselineCodeBlock()
    {
        return m_baselineCodeBlock;
    }
    
    VirtualRegister baselineArgumentsRegisterFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return baselineCodeBlock()->argumentsRegister();
        
        return VirtualRegister(baselineCodeBlockForInlineCallFrame(
            inlineCallFrame)->argumentsRegister().offset() + inlineCallFrame->stackOffset);
    }
    
    VirtualRegister baselineArgumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        return baselineArgumentsRegisterFor(codeOrigin.inlineCallFrame);
    }
    
    SharedSymbolTable* symbolTableFor(const CodeOrigin& codeOrigin)
    {
        return baselineCodeBlockFor(codeOrigin)->symbolTable();
    }

    int offsetOfLocals(const CodeOrigin& codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return 0;
        return codeOrigin.inlineCallFrame->stackOffset * sizeof(Register);
    }

    int offsetOfArgumentsIncludingThis(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return CallFrame::argumentOffsetIncludingThis(0) * sizeof(Register);
        if (inlineCallFrame->arguments.size() <= 1)
            return 0;
        ValueRecovery recovery = inlineCallFrame->arguments[1];
        RELEASE_ASSERT(recovery.technique() == DisplacedInJSStack);
        return (recovery.virtualRegister().offset() - 1) * sizeof(Register);
    }
    
    int offsetOfArgumentsIncludingThis(const CodeOrigin& codeOrigin)
    {
        return offsetOfArgumentsIncludingThis(codeOrigin.inlineCallFrame);
    }

    void writeBarrier(GPRReg owner, GPRReg scratch1, GPRReg scratch2, WriteBarrierUseKind useKind)
    {
        UNUSED_PARAM(owner);
        UNUSED_PARAM(scratch1);
        UNUSED_PARAM(scratch2);
        UNUSED_PARAM(useKind);
        ASSERT(owner != scratch1);
        ASSERT(owner != scratch2);
        ASSERT(scratch1 != scratch2);
        
#if ENABLE(WRITE_BARRIER_PROFILING)
        emitCount(WriteBarrierCounters::jitCounterFor(useKind));
#endif
    }

    Vector<BytecodeAndMachineOffset>& decodedCodeMapFor(CodeBlock*);
    
protected:
    VM* m_vm;
    CodeBlock* m_codeBlock;
    CodeBlock* m_baselineCodeBlock;

    HashMap<CodeBlock*, Vector<BytecodeAndMachineOffset>> m_decodedCodeMaps;
};

} // namespace JSC

#endif // ENABLE(JIT)

#endif // AssemblyHelpers_h

