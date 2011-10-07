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

#ifndef DFGJITCodeGenerator_h
#define DFGJITCodeGenerator_h

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include <dfg/DFGGenerationInfo.h>
#include <dfg/DFGGraph.h>
#include <dfg/DFGJITCompiler.h>
#include <dfg/DFGNode.h>
#include <dfg/DFGOperations.h>
#include <dfg/DFGRegisterBank.h>

namespace JSC { namespace DFG {

class SpeculateIntegerOperand;
class SpeculateStrictInt32Operand;
class SpeculateDoubleOperand;
class SpeculateCellOperand;
class SpeculateBooleanOperand;


// === JITCodeGenerator ===
//
// This class provides common infrastructure used by the speculative &
// non-speculative JITs. Provides common mechanisms for virtual and
// physical register management, calls out from JIT code to helper
// functions, etc.
class JITCodeGenerator {
protected:
    typedef MacroAssembler::TrustedImm32 TrustedImm32;
    typedef MacroAssembler::Imm32 Imm32;

    // These constants are used to set priorities for spill order for
    // the register allocator.
    enum SpillOrder {
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled  = 2, // no spill
        SpillOrderJS       = 4, // needs spill
        SpillOrderCell     = 4, // needs spill
        SpillOrderStorage  = 4, // needs spill
        SpillOrderInteger  = 5, // needs spill and box
        SpillOrderDouble   = 6, // needs spill and convert
    };
    
    enum UseChildrenMode { CallUseChildren, UseChildrenCalledExplicitly };
    
    static const double twoToThe32;

public:
    Node& at(NodeIndex nodeIndex)
    {
        return m_jit.graph()[nodeIndex];
    }
    
    GPRReg fillInteger(NodeIndex, DataFormat& returnFormat);
    FPRReg fillDouble(NodeIndex);
#if USE(JSVALUE64)
    GPRReg fillJSValue(NodeIndex);
#elif USE(JSVALUE32_64)
    bool fillJSValue(NodeIndex, GPRReg&, GPRReg&, FPRReg&);
#endif
    GPRReg fillStorage(NodeIndex);

    // lock and unlock GPR & FPR registers.
    void lock(GPRReg reg)
    {
        m_gprs.lock(reg);
    }
    void lock(FPRReg reg)
    {
        m_fprs.lock(reg);
    }
    void unlock(GPRReg reg)
    {
        m_gprs.unlock(reg);
    }
    void unlock(FPRReg reg)
    {
        m_fprs.unlock(reg);
    }

    // Used to check whether a child node is on its last use,
    // and its machine registers may be reused.
    bool canReuse(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = at(nodeIndex).virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.canReuse();
    }
    GPRReg reuse(GPRReg reg)
    {
        m_gprs.lock(reg);
        return reg;
    }
    FPRReg reuse(FPRReg reg)
    {
        m_fprs.lock(reg);
        return reg;
    }

    // Allocate a gpr/fpr.
    GPRReg allocate()
    {
        VirtualRegister spillMe;
        GPRReg gpr = m_gprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister) {
#if USE(JSVALUE32_64)
            GenerationInfo& info = m_generationInfo[spillMe];
            ASSERT(info.registerFormat() != DataFormatJSDouble);
            if ((info.registerFormat() & DataFormatJS))
                m_gprs.release(info.tagGPR() == gpr ? info.payloadGPR() : info.tagGPR());
#endif
            spill(spillMe);
        }
        return gpr;
    }
    GPRReg allocate(GPRReg specific)
    {
        VirtualRegister spillMe = m_gprs.allocateSpecific(specific);
        if (spillMe != InvalidVirtualRegister) {
#if USE(JSVALUE32_64)
            GenerationInfo& info = m_generationInfo[spillMe];
            ASSERT(info.registerFormat() != DataFormatJSDouble);
            if ((info.registerFormat() & DataFormatJS))
                m_gprs.release(info.tagGPR() == specific ? info.payloadGPR() : info.tagGPR());
#endif
            spill(spillMe);
        }
        return specific;
    }
    GPRReg tryAllocate()
    {
        return m_gprs.tryAllocate();
    }
    FPRReg fprAllocate()
    {
        VirtualRegister spillMe;
        FPRReg fpr = m_fprs.allocate(spillMe);
        if (spillMe != InvalidVirtualRegister)
            spill(spillMe);
        return fpr;
    }

    // Check whether a VirtualRegsiter is currently in a machine register.
    // We use this when filling operands to fill those that are already in
    // machine registers first (by locking VirtualRegsiters that are already
    // in machine register before filling those that are not we attempt to
    // avoid spilling values we will need immediately).
    bool isFilled(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = at(nodeIndex).virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() != DataFormatNone;
    }
    bool isFilledDouble(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = at(nodeIndex).virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        return info.registerFormat() == DataFormatDouble;
    }

    // Called on an operand once it has been consumed by a parent node.
    void use(NodeIndex nodeIndex)
    {
        VirtualRegister virtualRegister = at(nodeIndex).virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];

        // use() returns true when the value becomes dead, and any
        // associated resources may be freed.
        if (!info.use())
            return;

        // Release the associated machine registers.
        DataFormat registerFormat = info.registerFormat();
#if USE(JSVALUE64)
        if (registerFormat == DataFormatDouble)
            m_fprs.release(info.fpr());
        else if (registerFormat != DataFormatNone)
            m_gprs.release(info.gpr());
#elif USE(JSVALUE32_64)
        if (registerFormat == DataFormatDouble || registerFormat == DataFormatJSDouble)
            m_fprs.release(info.fpr());
        else if (registerFormat == DataFormatInteger || registerFormat == DataFormatCell || registerFormat == DataFormatStorage)
            m_gprs.release(info.gpr());
        else if (registerFormat & DataFormatJS) {
            m_gprs.release(info.tagGPR());
            m_gprs.release(info.payloadGPR());
        }
#endif
    }

    static void markCellCard(MacroAssembler&, GPRReg ownerGPR, GPRReg scratchGPR1, GPRReg scratchGPR2);
    static void writeBarrier(MacroAssembler&, GPRReg ownerGPR, GPRReg scratchGPR1, GPRReg scratchGPR2, WriteBarrierUseKind);

    void writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, NodeIndex valueIndex, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(GPRReg ownerGPR, JSCell* value, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(JSCell* owner, GPRReg valueGPR, NodeIndex valueIndex, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg);

    static GPRReg selectScratchGPR(GPRReg preserve1 = InvalidGPRReg, GPRReg preserve2 = InvalidGPRReg, GPRReg preserve3 = InvalidGPRReg)
    {
        if (preserve1 != GPRInfo::regT0 && preserve2 != GPRInfo::regT0 && preserve3 != GPRInfo::regT0)
            return GPRInfo::regT0;

        if (preserve1 != GPRInfo::regT1 && preserve2 != GPRInfo::regT1 && preserve3 != GPRInfo::regT1)
            return GPRInfo::regT1;

        if (preserve1 != GPRInfo::regT2 && preserve2 != GPRInfo::regT2 && preserve3 != GPRInfo::regT2)
            return GPRInfo::regT2;

        return GPRInfo::regT3;
    }

protected:
    JITCodeGenerator(JITCompiler& jit)
        : m_jit(jit)
        , m_compileIndex(0)
        , m_generationInfo(m_jit.codeBlock()->m_numCalleeRegisters)
        , m_blockHeads(jit.graph().m_blocks.size())
    {
    }
    
    void clearGenerationInfo();

#if USE(JSVALUE32_64)
    bool registersMatched(GenerationInfo& info, GPRReg exclude, GPRReg exclude2)
    {
        ASSERT(info.registerFormat() != DataFormatNone);
        ASSERT(info.registerFormat() != DataFormatDouble);
        ASSERT(info.registerFormat() != DataFormatJSDouble);
        return !(info.registerFormat() & DataFormatJS) ? (info.gpr() == exclude || info.gpr() == exclude2) :  (info.tagGPR() == exclude || info.tagGPR() == exclude2 || info.payloadGPR() == exclude || info.payloadGPR() == exclude2);
    }
#endif

    // These methods are used when generating 'unexpected'
    // calls out from JIT code to C++ helper routines -
    // they spill all live values to the appropriate
    // slots in the RegisterFile without changing any state
    // in the GenerationInfo.
    void silentSpillGPR(VirtualRegister spillMe, GPRReg exclude = InvalidGPRReg, GPRReg exclude2 = InvalidGPRReg)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        ASSERT(info.registerFormat() != DataFormatNone);
        ASSERT(info.registerFormat() != DataFormatDouble);

#if USE(JSVALUE64)
        UNUSED_PARAM(exclude2);
        if (!info.needsSpill() || (info.gpr() == exclude))
#elif USE(JSVALUE32_64)
        if (!info.needsSpill() || registersMatched(info, exclude, exclude2))
#endif
            return;

        DataFormat registerFormat = info.registerFormat();

#if USE(JSVALUE64)
        if (registerFormat == DataFormatInteger)
            m_jit.store32(info.gpr(), JITCompiler::addressFor(spillMe));
        else {
            ASSERT(registerFormat & DataFormatJS || registerFormat == DataFormatCell || registerFormat == DataFormatStorage);
            m_jit.storePtr(info.gpr(), JITCompiler::addressFor(spillMe));
        }
#elif USE(JSVALUE32_64)
        if (registerFormat == DataFormatInteger)
            m_jit.store32(info.gpr(), JITCompiler::payloadFor(spillMe));
        else if (registerFormat == DataFormatCell)
            m_jit.storePtr(info.gpr(), JITCompiler::payloadFor(spillMe));
        else if (registerFormat == DataFormatStorage)
            m_jit.storePtr(info.gpr(), JITCompiler::addressFor(spillMe));
        else {
            ASSERT(registerFormat & DataFormatJS);
            m_jit.store32(info.tagGPR(), JITCompiler::tagFor(spillMe));
            m_jit.store32(info.payloadGPR(), JITCompiler::payloadFor(spillMe));
        }
#endif
    }
    void silentSpillFPR(VirtualRegister spillMe, FPRReg exclude = InvalidFPRReg)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        ASSERT(info.registerFormat() == DataFormatDouble);

        if (info.fpr() == exclude)
            return;
        if (!info.needsSpill()) {
            // it's either a constant or it's already been spilled
            ASSERT(at(info.nodeIndex()).hasConstant() || info.spillFormat() != DataFormatNone);
            return;
        }
        
        // it's neither a constant nor has it been spilled.
        ASSERT(!at(info.nodeIndex()).hasConstant());
        ASSERT(info.spillFormat() == DataFormatNone);

        m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
    }

    void silentFillGPR(VirtualRegister spillMe, GPRReg exclude = InvalidGPRReg, GPRReg exclude2 = InvalidGPRReg)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
#if USE(JSVALUE64)
        UNUSED_PARAM(exclude2);
        if (info.gpr() == exclude)
#elif USE(JSVALUE32_64)
        if (registersMatched(info, exclude, exclude2))
#endif
            return;

        NodeIndex nodeIndex = info.nodeIndex();
        Node& node = at(nodeIndex);
        ASSERT(info.registerFormat() != DataFormatNone);
        ASSERT(info.registerFormat() != DataFormatDouble);
        DataFormat registerFormat = info.registerFormat();

#if USE(JSVALUE64)
        if (registerFormat == DataFormatInteger) {
            if (node.hasConstant()) {
                ASSERT(isInt32Constant(nodeIndex));
                m_jit.move(Imm32(valueOfInt32Constant(nodeIndex)), info.gpr());
            } else
                m_jit.load32(JITCompiler::addressFor(spillMe), info.gpr());
            return;
        }

        if (node.hasConstant())
            m_jit.move(valueOfJSConstantAsImmPtr(nodeIndex), info.gpr());
        else {
            ASSERT(registerFormat & DataFormatJS || registerFormat == DataFormatCell || registerFormat == DataFormatStorage);
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), info.gpr());
        }
#elif USE(JSVALUE32_64)
        if (registerFormat == DataFormatInteger || registerFormat == DataFormatCell) {
            if (node.isConstant())
                m_jit.move(Imm32(valueOfInt32Constant(nodeIndex)), info.gpr());
            else
                m_jit.load32(JITCompiler::payloadFor(spillMe), info.gpr());
        } else if (registerFormat == DataFormatStorage)
            m_jit.load32(JITCompiler::addressFor(spillMe), info.gpr());
        else 
            m_jit.emitLoad(nodeIndex, info.tagGPR(), info.payloadGPR());
#endif
    }

    void silentFillFPR(VirtualRegister spillMe, GPRReg canTrample, FPRReg exclude = InvalidFPRReg)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        if (info.fpr() == exclude)
            return;

        NodeIndex nodeIndex = info.nodeIndex();
#if USE(JSVALUE64)
        Node& node = at(nodeIndex);
        ASSERT(info.registerFormat() == DataFormatDouble);

        if (node.hasConstant()) {
            ASSERT(isNumberConstant(nodeIndex));
            m_jit.move(JITCompiler::ImmPtr(bitwise_cast<void*>(valueOfNumberConstant(nodeIndex))), canTrample);
            m_jit.movePtrToDouble(canTrample, info.fpr());
            return;
        }
        
        if (info.spillFormat() != DataFormatNone) {
            // it was already spilled previously, which means we need unboxing.
            ASSERT(info.spillFormat() & DataFormatJS);
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), canTrample);
            unboxDouble(canTrample, info.fpr());
            return;
        }

        m_jit.loadDouble(JITCompiler::addressFor(spillMe), info.fpr());
#elif USE(JSVALUE32_64)
        UNUSED_PARAM(canTrample);
        ASSERT(info.registerFormat() == DataFormatDouble || info.registerFormat() == DataFormatJSDouble);
        m_jit.emitLoadDouble(nodeIndex, info.fpr());
#endif
    }

    void silentSpillAllRegisters(GPRReg exclude, GPRReg exclude2 = InvalidGPRReg)
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillGPR(iter.name(), exclude, exclude2);
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillFPR(iter.name());
        }
    }
    void silentSpillAllRegisters(FPRReg exclude)
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillGPR(iter.name());
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillFPR(iter.name(), exclude);
        }
    }

    void silentFillAllRegisters(GPRReg exclude, GPRReg exclude2 = InvalidGPRReg)
    {
        GPRReg canTrample = GPRInfo::regT0;
        if (exclude == GPRInfo::regT0)
            canTrample = GPRInfo::regT1;
        
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentFillFPR(iter.name(), canTrample);
        }
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentFillGPR(iter.name(), exclude, exclude2);
        }
    }
    void silentFillAllRegisters(FPRReg exclude)
    {
        GPRReg canTrample = GPRInfo::regT0;
        
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister) {
                ASSERT_UNUSED(exclude, iter.regID() != exclude);
                silentFillFPR(iter.name(), canTrample, exclude);
            }
        }
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentFillGPR(iter.name());
        }
    }

    // These methods convert between doubles, and doubles boxed and JSValues.
#if USE(JSVALUE64)
    GPRReg boxDouble(FPRReg fpr, GPRReg gpr)
    {
        return m_jit.boxDouble(fpr, gpr);
    }
    FPRReg unboxDouble(GPRReg gpr, FPRReg fpr)
    {
        return m_jit.unboxDouble(gpr, fpr);
    }
    GPRReg boxDouble(FPRReg fpr)
    {
        return boxDouble(fpr, allocate());
    }
#elif USE(JSVALUE32_64)
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR, VirtualRegister virtualRegister)
    {
        m_jit.boxDouble(fpr, tagGPR, payloadGPR, virtualRegister);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, VirtualRegister virtualRegister)
    {
        m_jit.unboxDouble(tagGPR, payloadGPR, fpr, virtualRegister);
    }
#endif

    // Spill a VirtualRegister to the RegisterFile.
    void spill(VirtualRegister spillMe)
    {
        GenerationInfo& info = m_generationInfo[spillMe];

#if USE(JSVALUE32_64)
        if (info.registerFormat() == DataFormatNone) // it has been spilled. JS values which have two GPRs can reach here
            return;
#endif
        // Check the GenerationInfo to see if this value need writing
        // to the RegisterFile - if not, mark it as spilled & return.
        if (!info.needsSpill()) {
            info.setSpilled();
            return;
        }

        DataFormat spillFormat = info.registerFormat();
        switch (spillFormat) {
        case DataFormatStorage: {
            // This is special, since it's not a JS value - as in it's not visible to JS
            // code.
            m_jit.storePtr(info.gpr(), JITCompiler::addressFor(spillMe));
            info.spill(DataFormatStorage);
            return;
        }

#if USE(JSVALUE64)
        case DataFormatDouble: {
            // All values are spilled as JSValues, so box the double via a temporary gpr.
            GPRReg gpr = boxDouble(info.fpr());
            m_jit.storePtr(gpr, JITCompiler::addressFor(spillMe));
            unlock(gpr);
            info.spill(DataFormatJSDouble);
            return;
        }

        default:
            // The following code handles JSValues, int32s, and cells.
            ASSERT(spillFormat == DataFormatInteger || spillFormat == DataFormatCell || spillFormat & DataFormatJS);
            
            GPRReg reg = info.gpr();
            // We need to box int32 and cell values ...
            // but on JSVALUE64 boxing a cell is a no-op!
            if (spillFormat == DataFormatInteger)
                m_jit.orPtr(GPRInfo::tagTypeNumberRegister, reg);
            
            // Spill the value, and record it as spilled in its boxed form.
            m_jit.storePtr(reg, JITCompiler::addressFor(spillMe));
            info.spill((DataFormat)(spillFormat | DataFormatJS));
            return;
        }
#elif USE(JSVALUE32_64)
        case DataFormatDouble:
        case DataFormatJSDouble: {
            // On JSVALUE32_64 boxing a double is a no-op.
            m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
            info.spill(DataFormatJSDouble);
            return;
        }
        default:
            // The following code handles JSValues, int32s, and cells.
            ASSERT(spillFormat == DataFormatInteger || spillFormat == DataFormatCell || spillFormat & DataFormatJS);

            if (spillFormat == DataFormatInteger || spillFormat == DataFormatCell) {
                GPRReg reg = info.gpr();
                m_jit.store32(reg, JITCompiler::payloadFor(spillMe));
                // We need to box int32 and cell values ...
                if (spillFormat == DataFormatInteger)
                    m_jit.store32(TrustedImm32(JSValue::Int32Tag), JITCompiler::tagFor(spillMe));
                else // cells
                    m_jit.store32(TrustedImm32(JSValue::CellTag), JITCompiler::tagFor(spillMe));
            } else { // JSValue
                m_jit.store32(info.tagGPR(), JITCompiler::tagFor(spillMe));
                m_jit.store32(info.payloadGPR(), JITCompiler::payloadFor(spillMe));
            }
            info.spill((DataFormat)(spillFormat | DataFormatJS));
            return;
        }
#endif
    }
    
    bool isStrictInt32(NodeIndex);
    
    bool isKnownInteger(NodeIndex);
    bool isKnownNumeric(NodeIndex);
    bool isKnownCell(NodeIndex);
    
    bool isKnownNotInteger(NodeIndex);
    bool isKnownNotNumber(NodeIndex);

    bool isKnownBoolean(NodeIndex);

    bool isKnownNotCell(NodeIndex);
    
    // Checks/accessors for constant values.
    bool isConstant(NodeIndex nodeIndex) { return m_jit.isConstant(nodeIndex); }
    bool isJSConstant(NodeIndex nodeIndex) { return m_jit.isJSConstant(nodeIndex); }
    bool isInt32Constant(NodeIndex nodeIndex) { return m_jit.isInt32Constant(nodeIndex); }
    bool isDoubleConstant(NodeIndex nodeIndex) { return m_jit.isDoubleConstant(nodeIndex); }
    bool isNumberConstant(NodeIndex nodeIndex) { return m_jit.isNumberConstant(nodeIndex); }
    bool isBooleanConstant(NodeIndex nodeIndex) { return m_jit.isBooleanConstant(nodeIndex); }
    bool isFunctionConstant(NodeIndex nodeIndex) { return m_jit.isFunctionConstant(nodeIndex); }
    int32_t valueOfInt32Constant(NodeIndex nodeIndex) { return m_jit.valueOfInt32Constant(nodeIndex); }
    double valueOfNumberConstant(NodeIndex nodeIndex) { return m_jit.valueOfNumberConstant(nodeIndex); }
#if USE(JSVALUE32_64)
    void* addressOfDoubleConstant(NodeIndex nodeIndex) { return m_jit.addressOfDoubleConstant(nodeIndex); }
#endif
    JSValue valueOfJSConstant(NodeIndex nodeIndex) { return m_jit.valueOfJSConstant(nodeIndex); }
    bool valueOfBooleanConstant(NodeIndex nodeIndex) { return m_jit.valueOfBooleanConstant(nodeIndex); }
    JSFunction* valueOfFunctionConstant(NodeIndex nodeIndex) { return m_jit.valueOfFunctionConstant(nodeIndex); }
    bool isNullConstant(NodeIndex nodeIndex)
    {
        if (!isConstant(nodeIndex))
            return false;
        return valueOfJSConstant(nodeIndex).isNull();
    }

    Identifier* identifier(unsigned index)
    {
        return &m_jit.codeBlock()->identifier(index);
    }

    // Spill all VirtualRegisters back to the RegisterFile.
    void flushRegisters()
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister) {
                spill(iter.name());
                iter.release();
            }
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister) {
                spill(iter.name());
                iter.release();
            }
        }
    }

#ifndef NDEBUG
    // Used to ASSERT flushRegisters() has been called prior to
    // calling out from JIT code to a C helper function.
    bool isFlushed()
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                return false;
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                return false;
        }
        return true;
    }
#endif

#if USE(JSVALUE64)
    MacroAssembler::ImmPtr valueOfJSConstantAsImmPtr(NodeIndex nodeIndex)
    {
        return MacroAssembler::ImmPtr(JSValue::encode(valueOfJSConstant(nodeIndex)));
    }
#endif

    // Helper functions to enable code sharing in implementations of bit/shift ops.
    void bitOp(NodeType op, int32_t imm, GPRReg op1, GPRReg result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(Imm32(imm), op1, result);
            break;
        case BitOr:
            m_jit.or32(Imm32(imm), op1, result);
            break;
        case BitXor:
            m_jit.xor32(Imm32(imm), op1, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void bitOp(NodeType op, GPRReg op1, GPRReg op2, GPRReg result)
    {
        switch (op) {
        case BitAnd:
            m_jit.and32(op1, op2, result);
            break;
        case BitOr:
            m_jit.or32(op1, op2, result);
            break;
        case BitXor:
            m_jit.xor32(op1, op2, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, GPRReg op1, int32_t shiftAmount, GPRReg result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, Imm32(shiftAmount), result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, Imm32(shiftAmount), result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    void shiftOp(NodeType op, GPRReg op1, GPRReg shiftAmount, GPRReg result)
    {
        switch (op) {
        case BitRShift:
            m_jit.rshift32(op1, shiftAmount, result);
            break;
        case BitLShift:
            m_jit.lshift32(op1, shiftAmount, result);
            break;
        case BitURShift:
            m_jit.urshift32(op1, shiftAmount, result);
            break;
        default:
            ASSERT_NOT_REACHED();
        }
    }
    
    // Returns the node index of the branch node if peephole is okay, NoNode otherwise.
    NodeIndex detectPeepHoleBranch()
    {
        NodeIndex lastNodeIndex = m_jit.graph().m_blocks[m_block]->end - 1;

        // Check that no intervening nodes will be generated.
        for (NodeIndex index = m_compileIndex + 1; index < lastNodeIndex; ++index) {
            if (at(index).shouldGenerate())
                return NoNode;
        }

        // Check if the lastNode is a branch on this node.
        Node& lastNode = at(lastNodeIndex);
        return lastNode.op == Branch && lastNode.child1() == m_compileIndex ? lastNodeIndex : NoNode;
    }
    
    void nonSpeculativeValueToNumber(Node&);
    void nonSpeculativeValueToInt32(Node&);
    void nonSpeculativeUInt32ToNumber(Node&);

    void nonSpeculativeKnownConstantArithOp(NodeType op, NodeIndex regChild, NodeIndex immChild, bool commute);
    void nonSpeculativeBasicArithOp(NodeType op, Node&);
    
    // Handles both ValueAdd and ArithAdd.
    void nonSpeculativeAdd(NodeType op, Node& node)
    {
        if (isInt32Constant(node.child1())) {
            nonSpeculativeKnownConstantArithOp(op, node.child2(), node.child1(), true);
            return;
        }
        
        if (isInt32Constant(node.child2())) {
            nonSpeculativeKnownConstantArithOp(op, node.child1(), node.child2(), false);
            return;
        }
        
        nonSpeculativeBasicArithOp(op, node);
    }
    
    void nonSpeculativeArithSub(Node& node)
    {
        if (isInt32Constant(node.child2())) {
            nonSpeculativeKnownConstantArithOp(ArithSub, node.child1(), node.child2(), false);
            return;
        }
        
        nonSpeculativeBasicArithOp(ArithSub, node);
    }
    
    void nonSpeculativeArithMod(Node&);
    void nonSpeculativeCheckHasInstance(Node&);
    void nonSpeculativeInstanceOf(Node&);

#if USE(JSVALUE64)
    JITCompiler::Call cachedGetById(GPRReg baseGPR, GPRReg resultGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), NodeType = GetById);
    void cachedPutById(GPRReg base, GPRReg value, NodeIndex valueIndex, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
    void cachedGetMethod(GPRReg baseGPR, GPRReg resultGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#elif USE(JSVALUE32_64)
    JITCompiler::Call cachedGetById(GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), NodeType = GetById);
    void cachedPutById(GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, NodeIndex valueIndex, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
    void cachedGetMethod(GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#endif

    void nonSpeculativeNonPeepholeCompareNull(NodeIndex operand, bool invert = false);
    void nonSpeculativePeepholeBranchNull(NodeIndex operand, NodeIndex branchNodeIndex, bool invert = false);
    bool nonSpeculativeCompareNull(Node&, NodeIndex operand, bool invert = false);
    
    void nonSpeculativePeepholeBranch(Node&, NodeIndex branchNodeIndex, MacroAssembler::RelationalCondition, Z_DFGOperation_EJJ helperFunction);
    void nonSpeculativeNonPeepholeCompare(Node&, MacroAssembler::RelationalCondition, Z_DFGOperation_EJJ helperFunction);
    bool nonSpeculativeCompare(Node&, MacroAssembler::RelationalCondition, Z_DFGOperation_EJJ helperFunction);
    
    void nonSpeculativePeepholeStrictEq(Node&, NodeIndex branchNodeIndex, bool invert = false);
    void nonSpeculativeNonPeepholeStrictEq(Node&, bool invert = false);
    bool nonSpeculativeStrictEq(Node&, bool invert = false);
    
    MacroAssembler::Address addressOfCallData(int idx)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + idx) * static_cast<int>(sizeof(Register)));
    }

#if USE(JSVALUE32_64)    
    MacroAssembler::Address tagOfCallData(int idx)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + idx) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    MacroAssembler::Address payloadOfCallData(int idx)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + idx) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }
#endif

    void emitCall(Node&);
    
    // Called once a node has completed code generation but prior to setting
    // its result, to free up its children. (This must happen prior to setting
    // the nodes result, since the node may have the same VirtualRegister as
    // a child, and as such will use the same GeneratioInfo).
    void useChildren(Node&);

    // These method called to initialize the the GenerationInfo
    // to describe the result of an operation.
    void integerResult(GPRReg reg, NodeIndex nodeIndex, DataFormat format = DataFormatInteger, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];

        if (format == DataFormatInteger) {
            m_jit.jitAssertIsInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderInteger);
            info.initInteger(nodeIndex, node.refCount(), reg);
        } else {
#if USE(JSVALUE64)
            ASSERT(format == DataFormatJSInteger);
            m_jit.jitAssertIsJSInt32(reg);
            m_gprs.retain(reg, virtualRegister, SpillOrderJS);
            info.initJSValue(nodeIndex, node.refCount(), reg, format);
#elif USE(JSVALUE32_64)
            ASSERT_NOT_REACHED();
#endif
        }
    }
    void integerResult(GPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode)
    {
        integerResult(reg, nodeIndex, DataFormatInteger, mode);
    }
    void noResult(NodeIndex nodeIndex, UseChildrenMode mode = CallUseChildren)
    {
        if (mode == UseChildrenCalledExplicitly)
            return;
        Node& node = at(nodeIndex);
        useChildren(node);
    }
    void cellResult(GPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderCell);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initCell(nodeIndex, node.refCount(), reg);
    }
#if USE(JSVALUE64)
    void jsValueResult(GPRReg reg, NodeIndex nodeIndex, DataFormat format = DataFormatJS, UseChildrenMode mode = CallUseChildren)
    {
        if (format == DataFormatJSInteger)
            m_jit.jitAssertIsJSInt32(reg);
        
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderJS);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initJSValue(nodeIndex, node.refCount(), reg, format);
    }
    void jsValueResult(GPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode)
    {
        jsValueResult(reg, nodeIndex, DataFormatJS, mode);
    }
#elif USE(JSVALUE32_64)
    void jsValueResult(GPRReg tag, GPRReg payload, NodeIndex nodeIndex, DataFormat format = DataFormatJS, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(tag, virtualRegister, SpillOrderJS);
        m_gprs.retain(payload, virtualRegister, SpillOrderJS);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initJSValue(nodeIndex, node.refCount(), tag, payload, format);
    }
    void jsValueResult(GPRReg tag, GPRReg payload, NodeIndex nodeIndex, UseChildrenMode mode)
    {
        jsValueResult(tag, payload, nodeIndex, DataFormatJS, mode);
    }
#endif
    void storageResult(GPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);
        
        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderStorage);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initStorage(nodeIndex, node.refCount(), reg);
    }
    void doubleResult(FPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        m_fprs.retain(reg, virtualRegister, SpillOrderDouble);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initDouble(nodeIndex, node.refCount(), reg);
    }
    void initConstantInfo(NodeIndex nodeIndex)
    {
        ASSERT(isInt32Constant(nodeIndex) || isNumberConstant(nodeIndex) || isJSConstant(nodeIndex));
        Node& node = at(nodeIndex);
        m_generationInfo[node.virtualRegister()].initConstant(nodeIndex, node.refCount());
    }

#if CPU(X86_64)
    // These methods used to sort arguments into the correct registers.
    template<GPRReg destA, GPRReg destB>
    void setupTwoStubArgs(GPRReg srcA, GPRReg srcB)
    {
        // Assuming that srcA != srcB, there are 7 interesting states the registers may be in:
        // (1) both are already in arg regs, the right way around.
        // (2) both are already in arg regs, the wrong way around.
        // (3) neither are currently in arg registers.
        // (4) srcA in in its correct reg.
        // (5) srcA in in the incorrect reg.
        // (6) srcB in in its correct reg.
        // (7) srcB in in the incorrect reg.
        //
        // The trivial approach is to simply emit two moves, to put srcA in place then srcB in
        // place (the MacroAssembler will omit redundant moves). This apporach will be safe in
        // cases 1, 3, 4, 5, 6, and in cases where srcA==srcB. The two problem cases are 2
        // (requires a swap) and 7 (must move srcB first, to avoid trampling.)

        if (srcB != destA) {
            // Handle the easy cases - two simple moves.
            m_jit.move(srcA, destA);
            m_jit.move(srcB, destB);
        } else if (srcA != destB) {
            // Handle the non-swap case - just put srcB in place first.
            m_jit.move(srcB, destB);
            m_jit.move(srcA, destA);
        } else
            m_jit.swap(destA, destB);
    }
    template<FPRReg destA, FPRReg destB>
    void setupTwoStubArgs(FPRReg srcA, FPRReg srcB)
    {
        // Assuming that srcA != srcB, there are 7 interesting states the registers may be in:
        // (1) both are already in arg regs, the right way around.
        // (2) both are already in arg regs, the wrong way around.
        // (3) neither are currently in arg registers.
        // (4) srcA in in its correct reg.
        // (5) srcA in in the incorrect reg.
        // (6) srcB in in its correct reg.
        // (7) srcB in in the incorrect reg.
        //
        // The trivial approach is to simply emit two moves, to put srcA in place then srcB in
        // place (the MacroAssembler will omit redundant moves). This apporach will be safe in
        // cases 1, 3, 4, 5, 6, and in cases where srcA==srcB. The two problem cases are 2
        // (requires a swap) and 7 (must move srcB first, to avoid trampling.)

        if (srcB != destA) {
            // Handle the easy cases - two simple moves.
            m_jit.moveDouble(srcA, destA);
            m_jit.moveDouble(srcB, destB);
            return;
        }
        
        if (srcA != destB) {
            // Handle the non-swap case - just put srcB in place first.
            m_jit.moveDouble(srcB, destB);
            m_jit.moveDouble(srcA, destA);
            return;
        }

        ASSERT(srcB == destA && srcA == destB);
        // Need to swap; pick a temporary register.
        FPRReg temp;
        if (destA != FPRInfo::argumentFPR3 && destA != FPRInfo::argumentFPR3)
            temp = FPRInfo::argumentFPR3;
        else if (destA != FPRInfo::argumentFPR2 && destA != FPRInfo::argumentFPR2)
            temp = FPRInfo::argumentFPR2;
        else {
            ASSERT(destA != FPRInfo::argumentFPR1 && destA != FPRInfo::argumentFPR1);
            temp = FPRInfo::argumentFPR1;
        }
        m_jit.moveDouble(destA, temp);
        m_jit.moveDouble(destB, destA);
        m_jit.moveDouble(temp, destB);
    }
    void setupStubArguments(GPRReg arg1, GPRReg arg2)
    {
        setupTwoStubArgs<GPRInfo::argumentGPR1, GPRInfo::argumentGPR2>(arg1, arg2);
    }
    void setupStubArguments(GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        // If neither of arg2/arg3 are in our way, then we can move arg1 into place.
        // Then we can use setupTwoStubArgs to fix arg2/arg3.
        if (arg2 != GPRInfo::argumentGPR1 && arg3 != GPRInfo::argumentGPR1) {
            m_jit.move(arg1, GPRInfo::argumentGPR1);
            setupTwoStubArgs<GPRInfo::argumentGPR2, GPRInfo::argumentGPR3>(arg2, arg3);
            return;
        }

        // If neither of arg1/arg3 are in our way, then we can move arg2 into place.
        // Then we can use setupTwoStubArgs to fix arg1/arg3.
        if (arg1 != GPRInfo::argumentGPR2 && arg3 != GPRInfo::argumentGPR2) {
            m_jit.move(arg2, GPRInfo::argumentGPR2);
            setupTwoStubArgs<GPRInfo::argumentGPR1, GPRInfo::argumentGPR3>(arg1, arg3);
            return;
        }

        // If neither of arg1/arg2 are in our way, then we can move arg3 into place.
        // Then we can use setupTwoStubArgs to fix arg1/arg2.
        if (arg1 != GPRInfo::argumentGPR3 && arg2 != GPRInfo::argumentGPR3) {
            m_jit.move(arg3, GPRInfo::argumentGPR3);
            setupTwoStubArgs<GPRInfo::argumentGPR1, GPRInfo::argumentGPR2>(arg1, arg2);
            return;
        }

        // If we get here, we haven't been able to move any of arg1/arg2/arg3.
        // Since all three are blocked, then all three must already be in the argument register.
        // But are they in the right ones?

        // First, ensure arg1 is in place.
        if (arg1 != GPRInfo::argumentGPR1) {
            m_jit.swap(arg1, GPRInfo::argumentGPR1);

            // If arg1 wasn't in argumentGPR1, one of arg2/arg3 must be.
            ASSERT(arg2 == GPRInfo::argumentGPR1 || arg3 == GPRInfo::argumentGPR1);
            // If arg2 was in argumentGPR1 it no longer is (due to the swap).
            // Otherwise arg3 must have been. Mark him as moved.
            if (arg2 == GPRInfo::argumentGPR1)
                arg2 = arg1;
            else
                arg3 = arg1;
        }

        // Either arg2 & arg3 need swapping, or we're all done.
        ASSERT((arg2 == GPRInfo::argumentGPR2 || arg3 == GPRInfo::argumentGPR3)
            || (arg2 == GPRInfo::argumentGPR3 || arg3 == GPRInfo::argumentGPR2));

        if (arg2 != GPRInfo::argumentGPR2)
            m_jit.swap(GPRInfo::argumentGPR2, GPRInfo::argumentGPR3);
    }

    // These methods add calls to C++ helper functions.
    void callOperation(J_DFGOperation_EP operation, GPRReg result, void* pointer)
    {
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(J_DFGOperation_EI operation, GPRReg result, Identifier* identifier)
    {
        callOperation((J_DFGOperation_EP)operation, result, identifier);
    }
    void callOperation(J_DFGOperation_EA operation, GPRReg result, GPRReg arg1)
    {
        callOperation((J_DFGOperation_EP)operation, result, arg1);
    }
    void callOperation(J_DFGOperation_EPS operation, GPRReg result, void* pointer, size_t size)
    {
        m_jit.move(JITCompiler::TrustedImmPtr(size), GPRInfo::argumentGPR2);
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(J_DFGOperation_ESS operation, GPRReg result, int startConstant, int numConstants)
    {
        m_jit.move(JITCompiler::TrustedImm32(numConstants), GPRInfo::argumentGPR2);
        m_jit.move(JITCompiler::TrustedImm32(startConstant), GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(J_DFGOperation_EJP operation, GPRReg result, GPRReg arg1, void* pointer)
    {
        m_jit.move(arg1, GPRInfo::argumentGPR1);
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), GPRInfo::argumentGPR2);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(J_DFGOperation_EJI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        callOperation((J_DFGOperation_EJP)operation, result, arg1, identifier);
    }
    void callOperation(J_DFGOperation_EJA operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        callOperation((J_DFGOperation_EJP)operation, result, arg1, arg2);
    }
    // This also handles J_DFGOperation_EP!
    void callOperation(J_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.move(arg1, GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(C_DFGOperation_E operation, GPRReg result)
    {
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(C_DFGOperation_EC operation, GPRReg result, GPRReg arg1)
    {
        m_jit.move(arg1, GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(Z_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.move(arg1, GPRInfo::argumentGPR1);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(Z_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        setupStubArguments(arg1, arg2);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    // This also handles J_DFGOperation_EJP!
    void callOperation(J_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        setupStubArguments(arg1, arg2);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(V_DFGOperation_EJJP operation, GPRReg arg1, GPRReg arg2, void* pointer)
    {
        setupStubArguments(arg1, arg2);
        m_jit.move(JITCompiler::TrustedImmPtr(pointer), GPRInfo::argumentGPR3);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
    }
    void callOperation(V_DFGOperation_EJJI operation, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        callOperation((V_DFGOperation_EJJP)operation, arg1, arg2, identifier);
    }
    void callOperation(V_DFGOperation_EJJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        setupStubArguments(arg1, arg2, arg3);
        m_jit.move(GPRInfo::callFrameRegister, GPRInfo::argumentGPR0);

        appendCallWithExceptionCheck(operation);
    }
    void callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        setupTwoStubArgs<FPRInfo::argumentFPR0, FPRInfo::argumentFPR1>(arg1, arg2);

        m_jit.appendCall(operation);
        m_jit.moveDouble(FPRInfo::returnValueFPR, result);
    }

#elif CPU(X86)

    void setupResults(GPRReg tag, GPRReg payload)
    {
        GPRReg srcA = GPRInfo::returnValueGPR;
        GPRReg srcB = GPRInfo::returnValueGPR2;
        GPRReg destA = payload;
        GPRReg destB = tag;

        if (srcB != destA) {
            // Handle the easy cases - two simple moves.
            m_jit.move(srcA, destA);
            m_jit.move(srcB, destB);
        } else if (srcA != destB) {
            // Handle the non-swap case - just put srcB in place first.
            m_jit.move(srcB, destB);
            m_jit.move(srcA, destA);
        } else
            m_jit.swap(destA, destB);
    }

    // These methods add calls to C++ helper functions.
    void callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, void* pointer)
    {
        m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(pointer)));
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.push(arg1);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_EI operation, GPRReg resultTag, GPRReg resultPayload, Identifier* identifier)
    {
        callOperation((J_DFGOperation_EP)operation, resultTag, resultPayload, identifier);
    }
    void callOperation(J_DFGOperation_EA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        callOperation((J_DFGOperation_EP)operation, resultTag, resultPayload, arg1);
    }
    void callOperation(J_DFGOperation_EPS operation, GPRReg resultTag, GPRReg resultPayload, void* pointer, size_t size)
    {
        m_jit.push(JITCompiler::TrustedImm32(size));
        m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(pointer)));
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_ESS operation, GPRReg resultTag, GPRReg resultPayload, int startConstant, int numConstants)
    {
        m_jit.push(JITCompiler::TrustedImm32(numConstants));
        m_jit.push(JITCompiler::TrustedImm32(startConstant));
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, void* pointer)
    {
        m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(pointer)));
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.push(arg2);
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(J_DFGOperation_EJI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, Identifier* identifier)
    {
        callOperation((J_DFGOperation_EJP)operation, resultTag, resultPayload, arg1Tag, arg1Payload, identifier);
    }
    void callOperation(J_DFGOperation_EJA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        callOperation((J_DFGOperation_EJP)operation, resultTag, resultPayload, arg1Tag, arg1Payload, arg2);
    }
    void callOperation(J_DFGOperation_EJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(C_DFGOperation_E operation, GPRReg result)
    {
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(C_DFGOperation_EC operation, GPRReg result, GPRReg arg1)
    {
        m_jit.push(arg1);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(Z_DFGOperation_EJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(Z_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.push(arg2Tag);
        m_jit.push(arg2Payload);
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        m_jit.move(GPRInfo::returnValueGPR, result);
    }
    void callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.push(arg2Tag);
        m_jit.push(arg2Payload);
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
        setupResults(resultTag, resultPayload);
    }
    void callOperation(V_DFGOperation_EJJP operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload, void* pointer)
    {
        m_jit.push(JITCompiler::TrustedImm32(reinterpret_cast<int>(pointer)));
        m_jit.push(arg2Tag);
        m_jit.push(arg2Payload);
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
    }
    void callOperation(V_DFGOperation_EJJI operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload, Identifier* identifier)
    {
        callOperation((V_DFGOperation_EJJP)operation, arg1Tag, arg1Payload, arg2Tag, arg2Payload, identifier);
    }
    void callOperation(V_DFGOperation_EJJJ operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.push(arg3Tag);
        m_jit.push(arg3Payload);
        m_jit.push(arg2Tag);
        m_jit.push(arg2Payload);
        m_jit.push(arg1Tag);
        m_jit.push(arg1Payload);
        m_jit.push(GPRInfo::callFrameRegister);

        appendCallWithExceptionCheck(operation);
    }

    void callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        m_jit.subPtr(TrustedImm32(2 * sizeof(double)), JITCompiler::stackPointerRegister);
        m_jit.storeDouble(arg2, JITCompiler::Address(JITCompiler::stackPointerRegister, sizeof(double)));
        m_jit.storeDouble(arg1, JITCompiler::stackPointerRegister);

        m_jit.appendCall(operation);
#if !CALLING_CONVENTION_IS_CDECL
        // For D_DFGOperation_DD calls we're currently using the system's default calling convention.
        // On Mac OS the arguments are still on the stack at this point, on Windows they are not.
        // Make other platforms match the Mac here, since we'll need some scratch space for the fstpl, below.
        m_jit.subPtr(TrustedImm32(2 * sizeof(double)), JITCompiler::stackPointerRegister);
#endif
        m_jit.assembler().fstpl(0, JITCompiler::stackPointerRegister);
        m_jit.loadDouble(JITCompiler::stackPointerRegister, result);
        m_jit.addPtr(TrustedImm32(2 * sizeof(double)), JITCompiler::stackPointerRegister);
    }
#endif

    JITCompiler::Call appendCallWithExceptionCheck(const FunctionPtr& function)
    {
        return m_jit.appendCallWithExceptionCheck(function, at(m_compileIndex).codeOrigin);
    }

    void addBranch(const MacroAssembler::Jump& jump, BlockIndex destination)
    {
        m_branches.append(BranchRecord(jump, destination));
    }

    void linkBranches()
    {
        for (size_t i = 0; i < m_branches.size(); ++i) {
            BranchRecord& branch = m_branches[i];
            branch.jump.linkTo(m_blockHeads[branch.destination], &m_jit);
        }
    }

#ifndef NDEBUG
    void dump(const char* label = 0);
#endif

#if ENABLE(DFG_CONSISTENCY_CHECK)
    void checkConsistency();
#else
    void checkConsistency() {}
#endif

    // The JIT, while also provides MacroAssembler functionality.
    JITCompiler& m_jit;
    // The current node being generated.
    BlockIndex m_block;
    NodeIndex m_compileIndex;
    // Virtual and physical register maps.
    Vector<GenerationInfo, 32> m_generationInfo;
    RegisterBank<GPRInfo> m_gprs;
    RegisterBank<FPRInfo> m_fprs;

    Vector<MacroAssembler::Label> m_blockHeads;
    struct BranchRecord {
        BranchRecord(MacroAssembler::Jump jump, BlockIndex destination)
            : jump(jump)
            , destination(destination)
        {
        }

        MacroAssembler::Jump jump;
        BlockIndex destination;
    };
    Vector<BranchRecord, 8> m_branches;
};

// === Operand types ===
//
// IntegerOperand, DoubleOperand and JSValueOperand.
//
// These classes are used to lock the operands to a node into machine
// registers. These classes implement of pattern of locking a value
// into register at the point of construction only if it is already in
// registers, and otherwise loading it lazily at the point it is first
// used. We do so in order to attempt to avoid spilling one operand
// in order to make space available for another.

class IntegerOperand {
public:
    explicit IntegerOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }

    ~IntegerOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    DataFormat format()
    {
        gpr(); // m_format is set when m_gpr is locked.
        ASSERT(m_format == DataFormatInteger || m_format == DataFormatJSInteger);
        return m_format;
    }

    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillInteger(index(), m_format);
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class DoubleOperand {
public:
    explicit DoubleOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_fprOrInvalid(InvalidFPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilledDouble(index))
            fpr();
    }

    ~DoubleOperand()
    {
        ASSERT(m_fprOrInvalid != InvalidFPRReg);
        m_jit->unlock(m_fprOrInvalid);
    }

    NodeIndex index() const
    {
        return m_index;
    }

    FPRReg fpr()
    {
        if (m_fprOrInvalid == InvalidFPRReg)
            m_fprOrInvalid = m_jit->fillDouble(index());
        return m_fprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    FPRReg m_fprOrInvalid;
};

class JSValueOperand {
public:
    explicit JSValueOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
#if USE(JSVALUE64)
        , m_gprOrInvalid(InvalidGPRReg)
#elif USE(JSVALUE32_64)
        , m_isDouble(false)
#endif
    {
        ASSERT(m_jit);
#if USE(JSVALUE64)
        if (jit->isFilled(index))
            gpr();
#elif USE(JSVALUE32_64)
        m_register.pair.tagGPR = InvalidGPRReg;
        m_register.pair.payloadGPR = InvalidGPRReg;
        if (jit->isFilled(index))
            fill();
#endif
    }

    ~JSValueOperand()
    {
#if USE(JSVALUE64)
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
#elif USE(JSVALUE32_64)
        if (m_isDouble) {
            ASSERT(m_register.fpr != InvalidFPRReg);
            m_jit->unlock(m_register.fpr);
        } else {
            ASSERT(m_register.pair.tagGPR != InvalidGPRReg && m_register.pair.payloadGPR != InvalidGPRReg);
            m_jit->unlock(m_register.pair.tagGPR);
            m_jit->unlock(m_register.pair.payloadGPR);
        }
#endif
    }

    NodeIndex index() const
    {
        return m_index;
    }

#if USE(JSVALUE64)
    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillJSValue(index());
        return m_gprOrInvalid;
    }
#elif USE(JSVALUE32_64)
    bool isDouble() { return m_isDouble; }

    void fill()
    {
        if (m_register.pair.tagGPR == InvalidGPRReg && m_register.pair.payloadGPR == InvalidGPRReg)
            m_isDouble = !m_jit->fillJSValue(index(), m_register.pair.tagGPR, m_register.pair.payloadGPR, m_register.fpr);
    }

    GPRReg tagGPR()
    {
        fill();
        ASSERT(!m_isDouble);
        return m_register.pair.tagGPR;
    }

    GPRReg payloadGPR()
    {
        fill();
        ASSERT(!m_isDouble);
        return m_register.pair.payloadGPR;
    }

    FPRReg fpr()
    {
        fill();
        ASSERT(m_isDouble);
        return m_register.fpr;
    }
#endif

    void use()
    {
        m_jit->use(m_index);
    }

private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
#if USE(JSVALUE64)
    GPRReg m_gprOrInvalid;
#elif USE(JSVALUE32_64)
    union {
        struct {
            GPRReg tagGPR;
            GPRReg payloadGPR;
        } pair;
        FPRReg fpr;
    } m_register;
    bool m_isDouble;
#endif
};

class StorageOperand {
public:
    explicit StorageOperand(JITCodeGenerator* jit, NodeIndex index)
        : m_jit(jit)
        , m_index(index)
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        if (jit->isFilled(index))
            gpr();
    }
    
    ~StorageOperand()
    {
        ASSERT(m_gprOrInvalid != InvalidGPRReg);
        m_jit->unlock(m_gprOrInvalid);
    }
    
    NodeIndex index() const
    {
        return m_index;
    }
    
    GPRReg gpr()
    {
        if (m_gprOrInvalid == InvalidGPRReg)
            m_gprOrInvalid = m_jit->fillStorage(index());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }
    
private:
    JITCodeGenerator* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};


// === Temporaries ===
//
// These classes are used to allocate temporary registers.
// A mechanism is provided to attempt to reuse the registers
// currently allocated to child nodes whose value is consumed
// by, and not live after, this operation.

class GPRTemporary {
public:
    GPRTemporary();
    GPRTemporary(JITCodeGenerator*);
    GPRTemporary(JITCodeGenerator*, GPRReg specific);
    GPRTemporary(JITCodeGenerator*, SpeculateIntegerOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateIntegerOperand&, SpeculateIntegerOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateStrictInt32Operand&);
    GPRTemporary(JITCodeGenerator*, IntegerOperand&);
    GPRTemporary(JITCodeGenerator*, IntegerOperand&, IntegerOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateCellOperand&);
    GPRTemporary(JITCodeGenerator*, SpeculateBooleanOperand&);
#if USE(JSVALUE64)
    GPRTemporary(JITCodeGenerator*, JSValueOperand&);
#elif USE(JSVALUE32_64)
    GPRTemporary(JITCodeGenerator*, JSValueOperand&, bool tag = true);
#endif
    GPRTemporary(JITCodeGenerator*, StorageOperand&);

    void adopt(GPRTemporary&);

    ~GPRTemporary()
    {
        if (m_jit && m_gpr != InvalidGPRReg)
            m_jit->unlock(gpr());
    }

    GPRReg gpr()
    {
        // In some cases we have lazy allocation.
        if (m_jit && m_gpr == InvalidGPRReg)
            m_gpr = m_jit->allocate();
        return m_gpr;
    }

private:
    JITCodeGenerator* m_jit;
    GPRReg m_gpr;
};

class FPRTemporary {
public:
    FPRTemporary(JITCodeGenerator*);
    FPRTemporary(JITCodeGenerator*, DoubleOperand&);
    FPRTemporary(JITCodeGenerator*, DoubleOperand&, DoubleOperand&);
    FPRTemporary(JITCodeGenerator*, SpeculateDoubleOperand&);
    FPRTemporary(JITCodeGenerator*, SpeculateDoubleOperand&, SpeculateDoubleOperand&);
#if USE(JSVALUE32_64)
    FPRTemporary(JITCodeGenerator*, JSValueOperand&);
#endif

    ~FPRTemporary()
    {
        m_jit->unlock(fpr());
    }

    FPRReg fpr() const
    {
        ASSERT(m_fpr != InvalidFPRReg);
        return m_fpr;
    }

protected:
    FPRTemporary(JITCodeGenerator* jit, FPRReg lockedFPR)
        : m_jit(jit)
        , m_fpr(lockedFPR)
    {
    }

private:
    JITCodeGenerator* m_jit;
    FPRReg m_fpr;
};


// === Results ===
//
// These classes lock the result of a call to a C++ helper function.

class GPRResult : public GPRTemporary {
public:
    GPRResult(JITCodeGenerator* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR)
    {
    }
};

#if USE(JSVALUE32_64)
class GPRResult2 : public GPRTemporary {
public:
    GPRResult2(JITCodeGenerator* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR2)
    {
    }
};
#endif

class FPRResult : public FPRTemporary {
public:
    FPRResult(JITCodeGenerator* jit)
        : FPRTemporary(jit, lockedResult(jit))
    {
    }

private:
    static FPRReg lockedResult(JITCodeGenerator* jit)
    {
        jit->lock(FPRInfo::returnValueFPR);
        return FPRInfo::returnValueFPR;
    }
};

} } // namespace JSC::DFG

#endif
#endif

