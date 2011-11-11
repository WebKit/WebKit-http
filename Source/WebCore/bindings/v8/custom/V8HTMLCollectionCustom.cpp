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
#include "V8HTMLCollection.h"

#include "HTMLCollection.h"
#include "V8Binding.h"
#include "V8HTMLAllCollection.h"
#include "V8NamedNodesCollection.h"
#include "V8Node.h"
#include "V8NodeList.h"
#include "V8Proxy.h"

namespace WebCore {

static v8::Handle<v8::Value> getNamedItems(HTMLCollection* collection, AtomicString name)
{
    Vector<RefPtr<Node> > namedItems;
    collection->namedItems(name, namedItems);

    if (!namedItems.size())
        return v8::Handle<v8::Value>();

    if (namedItems.size() == 1)
        return toV8(namedItems.at(0).release());

    return toV8(V8NamedNodesCollection::create(namedItems));
}

static v8::Handle<v8::Value> getItem(HTMLCollection* collection, v8::Handle<v8::Value> argument)
{
    v8::Local<v8::Uint32> index = argument->ToArrayIndex();
    if (index.IsEmpty()) {
        v8::Local<v8::String> asString = argument->ToString();
        if (asString.IsEmpty())
            return v8::Handle<v8::Value>();
        v8::Handle<v8::Value> result = getNamedItems(collection, toWebCoreString(asString));

        if (result.IsEmpty())
            return v8::Undefined();

        return result;
    }

    RefPtr<Node> result = collection->item(index->Uint32Value());
    return toV8(result.release());
}

v8::Handle<v8::Value> V8HTMLCollection::namedPropertyGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    INC_STATS("DOM.HTMLCollection.NamedPropertyGetter");

    if (!info.Holder()->GetRealNamedPropertyInPrototypeChain(name).IsEmpty())
        return notHandledByInterceptor();
    if (info.Holder()->HasRealNamedCallbackProperty(name))
        return notHandledByInterceptor();

    HTMLCollection* imp = V8HTMLCollection::toNative(info.Holder());
    return getNamedItems(imp, v8StringToAtomicWebCoreString(name));
}

v8::Handle<v8::Value> V8HTMLCollection::itemCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLCollection.item()");
    HTMLCollection* imp = V8HTMLCollection::toNative(args.Holder());
    return getItem(imp, args[0]);
}

v8::Handle<v8::Value> V8HTMLCollection::namedItemCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLCollection.namedItem()");
    HTMLCollection* imp = V8HTMLCollection::toNative(args.Holder());
    v8::Handle<v8::Value> result = getNamedItems(imp, toWebCoreString(args[0]));

    if (result.IsEmpty())
        return v8::Undefined();

    return result;
}

v8::Handle<v8::Value> toV8(HTMLCollection* impl)
{
    if (impl->type() == DocAll)
        return toV8(static_cast<HTMLAllCollection*>(impl));
    return V8HTMLCollection::wrap(impl);
}

} // namespace WebCore
