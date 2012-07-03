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

#ifndef DFGOSRExit_h
#define DFGOSRExit_h

#include <wtf/Platform.h>

#if ENABLE(DFG_JIT)

#include "CodeOrigin.h"
#include "DFGCommon.h"
#include "DFGCorrectableJumpPoint.h"
#include "DFGExitProfile.h"
#include "DFGGPRInfo.h"
#include "DFGValueRecoveryOverride.h"
#include "MacroAssembler.h"
#include "MethodOfGettingAValueProfile.h"
#include "Operands.h"
#include "ValueProfile.h"
#include "ValueRecovery.h"
#include <wtf/Vector.h>

namespace JSC { namespace DFG {

class SpeculativeJIT;

// This enum describes the types of additional recovery that
// may need be performed should a speculation check fail.
enum SpeculationRecoveryType {
    SpeculativeAdd,
    BooleanSpeculationCheck
};

// === SpeculationRecovery ===
//
// This class provides additional information that may be associated with a
// speculation check - for example 
class SpeculationRecovery {
public:
    SpeculationRecovery(SpeculationRecoveryType type, GPRReg dest, GPRReg src)
        : m_type(type)
        , m_dest(dest)
        , m_src(src)
    {
    }

    SpeculationRecoveryType type() { return m_type; }
    GPRReg dest() { return m_dest; }
    GPRReg src() { return m_src; }

private:
    // Indicates the type of additional recovery to be performed.
    SpeculationRecoveryType m_type;
    // different recovery types may required different additional information here.
    GPRReg m_dest;
    GPRReg m_src;
};

// === OSRExit ===
//
// This structure describes how to exit the speculative path by
// going into baseline code.
struct OSRExit {
    OSRExit(ExitKind, JSValueSource, MethodOfGettingAValueProfile, MacroAssembler::Jump, SpeculativeJIT*, unsigned streamIndex, unsigned recoveryIndex = 0);
    
    MacroAssemblerCodeRef m_code;
    
    JSValueSource m_jsValueSource;
    MethodOfGettingAValueProfile m_valueProfile;
    
    CorrectableJumpPoint m_check;
    NodeIndex m_nodeIndex;
    CodeOrigin m_codeOrigin;
    CodeOrigin m_codeOriginForExitProfile;
    
    unsigned m_recoveryIndex;
    unsigned m_watchpointIndex;
    
    ExitKind m_kind;
    uint32_t m_count;
    
    bool considerAddingAsFrequentExitSite(CodeBlock* dfgCodeBlock, CodeBlock* profiledCodeBlock)
    {
        if (!m_count || !exitKindIsCountable(m_kind))
            return false;
        return considerAddingAsFrequentExitSiteSlow(dfgCodeBlock, profiledCodeBlock);
    }
    
    unsigned m_streamIndex;
    int m_lastSetOperand;
    
    RefPtr<ValueRecoveryOverride> m_valueRecoveryOverride;

private:
    bool considerAddingAsFrequentExitSiteSlow(CodeBlock* dfgCodeBlock, CodeBlock* profiledCodeBlock);
};

#if DFG_ENABLE(VERBOSE_SPECULATION_FAILURE)
struct SpeculationFailureDebugInfo {
    CodeBlock* codeBlock;
    NodeIndex nodeIndex;
};
#endif

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)

#endif // DFGOSRExit_h

