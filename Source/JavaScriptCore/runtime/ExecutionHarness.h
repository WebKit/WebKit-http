/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#ifndef ExecutionHarness_h
#define ExecutionHarness_h

#include <wtf/Platform.h>

#if ENABLE(JIT)

#include "JITDriver.h"
#include "LLIntEntrypoints.h"

namespace JSC {

template<typename CodeBlockType>
inline bool prepareForExecution(JSGlobalData& globalData, OwnPtr<CodeBlockType>& codeBlock, JITCode& jitCode, JITCode::JITType jitType)
{
#if ENABLE(LLINT)
    if (JITCode::isBaselineCode(jitType)) {
        // Start off in the low level interpreter.
        LLInt::getEntrypoint(globalData, codeBlock.get(), jitCode);
        codeBlock->setJITCode(jitCode, MacroAssemblerCodePtr());
        return true;
    }
#endif // ENABLE(LLINT)
    return jitCompileIfAppropriate(globalData, codeBlock, jitCode, jitType, JITCode::isBaselineCode(jitType) ? JITCompilationMustSucceed : JITCompilationCanFail);
}

inline bool prepareFunctionForExecution(JSGlobalData& globalData, OwnPtr<FunctionCodeBlock>& codeBlock, JITCode& jitCode, MacroAssemblerCodePtr& jitCodeWithArityCheck, SharedSymbolTable*& symbolTable, JITCode::JITType jitType, CodeSpecializationKind kind)
{
#if ENABLE(LLINT)
    if (JITCode::isBaselineCode(jitType)) {
        // Start off in the low level interpreter.
        LLInt::getFunctionEntrypoint(globalData, kind, jitCode, jitCodeWithArityCheck);
        codeBlock->setJITCode(jitCode, jitCodeWithArityCheck);
        return true;
    }
#else
    UNUSED_PARAM(kind);
#endif // ENABLE(LLINT)
    return jitCompileFunctionIfAppropriate(globalData, codeBlock, jitCode, jitCodeWithArityCheck, symbolTable, jitType, JITCode::isBaselineCode(jitType) ? JITCompilationMustSucceed : JITCompilationCanFail);
}

} // namespace JSC

#endif // ENABLE(JIT)

#endif // ExecutionHarness_h

