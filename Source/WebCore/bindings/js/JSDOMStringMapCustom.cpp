/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
#include "JSDOMStringMap.h"

#include "DOMStringMap.h"
#include "Element.h"
#include "JSNode.h"
#include <wtf/text/AtomicString.h>

using namespace JSC;

namespace WebCore {

bool JSDOMStringMap::canGetItemsForName(ExecState*, DOMStringMap* impl, const Identifier& propertyName)
{
    return impl->contains(identifierToAtomicString(propertyName));
}

JSValue JSDOMStringMap::nameGetter(ExecState* exec, JSValue slotBase, const Identifier& propertyName)
{
    JSDOMStringMap* thisObj = static_cast<JSDOMStringMap*>(asObject(slotBase));
    return jsString(exec, thisObj->impl()->item(identifierToAtomicString(propertyName)));
}

void JSDOMStringMap::getOwnPropertyNames(ExecState* exec, PropertyNameArray& propertyNames, EnumerationMode mode)
{
    Vector<String> names;
    m_impl->getNames(names);
    size_t length = names.size();
    for (size_t i = 0; i < length; ++i)
        propertyNames.add(Identifier(exec, stringToUString(names[i])));

    Base::getOwnPropertyNames(exec, propertyNames, mode);
}

bool JSDOMStringMap::deleteProperty(ExecState* exec, const Identifier& propertyName)
{
    AtomicString stringName = identifierToAtomicString(propertyName);
    if (!m_impl->contains(stringName))
        return false;
    ExceptionCode ec = 0;
    m_impl->deleteItem(stringName, ec);
    setDOMException(exec, ec);
    return !ec;
}

bool JSDOMStringMap::putDelegate(ExecState* exec, const Identifier& propertyName, JSValue value, PutPropertySlot&)
{
    String stringValue = ustringToString(value.toString(exec));
    if (exec->hadException())
        return false;
    ExceptionCode ec = 0;
    impl()->setItem(identifierToString(propertyName), stringValue, ec);
    setDOMException(exec, ec);
    return !ec;
}

} // namespace WebCore
