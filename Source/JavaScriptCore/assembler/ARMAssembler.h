/*
 * Copyright (C) 2009, 2010 University of Szeged
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY UNIVERSITY OF SZEGED ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL UNIVERSITY OF SZEGED OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ARMAssembler_h
#define ARMAssembler_h

#if ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#include "AssemblerBufferWithConstantPool.h"
#include "JITCompilationEffort.h"
#include <wtf/Assertions.h>
namespace JSC {

    typedef uint32_t ARMWord;

    namespace ARMRegisters {
        typedef enum {
            r0 = 0,
            r1,
            r2,
            r3, S0 = r3, /* Same as thumb assembler. */
            r4,
            r5,
            r6,
            r7,
            r8,
            r9,
            r10,
            r11,
            r12, S1 = r12,
            r13, sp = r13,
            r14, lr = r14,
            r15, pc = r15
        } RegisterID;

        typedef enum {
            d0,
            d1,
            d2,
            d3,
            d4,
            d5,
            d6,
            d7, SD0 = d7, /* Same as thumb assembler. */
            d8,
            d9,
            d10,
            d11,
            d12,
            d13,
            d14,
            d15,
            d16,
            d17,
            d18,
            d19,
            d20,
            d21,
            d22,
            d23,
            d24,
            d25,
            d26,
            d27,
            d28,
            d29,
            d30,
            d31
        } FPRegisterID;

    } // namespace ARMRegisters

    class ARMAssembler {
    public:
        typedef ARMRegisters::RegisterID RegisterID;
        typedef ARMRegisters::FPRegisterID FPRegisterID;
        typedef AssemblerBufferWithConstantPool<2048, 4, 4, ARMAssembler> ARMBuffer;
        typedef SegmentedVector<AssemblerLabel, 64> Jumps;

        ARMAssembler()
            : m_indexOfTailOfLastWatchpoint(1)
        {
        }

        // ARM conditional constants
        typedef enum {
            EQ = 0x00000000, // Zero
            NE = 0x10000000, // Non-zero
            CS = 0x20000000,
            CC = 0x30000000,
            MI = 0x40000000,
            PL = 0x50000000,
            VS = 0x60000000,
            VC = 0x70000000,
            HI = 0x80000000,
            LS = 0x90000000,
            GE = 0xa0000000,
            LT = 0xb0000000,
            GT = 0xc0000000,
            LE = 0xd0000000,
            AL = 0xe0000000
        } Condition;

        // ARM instruction constants
        enum {
            AND = (0x0 << 21),
            EOR = (0x1 << 21),
            SUB = (0x2 << 21),
            RSB = (0x3 << 21),
            ADD = (0x4 << 21),
            ADC = (0x5 << 21),
            SBC = (0x6 << 21),
            RSC = (0x7 << 21),
            TST = (0x8 << 21),
            TEQ = (0x9 << 21),
            CMP = (0xa << 21),
            CMN = (0xb << 21),
            ORR = (0xc << 21),
            MOV = (0xd << 21),
            BIC = (0xe << 21),
            MVN = (0xf << 21),
            MUL = 0x00000090,
            MULL = 0x00c00090,
            VMOV_F64 = 0x0eb00b40,
            VADD_F64 = 0x0e300b00,
            VDIV_F64 = 0x0e800b00,
            VSUB_F64 = 0x0e300b40,
            VMUL_F64 = 0x0e200b00,
            VCMP_F64 = 0x0eb40b40,
            VSQRT_F64 = 0x0eb10bc0,
            VABS_F64 = 0x0eb00bc0,
            VNEG_F64 = 0x0eb10b40,
            STMDB = 0x09200000,
            LDMIA = 0x08b00000,
            B = 0x0a000000,
            BL = 0x0b000000,
            BX = 0x012fff10,
            VMOV_VFP64 = 0x0c400a10,
            VMOV_ARM64 = 0x0c500a10,
            VMOV_VFP32 = 0x0e000a10,
            VMOV_ARM32 = 0x0e100a10,
            VCVT_F64_S32 = 0x0eb80bc0,
            VCVT_S32_F64 = 0x0ebd0b40,
            VCVT_U32_F64 = 0x0ebc0b40,
            VCVT_F32_F64 = 0x0eb70bc0,
            VCVT_F64_F32 = 0x0eb70ac0,
            VMRS_APSR = 0x0ef1fa10,
            CLZ = 0x016f0f10,
            BKPT = 0xe1200070,
            BLX = 0x012fff30,
#if WTF_ARM_ARCH_AT_LEAST(7)
            MOVW = 0x03000000,
            MOVT = 0x03400000,
#endif
            NOP = 0xe1a00000,
        };

        enum {
            OP2_IMM = (1 << 25),
            OP2_IMM_HALF = (1 << 22),
            OP2_INV_IMM = (1 << 26),
            SET_CC = (1 << 20),
            OP2_OFSREG = (1 << 25),
            // Data transfer flags.
            DT_UP = (1 << 23),
            DT_WB = (1 << 21),
            DT_PRE = (1 << 24),
            DT_LOAD = (1 << 20),
            DT_BYTE = (1 << 22),
        };

        enum DataTransferTypeA {
            LoadUint32 = 0x05000000 | DT_LOAD,
            LoadUint8 = 0x05400000 | DT_LOAD,
            StoreUint32 = 0x05000000,
            StoreUint8 = 0x05400000,
        };

        enum DataTransferTypeB {
            LoadUint16 = 0x010000b0 | DT_LOAD,
            LoadInt16 = 0x010000f0 | DT_LOAD,
            LoadInt8 = 0x010000d0 | DT_LOAD,
            StoreUint16 = 0x010000b0,
        };

        enum DataTransferTypeFloat {
            LoadFloat = 0x0d000a00 | DT_LOAD,
            LoadDouble = 0x0d000b00 | DT_LOAD,
            StoreFloat = 0x0d000a00,
            StoreDouble = 0x0d000b00,
        };

        // Masks of ARM instructions
        enum {
            BRANCH_MASK = 0x00ffffff,
            NONARM = 0xf0000000,
            SDT_MASK = 0x0c000000,
            SDT_OFFSET_MASK = 0xfff,
        };

        enum {
            BOFFSET_MIN = -0x00800000,
            BOFFSET_MAX = 0x007fffff,
            SDT = 0x04000000,
        };

        enum {
            padForAlign8  = 0x00,
            padForAlign16 = 0x0000,
            padForAlign32 = 0xe12fff7f // 'bkpt 0xffff' instruction.
        };

        static const ARMWord INVALID_IMM = 0xf0000000;
        static const ARMWord InvalidBranchTarget = 0xffffffff;
        static const int DefaultPrefetching = 2;

        // Instruction formating

        void emitInst(ARMWord op, int rd, int rn, ARMWord op2)
        {
            ASSERT(((op2 & ~OP2_IMM) <= 0xfff) || (((op2 & ~OP2_IMM_HALF) <= 0xfff)));
            m_buffer.putInt(op | RN(rn) | RD(rd) | op2);
        }

        void emitDoublePrecisionInst(ARMWord op, int dd, int dn, int dm)
        {
            ASSERT((dd >= 0 && dd <= 31) && (dn >= 0 && dn <= 31) && (dm >= 0 && dm <= 31));
            m_buffer.putInt(op | ((dd & 0xf) << 12) | ((dd & 0x10) << (22 - 4))
                               | ((dn & 0xf) << 16) | ((dn & 0x10) << (7 - 4))
                               | (dm & 0xf) | ((dm & 0x10) << (5 - 4)));
        }

        void emitSinglePrecisionInst(ARMWord op, int sd, int sn, int sm)
        {
            ASSERT((sd >= 0 && sd <= 31) && (sn >= 0 && sn <= 31) && (sm >= 0 && sm <= 31));
            m_buffer.putInt(op | ((sd >> 1) << 12) | ((sd & 0x1) << 22)
                               | ((sn >> 1) << 16) | ((sn & 0x1) << 7)
                               | (sm >> 1) | ((sm & 0x1) << 5));
        }

        void and_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | AND, rd, rn, op2);
        }

        void ands_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | AND | SET_CC, rd, rn, op2);
        }

        void eor_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | EOR, rd, rn, op2);
        }

        void eors_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | EOR | SET_CC, rd, rn, op2);
        }

        void sub_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | SUB, rd, rn, op2);
        }

        void subs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | SUB | SET_CC, rd, rn, op2);
        }

        void rsb_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | RSB, rd, rn, op2);
        }

        void rsbs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | RSB | SET_CC, rd, rn, op2);
        }

        void add_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ADD, rd, rn, op2);
        }

        void adds_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ADD | SET_CC, rd, rn, op2);
        }

        void adc_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ADC, rd, rn, op2);
        }

        void adcs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ADC | SET_CC, rd, rn, op2);
        }

        void sbc_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | SBC, rd, rn, op2);
        }

        void sbcs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | SBC | SET_CC, rd, rn, op2);
        }

        void rsc_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | RSC, rd, rn, op2);
        }

        void rscs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | RSC | SET_CC, rd, rn, op2);
        }

        void tst_r(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | TST | SET_CC, 0, rn, op2);
        }

        void teq_r(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | TEQ | SET_CC, 0, rn, op2);
        }

        void cmp_r(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | CMP | SET_CC, 0, rn, op2);
        }

        void cmn_r(int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | CMN | SET_CC, 0, rn, op2);
        }

        void orr_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ORR, rd, rn, op2);
        }

        void orrs_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | ORR | SET_CC, rd, rn, op2);
        }

        void mov_r(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | MOV, rd, ARMRegisters::r0, op2);
        }

#if WTF_ARM_ARCH_AT_LEAST(7)
        void movw_r(int rd, ARMWord op2, Condition cc = AL)
        {
            ASSERT((op2 | 0xf0fff) == 0xf0fff);
            m_buffer.putInt(static_cast<ARMWord>(cc) | MOVW | RD(rd) | op2);
        }

        void movt_r(int rd, ARMWord op2, Condition cc = AL)
        {
            ASSERT((op2 | 0xf0fff) == 0xf0fff);
            m_buffer.putInt(static_cast<ARMWord>(cc) | MOVT | RD(rd) | op2);
        }
#endif

        void movs_r(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | MOV | SET_CC, rd, ARMRegisters::r0, op2);
        }

        void bic_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | BIC, rd, rn, op2);
        }

        void bics_r(int rd, int rn, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | BIC | SET_CC, rd, rn, op2);
        }

        void mvn_r(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | MVN, rd, ARMRegisters::r0, op2);
        }

        void mvns_r(int rd, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | MVN | SET_CC, rd, ARMRegisters::r0, op2);
        }

        void mul_r(int rd, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(static_cast<ARMWord>(cc) | MUL | RN(rd) | RS(rn) | RM(rm));
        }

        void muls_r(int rd, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(static_cast<ARMWord>(cc) | MUL | SET_CC | RN(rd) | RS(rn) | RM(rm));
        }

        void mull_r(int rdhi, int rdlo, int rn, int rm, Condition cc = AL)
        {
            m_buffer.putInt(static_cast<ARMWord>(cc) | MULL | RN(rdhi) | RD(rdlo) | RS(rn) | RM(rm));
        }

        void vmov_f64_r(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VMOV_F64, dd, 0, dm);
        }

        void vadd_f64_r(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VADD_F64, dd, dn, dm);
        }

        void vdiv_f64_r(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VDIV_F64, dd, dn, dm);
        }

        void vsub_f64_r(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VSUB_F64, dd, dn, dm);
        }

        void vmul_f64_r(int dd, int dn, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VMUL_F64, dd, dn, dm);
        }

        void vcmp_f64_r(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCMP_F64, dd, 0, dm);
        }

        void vsqrt_f64_r(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VSQRT_F64, dd, 0, dm);
        }

        void vabs_f64_r(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VABS_F64, dd, 0, dm);
        }

        void vneg_f64_r(int dd, int dm, Condition cc = AL)
        {
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VNEG_F64, dd, 0, dm);
        }

        void ldr_imm(int rd, ARMWord imm, Condition cc = AL)
        {
            m_buffer.putIntWithConstantInt(static_cast<ARMWord>(cc) | LoadUint32 | DT_UP | RN(ARMRegisters::pc) | RD(rd), imm, true);
        }

        void ldr_un_imm(int rd, ARMWord imm, Condition cc = AL)
        {
            m_buffer.putIntWithConstantInt(static_cast<ARMWord>(cc) | LoadUint32 | DT_UP | RN(ARMRegisters::pc) | RD(rd), imm);
        }

        void dtr_u(DataTransferTypeA transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType | DT_UP, rd, rb, op2);
        }

        void dtr_ur(DataTransferTypeA transferType, int rd, int rb, int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType | DT_UP | OP2_OFSREG, rd, rb, rm);
        }

        void dtr_d(DataTransferTypeA transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType, rd, rb, op2);
        }

        void dtr_dr(DataTransferTypeA transferType, int rd, int rb, int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType | OP2_OFSREG, rd, rb, rm);
        }

        void dtrh_u(DataTransferTypeB transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType | DT_UP, rd, rb, op2);
        }

        void dtrh_ur(DataTransferTypeB transferType, int rd, int rn, int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType | DT_UP, rd, rn, rm);
        }

        void dtrh_d(DataTransferTypeB transferType, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType, rd, rb, op2);
        }

        void dtrh_dr(DataTransferTypeB transferType, int rd, int rn, int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | transferType, rd, rn, rm);
        }

        void fdtr_u(DataTransferTypeFloat type, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            ASSERT(op2 <= 0xff && rd <= 15);
            /* Only d0-d15 and s0, s2, s4 ... s30 are supported. */
            m_buffer.putInt(static_cast<ARMWord>(cc) | DT_UP | type | (rd << 12) | RN(rb) | op2);
        }

        void fdtr_d(DataTransferTypeFloat type, int rd, int rb, ARMWord op2, Condition cc = AL)
        {
            ASSERT(op2 <= 0xff && rd <= 15);
            /* Only d0-d15 and s0, s2, s4 ... s30 are supported. */
            m_buffer.putInt(static_cast<ARMWord>(cc) | type | (rd << 12) | RN(rb) | op2);
        }

        void push_r(int reg, Condition cc = AL)
        {
            ASSERT(ARMWord(reg) <= 0xf);
            m_buffer.putInt(static_cast<ARMWord>(cc) | StoreUint32 | DT_WB | RN(ARMRegisters::sp) | RD(reg) | 0x4);
        }

        void pop_r(int reg, Condition cc = AL)
        {
            ASSERT(ARMWord(reg) <= 0xf);
            m_buffer.putInt(static_cast<ARMWord>(cc) | (LoadUint32 ^ DT_PRE) | DT_UP | RN(ARMRegisters::sp) | RD(reg) | 0x4);
        }

        inline void poke_r(int reg, Condition cc = AL)
        {
            dtr_d(StoreUint32, ARMRegisters::sp, 0, reg, cc);
        }

        inline void peek_r(int reg, Condition cc = AL)
        {
            dtr_u(LoadUint32, reg, ARMRegisters::sp, 0, cc);
        }

        void vmov_vfp64_r(int sm, int rt, int rt2, Condition cc = AL)
        {
            ASSERT(rt != rt2);
            m_buffer.putInt(static_cast<ARMWord>(cc) | VMOV_VFP64 | RN(rt2) | RD(rt) | (sm & 0xf) | ((sm & 0x10) << (5 - 4)));
        }

        void vmov_arm64_r(int rt, int rt2, int sm, Condition cc = AL)
        {
            ASSERT(rt != rt2);
            m_buffer.putInt(static_cast<ARMWord>(cc) | VMOV_ARM64 | RN(rt2) | RD(rt) | (sm & 0xf) | ((sm & 0x10) << (5 - 4)));
        }

        void vmov_vfp32_r(int sn, int rt, Condition cc = AL)
        {
            ASSERT(rt <= 15);
            emitSinglePrecisionInst(static_cast<ARMWord>(cc) | VMOV_VFP32, rt << 1, sn, 0);
        }

        void vmov_arm32_r(int rt, int sn, Condition cc = AL)
        {
            ASSERT(rt <= 15);
            emitSinglePrecisionInst(static_cast<ARMWord>(cc) | VMOV_ARM32, rt << 1, sn, 0);
        }

        void vcvt_f64_s32_r(int dd, int sm, Condition cc = AL)
        {
            ASSERT(!(sm & 0x1)); // sm must be divisible by 2
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCVT_F64_S32, dd, 0, (sm >> 1));
        }

        void vcvt_s32_f64_r(int sd, int dm, Condition cc = AL)
        {
            ASSERT(!(sd & 0x1)); // sd must be divisible by 2
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCVT_S32_F64, (sd >> 1), 0, dm);
        }

        void vcvt_u32_f64_r(int sd, int dm, Condition cc = AL)
        {
            ASSERT(!(sd & 0x1)); // sd must be divisible by 2
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCVT_U32_F64, (sd >> 1), 0, dm);
        }

        void vcvt_f64_f32_r(int dd, int sm, Condition cc = AL)
        {
            ASSERT(dd <= 15 && sm <= 15);
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCVT_F64_F32, dd, 0, sm);
        }

        void vcvt_f32_f64_r(int dd, int sm, Condition cc = AL)
        {
            ASSERT(dd <= 15 && sm <= 15);
            emitDoublePrecisionInst(static_cast<ARMWord>(cc) | VCVT_F32_F64, dd, 0, sm);
        }

        void vmrs_apsr(Condition cc = AL)
        {
            m_buffer.putInt(static_cast<ARMWord>(cc) | VMRS_APSR);
        }

        void clz_r(int rd, int rm, Condition cc = AL)
        {
            m_buffer.putInt(static_cast<ARMWord>(cc) | CLZ | RD(rd) | RM(rm));
        }

        void bkpt(ARMWord value)
        {
            m_buffer.putInt(BKPT | ((value & 0xff0) << 4) | (value & 0xf));
        }

        void nop()
        {
            m_buffer.putInt(NOP);
        }

        void bx(int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | BX, 0, 0, RM(rm));
        }

        AssemblerLabel blx(int rm, Condition cc = AL)
        {
            emitInst(static_cast<ARMWord>(cc) | BLX, 0, 0, RM(rm));
            return m_buffer.label();
        }

        static ARMWord lsl(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x00;
        }

        static ARMWord lsr(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x20;
        }

        static ARMWord asr(int reg, ARMWord value)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(value <= 0x1f);
            return reg | (value << 7) | 0x40;
        }

        static ARMWord lsl_r(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x10;
        }

        static ARMWord lsr_r(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x30;
        }

        static ARMWord asr_r(int reg, int shiftReg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            ASSERT(shiftReg <= ARMRegisters::pc);
            return reg | (shiftReg << 8) | 0x50;
        }

        // General helpers

        size_t codeSize() const
        {
            return m_buffer.codeSize();
        }

        void ensureSpace(int insnSpace, int constSpace)
        {
            m_buffer.ensureSpace(insnSpace, constSpace);
        }

        int sizeOfConstantPool()
        {
            return m_buffer.sizeOfConstantPool();
        }

        AssemblerLabel labelIgnoringWatchpoints()
        {
            m_buffer.ensureSpaceForAnyInstruction();
            return m_buffer.label();
        }

        AssemblerLabel labelForWatchpoint()
        {
            m_buffer.ensureSpaceForAnyInstruction(maxJumpReplacementSize() / sizeof(ARMWord));
            AssemblerLabel result = m_buffer.label();
            if (result.m_offset != (m_indexOfTailOfLastWatchpoint - maxJumpReplacementSize()))
                result = label();
            m_indexOfTailOfLastWatchpoint = result.m_offset + maxJumpReplacementSize();
            return label();
        }

        AssemblerLabel label()
        {
            AssemblerLabel result = labelIgnoringWatchpoints();
            while (result.m_offset + 1 < m_indexOfTailOfLastWatchpoint) {
                nop();
                // The available number of instructions are ensured by labelForWatchpoint.
                result = m_buffer.label();
            }
            return result;
        }

        AssemblerLabel align(int alignment)
        {
            while (!m_buffer.isAligned(alignment))
                mov_r(ARMRegisters::r0, ARMRegisters::r0);

            return label();
        }

        AssemblerLabel loadBranchTarget(int rd, Condition cc = AL, int useConstantPool = 0)
        {
            ensureSpace(sizeof(ARMWord), sizeof(ARMWord));
            m_jumps.append(m_buffer.codeSize() | (useConstantPool & 0x1));
            ldr_un_imm(rd, InvalidBranchTarget, cc);
            return m_buffer.label();
        }

        AssemblerLabel jmp(Condition cc = AL, int useConstantPool = 0)
        {
            return loadBranchTarget(ARMRegisters::pc, cc, useConstantPool);
        }

        PassRefPtr<ExecutableMemoryHandle> executableCopy(JSGlobalData&, void* ownerUID, JITCompilationEffort);

        unsigned debugOffset() { return m_buffer.debugOffset(); }

        // DFG assembly helpers for moving data between fp and registers.
        void vmov(RegisterID rd1, RegisterID rd2, FPRegisterID rn)
        {
            vmov_arm64_r(rd1, rd2, rn);
        }

        void vmov(FPRegisterID rd, RegisterID rn1, RegisterID rn2)
        {
            vmov_vfp64_r(rd, rn1, rn2);
        }

        // Patching helpers

        static ARMWord* getLdrImmAddress(ARMWord* insn)
        {
            // Check for call
            if ((*insn & 0x0f7f0000) != 0x051f0000) {
                // Must be BLX
                ASSERT((*insn & 0x012fff30) == 0x012fff30);
                insn--;
            }

            // Must be an ldr ..., [pc +/- imm]
            ASSERT((*insn & 0x0f7f0000) == 0x051f0000);

            ARMWord addr = reinterpret_cast<ARMWord>(insn) + DefaultPrefetching * sizeof(ARMWord);
            if (*insn & DT_UP)
                return reinterpret_cast<ARMWord*>(addr + (*insn & SDT_OFFSET_MASK));
            return reinterpret_cast<ARMWord*>(addr - (*insn & SDT_OFFSET_MASK));
        }

        static ARMWord* getLdrImmAddressOnPool(ARMWord* insn, uint32_t* constPool)
        {
            // Must be an ldr ..., [pc +/- imm]
            ASSERT((*insn & 0x0f7f0000) == 0x051f0000);

            if (*insn & 0x1)
                return reinterpret_cast<ARMWord*>(constPool + ((*insn & SDT_OFFSET_MASK) >> 1));
            return getLdrImmAddress(insn);
        }

        static void patchPointerInternal(intptr_t from, void* to)
        {
            ARMWord* insn = reinterpret_cast<ARMWord*>(from);
            ARMWord* addr = getLdrImmAddress(insn);
            *addr = reinterpret_cast<ARMWord>(to);
        }

        static ARMWord patchConstantPoolLoad(ARMWord load, ARMWord value)
        {
            value = (value << 1) + 1;
            ASSERT(!(value & ~0xfff));
            return (load & ~0xfff) | value;
        }

        static void patchConstantPoolLoad(void* loadAddr, void* constPoolAddr);

        // Read pointers
        static void* readPointer(void* from)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(from);
            ARMWord* address = getLdrImmAddress(instruction);
            return *reinterpret_cast<void**>(address);
        }

        // Patch pointers

        static void linkPointer(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(code) + from.m_offset, to);
        }

        static void repatchInt32(void* where, int32_t to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(where), reinterpret_cast<void*>(to));
        }

        static void repatchCompact(void* where, int32_t value)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(where);
            ASSERT((*instruction & 0x0f700000) == LoadUint32);
            if (value >= 0)
                *instruction = (*instruction & 0xff7ff000) | DT_UP | value;
            else
                *instruction = (*instruction & 0xff7ff000) | -value;
            cacheFlush(instruction, sizeof(ARMWord));
        }

        static void repatchPointer(void* from, void* to)
        {
            patchPointerInternal(reinterpret_cast<intptr_t>(from), to);
        }

        // Linkers
        static intptr_t getAbsoluteJumpAddress(void* base, int offset = 0)
        {
            return reinterpret_cast<intptr_t>(base) + offset - sizeof(ARMWord);
        }

        void linkJump(AssemblerLabel from, AssemblerLabel to)
        {
            ARMWord* insn = reinterpret_cast<ARMWord*>(getAbsoluteJumpAddress(m_buffer.data(), from.m_offset));
            ARMWord* addr = getLdrImmAddressOnPool(insn, m_buffer.poolAddress());
            *addr = static_cast<ARMWord>(to.m_offset);
        }

        static void linkJump(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(code, from.m_offset), to);
        }

        static void relinkJump(void* from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(from), to);
        }

        static void linkCall(void* code, AssemblerLabel from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(code, from.m_offset), to);
        }

        static void relinkCall(void* from, void* to)
        {
            patchPointerInternal(getAbsoluteJumpAddress(from), to);
        }

        static void* readCallTarget(void* from)
        {
            return reinterpret_cast<void*>(readPointer(reinterpret_cast<void*>(getAbsoluteJumpAddress(from))));
        }

        static void replaceWithJump(void* instructionStart, void* to)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart) - 1;
            intptr_t difference = reinterpret_cast<intptr_t>(to) - (reinterpret_cast<intptr_t>(instruction) + DefaultPrefetching * sizeof(ARMWord));

            if (!(difference & 1)) {
                difference >>= 2;
                if ((difference <= BOFFSET_MAX && difference >= BOFFSET_MIN)) {
                     // Direct branch.
                     instruction[0] = B | AL | (difference & BRANCH_MASK);
                     cacheFlush(instruction, sizeof(ARMWord));
                     return;
                }
            }

            // Load target.
            instruction[0] = LoadUint32 | AL | RN(ARMRegisters::pc) | RD(ARMRegisters::pc) | 4;
            instruction[1] = reinterpret_cast<ARMWord>(to);
            cacheFlush(instruction, sizeof(ARMWord) * 2);
        }

        static ptrdiff_t maxJumpReplacementSize()
        {
            return sizeof(ARMWord) * 2;
        }

        static void replaceWithLoad(void* instructionStart)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);
            cacheFlush(instruction, sizeof(ARMWord));

            ASSERT((*instruction & 0x0ff00000) == 0x02800000 || (*instruction & 0x0ff00000) == 0x05900000);
            if ((*instruction & 0x0ff00000) == 0x02800000) {
                 *instruction = (*instruction & 0xf00fffff) | 0x05900000;
                 cacheFlush(instruction, sizeof(ARMWord));
            }
        }

        static void replaceWithAddressComputation(void* instructionStart)
        {
            ARMWord* instruction = reinterpret_cast<ARMWord*>(instructionStart);
            cacheFlush(instruction, sizeof(ARMWord));

            ASSERT((*instruction & 0x0ff00000) == 0x02800000 || (*instruction & 0x0ff00000) == 0x05900000);
            if ((*instruction & 0x0ff00000) == 0x05900000) {
                 *instruction = (*instruction & 0xf00fffff) | 0x02800000;
                 cacheFlush(instruction, sizeof(ARMWord));
            }
        }

        // Address operations

        static void* getRelocatedAddress(void* code, AssemblerLabel label)
        {
            return reinterpret_cast<void*>(reinterpret_cast<char*>(code) + label.m_offset);
        }

        // Address differences

        static int getDifferenceBetweenLabels(AssemblerLabel a, AssemblerLabel b)
        {
            return b.m_offset - a.m_offset;
        }

        static unsigned getCallReturnOffset(AssemblerLabel call)
        {
            return call.m_offset;
        }

        // Handle immediates

        static ARMWord getOp2(ARMWord imm);

        // Fast case if imm is known to be between 0 and 0xff
        static ARMWord getOp2Byte(ARMWord imm)
        {
            ASSERT(imm <= 0xff);
            return OP2_IMM | imm;
        }

        static ARMWord getOp2Half(ARMWord imm)
        {
            ASSERT(imm <= 0xff);
            return OP2_IMM_HALF | (imm & 0x0f) | ((imm & 0xf0) << 4);
        }

#if WTF_ARM_ARCH_AT_LEAST(7)
        static ARMWord getImm16Op2(ARMWord imm)
        {
            if (imm <= 0xffff)
                return (imm & 0xf000) << 4 | (imm & 0xfff);
            return INVALID_IMM;
        }
#endif
        ARMWord getImm(ARMWord imm, int tmpReg, bool invert = false);
        void moveImm(ARMWord imm, int dest);
        ARMWord encodeComplexImm(ARMWord imm, int dest);

        // Memory load/store helpers

        void dataTransfer32(DataTransferTypeA, RegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransfer32(DataTransferTypeA, RegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);
        void dataTransfer16(DataTransferTypeB, RegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransfer16(DataTransferTypeB, RegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);
        void dataTransferFloat(DataTransferTypeFloat, FPRegisterID srcDst, RegisterID base, int32_t offset);
        void baseIndexTransferFloat(DataTransferTypeFloat, FPRegisterID srcDst, RegisterID base, RegisterID index, int scale, int32_t offset);

        // Constant pool hnadlers

        static ARMWord placeConstantPoolBarrier(int offset)
        {
            offset = (offset - sizeof(ARMWord)) >> 2;
            ASSERT((offset <= BOFFSET_MAX && offset >= BOFFSET_MIN));
            return AL | B | (offset & BRANCH_MASK);
        }

#if OS(LINUX) && COMPILER(RVCT)
        static __asm void cacheFlush(void* code, size_t);
#else
        static void cacheFlush(void* code, size_t size)
        {
#if OS(LINUX) && COMPILER(GCC)
            uintptr_t currentPage = reinterpret_cast<uintptr_t>(code) & ~(pageSize() - 1);
            uintptr_t lastPage = (reinterpret_cast<uintptr_t>(code) + size) & ~(pageSize() - 1);
            do {
                asm volatile(
                    "push    {r7}\n"
                    "mov     r0, %0\n"
                    "mov     r1, %1\n"
                    "mov     r7, #0xf0000\n"
                    "add     r7, r7, #0x2\n"
                    "mov     r2, #0x0\n"
                    "svc     0x0\n"
                    "pop     {r7}\n"
                    :
                    : "r" (currentPage), "r" (currentPage + pageSize())
                    : "r0", "r1", "r2");
                currentPage += pageSize();
            } while (lastPage >= currentPage);
#elif OS(WINCE)
            CacheRangeFlush(code, size, CACHE_SYNC_ALL);
#elif OS(QNX) && ENABLE(ASSEMBLER_WX_EXCLUSIVE)
            UNUSED_PARAM(code);
            UNUSED_PARAM(size);
#elif OS(QNX)
            msync(code, size, MS_INVALIDATE_ICACHE);
#else
#error "The cacheFlush support is missing on this platform."
#endif
        }
#endif

    private:
        static ARMWord RM(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg;
        }

        static ARMWord RS(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 8;
        }

        static ARMWord RD(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 12;
        }

        static ARMWord RN(int reg)
        {
            ASSERT(reg <= ARMRegisters::pc);
            return reg << 16;
        }

        static ARMWord getConditionalField(ARMWord i)
        {
            return i & 0xf0000000;
        }

        int genInt(int reg, ARMWord imm, bool positive);

        ARMBuffer m_buffer;
        Jumps m_jumps;
        uint32_t m_indexOfTailOfLastWatchpoint;
    };

} // namespace JSC

#endif // ENABLE(ASSEMBLER) && CPU(ARM_TRADITIONAL)

#endif // ARMAssembler_h
