/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#include "APICast.h"
#include "Error.h"
#include "ExceptionHelpers.h"
#include "JSCallbackFunction.h"
#include "JSClassRef.h"
#include "JSFunction.h"
#include "JSGlobalObject.h"
#include "JSLock.h"
#include "JSObjectRef.h"
#include "JSString.h"
#include "JSStringRef.h"
#include "OpaqueJSString.h"
#include "PropertyNameArray.h"
#include <wtf/Vector.h>

namespace JSC {

template <class Parent>
inline JSCallbackObject<Parent>* JSCallbackObject<Parent>::asCallbackObject(JSValue value)
{
    ASSERT(asObject(value)->inherits(info()));
    return jsCast<JSCallbackObject*>(asObject(value));
}

template <class Parent>
inline JSCallbackObject<Parent>* JSCallbackObject<Parent>::asCallbackObject(EncodedJSValue value)
{
    ASSERT(asObject(JSValue::decode(value))->inherits(info()));
    return jsCast<JSCallbackObject*>(asObject(JSValue::decode(value)));
}

template <class Parent>
JSCallbackObject<Parent>::JSCallbackObject(ExecState* exec, Structure* structure, JSClassRef jsClass, void* data)
    : Parent(exec->vm(), structure)
    , m_callbackObjectData(adoptPtr(new JSCallbackObjectData(data, jsClass)))
{
}

// Global object constructor.
// FIXME: Move this into a separate JSGlobalCallbackObject class derived from this one.
template <class Parent>
JSCallbackObject<Parent>::JSCallbackObject(VM& vm, JSClassRef jsClass, Structure* structure)
    : Parent(vm, structure)
    , m_callbackObjectData(adoptPtr(new JSCallbackObjectData(0, jsClass)))
{
}

template <class Parent>
void JSCallbackObject<Parent>::finishCreation(ExecState* exec)
{
    Base::finishCreation(exec->vm());
    ASSERT(Parent::inherits(info()));
    init(exec);
}

// This is just for Global object, so we can assume that Base::finishCreation is JSGlobalObject::finishCreation.
template <class Parent>
void JSCallbackObject<Parent>::finishCreation(VM& vm)
{
    ASSERT(Parent::inherits(info()));
    ASSERT(Parent::isGlobalObject());
    Base::finishCreation(vm);
    init(jsCast<JSGlobalObject*>(this)->globalExec());
}

template <class Parent>
void JSCallbackObject<Parent>::init(ExecState* exec)
{
    ASSERT(exec);
    
    Vector<JSObjectInitializeCallback, 16> initRoutines;
    JSClassRef jsClass = classRef();
    do {
        if (JSObjectInitializeCallback initialize = jsClass->initialize)
            initRoutines.append(initialize);
    } while ((jsClass = jsClass->parentClass));
    
    // initialize from base to derived
    for (int i = static_cast<int>(initRoutines.size()) - 1; i >= 0; i--) {
        JSLock::DropAllLocks dropAllLocks(exec);
        JSObjectInitializeCallback initialize = initRoutines[i];
        initialize(toRef(exec), toRef(this));
    }

    for (JSClassRef jsClassPtr = classRef(); jsClassPtr; jsClassPtr = jsClassPtr->parentClass) {
        if (jsClassPtr->finalize) {
            WeakSet::allocate(this, m_callbackObjectData.get(), classRef());
            break;
        }
    }
}

template <class Parent>
String JSCallbackObject<Parent>::className(const JSObject* object)
{
    const JSCallbackObject* thisObject = jsCast<const JSCallbackObject*>(object);
    String thisClassName = thisObject->classRef()->className();
    if (!thisClassName.isEmpty())
        return thisClassName;
    
    return Parent::className(object);
}

template <class Parent>
bool JSCallbackObject<Parent>::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(object);
    JSContextRef ctx = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    RefPtr<OpaqueJSString> propertyNameRef;
    
    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
            // optional optimization to bypass getProperty in cases when we only need to know if the property exists
            if (JSObjectHasPropertyCallback hasProperty = jsClass->hasProperty) {
                if (!propertyNameRef)
                    propertyNameRef = OpaqueJSString::create(name);
                JSLock::DropAllLocks dropAllLocks(exec);
                if (hasProperty(ctx, thisRef, propertyNameRef.get())) {
                    slot.setCustom(thisObject, ReadOnly | DontEnum, callbackGetter);
                    return true;
                }
            } else if (JSObjectGetPropertyCallback getProperty = jsClass->getProperty) {
                if (!propertyNameRef)
                    propertyNameRef = OpaqueJSString::create(name);
                JSValueRef exception = 0;
                JSValueRef value;
                {
                    JSLock::DropAllLocks dropAllLocks(exec);
                    value = getProperty(ctx, thisRef, propertyNameRef.get(), &exception);
                }
                if (exception) {
                    exec->vm().throwException(exec, toJS(exec, exception));
                    slot.setValue(thisObject, ReadOnly | DontEnum, jsUndefined());
                    return true;
                }
                if (value) {
                    slot.setValue(thisObject, ReadOnly | DontEnum, toJS(exec, value));
                    return true;
                }
            }
            
            if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
                if (staticValues->contains(name)) {
                    JSValue value = thisObject->getStaticValue(exec, propertyName);
                    if (value) {
                        slot.setValue(thisObject, ReadOnly | DontEnum, value);
                        return true;
                    }
                }
            }
            
            if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
                if (staticFunctions->contains(name)) {
                    slot.setCustom(thisObject, ReadOnly | DontEnum, staticFunctionGetter);
                    return true;
                }
            }
        }
    }

    return Parent::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

template <class Parent>
bool JSCallbackObject<Parent>::getOwnPropertySlotByIndex(JSObject* object, ExecState* exec, unsigned propertyName, PropertySlot& slot)
{
    return object->methodTable()->getOwnPropertySlot(object, exec, Identifier::from(exec, propertyName), slot);
}

template <class Parent>
JSValue JSCallbackObject<Parent>::defaultValue(const JSObject* object, ExecState* exec, PreferredPrimitiveType hint)
{
    const JSCallbackObject* thisObject = jsCast<const JSCallbackObject*>(object);
    JSContextRef ctx = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    ::JSType jsHint = hint == PreferString ? kJSTypeString : kJSTypeNumber;

    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectConvertToTypeCallback convertToType = jsClass->convertToType) {
            JSValueRef exception = 0;
            JSValueRef result = convertToType(ctx, thisRef, jsHint, &exception);
            if (exception) {
                exec->vm().throwException(exec, toJS(exec, exception));
                return jsUndefined();
            }
            if (result)
                return toJS(exec, result);
        }
    }
    
    return Parent::defaultValue(object, exec, hint);
}

template <class Parent>
void JSCallbackObject<Parent>::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    JSContextRef ctx = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    RefPtr<OpaqueJSString> propertyNameRef;
    JSValueRef valueRef = toRef(exec, value);
    
    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
            if (JSObjectSetPropertyCallback setProperty = jsClass->setProperty) {
                if (!propertyNameRef)
                    propertyNameRef = OpaqueJSString::create(name);
                JSValueRef exception = 0;
                bool result;
                {
                    JSLock::DropAllLocks dropAllLocks(exec);
                    result = setProperty(ctx, thisRef, propertyNameRef.get(), valueRef, &exception);
                }
                if (exception)
                    exec->vm().throwException(exec, toJS(exec, exception));
                if (result || exception)
                    return;
            }
            
            if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
                if (StaticValueEntry* entry = staticValues->get(name)) {
                    if (entry->attributes & kJSPropertyAttributeReadOnly)
                        return;
                    if (JSObjectSetPropertyCallback setProperty = entry->setProperty) {
                        JSValueRef exception = 0;
                        bool result;
                        {
                            JSLock::DropAllLocks dropAllLocks(exec);
                            result = setProperty(ctx, thisRef, entry->propertyNameRef.get(), valueRef, &exception);
                        }
                        if (exception)
                            exec->vm().throwException(exec, toJS(exec, exception));
                        if (result || exception)
                            return;
                    }
                }
            }
            
            if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
                if (StaticFunctionEntry* entry = staticFunctions->get(name)) {
                    if (entry->attributes & kJSPropertyAttributeReadOnly)
                        return;
                    thisObject->JSCallbackObject<Parent>::putDirect(exec->vm(), propertyName, value); // put as override property
                    return;
                }
            }
        }
    }

    return Parent::put(thisObject, exec, propertyName, value, slot);
}

template <class Parent>
void JSCallbackObject<Parent>::putByIndex(JSCell* cell, ExecState* exec, unsigned propertyIndex, JSValue value, bool shouldThrow)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    JSContextRef ctx = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    RefPtr<OpaqueJSString> propertyNameRef;
    JSValueRef valueRef = toRef(exec, value);
    Identifier propertyName = Identifier::from(exec, propertyIndex);

    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectSetPropertyCallback setProperty = jsClass->setProperty) {
            if (!propertyNameRef)
                propertyNameRef = OpaqueJSString::create(propertyName.impl());
            JSValueRef exception = 0;
            bool result;
            {
                JSLock::DropAllLocks dropAllLocks(exec);
                result = setProperty(ctx, thisRef, propertyNameRef.get(), valueRef, &exception);
            }
            if (exception)
                exec->vm().throwException(exec, toJS(exec, exception));
            if (result || exception)
                return;
        }

        if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
            if (StaticValueEntry* entry = staticValues->get(propertyName.impl())) {
                if (entry->attributes & kJSPropertyAttributeReadOnly)
                    return;
                if (JSObjectSetPropertyCallback setProperty = entry->setProperty) {
                    JSValueRef exception = 0;
                    bool result;
                    {
                        JSLock::DropAllLocks dropAllLocks(exec);
                        result = setProperty(ctx, thisRef, entry->propertyNameRef.get(), valueRef, &exception);
                    }
                    if (exception)
                        exec->vm().throwException(exec, toJS(exec, exception));
                    if (result || exception)
                        return;
                }
            }
        }

        if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
            if (StaticFunctionEntry* entry = staticFunctions->get(propertyName.impl())) {
                if (entry->attributes & kJSPropertyAttributeReadOnly)
                    return;
                break;
            }
        }
    }

    return Parent::putByIndex(thisObject, exec, propertyIndex, value, shouldThrow);
}

template <class Parent>
bool JSCallbackObject<Parent>::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    JSContextRef ctx = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    RefPtr<OpaqueJSString> propertyNameRef;
    
    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
            if (JSObjectDeletePropertyCallback deleteProperty = jsClass->deleteProperty) {
                if (!propertyNameRef)
                    propertyNameRef = OpaqueJSString::create(name);
                JSValueRef exception = 0;
                bool result;
                {
                    JSLock::DropAllLocks dropAllLocks(exec);
                    result = deleteProperty(ctx, thisRef, propertyNameRef.get(), &exception);
                }
                if (exception)
                    exec->vm().throwException(exec, toJS(exec, exception));
                if (result || exception)
                    return true;
            }
            
            if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
                if (StaticValueEntry* entry = staticValues->get(name)) {
                    if (entry->attributes & kJSPropertyAttributeDontDelete)
                        return false;
                    return true;
                }
            }
            
            if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
                if (StaticFunctionEntry* entry = staticFunctions->get(name)) {
                    if (entry->attributes & kJSPropertyAttributeDontDelete)
                        return false;
                    return true;
                }
            }
        }
    }

    return Parent::deleteProperty(thisObject, exec, propertyName);
}

template <class Parent>
bool JSCallbackObject<Parent>::deletePropertyByIndex(JSCell* cell, ExecState* exec, unsigned propertyName)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    return thisObject->methodTable()->deleteProperty(thisObject, exec, Identifier::from(exec, propertyName));
}

template <class Parent>
ConstructType JSCallbackObject<Parent>::getConstructData(JSCell* cell, ConstructData& constructData)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (jsClass->callAsConstructor) {
            constructData.native.function = construct;
            return ConstructTypeHost;
        }
    }
    return ConstructTypeNone;
}

template <class Parent>
EncodedJSValue JSCallbackObject<Parent>::construct(ExecState* exec)
{
    JSObject* constructor = exec->callee();
    JSContextRef execRef = toRef(exec);
    JSObjectRef constructorRef = toRef(constructor);
    
    for (JSClassRef jsClass = jsCast<JSCallbackObject<Parent>*>(constructor)->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectCallAsConstructorCallback callAsConstructor = jsClass->callAsConstructor) {
            size_t argumentCount = exec->argumentCount();
            Vector<JSValueRef, 16> arguments;
            arguments.reserveInitialCapacity(argumentCount);
            for (size_t i = 0; i < argumentCount; ++i)
                arguments.uncheckedAppend(toRef(exec, exec->uncheckedArgument(i)));
            JSValueRef exception = 0;
            JSObject* result;
            {
                JSLock::DropAllLocks dropAllLocks(exec);
                result = toJS(callAsConstructor(execRef, constructorRef, argumentCount, arguments.data(), &exception));
            }
            if (exception)
                exec->vm().throwException(exec, toJS(exec, exception));
            return JSValue::encode(result);
        }
    }
    
    RELEASE_ASSERT_NOT_REACHED(); // getConstructData should prevent us from reaching here
    return JSValue::encode(JSValue());
}

template <class Parent>
bool JSCallbackObject<Parent>::customHasInstance(JSObject* object, ExecState* exec, JSValue value)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(object);
    JSContextRef execRef = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    
    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectHasInstanceCallback hasInstance = jsClass->hasInstance) {
            JSValueRef valueRef = toRef(exec, value);
            JSValueRef exception = 0;
            bool result;
            {
                JSLock::DropAllLocks dropAllLocks(exec);
                result = hasInstance(execRef, thisRef, valueRef, &exception);
            }
            if (exception)
                exec->vm().throwException(exec, toJS(exec, exception));
            return result;
        }
    }
    return false;
}

template <class Parent>
CallType JSCallbackObject<Parent>::getCallData(JSCell* cell, CallData& callData)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(cell);
    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (jsClass->callAsFunction) {
            callData.native.function = call;
            return CallTypeHost;
        }
    }
    return CallTypeNone;
}

template <class Parent>
EncodedJSValue JSCallbackObject<Parent>::call(ExecState* exec)
{
    JSContextRef execRef = toRef(exec);
    JSObjectRef functionRef = toRef(exec->callee());
    JSObjectRef thisObjRef = toRef(jsCast<JSObject*>(exec->thisValue().toThis(exec, NotStrictMode)));
    
    for (JSClassRef jsClass = jsCast<JSCallbackObject<Parent>*>(toJS(functionRef))->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectCallAsFunctionCallback callAsFunction = jsClass->callAsFunction) {
            size_t argumentCount = exec->argumentCount();
            Vector<JSValueRef, 16> arguments;
            arguments.reserveInitialCapacity(argumentCount);
            for (size_t i = 0; i < argumentCount; ++i)
                arguments.uncheckedAppend(toRef(exec, exec->uncheckedArgument(i)));
            JSValueRef exception = 0;
            JSValue result;
            {
                JSLock::DropAllLocks dropAllLocks(exec);
                result = toJS(exec, callAsFunction(execRef, functionRef, thisObjRef, argumentCount, arguments.data(), &exception));
            }
            if (exception)
                exec->vm().throwException(exec, toJS(exec, exception));
            return JSValue::encode(result);
        }
    }
    
    RELEASE_ASSERT_NOT_REACHED(); // getCallData should prevent us from reaching here
    return JSValue::encode(JSValue());
}

template <class Parent>
void JSCallbackObject<Parent>::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSCallbackObject* thisObject = jsCast<JSCallbackObject*>(object);
    JSContextRef execRef = toRef(exec);
    JSObjectRef thisRef = toRef(thisObject);
    
    for (JSClassRef jsClass = thisObject->classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (JSObjectGetPropertyNamesCallback getPropertyNames = jsClass->getPropertyNames) {
            JSLock::DropAllLocks dropAllLocks(exec);
            getPropertyNames(execRef, thisRef, toRef(&propertyNames));
        }
        
        if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
            typedef OpaqueJSClassStaticValuesTable::const_iterator iterator;
            iterator end = staticValues->end();
            for (iterator it = staticValues->begin(); it != end; ++it) {
                StringImpl* name = it->key.get();
                StaticValueEntry* entry = it->value.get();
                if (entry->getProperty && (!(entry->attributes & kJSPropertyAttributeDontEnum) || (mode == IncludeDontEnumProperties)))
                    propertyNames.add(Identifier(exec, name));
            }
        }
        
        if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
            typedef OpaqueJSClassStaticFunctionsTable::const_iterator iterator;
            iterator end = staticFunctions->end();
            for (iterator it = staticFunctions->begin(); it != end; ++it) {
                StringImpl* name = it->key.get();
                StaticFunctionEntry* entry = it->value.get();
                if (!(entry->attributes & kJSPropertyAttributeDontEnum) || (mode == IncludeDontEnumProperties))
                    propertyNames.add(Identifier(exec, name));
            }
        }
    }
    
    Parent::getOwnNonIndexPropertyNames(thisObject, exec, propertyNames, mode);
}

template <class Parent>
void JSCallbackObject<Parent>::setPrivate(void* data)
{
    m_callbackObjectData->privateData = data;
}

template <class Parent>
void* JSCallbackObject<Parent>::getPrivate()
{
    return m_callbackObjectData->privateData;
}

template <class Parent>
bool JSCallbackObject<Parent>::inherits(JSClassRef c) const
{
    for (JSClassRef jsClass = classRef(); jsClass; jsClass = jsClass->parentClass) {
        if (jsClass == c)
            return true;
    }
    return false;
}

template <class Parent>
JSValue JSCallbackObject<Parent>::getStaticValue(ExecState* exec, PropertyName propertyName)
{
    JSObjectRef thisRef = toRef(this);
    
    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = classRef(); jsClass; jsClass = jsClass->parentClass) {
            if (OpaqueJSClassStaticValuesTable* staticValues = jsClass->staticValues(exec)) {
                if (StaticValueEntry* entry = staticValues->get(name)) {
                    if (JSObjectGetPropertyCallback getProperty = entry->getProperty) {
                        JSValueRef exception = 0;
                        JSValueRef value;
                        {
                            JSLock::DropAllLocks dropAllLocks(exec);
                            value = getProperty(toRef(exec), thisRef, entry->propertyNameRef.get(), &exception);
                        }
                        if (exception) {
                            exec->vm().throwException(exec, toJS(exec, exception));
                            return jsUndefined();
                        }
                        if (value)
                            return toJS(exec, value);
                    }
                }
            }
        }
    }

    return JSValue();
}

template <class Parent>
EncodedJSValue JSCallbackObject<Parent>::staticFunctionGetter(ExecState* exec, JSObject* slotParent, EncodedJSValue, PropertyName propertyName)
{
    JSCallbackObject* thisObj = asCallbackObject(slotParent);
    
    // Check for cached or override property.
    PropertySlot slot2(thisObj);
    if (Parent::getOwnPropertySlot(thisObj, exec, propertyName, slot2))
        return JSValue::encode(slot2.getValue(exec, propertyName));

    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = thisObj->classRef(); jsClass; jsClass = jsClass->parentClass) {
            if (OpaqueJSClassStaticFunctionsTable* staticFunctions = jsClass->staticFunctions(exec)) {
                if (StaticFunctionEntry* entry = staticFunctions->get(name)) {
                    if (JSObjectCallAsFunctionCallback callAsFunction = entry->callAsFunction) {
                        VM& vm = exec->vm();
                        JSObject* o = JSCallbackFunction::create(vm, thisObj->globalObject(), callAsFunction, name);
                        thisObj->putDirect(vm, propertyName, o, entry->attributes);
                        return JSValue::encode(o);
                    }
                }
            }
        }
    }

    return JSValue::encode(exec->vm().throwException(exec, createReferenceError(exec, ASCIILiteral("Static function property defined with NULL callAsFunction callback."))));
}

template <class Parent>
EncodedJSValue JSCallbackObject<Parent>::callbackGetter(ExecState* exec, JSObject* slotParent, EncodedJSValue, PropertyName propertyName)
{
    JSCallbackObject* thisObj = asCallbackObject(slotParent);
    
    JSObjectRef thisRef = toRef(thisObj);
    RefPtr<OpaqueJSString> propertyNameRef;
    
    if (StringImpl* name = propertyName.uid()) {
        for (JSClassRef jsClass = thisObj->classRef(); jsClass; jsClass = jsClass->parentClass) {
            if (JSObjectGetPropertyCallback getProperty = jsClass->getProperty) {
                if (!propertyNameRef)
                    propertyNameRef = OpaqueJSString::create(name);
                JSValueRef exception = 0;
                JSValueRef value;
                {
                    JSLock::DropAllLocks dropAllLocks(exec);
                    value = getProperty(toRef(exec), thisRef, propertyNameRef.get(), &exception);
                }
                if (exception) {
                    exec->vm().throwException(exec, toJS(exec, exception));
                    return JSValue::encode(jsUndefined());
                }
                if (value)
                    return JSValue::encode(toJS(exec, value));
            }
        }
    }

    return JSValue::encode(exec->vm().throwException(exec, createReferenceError(exec, ASCIILiteral("hasProperty callback returned true for a property that doesn't exist."))));
}

} // namespace JSC
