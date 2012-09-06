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

#if ENABLE(XSLT)

#include "V8XSLTProcessor.h"

#include "Document.h"
#include "DocumentFragment.h"
#include "Node.h"

#include "V8Binding.h"
#include "V8Document.h"
#include "V8DocumentFragment.h"
#include "V8Node.h"
#include "XSLTProcessor.h"

#include <wtf/RefPtr.h>

namespace WebCore {

v8::Handle<v8::Value> V8XSLTProcessor::setParameterCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.XSLTProcessor.setParameter");
    if (isUndefinedOrNull(args[1]) || isUndefinedOrNull(args[2]))
        return v8::Undefined();

    XSLTProcessor* imp = V8XSLTProcessor::toNative(args.Holder());

    String namespaceURI = toWebCoreString(args[0]);
    String localName = toWebCoreString(args[1]);
    String value = toWebCoreString(args[2]);
    imp->setParameter(namespaceURI, localName, value);

    return v8::Undefined();
}

v8::Handle<v8::Value> V8XSLTProcessor::getParameterCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.XSLTProcessor.getParameter");
    if (isUndefinedOrNull(args[1]))
        return v8::Undefined();

    XSLTProcessor* imp = V8XSLTProcessor::toNative(args.Holder());

    String namespaceURI = toWebCoreString(args[0]);
    String localName = toWebCoreString(args[1]);
    String result = imp->getParameter(namespaceURI, localName);
    if (result.isNull())
        return v8::Undefined();

    return v8String(result, args.GetIsolate());
}

v8::Handle<v8::Value> V8XSLTProcessor::removeParameterCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.XSLTProcessor.removeParameter");
    if (isUndefinedOrNull(args[1]))
        return v8::Undefined();

    XSLTProcessor* imp = V8XSLTProcessor::toNative(args.Holder());

    String namespaceURI = toWebCoreString(args[0]);
    String localName = toWebCoreString(args[1]);
    imp->removeParameter(namespaceURI, localName);
    return v8::Undefined();
}

} // namespace WebCore

#endif // ENABLE(XSLT)
