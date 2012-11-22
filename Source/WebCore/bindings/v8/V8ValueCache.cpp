/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "V8ValueCache.h"

#include "V8Binding.h"

namespace WebCore {

static v8::Local<v8::String> makeExternalString(const String& string)
{
    if (string.is8Bit() && string.containsOnlyASCII()) {
        WebCoreStringResource8* stringResource = new WebCoreStringResource8(string);
        v8::Local<v8::String> newString = v8::String::NewExternal(stringResource);
        if (newString.IsEmpty())
            delete stringResource;
        return newString;
    }

    WebCoreStringResource16* stringResource = new WebCoreStringResource16(string);
    v8::Local<v8::String> newString = v8::String::NewExternal(stringResource);
    if (newString.IsEmpty())
        delete stringResource;

    return newString;
}

static void cachedStringCallback(v8::Persistent<v8::Value> wrapper, void* parameter)
{
    StringImpl* stringImpl = static_cast<StringImpl*>(parameter);
    V8PerIsolateData::current()->stringCache()->remove(stringImpl);
    wrapper.Dispose();
    wrapper.Clear();
    stringImpl->deref();
}

void StringCache::remove(StringImpl* stringImpl) 
{
    ASSERT(m_stringCache.contains(stringImpl));
    m_stringCache.remove(stringImpl);
    // Make sure that already disposed m_lastV8String is not used in
    // StringCache::v8ExternalString().
    clearOnGC();
}

v8::Local<v8::String> StringCache::v8ExternalStringSlow(StringImpl* stringImpl, v8::Isolate* isolate)
{
    if (!stringImpl->length())
        return v8::String::Empty(isolate);

    v8::String* cachedV8String = m_stringCache.get(stringImpl);
    if (cachedV8String) {
        v8::Persistent<v8::String> handle(cachedV8String);
        if (handle.IsWeak()) {
            m_lastStringImpl = stringImpl;
            m_lastV8String = handle;
            return v8::Local<v8::String>::New(handle);
        }
    }

    v8::Local<v8::String> newString = makeExternalString(String(stringImpl));
    if (newString.IsEmpty())
        return newString;

    v8::Persistent<v8::String> wrapper = v8::Persistent<v8::String>::New(newString);
    if (wrapper.IsEmpty())
        return newString;

    stringImpl->ref();
    wrapper.MarkIndependent();
    wrapper.MakeWeak(stringImpl, cachedStringCallback);
    m_stringCache.set(stringImpl, *wrapper);

    m_lastStringImpl = stringImpl;
    m_lastV8String = wrapper;

    return newString;
}

void IntegerCache::createSmallIntegers(v8::Isolate* isolate)
{
    ASSERT(!m_initialized);
    // We initialize m_smallIntegers not in a constructor but in v8Integer(),
    // because Integer::New() requires a HandleScope. At the point where
    // IntegerCache is constructed, a HandleScope might not exist.
    for (int value = 0; value < numberOfCachedSmallIntegers; value++)
        m_smallIntegers[value] = v8::Persistent<v8::Integer>::New(v8::Integer::New(value, isolate));
    m_initialized = true;
}

IntegerCache::~IntegerCache()
{
    if (m_initialized) {
        for (int value = 0; value < numberOfCachedSmallIntegers; value++) {
            m_smallIntegers[value].Dispose();
            m_smallIntegers[value].Clear();
        }
        m_initialized = false;
    }
}

} // namespace WebCore
