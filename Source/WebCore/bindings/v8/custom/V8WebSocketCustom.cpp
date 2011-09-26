/*
 * Copyright (C) 2011 Google Inc.  All rights reserved.
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

#if ENABLE(WEB_SOCKETS)

#include "V8WebSocket.h"

#include "ExceptionCode.h"
#include "Frame.h"
#include "Settings.h"
#include "V8ArrayBuffer.h"
#include "V8Binding.h"
#include "V8Blob.h"
#include "V8Proxy.h"
#include "V8Utilities.h"
#include "WebSocket.h"
#include "WebSocketChannel.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"
#include <wtf/MathExtras.h>
#include <wtf/Vector.h>

namespace WebCore {

v8::Handle<v8::Value> V8WebSocket::constructorCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.WebSocket.Constructor");

    if (!args.IsConstructCall())
        return throwError("DOM object custructor cannot be called as a function.", V8Proxy::TypeError);
    if (args.Length() == 0)
        return throwError("Not enough arguments", V8Proxy::SyntaxError);

    v8::TryCatch tryCatch;
    v8::Handle<v8::String> urlstring = args[0]->ToString();
    if (tryCatch.HasCaught())
        return throwError(tryCatch.Exception());
    if (urlstring.IsEmpty())
        return throwError("Empty URL", V8Proxy::SyntaxError);

    // Get the script execution context.
    ScriptExecutionContext* context = getScriptExecutionContext();
    if (!context)
        return throwError("WebSocket constructor's associated frame is not available", V8Proxy::ReferenceError);

    const KURL& url = context->completeURL(toWebCoreString(urlstring));

    RefPtr<WebSocket> webSocket = WebSocket::create(context);
    ExceptionCode ec = 0;

    if (args.Length() < 2)
        webSocket->connect(url, ec);
    else {
        v8::Local<v8::Value> protocolsValue = args[1];
        if (protocolsValue->IsArray()) {
            Vector<String> protocols;
            v8::Local<v8::Array> protocolsArray = v8::Local<v8::Array>::Cast(protocolsValue);
            for (uint32_t i = 0; i < protocolsArray->Length(); ++i) {
                v8::TryCatch tryCatchProtocol;
                v8::Handle<v8::String> protocol = protocolsArray->Get(v8::Int32::New(i))->ToString();
                if (tryCatchProtocol.HasCaught())
                    return throwError(tryCatchProtocol.Exception());
                protocols.append(toWebCoreString(protocol));
            }
            webSocket->connect(url, protocols, ec);
        } else {
            v8::TryCatch tryCatchProtocol;
            v8::Handle<v8::String> protocol = protocolsValue->ToString();
            if (tryCatchProtocol.HasCaught())
                return throwError(tryCatchProtocol.Exception());
            webSocket->connect(url, toWebCoreString(protocol), ec);
        }
    }
    if (ec)
        return throwError(ec);

    // Setup the standard wrapper object internal fields.
    V8DOMWrapper::setDOMWrapper(args.Holder(), &info, webSocket.get());

    // Add object to the wrapper map.
    webSocket->ref();
    V8DOMWrapper::setJSWrapperForActiveDOMObject(webSocket.get(), v8::Persistent<v8::Object>::New(args.Holder()));

    return args.Holder();
}

v8::Handle<v8::Value> V8WebSocket::sendCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.WebSocket.send()");

    if (!args.Length())
        return throwError("Not enough arguments", V8Proxy::SyntaxError);

    WebSocket* webSocket = V8WebSocket::toNative(args.Holder());
    v8::Handle<v8::Value> message = args[0];
    ExceptionCode ec = 0;
    bool result;
    if (V8ArrayBuffer::HasInstance(message)) {
        ArrayBuffer* arrayBuffer = V8ArrayBuffer::toNative(v8::Handle<v8::Object>::Cast(message));
        ASSERT(arrayBuffer);
        result = webSocket->send(arrayBuffer, ec);
    } else if (V8Blob::HasInstance(message)) {
        Blob* blob = V8Blob::toNative(v8::Handle<v8::Object>::Cast(message));
        ASSERT(blob);
        result = webSocket->send(blob, ec);
    } else {
        v8::TryCatch tryCatch;
        v8::Handle<v8::String> stringMessage = message->ToString();
        if (tryCatch.HasCaught())
            return throwError(tryCatch.Exception());
        result = webSocket->send(toWebCoreString(stringMessage), ec);
    }
    if (ec)
        return throwError(ec);

    return v8Boolean(result);
}

v8::Handle<v8::Value> V8WebSocket::closeCallback(const v8::Arguments& args)
{
    // FIXME: We should implement [Clamp] for IDL binding code generator, and
    // remove this custom method.
    WebSocket* webSocket = toNative(args.Holder());
    int argumentCount = args.Length();
    int code = WebSocketChannel::CloseEventCodeNotSpecified;
    String reason = "";
    if (argumentCount >= 1) {
        double x = args[0]->NumberValue();
        double maxValue = static_cast<double>(std::numeric_limits<uint16_t>::max());
        double minValue = static_cast<double>(std::numeric_limits<uint16_t>::min());
        if (isnan(x))
            x = 0.0;
        else
            x = clampTo(x, minValue, maxValue);
        code = clampToInteger(x);
        if (argumentCount >= 2) {
            v8::TryCatch tryCatch;
            v8::Handle<v8::String> reasonValue = args[1]->ToString();
            if (tryCatch.HasCaught())
                return throwError(tryCatch.Exception());
            reason = toWebCoreString(reasonValue);
        }
    }
    ExceptionCode ec = 0;
    webSocket->close(code, reason, ec);
    if (ec)
        return throwError(ec);
    return v8::Undefined();
}

}  // namespace WebCore

#endif  // ENABLE(WEB_SOCKETS)
