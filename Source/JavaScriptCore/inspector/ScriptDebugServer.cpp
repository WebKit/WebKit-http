/*
 * Copyright (C) 2008, 2009, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
 * Copyright (C) 2013 University of Washington. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ScriptDebugServer.h"

#include "DebuggerCallFrame.h"
#include "DebuggerScope.h"
#include "Exception.h"
#include "JSJavaScriptCallFrame.h"
#include "JavaScriptCallFrame.h"
#include "SourceProvider.h"
#include <wtf/NeverDestroyed.h>
#include <wtf/SetForScope.h>

namespace Inspector {

using namespace JSC;

ScriptDebugServer::ScriptDebugServer(VM& vm)
    : Debugger(vm)
{
}

ScriptDebugServer::~ScriptDebugServer()
{
}

void ScriptDebugServer::setBreakpointActions(BreakpointID id, const ScriptBreakpoint& scriptBreakpoint)
{
    ASSERT(id != noBreakpointID);
    ASSERT(!m_breakpointIDToActions.contains(id));

    m_breakpointIDToActions.set(id, scriptBreakpoint.actions);
}

void ScriptDebugServer::removeBreakpointActions(BreakpointID id)
{
    ASSERT(id != noBreakpointID);

    m_breakpointIDToActions.remove(id);
}

const BreakpointActions& ScriptDebugServer::getActionsForBreakpoint(BreakpointID id)
{
    ASSERT(id != noBreakpointID);

    auto entry = m_breakpointIDToActions.find(id);
    if (entry != m_breakpointIDToActions.end())
        return entry->value;

    static NeverDestroyed<BreakpointActions> emptyActionVector = BreakpointActions();
    return emptyActionVector;
}

void ScriptDebugServer::clearBreakpointActions()
{
    m_breakpointIDToActions.clear();
}

bool ScriptDebugServer::evaluateBreakpointAction(const ScriptBreakpointAction& breakpointAction)
{
    DebuggerCallFrame& debuggerCallFrame = currentDebuggerCallFrame();

    switch (breakpointAction.type) {
    case ScriptBreakpointActionTypeLog:
        dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
            listener.breakpointActionLog(debuggerCallFrame.globalObject(), breakpointAction.data);
        });
        break;

    case ScriptBreakpointActionTypeEvaluate: {
        NakedPtr<Exception> exception;
        JSObject* scopeExtensionObject = nullptr;
        debuggerCallFrame.evaluateWithScopeExtension(breakpointAction.data, scopeExtensionObject, exception);
        if (exception)
            reportException(debuggerCallFrame.globalObject(), exception);
        break;
    }

    case ScriptBreakpointActionTypeSound:
        dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
            listener.breakpointActionSound(breakpointAction.identifier);
        });
        break;

    case ScriptBreakpointActionTypeProbe: {
        NakedPtr<Exception> exception;
        JSObject* scopeExtensionObject = nullptr;
        JSValue result = debuggerCallFrame.evaluateWithScopeExtension(breakpointAction.data, scopeExtensionObject, exception);
        JSC::JSGlobalObject* globalObject = debuggerCallFrame.globalObject();
        if (exception)
            reportException(globalObject, exception);

        dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
            listener.breakpointActionProbe(globalObject, breakpointAction, m_currentProbeBatchId, m_nextProbeSampleId++, exception ? exception->value() : result);
        });
        break;
    }

    default:
        ASSERT_NOT_REACHED();
    }

    return true;
}

void ScriptDebugServer::sourceParsed(JSGlobalObject* globalObject, SourceProvider* sourceProvider, int errorLine, const String& errorMessage)
{
    // Preemptively check whether we can dispatch so that we don't do any unnecessary allocations.
    if (!canDispatchFunctionToListeners())
        return;

    if (errorLine != -1) {
        auto sourceURL = sourceProvider->sourceURL();
        auto data = sourceProvider->source().toString();
        auto firstLine = sourceProvider->startPosition().m_line.oneBasedInt();
        dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
            listener.failedToParseSource(sourceURL, data, firstLine, errorLine, errorMessage);
        });
        return;
    }

    JSC::SourceID sourceID = sourceProvider->asID();

    // FIXME: <https://webkit.org/b/162773> Web Inspector: Simplify ScriptDebugListener::Script to use SourceProvider
    ScriptDebugListener::Script script;
    script.sourceProvider = sourceProvider;
    script.url = sourceProvider->sourceURL();
    script.source = sourceProvider->source().toString();
    script.startLine = sourceProvider->startPosition().m_line.zeroBasedInt();
    script.startColumn = sourceProvider->startPosition().m_column.zeroBasedInt();
    script.isContentScript = isContentScript(globalObject);
    script.sourceURL = sourceProvider->sourceURLDirective();
    script.sourceMappingURL = sourceProvider->sourceMappingURLDirective();

    int sourceLength = script.source.length();
    int lineCount = 1;
    int lastLineStart = 0;
    for (int i = 0; i < sourceLength; ++i) {
        if (script.source[i] == '\n') {
            lineCount += 1;
            lastLineStart = i + 1;
        }
    }

    script.endLine = script.startLine + lineCount - 1;
    if (lineCount == 1)
        script.endColumn = script.startColumn + sourceLength;
    else
        script.endColumn = sourceLength - lastLineStart;

    dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
        listener.didParseSource(sourceID, script);
    });
}

void ScriptDebugServer::willRunMicrotask()
{
    dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
        listener.willRunMicrotask();
    });
}

void ScriptDebugServer::didRunMicrotask()
{
    dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
        listener.didRunMicrotask();
    });
}

bool ScriptDebugServer::canDispatchFunctionToListeners() const
{
    if (m_callingListeners)
        return false;
    if (m_listeners.isEmpty())
        return false;
    return true;
}

void ScriptDebugServer::dispatchFunctionToListeners(Function<void(ScriptDebugListener&)> callback)
{
    if (!canDispatchFunctionToListeners())
        return;

    SetForScope<bool> change(m_callingListeners, true);

    for (auto* listener : copyToVector(m_listeners))
        callback(*listener);
}

void ScriptDebugServer::notifyDoneProcessingDebuggerEvents()
{
    m_doneProcessingDebuggerEvents = true;
}

void ScriptDebugServer::handleBreakpointHit(JSC::JSGlobalObject* globalObject, const JSC::Breakpoint& breakpoint)
{
    ASSERT(isAttached(globalObject));

    m_currentProbeBatchId++;

    auto entry = m_breakpointIDToActions.find(breakpoint.id);
    if (entry != m_breakpointIDToActions.end()) {
        BreakpointActions actions = entry->value;
        for (size_t i = 0; i < actions.size(); ++i) {
            if (!evaluateBreakpointAction(actions[i]))
                return;
            if (!isAttached(globalObject))
                return;
        }
    }
}

void ScriptDebugServer::handleExceptionInBreakpointCondition(JSC::JSGlobalObject* globalObject, JSC::Exception* exception) const
{
    reportException(globalObject, exception);
}

void ScriptDebugServer::handlePause(JSGlobalObject* globalObject, Debugger::ReasonForPause)
{
    dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
        ASSERT(isPaused());
        auto& debuggerCallFrame = currentDebuggerCallFrame();
        auto* globalObject = debuggerCallFrame.scope()->globalObject();
        auto jsCallFrame = toJS(globalObject, globalObject, JavaScriptCallFrame::create(debuggerCallFrame).ptr());
        listener.didPause(globalObject, jsCallFrame, exceptionOrCaughtValue(globalObject));
    });

    didPause(globalObject);

    m_doneProcessingDebuggerEvents = false;
    runEventLoopWhilePaused();

    didContinue(globalObject);

    dispatchFunctionToListeners([&] (ScriptDebugListener& listener) {
        listener.didContinue();
    });
}

void ScriptDebugServer::addListener(ScriptDebugListener* listener)
{
    ASSERT(listener);

    bool wasEmpty = m_listeners.isEmpty();
    m_listeners.add(listener);

    // First listener. Attach the debugger.
    if (wasEmpty)
        attachDebugger();
}

void ScriptDebugServer::removeListener(ScriptDebugListener* listener, bool isBeingDestroyed)
{
    ASSERT(listener);

    m_listeners.remove(listener);

    // Last listener. Detach the debugger.
    if (m_listeners.isEmpty())
        detachDebugger(isBeingDestroyed);
}

JSC::JSValue ScriptDebugServer::exceptionOrCaughtValue(JSC::JSGlobalObject* globalObject)
{
    if (reasonForPause() == PausedForException)
        return currentException();

    for (RefPtr<DebuggerCallFrame> frame = &currentDebuggerCallFrame(); frame; frame = frame->callerFrame()) {
        DebuggerScope& scope = *frame->scope();
        if (scope.isCatchScope())
            return scope.caughtValue(globalObject);
    }

    return { };
}

} // namespace Inspector
