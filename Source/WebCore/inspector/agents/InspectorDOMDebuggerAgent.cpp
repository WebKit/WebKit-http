/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2015 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "InspectorDOMDebuggerAgent.h"

#include "Event.h"
#include "EventTarget.h"
#include "InspectorDOMAgent.h"
#include "InstrumentingAgents.h"
#include "JSEvent.h"
#include "RegisteredEventListener.h"
#include "ScriptExecutionContext.h"
#include <JavaScriptCore/ContentSearchUtilities.h>
#include <JavaScriptCore/InjectedScript.h>
#include <JavaScriptCore/InjectedScriptManager.h>
#include <JavaScriptCore/InspectorFrontendDispatchers.h>
#include <JavaScriptCore/RegularExpression.h>
#include <wtf/JSONValues.h>

namespace WebCore {

using namespace Inspector;

InspectorDOMDebuggerAgent::InspectorDOMDebuggerAgent(WebAgentContext& context, InspectorDebuggerAgent* debuggerAgent)
    : InspectorAgentBase("DOMDebugger"_s, context)
    , m_debuggerAgent(debuggerAgent)
    , m_backendDispatcher(Inspector::DOMDebuggerBackendDispatcher::create(context.backendDispatcher, this))
    , m_injectedScriptManager(context.injectedScriptManager)
{
    m_debuggerAgent->addListener(*this);
}

InspectorDOMDebuggerAgent::~InspectorDOMDebuggerAgent() = default;

bool InspectorDOMDebuggerAgent::enabled() const
{
    return m_instrumentingAgents.enabledDOMDebuggerAgent() == this;
}

void InspectorDOMDebuggerAgent::enable()
{
    m_instrumentingAgents.setEnabledDOMDebuggerAgent(this);
}

void InspectorDOMDebuggerAgent::disable()
{
    m_instrumentingAgents.setEnabledDOMDebuggerAgent(nullptr);

    m_listenerBreakpoints.clear();
    m_urlBreakpoints.clear();
    m_pauseOnAllIntervalsEnabled = false;
    m_pauseOnAllListenersEnabled = false;
    m_pauseOnAllTimeoutsEnabled = false;
    m_pauseOnAllURLsEnabled = false;
}

// Browser debugger agent enabled only when JS debugger is enabled.
void InspectorDOMDebuggerAgent::debuggerWasEnabled()
{
    ASSERT(!enabled());
    enable();
}

void InspectorDOMDebuggerAgent::debuggerWasDisabled()
{
    ASSERT(enabled());
    disable();
}

void InspectorDOMDebuggerAgent::didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*)
{
}

void InspectorDOMDebuggerAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    disable();
}

void InspectorDOMDebuggerAgent::discardAgent()
{
    m_debuggerAgent->removeListener(*this);
    m_debuggerAgent = nullptr;
}

void InspectorDOMDebuggerAgent::setEventBreakpoint(ErrorString& errorString, const String& breakpointTypeString, const String* eventName)
{
    if (breakpointTypeString.isEmpty()) {
        errorString = "breakpointType is empty"_s;
        return;
    }

    auto breakpointType = Inspector::Protocol::InspectorHelpers::parseEnumValueFromString<Inspector::Protocol::DOMDebugger::EventBreakpointType>(breakpointTypeString);
    if (!breakpointType) {
        errorString = makeString("Unknown breakpointType: "_s, breakpointTypeString);
        return;
    }

    if (eventName && !eventName->isEmpty()) {
        if (breakpointType.value() == Inspector::Protocol::DOMDebugger::EventBreakpointType::Listener) {
            if (!m_listenerBreakpoints.add(*eventName))
                errorString = "Breakpoint with eventName already exists"_s;
            return;
        }

        errorString = "Unexpected eventName"_s;
        return;
    }

    switch (breakpointType.value()) {
    case Inspector::Protocol::DOMDebugger::EventBreakpointType::AnimationFrame:
        setAnimationFrameBreakpoint(errorString, true);
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Interval:
        if (m_pauseOnAllIntervalsEnabled)
            errorString = "Breakpoint for Interval already exists"_s;
        m_pauseOnAllIntervalsEnabled = true;
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Listener:
        if (m_pauseOnAllListenersEnabled)
            errorString = "Breakpoint for Listener already exists"_s;
        m_pauseOnAllListenersEnabled = true;
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Timeout:
        if (m_pauseOnAllTimeoutsEnabled)
            errorString = "Breakpoint for Timeout already exists"_s;
        m_pauseOnAllTimeoutsEnabled = true;
        break;
    }
}

void InspectorDOMDebuggerAgent::removeEventBreakpoint(ErrorString& errorString, const String& breakpointTypeString, const String* eventName)
{
    if (breakpointTypeString.isEmpty()) {
        errorString = "breakpointType is empty"_s;
        return;
    }

    auto breakpointType = Inspector::Protocol::InspectorHelpers::parseEnumValueFromString<Inspector::Protocol::DOMDebugger::EventBreakpointType>(breakpointTypeString);
    if (!breakpointType) {
        errorString = makeString("Unknown breakpointType: "_s, breakpointTypeString);
        return;
    }

    if (eventName && !eventName->isEmpty()) {
        if (breakpointType.value() == Inspector::Protocol::DOMDebugger::EventBreakpointType::Listener) {
            if (!m_listenerBreakpoints.remove(*eventName))
                errorString = "Breakpoint for given eventName missing"_s;
            return;
        }

        errorString = "Unexpected eventName"_s;
        return;
    }

    switch (breakpointType.value()) {
    case Inspector::Protocol::DOMDebugger::EventBreakpointType::AnimationFrame:
        setAnimationFrameBreakpoint(errorString, false);
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Interval:
        if (!m_pauseOnAllIntervalsEnabled)
            errorString = "Breakpoint for Intervals missing"_s;
        m_pauseOnAllIntervalsEnabled = false;
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Listener:
        if (!m_pauseOnAllListenersEnabled)
            errorString = "Breakpoint for Listeners missing"_s;
        m_pauseOnAllListenersEnabled = false;
        break;

    case Inspector::Protocol::DOMDebugger::EventBreakpointType::Timeout:
        if (!m_pauseOnAllTimeoutsEnabled)
            errorString = "Breakpoint for Timeouts missing"_s;
        m_pauseOnAllTimeoutsEnabled = false;
        break;
    }
}

void InspectorDOMDebuggerAgent::willHandleEvent(Event& event, const RegisteredEventListener& registeredEventListener)
{
    if (!m_debuggerAgent->breakpointsActive())
        return;

    auto state = event.target()->scriptExecutionContext()->execState();
    auto injectedScript = m_injectedScriptManager.injectedScriptFor(state);
    if (injectedScript.hasNoValue())
        return;

    {
        JSC::JSLockHolder lock(state);

        injectedScript.setEventValue(toJS(state, deprecatedGlobalObjectForPrototype(state), event));
    }

    auto* domAgent = m_instrumentingAgents.persistentDOMAgent();

    bool shouldPause = m_debuggerAgent->pauseOnNextStatementEnabled() || m_pauseOnAllListenersEnabled || m_listenerBreakpoints.contains(event.type());
    if (!shouldPause && domAgent)
        shouldPause = domAgent->hasBreakpointForEventListener(*event.currentTarget(), event.type(), registeredEventListener.callback(), registeredEventListener.useCapture());
    if (!shouldPause)
        return;

    Ref<JSON::Object> eventData = JSON::Object::create();
    eventData->setString("eventName"_s, event.type());
    if (domAgent) {
        int eventListenerId = domAgent->idForEventListener(*event.currentTarget(), event.type(), registeredEventListener.callback(), registeredEventListener.useCapture());
        if (eventListenerId)
            eventData->setInteger("eventListenerId"_s, eventListenerId);
    }

    m_debuggerAgent->schedulePauseOnNextStatement(Inspector::DebuggerFrontendDispatcher::Reason::Listener, WTFMove(eventData));
}

void InspectorDOMDebuggerAgent::didHandleEvent()
{
    m_injectedScriptManager.clearEventValue();
}

void InspectorDOMDebuggerAgent::willFireTimer(bool oneShot)
{
    if (!m_debuggerAgent->breakpointsActive())
        return;

    bool shouldPause = m_debuggerAgent->pauseOnNextStatementEnabled() || (oneShot ? m_pauseOnAllTimeoutsEnabled : m_pauseOnAllIntervalsEnabled);
    if (!shouldPause)
        return;

    auto breakReason = oneShot ? Inspector::DebuggerFrontendDispatcher::Reason::Timeout : Inspector::DebuggerFrontendDispatcher::Reason::Interval;
    m_debuggerAgent->schedulePauseOnNextStatement(breakReason, nullptr);
}

void InspectorDOMDebuggerAgent::setURLBreakpoint(ErrorString& errorString, const String& url, const bool* optionalIsRegex)
{
    if (url.isEmpty()) {
        if (m_pauseOnAllURLsEnabled)
            errorString = "Breakpoint for all URLs already exists"_s;
        m_pauseOnAllURLsEnabled = true;
        return;
    }

    bool isRegex = optionalIsRegex ? *optionalIsRegex : false;
    auto result = m_urlBreakpoints.set(url, isRegex ? URLBreakpointType::RegularExpression : URLBreakpointType::Text);
    if (!result.isNewEntry)
        errorString = "Breakpoint for given url already exists"_s;
}

void InspectorDOMDebuggerAgent::removeURLBreakpoint(ErrorString& errorString, const String& url)
{
    if (url.isEmpty()) {
        if (!m_pauseOnAllURLsEnabled)
            errorString = "Breakpoint for all URLs missing"_s;
        m_pauseOnAllURLsEnabled = false;
        return;
    }

    auto result = m_urlBreakpoints.remove(url);
    if (!result)
        errorString = "Breakpoint for given url missing"_s;
}

void InspectorDOMDebuggerAgent::breakOnURLIfNeeded(const String& url, URLBreakpointSource source)
{
    if (!m_debuggerAgent->breakpointsActive())
        return;

    String breakpointURL;
    if (m_pauseOnAllURLsEnabled)
        breakpointURL = emptyString();
    else {
        for (auto& [query, type] : m_urlBreakpoints) {
            bool isRegex = type == URLBreakpointType::RegularExpression;
            auto searchStringType = isRegex ? ContentSearchUtilities::SearchStringType::Regex : ContentSearchUtilities::SearchStringType::ContainsString;
            auto regex = ContentSearchUtilities::createRegularExpressionForSearchString(query, false, searchStringType);
            if (regex.match(url) != -1) {
                breakpointURL = query;
                break;
            }
        }
    }

    if (breakpointURL.isNull())
        return;

    Inspector::DebuggerFrontendDispatcher::Reason breakReason;
    if (source == URLBreakpointSource::Fetch)
        breakReason = Inspector::DebuggerFrontendDispatcher::Reason::Fetch;
    else if (source == URLBreakpointSource::XHR)
        breakReason = Inspector::DebuggerFrontendDispatcher::Reason::XHR;
    else {
        ASSERT_NOT_REACHED();
        breakReason = Inspector::DebuggerFrontendDispatcher::Reason::Other;
    }

    Ref<JSON::Object> eventData = JSON::Object::create();
    eventData->setString("breakpointURL", breakpointURL);
    eventData->setString("url", url);
    m_debuggerAgent->breakProgram(breakReason, WTFMove(eventData));
}

void InspectorDOMDebuggerAgent::willSendXMLHttpRequest(const String& url)
{
    breakOnURLIfNeeded(url, URLBreakpointSource::XHR);
}

void InspectorDOMDebuggerAgent::willFetch(const String& url)
{
    breakOnURLIfNeeded(url, URLBreakpointSource::Fetch);
}

} // namespace WebCore
