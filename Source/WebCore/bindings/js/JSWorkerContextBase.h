/*
 * Copyright (C) 2008, 2009 Apple Inc. All rights reserved.
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
 *
 */

#ifndef JSWorkerContextBase_h
#define JSWorkerContextBase_h

#if ENABLE(WORKERS)

#include "JSDOMGlobalObject.h"

namespace WebCore {

    class JSDedicatedWorkerContext;
    class JSSharedWorkerContext;
    class JSWorkerContext;
    class WorkerContext;

    class JSWorkerContextBase : public JSDOMGlobalObject {
        typedef JSDOMGlobalObject Base;
    public:
        virtual ~JSWorkerContextBase();

        static const JSC::ClassInfo s_info;

        WorkerContext* impl() const { return m_impl.get(); }
        virtual ScriptExecutionContext* scriptExecutionContext() const;

        static JSC::Structure* createStructure(JSC::JSGlobalData& globalData, JSC::JSGlobalObject* globalObject, JSC::JSValue prototype)
        {
            return JSC::Structure::create(globalData, globalObject, prototype, JSC::TypeInfo(JSC::ObjectType, StructureFlags), &s_info);
        }

    protected:
        JSWorkerContextBase(JSC::JSGlobalData&, JSC::Structure*, PassRefPtr<WorkerContext>);
        void finishCreation(JSC::JSGlobalData&);

    private:
        RefPtr<WorkerContext> m_impl;
    };

    // Returns a JSWorkerContext or jsNull()
    // Always ignores the execState and passed globalObject, WorkerContext is itself a globalObject and will always use its own prototype chain.
    JSC::JSValue toJS(JSC::ExecState*, JSDOMGlobalObject*, WorkerContext*);
    JSC::JSValue toJS(JSC::ExecState*, WorkerContext*);

    JSDedicatedWorkerContext* toJSDedicatedWorkerContext(JSC::JSValue);
    JSWorkerContext* toJSWorkerContext(JSC::JSValue);

#if ENABLE(SHARED_WORKERS)
    JSSharedWorkerContext* toJSSharedWorkerContext(JSC::JSValue);
#endif

} // namespace WebCore

#endif // ENABLE(WORKERS)

#endif // JSWorkerContextBase_h
