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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#import "config.h"

#import "runtime/ObjectPrototype.h"
#import "JSDOMBinding.h"
#import "ObjCRuntimeObject.h"
#import "objc_instance.h"

namespace JSC {
namespace Bindings {

const ClassInfo ObjCRuntimeObject::s_info = { "ObjCRuntimeObject", &RuntimeObject::s_info, 0, 0 };

ObjCRuntimeObject::ObjCRuntimeObject(ExecState* exec, JSGlobalObject* globalObject, PassRefPtr<ObjcInstance> instance, Structure* structure)
    // FIXME: deprecatedGetDOMStructure uses the prototype off of the wrong global object
    // We need to pass in the right global object for "i".
    : RuntimeObject(exec, globalObject, structure, instance)
{
    ASSERT(inherits(&s_info));
}

ObjCRuntimeObject::~ObjCRuntimeObject()
{
}

ObjcInstance* ObjCRuntimeObject::getInternalObjCInstance() const
{
    return static_cast<ObjcInstance*>(getInternalInstance());
}


}
}
