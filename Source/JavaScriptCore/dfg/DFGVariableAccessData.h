/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#ifndef DFGVariableAccessData_h
#define DFGVariableAccessData_h

#include "DFGDoubleFormatState.h"
#include "DFGNodeFlags.h"
#include "Operands.h"
#include "SpeculatedType.h"
#include "VirtualRegister.h"
#include <wtf/Platform.h>
#include <wtf/UnionFind.h>
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

enum DoubleBallot { VoteValue, VoteDouble };

class VariableAccessData : public UnionFind<VariableAccessData> {
public:
    VariableAccessData()
        : m_local(static_cast<VirtualRegister>(std::numeric_limits<int>::min()))
        , m_prediction(SpecNone)
        , m_argumentAwarePrediction(SpecNone)
        , m_flags(0)
        , m_doubleFormatState(EmptyDoubleFormatState)
        , m_isCaptured(false)
        , m_isArgumentsAlias(false)
        , m_structureCheckHoistingFailed(false)
    {
        clearVotes();
    }
    
    VariableAccessData(VirtualRegister local, bool isCaptured)
        : m_local(local)
        , m_prediction(SpecNone)
        , m_argumentAwarePrediction(SpecNone)
        , m_flags(0)
        , m_doubleFormatState(EmptyDoubleFormatState)
        , m_isCaptured(isCaptured)
        , m_isArgumentsAlias(false)
        , m_structureCheckHoistingFailed(false)
    {
        clearVotes();
    }
    
    VirtualRegister local()
    {
        ASSERT(m_local == find()->m_local);
        return m_local;
    }
    
    int operand()
    {
        return static_cast<int>(local());
    }
    
    bool mergeIsCaptured(bool isCaptured)
    {
        bool newIsCaptured = m_isCaptured | isCaptured;
        if (newIsCaptured == m_isCaptured)
            return false;
        m_isCaptured = newIsCaptured;
        return true;
    }
    
    bool isCaptured()
    {
        return m_isCaptured;
    }
    
    bool mergeStructureCheckHoistingFailed(bool failed)
    {
        bool newFailed = m_structureCheckHoistingFailed | failed;
        if (newFailed == m_structureCheckHoistingFailed)
            return false;
        m_structureCheckHoistingFailed = newFailed;
        return true;
    }
    
    bool structureCheckHoistingFailed()
    {
        return m_structureCheckHoistingFailed;
    }
    
    bool mergeIsArgumentsAlias(bool isArgumentsAlias)
    {
        bool newIsArgumentsAlias = m_isArgumentsAlias | isArgumentsAlias;
        if (newIsArgumentsAlias == m_isArgumentsAlias)
            return false;
        m_isArgumentsAlias = newIsArgumentsAlias;
        return true;
    }
    
    bool isArgumentsAlias()
    {
        return m_isArgumentsAlias;
    }
    
    bool predict(SpeculatedType prediction)
    {
        VariableAccessData* self = find();
        bool result = mergeSpeculation(self->m_prediction, prediction);
        if (result)
            mergeSpeculation(m_argumentAwarePrediction, m_prediction);
        return result;
    }
    
    SpeculatedType nonUnifiedPrediction()
    {
        return m_prediction;
    }
    
    SpeculatedType prediction()
    {
        return find()->m_prediction;
    }
    
    SpeculatedType argumentAwarePrediction()
    {
        return find()->m_argumentAwarePrediction;
    }
    
    bool mergeArgumentAwarePrediction(SpeculatedType prediction)
    {
        return mergeSpeculation(find()->m_argumentAwarePrediction, prediction);
    }
    
    void clearVotes()
    {
        ASSERT(find() == this);
        m_votes[0] = 0;
        m_votes[1] = 0;
    }
    
    void vote(unsigned ballot)
    {
        ASSERT(ballot < 2);
        m_votes[ballot]++;
    }
    
    double voteRatio()
    {
        ASSERT(find() == this);
        return static_cast<double>(m_votes[1]) / m_votes[0];
    }
    
    bool shouldUseDoubleFormatAccordingToVote()
    {
        // We don't support this facility for arguments, yet.
        // FIXME: make this work for arguments.
        if (operandIsArgument(operand()))
            return false;
        
        // If the variable is not a number prediction, then this doesn't
        // make any sense.
        if (!isNumberSpeculation(prediction()))
            return false;
        
        // If the variable is predicted to hold only doubles, then it's a
        // no-brainer: it should be formatted as a double.
        if (isDoubleSpeculation(prediction()))
            return true;
        
        // If the variable is known to be used as an integer, then be safe -
        // don't force it to be a double.
        if (flags() & NodeUsedAsInt)
            return false;
        
        // If the variable has been voted to become a double, then make it a
        // double.
        if (voteRatio() >= Options::doubleVoteRatioForDoubleFormat())
            return true;
        
        return false;
    }
    
    DoubleFormatState doubleFormatState()
    {
        return find()->m_doubleFormatState;
    }
    
    bool shouldUseDoubleFormat()
    {
        ASSERT(isRoot());
        return m_doubleFormatState == UsingDoubleFormat;
    }
    
    bool tallyVotesForShouldUseDoubleFormat()
    {
        ASSERT(isRoot());
        
        if (m_doubleFormatState == CantUseDoubleFormat)
            return false;
        
        bool newValueOfShouldUseDoubleFormat = shouldUseDoubleFormatAccordingToVote();
        if (!newValueOfShouldUseDoubleFormat) {
            // We monotonically convert to double. Hence, if the fixpoint leads us to conclude that we should
            // switch back to int, we instead ignore this and stick with double.
            return false;
        }
        
        if (m_doubleFormatState == UsingDoubleFormat)
            return false;
        
        return DFG::mergeDoubleFormatState(m_doubleFormatState, UsingDoubleFormat);
    }
    
    bool mergeDoubleFormatState(DoubleFormatState doubleFormatState)
    {
        return DFG::mergeDoubleFormatState(find()->m_doubleFormatState, doubleFormatState);
    }
    
    bool makePredictionForDoubleFormat()
    {
        ASSERT(isRoot());
        
        if (m_doubleFormatState != UsingDoubleFormat)
            return false;
        
        return mergeSpeculation(m_prediction, SpecDouble);
    }
    
    NodeFlags flags() const { return m_flags; }
    
    bool mergeFlags(NodeFlags newFlags)
    {
        newFlags |= m_flags;
        if (newFlags == m_flags)
            return false;
        m_flags = newFlags;
        return true;
    }
    
private:
    // This is slightly space-inefficient, since anything we're unified with
    // will have the same operand and should have the same prediction. But
    // putting them here simplifies the code, and we don't expect DFG space
    // usage for variable access nodes do be significant.

    VirtualRegister m_local;
    SpeculatedType m_prediction;
    SpeculatedType m_argumentAwarePrediction;
    NodeFlags m_flags;
    
    float m_votes[2]; // Used primarily for double voting but may be reused for other purposes.
    DoubleFormatState m_doubleFormatState;
    
    bool m_isCaptured;
    bool m_isArgumentsAlias;
    bool m_structureCheckHoistingFailed;
};

} } // namespace JSC::DFG

#endif // DFGVariableAccessData_h
