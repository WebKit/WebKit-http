/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
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
#include "DFGUseKind.h"

#if ENABLE(DFG_JIT)

#include "JSCInlines.h"

namespace WTF {

using namespace JSC::DFG;

void printInternal(PrintStream& out, UseKind useKind)
{
    switch (useKind) {
    case UntypedUse:
        out.print("Untyped");
        return;
    case Int32Use:
        out.print("Int32");
        return;
    case KnownInt32Use:
        out.print("KnownInt32");
        return;
    case Int52RepUse:
        out.print("Int52Rep");
        return;
    case MachineIntUse:
        out.print("MachineInt");
        return;
    case NumberUse:
        out.print("Number");
        return;
    case DoubleRepUse:
        out.print("DoubleRep");
        return;
    case DoubleRepRealUse:
        out.print("DoubleRepReal");
        return;
    case DoubleRepMachineIntUse:
        out.print("DoubleRepMachineInt");
        return;
    case BooleanUse:
        out.print("Boolean");
        return;
    case CellUse:
        out.print("Cell");
        return;
    case KnownCellUse:
        out.print("KnownCell");
        return;
    case ObjectUse:
        out.print("Object");
        return;
    case FunctionUse:
        out.print("Function");
        return;
    case FinalObjectUse:
        out.print("FinalObject");
        return;
    case ObjectOrOtherUse:
        out.print("ObjectOrOther");
        return;
    case StringIdentUse:
        out.print("StringIdent");
        return;
    case StringUse:
        out.print("String");
        return;
    case KnownStringUse:
        out.print("KnownString");
        return;
    case StringObjectUse:
        out.print("StringObject");
        return;
    case StringOrStringObjectUse:
        out.print("StringOrStringObject");
        return;
    case NotStringVarUse:
        out.print("NotStringVar");
        return;
    case NotCellUse:
        out.print("NotCell");
        return;
    case OtherUse:
        out.print("Other");
        return;
    case MiscUse:
        out.print("Misc");
        return;
    case LastUseKind:
        RELEASE_ASSERT_NOT_REACHED();
        return;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WTF

#endif // ENABLE(DFG_JIT)

