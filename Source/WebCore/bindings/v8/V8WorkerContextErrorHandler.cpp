/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "V8WorkerContextErrorHandler.h"

#include "EventNames.h"
#include "ErrorEvent.h"
#include "V8Binding.h"
#include "V8RecursionScope.h"

namespace WebCore {

V8WorkerContextErrorHandler::V8WorkerContextErrorHandler(v8::Local<v8::Object> listener, bool isInline, const WorldContextHandle& worldContext)
    : V8WorkerContextEventListener(listener, isInline, worldContext)
{
}

v8::Local<v8::Value> V8WorkerContextErrorHandler::callListenerFunction(ScriptExecutionContext* context, v8::Handle<v8::Value> jsEvent, Event* event)
{
    ASSERT(event->hasInterface(eventNames().interfaceForErrorEvent));
    v8::Local<v8::Object> listener = getListenerObject(context);
    v8::Local<v8::Value> returnValue;
    if (!listener.IsEmpty() && listener->IsFunction()) {
        ErrorEvent* errorEvent = static_cast<ErrorEvent*>(event);
        v8::Local<v8::Function> callFunction = v8::Local<v8::Function>::Cast(listener);
        v8::Local<v8::Object> thisValue = v8::Context::GetCurrent()->Global();
        v8::Handle<v8::Value> parameters[3] = { v8String(errorEvent->message()), v8String(errorEvent->filename()), v8Integer(errorEvent->lineno()) };
        V8RecursionScope recursionScope(context);
        returnValue = callFunction->Call(thisValue, 3, parameters);
    }
    return returnValue;
}

bool V8WorkerContextErrorHandler::shouldPreventDefault(v8::Local<v8::Value> returnValue)
{
    return returnValue->IsBoolean() && returnValue->BooleanValue();
}

} // namespace WebCore

#endif // WORKERS
