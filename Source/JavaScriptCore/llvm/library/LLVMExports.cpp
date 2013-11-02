/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#include "config_llvm.h"

#if HAVE(LLVM)

#include "LLVMAPI.h"

extern "C" WTF_EXPORT_PRIVATE JSC::LLVMAPI* initializeAndGetJSCLLVMAPI();

extern "C" JSC::LLVMAPI* initializeAndGetJSCLLVMAPI()
{
    LLVMLinkInMCJIT();
    LLVMInitializeNativeTarget();
    LLVMInitializeX86AsmPrinter();
    LLVMInitializeX86Disassembler();
    
    JSC::LLVMAPI* result = new JSC::LLVMAPI;
    
#define LLVM_API_FUNCTION_ASSIGNMENT(returnType, name, signature) \
    result->name = LLVM##name;
    FOR_EACH_LLVM_API_FUNCTION(LLVM_API_FUNCTION_ASSIGNMENT);
#undef LLVM_API_FUNCTION_ASSIGNMENT
    
    return result;
}

#endif // HAVE(LLVM)
