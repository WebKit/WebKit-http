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

#ifndef DFGSpeculativeJIT_h
#define DFGSpeculativeJIT_h

#if ENABLE(DFG_JIT)

#include "DFGAbstractState.h"
#include "DFGGenerationInfo.h"
#include "DFGJITCompiler.h"
#include "DFGOSRExit.h"
#include "DFGOperations.h"
#include "MarkedAllocator.h"
#include "ValueRecovery.h"

namespace JSC { namespace DFG {

class JSValueOperand;
class SpeculativeJIT;
class SpeculateIntegerOperand;
class SpeculateStrictInt32Operand;
class SpeculateDoubleOperand;
class SpeculateCellOperand;
class SpeculateBooleanOperand;


enum ValueSourceKind {
    SourceNotSet,
    ValueInRegisterFile,
    Int32InRegisterFile,
    CellInRegisterFile,
    BooleanInRegisterFile,
    DoubleInRegisterFile,
    SourceIsDead,
    HaveNode
};

class ValueSource {
public:
    ValueSource()
        : m_nodeIndex(nodeIndexFromKind(SourceNotSet))
    {
    }
    
    explicit ValueSource(ValueSourceKind valueSourceKind)
        : m_nodeIndex(nodeIndexFromKind(valueSourceKind))
    {
        ASSERT(kind() != SourceNotSet);
        ASSERT(kind() != HaveNode);
    }
    
    explicit ValueSource(NodeIndex nodeIndex)
        : m_nodeIndex(nodeIndex)
    {
        ASSERT(kind() == HaveNode);
    }
    
    static ValueSource forPrediction(PredictedType prediction)
    {
        if (isInt32Prediction(prediction))
            return ValueSource(Int32InRegisterFile);
        if (isArrayPrediction(prediction))
            return ValueSource(CellInRegisterFile);
        if (isBooleanPrediction(prediction))
            return ValueSource(BooleanInRegisterFile);
        return ValueSource(ValueInRegisterFile);
    }
    
    bool isSet() const
    {
        return kindFromNodeIndex(m_nodeIndex) != SourceNotSet;
    }
    
    ValueSourceKind kind() const
    {
        return kindFromNodeIndex(m_nodeIndex);
    }
    
    NodeIndex nodeIndex() const
    {
        ASSERT(kind() == HaveNode);
        return m_nodeIndex;
    }
    
    void dump(FILE* out) const;
    
private:
    static NodeIndex nodeIndexFromKind(ValueSourceKind kind)
    {
        ASSERT(kind >= SourceNotSet && kind < HaveNode);
        return NoNode - kind;
    }
    
    static ValueSourceKind kindFromNodeIndex(NodeIndex nodeIndex)
    {
        unsigned kind = static_cast<unsigned>(NoNode - nodeIndex);
        if (kind >= static_cast<unsigned>(HaveNode))
            return HaveNode;
        return static_cast<ValueSourceKind>(kind);
    }
    
    NodeIndex m_nodeIndex;
};


enum GeneratedOperandType { GeneratedOperandTypeUnknown, GeneratedOperandInteger, GeneratedOperandDouble, GeneratedOperandJSValue};

// === SpeculativeJIT ===
//
// The SpeculativeJIT is used to generate a fast, but potentially
// incomplete code path for the dataflow. When code generating
// we may make assumptions about operand types, dynamically check,
// and bail-out to an alternate code path if these checks fail.
// Importantly, the speculative code path cannot be reentered once
// a speculative check has failed. This allows the SpeculativeJIT
// to propagate type information (including information that has
// only speculatively been asserted) through the dataflow.
class SpeculativeJIT {
    friend struct OSRExit;
private:
    typedef JITCompiler::TrustedImm32 TrustedImm32;
    typedef JITCompiler::Imm32 Imm32;
    typedef JITCompiler::TrustedImmPtr TrustedImmPtr;
    typedef JITCompiler::ImmPtr ImmPtr;

    // These constants are used to set priorities for spill order for
    // the register allocator.
#if USE(JSVALUE64)
    enum SpillOrder {
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled  = 2, // no spill
        SpillOrderJS       = 4, // needs spill
        SpillOrderCell     = 4, // needs spill
        SpillOrderStorage  = 4, // needs spill
        SpillOrderInteger  = 5, // needs spill and box
        SpillOrderBoolean  = 5, // needs spill and box
        SpillOrderDouble   = 6, // needs spill and convert
    };
#elif USE(JSVALUE32_64)
    enum SpillOrder {
        SpillOrderConstant = 1, // no spill, and cheap fill
        SpillOrderSpilled  = 2, // no spill
        SpillOrderJS       = 4, // needs spill
        SpillOrderStorage  = 4, // needs spill
        SpillOrderDouble   = 4, // needs spill
        SpillOrderInteger  = 5, // needs spill and box
        SpillOrderCell     = 5, // needs spill and box
        SpillOrderBoolean  = 5, // needs spill and box
    };
#endif

    enum UseChildrenMode { CallUseChildren, UseChildrenCalledExplicitly };
    
public:
    SpeculativeJIT(JITCompiler&);

    bool compile();
    void createOSREntries();
    void linkOSREntries(LinkBuffer&);

    Node& at(NodeIndex nodeIndex)
    {
        return m_jit.graph()[nodeIndex];
    }
    Node& at(Edge nodeUse)
    {
        return at(nodeUse.index());
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
    bool canReuse(Edge nodeUse)
    {
        return canReuse(nodeUse.index());
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
        else if (registerFormat & DataFormatJS) {
            m_gprs.release(info.tagGPR());
            m_gprs.release(info.payloadGPR());
        } else if (registerFormat != DataFormatNone)
            m_gprs.release(info.gpr());
#endif
    }
    void use(Edge nodeUse)
    {
        use(nodeUse.index());
    }

    static void markCellCard(MacroAssembler&, GPRReg ownerGPR, GPRReg scratchGPR1, GPRReg scratchGPR2);
    static void writeBarrier(MacroAssembler&, GPRReg ownerGPR, GPRReg scratchGPR1, GPRReg scratchGPR2, WriteBarrierUseKind);

    void writeBarrier(GPRReg ownerGPR, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(GPRReg ownerGPR, JSCell* value, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg, GPRReg scratchGPR2 = InvalidGPRReg);
    void writeBarrier(JSCell* owner, GPRReg valueGPR, Edge valueUse, WriteBarrierUseKind, GPRReg scratchGPR1 = InvalidGPRReg);

    static GPRReg selectScratchGPR(GPRReg preserve1 = InvalidGPRReg, GPRReg preserve2 = InvalidGPRReg, GPRReg preserve3 = InvalidGPRReg, GPRReg preserve4 = InvalidGPRReg)
    {
        return AssemblyHelpers::selectScratchGPR(preserve1, preserve2, preserve3, preserve4);
    }

    // Called by the speculative operand types, below, to fill operand to
    // machine registers, implicitly generating speculation checks as needed.
    GPRReg fillSpeculateInt(NodeIndex, DataFormat& returnFormat);
    GPRReg fillSpeculateIntStrict(NodeIndex);
    FPRReg fillSpeculateDouble(NodeIndex);
    GPRReg fillSpeculateCell(NodeIndex);
    GPRReg fillSpeculateBoolean(NodeIndex);
    GeneratedOperandType checkGeneratedTypeForToInt32(NodeIndex);

private:
    void compile(Node&);
    void compileMovHint(Node&);
    void compile(BasicBlock&);

    void checkArgumentTypes();

    void clearGenerationInfo();

    // These methods are used when generating 'unexpected'
    // calls out from JIT code to C++ helper routines -
    // they spill all live values to the appropriate
    // slots in the RegisterFile without changing any state
    // in the GenerationInfo.
    void silentSpillGPR(VirtualRegister spillMe, GPRReg source)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        ASSERT(info.registerFormat() != DataFormatNone);
        ASSERT(info.registerFormat() != DataFormatDouble);

        if (!info.needsSpill())
            return;

        DataFormat registerFormat = info.registerFormat();

#if USE(JSVALUE64)
        ASSERT(info.gpr() == source);
        if (registerFormat == DataFormatInteger)
            m_jit.store32(source, JITCompiler::addressFor(spillMe));
        else {
            ASSERT(registerFormat & DataFormatJS || registerFormat == DataFormatCell || registerFormat == DataFormatStorage);
            m_jit.storePtr(source, JITCompiler::addressFor(spillMe));
        }
#elif USE(JSVALUE32_64)
        if (registerFormat & DataFormatJS) {
            ASSERT(info.tagGPR() == source || info.payloadGPR() == source);
            m_jit.store32(source, source == info.tagGPR() ? JITCompiler::tagFor(spillMe) : JITCompiler::payloadFor(spillMe));
        } else {
            ASSERT(info.gpr() == source);
            m_jit.store32(source, JITCompiler::payloadFor(spillMe));
        }
#endif
    }
    void silentSpillFPR(VirtualRegister spillMe, FPRReg source)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        ASSERT(info.registerFormat() == DataFormatDouble);

        if (!info.needsSpill()) {
            // it's either a constant or it's already been spilled
            ASSERT(at(info.nodeIndex()).hasConstant() || info.spillFormat() != DataFormatNone);
            return;
        }
        
        // it's neither a constant nor has it been spilled.
        ASSERT(!at(info.nodeIndex()).hasConstant());
        ASSERT(info.spillFormat() == DataFormatNone);
        ASSERT(info.fpr() == source);

        m_jit.storeDouble(source, JITCompiler::addressFor(spillMe));
    }

    void silentFillGPR(VirtualRegister spillMe, GPRReg target)
    {
        GenerationInfo& info = m_generationInfo[spillMe];

        NodeIndex nodeIndex = info.nodeIndex();
        Node& node = at(nodeIndex);
        ASSERT(info.registerFormat() != DataFormatNone);
        ASSERT(info.registerFormat() != DataFormatDouble);
        DataFormat registerFormat = info.registerFormat();

        if (registerFormat == DataFormatInteger) {
            ASSERT(info.gpr() == target);
            ASSERT(isJSInteger(info.registerFormat()));
            if (node.hasConstant()) {
                ASSERT(isInt32Constant(nodeIndex));
                m_jit.move(Imm32(valueOfInt32Constant(nodeIndex)), target);
            } else
                m_jit.load32(JITCompiler::payloadFor(spillMe), target);
            return;
        }
        
        if (registerFormat == DataFormatBoolean) {
#if USE(JSVALUE64)
            ASSERT_NOT_REACHED();
#elif USE(JSVALUE32_64)
            ASSERT(info.gpr() == target);
            if (node.hasConstant()) {
                ASSERT(isBooleanConstant(nodeIndex));
                m_jit.move(TrustedImm32(valueOfBooleanConstant(nodeIndex)), target);
            } else
                m_jit.load32(JITCompiler::payloadFor(spillMe), target);
#endif
            return;
        }

        if (registerFormat == DataFormatCell) {
            ASSERT(info.gpr() == target);
            if (node.hasConstant()) {
                JSValue value = valueOfJSConstant(nodeIndex);
                ASSERT(value.isCell());
                m_jit.move(TrustedImmPtr(value.asCell()), target);
            } else
                m_jit.loadPtr(JITCompiler::payloadFor(spillMe), target);
            return;
        }

        if (registerFormat == DataFormatStorage) {
            ASSERT(info.gpr() == target);
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), target);
            return;
        }

        ASSERT(registerFormat & DataFormatJS);
#if USE(JSVALUE64)
        ASSERT(info.gpr() == target);
        if (node.hasConstant()) {
            if (valueOfJSConstant(nodeIndex).isCell())
                m_jit.move(valueOfJSConstantAsImmPtr(nodeIndex).asTrustedImmPtr(), target);
            else
                m_jit.move(valueOfJSConstantAsImmPtr(nodeIndex), target);
        } else if (info.spillFormat() == DataFormatInteger) {
            ASSERT(registerFormat == DataFormatJSInteger);
            m_jit.load32(JITCompiler::payloadFor(spillMe), target);
            m_jit.orPtr(GPRInfo::tagTypeNumberRegister, target);
        } else if (info.spillFormat() == DataFormatDouble) {
            ASSERT(registerFormat == DataFormatJSDouble);
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), target);
            m_jit.subPtr(GPRInfo::tagTypeNumberRegister, target);
        } else
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), target);
#else
        ASSERT(info.tagGPR() == target || info.payloadGPR() == target);
        if (node.hasConstant()) {
            JSValue v = valueOfJSConstant(nodeIndex);
            m_jit.move(info.tagGPR() == target ? Imm32(v.tag()) : Imm32(v.payload()), target);
        } else if (info.payloadGPR() == target)
            m_jit.load32(JITCompiler::payloadFor(spillMe), target);
        else { // Fill the Tag
            switch (info.spillFormat()) {
            case DataFormatInteger:
                ASSERT(registerFormat == DataFormatJSInteger);
                m_jit.move(TrustedImm32(JSValue::Int32Tag), target);
                break;
            case DataFormatCell:
                ASSERT(registerFormat == DataFormatJSCell);
                m_jit.move(TrustedImm32(JSValue::CellTag), target);
                break;
            case DataFormatBoolean:
                ASSERT(registerFormat == DataFormatJSBoolean);
                m_jit.move(TrustedImm32(JSValue::BooleanTag), target);
                break;
            default:
                m_jit.load32(JITCompiler::tagFor(spillMe), target);
                break;
            }
        }
#endif
    }

    void silentFillFPR(VirtualRegister spillMe, GPRReg canTrample, FPRReg target)
    {
        GenerationInfo& info = m_generationInfo[spillMe];
        ASSERT(info.fpr() == target);

        NodeIndex nodeIndex = info.nodeIndex();
        Node& node = at(nodeIndex);
#if USE(JSVALUE64)
        ASSERT(info.registerFormat() == DataFormatDouble);

        if (node.hasConstant()) {
            ASSERT(isNumberConstant(nodeIndex));
            m_jit.move(ImmPtr(bitwise_cast<void*>(valueOfNumberConstant(nodeIndex))), canTrample);
            m_jit.movePtrToDouble(canTrample, target);
            return;
        }
        
        if (info.spillFormat() != DataFormatNone && info.spillFormat() != DataFormatDouble) {
            // it was already spilled previously and not as a double, which means we need unboxing.
            ASSERT(info.spillFormat() & DataFormatJS);
            m_jit.loadPtr(JITCompiler::addressFor(spillMe), canTrample);
            unboxDouble(canTrample, target);
            return;
        }

        m_jit.loadDouble(JITCompiler::addressFor(spillMe), target);
#elif USE(JSVALUE32_64)
        UNUSED_PARAM(canTrample);
        ASSERT(info.registerFormat() == DataFormatDouble || info.registerFormat() == DataFormatJSDouble);
        if (node.hasConstant()) {
            ASSERT(isNumberConstant(nodeIndex));
            m_jit.loadDouble(addressOfDoubleConstant(nodeIndex), target);
        } else
            m_jit.loadDouble(JITCompiler::addressFor(spillMe), target);
#endif
    }

    void silentSpillAllRegisters(GPRReg exclude, GPRReg exclude2 = InvalidGPRReg)
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            GPRReg gpr = iter.regID();
            if (iter.name() != InvalidVirtualRegister && gpr != exclude && gpr != exclude2)
                silentSpillGPR(iter.name(), gpr);
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillFPR(iter.name(), iter.regID());
        }
    }
    void silentSpillAllRegisters(FPRReg exclude)
    {
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentSpillGPR(iter.name(), iter.regID());
        }
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            FPRReg fpr = iter.regID();
            if (iter.name() != InvalidVirtualRegister && fpr != exclude)
                silentSpillFPR(iter.name(), fpr);
        }
    }

    void silentFillAllRegisters(GPRReg exclude, GPRReg exclude2 = InvalidGPRReg)
    {
        GPRReg canTrample = GPRInfo::regT0;
        if (exclude == GPRInfo::regT0)
            canTrample = GPRInfo::regT1;
        
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentFillFPR(iter.name(), canTrample, iter.regID());
        }
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            GPRReg gpr = iter.regID();
            if (iter.name() != InvalidVirtualRegister && gpr != exclude && gpr != exclude2)
                silentFillGPR(iter.name(), gpr);
        }
    }
    void silentFillAllRegisters(FPRReg exclude)
    {
        GPRReg canTrample = GPRInfo::regT0;
        
        for (fpr_iterator iter = m_fprs.begin(); iter != m_fprs.end(); ++iter) {
            FPRReg fpr = iter.regID();
            if (iter.name() != InvalidVirtualRegister && fpr != exclude)
                silentFillFPR(iter.name(), canTrample, fpr);
        }
        for (gpr_iterator iter = m_gprs.begin(); iter != m_gprs.end(); ++iter) {
            if (iter.name() != InvalidVirtualRegister)
                silentFillGPR(iter.name(), iter.regID());
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
    void boxDouble(FPRReg fpr, GPRReg tagGPR, GPRReg payloadGPR)
    {
        m_jit.boxDouble(fpr, tagGPR, payloadGPR);
    }
    void unboxDouble(GPRReg tagGPR, GPRReg payloadGPR, FPRReg fpr, FPRReg scratchFPR)
    {
        m_jit.unboxDouble(tagGPR, payloadGPR, fpr, scratchFPR);
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

        case DataFormatInteger: {
            m_jit.store32(info.gpr(), JITCompiler::payloadFor(spillMe));
            info.spill(DataFormatInteger);
            return;
        }

#if USE(JSVALUE64)
        case DataFormatDouble: {
            m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
            info.spill(DataFormatDouble);
            return;
        }
            
        default:
            // The following code handles JSValues, int32s, and cells.
            ASSERT(spillFormat == DataFormatCell || spillFormat & DataFormatJS);
            
            GPRReg reg = info.gpr();
            // We need to box int32 and cell values ...
            // but on JSVALUE64 boxing a cell is a no-op!
            if (spillFormat == DataFormatInteger)
                m_jit.orPtr(GPRInfo::tagTypeNumberRegister, reg);
            
            // Spill the value, and record it as spilled in its boxed form.
            m_jit.storePtr(reg, JITCompiler::addressFor(spillMe));
            info.spill((DataFormat)(spillFormat | DataFormatJS));
            return;
#elif USE(JSVALUE32_64)
        case DataFormatCell:
        case DataFormatBoolean: {
            m_jit.store32(info.gpr(), JITCompiler::payloadFor(spillMe));
            info.spill(spillFormat);
            return;
        }

        case DataFormatDouble:
        case DataFormatJSDouble: {
            // On JSVALUE32_64 boxing a double is a no-op.
            m_jit.storeDouble(info.fpr(), JITCompiler::addressFor(spillMe));
            info.spill(DataFormatJSDouble);
            return;
        }

        default:
            // The following code handles JSValues.
            ASSERT(spillFormat & DataFormatJS);
            m_jit.store32(info.tagGPR(), JITCompiler::tagFor(spillMe));
            m_jit.store32(info.payloadGPR(), JITCompiler::payloadFor(spillMe));
            info.spill(spillFormat);
            return;
#endif
        }
    }
    
    bool isStrictInt32(NodeIndex);
    
    bool isKnownInteger(NodeIndex);
    bool isKnownNumeric(NodeIndex);
    bool isKnownCell(NodeIndex);
    
    bool isKnownNotInteger(NodeIndex);
    bool isKnownNotNumber(NodeIndex);

    bool isKnownNotCell(NodeIndex);
    
    // Checks/accessors for constant values.
    bool isConstant(NodeIndex nodeIndex) { return m_jit.graph().isConstant(nodeIndex); }
    bool isJSConstant(NodeIndex nodeIndex) { return m_jit.graph().isJSConstant(nodeIndex); }
    bool isInt32Constant(NodeIndex nodeIndex) { return m_jit.graph().isInt32Constant(nodeIndex); }
    bool isDoubleConstant(NodeIndex nodeIndex) { return m_jit.graph().isDoubleConstant(nodeIndex); }
    bool isNumberConstant(NodeIndex nodeIndex) { return m_jit.graph().isNumberConstant(nodeIndex); }
    bool isBooleanConstant(NodeIndex nodeIndex) { return m_jit.graph().isBooleanConstant(nodeIndex); }
    bool isFunctionConstant(NodeIndex nodeIndex) { return m_jit.graph().isFunctionConstant(nodeIndex); }
    int32_t valueOfInt32Constant(NodeIndex nodeIndex) { return m_jit.graph().valueOfInt32Constant(nodeIndex); }
    double valueOfNumberConstant(NodeIndex nodeIndex) { return m_jit.graph().valueOfNumberConstant(nodeIndex); }
    int32_t valueOfNumberConstantAsInt32(NodeIndex nodeIndex)
    {
        if (isInt32Constant(nodeIndex))
            return valueOfInt32Constant(nodeIndex);
        return JSC::toInt32(valueOfNumberConstant(nodeIndex));
    }
#if USE(JSVALUE32_64)
    void* addressOfDoubleConstant(NodeIndex nodeIndex) { return m_jit.addressOfDoubleConstant(nodeIndex); }
#endif
    JSValue valueOfJSConstant(NodeIndex nodeIndex) { return m_jit.graph().valueOfJSConstant(nodeIndex); }
    bool valueOfBooleanConstant(NodeIndex nodeIndex) { return m_jit.graph().valueOfBooleanConstant(nodeIndex); }
    JSFunction* valueOfFunctionConstant(NodeIndex nodeIndex) { return m_jit.graph().valueOfFunctionConstant(nodeIndex); }
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
    
    // Returns the index of the branch node if peephole is okay, UINT_MAX otherwise.
    unsigned detectPeepHoleBranch()
    {
        BasicBlock* block = m_jit.graph().m_blocks[m_block].get();

        // Check that no intervening nodes will be generated.
        for (unsigned index = m_indexInBlock + 1; index < block->size() - 1; ++index) {
            NodeIndex nodeIndex = block->at(index);
            if (at(nodeIndex).shouldGenerate())
                return UINT_MAX;
        }

        // Check if the lastNode is a branch on this node.
        Node& lastNode = at(block->last());
        return lastNode.op() == Branch && lastNode.child1().index() == m_compileIndex ? block->size() - 1 : UINT_MAX;
    }
    
    void nonSpeculativeValueToNumber(Node&);
    void nonSpeculativeValueToInt32(Node&);
    void nonSpeculativeUInt32ToNumber(Node&);

    enum SpillRegistersMode { NeedToSpill, DontSpill };
#if USE(JSVALUE64)
    JITCompiler::Call cachedGetById(CodeOrigin, GPRReg baseGPR, GPRReg resultGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), SpillRegistersMode = NeedToSpill);
    void cachedPutById(CodeOrigin, GPRReg base, GPRReg value, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#elif USE(JSVALUE32_64)
    JITCompiler::Call cachedGetById(CodeOrigin, GPRReg baseTagGPROrNone, GPRReg basePayloadGPR, GPRReg resultTagGPR, GPRReg resultPayloadGPR, GPRReg scratchGPR, unsigned identifierNumber, JITCompiler::Jump slowPathTarget = JITCompiler::Jump(), SpillRegistersMode = NeedToSpill);
    void cachedPutById(CodeOrigin, GPRReg basePayloadGPR, GPRReg valueTagGPR, GPRReg valuePayloadGPR, Edge valueUse, GPRReg scratchGPR, unsigned identifierNumber, PutKind, JITCompiler::Jump slowPathTarget = JITCompiler::Jump());
#endif

    void nonSpeculativeNonPeepholeCompareNull(Edge operand, bool invert = false);
    void nonSpeculativePeepholeBranchNull(Edge operand, NodeIndex branchNodeIndex, bool invert = false);
    bool nonSpeculativeCompareNull(Node&, Edge operand, bool invert = false);
    
    void nonSpeculativePeepholeBranch(Node&, NodeIndex branchNodeIndex, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    void nonSpeculativeNonPeepholeCompare(Node&, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    bool nonSpeculativeCompare(Node&, MacroAssembler::RelationalCondition, S_DFGOperation_EJJ helperFunction);
    
    void nonSpeculativePeepholeStrictEq(Node&, NodeIndex branchNodeIndex, bool invert = false);
    void nonSpeculativeNonPeepholeStrictEq(Node&, bool invert = false);
    bool nonSpeculativeStrictEq(Node&, bool invert = false);
    
    void compileInstanceOfForObject(Node&, GPRReg valueReg, GPRReg prototypeReg, GPRReg scratchAndResultReg);
    void compileInstanceOf(Node&);
    
    // Access to our fixed callee CallFrame.
    MacroAssembler::Address callFrameSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)));
    }

    // Access to our fixed callee CallFrame.
    MacroAssembler::Address argumentSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)));
    }

    MacroAssembler::Address callFrameTagSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    MacroAssembler::Address callFramePayloadSlot(int slot)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + slot) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }

    MacroAssembler::Address argumentTagSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag));
    }

    MacroAssembler::Address argumentPayloadSlot(int argument)
    {
        return MacroAssembler::Address(GPRInfo::callFrameRegister, (m_jit.codeBlock()->m_numCalleeRegisters + argumentToOperand(argument)) * static_cast<int>(sizeof(Register)) + OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload));
    }

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
    void booleanResult(GPRReg reg, NodeIndex nodeIndex, UseChildrenMode mode = CallUseChildren)
    {
        Node& node = at(nodeIndex);
        if (mode == CallUseChildren)
            useChildren(node);

        VirtualRegister virtualRegister = node.virtualRegister();
        m_gprs.retain(reg, virtualRegister, SpillOrderBoolean);
        GenerationInfo& info = m_generationInfo[virtualRegister];
        info.initBoolean(nodeIndex, node.refCount(), reg);
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
    
    // These methods add calls to C++ helper functions.
    // These methods are broadly value representation specific (i.e.
    // deal with the fact that a JSValue may be passed in one or two
    // machine registers, and delegate the calling convention specific
    // decision as to how to fill the regsiters to setupArguments* methods.
#if USE(JSVALUE64)
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg result, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(Z_DFGOperation_D operation, GPRReg result, FPRReg arg1)
    {
        m_jit.setupArguments(arg1);
        JITCompiler::Call call = m_jit.appendCall(operation);
        m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call callOperation(J_DFGOperation_EGI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EI operation, GPRReg result, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EA operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPS operation, GPRReg result, void* pointer, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ESS operation, GPRReg result, int startConstant, int numConstants)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(startConstant), TrustedImm32(numConstants));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg result, GPRReg arg1, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg result, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJA operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_E operation, GPRReg result)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_J operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArguments(arg1);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJ operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1, MacroAssembler::TrustedImm32 imm)
    {
        m_jit.setupArgumentsWithExecState(arg1, MacroAssembler::TrustedImmPtr(static_cast<const void*>(JSValue::encode(jsNumber(imm.m_value)))));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg result, MacroAssembler::TrustedImm32 imm, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(MacroAssembler::TrustedImmPtr(static_cast<const void*>(JSValue::encode(jsNumber(imm.m_value)))), arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECJ operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EC operation, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJPP operation, GPRReg arg1, GPRReg arg2, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJCI operation, GPRReg arg1, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EPZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EAZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_ECJJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(D_DFGOperation_EJ operation, FPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(D_DFGOperation_ZZ operation, FPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
#else
    JITCompiler::Call callOperation(Z_DFGOperation_D operation, GPRReg result, FPRReg arg1)
    {
        prepareForExternalCall();
        m_jit.setupArguments(arg1);
        JITCompiler::Call call = m_jit.appendCall(operation);
        m_jit.zeroExtend32ToPtr(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EGI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EI operation, GPRReg resultTag, GPRReg resultPayload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EPS operation, GPRReg resultTag, GPRReg resultPayload, void* pointer, size_t size)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(pointer), TrustedImmPtr(size));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ESS operation, GPRReg resultTag, GPRReg resultPayload, int startConstant, int numConstants)
    {
        m_jit.setupArgumentsWithExecState(TrustedImm32(startConstant), TrustedImm32(numConstants));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJP operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJI operation, GPRReg resultTag, GPRReg resultPayload, int32_t arg1Tag, GPRReg arg1Payload, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, TrustedImm32(arg1Tag), TrustedImmPtr(identifier));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJA operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(C_DFGOperation_E operation, GPRReg result)
    {
        m_jit.setupArgumentsExecState();
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_EC operation, GPRReg result, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(C_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, JSCell* cell)
    {
        m_jit.setupArgumentsWithExecState(arg1, TrustedImmPtr(cell));
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_J operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArguments(arg1Payload, arg1Tag);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_ECC operation, GPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(S_DFGOperation_EJJ operation, GPRReg result, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1Tag, GPRReg arg1Payload, MacroAssembler::TrustedImm32 imm)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, imm, TrustedImm32(JSValue::Int32Tag));
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_EJJ operation, GPRReg resultTag, GPRReg resultPayload, MacroAssembler::TrustedImm32 imm, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(imm, TrustedImm32(JSValue::Int32Tag), arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECJ operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2Tag, GPRReg arg2Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2Payload, arg2Tag);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(J_DFGOperation_ECC operation, GPRReg resultTag, GPRReg resultPayload, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2);
        return appendCallWithExceptionCheckSetResult(operation, resultPayload, resultTag);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EC operation, GPRReg arg1)
    {
        m_jit.setupArgumentsWithExecState(arg1);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJPP operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2, void* pointer)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2, TrustedImmPtr(pointer));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EJCI operation, GPRReg arg1Tag, GPRReg arg1Payload, GPRReg arg2, Identifier* identifier)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag, arg2, TrustedImmPtr(identifier));
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_ECJJ operation, GPRReg arg1, GPRReg arg2Tag, GPRReg arg2Payload, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2Payload, arg2Tag, arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EPZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }
    JITCompiler::Call callOperation(V_DFGOperation_EAZJ operation, GPRReg arg1, GPRReg arg2, GPRReg arg3Tag, GPRReg arg3Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1, arg2, arg3Payload, arg3Tag);
        return appendCallWithExceptionCheck(operation);
    }

    JITCompiler::Call callOperation(D_DFGOperation_EJ operation, FPRReg result, GPRReg arg1Tag, GPRReg arg1Payload)
    {
        m_jit.setupArgumentsWithExecState(arg1Payload, arg1Tag);
        return appendCallWithExceptionCheckSetResult(operation, result);
    }

    JITCompiler::Call callOperation(D_DFGOperation_ZZ operation, FPRReg result, GPRReg arg1, GPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
    JITCompiler::Call callOperation(D_DFGOperation_DD operation, FPRReg result, FPRReg arg1, FPRReg arg2)
    {
        m_jit.setupArguments(arg1, arg2);
        return appendCallSetResult(operation, result);
    }
#endif
    
#ifndef NDEBUG
    void prepareForExternalCall()
    {
        for (unsigned i = 0; i < sizeof(void*) / 4; i++)
            m_jit.store32(TrustedImm32(0xbadbeef), reinterpret_cast<char*>(&m_jit.globalData()->topCallFrame) + i * 4);
    }
#else
    void prepareForExternalCall() { }
#endif

    // These methods add call instructions, with optional exception checks & setting results.
    JITCompiler::Call appendCallWithExceptionCheck(const FunctionPtr& function)
    {
        prepareForExternalCall();
        CodeOrigin codeOrigin = at(m_compileIndex).codeOrigin;
        CallBeginToken token = m_jit.beginCall();
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.addExceptionCheck(call, codeOrigin, token);
        return call;
    }
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, GPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.move(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, GPRReg result)
    {
        prepareForExternalCall();
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.move(GPRInfo::returnValueGPR, result);
        return call;
    }
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, GPRReg result1, GPRReg result2)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.setupResults(result1, result2);
        return call;
    }
#if CPU(X86)
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.assembler().fstpl(0, JITCompiler::stackPointerRegister);
        m_jit.loadDouble(JITCompiler::stackPointerRegister, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.assembler().fstpl(0, JITCompiler::stackPointerRegister);
        m_jit.loadDouble(JITCompiler::stackPointerRegister, result);
        return call;
    }
#elif CPU(ARM)
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.assembler().vmov(result, GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.assembler().vmov(result, GPRInfo::returnValueGPR, GPRInfo::returnValueGPR2);
        return call;
    }
#else
    JITCompiler::Call appendCallWithExceptionCheckSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = appendCallWithExceptionCheck(function);
        m_jit.moveDouble(FPRInfo::returnValueFPR, result);
        return call;
    }
    JITCompiler::Call appendCallSetResult(const FunctionPtr& function, FPRReg result)
    {
        JITCompiler::Call call = m_jit.appendCall(function);
        m_jit.moveDouble(FPRInfo::returnValueFPR, result);
        return call;
    }
#endif
    
    void branchDouble(JITCompiler::DoubleCondition cond, FPRReg left, FPRReg right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchDouble(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchDouble(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    void branchDoubleNonZero(FPRReg value, FPRReg scratch, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchDoubleNonZero(value, scratch), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchDoubleZeroOrNaN(value, scratch);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branch32(JITCompiler::RelationalCondition cond, T left, U right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branch32(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branch32(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTest32(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest32(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest32(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTest32(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest32(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest32(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchPtr(JITCompiler::RelationalCondition cond, T left, U right, BlockIndex destination)
    {
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchPtr(cond, left, right), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchPtr(JITCompiler::invert(cond), left, right);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTestPtr(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTestPtr(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTestPtr(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTestPtr(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTestPtr(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTestPtr(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T, typename U>
    void branchTest8(JITCompiler::ResultCondition cond, T value, U mask, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest8(cond, value, mask), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest8(JITCompiler::invert(cond), value, mask);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    template<typename T>
    void branchTest8(JITCompiler::ResultCondition cond, T value, BlockIndex destination)
    {
        ASSERT(JITCompiler::isInvertible(cond));
        
        if (!haveEdgeCodeToEmit(destination))
            return addBranch(m_jit.branchTest8(cond, value), destination);
        
        JITCompiler::Jump notTaken = m_jit.branchTest8(JITCompiler::invert(cond), value);
        emitEdgeCode(destination);
        addBranch(m_jit.jump(), destination);
        notTaken.link(&m_jit);
    }
    
    enum FallThroughMode {
        AtFallThroughPoint,
        ForceJump
    };
    void jump(BlockIndex destination, FallThroughMode fallThroughMode = AtFallThroughPoint)
    {
        if (haveEdgeCodeToEmit(destination))
            emitEdgeCode(destination);
        if (destination == m_block + 1
            && fallThroughMode == AtFallThroughPoint)
            return;
        addBranch(m_jit.jump(), destination);
    }
    
    inline bool haveEdgeCodeToEmit(BlockIndex)
    {
        return DFG_ENABLE_EDGE_CODE_VERIFICATION;
    }
    void emitEdgeCode(BlockIndex destination)
    {
        if (!DFG_ENABLE_EDGE_CODE_VERIFICATION)
            return;
        m_jit.move(TrustedImm32(destination), GPRInfo::regT0);
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

    BasicBlock* block()
    {
        return m_jit.graph().m_blocks[m_block].get();
    }

#ifndef NDEBUG
    void dump(const char* label = 0);
#endif

#if DFG_ENABLE(CONSISTENCY_CHECK)
    void checkConsistency();
#else
    void checkConsistency() { }
#endif

    bool isInteger(NodeIndex nodeIndex)
    {
        Node& node = at(nodeIndex);
        if (node.hasInt32Result())
            return true;

        if (isInt32Constant(nodeIndex))
            return true;

        VirtualRegister virtualRegister = node.virtualRegister();
        GenerationInfo& info = m_generationInfo[virtualRegister];
        
        return info.isJSInteger();
    }
    
    bool compare(Node&, MacroAssembler::RelationalCondition, MacroAssembler::DoubleCondition, S_DFGOperation_EJJ);
    bool compilePeepHoleBranch(Node&, MacroAssembler::RelationalCondition, MacroAssembler::DoubleCondition, S_DFGOperation_EJJ);
    void compilePeepHoleIntegerBranch(Node&, NodeIndex branchNodeIndex, JITCompiler::RelationalCondition);
    void compilePeepHoleDoubleBranch(Node&, NodeIndex branchNodeIndex, JITCompiler::DoubleCondition);
    void compilePeepHoleObjectEquality(Node&, NodeIndex branchNodeIndex, const ClassInfo*, PredictionChecker);
    void compilePeepHoleObjectToObjectOrOtherEquality(
        Edge leftChild, Edge rightChild, NodeIndex branchNodeIndex, const ClassInfo*, PredictionChecker);
    void compileObjectEquality(Node&, const ClassInfo*, PredictionChecker);
    void compileObjectToObjectOrOtherEquality(
        Edge leftChild, Edge rightChild, const ClassInfo*, PredictionChecker);
    void compileValueAdd(Node&);
    void compileObjectOrOtherLogicalNot(Edge value, const ClassInfo*, bool needSpeculationCheck);
    void compileLogicalNot(Node&);
    void emitObjectOrOtherBranch(Edge value, BlockIndex taken, BlockIndex notTaken, const ClassInfo*, bool needSpeculationCheck);
    void emitBranch(Node&);
    
    void compileIntegerCompare(Node&, MacroAssembler::RelationalCondition);
    void compileDoubleCompare(Node&, MacroAssembler::DoubleCondition);
    
    bool compileStrictEqForConstant(Node&, Edge value, JSValue constant);
    
    bool compileStrictEq(Node&);
    
    void compileGetCharCodeAt(Node&);
    void compileGetByValOnString(Node&);
    void compileValueToInt32(Node&);
    void compileUInt32ToNumber(Node&);
    void compileDoubleAsInt32(Node&);
    void compileInt32ToDouble(Node&);
    void compileAdd(Node&);
    void compileArithSub(Node&);
    void compileArithNegate(Node&);
    void compileArithMul(Node&);
#if CPU(X86) || CPU(X86_64)
    void compileIntegerArithDivForX86(Node&);
#endif
    void compileArithMod(Node&);
    void compileSoftModulo(Node&);
    void compileGetTypedArrayLength(const TypedArrayDescriptor&, Node&, bool needsSpeculationCheck);
    enum TypedArraySpeculationRequirements {
        NoTypedArraySpecCheck,
        NoTypedArrayTypeSpecCheck,
        AllTypedArraySpecChecks
    };
    enum TypedArraySignedness {
        SignedTypedArray,
        UnsignedTypedArray
    };
    enum TypedArrayRounding {
        TruncateRounding,
        ClampRounding
    };
    void compileGetIndexedPropertyStorage(Node&);
    void compileGetByValOnIntTypedArray(const TypedArrayDescriptor&, Node&, size_t elementSize, TypedArraySpeculationRequirements, TypedArraySignedness);
    void compilePutByValForIntTypedArray(const TypedArrayDescriptor&, GPRReg base, GPRReg property, Node&, size_t elementSize, TypedArraySpeculationRequirements, TypedArraySignedness, TypedArrayRounding = TruncateRounding);
    void compileGetByValOnFloatTypedArray(const TypedArrayDescriptor&, Node&, size_t elementSize, TypedArraySpeculationRequirements);
    void compilePutByValForFloatTypedArray(const TypedArrayDescriptor&, GPRReg base, GPRReg property, Node&, size_t elementSize, TypedArraySpeculationRequirements);
    void compileNewFunctionNoCheck(Node&);
    void compileNewFunctionExpression(Node&);
    bool compileRegExpExec(Node&);
    
    template <typename ClassType, bool destructor, typename StructureType> 
    void emitAllocateBasicJSObject(StructureType structure, GPRReg resultGPR, GPRReg scratchGPR, MacroAssembler::JumpList& slowPath)
    {
        MarkedAllocator* allocator = 0;
        if (destructor)
            allocator = &m_jit.globalData()->heap.allocatorForObjectWithDestructor(sizeof(ClassType));
        else
            allocator = &m_jit.globalData()->heap.allocatorForObjectWithoutDestructor(sizeof(ClassType));

        m_jit.loadPtr(&allocator->m_freeList.head, resultGPR);
        slowPath.append(m_jit.branchTestPtr(MacroAssembler::Zero, resultGPR));
        
        // The object is half-allocated: we have what we know is a fresh object, but
        // it's still on the GC's free list.
        
        // Ditch the structure by placing it into the structure slot, so that we can reuse
        // scratchGPR.
        m_jit.storePtr(structure, MacroAssembler::Address(resultGPR, JSObject::structureOffset()));
        
        // Now that we have scratchGPR back, remove the object from the free list
        m_jit.loadPtr(MacroAssembler::Address(resultGPR), scratchGPR);
        m_jit.storePtr(scratchGPR, &allocator->m_freeList.head);
        
        // Initialize the object's classInfo pointer
        m_jit.storePtr(MacroAssembler::TrustedImmPtr(&ClassType::s_info), MacroAssembler::Address(resultGPR, JSCell::classInfoOffset()));
        
        // Initialize the object's inheritorID.
        m_jit.storePtr(MacroAssembler::TrustedImmPtr(0), MacroAssembler::Address(resultGPR, JSObject::offsetOfInheritorID()));
        
        // Initialize the object's property storage pointer.
        m_jit.addPtr(MacroAssembler::TrustedImm32(sizeof(JSObject)), resultGPR, scratchGPR);
        m_jit.storePtr(scratchGPR, MacroAssembler::Address(resultGPR, ClassType::offsetOfPropertyStorage()));
    }

    // It is acceptable to have structure be equal to scratch, so long as you're fine
    // with the structure GPR being clobbered.
    template<typename T>
    void emitAllocateJSFinalObject(T structure, GPRReg resultGPR, GPRReg scratchGPR, MacroAssembler::JumpList& slowPath)
    {
        return emitAllocateBasicJSObject<JSFinalObject, false>(structure, resultGPR, scratchGPR, slowPath);
    }

#if USE(JSVALUE64) 
    JITCompiler::Jump convertToDouble(GPRReg value, FPRReg result, GPRReg tmp);
#elif USE(JSVALUE32_64)
    JITCompiler::Jump convertToDouble(JSValueOperand&, FPRReg result);
#endif

    // Add a speculation check without additional recovery.
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail)
    {
        if (!m_compileOkay)
            return;
        m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(nodeIndex), jumpToFail, this));
    }
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail)
    {
        speculationCheck(kind, jsValueSource, nodeUse.index(), jumpToFail);
    }
    // Add a set of speculation checks without additional recovery.
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::JumpList& jumpsToFail)
    {
        Vector<MacroAssembler::Jump, 16> jumpVector = jumpsToFail.jumps();
        for (unsigned i = 0; i < jumpVector.size(); ++i)
            speculationCheck(kind, jsValueSource, nodeIndex, jumpVector[i]);
    }
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::JumpList& jumpsToFail)
    {
        speculationCheck(kind, jsValueSource, nodeUse.index(), jumpsToFail);
    }
    // Add a speculation check with additional recovery.
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
    {
        if (!m_compileOkay)
            return;
        m_jit.codeBlock()->appendSpeculationRecovery(recovery);
        m_jit.codeBlock()->appendOSRExit(OSRExit(kind, jsValueSource, m_jit.graph().methodOfGettingAValueProfileFor(nodeIndex), jumpToFail, this, m_jit.codeBlock()->numberOfSpeculationRecoveries()));
    }
    void speculationCheck(ExitKind kind, JSValueSource jsValueSource, Edge nodeUse, MacroAssembler::Jump jumpToFail, const SpeculationRecovery& recovery)
    {
        speculationCheck(kind, jsValueSource, nodeUse.index(), jumpToFail, recovery);
    }
    void forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::Jump jumpToFail, const ValueRecovery& valueRecovery)
    {
        speculationCheck(kind, jsValueSource, nodeIndex, jumpToFail);
        
        unsigned setLocalIndexInBlock = m_indexInBlock + 1;
        
        Node* setLocal = &at(m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock));
        
        if (setLocal->op() == Int32ToDouble) {
            setLocal = &at(m_jit.graph().m_blocks[m_block]->at(++setLocalIndexInBlock));
            ASSERT(at(setLocal->child1()).child1() == m_compileIndex);
        } else
            ASSERT(setLocal->child1() == m_compileIndex);
        
        ASSERT(setLocal->op() == SetLocal);
        ASSERT(setLocal->codeOrigin == at(m_compileIndex).codeOrigin);

        Node* nextNode = &at(m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock + 1));
        if (nextNode->codeOrigin == at(m_compileIndex).codeOrigin) {
            ASSERT(nextNode->op() == Flush);
            nextNode = &at(m_jit.graph().m_blocks[m_block]->at(setLocalIndexInBlock + 2));
            ASSERT(nextNode->codeOrigin != at(m_compileIndex).codeOrigin); // duplicate the same assertion as below so that if we fail, we'll know we came down this path.
        }
        ASSERT(nextNode->codeOrigin != at(m_compileIndex).codeOrigin);
        
        OSRExit& exit = m_jit.codeBlock()->lastOSRExit();
        exit.m_codeOrigin = nextNode->codeOrigin;
        exit.m_lastSetOperand = setLocal->local();
        
        exit.valueRecoveryForOperand(setLocal->local()) = valueRecovery;
    }
    void forwardSpeculationCheck(ExitKind kind, JSValueSource jsValueSource, NodeIndex nodeIndex, MacroAssembler::JumpList& jumpsToFail, const ValueRecovery& valueRecovery)
    {
        Vector<MacroAssembler::Jump, 16> jumpVector = jumpsToFail.jumps();
        for (unsigned i = 0; i < jumpVector.size(); ++i)
            forwardSpeculationCheck(kind, jsValueSource, nodeIndex, jumpVector[i], valueRecovery);
    }

    // Called when we statically determine that a speculation will fail.
    void terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, NodeIndex nodeIndex)
    {
#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("SpeculativeJIT was terminated.\n");
#endif
        if (!m_compileOkay)
            return;
        speculationCheck(kind, jsValueRegs, nodeIndex, m_jit.jump());
        m_compileOkay = false;
    }
    void terminateSpeculativeExecution(ExitKind kind, JSValueRegs jsValueRegs, Edge nodeUse)
    {
        terminateSpeculativeExecution(kind, jsValueRegs, nodeUse.index());
    }
    
    template<bool strict>
    GPRReg fillSpeculateIntInternal(NodeIndex, DataFormat& returnFormat);
    
    // It is possible, during speculative generation, to reach a situation in which we
    // can statically determine a speculation will fail (for example, when two nodes
    // will make conflicting speculations about the same operand). In such cases this
    // flag is cleared, indicating no further code generation should take place.
    bool m_compileOkay;
    
    // Tracking for which nodes are currently holding the values of arguments and bytecode
    // operand-indexed variables.
    
    ValueSource valueSourceForOperand(int operand)
    {
        return valueSourceReferenceForOperand(operand);
    }
    
    void setNodeIndexForOperand(NodeIndex nodeIndex, int operand)
    {
        valueSourceReferenceForOperand(operand) = ValueSource(nodeIndex);
    }
    
    // Call this with care, since it both returns a reference into an array
    // and potentially resizes the array. So it would not be right to call this
    // twice and then perform operands on both references, since the one from
    // the first call may no longer be valid.
    ValueSource& valueSourceReferenceForOperand(int operand)
    {
        if (operandIsArgument(operand)) {
            int argument = operandToArgument(operand);
            return m_arguments[argument];
        }
        
        if ((unsigned)operand >= m_variables.size())
            m_variables.resize(operand + 1);
        
        return m_variables[operand];
    }
    
    // The JIT, while also provides MacroAssembler functionality.
    JITCompiler& m_jit;
    // The current node being generated.
    BlockIndex m_block;
    NodeIndex m_compileIndex;
    unsigned m_indexInBlock;
    // Virtual and physical register maps.
    Vector<GenerationInfo, 32> m_generationInfo;
    RegisterBank<GPRInfo> m_gprs;
    RegisterBank<FPRInfo> m_fprs;

    Vector<MacroAssembler::Label> m_blockHeads;
    Vector<MacroAssembler::Label> m_osrEntryHeads;
    
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

    Vector<ValueSource, 0> m_arguments;
    Vector<ValueSource, 0> m_variables;
    int m_lastSetOperand;
    CodeOrigin m_codeOriginForOSR;
    
    AbstractState m_state;
    
    ValueRecovery computeValueRecoveryFor(const ValueSource&);

    ValueRecovery computeValueRecoveryFor(int operand)
    {
        return computeValueRecoveryFor(valueSourceForOperand(operand));
    }
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
    explicit IntegerOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
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
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class DoubleOperand {
public:
    explicit DoubleOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_fprOrInvalid(InvalidFPRReg)
    {
        ASSERT(m_jit);
        
        // This is counter-intuitive but correct. DoubleOperand is intended to
        // be used only when you're a node that is happy to accept an untyped
        // value, but will special-case for doubles (using DoubleOperand) if the
        // value happened to already be represented as a double. The implication
        // is that you will not try to force the value to become a double if it
        // is not one already.
        ASSERT(use.useKind() != DoubleUse);
        
        if (jit->isFilledDouble(m_index))
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
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    FPRReg m_fprOrInvalid;
};

class JSValueOperand {
public:
    explicit JSValueOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
#if USE(JSVALUE64)
        , m_gprOrInvalid(InvalidGPRReg)
#elif USE(JSVALUE32_64)
        , m_isDouble(false)
#endif
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
#if USE(JSVALUE64)
        if (jit->isFilled(m_index))
            gpr();
#elif USE(JSVALUE32_64)
        m_register.pair.tagGPR = InvalidGPRReg;
        m_register.pair.payloadGPR = InvalidGPRReg;
        if (jit->isFilled(m_index))
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
    JSValueRegs jsValueRegs()
    {
        return JSValueRegs(gpr());
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

    JSValueRegs jsValueRegs()
    {
        return JSValueRegs(tagGPR(), payloadGPR());
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
    SpeculativeJIT* m_jit;
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
    explicit StorageOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
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
    SpeculativeJIT* m_jit;
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
    GPRTemporary(SpeculativeJIT*);
    GPRTemporary(SpeculativeJIT*, GPRReg specific);
    GPRTemporary(SpeculativeJIT*, SpeculateIntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateIntegerOperand&, SpeculateIntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateStrictInt32Operand&);
    GPRTemporary(SpeculativeJIT*, IntegerOperand&);
    GPRTemporary(SpeculativeJIT*, IntegerOperand&, IntegerOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateCellOperand&);
    GPRTemporary(SpeculativeJIT*, SpeculateBooleanOperand&);
#if USE(JSVALUE64)
    GPRTemporary(SpeculativeJIT*, JSValueOperand&);
#elif USE(JSVALUE32_64)
    GPRTemporary(SpeculativeJIT*, JSValueOperand&, bool tag = true);
#endif
    GPRTemporary(SpeculativeJIT*, StorageOperand&);

    void adopt(GPRTemporary&);

    ~GPRTemporary()
    {
        if (m_jit && m_gpr != InvalidGPRReg)
            m_jit->unlock(gpr());
    }

    GPRReg gpr()
    {
        return m_gpr;
    }

private:
    SpeculativeJIT* m_jit;
    GPRReg m_gpr;
};

class FPRTemporary {
public:
    FPRTemporary(SpeculativeJIT*);
    FPRTemporary(SpeculativeJIT*, DoubleOperand&);
    FPRTemporary(SpeculativeJIT*, DoubleOperand&, DoubleOperand&);
    FPRTemporary(SpeculativeJIT*, SpeculateDoubleOperand&);
    FPRTemporary(SpeculativeJIT*, SpeculateDoubleOperand&, SpeculateDoubleOperand&);
#if USE(JSVALUE32_64)
    FPRTemporary(SpeculativeJIT*, JSValueOperand&);
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
    FPRTemporary(SpeculativeJIT* jit, FPRReg lockedFPR)
        : m_jit(jit)
        , m_fpr(lockedFPR)
    {
    }

private:
    SpeculativeJIT* m_jit;
    FPRReg m_fpr;
};


// === Results ===
//
// These classes lock the result of a call to a C++ helper function.

class GPRResult : public GPRTemporary {
public:
    GPRResult(SpeculativeJIT* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR)
    {
    }
};

#if USE(JSVALUE32_64)
class GPRResult2 : public GPRTemporary {
public:
    GPRResult2(SpeculativeJIT* jit)
        : GPRTemporary(jit, GPRInfo::returnValueGPR2)
    {
    }
};
#endif

class FPRResult : public FPRTemporary {
public:
    FPRResult(SpeculativeJIT* jit)
        : FPRTemporary(jit, lockedResult(jit))
    {
    }

private:
    static FPRReg lockedResult(SpeculativeJIT* jit)
    {
        jit->lock(FPRInfo::returnValueFPR);
        return FPRInfo::returnValueFPR;
    }
};


// === Speculative Operand types ===
//
// SpeculateIntegerOperand, SpeculateStrictInt32Operand and SpeculateCellOperand.
//
// These are used to lock the operands to a node into machine registers within the
// SpeculativeJIT. The classes operate like those above, however these will
// perform a speculative check for a more restrictive type than we can statically
// determine the operand to have. If the operand does not have the requested type,
// a bail-out to the non-speculative path will be taken.

class SpeculateIntegerOperand {
public:
    explicit SpeculateIntegerOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
#ifndef NDEBUG
        , m_format(DataFormatNone)
#endif
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
            gpr();
    }

    ~SpeculateIntegerOperand()
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
            m_gprOrInvalid = m_jit->fillSpeculateInt(index(), m_format);
        return m_gprOrInvalid;
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
    DataFormat m_format;
};

class SpeculateStrictInt32Operand {
public:
    explicit SpeculateStrictInt32Operand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
            gpr();
    }

    ~SpeculateStrictInt32Operand()
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
            m_gprOrInvalid = m_jit->fillSpeculateIntStrict(index());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};

class SpeculateDoubleOperand {
public:
    explicit SpeculateDoubleOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_fprOrInvalid(InvalidFPRReg)
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() == DoubleUse);
        if (jit->isFilled(m_index))
            fpr();
    }

    ~SpeculateDoubleOperand()
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
            m_fprOrInvalid = m_jit->fillSpeculateDouble(index());
        return m_fprOrInvalid;
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    FPRReg m_fprOrInvalid;
};

class SpeculateCellOperand {
public:
    explicit SpeculateCellOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
            gpr();
    }

    ~SpeculateCellOperand()
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
            m_gprOrInvalid = m_jit->fillSpeculateCell(index());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};

class SpeculateBooleanOperand {
public:
    explicit SpeculateBooleanOperand(SpeculativeJIT* jit, Edge use)
        : m_jit(jit)
        , m_index(use.index())
        , m_gprOrInvalid(InvalidGPRReg)
    {
        ASSERT(m_jit);
        ASSERT(use.useKind() != DoubleUse);
        if (jit->isFilled(m_index))
            gpr();
    }
    
    ~SpeculateBooleanOperand()
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
            m_gprOrInvalid = m_jit->fillSpeculateBoolean(index());
        return m_gprOrInvalid;
    }
    
    void use()
    {
        m_jit->use(m_index);
    }

private:
    SpeculativeJIT* m_jit;
    NodeIndex m_index;
    GPRReg m_gprOrInvalid;
};

inline SpeculativeJIT::SpeculativeJIT(JITCompiler& jit)
    : m_compileOkay(true)
    , m_jit(jit)
    , m_compileIndex(0)
    , m_indexInBlock(0)
    , m_generationInfo(m_jit.codeBlock()->m_numCalleeRegisters)
    , m_blockHeads(jit.graph().m_blocks.size())
    , m_arguments(jit.codeBlock()->numParameters())
    , m_variables(jit.graph().m_localVars)
    , m_lastSetOperand(std::numeric_limits<int>::max())
    , m_state(m_jit.graph())
{
}

} } // namespace JSC::DFG

#endif
#endif

