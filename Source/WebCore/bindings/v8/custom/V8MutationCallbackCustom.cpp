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

#if ENABLE(MUTATION_OBSERVERS)

#include "V8MutationCallback.h"

#include "ScriptExecutionContext.h"
#include "V8Binding.h"
#include "V8CustomVoidCallback.h"
#include "V8MutationRecord.h"
#include "V8Proxy.h"
#include "V8WebKitMutationObserver.h"
#include <wtf/Assertions.h>
#include <wtf/GetPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

bool V8MutationCallback::handleEvent(MutationRecordArray* mutations, WebKitMutationObserver* observer)
{
    ASSERT(mutations);
    if (!mutations)
        return true;

    if (!canInvokeCallback())
        return true;

    v8::HandleScope handleScope;

    v8::Handle<v8::Context> v8Context = toV8Context(scriptExecutionContext(), m_worldContext);
    if (v8Context.IsEmpty())
        return true;

    v8::Context::Scope scope(v8Context);

    v8::Local<v8::Array> mutationsArray = v8::Array::New(mutations->size());
    for (size_t i = 0; i < mutations->size(); ++i)
        mutationsArray->Set(v8::Integer::New(i), toV8(mutations->at(i).get()));

    v8::Handle<v8::Value> observerHandle = toV8(observer);
    if (observerHandle.IsEmpty()) {
        CRASH();
        return true;
    }

    v8::Handle<v8::Value> argv[] = {
        mutationsArray,
        observerHandle
    };

    bool callbackReturnValue = false;
    return !invokeCallback(m_callback, 2, argv, callbackReturnValue, scriptExecutionContext());
}

} // namespace WebCore

#endif // ENABLE(MUTATION_OBSERVERS)
