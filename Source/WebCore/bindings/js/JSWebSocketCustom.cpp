/*
 * Copyright (C) 2009 Google Inc.  All rights reserved.
 * Copyright (C) 2009, 2010 Apple, Inc. All rights reserved.
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

#include "JSWebSocket.h"

#include "KURL.h"
#include "JSEventListener.h"
#include "WebSocket.h"
#include <runtime/Error.h>
#include <wtf/Vector.h>

using namespace JSC;

namespace WebCore {

EncodedJSValue JSC_HOST_CALL JSWebSocketConstructor::constructJSWebSocket(ExecState* exec)
{
    JSWebSocketConstructor* jsConstructor = static_cast<JSWebSocketConstructor*>(exec->callee());
    ScriptExecutionContext* context = jsConstructor->scriptExecutionContext();
    if (!context)
        return throwVMError(exec, createReferenceError(exec, "WebSocket constructor associated document is unavailable"));

    if (!exec->argumentCount())
        return throwVMError(exec, createSyntaxError(exec, "Not enough arguments"));

    String urlString = ustringToString(exec->argument(0).toString(exec));
    if (exec->hadException())
        return throwVMError(exec, createSyntaxError(exec, "wrong URL"));
    RefPtr<WebSocket> webSocket = WebSocket::create(context);
    ExceptionCode ec = 0;
    if (exec->argumentCount() < 2)
        webSocket->connect(urlString, ec);
    else {
        JSValue protocolsValue = exec->argument(1);
        if (isJSArray(&exec->globalData(), protocolsValue)) {
            Vector<String> protocols;
            JSArray* protocolsArray = asArray(protocolsValue);
            for (unsigned i = 0; i < protocolsArray->length(); ++i) {
                String protocol = ustringToString(protocolsArray->getIndex(i).toString(exec));
                if (exec->hadException())
                    return JSValue::encode(JSValue());
                protocols.append(protocol);
            }
            webSocket->connect(urlString, protocols, ec);
        } else {
            String protocol = ustringToString(protocolsValue.toString(exec));
            if (exec->hadException())
                return JSValue::encode(JSValue());
            webSocket->connect(urlString, protocol, ec);
        }
    }
    setDOMException(exec, ec);
    return JSValue::encode(CREATE_DOM_WRAPPER(exec, jsConstructor->globalObject(), WebSocket, webSocket.get()));
}

} // namespace WebCore

#endif
