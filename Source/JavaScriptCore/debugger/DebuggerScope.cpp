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

#include "config.h"
#include "DebuggerScope.h"

#include "JSActivation.h"
#include "JSCInlines.h"
#include "JSWithScope.h"

namespace JSC {

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(DebuggerScope);

const ClassInfo DebuggerScope::s_info = { "DebuggerScope", &Base::s_info, 0, CREATE_METHOD_TABLE(DebuggerScope) };

DebuggerScope::DebuggerScope(VM& vm, JSScope* scope)
    : JSNonFinalObject(vm, scope->globalObject()->debuggerScopeStructure())
{
    ASSERT(scope);
    m_scope.set(vm, this, scope);
}

void DebuggerScope::finishCreation(VM& vm)
{
    Base::finishCreation(vm);
}

void DebuggerScope::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    DebuggerScope* thisObject = jsCast<DebuggerScope*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    JSObject::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_scope);
    visitor.append(&thisObject->m_next);
}

String DebuggerScope::className(const JSObject* object)
{
    const DebuggerScope* scope = jsCast<const DebuggerScope*>(object);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return String();
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    return thisObject->methodTable()->className(thisObject);
}

bool DebuggerScope::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    DebuggerScope* scope = jsCast<DebuggerScope*>(object);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return false;
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    slot.setThisValue(JSValue(thisObject));

    // By default, JSObject::getPropertySlot() will look in the DebuggerScope's prototype
    // chain and not the wrapped scope, and JSObject::getPropertySlot() cannot be overridden
    // to behave differently for the DebuggerScope.
    //
    // Instead, we'll treat all properties in the wrapped scope and its prototype chain as
    // the own properties of the DebuggerScope. This is fine because the WebInspector
    // does not presently need to distinguish between what's owned at each level in the
    // prototype chain. Hence, we'll invoke getPropertySlot() on the wrapped scope here
    // instead of getOwnPropertySlot().
    return thisObject->getPropertySlot(exec, propertyName, slot);
}

void DebuggerScope::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    DebuggerScope* scope = jsCast<DebuggerScope*>(cell);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return;
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    slot.setThisValue(JSValue(thisObject));
    thisObject->methodTable()->put(thisObject, exec, propertyName, value, slot);
}

bool DebuggerScope::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    DebuggerScope* scope = jsCast<DebuggerScope*>(cell);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return false;
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    return thisObject->methodTable()->deleteProperty(thisObject, exec, propertyName);
}

void DebuggerScope::getOwnPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    DebuggerScope* scope = jsCast<DebuggerScope*>(object);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return;
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    thisObject->methodTable()->getPropertyNames(thisObject, exec, propertyNames, mode);
}

bool DebuggerScope::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, const PropertyDescriptor& descriptor, bool shouldThrow)
{
    DebuggerScope* scope = jsCast<DebuggerScope*>(object);
    ASSERT(scope->isValid());
    if (!scope->isValid())
        return false;
    JSObject* thisObject = JSScope::objectAtScope(scope->jsScope());
    return thisObject->methodTable()->defineOwnProperty(thisObject, exec, propertyName, descriptor, shouldThrow);
}

DebuggerScope* DebuggerScope::next()
{
    ASSERT(isValid());
    if (!m_next && m_scope->next()) {
        VM& vm = *m_scope->vm();
        DebuggerScope* nextScope = create(vm, m_scope->next());
        m_next.set(vm, this, nextScope);
    }
    return m_next.get();
}

void DebuggerScope::invalidateChain()
{
    DebuggerScope* scope = this;
    while (scope) {
        ASSERT(scope->isValid());
        DebuggerScope* nextScope = scope->m_next.get();
        scope->m_next.clear();
        scope->m_scope.clear();
        scope = nextScope;
    }
}

bool DebuggerScope::isWithScope() const
{
    return m_scope->isWithScope();
}

bool DebuggerScope::isGlobalScope() const
{
    return m_scope->isGlobalObject();
}

bool DebuggerScope::isFunctionOrEvalScope() const
{
    // In the current debugger implementation, every function or eval will create an
    // activation object. Hence, an activation object implies a function or eval scope.
    return m_scope->isActivationObject();
}

} // namespace JSC
