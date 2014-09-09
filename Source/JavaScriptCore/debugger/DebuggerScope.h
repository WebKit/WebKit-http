/*
 * Copyright (C) 2008-2009, 2014 Apple Inc. All rights reserved.
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

#ifndef DebuggerScope_h
#define DebuggerScope_h

#include "JSObject.h"

namespace JSC {

class DebuggerCallFrame;
class JSScope;

class DebuggerScope : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    static DebuggerScope* create(VM& vm, JSScope* scope)
    {
        DebuggerScope* debuggerScope = new (NotNull, allocateCell<DebuggerScope>(vm.heap)) DebuggerScope(vm, scope);
        debuggerScope->finishCreation(vm);
        return debuggerScope;
    }

    static void visitChildren(JSCell*, SlotVisitor&);
    static String className(const JSObject*);
    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);
    static void put(JSCell*, ExecState*, PropertyName, JSValue, PutPropertySlot&);
    static bool deleteProperty(JSCell*, ExecState*, PropertyName);
    static void getOwnPropertyNames(JSObject*, ExecState*, PropertyNameArray&, EnumerationMode);
    static bool defineOwnProperty(JSObject*, ExecState*, PropertyName, const PropertyDescriptor&, bool shouldThrow);

    DECLARE_EXPORT_INFO;

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject) 
    {
        return Structure::create(vm, globalObject, jsNull(), TypeInfo(ObjectType, StructureFlags), info()); 
    }

    class iterator {
    public:
        iterator(DebuggerScope* node)
            : m_node(node)
        {
        }

        DebuggerScope* get() { return m_node; }
        iterator& operator++() { m_node = m_node->next(); return *this; }
        // postfix ++ intentionally omitted

        bool operator==(const iterator& other) const { return m_node == other.m_node; }
        bool operator!=(const iterator& other) const { return m_node != other.m_node; }

    private:
        DebuggerScope* m_node;
    };

    iterator begin();
    iterator end();
    DebuggerScope* next();

    void invalidateChain();
    bool isValid() const { return !!m_scope; }

    bool isWithScope() const;
    bool isGlobalScope() const;
    bool isFunctionOrEvalScope() const;

private:
    JS_EXPORT_PRIVATE DebuggerScope(VM&, JSScope*);
    JS_EXPORT_PRIVATE void finishCreation(VM&);

    JSScope* jsScope() const { return m_scope.get(); }

    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | OverridesGetPropertyNames | JSObject::StructureFlags;

    WriteBarrier<JSScope> m_scope;
    WriteBarrier<DebuggerScope> m_next;

    friend class DebuggerCallFrame;
};

inline DebuggerScope::iterator DebuggerScope::begin()
{
    return iterator(this); 
}

inline DebuggerScope::iterator DebuggerScope::end()
{ 
    return iterator(0); 
}

} // namespace JSC

#endif // DebuggerScope_h
