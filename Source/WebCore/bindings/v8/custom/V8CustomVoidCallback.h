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

#ifndef V8CustomVoidCallback_h
#define V8CustomVoidCallback_h

#include "VoidCallback.h"
#include "WorldContextHandle.h"
#include <v8.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class ScriptExecutionContext;

class V8CustomVoidCallback : public VoidCallback {
public:
    static PassRefPtr<V8CustomVoidCallback> create(v8::Local<v8::Value> value, ScriptExecutionContext* context)
    {
        ASSERT(value->IsObject());
        return adoptRef(new V8CustomVoidCallback(value->ToObject(), context));
    }
    virtual ~V8CustomVoidCallback();

    virtual void handleEvent();

private:
    V8CustomVoidCallback(v8::Local<v8::Object>, ScriptExecutionContext* context);

    v8::Persistent<v8::Object> m_callback;
    RefPtr<ScriptExecutionContext> m_scriptExecutionContext;
    WorldContextHandle m_worldContext;
};

// Returns false if callback failed (null, wrong type, or threw exception).
bool invokeCallback(v8::Persistent<v8::Object> callback, int argc, v8::Handle<v8::Value> argv[], bool& callbackReturnValue, ScriptExecutionContext*);
bool invokeCallback(v8::Persistent<v8::Object> callback, v8::Handle<v8::Object> thisObject, int argc, v8::Handle<v8::Value> argv[], bool& callbackReturnValue, ScriptExecutionContext*);

} // namespace WebCore

#endif // V8CustomVoidCallback_h
