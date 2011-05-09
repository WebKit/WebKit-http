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
#include "V8MessageEvent.h"

#include "MessageEvent.h"
#include "SerializedScriptValue.h"

#include "V8Binding.h"
#include "V8DOMWindow.h"
#include "V8MessagePort.h"
#include "V8MessagePortCustom.h"
#include "V8Proxy.h"

namespace WebCore {

v8::Handle<v8::Value> V8MessageEvent::portsAccessorGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    INC_STATS("DOM.MessageEvent.ports");
    MessageEvent* event = V8MessageEvent::toNative(info.Holder());

    MessagePortArray* ports = event->ports();
    if (!ports || ports->isEmpty())
        return v8::Null();

    v8::Local<v8::Array> portArray = v8::Array::New(ports->size());
    for (size_t i = 0; i < ports->size(); ++i)
        portArray->Set(v8::Integer::New(i), toV8((*ports)[i].get()));

    return portArray;
}

v8::Handle<v8::Value> V8MessageEvent::initMessageEventCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.MessageEvent.initMessageEvent");
    MessageEvent* event = V8MessageEvent::toNative(args.Holder());
    String typeArg = v8ValueToWebCoreString(args[0]);
    bool canBubbleArg = args[1]->BooleanValue();
    bool cancelableArg = args[2]->BooleanValue();
    RefPtr<SerializedScriptValue> dataArg = SerializedScriptValue::create(args[3]);
    String originArg = v8ValueToWebCoreString(args[4]);
    String lastEventIdArg = v8ValueToWebCoreString(args[5]);

    DOMWindow* sourceArg = 0;
    if (args[6]->IsObject()) {
        v8::Handle<v8::Object> wrapper = v8::Handle<v8::Object>::Cast(args[6]);
        v8::Handle<v8::Object> window = V8DOMWrapper::lookupDOMWrapper(V8DOMWindow::GetTemplate(), wrapper);
        if (!window.IsEmpty())
            sourceArg = V8DOMWindow::toNative(window);
    }
    OwnPtr<MessagePortArray> portArray;

    if (!isUndefinedOrNull(args[7])) {
        portArray = adoptPtr(new MessagePortArray);
        if (!getMessagePortArray(args[7], *portArray))
            return v8::Undefined();
    }
    event->initMessageEvent(typeArg, canBubbleArg, cancelableArg, dataArg.release(), originArg, lastEventIdArg, sourceArg, portArray.release());
    v8::PropertyAttribute dataAttr = static_cast<v8::PropertyAttribute>(v8::DontDelete | v8::ReadOnly);
    SerializedScriptValue::deserializeAndSetProperty(args.Holder(), "data", dataAttr, event->data());
    return v8::Undefined();
  }

} // namespace WebCore
