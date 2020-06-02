/*
 *  Copyright (C) 2005, 2008 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "PropertySlot.h"

#include "GetterSetter.h"
#include "JSCJSValueInlines.h"
#include "JSObject.h"

namespace JSC {

JSValue PropertySlot::functionGetter(JSGlobalObject* globalObject) const
{
    ASSERT(m_thisValue);
    return callGetter(globalObject, m_thisValue, m_data.getter.getterSetter);
}

JSValue PropertySlot::customGetter(JSGlobalObject* globalObject, PropertyName propertyName) const
{
    // FIXME: Remove this differences in custom values and custom accessors.
    // https://bugs.webkit.org/show_bug.cgi?id=158014
    JSValue thisValue = m_attributes & PropertyAttribute::CustomAccessor ? m_thisValue : JSValue(slotBase());
    if (auto domAttribute = this->domAttribute()) {
        VM& vm = globalObject->vm();
        if (!thisValue.inherits(vm, domAttribute->classInfo)) {
            auto scope = DECLARE_THROW_SCOPE(vm);
            return throwDOMAttributeGetterTypeError(globalObject, scope, domAttribute->classInfo, propertyName);
        }
    }
    return JSValue::decode(m_data.custom.getValue(globalObject, JSValue::encode(thisValue), propertyName));
}

JSValue PropertySlot::customAccessorGetter(JSGlobalObject* globalObject, PropertyName propertyName) const
{
    if (!m_data.customAccessor.getterSetter->getter())
        return jsUndefined();

    if (auto domAttribute = this->domAttribute()) {
        VM& vm = globalObject->vm();
        if (!m_thisValue.inherits(vm, domAttribute->classInfo)) {
            auto scope = DECLARE_THROW_SCOPE(vm);
            return throwDOMAttributeGetterTypeError(globalObject, scope, domAttribute->classInfo, propertyName);
        }
    }
    return JSValue::decode(m_data.customAccessor.getterSetter->getter()(globalObject, JSValue::encode(m_thisValue), propertyName));
}

JSValue PropertySlot::getPureResult() const
{
    JSValue result;
    if (isTaintedByOpaqueObject())
        result = jsNull();
    else if (isCacheableValue())
        result = JSValue::decode(m_data.value);
    else if (isCacheableGetter())
        result = getterSetter();
    else if (isUnset())
        result = jsUndefined();
    else
        result = jsNull();
    
    return result;
}

} // namespace JSC
