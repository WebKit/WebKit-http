/*
 * Copyright (C) 2008, 2009 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
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
 *
 */

#include "config.h"
#include "JSDOMGlobalObject.h"

#include "Document.h"
#include "JSDOMWindow.h"
#include "JSEventListener.h"

#if ENABLE(WORKERS)
#include "JSWorkerContext.h"
#include "WorkerContext.h"
#endif

using namespace JSC;

namespace WebCore {

const ClassInfo JSDOMGlobalObject::s_info = { "DOMGlobalObject", &JSGlobalObject::s_info, 0, 0, CREATE_METHOD_TABLE(JSDOMGlobalObject) };

JSDOMGlobalObject::JSDOMGlobalObject(JSGlobalData& globalData, Structure* structure, PassRefPtr<DOMWrapperWorld> world, const GlobalObjectMethodTable* globalObjectMethodTable)
    : JSGlobalObject(globalData, structure, globalObjectMethodTable)
    , m_currentEvent(0)
    , m_world(world)
{
}

JSDOMGlobalObject::~JSDOMGlobalObject()
{
}

void JSDOMGlobalObject::finishCreation(JSGlobalData& globalData)
{
    Base::finishCreation(globalData);
    ASSERT(inherits(&s_info));
}

void JSDOMGlobalObject::finishCreation(JSGlobalData& globalData, JSGlobalThis* thisValue)
{
    Base::finishCreation(globalData, thisValue);
    ASSERT(inherits(&s_info));
}

void JSDOMGlobalObject::visitChildren(JSCell* cell, SlotVisitor& visitor)
{
    JSDOMGlobalObject* thisObject = jsCast<JSDOMGlobalObject*>(cell);
    ASSERT_GC_OBJECT_INHERITS(thisObject, &s_info);
    COMPILE_ASSERT(StructureFlags & OverridesVisitChildren, OverridesVisitChildrenWithoutSettingFlag);
    ASSERT(thisObject->structure()->typeInfo().overridesVisitChildren());
    Base::visitChildren(thisObject, visitor);

    JSDOMStructureMap::iterator end = thisObject->structures().end();
    for (JSDOMStructureMap::iterator it = thisObject->structures().begin(); it != end; ++it)
        visitor.append(&it->second);

    JSDOMConstructorMap::iterator end2 = thisObject->constructors().end();
    for (JSDOMConstructorMap::iterator it2 = thisObject->constructors().begin(); it2 != end2; ++it2)
        visitor.append(&it2->second);

    if (thisObject->m_injectedScript)
        visitor.append(&thisObject->m_injectedScript);
}

void JSDOMGlobalObject::setCurrentEvent(Event* currentEvent)
{
    m_currentEvent = currentEvent;
}

Event* JSDOMGlobalObject::currentEvent() const
{
    return m_currentEvent;
}

void JSDOMGlobalObject::setInjectedScript(JSObject* injectedScript)
{
    m_injectedScript.setMayBeNull(globalData(), this, injectedScript);
}

JSObject* JSDOMGlobalObject::injectedScript() const
{
    return m_injectedScript.get();
}

JSDOMGlobalObject* toJSDOMGlobalObject(Document* document, JSC::ExecState* exec)
{
    return toJSDOMWindow(document->frame(), currentWorld(exec));
}

JSDOMGlobalObject* toJSDOMGlobalObject(ScriptExecutionContext* scriptExecutionContext, JSC::ExecState* exec)
{
    if (scriptExecutionContext->isDocument())
        return toJSDOMGlobalObject(static_cast<Document*>(scriptExecutionContext), exec);

#if ENABLE(WORKERS)
    if (scriptExecutionContext->isWorkerContext())
        return static_cast<WorkerContext*>(scriptExecutionContext)->script()->workerContextWrapper();
#endif

    ASSERT_NOT_REACHED();
    return 0;
}

JSDOMGlobalObject* toJSDOMGlobalObject(Document* document, DOMWrapperWorld* world)
{
    return toJSDOMWindow(document->frame(), world);
}

JSDOMGlobalObject* toJSDOMGlobalObject(ScriptExecutionContext* scriptExecutionContext, DOMWrapperWorld* world)
{
    if (scriptExecutionContext->isDocument())
        return toJSDOMGlobalObject(static_cast<Document*>(scriptExecutionContext), world);

#if ENABLE(WORKERS)
    if (scriptExecutionContext->isWorkerContext())
        return static_cast<WorkerContext*>(scriptExecutionContext)->script()->workerContextWrapper();
#endif

    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace WebCore
