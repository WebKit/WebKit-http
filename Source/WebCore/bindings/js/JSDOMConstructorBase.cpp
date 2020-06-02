/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2004-2011, 2013, 2016 Apple Inc. All rights reserved.
 *  Copyright (C) 2007 Samuel Weinig <sam@webkit.org>
 *  Copyright (C) 2013 Michael Pruett <michael@68k.org>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "JSDOMConstructor.h"

#include "WebCoreJSClientData.h"
#include <JavaScriptCore/JSCInlines.h>

namespace WebCore {
using namespace JSC;

STATIC_ASSERT_IS_TRIVIALLY_DESTRUCTIBLE(JSDOMConstructorBase);

static EncodedJSValue JSC_HOST_CALL callThrowTypeError(JSGlobalObject* globalObject, CallFrame*)
{
    VM& vm = globalObject->vm();
    auto scope = DECLARE_THROW_SCOPE(vm);
    throwTypeError(globalObject, scope, "Constructor requires 'new' operator"_s);
    return JSValue::encode(jsNull());
}

CallData JSDOMConstructorBase::getCallData(JSCell*)
{
    CallData callData;
    callData.type = CallData::Type::Native;
    callData.native.function = callThrowTypeError;
    return callData;
}

JSC::IsoSubspace* JSDOMConstructorBase::subspaceForImpl(JSC::VM& vm)
{
    return &static_cast<JSVMClientData*>(vm.clientData)->domConstructorSpace();
}

} // namespace WebCore
