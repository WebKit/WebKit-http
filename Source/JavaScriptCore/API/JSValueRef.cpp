/*
 * Copyright (C) 2006, 2007 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "JSValueRef.h"

#include "APICast.h"
#include "APIShims.h"
#include "JSCallbackObject.h"

#include <runtime/JSGlobalObject.h>
#include <runtime/JSONObject.h>
#include <runtime/JSString.h>
#include <runtime/LiteralParser.h>
#include <runtime/Operations.h>
#include <runtime/Protect.h>
#include <runtime/UString.h>
#include <runtime/JSValue.h>

#include <wtf/Assertions.h>
#include <wtf/text/StringHash.h>

#include <algorithm> // for std::min

using namespace JSC;

::JSType JSValueGetType(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);

    if (jsValue.isUndefined())
        return kJSTypeUndefined;
    if (jsValue.isNull())
        return kJSTypeNull;
    if (jsValue.isBoolean())
        return kJSTypeBoolean;
    if (jsValue.isNumber())
        return kJSTypeNumber;
    if (jsValue.isString())
        return kJSTypeString;
    ASSERT(jsValue.isObject());
    return kJSTypeObject;
}

bool JSValueIsUndefined(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isUndefined();
}

bool JSValueIsNull(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isNull();
}

bool JSValueIsBoolean(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isBoolean();
}

bool JSValueIsNumber(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isNumber();
}

bool JSValueIsString(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isString();
}

bool JSValueIsObject(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.isObject();
}

bool JSValueIsObjectOfClass(JSContextRef ctx, JSValueRef value, JSClassRef jsClass)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    
    if (JSObject* o = jsValue.getObject()) {
        if (o->inherits(&JSCallbackObject<JSGlobalObject>::s_info))
            return static_cast<JSCallbackObject<JSGlobalObject>*>(o)->inherits(jsClass);
        if (o->inherits(&JSCallbackObject<JSNonFinalObject>::s_info))
            return static_cast<JSCallbackObject<JSNonFinalObject>*>(o)->inherits(jsClass);
    }
    return false;
}

bool JSValueIsEqual(JSContextRef ctx, JSValueRef a, JSValueRef b, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsA = toJS(exec, a);
    JSValue jsB = toJS(exec, b);

    bool result = JSValue::equal(exec, jsA, jsB); // false if an exception is thrown
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

bool JSValueIsStrictEqual(JSContextRef ctx, JSValueRef a, JSValueRef b)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsA = toJS(exec, a);
    JSValue jsB = toJS(exec, b);

    return JSValue::strictEqual(exec, jsA, jsB);
}

bool JSValueIsInstanceOfConstructor(JSContextRef ctx, JSValueRef value, JSObjectRef constructor, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);

    JSObject* jsConstructor = toJS(constructor);
    if (!jsConstructor->structure()->typeInfo().implementsHasInstance())
        return false;
    bool result = jsConstructor->methodTable()->hasInstance(jsConstructor, exec, jsValue, jsConstructor->get(exec, exec->propertyNames().prototype)); // false if an exception is thrown
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
    }
    return result;
}

JSValueRef JSValueMakeUndefined(JSContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsUndefined());
}

JSValueRef JSValueMakeNull(JSContextRef ctx)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsNull());
}

JSValueRef JSValueMakeBoolean(JSContextRef ctx, bool value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsBoolean(value));
}

JSValueRef JSValueMakeNumber(JSContextRef ctx, double value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    // Our JSValue representation relies on a standard bit pattern for NaN. NaNs
    // generated internally to JavaScriptCore naturally have that representation,
    // but an external NaN might not.
    if (isnan(value))
        value = std::numeric_limits<double>::quiet_NaN();

    return toRef(exec, jsNumber(value));
}

JSValueRef JSValueMakeString(JSContextRef ctx, JSStringRef string)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    return toRef(exec, jsString(exec, string->ustring()));
}

JSValueRef JSValueMakeFromJSONString(JSContextRef ctx, JSStringRef string)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    UString str = string->ustring();
    if (str.is8Bit()) {
        LiteralParser<LChar> parser(exec, str.characters8(), str.length(), StrictJSON);
        return toRef(exec, parser.tryLiteralParse());
    }
    LiteralParser<UChar> parser(exec, str.characters16(), str.length(), StrictJSON);
    return toRef(exec, parser.tryLiteralParse());
}

JSStringRef JSValueCreateJSONString(JSContextRef ctx, JSValueRef apiValue, unsigned indent, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);
    JSValue value = toJS(exec, apiValue);
    UString result = JSONStringify(exec, value, indent);
    if (exception)
        *exception = 0;
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        return 0;
    }
    return OpaqueJSString::create(result).leakRef();
}

bool JSValueToBoolean(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    return jsValue.toBoolean(exec);
}

double JSValueToNumber(JSContextRef ctx, JSValueRef value, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);

    double number = jsValue.toNumber(exec);
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        number = std::numeric_limits<double>::quiet_NaN();
    }
    return number;
}

JSStringRef JSValueToStringCopy(JSContextRef ctx, JSValueRef value, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    
    RefPtr<OpaqueJSString> stringRef(OpaqueJSString::create(jsValue.toString(exec)));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        stringRef.clear();
    }
    return stringRef.release().leakRef();
}

JSObjectRef JSValueToObject(JSContextRef ctx, JSValueRef value, JSValueRef* exception)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJS(exec, value);
    
    JSObjectRef objectRef = toRef(jsValue.toObject(exec));
    if (exec->hadException()) {
        if (exception)
            *exception = toRef(exec, exec->exception());
        exec->clearException();
        objectRef = 0;
    }
    return objectRef;
}    

void JSValueProtect(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJSForGC(exec, value);
    gcProtect(jsValue);
}

void JSValueUnprotect(JSContextRef ctx, JSValueRef value)
{
    ExecState* exec = toJS(ctx);
    APIEntryShim entryShim(exec);

    JSValue jsValue = toJSForGC(exec, value);
    gcUnprotect(jsValue);
}
