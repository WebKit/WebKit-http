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
#include "WebIDBKeyPath.h"

#if ENABLE(INDEXED_DATABASE)

#include "IDBKeyPath.h"
#include "platform/WebString.h"
#include "platform/WebVector.h"
#include <wtf/Vector.h>

using namespace WebCore;

namespace WebKit {

WebIDBKeyPath WebIDBKeyPath::create(const WebString& keyPath)
{
    return WebIDBKeyPath(IDBKeyPath(keyPath));
}

WebIDBKeyPath WebIDBKeyPath::create(const WebVector<WebString>& keyPath)
{
    Vector<String> strings;
    for (size_t i = 0; i < keyPath.size(); ++i)
        strings.append(keyPath[i]);
    return WebIDBKeyPath(IDBKeyPath(strings));
}

WebIDBKeyPath WebIDBKeyPath::createNull()
{
    return WebIDBKeyPath(IDBKeyPath());
}

void WebIDBKeyPath::assign(const WebIDBKeyPath& keyPath)
{
    ASSERT(keyPath.m_private.get());
    m_private.reset(new IDBKeyPath(keyPath));
}

void WebIDBKeyPath::reset()
{
    m_private.reset(0);
}

bool WebIDBKeyPath::isValid() const
{
    ASSERT(m_private.get());
    return m_private->isValid();
}

WebIDBKeyPath::Type WebIDBKeyPath::type() const
{
    ASSERT(m_private.get());
    return Type(m_private->type());
}


WebVector<WebString> WebIDBKeyPath::array() const
{
    ASSERT(m_private.get());
    ASSERT(m_private->type() == IDBKeyPath::ArrayType);
    return m_private->array();
}

WebString WebIDBKeyPath::string() const
{
    ASSERT(m_private.get());
    ASSERT(m_private->type() == IDBKeyPath::StringType);
    return m_private->string();
}

WebIDBKeyPath::WebIDBKeyPath(const WebCore::IDBKeyPath& value)
    : m_private(new IDBKeyPath(value))
{
    ASSERT(m_private.get());
}

WebIDBKeyPath& WebIDBKeyPath::operator=(const WebCore::IDBKeyPath& value)
{
    ASSERT(m_private.get());
    m_private.reset(new IDBKeyPath(value));
    return *this;
}

WebIDBKeyPath::operator const WebCore::IDBKeyPath&() const
{
    ASSERT(m_private.get());
    return *(m_private.get());
}

} // namespace WebKit

#endif // ENABLE(INDEXED_DATABASE)
