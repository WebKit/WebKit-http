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

#ifndef RUNTIME_FUNCTION_H_
#define RUNTIME_FUNCTION_H_

#include "BridgeJSC.h"
#include <runtime/InternalFunction.h>
#include <runtime/JSGlobalObject.h>
#include <wtf/OwnPtr.h>

namespace JSC {

class RuntimeMethod : public InternalFunction {
public:
    typedef InternalFunction Base;

    static RuntimeMethod* create(ExecState* exec, JSGlobalObject* globalObject, Structure* structure, const String& name, Bindings::Method* method)
    {
        RuntimeMethod* runtimeMethod = new (NotNull, allocateCell<RuntimeMethod>(*exec->heap())) RuntimeMethod(globalObject, structure, method);
        runtimeMethod->finishCreation(exec->vm(), name);
        return runtimeMethod;
    }

    Bindings::Method* method() const { return m_method; }

    DECLARE_INFO;

    static FunctionPrototype* createPrototype(VM&, JSGlobalObject* globalObject)
    {
        return globalObject->functionPrototype();
    }

    static Structure* createStructure(VM& vm, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(vm, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), info());
    }

protected:
    RuntimeMethod(JSGlobalObject*, Structure*, Bindings::Method*);
    void finishCreation(VM&, const String&);
    static const unsigned StructureFlags = OverridesGetOwnPropertySlot | InternalFunction::StructureFlags;
    static CallType getCallData(JSCell*, CallData&);

    static bool getOwnPropertySlot(JSObject*, ExecState*, PropertyName, PropertySlot&);

private:
    static EncodedJSValue lengthGetter(ExecState*, JSObject*, EncodedJSValue, PropertyName);

    Bindings::Method* m_method;
};

} // namespace JSC

#endif
