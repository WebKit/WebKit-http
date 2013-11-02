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

#include "config.h"

#if ENABLE(ASSEMBLER) && CPU(ARM_THUMB2)
#include "MacroAssemblerARMv7.h"

#if USE(MASM_PROBE)
#include <wtf/StdLibExtras.h>
#endif

namespace JSC {

#if USE(MASM_PROBE)

void MacroAssemblerARMv7::ProbeContext::dumpCPURegisters(const char* indentation)
{
    #define DUMP_GPREGISTER(_type, _regName) { \
        int32_t value = reinterpret_cast<int32_t>(cpu._regName); \
        dataLogF("%s    %5s: 0x%08x   %d\n", indentation, #_regName, value, value) ; \
    }
    FOR_EACH_CPU_GPREGISTER(DUMP_GPREGISTER)
    FOR_EACH_CPU_SPECIAL_REGISTER(DUMP_GPREGISTER)
    #undef DUMP_GPREGISTER

    #define DUMP_FPREGISTER(_type, _regName) { \
        uint32_t* u = reinterpret_cast<uint32_t*>(&cpu._regName); \
        double* d = reinterpret_cast<double*>(&cpu._regName); \
        dataLogF("%s    %5s: 0x %08x %08x   %12g\n", \
            indentation, #_regName, u[1], u[0], d[0]); \
    }
    FOR_EACH_CPU_FPREGISTER(DUMP_FPREGISTER)
    #undef DUMP_FPREGISTER
}

void MacroAssemblerARMv7::ProbeContext::dump(const char* indentation)
{
    if (!indentation)
        indentation = "";

    dataLogF("%sProbeContext %p {\n", indentation, this);
    dataLogF("%s  probeFunction: %p\n", indentation, probeFunction);
    dataLogF("%s  arg1: %p %llu\n", indentation, arg1, reinterpret_cast<int64_t>(arg1));
    dataLogF("%s  arg2: %p %llu\n", indentation, arg2, reinterpret_cast<int64_t>(arg2));
    dataLogF("%s  cpu: {\n", indentation);

    dumpCPURegisters(indentation);

    dataLogF("%s  }\n", indentation);
    dataLogF("%s}\n", indentation);
}


extern "C" void ctiMasmProbeTrampoline();

// For details on "What code is emitted for the probe?" and "What values are in
// the saved registers?", see comment for MacroAssemblerX86::probe() in
// MacroAssemblerX86_64.h.

void MacroAssemblerARMv7::probe(MacroAssemblerARMv7::ProbeFunction function, void* arg1, void* arg2)
{
    push(RegisterID::sp);
    push(RegisterID::lr);
    push(RegisterID::ip);
    push(RegisterID::r0);
    // The following uses RegisterID::ip. So, they must come after we push ip above.
    push(trustedImm32FromPtr(arg2));
    push(trustedImm32FromPtr(arg1));
    push(trustedImm32FromPtr(function));

    move(trustedImm32FromPtr(ctiMasmProbeTrampoline), RegisterID::ip);
    m_assembler.blx(RegisterID::ip);
}
#endif // USE(MASM_PROBE)

} // namespace JSC

#endif // ENABLE(ASSEMBLER)

