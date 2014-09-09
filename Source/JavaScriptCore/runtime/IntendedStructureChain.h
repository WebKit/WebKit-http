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

#ifndef IntendedStructureChain_h
#define IntendedStructureChain_h

#include "ConstantStructureCheck.h"
#include "Structure.h"
#include <wtf/RefCounted.h>

namespace JSC {

class CodeBlock;
class JSGlobalObject;
class StructureChain;
class VM;
struct DumpContext;

class IntendedStructureChain : public RefCounted<IntendedStructureChain> {
public:
    IntendedStructureChain(JSGlobalObject* globalObject, JSValue prototype);
    IntendedStructureChain(JSGlobalObject* globalObject, Structure*);
    IntendedStructureChain(CodeBlock* codeBlock, Structure* head, Structure* prototypeStructure);
    IntendedStructureChain(CodeBlock* codeBlock, Structure* head, StructureChain* chain);
    IntendedStructureChain(CodeBlock* codeBlock, Structure* head, StructureChain* chain, unsigned count);
    ~IntendedStructureChain();
    
    bool isStillValid() const;
    bool matches(StructureChain*) const;
    bool mayInterceptStoreTo(VM&, StringImpl* uid);
    bool isNormalized();
    
    JSValue prototype() const { return m_prototype; }
    
    size_t size() const { return m_vector.size(); }
    Structure* at(size_t index) const { return m_vector[index]; }
    Structure* operator[](size_t index) const { return at(index); }
    
    JSObject* terminalPrototype() const;
    
    Structure* last() const { return m_vector.last(); }
    
    bool operator==(const IntendedStructureChain&) const;
    bool operator!=(const IntendedStructureChain& other) const
    {
        return !(*this == other);
    }
    
    void gatherChecks(ConstantStructureCheckVector&) const;
    
    void visitChildren(SlotVisitor&);
    void dump(PrintStream&) const;
    void dumpInContext(PrintStream&, DumpContext*) const;
    
private:
    JSGlobalObject* m_globalObject;
    JSValue m_prototype;
    Vector<Structure*> m_vector;
};

} // namespace JSC

#endif // IntendedStructureChain_h
