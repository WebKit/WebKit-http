/*
 * Copyright (C) 2008, 2012 Apple Inc. All rights reserved.
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

#ifndef MacroAssemblerX86_64_h
#define MacroAssemblerX86_64_h

#if ENABLE(ASSEMBLER) && CPU(X86_64)

#include "MacroAssemblerX86Common.h"

#define REPTACH_OFFSET_CALL_R11 3

namespace JSC {

class MacroAssemblerX86_64 : public MacroAssemblerX86Common {
public:
    static const Scale ScalePtr = TimesEight;

    using MacroAssemblerX86Common::add32;
    using MacroAssemblerX86Common::and32;
    using MacroAssemblerX86Common::branchAdd32;
    using MacroAssemblerX86Common::or32;
    using MacroAssemblerX86Common::sub32;
    using MacroAssemblerX86Common::load32;
    using MacroAssemblerX86Common::store32;
    using MacroAssemblerX86Common::store8;
    using MacroAssemblerX86Common::call;
    using MacroAssemblerX86Common::jump;
    using MacroAssemblerX86Common::addDouble;
    using MacroAssemblerX86Common::loadDouble;
    using MacroAssemblerX86Common::convertInt32ToDouble;

    void add32(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        add32(imm, Address(scratchRegister));
    }
    
    void and32(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        and32(imm, Address(scratchRegister));
    }
    
    void add32(AbsoluteAddress address, RegisterID dest)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        add32(Address(scratchRegister), dest);
    }
    
    void or32(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        or32(imm, Address(scratchRegister));
    }

    void or32(RegisterID reg, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        or32(reg, Address(scratchRegister));
    }

    void sub32(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        sub32(imm, Address(scratchRegister));
    }

    void load32(const void* address, RegisterID dest)
    {
        if (dest == X86Registers::eax)
            m_assembler.movl_mEAX(address);
        else {
            move(TrustedImmPtr(address), dest);
            load32(dest, dest);
        }
    }

    void addDouble(AbsoluteAddress address, FPRegisterID dest)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        m_assembler.addsd_mr(0, scratchRegister, dest);
    }

    void convertInt32ToDouble(TrustedImm32 imm, FPRegisterID dest)
    {
        move(imm, scratchRegister);
        m_assembler.cvtsi2sd_rr(scratchRegister, dest);
    }

    void store32(TrustedImm32 imm, void* address)
    {
        move(TrustedImmPtr(address), scratchRegister);
        store32(imm, scratchRegister);
    }
    
    void store8(TrustedImm32 imm, void* address)
    {
        move(TrustedImmPtr(address), scratchRegister);
        store8(imm, Address(scratchRegister));
    }

    Call call()
    {
        DataLabelPtr label = moveWithPatch(TrustedImmPtr(0), scratchRegister);
        Call result = Call(m_assembler.call(scratchRegister), Call::Linkable);
        ASSERT_UNUSED(label, differenceBetween(label, result) == REPTACH_OFFSET_CALL_R11);
        return result;
    }

    // Address is a memory location containing the address to jump to
    void jump(AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        jump(Address(scratchRegister));
    }

    Call tailRecursiveCall()
    {
        DataLabelPtr label = moveWithPatch(TrustedImmPtr(0), scratchRegister);
        Jump newJump = Jump(m_assembler.jmp_r(scratchRegister));
        ASSERT_UNUSED(label, differenceBetween(label, newJump) == REPTACH_OFFSET_CALL_R11);
        return Call::fromTailJump(newJump);
    }

    Call makeTailRecursiveCall(Jump oldJump)
    {
        oldJump.link(this);
        DataLabelPtr label = moveWithPatch(TrustedImmPtr(0), scratchRegister);
        Jump newJump = Jump(m_assembler.jmp_r(scratchRegister));
        ASSERT_UNUSED(label, differenceBetween(label, newJump) == REPTACH_OFFSET_CALL_R11);
        return Call::fromTailJump(newJump);
    }


    void addPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.addq_rr(src, dest);
    }
    
    void addPtr(Address src, RegisterID dest)
    {
        m_assembler.addq_mr(src.offset, src.base, dest);
    }

    void addPtr(AbsoluteAddress src, RegisterID dest)
    {
        move(TrustedImmPtr(src.m_ptr), scratchRegister);
        addPtr(Address(scratchRegister), dest);
    }

    void addPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        m_assembler.addq_ir(imm.m_value, srcDest);
    }

    void addPtr(TrustedImmPtr imm, RegisterID dest)
    {
        move(imm, scratchRegister);
        m_assembler.addq_rr(scratchRegister, dest);
    }

    void addPtr(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        m_assembler.leaq_mr(imm.m_value, src, dest);
    }

    void addPtr(TrustedImm32 imm, Address address)
    {
        m_assembler.addq_im(imm.m_value, address.offset, address.base);
    }

    void addPtr(TrustedImm32 imm, AbsoluteAddress address)
    {
        move(TrustedImmPtr(address.m_ptr), scratchRegister);
        addPtr(imm, Address(scratchRegister));
    }

    void add64(TrustedImm32 imm, AbsoluteAddress address)
    {
        addPtr(imm, address);
    }

    void andPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.andq_rr(src, dest);
    }

    void andPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        m_assembler.andq_ir(imm.m_value, srcDest);
    }
    
    void negPtr(RegisterID dest)
    {
        m_assembler.negq_r(dest);
    }

    void orPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.orq_rr(src, dest);
    }

    void orPtr(TrustedImmPtr imm, RegisterID dest)
    {
        move(imm, scratchRegister);
        m_assembler.orq_rr(scratchRegister, dest);
    }

    void orPtr(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.orq_ir(imm.m_value, dest);
    }

    void orPtr(RegisterID op1, RegisterID op2, RegisterID dest)
    {
        if (op1 == op2)
            move(op1, dest);
        else if (op1 == dest)
            orPtr(op2, dest);
        else {
            move(op2, dest);
            orPtr(op1, dest);
        }
    }

    void orPtr(TrustedImm32 imm, RegisterID src, RegisterID dest)
    {
        move(src, dest);
        orPtr(imm, dest);
    }
    
    void rotateRightPtr(TrustedImm32 imm, RegisterID srcDst)
    {
        m_assembler.rorq_i8r(imm.m_value, srcDst);
    }

    void subPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.subq_rr(src, dest);
    }
    
    void subPtr(TrustedImm32 imm, RegisterID dest)
    {
        m_assembler.subq_ir(imm.m_value, dest);
    }
    
    void subPtr(TrustedImmPtr imm, RegisterID dest)
    {
        move(imm, scratchRegister);
        m_assembler.subq_rr(scratchRegister, dest);
    }

    void xorPtr(RegisterID src, RegisterID dest)
    {
        m_assembler.xorq_rr(src, dest);
    }
    
    void xorPtr(RegisterID src, Address dest)
    {
        m_assembler.xorq_rm(src, dest.offset, dest.base);
    }

    void xorPtr(TrustedImm32 imm, RegisterID srcDest)
    {
        m_assembler.xorq_ir(imm.m_value, srcDest);
    }

    void loadPtr(ImplicitAddress address, RegisterID dest)
    {
        m_assembler.movq_mr(address.offset, address.base, dest);
    }

    ConvertibleLoadLabel convertibleLoadPtr(Address address, RegisterID dest)
    {
        ConvertibleLoadLabel result = ConvertibleLoadLabel(this);
        m_assembler.movq_mr(address.offset, address.base, dest);
        return result;
    }

    void loadPtr(BaseIndex address, RegisterID dest)
    {
        m_assembler.movq_mr(address.offset, address.base, address.index, address.scale, dest);
    }

    void loadPtr(const void* address, RegisterID dest)
    {
        if (dest == X86Registers::eax)
            m_assembler.movq_mEAX(address);
        else {
            move(TrustedImmPtr(address), dest);
            loadPtr(dest, dest);
        }
    }

    DataLabel32 loadPtrWithAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movq_mr_disp32(address.offset, address.base, dest);
        return DataLabel32(this);
    }
    
    DataLabelCompact loadPtrWithCompactAddressOffsetPatch(Address address, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movq_mr_disp8(address.offset, address.base, dest);
        return DataLabelCompact(this);
    }

    void storePtr(RegisterID src, ImplicitAddress address)
    {
        m_assembler.movq_rm(src, address.offset, address.base);
    }

    void storePtr(RegisterID src, BaseIndex address)
    {
        m_assembler.movq_rm(src, address.offset, address.base, address.index, address.scale);
    }
    
    void storePtr(RegisterID src, void* address)
    {
        if (src == X86Registers::eax)
            m_assembler.movq_EAXm(address);
        else {
            move(TrustedImmPtr(address), scratchRegister);
            storePtr(src, scratchRegister);
        }
    }

    void storePtr(TrustedImmPtr imm, ImplicitAddress address)
    {
        move(imm, scratchRegister);
        storePtr(scratchRegister, address);
    }

    void storePtr(TrustedImmPtr imm, BaseIndex address)
    {
        move(imm, scratchRegister);
        m_assembler.movq_rm(scratchRegister, address.offset, address.base, address.index, address.scale);
    }
    
    DataLabel32 storePtrWithAddressOffsetPatch(RegisterID src, Address address)
    {
        padBeforePatch();
        m_assembler.movq_rm_disp32(src, address.offset, address.base);
        return DataLabel32(this);
    }

    void movePtrToDouble(RegisterID src, FPRegisterID dest)
    {
        m_assembler.movq_rr(src, dest);
    }

    void moveDoubleToPtr(FPRegisterID src, RegisterID dest)
    {
        m_assembler.movq_rr(src, dest);
    }

    void comparePtr(RelationalCondition cond, RegisterID left, TrustedImm32 right, RegisterID dest)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value)
            m_assembler.testq_rr(left, left);
        else
            m_assembler.cmpq_ir(right.m_value, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }
    
    void comparePtr(RelationalCondition cond, RegisterID left, RegisterID right, RegisterID dest)
    {
        m_assembler.cmpq_rr(right, left);
        m_assembler.setCC_r(x86Condition(cond), dest);
        m_assembler.movzbl_rr(dest, dest);
    }
    
    Jump branchAdd32(ResultCondition cond, TrustedImm32 src, AbsoluteAddress dest)
    {
        move(TrustedImmPtr(dest.m_ptr), scratchRegister);
        add32(src, Address(scratchRegister));
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchPtr(RelationalCondition cond, RegisterID left, RegisterID right)
    {
        m_assembler.cmpq_rr(right, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchPtr(RelationalCondition cond, RegisterID left, TrustedImmPtr right)
    {
        if (((cond == Equal) || (cond == NotEqual)) && !right.m_value) {
            m_assembler.testq_rr(left, left);
            return Jump(m_assembler.jCC(x86Condition(cond)));
        }
        move(right, scratchRegister);
        return branchPtr(cond, left, scratchRegister);
    }

    Jump branchPtr(RelationalCondition cond, RegisterID left, Address right)
    {
        m_assembler.cmpq_mr(right.offset, right.base, left);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchPtr(RelationalCondition cond, AbsoluteAddress left, RegisterID right)
    {
        move(TrustedImmPtr(left.m_ptr), scratchRegister);
        return branchPtr(cond, Address(scratchRegister), right);
    }

    Jump branchPtr(RelationalCondition cond, Address left, RegisterID right)
    {
        m_assembler.cmpq_rm(right, left.offset, left.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchPtr(RelationalCondition cond, Address left, TrustedImmPtr right)
    {
        move(right, scratchRegister);
        return branchPtr(cond, left, scratchRegister);
    }

    Jump branchTestPtr(ResultCondition cond, RegisterID reg, RegisterID mask)
    {
        m_assembler.testq_rr(reg, mask);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }
    
    Jump branchTestPtr(ResultCondition cond, RegisterID reg, TrustedImm32 mask = TrustedImm32(-1))
    {
        // if we are only interested in the low seven bits, this can be tested with a testb
        if (mask.m_value == -1)
            m_assembler.testq_rr(reg, reg);
        else if ((mask.m_value & ~0x7f) == 0)
            m_assembler.testb_i8r(mask.m_value, reg);
        else
            m_assembler.testq_i32r(mask.m_value, reg);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    void testPtr(ResultCondition cond, RegisterID reg, TrustedImm32 mask, RegisterID dest)
    {
        if (mask.m_value == -1)
            m_assembler.testq_rr(reg, reg);
        else if ((mask.m_value & ~0x7f) == 0)
            m_assembler.testb_i8r(mask.m_value, reg);
        else
            m_assembler.testq_i32r(mask.m_value, reg);
        set32(x86Condition(cond), dest);
    }

    void testPtr(ResultCondition cond, RegisterID reg, RegisterID mask, RegisterID dest)
    {
        m_assembler.testq_rr(reg, mask);
        set32(x86Condition(cond), dest);
    }

    Jump branchTestPtr(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        loadPtr(address.m_ptr, scratchRegister);
        return branchTestPtr(cond, scratchRegister, mask);
    }

    Jump branchTestPtr(ResultCondition cond, Address address, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1)
            m_assembler.cmpq_im(0, address.offset, address.base);
        else
            m_assembler.testq_i32m(mask.m_value, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTestPtr(ResultCondition cond, Address address, RegisterID reg)
    {
        m_assembler.testq_rm(reg, address.offset, address.base);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchTestPtr(ResultCondition cond, BaseIndex address, TrustedImm32 mask = TrustedImm32(-1))
    {
        if (mask.m_value == -1)
            m_assembler.cmpq_im(0, address.offset, address.base, address.index, address.scale);
        else
            m_assembler.testq_i32m(mask.m_value, address.offset, address.base, address.index, address.scale);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }


    Jump branchAddPtr(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        addPtr(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchAddPtr(ResultCondition cond, RegisterID src, RegisterID dest)
    {
        addPtr(src, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSubPtr(ResultCondition cond, TrustedImm32 imm, RegisterID dest)
    {
        subPtr(imm, dest);
        return Jump(m_assembler.jCC(x86Condition(cond)));
    }

    Jump branchSubPtr(ResultCondition cond, RegisterID src1, TrustedImm32 src2, RegisterID dest)
    {
        move(src1, dest);
        return branchSubPtr(cond, src2, dest);
    }

    DataLabelPtr moveWithPatch(TrustedImmPtr initialValue, RegisterID dest)
    {
        padBeforePatch();
        m_assembler.movq_i64r(initialValue.asIntptr(), dest);
        return DataLabelPtr(this);
    }

    Jump branchPtrWithPatch(RelationalCondition cond, RegisterID left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, scratchRegister);
        return branchPtr(cond, left, scratchRegister);
    }

    Jump branchPtrWithPatch(RelationalCondition cond, Address left, DataLabelPtr& dataLabel, TrustedImmPtr initialRightValue = TrustedImmPtr(0))
    {
        dataLabel = moveWithPatch(initialRightValue, scratchRegister);
        return branchPtr(cond, left, scratchRegister);
    }

    DataLabelPtr storePtrWithPatch(TrustedImmPtr initialValue, ImplicitAddress address)
    {
        DataLabelPtr label = moveWithPatch(initialValue, scratchRegister);
        storePtr(scratchRegister, address);
        return label;
    }

    using MacroAssemblerX86Common::branchTest8;
    Jump branchTest8(ResultCondition cond, ExtendedAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        TrustedImmPtr addr(reinterpret_cast<void*>(address.offset));
        MacroAssemblerX86Common::move(addr, scratchRegister);
        return MacroAssemblerX86Common::branchTest8(cond, BaseIndex(scratchRegister, address.base, TimesOne), mask);
    }
    
    Jump branchTest8(ResultCondition cond, AbsoluteAddress address, TrustedImm32 mask = TrustedImm32(-1))
    {
        MacroAssemblerX86Common::move(TrustedImmPtr(address.m_ptr), scratchRegister);
        return MacroAssemblerX86Common::branchTest8(cond, Address(scratchRegister), mask);
    }

    static bool supportsFloatingPoint() { return true; }
    // See comment on MacroAssemblerARMv7::supportsFloatingPointTruncate()
    static bool supportsFloatingPointTruncate() { return true; }
    static bool supportsFloatingPointSqrt() { return true; }
    static bool supportsFloatingPointAbs() { return true; }
    
    static FunctionPtr readCallTarget(CodeLocationCall call)
    {
        return FunctionPtr(X86Assembler::readPointer(call.dataLabelPtrAtOffset(-REPTACH_OFFSET_CALL_R11).dataLocation()));
    }

    static RegisterID scratchRegisterForBlinding() { return scratchRegister; }

private:
    friend class LinkBuffer;
    friend class RepatchBuffer;

    static void linkCall(void* code, Call call, FunctionPtr function)
    {
        if (!call.isFlagSet(Call::Near))
            X86Assembler::linkPointer(code, call.m_label.labelAtOffset(-REPTACH_OFFSET_CALL_R11), function.value());
        else
            X86Assembler::linkCall(code, call.m_label, function.value());
    }

    static void repatchCall(CodeLocationCall call, CodeLocationLabel destination)
    {
        X86Assembler::repatchPointer(call.dataLabelPtrAtOffset(-REPTACH_OFFSET_CALL_R11).dataLocation(), destination.executableAddress());
    }

    static void repatchCall(CodeLocationCall call, FunctionPtr destination)
    {
        X86Assembler::repatchPointer(call.dataLabelPtrAtOffset(-REPTACH_OFFSET_CALL_R11).dataLocation(), destination.executableAddress());
    }

};

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

#endif // MacroAssemblerX86_64_h
