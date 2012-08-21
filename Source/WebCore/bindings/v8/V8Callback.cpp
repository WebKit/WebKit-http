/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "V8Callback.h"

#include "Document.h"
#include "ScriptController.h"

namespace WebCore {

bool invokeCallback(v8::Persistent<v8::Object> callback, int argc, v8::Handle<v8::Value> argv[], bool& callbackReturnValue, ScriptExecutionContext* scriptExecutionContext)
{
    return invokeCallback(callback, v8::Context::GetCurrent()->Global(), argc, argv, callbackReturnValue, scriptExecutionContext);
}

bool invokeCallback(v8::Persistent<v8::Object> callback, v8::Handle<v8::Object> thisObject, int argc, v8::Handle<v8::Value> argv[], bool& callbackReturnValue, ScriptExecutionContext* scriptExecutionContext)
{
    v8::TryCatch exceptionCatcher;
    exceptionCatcher.SetVerbose(true);

    v8::Local<v8::Function> callbackFunction;
    if (callback->IsFunction()) {
        callbackFunction = v8::Local<v8::Function>::New(v8::Persistent<v8::Function>::Cast(callback));
    } else if (callback->IsObject()) {
        v8::Local<v8::Value> handleEventFunction = callback->Get(v8::String::NewSymbol("handleEvent"));
        if (handleEventFunction->IsFunction())
            callbackFunction = v8::Local<v8::Function>::Cast(handleEventFunction);
    } else
        return false;

    if (callbackFunction.IsEmpty())
        return false;

    v8::Handle<v8::Value> result = ScriptController::callFunctionWithInstrumentation(scriptExecutionContext, callbackFunction, thisObject, argc, argv);

    callbackReturnValue = !result.IsEmpty() && result->BooleanValue();
    return exceptionCatcher.HasCaught();
}

} // namespace WebCore
