/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008 Apple Inc. All rights reserved.
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
#include "JSCell.h"

#include "JSFunction.h"
#include "JSString.h"
#include "JSObject.h"
#include "NumberObject.h"
#include <wtf/MathExtras.h>

namespace JSC {

bool JSCell::getString(ExecState* exec, UString&stringValue) const
{
    if (!isString())
        return false;
    stringValue = static_cast<const JSString*>(this)->value(exec);
    return true;
}

UString JSCell::getString(ExecState* exec) const
{
    return isString() ? static_cast<const JSString*>(this)->value(exec) : UString();
}

JSObject* JSCell::getObject()
{
    return isObject() ? asObject(this) : 0;
}

const JSObject* JSCell::getObject() const
{
    return isObject() ? static_cast<const JSObject*>(this) : 0;
}

CallType JSCell::getCallDataVirtual(CallData& callData)
{
    return getCallData(this, callData);
}

CallType JSCell::getCallData(JSCell*, CallData&)
{
    return CallTypeNone;
}

ConstructType JSCell::getConstructData(ConstructData&)
{
    return ConstructTypeNone;
}

bool JSCell::getOwnPropertySlot(ExecState* exec, const Identifier& identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

bool JSCell::getOwnPropertySlot(ExecState* exec, unsigned identifier, PropertySlot& slot)
{
    // This is not a general purpose implementation of getOwnPropertySlot.
    // It should only be called by JSValue::get.
    // It calls getPropertySlot, not getOwnPropertySlot.
    JSObject* object = toObject(exec, exec->lexicalGlobalObject());
    slot.setBase(object);
    if (!object->getPropertySlot(exec, identifier, slot))
        slot.setUndefined();
    return true;
}

void JSCell::put(ExecState* exec, const Identifier& identifier, JSValue value, PutPropertySlot& slot)
{
    put(this, exec, identifier, value, slot);
}

void JSCell::put(JSCell* cell, ExecState* exec, const Identifier& identifier, JSValue value, PutPropertySlot& slot)
{
    cell->toObject(exec, exec->lexicalGlobalObject())->put(exec, identifier, value, slot);
}

void JSCell::put(ExecState* exec, unsigned identifier, JSValue value)
{
    put(this, exec, identifier, value);
}

void JSCell::put(JSCell* cell, ExecState* exec, unsigned identifier, JSValue value)
{
    cell->toObject(exec, exec->lexicalGlobalObject())->put(exec, identifier, value);
}

bool JSCell::deleteProperty(ExecState* exec, const Identifier& identifier)
{
    return deleteProperty(this, exec, identifier);
}

bool JSCell::deleteProperty(JSCell* cell, ExecState* exec, const Identifier& identifier)
{
    return cell->toObject(exec, exec->lexicalGlobalObject())->deleteProperty(exec, identifier);
}

bool JSCell::deleteProperty(ExecState* exec, unsigned identifier)
{
    return deleteProperty(this, exec, identifier);
}

bool JSCell::deleteProperty(JSCell* cell, ExecState* exec, unsigned identifier)
{
    return cell->toObject(exec, exec->lexicalGlobalObject())->deleteProperty(exec, identifier);
}

JSObject* JSCell::toThisObject(ExecState* exec) const
{
    return toObject(exec, exec->lexicalGlobalObject());
}

JSValue JSCell::toPrimitive(ExecState* exec, PreferredPrimitiveType preferredType) const
{
    if (isString())
        return static_cast<const JSString*>(this)->toPrimitive(exec, preferredType);
    return static_cast<const JSObject*>(this)->toPrimitive(exec, preferredType);
}

bool JSCell::getPrimitiveNumber(ExecState* exec, double& number, JSValue& value) const
{
    if (isString())
        return static_cast<const JSString*>(this)->getPrimitiveNumber(exec, number, value);
    return static_cast<const JSObject*>(this)->getPrimitiveNumber(exec, number, value);
}

double JSCell::toNumber(ExecState*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

UString JSCell::toString(ExecState*) const
{
    ASSERT_NOT_REACHED();
    return UString();
}

JSObject* JSCell::toObject(ExecState* exec, JSGlobalObject* globalObject) const
{
    if (isString())
        return static_cast<const JSString*>(this)->toObject(exec, globalObject);
    ASSERT(isObject());
    return static_cast<JSObject*>(const_cast<JSCell*>(this));
}

void slowValidateCell(JSCell* cell)
{
    ASSERT_GC_OBJECT_LOOKS_VALID(cell);
}

} // namespace JSC
