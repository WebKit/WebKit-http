/*
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All Rights Reserved.
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
#include "JSEventListener.h"

#include "Event.h"
#include "Frame.h"
#include "InspectorCounters.h"
#include "JSEvent.h"
#include "JSEventTarget.h"
#include "JSMainThreadExecState.h"
#include "WorkerContext.h"
#include <runtime/ExceptionHelpers.h>
#include <runtime/JSLock.h>
#include <wtf/RefCountedLeakCounter.h>

using namespace JSC;

namespace WebCore {

JSEventListener::JSEventListener(JSObject* function, JSObject* wrapper, bool isAttribute, DOMWrapperWorld* isolatedWorld)
    : EventListener(JSEventListenerType)
    , m_wrapper(wrapper)
    , m_isAttribute(isAttribute)
    , m_isolatedWorld(isolatedWorld)
{
    if (wrapper)
        m_jsFunction.setMayBeNull(*m_isolatedWorld->globalData(), wrapper, function);
    else
        ASSERT(!function);
#if ENABLE(INSPECTOR)
    ThreadLocalInspectorCounters::current().incrementCounter(ThreadLocalInspectorCounters::JSEventListenerCounter);
#endif
}

JSEventListener::~JSEventListener()
{
#if ENABLE(INSPECTOR)
    ThreadLocalInspectorCounters::current().decrementCounter(ThreadLocalInspectorCounters::JSEventListenerCounter);
#endif
}

JSObject* JSEventListener::initializeJSFunction(ScriptExecutionContext*) const
{
    ASSERT_NOT_REACHED();
    return 0;
}

void JSEventListener::visitJSFunction(SlotVisitor& visitor)
{
    if (m_jsFunction)
        visitor.append(&m_jsFunction);
}

void JSEventListener::handleEvent(ScriptExecutionContext* scriptExecutionContext, Event* event)
{
    ASSERT(scriptExecutionContext);
    if (!scriptExecutionContext || scriptExecutionContext->isJSExecutionForbidden())
        return;

    JSLockHolder lock(scriptExecutionContext->globalData());

    JSObject* jsFunction = this->jsFunction(scriptExecutionContext);
    if (!jsFunction)
        return;

    JSDOMGlobalObject* globalObject = toJSDOMGlobalObject(scriptExecutionContext, m_isolatedWorld.get());
    if (!globalObject)
        return;

    if (scriptExecutionContext->isDocument()) {
        JSDOMWindow* window = jsCast<JSDOMWindow*>(globalObject);
        if (!window->impl()->isCurrentlyDisplayedInFrame())
            return;
        // FIXME: Is this check needed for other contexts?
        ScriptController* script = window->impl()->frame()->script();
        if (!script->canExecuteScripts(AboutToExecuteScript) || script->isPaused())
            return;
    }

    ExecState* exec = globalObject->globalExec();
    JSValue handleEventFunction = jsFunction;

    CallData callData;
    CallType callType = getCallData(handleEventFunction, callData);
    // If jsFunction is not actually a function, see if it implements the EventListener interface and use that
    if (callType == CallTypeNone) {
        handleEventFunction = jsFunction->get(exec, Identifier(exec, "handleEvent"));
        callType = getCallData(handleEventFunction, callData);
    }

    if (callType != CallTypeNone) {
        RefPtr<JSEventListener> protect(this);

        MarkedArgumentBuffer args;
        args.append(toJS(exec, globalObject, event));

        Event* savedEvent = globalObject->currentEvent();
        globalObject->setCurrentEvent(event);

        JSGlobalData& globalData = globalObject->globalData();
        DynamicGlobalObjectScope globalObjectScope(globalData, globalData.dynamicGlobalObject ? globalData.dynamicGlobalObject : globalObject);

        globalData.timeoutChecker.start();
        InspectorInstrumentationCookie cookie = JSMainThreadExecState::instrumentFunctionCall(scriptExecutionContext, callType, callData);

        JSValue thisValue = handleEventFunction == jsFunction ? toJS(exec, globalObject, event->currentTarget()) : jsFunction;
        JSValue retval = scriptExecutionContext->isDocument()
            ? JSMainThreadExecState::call(exec, handleEventFunction, callType, callData, thisValue, args)
            : JSC::call(exec, handleEventFunction, callType, callData, thisValue, args);

        InspectorInstrumentation::didCallFunction(cookie);
        globalData.timeoutChecker.stop();

        globalObject->setCurrentEvent(savedEvent);

#if ENABLE(WORKERS)
        if (scriptExecutionContext->isWorkerContext()) {
            bool terminatorCausedException = (exec->hadException() && isTerminatedExecutionException(exec->exception()));
            if (terminatorCausedException || globalData.terminator.shouldTerminate())
                static_cast<WorkerContext*>(scriptExecutionContext)->script()->forbidExecution();
        }
#endif

        if (exec->hadException()) {
            event->target()->uncaughtExceptionInEventHandler();
            reportCurrentException(exec);
        } else {
            if (!retval.isUndefinedOrNull() && event->storesResultAsString())
                event->storeResult(retval.toString(exec)->value(exec));
            if (m_isAttribute) {
                if (retval.isFalse())
                    event->preventDefault();
            }
        }
    }
}

bool JSEventListener::virtualisAttribute() const
{
    return m_isAttribute;
}

bool JSEventListener::operator==(const EventListener& listener)
{
    if (const JSEventListener* jsEventListener = JSEventListener::cast(&listener))
        return m_jsFunction == jsEventListener->m_jsFunction && m_isAttribute == jsEventListener->m_isAttribute;
    return false;
}

} // namespace WebCore
