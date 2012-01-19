/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "IDBBindingUtilities.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBDatabaseException.h"
#include "IDBKey.h"
#include "IDBKeyPath.h"
#include "SerializedScriptValue.h"
#include "V8Binding.h"
#include "V8IDBKey.h"
#include <wtf/Vector.h>

namespace WebCore {

static const size_t maximumDepth = 2000;

static PassRefPtr<IDBKey> createIDBKeyFromValue(v8::Handle<v8::Value> value, Vector<v8::Handle<v8::Array> >& stack)
{
    if (value->IsNumber() && !isnan(value->NumberValue()))
        return IDBKey::createNumber(value->NumberValue());
    if (value->IsString())
        return IDBKey::createString(v8ValueToWebCoreString(value));
    if (value->IsDate())
        return IDBKey::createDate(value->NumberValue());
    if (value->IsArray()) {
        v8::Handle<v8::Array> array = v8::Handle<v8::Array>::Cast(value);

        if (stack.contains(array))
            return 0;
        if (stack.size() >= maximumDepth)
            return 0;
        stack.append(array);

        IDBKey::KeyArray subkeys;
        uint32_t length = array->Length();
        for (uint32_t i = 0; i < length; ++i) {
            v8::Local<v8::Value> item = array->Get(v8::Int32::New(i));
            RefPtr<IDBKey> subkey = createIDBKeyFromValue(item, stack);
            if (!subkey)
                return 0;
            subkeys.append(subkey);
        }

        stack.removeLast();
        return IDBKey::createArray(subkeys);
    }
    return 0;
}

PassRefPtr<IDBKey> createIDBKeyFromValue(v8::Handle<v8::Value> value)
{
    Vector<v8::Handle<v8::Array> > stack;
    RefPtr<IDBKey> key = createIDBKeyFromValue(value, stack);
    if (key)
        return key;
    return IDBKey::createInvalid();
}

namespace {

template<typename T>
bool getValueFrom(T indexOrName, v8::Handle<v8::Value>& v8Value)
{
    v8::Local<v8::Object> object = v8Value->ToObject();
    if (!object->Has(indexOrName))
        return false;
    v8Value = object->Get(indexOrName);
    return true;
}

template<typename T>
bool setValue(v8::Handle<v8::Value>& v8Object, T indexOrName, const v8::Handle<v8::Value>& v8Value)
{
    v8::Local<v8::Object> object = v8Object->ToObject();
    ASSERT(!object->Has(indexOrName));
    return object->Set(indexOrName, v8Value);
}

bool get(v8::Handle<v8::Value>& object, const String& keyPathElement)
{
    return object->IsObject() && getValueFrom(v8String(keyPathElement), object);
}

bool set(v8::Handle<v8::Value>& object, const String& keyPathElement, const v8::Handle<v8::Value>& v8Value)
{
    return object->IsObject() && setValue(object, v8String(keyPathElement), v8Value);
}

v8::Handle<v8::Value> getNthValueOnKeyPath(v8::Handle<v8::Value>& rootValue, const Vector<String>& keyPathElements, size_t index)
{
    v8::Handle<v8::Value> currentValue(rootValue);

    ASSERT(index <= keyPathElements.size());
    for (size_t i = 0; i < index; ++i) {
        if (!get(currentValue, keyPathElements[i]))
            return v8::Handle<v8::Value>();
    }

    return currentValue;
}

} // anonymous namespace

PassRefPtr<IDBKey> createIDBKeyFromSerializedValueAndKeyPath(PassRefPtr<SerializedScriptValue> value, const Vector<String>& keyPath)
{
    V8LocalContext localContext;
    v8::Handle<v8::Value> v8Value(value->deserialize());
    v8::Handle<v8::Value> v8Key(getNthValueOnKeyPath(v8Value, keyPath, keyPath.size()));
    if (v8Key.IsEmpty())
        return 0;
    return createIDBKeyFromValue(v8Key);
}

PassRefPtr<SerializedScriptValue> injectIDBKeyIntoSerializedValue(PassRefPtr<IDBKey> key, PassRefPtr<SerializedScriptValue> value, const Vector<String>& keyPath)
{
    V8LocalContext localContext;
    if (!keyPath.size())
        return 0;

    v8::Handle<v8::Value> v8Value(value->deserialize());
    v8::Handle<v8::Value> parent(getNthValueOnKeyPath(v8Value, keyPath, keyPath.size() - 1));
    if (parent.IsEmpty())
        return 0;

    if (!set(parent, keyPath.last(), toV8(key.get())))
        return 0;

    return SerializedScriptValue::create(v8Value);
}

} // namespace WebCore

#endif
