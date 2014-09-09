/*
 * Copyright (C) 2003, 2008 Apple Inc. All rights reserved.
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
#include "runtime_method.h"

#include "JSDOMBinding.h"
#include "JSHTMLElement.h"
#include "JSPluginElementFunctions.h"
#include "runtime_object.h"
#include <runtime/Error.h>
#include <runtime/FunctionPrototype.h>

using namespace WebCore;

namespace JSC {

using namespace Bindings;

WEBCORE_EXPORT const ClassInfo RuntimeMethod::s_info = { "RuntimeMethod", &InternalFunction::s_info, 0, CREATE_METHOD_TABLE(RuntimeMethod) };

RuntimeMethod::RuntimeMethod(JSGlobalObject* globalObject, Structure* structure, Method* method)
    // Callers will need to pass in the right global object corresponding to this native object "method".
    : InternalFunction(globalObject->vm(), structure)
    , m_method(method)
{
}

void RuntimeMethod::finishCreation(VM& vm, const String& ident)
{
    Base::finishCreation(vm, ident);
    ASSERT(inherits(info()));
}

EncodedJSValue RuntimeMethod::lengthGetter(ExecState* exec, JSObject*, EncodedJSValue thisValue, PropertyName)
{
    RuntimeMethod* thisObject = jsDynamicCast<RuntimeMethod*>(JSValue::decode(thisValue));
    if (!thisObject)
        return throwVMTypeError(exec);
    return JSValue::encode(jsNumber(thisObject->m_method->numParameters()));
}

bool RuntimeMethod::getOwnPropertySlot(JSObject* object, ExecState* exec, PropertyName propertyName, PropertySlot &slot)
{
    RuntimeMethod* thisObject = jsCast<RuntimeMethod*>(object);
    if (propertyName == exec->propertyNames().length) {
        slot.setCacheableCustom(thisObject, DontDelete | ReadOnly | DontEnum, thisObject->lengthGetter);
        return true;
    }
    
    return InternalFunction::getOwnPropertySlot(thisObject, exec, propertyName, slot);
}

static EncodedJSValue JSC_HOST_CALL callRuntimeMethod(ExecState* exec)
{
    RuntimeMethod* method = static_cast<RuntimeMethod*>(exec->callee());

    if (!method->method())
        return JSValue::encode(jsUndefined());

    RefPtr<Instance> instance;

    JSValue thisValue = exec->thisValue();
    if (thisValue.inherits(RuntimeObject::info())) {
        RuntimeObject* runtimeObject = static_cast<RuntimeObject*>(asObject(thisValue));
        instance = runtimeObject->getInternalInstance();
        if (!instance) 
            return JSValue::encode(RuntimeObject::throwInvalidAccessError(exec));
    } else {
        // Calling a runtime object of a plugin element?
        if (thisValue.inherits(JSHTMLElement::info()))
            instance = pluginInstance(jsCast<JSHTMLElement*>(asObject(thisValue))->impl());
        if (!instance)
            return throwVMTypeError(exec);
    }
    ASSERT(instance);

    instance->begin();
    JSValue result = instance->invokeMethod(exec, method);
    instance->end();
    return JSValue::encode(result);
}

CallType RuntimeMethod::getCallData(JSCell*, CallData& callData)
{
    callData.native.function = callRuntimeMethod;
    return CallTypeHost;
}

}
