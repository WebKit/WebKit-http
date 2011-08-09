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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef JSNPMethod_h
#define JSNPMethod_h

#include <JavaScriptCore/InternalFunction.h>

typedef void* NPIdentifier;

namespace WebKit {

// A JSObject that wraps an NPMethod.
class JSNPMethod : public JSC::InternalFunction {
public:
    typedef JSC::InternalFunction Base;

    static JSNPMethod* create(JSC::ExecState* exec, JSC::JSGlobalObject* globalObject, const JSC::Identifier& ident, NPIdentifier npIdent)
    {
        return new (JSC::allocateCell<JSNPMethod>(*exec->heap())) JSNPMethod(exec, globalObject, ident, npIdent);
    }

    static const JSC::ClassInfo s_info;

    NPIdentifier npIdentifier() const { return m_npIdentifier; }

private:    
    JSNPMethod(JSC::ExecState*, JSC::JSGlobalObject*, const JSC::Identifier&, NPIdentifier);

    static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSValue prototype)
    {
        return JSC::Structure::create(globalData, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), AnonymousSlotCount, &s_info);
    }

    virtual JSC::CallType getCallData(JSC::CallData&);
    
    NPIdentifier m_npIdentifier;
};


} // namespace WebKit

#endif // JSNPMethod_h
