/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2014 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef GetterSetter_h
#define GetterSetter_h

#include "JSCell.h"

#include "CallFrame.h"
#include "Structure.h"

namespace JSC {

class JSObject;

// This is an internal value object which stores getter and setter functions
// for a property. Instances of this class have the property that once a getter
// or setter is set to a non-null value, then they cannot be changed. This means
// that if a property holding a GetterSetter reference is constant-inferred and
// that constant is observed to have a non-null setter (or getter) then we can
// constant fold that setter (or getter).
class GetterSetter : public JSCell {
    friend class JIT;

private:        
    GetterSetter(VM& vm)
        : JSCell(vm, vm.getterSetterStructure.get())
    {
    }

public:
    typedef JSCell Base;

    static GetterSetter* create(VM& vm)
    {
        GetterSetter* getterSetter = new (NotNull, allocateCell<GetterSetter>(vm.heap)) GetterSetter(vm);
        getterSetter->finishCreation(vm);
        return getterSetter;
    }

    static void visitChildren(JSCell*, SlotVisitor&);

    JSObject* getter() const { return m_getter.get(); }

    JSObject* getterConcurrently() const
    {
        JSObject* result = getter();
        WTF::loadLoadFence();
        return result;
    }

    // Set the getter. It's only valid to call this if you've never set the getter on this
    // object.
    void setGetter(VM& vm, JSObject* getter)
    {
        RELEASE_ASSERT(!m_getter);
        WTF::storeStoreFence();
        m_getter.setMayBeNull(vm, this, getter);
    }

    JSObject* setter() const { return m_setter.get(); }

    JSObject* setterConcurrently() const
    {
        JSObject* result = setter();
        WTF::loadLoadFence();
        return result;
    }

    // Set the setter. It's only valid to call this if you've never set the setter on this
    // object.
    void setSetter(VM& vm, JSObject* setter)
    {
        RELEASE_ASSERT(!m_setter);
        WTF::storeStoreFence();
        m_setter.setMayBeNull(vm, this, setter);
    }

    GetterSetter* withGetter(VM&, JSObject* getter);
    GetterSetter* withSetter(VM&, JSObject* setter);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(GetterSetterType), info());
    }

    static ptrdiff_t offsetOfGetter()
    {
        return OBJECT_OFFSETOF(GetterSetter, m_getter);
    }

    static ptrdiff_t offsetOfSetter()
    {
        return OBJECT_OFFSETOF(GetterSetter, m_setter);
    }

    DECLARE_INFO;

private:
    WriteBarrier<JSObject> m_getter;
    WriteBarrier<JSObject> m_setter;  
};

GetterSetter* asGetterSetter(JSValue);

inline GetterSetter* asGetterSetter(JSValue value)
{
    ASSERT_WITH_SECURITY_IMPLICATION(value.asCell()->isGetterSetter());
    return static_cast<GetterSetter*>(value.asCell());
}

JSValue callGetter(ExecState*, JSValue base, JSValue getterSetter);
void callSetter(ExecState*, JSValue base, JSValue getterSetter, JSValue, ECMAMode);

} // namespace JSC

#endif // GetterSetter_h
