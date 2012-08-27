/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "JSGlobalThis.h"

#include "JSGlobalObject.h"

namespace JSC {

ASSERT_CLASS_FITS_IN_CELL(JSGlobalThis);
ASSERT_HAS_TRIVIAL_DESTRUCTOR(JSGlobalThis);

const ClassInfo JSGlobalThis::s_info = { "JSGlobalThis", &Base::s_info, 0, 0, CREATE_METHOD_TABLE(JSGlobalThis) };

void JSGlobalThis::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSGlobalThis* thisObject = jsCast<JSGlobalThis*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);

    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());

    Base::visitChildren(thisObject, visitor);
    visitor.append(&thisObject->m_unwrappedObject);
}

void JSGlobalThis::setUnwrappedObject(JSGlobalData& globalData, JSGlobalObject* globalObject)
{
    ASSERT_ARG(globalObject, globalObject);
    m_unwrappedObject.set(globalData, this, globalObject);
    setPrototype(globalData, globalObject->prototype());
    resetInheritorID(globalData);
}

} // namespace JSC
