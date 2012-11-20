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

#ifndef V8DOMWrapper_h
#define V8DOMWrapper_h

#include "DOMDataStore.h"
#include "DOMWrapperWorld.h"
#include "Event.h"
#include "Node.h"
#include "NodeFilter.h"
#include "V8CustomXPathNSResolver.h"
#include "V8DOMWindowShell.h"
#include "V8Utilities.h"
#include "WrapperTypeInfo.h"
#include <v8.h>
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

    class DOMWindow;
    class EventTarget;
    class Frame;
    class Node;
    class V8PerContextData;
    class WorkerContext;

    enum ListenerLookupType {
        ListenerFindOnly,
        ListenerFindOrCreate,
    };

    class V8DOMWrapper {
    public:
#ifndef NDEBUG
        // Checks if a v8 value can be a DOM wrapper
        static bool maybeDOMWrapper(v8::Handle<v8::Value>);
#endif

        static void setDOMWrapper(v8::Handle<v8::Object> object, WrapperTypeInfo* type, void* impl)
        {
            ASSERT(object->InternalFieldCount() >= 2);
            ASSERT(impl);
            ASSERT(type);
            object->SetAlignedPointerInInternalField(v8DOMWrapperObjectIndex, impl);
            object->SetAlignedPointerInInternalField(v8DOMWrapperTypeIndex, type);
        }

        static void clearDOMWrapper(v8::Handle<v8::Object> object, WrapperTypeInfo* type)
        {
            ASSERT(object->InternalFieldCount() >= 2);
            ASSERT(type);
            object->SetAlignedPointerInInternalField(v8DOMWrapperTypeIndex, type);
            object->SetAlignedPointerInInternalField(v8DOMWrapperObjectIndex, 0);
        }

        static v8::Handle<v8::Object> lookupDOMWrapper(v8::Handle<v8::FunctionTemplate> functionTemplate, v8::Handle<v8::Object> object)
        {
            return object.IsEmpty() ? object : object->FindInstanceInPrototypeChain(functionTemplate);
        }

        static WrapperTypeInfo* domWrapperType(v8::Handle<v8::Object>);

        static v8::Handle<v8::Value> convertEventTargetToV8Object(PassRefPtr<EventTarget> eventTarget, v8::Handle<v8::Object> creationContext = v8::Handle<v8::Object>(), v8::Isolate* isolate = 0)
        {
            return convertEventTargetToV8Object(eventTarget.get(), creationContext, isolate);
        }

        static v8::Handle<v8::Value> convertEventTargetToV8Object(EventTarget*, v8::Handle<v8::Object> creationContext = v8::Handle<v8::Object>(), v8::Isolate* = 0);

        static PassRefPtr<EventListener> getEventListener(v8::Local<v8::Value> value, bool isAttribute, ListenerLookupType lookup);

        // Wrap JS node filter in C++.
        static PassRefPtr<NodeFilter> wrapNativeNodeFilter(v8::Handle<v8::Value>);

        template<typename T>
        static v8::Persistent<v8::Object> createDOMWrapper(PassRefPtr<T>, WrapperTypeInfo*, v8::Handle<v8::Object>, v8::Isolate* = 0);

        // Check whether a V8 value is a wrapper of type |classType|.
        static bool isWrapperOfType(v8::Handle<v8::Value>, WrapperTypeInfo*);

        static void setNamedHiddenReference(v8::Handle<v8::Object> parent, const char* name, v8::Handle<v8::Value> child);

        static v8::Local<v8::Object> instantiateV8Object(v8::Handle<v8::Object> creationContext, WrapperTypeInfo*, void*);

    private:
        static void setWrapperClass(void*, v8::Persistent<v8::Object> wrapper)
        {
            wrapper.SetWrapperClassId(v8DOMObjectClassId);
        }

        static void setWrapperClass(Node*, v8::Persistent<v8::Object> wrapper)
        {
            wrapper.SetWrapperClassId(v8DOMNodeClassId);
        }
    };

    template<typename T>
    v8::Persistent<v8::Object> V8DOMWrapper::createDOMWrapper(PassRefPtr<T> object, WrapperTypeInfo* type, v8::Handle<v8::Object> wrapper, v8::Isolate* isolate)
    {
        setDOMWrapper(wrapper, type, object.get());
        v8::Persistent<v8::Object> wrapperHandle = v8::Persistent<v8::Object>::New(wrapper);
        ASSERT(maybeDOMWrapper(wrapperHandle));
        setWrapperClass(object.get(), wrapperHandle);
        DOMDataStore::current(isolate)->set(object.leakRef(), wrapperHandle);
        return wrapperHandle;
    }

}

#endif // V8DOMWrapper_h
