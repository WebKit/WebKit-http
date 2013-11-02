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

#include "config.h"
#include "DFGOSRExitCompiler.h"

#if ENABLE(DFG_JIT) && USE(JSVALUE64)

#include "DFGOperations.h"
#include "DFGOSRExitCompilerCommon.h"
#include "Operations.h"
#include "VirtualRegister.h"

#include <wtf/DataLog.h>

namespace JSC { namespace DFG {

void OSRExitCompiler::compileExit(const OSRExit& exit, const Operands<ValueRecovery>& operands, SpeculationRecovery* recovery)
{
    // 1) Pro-forma stuff.
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("OSR exit for (");
    for (CodeOrigin codeOrigin = exit.m_codeOrigin; ; codeOrigin = codeOrigin.inlineCallFrame->caller) {
        dataLogF("bc#%u", codeOrigin.bytecodeIndex);
        if (!codeOrigin.inlineCallFrame)
            break;
        dataLogF(" -> %p ", codeOrigin.inlineCallFrame->executable.get());
    }
    dataLogF(")  ");
    dataLog(operands);
#endif

    if (Options::printEachOSRExit()) {
        SpeculationFailureDebugInfo* debugInfo = new SpeculationFailureDebugInfo;
        debugInfo->codeBlock = m_jit.codeBlock();
        
        m_jit.debugCall(debugOperationPrintSpeculationFailure, debugInfo);
    }
    
#if DFG_ENABLE(JIT_BREAK_ON_SPECULATION_FAILURE)
    m_jit.breakpoint();
#endif
    
#if DFG_ENABLE(SUCCESS_STATS)
    static SamplingCounter counter("SpeculationFailure");
    m_jit.emitCount(counter);
#endif
    
    // 2) Perform speculation recovery. This only comes into play when an operation
    //    starts mutating state before verifying the speculation it has already made.
    
    if (recovery) {
        switch (recovery->type()) {
        case SpeculativeAdd:
            m_jit.sub32(recovery->src(), recovery->dest());
            m_jit.or64(GPRInfo::tagTypeNumberRegister, recovery->dest());
            break;
            
        case BooleanSpeculationCheck:
            m_jit.xor64(AssemblyHelpers::TrustedImm32(static_cast<int32_t>(ValueFalse)), recovery->dest());
            break;
            
        default:
            break;
        }
    }

    // 3) Refine some array and/or value profile, if appropriate.
    
    if (!!exit.m_jsValueSource) {
        if (exit.m_kind == BadCache || exit.m_kind == BadIndexingType) {
            // If the instruction that this originated from has an array profile, then
            // refine it. If it doesn't, then do nothing. The latter could happen for
            // hoisted checks, or checks emitted for operations that didn't have array
            // profiling - either ops that aren't array accesses at all, or weren't
            // known to be array acceses in the bytecode. The latter case is a FIXME
            // while the former case is an outcome of a CheckStructure not knowing why
            // it was emitted (could be either due to an inline cache of a property
            // property access, or due to an array profile).
            
            CodeOrigin codeOrigin = exit.m_codeOriginForExitProfile;
            if (ArrayProfile* arrayProfile = m_jit.baselineCodeBlockFor(codeOrigin)->getArrayProfile(codeOrigin.bytecodeIndex)) {
                GPRReg usedRegister;
                if (exit.m_jsValueSource.isAddress())
                    usedRegister = exit.m_jsValueSource.base();
                else
                    usedRegister = exit.m_jsValueSource.gpr();
                
                GPRReg scratch1;
                GPRReg scratch2;
                scratch1 = AssemblyHelpers::selectScratchGPR(usedRegister);
                scratch2 = AssemblyHelpers::selectScratchGPR(usedRegister, scratch1);
                
#if CPU(ARM64)
                m_jit.pushToSave(scratch1);
                m_jit.pushToSave(scratch2);
#else
                m_jit.push(scratch1);
                m_jit.push(scratch2);
#endif
                
                GPRReg value;
                if (exit.m_jsValueSource.isAddress()) {
                    value = scratch1;
                    m_jit.loadPtr(AssemblyHelpers::Address(exit.m_jsValueSource.asAddress()), value);
                } else
                    value = exit.m_jsValueSource.gpr();
                
                m_jit.loadPtr(AssemblyHelpers::Address(value, JSCell::structureOffset()), scratch1);
                m_jit.storePtr(scratch1, arrayProfile->addressOfLastSeenStructure());
                m_jit.load8(AssemblyHelpers::Address(scratch1, Structure::indexingTypeOffset()), scratch1);
                m_jit.move(AssemblyHelpers::TrustedImm32(1), scratch2);
                m_jit.lshift32(scratch1, scratch2);
                m_jit.or32(scratch2, AssemblyHelpers::AbsoluteAddress(arrayProfile->addressOfArrayModes()));
                
#if CPU(ARM64)
                m_jit.popToRestore(scratch2);
                m_jit.popToRestore(scratch1);
#else
                m_jit.pop(scratch2);
                m_jit.pop(scratch1);
#endif
            }
        }
        
        if (!!exit.m_valueProfile) {
            EncodedJSValue* bucket = exit.m_valueProfile.getSpecFailBucket(0);
            
            if (exit.m_jsValueSource.isAddress()) {
                // We can't be sure that we have a spare register. So use the tagTypeNumberRegister,
                // since we know how to restore it.
                m_jit.load64(AssemblyHelpers::Address(exit.m_jsValueSource.asAddress()), GPRInfo::tagTypeNumberRegister);
                m_jit.store64(GPRInfo::tagTypeNumberRegister, bucket);
                m_jit.move(AssemblyHelpers::TrustedImm64(TagTypeNumber), GPRInfo::tagTypeNumberRegister);
            } else
                m_jit.store64(exit.m_jsValueSource.gpr(), bucket);
        }
    }
    
    // What follows is an intentionally simple OSR exit implementation that generates
    // fairly poor code but is very easy to hack. In particular, it dumps all state that
    // needs conversion into a scratch buffer so that in step 6, where we actually do the
    // conversions, we know that all temp registers are free to use and the variable is
    // definitely in a well-known spot in the scratch buffer regardless of whether it had
    // originally been in a register or spilled. This allows us to decouple "where was
    // the variable" from "how was it represented". Consider that the
    // Int32DisplacedInJSStack recovery: it tells us that the value is in a
    // particular place and that that place holds an unboxed int32. We have two different
    // places that a value could be (displaced, register) and a bunch of different
    // ways of representing a value. The number of recoveries is two * a bunch. The code
    // below means that we have to have two + a bunch cases rather than two * a bunch.
    // Once we have loaded the value from wherever it was, the reboxing is the same
    // regardless of its location. Likewise, before we do the reboxing, the way we get to
    // the value (i.e. where we load it from) is the same regardless of its type. Because
    // the code below always dumps everything into a scratch buffer first, the two
    // questions become orthogonal, which simplifies adding new types and adding new
    // locations.
    //
    // This raises the question: does using such a suboptimal implementation of OSR exit,
    // where we always emit code to dump all state into a scratch buffer only to then
    // dump it right back into the stack, hurt us in any way? The asnwer is that OSR exits
    // are rare. Our tiering strategy ensures this. This is because if an OSR exit is
    // taken more than ~100 times, we jettison the DFG code block along with all of its
    // exits. It is impossible for an OSR exit - i.e. the code we compile below - to
    // execute frequently enough for the codegen to matter that much. It probably matters
    // enough that we don't want to turn this into some super-slow function call, but so
    // long as we're generating straight-line code, that code can be pretty bad. Also
    // because we tend to exit only along one OSR exit from any DFG code block - that's an
    // empirical result that we're extremely confident about - the code size of this
    // doesn't matter much. Hence any attempt to optimize the codegen here is just purely
    // harmful to the system: it probably won't reduce either net memory usage or net
    // execution time. It will only prevent us from cleanly decoupling "where was the
    // variable" from "how was it represented", which will make it more difficult to add
    // features in the future and it will make it harder to reason about bugs.

    // 4) Save all state from GPRs into the scratch buffer.
    
    ScratchBuffer* scratchBuffer = m_jit.vm()->scratchBufferForSize(sizeof(EncodedJSValue) * operands.size());
    EncodedJSValue* scratch = scratchBuffer ? static_cast<EncodedJSValue*>(scratchBuffer->dataBuffer()) : 0;
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        
        switch (recovery.technique()) {
        case InGPR:
        case UnboxedInt32InGPR:
        case UInt32InGPR:
        case UnboxedInt52InGPR:
        case UnboxedStrictInt52InGPR:
        case UnboxedCellInGPR:
            m_jit.store64(recovery.gpr(), scratch + index);
            break;
            
        default:
            break;
        }
    }
    
    // And voila, all GPRs are free to reuse.
    
    // 5) Save all state from FPRs into the scratch buffer.
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        
        switch (recovery.technique()) {
        case InFPR:
            m_jit.move(AssemblyHelpers::TrustedImmPtr(scratch + index), GPRInfo::regT0);
            m_jit.storeDouble(recovery.fpr(), GPRInfo::regT0);
            break;
            
        default:
            break;
        }
    }
    
    // Now, all FPRs are also free.
    
    // 6) Save all state from the stack into the scratch buffer. For simplicity we
    //    do this even for state that's already in the right place on the stack.
    //    It makes things simpler later.

    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        
        switch (recovery.technique()) {
        case DisplacedInJSStack:
        case CellDisplacedInJSStack:
        case BooleanDisplacedInJSStack:
        case Int32DisplacedInJSStack:
        case DoubleDisplacedInJSStack:
        case Int52DisplacedInJSStack:
        case StrictInt52DisplacedInJSStack:
            m_jit.load64(AssemblyHelpers::addressFor(recovery.virtualRegister()), GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, scratch + index);
            break;
            
        default:
            break;
        }
    }
    
    // 7) Do all data format conversions and store the results into the stack.
    
    bool haveArguments = false;
    
    for (size_t index = 0; index < operands.size(); ++index) {
        const ValueRecovery& recovery = operands[index];
        int operand = operands.operandForIndex(index);
        
        switch (recovery.technique()) {
        case InGPR:
        case UnboxedCellInGPR:
        case DisplacedInJSStack:
        case CellDisplacedInJSStack:
        case BooleanDisplacedInJSStack:
            m_jit.load64(scratch + index, GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case UnboxedInt32InGPR:
        case Int32DisplacedInJSStack:
            m_jit.load64(scratch + index, GPRInfo::regT0);
            m_jit.zeroExtend32ToPtr(GPRInfo::regT0, GPRInfo::regT0);
            m_jit.or64(GPRInfo::tagTypeNumberRegister, GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case UnboxedInt52InGPR:
        case Int52DisplacedInJSStack:
            m_jit.load64(scratch + index, GPRInfo::regT0);
            m_jit.rshift64(
                AssemblyHelpers::TrustedImm32(JSValue::int52ShiftAmount), GPRInfo::regT0);
            m_jit.boxInt52(GPRInfo::regT0, GPRInfo::regT0, GPRInfo::regT1, FPRInfo::fpRegT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case UnboxedStrictInt52InGPR:
        case StrictInt52DisplacedInJSStack:
            m_jit.load64(scratch + index, GPRInfo::regT0);
            m_jit.boxInt52(GPRInfo::regT0, GPRInfo::regT0, GPRInfo::regT1, FPRInfo::fpRegT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case UInt32InGPR:
            m_jit.load64(scratch + index, GPRInfo::regT0);
            m_jit.zeroExtend32ToPtr(GPRInfo::regT0, GPRInfo::regT0);
            m_jit.boxInt52(GPRInfo::regT0, GPRInfo::regT0, GPRInfo::regT1, FPRInfo::fpRegT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case InFPR:
        case DoubleDisplacedInJSStack:
            m_jit.move(AssemblyHelpers::TrustedImmPtr(scratch + index), GPRInfo::regT0);
            m_jit.loadDouble(GPRInfo::regT0, FPRInfo::fpRegT0);
            m_jit.boxDouble(FPRInfo::fpRegT0, GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
            break;
            
        case Constant:
            m_jit.store64(
                AssemblyHelpers::TrustedImm64(JSValue::encode(recovery.constant())),
                AssemblyHelpers::addressFor(operand));
            break;
            
        case ArgumentsThatWereNotCreated:
            haveArguments = true;
            // We can't restore this yet but we can make sure that the stack appears
            // sane.
            m_jit.store64(
                AssemblyHelpers::TrustedImm64(JSValue::encode(JSValue())),
                AssemblyHelpers::addressFor(operand));
            break;
            
        default:
            break;
        }
    }
    
    // 8) Adjust the old JIT's execute counter. Since we are exiting OSR, we know
    //    that all new calls into this code will go to the new JIT, so the execute
    //    counter only affects call frames that performed OSR exit and call frames
    //    that were still executing the old JIT at the time of another call frame's
    //    OSR exit. We want to ensure that the following is true:
    //
    //    (a) Code the performs an OSR exit gets a chance to reenter optimized
    //        code eventually, since optimized code is faster. But we don't
    //        want to do such reentery too aggressively (see (c) below).
    //
    //    (b) If there is code on the call stack that is still running the old
    //        JIT's code and has never OSR'd, then it should get a chance to
    //        perform OSR entry despite the fact that we've exited.
    //
    //    (c) Code the performs an OSR exit should not immediately retry OSR
    //        entry, since both forms of OSR are expensive. OSR entry is
    //        particularly expensive.
    //
    //    (d) Frequent OSR failures, even those that do not result in the code
    //        running in a hot loop, result in recompilation getting triggered.
    //
    //    To ensure (c), we'd like to set the execute counter to
    //    counterValueForOptimizeAfterWarmUp(). This seems like it would endanger
    //    (a) and (b), since then every OSR exit would delay the opportunity for
    //    every call frame to perform OSR entry. Essentially, if OSR exit happens
    //    frequently and the function has few loops, then the counter will never
    //    become non-negative and OSR entry will never be triggered. OSR entry
    //    will only happen if a loop gets hot in the old JIT, which does a pretty
    //    good job of ensuring (a) and (b). But that doesn't take care of (d),
    //    since each speculation failure would reset the execute counter.
    //    So we check here if the number of speculation failures is significantly
    //    larger than the number of successes (we want 90% success rate), and if
    //    there have been a large enough number of failures. If so, we set the
    //    counter to 0; otherwise we set the counter to
    //    counterValueForOptimizeAfterWarmUp().
    
    handleExitCounts(m_jit, exit);
    
    // 9) Reify inlined call frames.
    
    reifyInlinedCallFrames(m_jit, exit);
    
    // 10) Create arguments if necessary and place them into the appropriate aliased
    //     registers.
    
    if (haveArguments) {
        HashSet<InlineCallFrame*, DefaultHash<InlineCallFrame*>::Hash,
            NullableHashTraits<InlineCallFrame*>> didCreateArgumentsObject;

        for (size_t index = 0; index < operands.size(); ++index) {
            const ValueRecovery& recovery = operands[index];
            if (recovery.technique() != ArgumentsThatWereNotCreated)
                continue;
            int operand = operands.operandForIndex(index);
            // Find the right inline call frame.
            InlineCallFrame* inlineCallFrame = 0;
            for (InlineCallFrame* current = exit.m_codeOrigin.inlineCallFrame;
                 current;
                 current = current->caller.inlineCallFrame) {
                if (current->stackOffset >= operand) {
                    inlineCallFrame = current;
                    break;
                }
            }

            if (!m_jit.baselineCodeBlockFor(inlineCallFrame)->usesArguments())
                continue;
            VirtualRegister argumentsRegister = m_jit.baselineArgumentsRegisterFor(inlineCallFrame);
            if (didCreateArgumentsObject.add(inlineCallFrame).isNewEntry) {
                // We know this call frame optimized out an arguments object that
                // the baseline JIT would have created. Do that creation now.
                if (inlineCallFrame) {
                    m_jit.addPtr(AssemblyHelpers::TrustedImm32(inlineCallFrame->stackOffset * sizeof(EncodedJSValue)), GPRInfo::callFrameRegister, GPRInfo::regT0);
                    m_jit.setupArguments(GPRInfo::regT0);
                } else
                    m_jit.setupArgumentsExecState();
                m_jit.move(
                    AssemblyHelpers::TrustedImmPtr(
                        bitwise_cast<void*>(operationCreateArguments)),
                    GPRInfo::nonArgGPR0);
                m_jit.call(GPRInfo::nonArgGPR0);
                m_jit.store64(GPRInfo::returnValueGPR, AssemblyHelpers::addressFor(argumentsRegister));
                m_jit.store64(
                    GPRInfo::returnValueGPR,
                    AssemblyHelpers::addressFor(unmodifiedArgumentsRegister(argumentsRegister)));
                m_jit.move(GPRInfo::returnValueGPR, GPRInfo::regT0); // no-op move on almost all platforms.
            }

            m_jit.load64(AssemblyHelpers::addressFor(argumentsRegister), GPRInfo::regT0);
            m_jit.store64(GPRInfo::regT0, AssemblyHelpers::addressFor(operand));
        }
    }
    
    // 11) Load the result of the last bytecode operation into regT0.
    
    if (exit.m_lastSetOperand.isValid())
        m_jit.load64(AssemblyHelpers::addressFor(exit.m_lastSetOperand), GPRInfo::cachedResultRegister);
    
    // 12) And finish.
    
    adjustAndJumpToTarget(m_jit, exit);
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT) && USE(JSVALUE64)
