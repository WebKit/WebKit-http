/*
 *  Copyright (C) 1999-2000 Harri Porten (porten@kde.org)
 *  Copyright (C) 2008, 2011 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include "config.h"
#include "ObjectPrototype.h"

#include "Error.h"
#include "GetterSetter.h"
#include "JSFunction.h"
#include "JSString.h"
#include "JSCInlines.h"
#include "StructureRareDataInlines.h"

namespace JSC {

static EncodedJSValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState*);
static EncodedJSValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState*);

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(ObjectPrototype);

const ClassInfo ObjectPrototype::s_info = { "Object", &JSNonFinalObject::s_info, 0, CREATE_METHOD_TABLE(ObjectPrototype) };

ObjectPrototype::ObjectPrototype(VM& vm, Structure* stucture)
    : JSNonFinalObject(vm, stucture)
{
}

void ObjectPrototype::finishCreation(VM& vm, JSGlobalObject* globalObject)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    vm.prototypeMap.addPrototype(this);
    
    JSC_NATIVE_FUNCTION(vm.propertyNames->toString, objectProtoFuncToString, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->toLocaleString, objectProtoFuncToLocaleString, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->valueOf, objectProtoFuncValueOf, DontEnum, 0);
    JSC_NATIVE_FUNCTION(vm.propertyNames->hasOwnProperty, objectProtoFuncHasOwnProperty, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->propertyIsEnumerable, objectProtoFuncPropertyIsEnumerable, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->isPrototypeOf, objectProtoFuncIsPrototypeOf, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__defineGetter__, objectProtoFuncDefineGetter, DontEnum, 2);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__defineSetter__, objectProtoFuncDefineSetter, DontEnum, 2);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__lookupGetter__, objectProtoFuncLookupGetter, DontEnum, 1);
    JSC_NATIVE_FUNCTION(vm.propertyNames->__lookupSetter__, objectProtoFuncLookupSetter, DontEnum, 1);
}

ObjectPrototype* ObjectPrototype::create(VM& vm, JSGlobalObject* globalObject, Structure* structure)
{
    ObjectPrototype* prototype = new (NotNull, allocateCell<ObjectPrototype>(vm.heap)) ObjectPrototype(vm, structure);
    prototype->finishCreation(vm, globalObject);
    return prototype;
}

// ------------------------------ Functions --------------------------------

EncodedJSValue JSC_HOST_CALL objectProtoFuncValueOf(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);
    return JSValue::encode(thisValue.toObject(exec));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncHasOwnProperty(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);
    return JSValue::encode(jsBoolean(thisValue.toObject(exec)->hasOwnProperty(exec, exec->argument(0).toString(exec)->toIdentifier(exec))));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncIsPrototypeOf(ExecState* exec)
{
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);
    JSObject* thisObj = thisValue.toObject(exec);

    if (!exec->argument(0).isObject())
        return JSValue::encode(jsBoolean(false));

    JSValue v = asObject(exec->argument(0))->prototype();

    while (true) {
        if (!v.isObject())
            return JSValue::encode(jsBoolean(false));
        if (v == thisObj)
            return JSValue::encode(jsBoolean(true));
        v = asObject(v)->prototype();
    }
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineGetter(ExecState* exec)
{
    JSObject* thisObject = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue get = exec->argument(1);
    CallData callData;
    if (getCallData(get, callData) == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("invalid getter usage")));

    PropertyDescriptor descriptor;
    descriptor.setGetter(get);
    descriptor.setEnumerable(true);
    descriptor.setConfigurable(true);
    thisObject->methodTable(exec->vm())->defineOwnProperty(thisObject, exec, exec->argument(0).toString(exec)->toIdentifier(exec), descriptor, false);

    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncDefineSetter(ExecState* exec)
{
    JSObject* thisObject = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    JSValue set = exec->argument(1);
    CallData callData;
    if (getCallData(set, callData) == CallTypeNone)
        return throwVMError(exec, createTypeError(exec, ASCIILiteral("invalid setter usage")));

    PropertyDescriptor descriptor;
    descriptor.setSetter(set);
    descriptor.setEnumerable(true);
    descriptor.setConfigurable(true);
    thisObject->methodTable(exec->vm())->defineOwnProperty(thisObject, exec, exec->argument(0).toString(exec)->toIdentifier(exec), descriptor, false);

    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupGetter(ExecState* exec)
{
    JSObject* thisObject = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    PropertySlot slot(thisObject);
    if (thisObject->getPropertySlot(exec, exec->argument(0).toString(exec)->toIdentifier(exec), slot)
        && slot.isAccessor()) {
        JSObject* getter = slot.getterSetter()->getter();
        return getter ? JSValue::encode(getter) : JSValue::encode(jsUndefined());
    }

    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncLookupSetter(ExecState* exec)
{
    JSObject* thisObject = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    PropertySlot slot(thisObject);
    if (thisObject->getPropertySlot(exec, exec->argument(0).toString(exec)->toIdentifier(exec), slot)
        && slot.isAccessor()) {
        JSObject* setter = slot.getterSetter()->setter();
        return setter ? JSValue::encode(setter) : JSValue::encode(jsUndefined());
    }

    return JSValue::encode(jsUndefined());
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncPropertyIsEnumerable(ExecState* exec)
{
    JSObject* thisObject = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    Identifier propertyName = exec->argument(0).toString(exec)->toIdentifier(exec);

    PropertyDescriptor descriptor;
    bool enumerable = thisObject->getOwnPropertyDescriptor(exec, propertyName, descriptor) && descriptor.enumerable();
    return JSValue::encode(jsBoolean(enumerable));
}

// 15.2.4.3 Object.prototype.toLocaleString()
EncodedJSValue JSC_HOST_CALL objectProtoFuncToLocaleString(ExecState* exec)
{
    // 1. Let O be the result of calling ToObject passing the this value as the argument.
    JSObject* object = exec->thisValue().toThis(exec, StrictMode).toObject(exec);
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    // 2. Let toString be the result of calling the [[Get]] internal method of O passing "toString" as the argument.
    JSValue toString = object->get(exec, exec->propertyNames().toString);

    // 3. If IsCallable(toString) is false, throw a TypeError exception.
    CallData callData;
    CallType callType = getCallData(toString, callData);
    if (callType == CallTypeNone)
        return JSValue::encode(jsUndefined());

    // 4. Return the result of calling the [[Call]] internal method of toString passing O as the this value and no arguments.
    return JSValue::encode(call(exec, toString, callType, callData, object, exec->emptyList()));
}

EncodedJSValue JSC_HOST_CALL objectProtoFuncToString(ExecState* exec)
{
    VM& vm = exec->vm();
    JSValue thisValue = exec->thisValue().toThis(exec, StrictMode);
    if (thisValue.isUndefinedOrNull())
        return JSValue::encode(thisValue.isUndefined() ? vm.smallStrings.undefinedObjectString() : vm.smallStrings.nullObjectString());
    JSObject* thisObject = thisValue.toObject(exec);

    JSString* result = thisObject->structure(vm)->objectToStringValue();
    if (!result) {
        RefPtr<StringImpl> newString = WTF::tryMakeString("[object ", thisObject->methodTable(exec->vm())->className(thisObject), "]");
        if (!newString)
            return JSValue::encode(throwOutOfMemoryError(exec));

        result = jsNontrivialString(&vm, newString.release());
        thisObject->structure(vm)->setObjectToStringValue(vm, result);
    }
    return JSValue::encode(result);
}

} // namespace JSC
