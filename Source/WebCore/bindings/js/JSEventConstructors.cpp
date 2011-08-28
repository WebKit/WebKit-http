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
#include "EventConstructors.h"

#include "Event.h"
#include "JSEvent.h"
#include <runtime/Error.h>

using namespace JSC;

namespace WebCore {

static void convertValue(ExecState* exec, JSValue value, bool& result)
{
    result = value.toBoolean(exec);
}

template <typename Result>
static bool tryGetProperty(ExecState* exec, JSObject* initializerObject, const char* propertyName, Result& result)
{
    Identifier identifier(exec, propertyName);
    PropertySlot slot(initializerObject);
    if (initializerObject->getPropertySlot(exec, identifier, slot)) {
        if (exec->hadException())
            return false;

        JSValue value = slot.getValue(exec, identifier);
        
        convertValue(exec, value, result);

        if (exec->hadException())
            return false;
    }
    
    return true;
}

template<typename Constructor, typename EventType, typename EventInitType>
static EncodedJSValue constructJSEventWithInitializer(ExecState* exec, bool (*filler)(EventInitType&, ExecState*, JSObject*))
{
    Constructor* jsConstructor = static_cast<Constructor*>(exec->callee());

    ScriptExecutionContext* executionContext = jsConstructor->scriptExecutionContext();
    if (!executionContext)
        return throwVMError(exec, createReferenceError(exec, "Constructor associated execution context is unavailable"));

    AtomicString eventType = ustringToAtomicString(exec->argument(0).toString(exec));
    if (exec->hadException())
        return JSValue::encode(jsUndefined());

    EventInitType eventInit;

    JSValue initializerValue = exec->argument(1);
    if (!initializerValue.isUndefinedOrNull()) {
        // Given the above test, this will always yield an object.
        JSObject* initializerObject = initializerValue.toObject(exec);

        // Attempt to fill in the EventInit.
        if (!filler(eventInit, exec, initializerObject))
            return JSValue::encode(jsUndefined());
    }

    RefPtr<EventType> event = EventType::create(eventType, eventInit);
    return JSValue::encode(toJS(exec, jsConstructor->globalObject(), event.get()));
}

#define DICTIONARY_START(Event) \
    static bool fill##Event##Init(Event##Init& eventInit, ExecState* exec, JSObject* initializerObject) \
    {

#define DICTIONARY_END(Event) \
        return true; \
    } \
    \
    EncodedJSValue JSC_HOST_CALL JS##Event##Constructor::constructJS##Event(ExecState* exec) \
    { \
        return constructJSEventWithInitializer<JS##Event##Constructor, Event, Event##Init>(exec, fill##Event##Init); \
    }

#define FILL_PARENT_PROPERTIES(parent) \
    if (!fill##parent##Init(eventInit, exec, initializerObject)) \
        return false;

#define FILL_PROPERTY(propertyName) \
    if (!tryGetProperty(exec, initializerObject, #propertyName, eventInit.propertyName)) \
        return false;

INSTANTIATE_ALL_EVENT_INITIALIZING_CONSTRUCTORS(DICTIONARY_START, DICTIONARY_END, FILL_PARENT_PROPERTIES, FILL_PROPERTY)

} // namespace WebCore
