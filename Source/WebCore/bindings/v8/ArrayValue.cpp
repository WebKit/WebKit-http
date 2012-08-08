/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
#include "ArrayValue.h"

#include "Dictionary.h"
#include "V8Binding.h"

namespace WebCore {

ArrayValue& ArrayValue::operator=(const ArrayValue& other)
{
    m_array = other.m_array;
    m_isolate = other.m_isolate;
    return *this;
}

bool ArrayValue::isUndefinedOrNull() const
{
    return m_array.IsEmpty() || WebCore::isUndefinedOrNull(m_array);
}

bool ArrayValue::length(size_t& length) const
{
    if (isUndefinedOrNull())
        return false;

    length = m_array->Length();
    return true;
}

bool ArrayValue::get(size_t index, Dictionary& value) const
{
    if (isUndefinedOrNull())
        return false;

    if (index >= m_array->Length())
        return false;

    v8::Local<v8::Value> indexedValue = m_array->Get(v8UnsignedInteger(index));
    if (indexedValue.IsEmpty() || !indexedValue->IsObject())
        return false;

    ASSERT(m_isolate);
    ASSERT(m_isolate == v8::Isolate::GetCurrent());
    value = Dictionary(indexedValue, m_isolate);
    return true;
}

} // namespace WebCore
