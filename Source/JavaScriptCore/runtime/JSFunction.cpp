/*
 *  Copyright (C) 1999-2002 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Cameron Zwarich (cwzwarich@uwaterloo.ca)
 *  Copyright (C) 2007 Maks Orlovich
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

#include "config.h"
#include "JSFunction.h"

#include "Arguments.h"
#include "CodeBlock.h"
#include "CommonIdentifiers.h"
#include "CallFrame.h"
#include "CallFrameInlines.h"
#include "ExceptionHelpers.h"
#include "FunctionPrototype.h"
#include "GetterSetter.h"
#include "JSArray.h"
#include "JSBoundFunction.h" 
#include "JSGlobalObject.h"
#include "JSNotAnObject.h"
#include "Interpreter.h"
#include "ObjectConstructor.h"
#include "ObjectPrototype.h"
#include "Operations.h"
#include "Parser.h"
#include "PropertyNameArray.h"
#include "StackVisitor.h"

namespace JSC {

EncodedJSValue JSC_HOST_CALL callHostFunctionAsConstructor(ExecState* exec)
{
    return throwVMError(exec, createNotAConstructorError(exec, exec->callee()));
}

const ClassInfo JSFunction::s_info = { "Function", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSFunction) };

bool JSFunction::isHostFunctionNonInline() const
{
    return isHostFunction();
}

JSFunction* JSFunction::create(VM& vm, JSGlobalObject* globalObject, int length, const String& name, NativeFunction nativeFunction, Intrinsic intrinsic, NativeFunction nativeConstructor)
{
    NativeExecutable* executable;
#if !ENABLE(JIT)
    UNUSED_PARAM(intrinsic);
#else
    if (intrinsic != NoIntrinsic && vm.canUseJIT()) {
        ASSERT(nativeConstructor == callHostFunctionAsConstructor);
        executable = vm.getHostFunction(nativeFunction, intrinsic);
    } else
#endif
        executable = vm.getHostFunction(nativeFunction, nativeConstructor);

    JSFunction* function = new (NotNull, allocateCell<JSFunction>(vm.heap)) JSFunction(vm, globalObject, globalObject->functionStructure());
    // Can't do this during initialization because getHostFunction might do a GC allocation.
    function->finishCreation(vm, executable, length, name);
    return function;
}

void JSFunction::destroy(JSCell* cell)
{
    static_cast<JSFunction*>(cell)->JSFunction::~JSFunction();
}

JSFunction::JSFunction(VM& vm, JSGlobalObject* globalObject, Structure* structure)
    : Base(vm, structure)
    , m_executable()
    , m_scope(vm, this, globalObject)
    // We initialize blind so that changes to the prototype after function creation but before
    // the optimizer kicks in don't disable optimizations. Once the optimizer kicks in, the
    // watchpoint will start watching and any changes will both force deoptimization and disable
    // future attempts to optimize. This is necessary because we are guaranteed that the
    // allocation profile is changed exactly once prior to optimizations kicking in. We could be
    // smarter and count the number of times the prototype is clobbered and only optimize if it
    // was clobbered exactly once, but that seems like overkill. In almost all cases it will be
    // clobbered once, and if it's clobbered more than once, that will probably only occur
    // before we started optimizing, anyway.
    , m_allocationProfileWatchpoint(InitializedBlind)
{
}

void JSFunction::finishCreation(VM& vm, NativeExecutable* executable, int length, const String& name)
{
    Base::finishCreation(vm);
    ASSERT(inherits(info()));
    m_executable.set(vm, this, executable);
    putDirect(vm, vm.propertyNames->name, jsString(&vm, name), DontDelete | ReadOnly | DontEnum);
    putDirect(vm, vm.propertyNames->length, jsNumber(length), DontDelete | ReadOnly | DontEnum);
}

ObjectAllocationProfile* JSFunction::createAllocationProfile(ExecState* exec, size_t inlineCapacity)
{
    VM& vm = exec->vm();
    JSObject* prototype = jsDynamicCast<JSObject*>(get(exec, vm.propertyNames->prototype));
    if (!prototype)
        prototype = globalObject()->objectPrototype();
    m_allocationProfile.initialize(globalObject()->vm(), this, prototype, inlineCapacity);
    return &m_allocationProfile;
}

String JSFunction::name(ExecState* exec)
{
    return get(exec, exec->vm().propertyNames->name).toWTFString(exec);
}

String JSFunction::displayName(ExecState* exec)
{
    JSValue displayName = getDirect(exec->vm(), exec->vm().propertyNames->displayName);
    
    if (displayName && isJSString(displayName))
        return asString(displayName)->tryGetValue();
    
    return String();
}

const String JSFunction::calculatedDisplayName(ExecState* exec)
{
    const String explicitName = displayName(exec);
    
    if (!explicitName.isEmpty())
        return explicitName;
    
    const String actualName = name(exec);
    if (!actualName.isEmpty() || isHostFunction())
        return actualName;
    
    return jsExecutable()->inferredName().string();
}

const SourceCode* JSFunction::sourceCode() const
{
    if (isHostFunction())
        return 0;
    return &jsExecutable()->source();
}

void JSFunction::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSFunction* thisObject = jsCast<JSFunction*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, info());
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    visitor.append(&thisObject->m_scope);
    visitor.append(&thisObject->m_executable);
    thisObject->m_allocationProfile.visitAggregate(visitor);
}

CallType JSFunction::getCallData(JSCell* cell, CallData& callData)
{
    JSFunction* thisObject = jsCast<JSFunction*>(cell);
    if (thisObject->isHostFunction()) {
        callData.native.function = thisObject->nativeFunction();
        return CallTypeHost;
    }
    callData.js.functionExecutable = thisObject->jsExecutable();
    callData.js.scope = thisObject->scope();
    return CallTypeJS;
}

class RetrieveArgumentsFunctor {
public:
    RetrieveArgumentsFunctor(JSFunction* functionObj)
        : m_targetCallee(jsDynamicCast<JSObject*>(functionObj))
        , m_result(jsNull())
    {
    }

    JSValue result() const { return m_result; }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        JSObject* callee = visitor->callee();
        if (callee != m_targetCallee)
            return StackVisitor::Continue;

        m_result = JSValue(visitor->createArguments());
        return StackVisitor::Done;
    }

private:
    JSObject* m_targetCallee;
    JSValue m_result;
};

static JSValue retrieveArguments(ExecState* exec, JSFunction* functionObj)
{
    RetrieveArgumentsFunctor functor(functionObj);
    exec->iterate(functor);
    return functor.result();
}

JSValue JSFunction::argumentsGetter(ExecState* exec, JSValue slotBase, PropertyName)
{
    JSFunction* thisObj = jsCast<JSFunction*>(slotBase);
    ASSERT(!thisObj->isHostFunction());

    return retrieveArguments(exec, thisObj);
}

class RetrieveCallerFunctionFunctor {
public:
    RetrieveCallerFunctionFunctor(JSFunction* functionObj)
        : m_targetCallee(jsDynamicCast<JSObject*>(functionObj))
        , m_hasFoundFrame(false)
        , m_hasSkippedToCallerFrame(false)
        , m_result(jsNull())
    {
    }

    JSValue result() const { return m_result; }

    StackVisitor::Status operator()(StackVisitor& visitor)
    {
        JSObject* callee = visitor->callee();

        if (callee && callee->inherits(JSBoundFunction::info()))
            return StackVisitor::Continue;

        if (!m_hasFoundFrame && (callee != m_targetCallee))
            return StackVisitor::Continue;

        m_hasFoundFrame = true;
        if (!m_hasSkippedToCallerFrame) {
            m_hasSkippedToCallerFrame = true;
            return StackVisitor::Continue;
        }

        if (callee)
            m_result = callee;
        return StackVisitor::Done;
    }

private:
    JSObject* m_targetCallee;
    bool m_hasFoundFrame;
    bool m_hasSkippedToCallerFrame;
    JSValue m_result;
};

static JSValue retrieveCallerFunction(ExecState* exec, JSFunction* functionObj)
{
    RetrieveCallerFunctionFunctor functor(functionObj);
    exec->iterate(functor);
    return functor.result();
}

JSValue JSFunction::callerGetter(ExecState* exec, JSValue slotBase, PropertyName)
{
    JSFunction* thisObj = jsCast<JSFunction*>(slotBase);
    ASSERT(!thisObj->isHostFunction());
    JSValue caller = retrieveCallerFunction(exec, thisObj);

    // See ES5.1 15.3.5.4 - Function.caller may not be used to retrieve a strict caller.
    if (!caller.isObject() || !asObject(caller)->inherits(JSFunction::info()))
        return caller;
    JSFunction* function = jsCast<JSFunction*>(caller);
    if (function->isHostFunction() || !function->jsExecutable()->isStrictMode())
        return caller;
    return throwTypeError(exec, ASCIILiteral("Function.caller used to retrieve strict caller"));
}

JSValue JSFunction::lengthGetter(ExecState*, JSValue slotBase, PropertyName)
{
    JSFunction* thisObj = jsCast<JSFunction*>(slotBase);
    ASSERT(!thisObj->isHostFunction());
    return jsNumber(thisObj->jsExecutable()->parameterCount());
}

JSValue JSFunction::nameGetter(ExecState*, JSValue slotBase, PropertyName)
{
    JSFunction* thisObj = jsCast<JSFunction*>(slotBase);
    ASSERT(!thisObj->isHostFunction());
    return thisObj->jsExecutable()->nameValue();
}

bool JSFunction::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot& slot)
{
    JSFunction* thisObject = jsCast<JSFunction*>(object);
    if (thisObject->isHostFunction())
        return Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);

    if (propertyName == exec->propertyNames().prototype) {
        VM& vm = exec->vm();
        unsigned attributes;
        PropertyOffset offset = thisObject->getDirectOffset(vm, propertyName, attributes);
        if (!isValidOffset(offset)) {
            JSObject* prototype = constructEmptyObject(exec);
            prototype->putDirect(vm, exec->propertyNames().constructor, thisObject, DontEnum);
            thisObject->putDirect(vm, exec->propertyNames().prototype, prototype, DontDelete | DontEnum);
            offset = thisObject->getDirectOffset(vm, exec->propertyNames().prototype, attributes);
            ASSERT(isValidOffset(offset));
        }

        slot.setValue(thisObject, attributes, thisObject->getDirect(offset), offset);
    }

    if (propertyName == exec->propertyNames().arguments) {
        if (thisObject->jsExecutable()->isStrictMode()) {
            bool result = Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
            if (!result) {
                thisObject->putDirectAccessor(exec, propertyName, thisObject->globalObject()->throwTypeErrorGetterSetter(exec->vm()), DontDelete | DontEnum | Accessor);
                result = Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
                ASSERT(result);
            }
            return result;
        }
        slot.setCacheableCustom(thisObject, ReadOnly | DontEnum | DontDelete, argumentsGetter);
        return true;
    }

    if (propertyName == exec->propertyNames().length) {
        slot.setCacheableCustom(thisObject, ReadOnly | DontEnum | DontDelete, lengthGetter);
        return true;
    }

    if (propertyName == exec->propertyNames().name) {
        slot.setCacheableCustom(thisObject, ReadOnly | DontEnum | DontDelete, nameGetter);
        return true;
    }

    if (propertyName == exec->propertyNames().caller) {
        if (thisObject->jsExecutable()->isStrictMode()) {
            bool result = Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
            if (!result) {
                thisObject->putDirectAccessor(exec, propertyName, thisObject->globalObject()->throwTypeErrorGetterSetter(exec->vm()), DontDelete | DontEnum | Accessor);
                result = Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
                ASSERT(result);
            }
            return result;
        }
        slot.setCacheableCustom(thisObject, ReadOnly | DontEnum | DontDelete, callerGetter);
        return true;
    }

    return Base::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

void JSFunction::getOwnNonIndexPropertyNames(JSObject* object, ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    JSFunction* thisObject = jsCast<JSFunction*>(object);
    if (!thisObject->isHostFunction() && (mode == IncludeDontEnumProperties)) {
        // Make sure prototype has been reified.
        PropertySlot slot(thisObject);
        thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, exec->propertyNames().prototype, slot);

        propertyNames.add(exec->propertyNames().arguments);
        propertyNames.add(exec->propertyNames().caller);
        propertyNames.add(exec->propertyNames().length);
        propertyNames.add(exec->propertyNames().name);
    }
    Base::getOwnNonIndexPropertyNames(thisObject, exec, propertyNames, mode);
}

void JSFunction::put(JSCell* cell, ExecState* exec, PropertyName propertyName, JSValue value, PutPropertySlot& slot)
{
    JSFunction* thisObject = jsCast<JSFunction*>(cell);
    if (thisObject->isHostFunction()) {
        Base::put(thisObject, exec, propertyName, value, slot);
        return;
    }
    if (propertyName == exec->propertyNames().prototype) {
        // Make sure prototype has been reified, such that it can only be overwritten
        // following the rules set out in ECMA-262 8.12.9.
        PropertySlot slot(thisObject);
        thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, propertyName, slot);
        thisObject->m_allocationProfile.clear();
        thisObject->m_allocationProfileWatchpoint.notifyWrite();
        // Don't allow this to be cached, since a [[Put]] must clear m_allocationProfile.
        PutPropertySlot dontCache;
        Base::put(thisObject, exec, propertyName, value, dontCache);
        return;
    }
    if (thisObject->jsExecutable()->isStrictMode() && (propertyName == exec->propertyNames().arguments || propertyName == exec->propertyNames().caller)) {
        // This will trigger the property to be reified, if this is not already the case!
        bool okay = thisObject->hasProperty(exec, propertyName);
        ASSERT_UNUSED(okay, okay);
        Base::put(thisObject, exec, propertyName, value, slot);
        return;
    }
    if (propertyName == exec->propertyNames().arguments || propertyName == exec->propertyNames().length || propertyName == exec->propertyNames().name || propertyName == exec->propertyNames().caller) {
        if (slot.isStrictMode())
            throwTypeError(exec, StrictModeReadonlyPropertyWriteError);
        return;
    }
    Base::put(thisObject, exec, propertyName, value, slot);
}

bool JSFunction::deleteProperty(JSCell* cell, ExecState* exec, PropertyName propertyName)
{
    JSFunction* thisObject = jsCast<JSFunction*>(cell);
    // For non-host functions, don't let these properties by deleted - except by DefineOwnProperty.
    if (!thisObject->isHostFunction() && !exec->vm().isInDefineOwnProperty()
        && (propertyName == exec->propertyNames().arguments
            || propertyName == exec->propertyNames().length
            || propertyName == exec->propertyNames().name
            || propertyName == exec->propertyNames().prototype
            || propertyName == exec->propertyNames().caller))
        return false;
    return Base::deleteProperty(thisObject, exec, propertyName);
}

bool JSFunction::defineOwnProperty(JSObject* object, ExecState* exec, PropertyName propertyName, const PropertyDescriptor& descriptor, bool throwException)
{
    JSFunction* thisObject = jsCast<JSFunction*>(object);
    if (thisObject->isHostFunction())
        return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);

    if (propertyName == exec->propertyNames().prototype) {
        // Make sure prototype has been reified, such that it can only be overwritten
        // following the rules set out in ECMA-262 8.12.9.
        PropertySlot slot(thisObject);
        thisObject->methodTable()->getOwnPropertySlot(thisObject, exec, propertyName, slot);
        thisObject->m_allocationProfile.clear();
        thisObject->m_allocationProfileWatchpoint.notifyWrite();
        return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
    }

    bool valueCheck;
    if (propertyName == exec->propertyNames().arguments) {
        if (thisObject->jsExecutable()->isStrictMode()) {
            PropertySlot slot(thisObject);
            if (!Base::getOwnPropertySlot(thisObject, exec, propertyName, slot))
                thisObject->putDirectAccessor(exec, propertyName, thisObject->globalObject()->throwTypeErrorGetterSetter(exec->vm()), DontDelete | DontEnum | Accessor);
            return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
        }
        valueCheck = !descriptor.value() || sameValue(exec, descriptor.value(), retrieveArguments(exec, thisObject));
    } else if (propertyName == exec->propertyNames().caller) {
        if (thisObject->jsExecutable()->isStrictMode()) {
            PropertySlot slot(thisObject);
            if (!Base::getOwnPropertySlot(thisObject, exec, propertyName, slot))
                thisObject->putDirectAccessor(exec, propertyName, thisObject->globalObject()->throwTypeErrorGetterSetter(exec->vm()), DontDelete | DontEnum | Accessor);
            return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
        }
        valueCheck = !descriptor.value() || sameValue(exec, descriptor.value(), retrieveCallerFunction(exec, thisObject));
    } else if (propertyName == exec->propertyNames().length)
        valueCheck = !descriptor.value() || sameValue(exec, descriptor.value(), jsNumber(thisObject->jsExecutable()->parameterCount()));
    else if (propertyName == exec->propertyNames().name)
        valueCheck = !descriptor.value() || sameValue(exec, descriptor.value(), thisObject->jsExecutable()->nameValue());
    else
        return Base::defineOwnProperty(object, exec, propertyName, descriptor, throwException);
     
    if (descriptor.configurablePresent() && descriptor.configurable()) {
        if (throwException)
            exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Attempting to configurable attribute of unconfigurable property.")));
        return false;
    }
    if (descriptor.enumerablePresent() && descriptor.enumerable()) {
        if (throwException)
            exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Attempting to change enumerable attribute of unconfigurable property.")));
        return false;
    }
    if (descriptor.isAccessorDescriptor()) {
        if (throwException)
            exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Attempting to change access mechanism for an unconfigurable property.")));
        return false;
    }
    if (descriptor.writablePresent() && descriptor.writable()) {
        if (throwException)
            exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Attempting to change writable attribute of unconfigurable property.")));
        return false;
    }
    if (!valueCheck) {
        if (throwException)
            exec->vm().throwException(exec, createTypeError(exec, ASCIILiteral("Attempting to change value of a readonly property.")));
        return false;
    }
    return true;
}

// ECMA 13.2.2 [[Construct]]
ConstructType JSFunction::getConstructData(JSCell* cell, ConstructData& constructData)
{
    JSFunction* thisObject = jsCast<JSFunction*>(cell);
    if (thisObject->isHostFunction()) {
        constructData.native.function = thisObject->nativeConstructor();
        return ConstructTypeHost;
    }
    constructData.js.functionExecutable = thisObject->jsExecutable();
    constructData.js.scope = thisObject->scope();
    return ConstructTypeJS;
}

String getCalculatedDisplayName(CallFrame* callFrame, JSObject* object)
{
    if (JSFunction* function = jsDynamicCast<JSFunction*>(object))
        return function->calculatedDisplayName(callFrame);
    if (InternalFunction* function = jsDynamicCast<InternalFunction*>(object))
        return function->calculatedDisplayName(callFrame);
    return "";
}

} // namespace JSC
