/*
 * Copyright (C) 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGVariableEventStream.h"

#if ENABLE(DFG_JIT)

#include "CodeBlock.h"
#include "DFGJITCode.h"
#include "DFGValueSource.h"
#include "Operations.h"
#include <wtf/DataLog.h>
#include <wtf/HashMap.h>

namespace JSC { namespace DFG {

void VariableEventStream::logEvent(const VariableEvent& event)
{
    dataLogF("seq#%u:", static_cast<unsigned>(size()));
    event.dump(WTF::dataFile());
    dataLogF(" ");
}

namespace {

struct MinifiedGenerationInfo {
    bool filled; // true -> in gpr/fpr/pair, false -> spilled
    VariableRepresentation u;
    DataFormat format;
    
    MinifiedGenerationInfo()
        : format(DataFormatNone)
    {
    }
    
    void update(const VariableEvent& event)
    {
        switch (event.kind()) {
        case BirthToFill:
        case Fill:
            filled = true;
            break;
        case BirthToSpill:
        case Spill:
            filled = false;
            break;
        case Death:
            format = DataFormatNone;
            return;
        default:
            return;
        }
        
        u = event.variableRepresentation();
        format = event.dataFormat();
    }
};

} // namespace

bool VariableEventStream::tryToSetConstantRecovery(ValueRecovery& recovery, CodeBlock* codeBlock, MinifiedNode* node) const
{
    if (!node)
        return false;
    
    if (node->hasConstantNumber()) {
        recovery = ValueRecovery::constant(
            codeBlock->constantRegister(
                FirstConstantRegisterIndex + node->constantNumber()).get());
        return true;
    }
    
    if (node->hasWeakConstant()) {
        recovery = ValueRecovery::constant(node->weakConstant());
        return true;
    }
    
    if (node->op() == PhantomArguments) {
        recovery = ValueRecovery::argumentsThatWereNotCreated();
        return true;
    }
    
    return false;
}

void VariableEventStream::reconstruct(
    CodeBlock* codeBlock, CodeOrigin codeOrigin, MinifiedGraph& graph,
    unsigned index, Operands<ValueRecovery>& valueRecoveries) const
{
    ASSERT(codeBlock->jitType() == JITCode::DFGJIT);
    CodeBlock* baselineCodeBlock = codeBlock->baselineVersion();
    
    unsigned numVariables;
    if (codeOrigin.inlineCallFrame)
        numVariables = baselineCodeBlockForInlineCallFrame(codeOrigin.inlineCallFrame)->m_numCalleeRegisters + VirtualRegister(codeOrigin.inlineCallFrame->stackOffset).toLocal() + 1;
    else
        numVariables = baselineCodeBlock->m_numCalleeRegisters;
    
    // Crazy special case: if we're at index == 0 then this must be an argument check
    // failure, in which case all variables are already set up. The recoveries should
    // reflect this.
    if (!index) {
        valueRecoveries = Operands<ValueRecovery>(codeBlock->numParameters(), numVariables);
        for (size_t i = 0; i < valueRecoveries.size(); ++i) {
            valueRecoveries[i] = ValueRecovery::displacedInJSStack(
                VirtualRegister(valueRecoveries.operandForIndex(i)), DataFormatJS);
        }
        return;
    }
    
    // Step 1: Find the last checkpoint, and figure out the number of virtual registers as we go.
    unsigned startIndex = index - 1;
    while (at(startIndex).kind() != Reset)
        startIndex--;
    
#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLogF("Computing OSR exit recoveries starting at seq#%u.\n", startIndex);
#endif

    // Step 2: Create a mock-up of the DFG's state and execute the events.
    Operands<ValueSource> operandSources(codeBlock->numParameters(), numVariables);
    for (unsigned i = operandSources.size(); i--;)
        operandSources[i] = ValueSource(SourceIsDead);
    HashMap<MinifiedID, MinifiedGenerationInfo> generationInfos;
    for (unsigned i = startIndex; i < index; ++i) {
        const VariableEvent& event = at(i);
        switch (event.kind()) {
        case Reset:
            // nothing to do.
            break;
        case BirthToFill:
        case BirthToSpill: {
            MinifiedGenerationInfo info;
            info.update(event);
            generationInfos.add(event.id(), info);
            break;
        }
        case Fill:
        case Spill:
        case Death: {
            HashMap<MinifiedID, MinifiedGenerationInfo>::iterator iter = generationInfos.find(event.id());
            ASSERT(iter != generationInfos.end());
            iter->value.update(event);
            break;
        }
        case MovHintEvent:
            if (operandSources.hasOperand(event.bytecodeRegister()))
                operandSources.setOperand(event.bytecodeRegister(), ValueSource(event.id()));
            break;
        case SetLocalEvent:
            if (operandSources.hasOperand(event.bytecodeRegister()))
                operandSources.setOperand(event.bytecodeRegister(), ValueSource::forDataFormat(event.machineRegister(), event.dataFormat()));
            break;
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }
    }
    
    // Step 3: Compute value recoveries!
    valueRecoveries = Operands<ValueRecovery>(codeBlock->numParameters(), numVariables);
    for (unsigned i = 0; i < operandSources.size(); ++i) {
        ValueSource& source = operandSources[i];
        if (source.isTriviallyRecoverable()) {
            valueRecoveries[i] = source.valueRecovery();
            continue;
        }
        
        ASSERT(source.kind() == HaveNode);
        MinifiedNode* node = graph.at(source.id());
        if (tryToSetConstantRecovery(valueRecoveries[i], codeBlock, node))
            continue;
        
        MinifiedGenerationInfo info = generationInfos.get(source.id());
        if (info.format == DataFormatNone) {
            // Try to see if there is an alternate node that would contain the value we want.
            //
            // Backward rewiring refers to:
            //
            //     a: Something(...)
            //     b: Id(@a) // some identity function
            //     c: SetLocal(@b)
            //
            // Where we find @b being dead, but @a is still alive.
            //
            // Forward rewiring refers to:
            //
            //     a: Something(...)
            //     b: SetLocal(@a)
            //     c: Id(@a) // some identity function
            //
            // Where we find @a being dead, but @b is still alive.
            
            bool found = false;
            
            if (node && permitsOSRBackwardRewiring(node->op())) {
                MinifiedID id = node->child1();
                if (tryToSetConstantRecovery(valueRecoveries[i], codeBlock, graph.at(id)))
                    continue;
                info = generationInfos.get(id);
                if (info.format != DataFormatNone)
                    found = true;
            }
            
            if (!found) {
                MinifiedID bestID;
                unsigned bestScore = 0;
                
                HashMap<MinifiedID, MinifiedGenerationInfo>::iterator iter = generationInfos.begin();
                HashMap<MinifiedID, MinifiedGenerationInfo>::iterator end = generationInfos.end();
                for (; iter != end; ++iter) {
                    MinifiedID id = iter->key;
                    node = graph.at(id);
                    if (!node)
                        continue;
                    if (!node->hasChild1())
                        continue;
                    if (node->child1() != source.id())
                        continue;
                    if (iter->value.format == DataFormatNone)
                        continue;
                    unsigned myScore = forwardRewiringSelectionScore(node->op());
                    if (myScore <= bestScore)
                        continue;
                    bestID = id;
                    bestScore = myScore;
                }
                
                if (!!bestID) {
                    info = generationInfos.get(bestID);
                    ASSERT(info.format != DataFormatNone);
                    found = true;
                }
            }
            
            if (!found) {
                valueRecoveries[i] = ValueRecovery::constant(jsUndefined());
                continue;
            }
        }
        
        ASSERT(info.format != DataFormatNone);
        
        if (info.filled) {
            if (info.format == DataFormatDouble) {
                valueRecoveries[i] = ValueRecovery::inFPR(info.u.fpr);
                continue;
            }
#if USE(JSVALUE32_64)
            if (info.format & DataFormatJS) {
                valueRecoveries[i] = ValueRecovery::inPair(info.u.pair.tagGPR, info.u.pair.payloadGPR);
                continue;
            }
#endif
            valueRecoveries[i] = ValueRecovery::inGPR(info.u.gpr, info.format);
            continue;
        }
        
        valueRecoveries[i] =
            ValueRecovery::displacedInJSStack(static_cast<VirtualRegister>(info.u.virtualReg), info.format);
    }
}

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

