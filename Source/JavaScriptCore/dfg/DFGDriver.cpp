/*
 * Copyright (C) 2011, 2012, 2013 Apple Inc. All rights reserved.
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
#include "DFGDriver.h"

#include "JSObject.h"
#include "JSString.h"

#include "CodeBlock.h"
#include "DFGJITCode.h"
#include "DFGPlan.h"
#include "DFGThunks.h"
#include "DFGWorklist.h"
#include "JITCode.h"
#include "Operations.h"
#include "Options.h"
#include "SamplingTool.h"
#include <wtf/Atomics.h>

#if ENABLE(FTL_JIT)
#include "FTLThunks.h"
#endif

namespace JSC { namespace DFG {

static unsigned numCompilations;

unsigned getNumCompilations()
{
    return numCompilations;
}

#if ENABLE(DFG_JIT)
static CompilationResult compileImpl(
    VM& vm, CodeBlock* codeBlock, CompilationMode mode, unsigned osrEntryBytecodeIndex,
    const Operands<JSValue>& mustHandleValues,
    PassRefPtr<DeferredCompilationCallback> callback, Worklist* worklist)
{
    SamplingRegion samplingRegion("DFG Compilation (Driver)");
    
    numCompilations++;
    
    ASSERT(codeBlock);
    ASSERT(codeBlock->alternative());
    ASSERT(codeBlock->alternative()->jitType() == JITCode::BaselineJIT);
    
    if (!Options::useDFGJIT() || !MacroAssembler::supportsFloatingPoint())
        return CompilationFailed;

    if (!Options::bytecodeRangeToDFGCompile().isInRange(codeBlock->instructionCount()))
        return CompilationFailed;
    
    if (logCompilationChanges())
        dataLog("DFG(Driver) compiling ", *codeBlock, " with ", mode, ", number of instructions = ", codeBlock->instructionCount(), "\n");
    
    // Make sure that any stubs that the DFG is going to use are initialized. We want to
    // make sure that all JIT code generation does finalization on the main thread.
    vm.getCTIStub(osrExitGenerationThunkGenerator);
    vm.getCTIStub(throwExceptionFromCallSlowPathGenerator);
    vm.getCTIStub(linkCallThunkGenerator);
    vm.getCTIStub(linkConstructThunkGenerator);
    vm.getCTIStub(linkClosureCallThunkGenerator);
    vm.getCTIStub(virtualCallThunkGenerator);
    vm.getCTIStub(virtualConstructThunkGenerator);
#if ENABLE(FTL_JIT)
    vm.getCTIStub(FTL::osrExitGenerationWithoutStackMapThunkGenerator);
#endif
    
    RefPtr<Plan> plan = adoptRef(
        new Plan(codeBlock, mode, osrEntryBytecodeIndex, mustHandleValues));
    
    if (worklist) {
        plan->callback = callback;
        if (logCompilationChanges())
            dataLog("Deferring DFG compilation of ", *codeBlock, " with queue length ", worklist->queueLength(), ".\n");
        worklist->enqueue(plan);
        return CompilationDeferred;
    }
    
    plan->compileInThread(*vm.dfgState);
    return plan->finalizeWithoutNotifyingCallback();
}
#else // ENABLE(DFG_JIT)
static CompilationResult compileImpl(
    VM&, CodeBlock*, CompilationMode, unsigned, const Operands<JSValue>&,
    PassRefPtr<DeferredCompilationCallback>, Worklist*)
{
    return CompilationFailed;
}
#endif // ENABLE(DFG_JIT)

CompilationResult compile(
    VM& vm, CodeBlock* codeBlock, CompilationMode mode, unsigned osrEntryBytecodeIndex,
    const Operands<JSValue>& mustHandleValues,
    PassRefPtr<DeferredCompilationCallback> passedCallback, Worklist* worklist)
{
    RefPtr<DeferredCompilationCallback> callback = passedCallback;
    CompilationResult result = compileImpl(
        vm, codeBlock, mode, osrEntryBytecodeIndex, mustHandleValues, callback, worklist);
    if (result != CompilationDeferred)
        callback->compilationDidComplete(codeBlock, result);
    return result;
}

} } // namespace JSC::DFG
