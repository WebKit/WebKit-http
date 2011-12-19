/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef CRuntimeObject_h
#define CRuntimeObject_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "JSDOMBinding.h"
#include "runtime_object.h"

namespace JSC {
namespace Bindings {

class CInstance;

class CRuntimeObject : public RuntimeObject {
public:
    typedef RuntimeObject Base;

    static CRuntimeObject* create(ExecState* exec, JSGlobalObject* globalObject, PassRefPtr<CInstance> instance)
    {
        // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
        // We need to pass in the right global object for "i".
        Structure* domStructure = WebCore::deprecatedGetDOMStructure<CRuntimeObject>(exec);
        CRuntimeObject* object = new (NotNull, allocateCell<CRuntimeObject>(*exec->heap())) CRuntimeObject(exec, globalObject, domStructure, instance);
        object->finishCreation(globalObject);
        return object;
    }

    CInstance* getInternalCInstance() const;

    static const ClassInfo s_info;

    static Structure* createStructure(JSGlobalData& globalData, JSGlobalObject* globalObject, JSValue prototype)
    {
        return Structure::create(globalData, globalObject, prototype, TypeInfo(ObjectType, StructureFlags), &s_info);
    }

private:
    CRuntimeObject(ExecState*, JSGlobalObject*, Structure*, PassRefPtr<CInstance>);
    void finishCreation(JSGlobalObject*);
};

}
}

#endif
#endif
