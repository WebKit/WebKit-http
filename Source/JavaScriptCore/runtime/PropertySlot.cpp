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

#include "JSFunction.h"
#include "JSGlobalObject.h"

namespace JSC {

JSValue PropertySlot::functionGetter(ExecState* exec) const
{
    // Prevent getter functions from observing execution if an exception is pending.
    if (exec->hadException())
        return exec->exception();

    CallData callData;
    CallType callType = m_data.getterFunc->getCallData(callData);
    
    // Only objects can have accessor properties.
    // If the base is WebCore's global object then we need to substitute the shell.
    ASSERT(m_slotBase.isObject());
    return call(exec, m_data.getterFunc, callType, callData, m_thisValue.toThisObject(exec), exec->emptyList());
}

} // namespace JSC
