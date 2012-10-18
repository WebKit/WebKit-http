/*
 * Copyright (C) 2012 Apple Inc. All Rights Reserved.
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

#ifndef JSScope_h
#define JSScope_h

#include "JSObject.h"

namespace JSC {

class ScopeChainIterator;

class JSScope : public JSNonFinalObject {
public:
    typedef JSNonFinalObject Base;

    friend class LLIntOffsetsExtractor;
    static size_t offsetOfNext();

    JS_EXPORT_PRIVATE static JSObject* objectAtScope(JSScope*);

    static JSValue resolve(CallFrame*, const Identifier&);
    static JSValue resolveSkip(CallFrame*, const Identifier&, int skip);
    static JSValue resolveGlobal(
        CallFrame*,
        const Identifier&,
        JSGlobalObject* globalObject,
        WriteBarrierBase<Structure>* cachedStructure,
        PropertyOffset* cachedOffset
    );
    static JSValue resolveGlobalDynamic(
        CallFrame*,
        const Identifier&,
        int skip,
        WriteBarrierBase<Structure>* cachedStructure,
        PropertyOffset* cachedOffset
    );
    static JSValue resolveBase(CallFrame*, const Identifier&, bool isStrict);
    static JSValue resolveWithBase(CallFrame*, const Identifier&, Register* base);
    static JSValue resolveWithThis(CallFrame*, const Identifier&, Register* base);

    static void visitChildren(JSCell*, SlotVisitor&);

    bool isDynamicScope(bool& requiresDynamicChecks) const;

    ScopeChainIterator begin();
    ScopeChainIterator end();
    JSScope* next();
    int localDepth();

    JSGlobalObject* globalObject();
    JSGlobalData* globalData();
    JSObject* globalThis();

protected:
    JSScope(JSGlobalData&, Structure*, JSScope* next);
    static const unsigned StructureFlags = OverridesVisitChildren | Base::StructureFlags;

private:
    WriteBarrier<JSScope> m_next;
};

inline JSScope::JSScope(JSGlobalData& globalData, Structure* structure, JSScope* next)
    : Base(globalData, structure)
    , m_next(globalData, this, next, WriteBarrier<JSScope>::MayBeNull)
{
}

class ScopeChainIterator {
public:
    ScopeChainIterator(JSScope* node)
        : m_node(node)
    {
    }

    JSObject* get() const { return JSScope::objectAtScope(m_node); }
    JSObject* operator->() const { return JSScope::objectAtScope(m_node); }

    ScopeChainIterator& operator++() { m_node = m_node->next(); return *this; }

    // postfix ++ intentionally omitted

    bool operator==(const ScopeChainIterator& other) const { return m_node == other.m_node; }
    bool operator!=(const ScopeChainIterator& other) const { return m_node != other.m_node; }

private:
    JSScope* m_node;
};

inline ScopeChainIterator JSScope::begin()
{
    return ScopeChainIterator(this); 
}

inline ScopeChainIterator JSScope::end()
{ 
    return ScopeChainIterator(0); 
}

inline JSScope* JSScope::next()
{ 
    return m_next.get();
}

inline JSGlobalObject* JSScope::globalObject()
{ 
    return structure()->globalObject();
}

inline JSGlobalData* JSScope::globalData()
{ 
    return Heap::heap(this)->globalData();
}

inline Register& Register::operator=(JSScope* scope)
{
    *this = JSValue(scope);
    return *this;
}

inline JSScope* Register::scope() const
{
    return jsCast<JSScope*>(jsValue());
}

inline JSGlobalData& ExecState::globalData() const
{
    ASSERT(scope()->globalData());
    return *scope()->globalData();
}

inline JSGlobalObject* ExecState::lexicalGlobalObject() const
{
    return scope()->globalObject();
}

inline JSObject* ExecState::globalThisValue() const
{
    return scope()->globalThis();
}

inline size_t JSScope::offsetOfNext()
{
    return OBJECT_OFFSETOF(JSScope, m_next);
}

} // namespace JSC

#endif // JSScope_h
