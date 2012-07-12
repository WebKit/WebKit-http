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

#ifndef DFGAssemblyHelpers_h
#define DFGAssemblyHelpers_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGFPRInfo.h"
#include "DFGGPRInfo.h"
#include "DFGNode.h"
#include "JSGlobalData.h"
#include "MacroAssembler.h"

namespace JSC { namespace DFG {

typedef void (*V_DFGDebugOperation_EP)(ExecState*, void*);

class AssemblyHelpers : public MacroAssembler {
public:
    AssemblyHelpers(JSGlobalData* globalData, CodeBlock* codeBlock)
        : m_globalData(globalData)
        , m_codeBlock(codeBlock)
        , m_baselineCodeBlock(codeBlock ? codeBlock->baselineVersion() : 0)
    {
        if (m_codeBlock) {
            ASSERT(m_baselineCodeBlock);
            ASSERT(!m_baselineCodeBlock->alternative());
            ASSERT(m_baselineCodeBlock->getJITType() == JITCode::BaselineJIT);
        }
    }
    
    CodeBlock* codeBlock() { return m_codeBlock; }
    JSGlobalData* globalData() { return m_globalData; }
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

#if CPU(ARM)
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

    void emitGetFromCallFrameHeaderPtr(RegisterFile::CallFrameHeaderEntry entry, GPRReg to)
    {
        loadPtr(Address(GPRInfo::callFrameRegister, entry * sizeof(Register)), to);
    }
    void emitPutToCallFrameHeader(GPRReg from, RegisterFile::CallFrameHeaderEntry entry)
    {
        storePtr(from, Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }

    void emitPutImmediateToCallFrameHeader(void* value, RegisterFile::CallFrameHeaderEntry entry)
    {
        storePtr(TrustedImmPtr(value), Address(GPRInfo::callFrameRegister, entry * sizeof(Register)));
    }

    Jump branchIfNotCell(GPRReg reg)
    {
#if USE(JSVALUE64)
        return branchTestPtr(MacroAssembler::NonZero, reg, GPRInfo::tagMaskRegister);
#else
        return branch32(MacroAssembler::NotEqual, reg, TrustedImm32(JSValue::CellTag));
#endif
    }
    
    static Address addressFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register));
    }
    static Address addressFor(int operand)
    {
        return addressFor(static_cast<VirtualRegister>(operand));
    }

    static Address tagFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }
    static Address tagFor(int operand)
    {
        return tagFor(static_cast<VirtualRegister>(operand));
    }

    static Address payloadFor(VirtualRegister virtualRegister)
    {
        return Address(GPRInfo::callFrameRegister, virtualRegister * sizeof(Register) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
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
    void debugCall(V_DFGDebugOperation_EP function, void* argument)
    {
        size_t scratchSize = sizeof(EncodedJSValue) * (GPRInfo::numberOfRegisters + FPRInfo::numberOfRegisters);
        ScratchBuffer* scratchBuffer = m_globalData->scratchBufferForSize(scratchSize);
        EncodedJSValue* buffer = static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer());

        for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i)
            storePtr(GPRInfo::toRegister(i), buffer + i);
        for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
            move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
            storeDouble(FPRInfo::toRegister(i), GPRInfo::regT0);
        }

        // Tell GC mark phase how much of the scratch buffer is active during call.
        move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
        storePtr(TrustedImmPtr(scratchSize), GPRInfo::regT0);

#if CPU(X86_64) || CPU(ARM)
        move(TrustedImmPtr(argument), GPRInfo::argumentGPR1);
        move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);
        GPRReg scratch = selectScratchGPR(GPRInfo::argumentGPR0, GPRInfo::argumentGPR1);
#elif CPU(X86)
        poke(GPRInfo::callFrameRegister, 0);
        poke(TrustedImmPtr(argument), 1);
        GPRReg scratch = GPRInfo::regT0;
#else
#error "DFG JIT not supported on this platform."
#endif
        move(TrustedImmPtr(reinterpret_cast<void*>(function)), scratch);
        call(scratch);

        move(TrustedImmPtr(scratchBuffer->activeLengthPtr()), GPRInfo::regT0);
        storePtr(TrustedImmPtr(0), GPRInfo::regT0);

        for (unsigned i = 0; i < FPRInfo::numberOfRegisters; ++i) {
            move(TrustedImmPtr(buffer + GPRInfo::numberOfRegisters + i), GPRInfo::regT0);
            loadDouble(GPRInfo::regT0, FPRInfo::toRegister(i));
        }
        for (unsigned i = 0; i < GPRInfo::numberOfRegisters; ++i)
            loadPtr(buffer + i, GPRInfo::toRegister(i));
    }

    // These methods JIT generate dynamic, debug-only checks - akin to ASSERTs.
#if DFG_ENABLE(JIT_ASSERT)
    void jitAssertIsInt32(GPRReg);
    void jitAssertIsJSInt32(GPRReg);
    void jitAssertIsJSNumber(GPRReg);
    void jitAssertIsJSDouble(GPRReg);
    void jitAssertIsCell(GPRReg);
    void jitAssertHasValidCallFrame();
#else
    void jitAssertIsInt32(GPRReg) { }
    void jitAssertIsJSInt32(GPRReg) { }
    void jitAssertIsJSNumber(GPRReg) { }
    void jitAssertIsJSDouble(GPRReg) { }
    void jitAssertIsCell(GPRReg) { }
    void jitAssertHasValidCallFrame() { }
#endif

    // These methods convert between doubles, and doubles boxed and JSValues.
#if USE(JSVALUE64)
    GPRReg boxDouble(FPRReg fpr, GPRReg gpr)
    {
        moveDoubleToPtr(fpr, gpr);
        subPtr(GPRInfo::tagTypeNumberRegister, gpr);
        jitAssertIsJSDouble(gpr);
        return gpr;
    }
    FPRReg unboxDouble(GPRReg gpr, FPRReg fpr)
    {
        jitAssertIsJSDouble(gpr);
        addPtr(GPRInfo::tagTypeNumberRegister, gpr);
        movePtrToDouble(gpr, fpr);
        return fpr;
    }
#endif

#if USE(JSVALUE32_64) && CPU(X86)
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR)
    {
        movePackedToInt32(fpr, payloadGPR);
        rshiftPacked(TrustedImm32(32), fpr);
        movePackedToInt32(fpr, tagGPR);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, FPRReg scratchFPR)
    {
        jitAssertIsJSDouble(tagGPR);
        moveInt32ToPacked(payloadGPR, fpr);
        moveInt32ToPacked(tagGPR, scratchFPR);
        lshiftPacked(TrustedImm32(32), scratchFPR);
        orPacked(scratchFPR, fpr);
    }
#endif

#if USE(JSVALUE32_64) && CPU(ARM)
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR)
    {
        m_assembler.vmov(payloadGPR, tagGPR, fpr);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, FPRReg scratchFPR)
    {
        jitAssertIsJSDouble(tagGPR);
        UNUSED_PARAM(scratchFPR);
        m_assembler.vmov(fpr, payloadGPR, tagGPR);
    }
#endif
    
    enum ExceptionCheckKind { NormalExceptionCheck, InvertedExceptionCheck };
    Jump emitExceptionCheck(ExceptionCheckKind kind = NormalExceptionCheck)
    {
#if USE(JSVALUE64)
        return branchTestPtr(kind == NormalExceptionCheck ? NonZero : Zero, AbsoluteAddress(&globalData()->exception));
#elif USE(JSVALUE32_64)
        return branch32(kind == NormalExceptionCheck ? NotEqual : Equal, AbsoluteAddress(reinterpret_cast<char*>(&globalData()->exception) + OBJECT_OFFSETOF(JSValue, u.asBits.tag)), TrustedImm32(JSValue::EmptyValueTag));
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
    
    JSObject* globalThisObjectFor(CodeOrigin codeOrigin)
    {
        JSGlobalObject* object = globalObjectFor(codeOrigin);
        return object->methodTable()->toThisObject(object, 0);
    }
    
    bool strictModeFor(CodeOrigin codeOrigin)
    {
        if (!codeOrigin.inlineCallFrame)
            return codeBlock()->isStrictMode();
        return codeOrigin.inlineCallFrame->callee->jsExecutable()->isStrictMode();
    }
    
    ExecutableBase* executableFor(const CodeOrigin& codeOrigin);
    
    CodeBlock* baselineCodeBlockFor(const CodeOrigin& codeOrigin)
    {
        return baselineCodeBlockForOriginAndBaselineCodeBlock(codeOrigin, baselineCodeBlock());
    }
    
    CodeBlock* baselineCodeBlock()
    {
        return m_baselineCodeBlock;
    }
    
    int argumentsRegisterFor(InlineCallFrame* inlineCallFrame)
    {
        if (!inlineCallFrame)
            return codeBlock()->argumentsRegister();
        
        return baselineCodeBlockForInlineCallFrame(
            inlineCallFrame)->argumentsRegister() + inlineCallFrame->stackOffset;
    }
    
    int argumentsRegisterFor(const CodeOrigin& codeOrigin)
    {
        return argumentsRegisterFor(codeOrigin.inlineCallFrame);
    }
    
    Vector<BytecodeAndMachineOffset>& decodedCodeMapFor(CodeBlock*);
    
    static const double twoToThe32;

protected:
    JSGlobalData* m_globalData;
    CodeBlock* m_codeBlock;
    CodeBlock* m_baselineCodeBlock;

    HashMap<CodeBlock*, Vector<BytecodeAndMachineOffset> > m_decodedCodeMaps;
};

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGAssemblyHelpers_h

