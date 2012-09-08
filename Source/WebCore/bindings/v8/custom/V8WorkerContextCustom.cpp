/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(WORKERS)
#include "V8WorkerContext.h"

#include "ContentSecurityPolicy.h"
#include "DOMTimer.h"
#include "ExceptionCode.h"
#include "ScheduledAction.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "V8Binding.h"
#include "V8Utilities.h"
#include "V8WorkerContextEventListener.h"
#include "WebSocket.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"

namespace WebCore {

v8::Handle<v8::Value> SetTimeoutOrInterval(const v8::Arguments& args, bool singleShot)
{
    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    int argumentCount = args.Length();
    if (argumentCount < 1)
        return v8::Undefined();

    v8::Handle<v8::Value> function = args[0];
    int32_t timeout = argumentCount >= 2 ? args[1]->Int32Value() : 0;
    int timerId;

    WorkerContextExecutionProxy* proxy = workerContext->script()->proxy();
    if (!proxy)
        return v8::Undefined();

    v8::Handle<v8::Context> v8Context = proxy->context();
    if (function->IsString()) {
        if (ContentSecurityPolicy* policy = workerContext->contentSecurityPolicy()) {
            RefPtr<ScriptCallStack> callStack = createScriptCallStackForInspector();
            if (!policy->allowEval(callStack.release()))
                return v8Integer(0, args.GetIsolate());
        }
        WTF::String stringFunction = toWebCoreString(function);
        timerId = DOMTimer::install(workerContext, adoptPtr(new ScheduledAction(v8Context, stringFunction, workerContext->url())), timeout, singleShot);
    } else if (function->IsFunction()) {
        size_t paramCount = argumentCount >= 2 ? argumentCount - 2 : 0;
        v8::Local<v8::Value>* params = 0;
        if (paramCount > 0) {
            params = new v8::Local<v8::Value>[paramCount];
            for (size_t i = 0; i < paramCount; ++i)
                params[i] = args[i+2];
        }
        // ScheduledAction takes ownership of actual params and releases them in its destructor.
        OwnPtr<ScheduledAction> action = adoptPtr(new ScheduledAction(v8Context, v8::Handle<v8::Function>::Cast(function), paramCount, params));
        // FIXME: We should use a OwnArrayPtr for params.
        delete [] params;
        timerId = DOMTimer::install(workerContext, action.release(), timeout, singleShot);
    } else
        return v8::Undefined();

    return v8Integer(timerId, args.GetIsolate());
}

v8::Handle<v8::Value> V8WorkerContext::importScriptsCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.WorkerContext.importScripts()");
    if (!args.Length())
        return v8::Undefined();

    Vector<String> urls;
    for (int i = 0; i < args.Length(); i++) {
        v8::TryCatch tryCatch;
        v8::Handle<v8::String> scriptUrl = args[i]->ToString();
        if (tryCatch.HasCaught() || scriptUrl.IsEmpty())
            return v8::Undefined();
        urls.append(toWebCoreString(scriptUrl));
    }

    WorkerContext* workerContext = V8WorkerContext::toNative(args.Holder());

    ExceptionCode ec = 0;
    workerContext->importScripts(urls, ec);

    if (ec)
        return setDOMException(ec, args.GetIsolate());

    return v8::Undefined();
}

v8::Handle<v8::Value> V8WorkerContext::setTimeoutCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.WorkerContext.setTimeout()");
    return SetTimeoutOrInterval(args, true);
}

v8::Handle<v8::Value> V8WorkerContext::setIntervalCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.WorkerContext.setInterval()");
    return SetTimeoutOrInterval(args, false);
}

v8::Handle<v8::Value> toV8(WorkerContext* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    // Notice that we explicitly ignore creationContext because the WorkerContext is its own creationContext.

    if (!impl)
        return v8NullWithCheck(isolate);

    WorkerContextExecutionProxy* proxy = impl->script()->proxy();
    if (!proxy)
        return v8NullWithCheck(isolate);

    v8::Handle<v8::Object> global = proxy->context()->Global();
    ASSERT(!global.IsEmpty());
    return global;
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
