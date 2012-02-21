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

#include "config.h"
#include "DFGOSRExitCompiler.h"

#if ENABLE(DFG_JIT)

#include "CallFrame.h"
#include "LinkBuffer.h"
#include "RepatchBuffer.h"

namespace JSC { namespace DFG {

extern "C" {

void compileOSRExit(ExecState* exec)
{
    CodeBlock* codeBlock = exec->codeBlock();
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->getJITType() == JITCode::DFGJIT);
    
    JSGlobalData* globalData = &exec->globalData();
    
    uint32_t exitIndex = globalData->osrExitIndex;
    OSRExit& exit = codeBlock->osrExit(exitIndex);
    
    SpeculationRecovery* recovery = 0;
    if (exit.m_recoveryIndex)
        recovery = &codeBlock->speculationRecovery(exit.m_recoveryIndex - 1);

#if DFG_ENABLE(DEBUG_VERBOSE)
    dataLog("Generating OSR exit #%u (bc#%u, @%u, %s) for code block %p.\n", exitIndex, exit.m_codeOrigin.bytecodeIndex, exit.m_nodeIndex, exitKindToString(exit.m_kind), codeBlock);
#endif

    {
        AssemblyHelpers jit(globalData, codeBlock);
        OSRExitCompiler exitCompiler(jit);
        
        exitCompiler.compileExit(exit, recovery);
        
        LinkBuffer patchBuffer(*globalData, &jit, codeBlock);
        exit.m_code = patchBuffer.finalizeCode();

#if DFG_ENABLE(DEBUG_VERBOSE)
        dataLog("OSR exit code at [%p, %p).\n", patchBuffer.debugAddress(), static_cast<char*>(patchBuffer.debugAddress()) + patchBuffer.debugSize());
#endif
    }
    
    {
        RepatchBuffer repatchBuffer(codeBlock);
        repatchBuffer.relink(exit.m_check.codeLocationForRepatch(codeBlock), CodeLocationLabel(exit.m_code.code()));
    }
    
    globalData->osrExitJumpDestination = exit.m_code.code().executableAddress();
}

} // extern "C"

} } // namespace JSC::DFG

#endif // ENABLE(DFG_JIT)
