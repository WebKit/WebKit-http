/*
 * Copyright (C) 2007, 2008, 2009 Google Inc. All rights reserved.
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
#include "V8HTMLDocument.h"

#include "BindingState.h"
#include "Frame.h"
#include "HTMLAllCollection.h"
#include "HTMLDocument.h"
#include "HTMLCollection.h"
#include "HTMLIFrameElement.h"
#include "HTMLNames.h"
#include "V8Binding.h"
#include "V8DOMWindow.h"
#include "V8DOMWindowShell.h"
#include "V8HTMLAllCollection.h"
#include "V8HTMLCollection.h"
#include "V8Node.h"
#include "V8RecursionScope.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/StdLibExtras.h>

namespace WebCore {

v8::Local<v8::Object> V8HTMLDocument::wrapInShadowObject(v8::Local<v8::Object> wrapper, Node* impl)
{
    DEFINE_STATIC_LOCAL(v8::Persistent<v8::FunctionTemplate>, shadowTemplate, ());
    if (shadowTemplate.IsEmpty()) {
        shadowTemplate = v8::Persistent<v8::FunctionTemplate>::New(v8::FunctionTemplate::New());
        if (shadowTemplate.IsEmpty())
            return v8::Local<v8::Object>();
        shadowTemplate->SetClassName(v8::String::New("HTMLDocument"));
        shadowTemplate->Inherit(V8HTMLDocument::GetTemplate());
        shadowTemplate->InstanceTemplate()->SetInternalFieldCount(V8HTMLDocument::internalFieldCount);
    }

    v8::Local<v8::Function> shadowConstructor = shadowTemplate->GetFunction();
    if (shadowConstructor.IsEmpty())
        return v8::Local<v8::Object>();

    v8::Local<v8::Object> shadow;
    {
        V8RecursionScope::MicrotaskSuppression scope;
        shadow = shadowConstructor->NewInstance();
    }
    if (shadow.IsEmpty())
        return v8::Local<v8::Object>();
    V8DOMWrapper::setDOMWrapper(shadow, &V8HTMLDocument::info, impl);
    shadow->SetPrototype(wrapper);
    return shadow;
}

v8::Handle<v8::Value> V8HTMLDocument::getNamedProperty(HTMLDocument* htmlDocument, const AtomicString& key, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate)
{
    if (!htmlDocument->hasNamedItem(key.impl()) && !htmlDocument->hasExtraNamedItem(key.impl()))
        return v8Undefined();

    RefPtr<HTMLCollection> items = htmlDocument->documentNamedItems(key);
    if (items->isEmpty())
        return v8Undefined();

    if (items->hasExactlyOneItem()) {
        Node* node = items->item(0);
        Frame* frame = 0;
        if (node->hasTagName(HTMLNames::iframeTag) && (frame = static_cast<HTMLIFrameElement*>(node)->contentFrame()))
            return toV8(frame->document()->domWindow(), creationContext, isolate);

        return toV8(node, creationContext, isolate);
    }

    return toV8(items.release(), creationContext, isolate);
}

// HTMLDocument ----------------------------------------------------------------

// Concatenates "args" to a string. If args is empty, returns empty string.
// Firefox/Safari/IE support non-standard arguments to document.write, ex:
//   document.write("a", "b", "c") --> document.write("abc")
//   document.write() --> document.write("")
static String writeHelperGetString(const v8::Arguments& args)
{
    StringBuilder builder;
    for (int i = 0; i < args.Length(); ++i)
        builder.append(toWebCoreString(args[i]));
    return builder.toString();
}

v8::Handle<v8::Value> V8HTMLDocument::writeCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLDocument.write()");
    HTMLDocument* htmlDocument = V8HTMLDocument::toNative(args.Holder());
    htmlDocument->write(writeHelperGetString(args), activeDOMWindow(BindingState::instance())->document());
    return v8::Undefined();
}

v8::Handle<v8::Value> V8HTMLDocument::writelnCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLDocument.writeln()");
    HTMLDocument* htmlDocument = V8HTMLDocument::toNative(args.Holder());
    htmlDocument->writeln(writeHelperGetString(args), activeDOMWindow(BindingState::instance())->document());
    return v8::Undefined();
}

v8::Handle<v8::Value> V8HTMLDocument::openCallback(const v8::Arguments& args)
{
    INC_STATS("DOM.HTMLDocument.open()");
    HTMLDocument* htmlDocument = V8HTMLDocument::toNative(args.Holder());

    if (args.Length() > 2) {
        if (RefPtr<Frame> frame = htmlDocument->frame()) {
            // Fetch the global object for the frame.
            v8::Local<v8::Context> context = frame->script()->currentWorldContext();
            // Bail out if we cannot get the context.
            if (context.IsEmpty())
                return v8::Undefined();
            v8::Local<v8::Object> global = context->Global();
            // Get the open property of the global object.
            v8::Local<v8::Value> function = global->Get(v8::String::New("open"));
            // If the open property is not a function throw a type error.
            if (!function->IsFunction())
                return throwTypeError("open is not a function", args.GetIsolate());
            // Wrap up the arguments and call the function.
            OwnArrayPtr<v8::Local<v8::Value> > params = adoptArrayPtr(new v8::Local<v8::Value>[args.Length()]);
            for (int i = 0; i < args.Length(); i++)
                params[i] = args[i];

            return frame->script()->callFunction(v8::Local<v8::Function>::Cast(function), global, args.Length(), params.get());
        }
    }

    htmlDocument->open(activeDOMWindow(BindingState::instance())->document());
    return args.Holder();
}

v8::Handle<v8::Value> V8HTMLDocument::allAccessorGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    INC_STATS("DOM.HTMLDocument.all._get");
    v8::Handle<v8::Object> holder = info.Holder();
    HTMLDocument* htmlDocument = V8HTMLDocument::toNative(holder);
    return toV8(htmlDocument->all(), info.Holder(), info.GetIsolate());
}

void V8HTMLDocument::allAccessorSetter(v8::Local<v8::String> name, v8::Local<v8::Value> value, const v8::AccessorInfo& info)
{
    // Just emulate a normal JS behaviour---install a property on this.
    info.This()->ForceSet(name, value);
}

v8::Handle<v8::Value> toV8(HTMLDocument* impl, v8::Handle<v8::Object> creationContext, v8::Isolate* isolate, bool forceNewObject)
{
    if (!impl)
        return v8NullWithCheck(isolate);
    v8::Handle<v8::Object> wrapper = V8HTMLDocument::wrap(impl, creationContext, isolate, forceNewObject);
    if (wrapper.IsEmpty())
        return wrapper;
    if (!V8DOMWindowShell::getEntered()) {
        if (Frame* frame = impl->frame())
            frame->script()->windowShell()->updateDocumentWrapper(wrapper);
    }
    return wrapper;
}

} // namespace WebCore
