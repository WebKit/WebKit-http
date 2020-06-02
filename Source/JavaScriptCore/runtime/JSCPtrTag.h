/*
 * Copyright (C) 2018-2019 Apple Inc. All rights reserved.
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

#pragma once

#include <wtf/PtrTag.h>

namespace JSC {

using PtrTag = WTF::PtrTag;

#define FOR_EACH_JSC_PTRTAG(v) \
    v(B3CCallPtrTag) \
    v(B3CompilationPtrTag) \
    v(BytecodePtrTag) \
    v(DOMJITFunctionPtrTag) \
    v(DisassemblyPtrTag) \
    v(ExceptionHandlerPtrTag) \
    v(ExecutableMemoryPtrTag) \
    v(JITProbePtrTag) \
    v(JITProbeTrampolinePtrTag) \
    v(JITProbeExecutorPtrTag) \
    v(JITProbeStackInitializationFunctionPtrTag) \
    v(JITThunkPtrTag) \
    v(JITStubRoutinePtrTag) \
    v(JSEntryPtrTag) \
    v(JSInternalPtrTag) \
    v(JSSwitchPtrTag) \
    v(LinkBufferPtrTag) \
    v(OperationPtrTag) \
    v(OSREntryPtrTag) \
    v(OSRExitPtrTag) \
    v(SlowPathPtrTag) \
    v(WasmEntryPtrTag) \
    v(Yarr8BitPtrTag) \
    v(Yarr16BitPtrTag) \
    v(YarrMatchOnly8BitPtrTag) \
    v(YarrMatchOnly16BitPtrTag) \
    v(YarrBacktrackPtrTag) \

#if COMPILER(MSVC)
#pragma warning(push)
#pragma warning(disable:4307)
#endif

FOR_EACH_JSC_PTRTAG(WTF_DECLARE_PTRTAG)

#if COMPILER(MSVC)
#pragma warning(pop)
#endif

void initializePtrTagLookup();

#if !CPU(ARM64E)
inline void initializePtrTagLookup() { }
#endif

} // namespace JSC

