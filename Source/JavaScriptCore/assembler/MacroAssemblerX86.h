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

#ifndef MacroAssemblerX86_h
#define MacroAssemblerX86_h

#if ENABLE(ASSEMBLER) && CPU(X86)

#include "MacroAssemblerX86Common.h"

namespace JSC {

class MacroAssemblerX86 : public MacroAssemblerX86Common {
public:
    static const Scale ScalePtr = TimesFour;

    using MacroAssemblerX86Common::add32;
    using MacroAssemblerX86Common::and32;
    using MacroAssemblerX86Common::branchAdd32;
    using MacroAssemblerX86Common::branchSub32;
    using MacroAssemblerX86Common::sub32;
    using MacroAssemblerX86Common::or32;
    using MacroAssemblerX86Common::load32;
    using MacroAssemblerX86Common::store32;
    using MacroAssemblerX86Common::branch32;
    using MacroAssemblerX86Common::call;
    using MacroAssemblerX86Common::jump;
    using MacroAssemblerX86Common::addDouble;
    using MacroAssemblerX86Common::loadDouble;
    using MacroAssemblerX86Common::storeDouble;
    using MacroAssemblerX86Common::convertInt32ToDouble;

    void add32(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.leal_mr(imm.m_value, src, dest);
    }

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.addl_im(imm.m_value, address.m_ptr);
    }
    
    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.addl_im(imm.m_value, address.m_ptr);
        m_assembler.adcl_im(imm.m_value >> 31, reinterpret_cast<const char*>(address.m_ptr) + sizeof(int32_t));
    }

    void and32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.andl_im(imm.m_value, address.m_ptr);
    }
    
    void or32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.orl_im(imm.m_value, address.m_ptr);
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        m_assembler.subl_im(imm.m_value, address.m_ptr);
    }

    void load32(const void* address, RegisterID dest)
    {
        m_assembler.movl_mr(address, dest);
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        m_assembler.addsd_mr(address.m_ptr, dest);
    }

    void loadDouble(const void* address, FPRegisterID dest)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_mr(address, dest);
    }

    void storeDouble(FPRegisterID src, const void* address)
    {
        ASSERT(isSSE2Present());
        m_assembler.movsd_rm(src, address);
    }

    void convertInt32ToDouble(AbsoluteAddress src, FPRegisterID dest)
    {
        m_assembler.cvtsi2sd_mr(src.m_ptr, dest);
    }

    void absDouble(FPRegisterID src, FPRegisterID dst)
    {
        ASSERT(src != dst);
        static const double negativeZeroConstant = -0.0;
        loadDouble(&negativeZeroConstant, dst);
        m_assembler.andnpd_rr(src, dst);
    }

    void store32(TrustedImm32 imm, void* address)
    {
        m_assembler.movl_i32m(imm.m_value, address);
    }

    void store32(RegisterID src, void* address)
    {
        m_assembler.movl_rm(src, address);
    }

    Jump branchAdd32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        m_assembler.addl_im(imm.m_value, dest.m_ptr);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSub32(ResultCondition cond, TrustedImm32 imm, AbsoluteAddress dest)
    {
        m_assembler.subl_im(imm.m_value, dest.m_ptr);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        m_assembler.cmpl_rm(right, left.m_ptr);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branch32(RelationalCondition cond, AbsoluteAddress left, TrustedImm32 right)
    {
        m_assembler.cmpl_im(right.m_value, left.m_ptr);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Call call()
    {
        return Call(m_assembler.call(), Call::Linkable);
    }

    // Address is a memory location containing the address to jump to
    void jump(AbsoluteAddress address)
    {
        m_assembler.jmp_m(address.m_ptr);
    }

    Call tailRecursiveCall()
    {
        return Call::fromTailJump(jump());
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        return Call::fromTailJump(oldJump);
    }


    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        m_assembler.movl_i32r(initialValue.asIntptr(), dest);
        return DataLabelPtr(this);
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_assembler.cmpl_ir_force32(initialRightValue.asIntptr(), left);
        dataLabel = DataLabelPtr(this);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        m_assembler.cmpl_im_force32(initialRightValue.asIntptr(), left.offset, left.base);
        dataLabel = DataLabelPtr(this);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        m_assembler.movl_i32m(initialValue.asIntptr(), address.offset, address.base);
        return DataLabelPtr(this);
    }

    static bool supportsFloatingPoint() { return isSSE2Present(); }
    // See comment on MacroAssemblerARMv7::supportsFloatingPointTruncate()
    static bool supportsFloatingPointTruncate() { return isSSE2Present(); }
    static bool supportsFloatingPointSqrt() { return isSSE2Present(); }
    static bool supportsFloatingPointAbs() { return isSSE2Present(); }
    
    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        intptr_t offset = reinterpret_cast<int32_t*>(call.dataLocation())[-1];
        return FunctionPtr(reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(call.dataLocation()) + offset));
    }

private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        X86Assembler::linkCall(code, call.m_label, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        X86Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        X86Assembler::relinkCall(call.dataLocation(), destination.executableAddress());
    }
};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerX86_h
