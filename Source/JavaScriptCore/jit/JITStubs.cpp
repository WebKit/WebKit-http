/*
 * Copyright (C) 2008, 2009, 2013 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Cameron Zwarich <cwzwarich@uwaterloo.ca>
 * Copyright (C) Research In Motion Limited 2010, 2011. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(JIT)
#include "JITStubs.h"

#include "Arguments.h"
#include "ArrayConstructor.h"
#include "CallFrame.h"
#include "CallFrameInlines.h"
#include "CodeBlock.h"
#include "CodeProfiling.h"
#include "CommonSlowPaths.h"
#include "DFGCompilationMode.h"
#include "DFGDriver.h"
#include "DFGOSREntry.h"
#include "DFGWorklist.h"
#include "Debugger.h"
#include "DeferGC.h"
#include "ErrorInstance.h"
#include "ExceptionHelpers.h"
#include "GetterSetter.h"
#include "Heap.h"
#include <wtf/InlineASM.h>
#include "JIT.h"
#include "JITExceptions.h"
#include "JITToDFGDeferredCompilationCallback.h"
#include "JSActivation.h"
#include "JSArray.h"
#include "JSFunction.h"
#include "JSGlobalObjectFunctions.h"
#include "JSNameScope.h"
#include "JSNotAnObject.h"
#include "JSPropertyNameIterator.h"
#include "JSString.h"
#include "JSWithScope.h"
#include "LegacyProfiler.h"
#include "NameInstance.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "Parser.h"
#include "RegExpObject.h"
#include "RegExpPrototype.h"
#include "Register.h"
#include "RepatchBuffer.h"
#include "SamplingTool.h"
#include "SlowPathCall.h"
#include "Strong.h"
#include "StructureRareDataInlines.h"
#include <wtf/StdLibExtras.h>
#include <stdarg.h>
#include <stdio.h>

using namespace std;

#if CPU(ARM_TRADITIONAL)
#include "JITStubsARM.h"
#elif CPU(ARM_THUMB2)
#include "JITStubsARMv7.h"
#elif CPU(MIPS)
#include "JITStubsMIPS.h"
#elif CPU(SH4)
#include "JITStubsSH4.h"
#elif CPU(X86)
#include "JITStubsX86.h"
#elif CPU(X86_64)
#include "JITStubsX86_64.h"
#elif CPU(ARM64)
#include "JITStubsARM64.h"
#else
#error "JIT not supported on this platform."
#endif

namespace JSC {

#if ENABLE(OPCODE_SAMPLING)
    #define CTI_SAMPLER stackFrame.vm->interpreter->sampler()
#else
    #define CTI_SAMPLER 0
#endif

} // namespace JSC

#endif // ENABLE(JIT)
