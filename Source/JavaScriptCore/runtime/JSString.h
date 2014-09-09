/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2014 Apple Inc. All rights reserved.
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

#ifndef JSString_h
#define JSString_h

#include "CallFrame.h"
#include "CommonIdentifiers.h"
#include "Identifier.h"
#include "PropertyDescriptor.h"
#include "PropertySlot.h"
#include "Structure.h"
#include <array>

namespace JSC {

class JSString;
class JSRopeString;
class LLIntOffsetsExtractor;

JSString* jsEmptyString(VM*);
JSString* jsEmptyString(ExecState*);
JSString* jsString(VM*, const String&); // returns empty string if passed null string
JSString* jsString(ExecState*, const String&); // returns empty string if passed null string

JSString* jsSingleCharacterString(VM*, UChar);
JSString* jsSingleCharacterString(ExecState*, UChar);
JSString* jsSingleCharacterSubstring(ExecState*, const String&, unsigned offset);
JSString* jsSubstring(VM*, const String&, unsigned offset, unsigned length);
JSString* jsSubstring(ExecState*, const String&, unsigned offset, unsigned length);

// Non-trivial strings are two or more characters long.
// These functions are faster than just calling jsString.
JSString* jsNontrivialString(VM*, const String&);
JSString* jsNontrivialString(ExecState*, const String&);

// Should be used for strings that are owned by an object that will
// likely outlive the JSValue this makes, such as the parse tree or a
// DOM object that contains a String
JSString* jsOwnedString(VM*, const String&);
JSString* jsOwnedString(ExecState*, const String&);

JSRopeString* jsStringBuilder(VM*);

class JSString : public JSCell {
public:
    friend class JIT;
    friend class VM;
    friend class SpecializedThunkJIT;
    friend class JSRopeString;
    friend class MarkStack;
    friend class SlotVisitor;
    friend struct ThunkHelpers;

    typedef JSCell Base;

    static const bool needsDestruction = true;
    static const bool hasImmortalStructure = true;
    static void destroy(JSCell*);

private:
    JSString(VM& vm, PassRefPtr<StringImpl> value)
        : JSCell(vm, vm.stringStructure.get())
        , m_flags(0)
        , m_value(value)
    {
    }

    JSString(VM& vm)
        : JSCell(vm, vm.stringStructure.get())
        , m_flags(0)
    {
    }

    void finishCreation(VM& vm, size_t length)
    {
        ASSERT(!m_value.isNull());
        Base::finishCreation(vm);
        m_length = length;
        setIs8Bit(m_value.impl()->is8Bit());
        vm.m_newStringsSinceLastHashCons++;
    }

    void finishCreation(VM& vm, size_t length, size_t cost)
    {
        ASSERT(!m_value.isNull());
        Base::finishCreation(vm);
        m_length = length;
        setIs8Bit(m_value.impl()->is8Bit());
        Heap::heap(this)->reportExtraMemoryCost(cost);
        vm.m_newStringsSinceLastHashCons++;
    }

protected:
    void finishCreation(VM& vm)
    {
        Base::finishCreation(vm);
        m_length = 0;
        setIs8Bit(true);
        vm.m_newStringsSinceLastHashCons++;
    }

public:
    static JSString* create(VM& vm, PassRefPtr<StringImpl> value)
    {
        ASSERT(value);
        int32_t length = value->length();
        RELEASE_ASSERT(length >= 0);
        size_t cost = value->cost();
        JSString* newString = new (NotNull, allocateCell<JSString>(vm.heap)) JSString(vm, value);
        newString->finishCreation(vm, length, cost);
        return newString;
    }
    static JSString* createHasOtherOwner(VM& vm, PassRefPtr<StringImpl> value)
    {
        ASSERT(value);
        size_t length = value->length();
        JSString* newString = new (NotNull, allocateCell<JSString>(vm.heap)) JSString(vm, value);
        newString->finishCreation(vm, length);
        return newString;
    }

    Identifier toIdentifier(ExecState*) const;
    AtomicString toAtomicString(ExecState*) const;
    AtomicStringImpl* toExistingAtomicString(ExecState*) const;
    const String& value(ExecState*) const;
    const String& tryGetValue() const;
    const StringImpl* tryGetValueImpl() const;
    unsigned length() const { return m_length; }

    JSValue toPrimitive(ExecState*, PreferredPrimitiveType) const;
    JS_EXPORT_PRIVATE bool toBoolean() const;
    bool getPrimitiveNumber(ExecState*, double& number, JSValue&) const;
    JSObject* toObject(ExecState*, JSGlobalObject*) const;
    double toNumber(ExecState*) const;

    bool getStringPropertySlot(ExecState*, PropertyName, PropertySlot&);
    bool getStringPropertySlot(ExecState*, unsigned propertyName, PropertySlot&);
    bool getStringPropertyDescriptor(ExecState*, PropertyName, PropertyDescriptor&);

    bool canGetIndex(unsigned i) { return i < m_length; }
    JSString* getIndex(ExecState*, unsigned);

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue proto)
    {
        return Structure::create(vm, globalObject, proto, TypeInfo(StringType, StructureFlags), info());
    }

    static size_t offsetOfLength() { return OBJECT_OFFSETOF(JSString, m_length); }
    static size_t offsetOfFlags() { return OBJECT_OFFSETOF(JSString, m_flags); }
    static size_t offsetOfValue() { return OBJECT_OFFSETOF(JSString, m_value); }

    DECLARE_EXPORT_INFO;

    static void dumpToStream(const JSCell*, PrintStream&);
    static void visitChildren(JSCell*, SlotVisitor&);

    enum {
        HashConsLock = 1u << 2,
        IsHashConsSingleton = 1u << 1,
        Is8Bit = 1u
    };

protected:
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InterceptsGetOwnPropertySlotByIndexEvenWhenLengthIsNotZero | StructureIsImmortal;

    friend class JSValue;

    bool isRope() const { return m_value.isNull(); }
    bool is8Bit() const { return m_flags & Is8Bit; }
    void setIs8Bit(bool flag) const
    {
        if (flag)
            m_flags |= Is8Bit;
        else
            m_flags &= ~Is8Bit;
    }
    bool shouldTryHashCons();
    bool isHashConsSingleton() const { return m_flags & IsHashConsSingleton; }
    void clearHashConsSingleton() { m_flags &= ~IsHashConsSingleton; }
    void setHashConsSingleton() { m_flags |= IsHashConsSingleton; }
    bool tryHashConsLock();
    void releaseHashConsLock();

    mutable unsigned m_flags;

    // A string is represented either by a String or a rope of fibers.
    unsigned m_length;
    mutable String m_value;

private:
    friend class LLIntOffsetsExtractor;

    static JSValue toThis(JSCell*, ExecState*, ECMAMode);

    String& string() { ASSERT(!isRope()); return m_value; }

    friend JSValue jsString(ExecState*, JSString*, JSString*);
    friend JSString* jsSubstring(ExecState*, JSString*, unsigned offset, unsigned length);
};

class JSRopeString : public JSString {
    friend class JSString;

    friend JSRopeString* jsStringBuilder(VM*);

    class RopeBuilder {
    public:
        RopeBuilder(VM& vm)
            : m_vm(vm)
            , m_jsString(jsStringBuilder(&vm))
            , m_index(0)
        {
        }

        bool append(JSString* jsString)
        {
            if (m_index == JSRopeString::s_maxInternalRopeLength)
                expand();
            if (static_cast<int32_t>(m_jsString->length() + jsString->length()) < 0) {
                m_jsString = nullptr;
                return false;
            }
            m_jsString->append(m_vm, m_index++, jsString);
            return true;
        }

        JSRopeString* release()
        {
            RELEASE_ASSERT(m_jsString);
            JSRopeString* tmp = m_jsString;
            m_jsString = 0;
            return tmp;
        }

        unsigned length() const { return m_jsString->m_length; }

    private:
        void expand();

        VM& m_vm;
        JSRopeString* m_jsString;
        size_t m_index;
    };

private:
    JSRopeString(VM& vm)
        : JSString(vm)
    {
    }

    void finishCreation(VM& vm, JSString* s1, JSString* s2)
    {
        Base::finishCreation(vm);
        m_length = s1->length() + s2->length();
        setIs8Bit(s1->is8Bit() && s2->is8Bit());
        setIsSubstring(false);
        fiber(0).set(vm, this, s1);
        fiber(1).set(vm, this, s2);
        fiber(2).clear();
    }

    void finishCreation(VM& vm, JSString* s1, JSString* s2, JSString* s3)
    {
        Base::finishCreation(vm);
        m_length = s1->length() + s2->length() + s3->length();
        setIs8Bit(s1->is8Bit() && s2->is8Bit() &&  s3->is8Bit());
        setIsSubstring(false);
        fiber(0).set(vm, this, s1);
        fiber(1).set(vm, this, s2);
        fiber(2).set(vm, this, s3);
    }

    void finishCreation(VM& vm, JSString* base, unsigned offset, unsigned length)
    {
        Base::finishCreation(vm);
        ASSERT(!base->isRope());
        ASSERT(!sumOverflows<int32_t>(offset, length));
        ASSERT(offset + length <= base->length());
        m_length = length;
        setIs8Bit(base->is8Bit());
        setIsSubstring(true);
        substringBase().set(vm, this, base);
        substringOffset() = offset;
    }

    void finishCreation(VM& vm)
    {
        JSString::finishCreation(vm);
        setIsSubstring(false);
        fiber(0).clear();
        fiber(1).clear();
        fiber(2).clear();
    }

    void append(VM& vm, size_t index, JSString* jsString)
    {
        fiber(index).set(vm, this, jsString);
        m_length += jsString->m_length;
        RELEASE_ASSERT(static_cast<int32_t>(m_length) >= 0);
        setIs8Bit(is8Bit() && jsString->is8Bit());
    }

    static JSRopeString* createNull(VM& vm)
    {
        JSRopeString* newString = new (NotNull, allocateCell<JSRopeString>(vm.heap)) JSRopeString(vm);
        newString->finishCreation(vm);
        return newString;
    }

public:
    static JSString* create(VM& vm, JSString* s1, JSString* s2)
    {
        JSRopeString* newString = new (NotNull, allocateCell<JSRopeString>(vm.heap)) JSRopeString(vm);
        newString->finishCreation(vm, s1, s2);
        return newString;
    }
    static JSString* create(VM& vm, JSString* s1, JSString* s2, JSString* s3)
    {
        JSRopeString* newString = new (NotNull, allocateCell<JSRopeString>(vm.heap)) JSRopeString(vm);
        newString->finishCreation(vm, s1, s2, s3);
        return newString;
    }

    static JSString* create(VM& vm, JSString* base, unsigned offset, unsigned length)
    {
        JSRopeString* newString = new (NotNull, allocateCell<JSRopeString>(vm.heap)) JSRopeString(vm);
        newString->finishCreation(vm, base, offset, length);
        return newString;
    }

    void visitFibers(SlotVisitor&);

    static ptrdiff_t offsetOfFibers() { return OBJECT_OFFSETOF(JSRopeString, u); }

    static const unsigned s_maxInternalRopeLength = 3;

private:
    friend JSValue jsStringFromRegisterArray(ExecState*, Register*, unsigned);
    friend JSValue jsStringFromArguments(ExecState*, JSValue);

    JS_EXPORT_PRIVATE void resolveRope(ExecState*) const;
    JS_EXPORT_PRIVATE void resolveRopeToAtomicString(ExecState*) const;
    JS_EXPORT_PRIVATE AtomicStringImpl* resolveRopeToExistingAtomicString(ExecState*) const;
    void resolveRopeSlowCase8(LChar*) const;
    void resolveRopeSlowCase(UChar*) const;
    void outOfMemory(ExecState*) const;
    void resolveRopeInternal8(LChar*) const;
    void resolveRopeInternal8NoSubstring(LChar*) const;
    void resolveRopeInternal16(UChar*) const;
    void resolveRopeInternal16NoSubstring(UChar*) const;
    void clearFibers() const;

    JS_EXPORT_PRIVATE JSString* getIndexSlowCase(ExecState*, unsigned);

    WriteBarrierBase<JSString>& fiber(unsigned i) const
    {
        ASSERT(!isSubstring());
        ASSERT(i < s_maxInternalRopeLength);
        return u[i].string;
    }

    WriteBarrierBase<JSString>& substringBase() const
    {
        return u[1].string;
    }

    uintptr_t& substringOffset() const
    {
        return u[2].number;
    }

    static uintptr_t notSubstringSentinel()
    {
        return 0;
    }

    static uintptr_t substringSentinel()
    {
        return 1;
    }

    bool isSubstring() const
    {
        return u[0].number == substringSentinel();
    }

    void setIsSubstring(bool isSubstring)
    {
        u[0].number = isSubstring ? substringSentinel() : notSubstringSentinel();
    }

    mutable union {
        uintptr_t number;
        WriteBarrierBase<JSString> string;
    } u[s_maxInternalRopeLength];
};


inline const StringImpl* JSString::tryGetValueImpl() const
{
    return m_value.impl();
}

JSString* asString(JSValue);

inline JSString* asString(JSValue value)
{
    ASSERT(value.asCell()->isString());
    return jsCast<JSString*>(value.asCell());
}

inline JSString* jsEmptyString(VM* vm)
{
    return vm->smallStrings.emptyString();
}

ALWAYS_INLINE JSString* jsSingleCharacterString(VM* vm, UChar c)
{
    if (c <= maxSingleCharacterString)
        return vm->smallStrings.singleCharacterString(c);
    return JSString::create(*vm, String(&c, 1).impl());
}

ALWAYS_INLINE JSString* jsSingleCharacterSubstring(ExecState* exec, const String& s, unsigned offset)
{
    VM* vm = &exec->vm();
    ASSERT(offset < static_cast<unsigned>(s.length()));
    UChar c = s.characterAt(offset);
    if (c <= maxSingleCharacterString)
        return vm->smallStrings.singleCharacterString(c);
    return JSString::create(*vm, StringImpl::createSubstringSharingImpl(s.impl(), offset, 1));
}

inline JSString* jsNontrivialString(VM* vm, const String& s)
{
    ASSERT(s.length() > 1);
    return JSString::create(*vm, s.impl());
}

ALWAYS_INLINE Identifier JSString::toIdentifier(ExecState* exec) const
{
    return Identifier(exec, toAtomicString(exec));
}

ALWAYS_INLINE AtomicString JSString::toAtomicString(ExecState* exec) const
{
    if (isRope())
        static_cast<const JSRopeString*>(this)->resolveRopeToAtomicString(exec);
    return AtomicString(m_value);
}

ALWAYS_INLINE AtomicStringImpl* JSString::toExistingAtomicString(ExecState* exec) const
{
    if (isRope())
        return static_cast<const JSRopeString*>(this)->resolveRopeToExistingAtomicString(exec);
    if (m_value.impl()->isAtomic())
        return static_cast<AtomicStringImpl*>(m_value.impl());
    if (AtomicStringImpl* existingAtomicString = AtomicString::find(m_value.impl())) {
        m_value = *existingAtomicString;
        setIs8Bit(m_value.impl()->is8Bit());
        return existingAtomicString;
    }
    return nullptr;
}

inline const String& JSString::value(ExecState* exec) const
{
    if (isRope())
        static_cast<const JSRopeString*>(this)->resolveRope(exec);
    return m_value;
}

inline const String& JSString::tryGetValue() const
{
    if (isRope())
        static_cast<const JSRopeString*>(this)->resolveRope(0);
    return m_value;
}

inline JSString* JSString::getIndex(ExecState* exec, unsigned i)
{
    ASSERT(canGetIndex(i));
    if (isRope())
        return static_cast<JSRopeString*>(this)->getIndexSlowCase(exec, i);
    ASSERT(i < m_value.length());
    return jsSingleCharacterSubstring(exec, m_value, i);
}

inline JSString* jsString(VM* vm, const String& s)
{
    int size = s.length();
    if (!size)
        return vm->smallStrings.emptyString();
    if (size == 1) {
        UChar c = s.characterAt(0);
        if (c <= maxSingleCharacterString)
            return vm->smallStrings.singleCharacterString(c);
    }
    return JSString::create(*vm, s.impl());
}

inline JSString* jsSubstring(ExecState* exec, JSString* s, unsigned offset, unsigned length)
{
    ASSERT(offset <= static_cast<unsigned>(s->length()));
    ASSERT(length <= static_cast<unsigned>(s->length()));
    ASSERT(offset + length <= static_cast<unsigned>(s->length()));
    VM& vm = exec->vm();
    if (!length)
        return vm.smallStrings.emptyString();
    s->value(exec); // For effect. We need to ensure that any string that is used as a substring base is not a rope.
    return JSRopeString::create(vm, s, offset, length);
}

inline JSString* jsSubstring8(VM* vm, const String& s, unsigned offset, unsigned length)
{
    ASSERT(offset <= static_cast<unsigned>(s.length()));
    ASSERT(length <= static_cast<unsigned>(s.length()));
    ASSERT(offset + length <= static_cast<unsigned>(s.length()));
    if (!length)
        return vm->smallStrings.emptyString();
    if (length == 1) {
        UChar c = s.characterAt(offset);
        if (c <= maxSingleCharacterString)
            return vm->smallStrings.singleCharacterString(c);
    }
    return JSString::createHasOtherOwner(*vm, StringImpl::createSubstringSharingImpl8(s.impl(), offset, length));
}

inline JSString* jsSubstring(VM* vm, const String& s, unsigned offset, unsigned length)
{
    ASSERT(offset <= static_cast<unsigned>(s.length()));
    ASSERT(length <= static_cast<unsigned>(s.length()));
    ASSERT(offset + length <= static_cast<unsigned>(s.length()));
    if (!length)
        return vm->smallStrings.emptyString();
    if (length == 1) {
        UChar c = s.characterAt(offset);
        if (c <= maxSingleCharacterString)
            return vm->smallStrings.singleCharacterString(c);
    }
    return JSString::createHasOtherOwner(*vm, StringImpl::createSubstringSharingImpl(s.impl(), offset, length));
}

inline JSString* jsOwnedString(VM* vm, const String& s)
{
    int size = s.length();
    if (!size)
        return vm->smallStrings.emptyString();
    if (size == 1) {
        UChar c = s.characterAt(0);
        if (c <= maxSingleCharacterString)
            return vm->smallStrings.singleCharacterString(c);
    }
    return JSString::createHasOtherOwner(*vm, s.impl());
}

inline JSRopeString* jsStringBuilder(VM* vm)
{
    return JSRopeString::createNull(*vm);
}

inline JSString* jsEmptyString(ExecState* exec) { return jsEmptyString(&exec->vm()); }
inline JSString* jsString(ExecState* exec, const String& s) { return jsString(&exec->vm(), s); }
inline JSString* jsSingleCharacterString(ExecState* exec, UChar c) { return jsSingleCharacterString(&exec->vm(), c); }
inline JSString* jsSubstring8(ExecState* exec, const String& s, unsigned offset, unsigned length) { return jsSubstring8(&exec->vm(), s, offset, length); }
inline JSString* jsSubstring(ExecState* exec, const String& s, unsigned offset, unsigned length) { return jsSubstring(&exec->vm(), s, offset, length); }
inline JSString* jsNontrivialString(ExecState* exec, const String& s) { return jsNontrivialString(&exec->vm(), s); }
inline JSString* jsOwnedString(ExecState* exec, const String& s) { return jsOwnedString(&exec->vm(), s); }

JS_EXPORT_PRIVATE JSString* jsStringWithCacheSlowCase(VM&, StringImpl&);

ALWAYS_INLINE JSString* jsStringWithCache(ExecState* exec, const String& s)
{
    VM& vm = exec->vm();
    StringImpl* stringImpl = s.impl();
    if (!stringImpl || !stringImpl->length())
        return jsEmptyString(&vm);

    if (stringImpl->length() == 1) {
        UChar singleCharacter = (*stringImpl)[0u];
        if (singleCharacter <= maxSingleCharacterString)
            return vm.smallStrings.singleCharacterString(static_cast<unsigned char>(singleCharacter));
    }

    if (JSString* lastCachedString = vm.lastCachedString.get()) {
        if (lastCachedString->tryGetValueImpl() == stringImpl)
            return lastCachedString;
    }

    return jsStringWithCacheSlowCase(vm, *stringImpl);
}

ALWAYS_INLINE JSString* jsStringWithCache(ExecState* exec, const AtomicString& s)
{
    return jsStringWithCache(exec, s.string());
}

ALWAYS_INLINE bool JSString::getStringPropertySlot(ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    if (propertyName == exec->propertyNames().length) {
        slot.setValue(this, DontEnum | DontDelete | ReadOnly, jsNumber(m_length));
        return true;
    }

    unsigned i = propertyName.asIndex();
    if (i < m_length) {
        ASSERT(i != PropertyName::NotAnIndex); // No need for an explicit check, the above test would always fail!
        slot.setValue(this, DontDelete | ReadOnly, getIndex(exec, i));
        return true;
    }

    return false;
}

ALWAYS_INLINE bool JSString::getStringPropertySlot(ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    if (propertyName < m_length) {
        slot.setValue(this, DontDelete | ReadOnly, getIndex(exec, propertyName));
        return true;
    }

    return false;
}

inline bool isJSString(JSValue v) { return v.isCell() && v.asCell()->type() == StringType; }

// --- JSValue inlines ----------------------------

inline bool JSValue::toBoolean(ExecState* exec) const
{
    if (isInt32())
        return asInt32();
    if (isDouble())
        return asDouble() > 0.0 || asDouble() < 0.0; // false for NaN
    if (isCell())
        return asCell()->toBoolean(exec);
    return isTrue(); // false, null, and undefined all convert to false.
}

inline JSString* JSValue::toString(ExecState* exec) const
{
    if (isString())
        return jsCast<JSString*>(asCell());
    return toStringSlowCase(exec);
}

inline String JSValue::toWTFString(ExecState* exec) const
{
    if (isString())
        return static_cast<JSString*>(asCell())->value(exec);
    return toWTFStringSlowCase(exec);
}

ALWAYS_INLINE String inlineJSValueNotStringtoString(const JSValue& value, ExecState* exec)
{
    VM& vm = exec->vm();
    if (value.isInt32())
        return vm.numericStrings.add(value.asInt32());
    if (value.isDouble())
        return vm.numericStrings.add(value.asDouble());
    if (value.isTrue())
        return vm.propertyNames->trueKeyword.string();
    if (value.isFalse())
        return vm.propertyNames->falseKeyword.string();
    if (value.isNull())
        return vm.propertyNames->nullKeyword.string();
    if (value.isUndefined())
        return vm.propertyNames->undefinedKeyword.string();
    return value.toString(exec)->value(exec);
}

ALWAYS_INLINE String JSValue::toWTFStringInline(ExecState* exec) const
{
    if (isString())
        return static_cast<JSString*>(asCell())->value(exec);

    return inlineJSValueNotStringtoString(*this, exec);
}

} // namespace JSC

#endif // JSString_h
