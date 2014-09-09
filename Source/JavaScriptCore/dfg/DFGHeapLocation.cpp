/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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
#include "DFGHeapLocation.h"

#if ENABLE(DFG_JIT)

namespace JSC { namespace DFG {

void HeapLocation::dump(PrintStream& out) const
{
    out.print(m_kind, ":", m_heap);
    
    if (!m_base)
        return;
    
    out.print("[", m_base);
    if (m_index)
        out.print(", ", m_index);
    out.print("]");
}

} } // namespace JSC::DFG

namespace WTF {

using namespace JSC::DFG;

void printInternal(PrintStream& out, LocationKind kind)
{
    switch (kind) {
    case InvalidLocationKind:
        out.print("InvalidLocationKind");
        return;
        
    case InvalidationPointLoc:
        out.print("InvalidationPointLoc");
        return;
        
    case IsObjectLoc:
        out.print("IsObjectLoc");
        return;
        
    case IsFunctionLoc:
        out.print("IsFunctionLoc");
        return;
        
    case TypeOfLoc:
        out.print("TypeOfLoc");
        return;
        
    case GetterLoc:
        out.print("GetterLoc");
        return;
        
    case SetterLoc:
        out.print("SetterLoc");
        return;
        
    case VariableLoc:
        out.print("VariableLoc");
        return;
        
    case ArrayLengthLoc:
        out.print("ArrayLengthLoc");
        return;
        
    case ButterflyLoc:
        out.print("ButterflyLoc");
        return;
        
    case CheckHasInstanceLoc:
        out.print("CheckHasInstanceLoc");
        return;
        
    case ClosureRegistersLoc:
        out.print("ClosureRegistersLoc");
        return;
        
    case ClosureVariableLoc:
        out.print("ClosureVariableLoc");
        return;
        
    case GlobalVariableLoc:
        out.print("GlobalVariableLoc");
        return;
        
    case HasIndexedPropertyLoc:
        out.print("HasIndexedPorpertyLoc");
        return;
        
    case IndexedPropertyLoc:
        out.print("IndexedPorpertyLoc");
        return;
        
    case IndexedPropertyStorageLoc:
        out.print("IndexedPropertyStorageLoc");
        return;
        
    case InstanceOfLoc:
        out.print("InstanceOfLoc");
        return;
        
    case MyArgumentByValLoc:
        out.print("MyArgumentByValLoc");
        return;
        
    case MyArgumentsLengthLoc:
        out.print("MyArgumentsLengthLoc");
        return;
        
    case NamedPropertyLoc:
        out.print("NamedPropertyLoc");
        return;
        
    case TypedArrayByteOffsetLoc:
        out.print("TypedArrayByteOffsetLoc");
        return;
        
    case VarInjectionWatchpointLoc:
        out.print("VarInjectionWatchpointLoc");
        return;
        
    case AllocationProfileWatchpointLoc:
        out.print("AllocationProfileWatchpointLoc");
        return;
    }
    
    RELEASE_ASSERT_NOT_REACHED();
}

} // namespace WTF

#endif // ENABLE(DFG_JIT)

