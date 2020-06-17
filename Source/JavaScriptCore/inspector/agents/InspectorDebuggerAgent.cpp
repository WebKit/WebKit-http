/*
 * Copyright (C) 2010-2017 Apple Inc. All rights reserved.
 * Copyright (C) 2010, 2011 Google Inc. All rights reserved.
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
#include "InspectorDebuggerAgent.h"

#include "AsyncStackTrace.h"
#include "ContentSearchUtilities.h"
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "RegularExpression.h"
#include "ScriptCallStack.h"
#include "ScriptCallStackFactory.h"
#include "ScriptDebugServer.h"
#include <wtf/JSONValues.h>
#include <wtf/Stopwatch.h>
#include <wtf/text/WTFString.h>

namespace Inspector {

const char* const InspectorDebuggerAgent::backtraceObjectGroup = "backtrace";

// Objects created and retained by evaluating breakpoint actions are put into object groups
// according to the breakpoint action identifier assigned by the frontend. A breakpoint may
// have several object groups, and objects from several backend breakpoint action instances may
// create objects in the same group.
static String objectGroupForBreakpointAction(const ScriptBreakpointAction& action)
{
    return makeString("breakpoint-action-", action.identifier);
}

static bool isWebKitInjectedScript(const String& sourceURL)
{
    return sourceURL.startsWith("__InjectedScript_") && sourceURL.endsWith(".js");
}

InspectorDebuggerAgent::InspectorDebuggerAgent(AgentContext& context)
    : InspectorAgentBase("Debugger"_s)
    , m_frontendDispatcher(makeUnique<DebuggerFrontendDispatcher>(context.frontendRouter))
    , m_backendDispatcher(DebuggerBackendDispatcher::create(context.backendDispatcher, this))
    , m_scriptDebugServer(context.environment.scriptDebugServer())
    , m_injectedScriptManager(context.injectedScriptManager)
{
    // FIXME: make pauseReason optional so that there was no need to init it with "other".
    clearPauseDetails();
}

InspectorDebuggerAgent::~InspectorDebuggerAgent() = default;

void InspectorDebuggerAgent::didCreateFrontendAndBackend(FrontendRouter*, BackendDispatcher*)
{
}

void InspectorDebuggerAgent::willDestroyFrontendAndBackend(DisconnectReason reason)
{
    if (enabled())
        disable(reason == DisconnectReason::InspectedTargetDestroyed);
}

void InspectorDebuggerAgent::enable()
{
    m_enabled = true;

    m_scriptDebugServer.addListener(this);

    for (auto* listener : copyToVector(m_listeners))
        listener->debuggerWasEnabled();

    for (auto& [sourceID, script] : m_scripts) {
        Optional<JSC::Debugger::BlackboxType> blackboxType;
        if (isWebKitInjectedScript(script.sourceURL)) {
            if (!m_pauseForInternalScripts)
                blackboxType = JSC::Debugger::BlackboxType::Ignored;
        } else if (shouldBlackboxURL(script.sourceURL) || shouldBlackboxURL(script.url))
            blackboxType = JSC::Debugger::BlackboxType::Deferred;
        m_scriptDebugServer.setBlackboxType(sourceID, blackboxType);
    }
}

void InspectorDebuggerAgent::disable(bool isBeingDestroyed)
{
    for (auto* listener : copyToVector(m_listeners))
        listener->debuggerWasDisabled();

    m_scriptDebugServer.removeListener(this, isBeingDestroyed);

    clearInspectorBreakpointState();

    if (!isBeingDestroyed)
        m_scriptDebugServer.deactivateBreakpoints();

    ASSERT(m_javaScriptBreakpoints.isEmpty());

    clearAsyncStackTraceData();

    m_pauseOnAssertionFailures = false;
    m_pauseOnMicrotasks = false;

    m_enabled = false;
}

void InspectorDebuggerAgent::enable(ErrorString& errorString)
{
    if (enabled()) {
        errorString = "Debugger domain already enabled"_s;
        return;
    }

    enable();
}

void InspectorDebuggerAgent::disable(ErrorString&)
{
    disable(false);
}

bool InspectorDebuggerAgent::breakpointsActive() const
{
    return m_scriptDebugServer.breakpointsActive();
}

void InspectorDebuggerAgent::setAsyncStackTraceDepth(ErrorString& errorString, int depth)
{
    if (m_asyncStackTraceDepth == depth)
        return;

    if (depth < 0) {
        errorString = "Unexpected negative depth"_s;
        return;
    }

    m_asyncStackTraceDepth = depth;

    if (!m_asyncStackTraceDepth)
        clearAsyncStackTraceData();
}

void InspectorDebuggerAgent::setBreakpointsActive(ErrorString&, bool active)
{
    if (active)
        m_scriptDebugServer.activateBreakpoints();
    else
        m_scriptDebugServer.deactivateBreakpoints();
}

bool InspectorDebuggerAgent::isPaused() const
{
    return m_scriptDebugServer.isPaused();
}

void InspectorDebuggerAgent::setSuppressAllPauses(bool suppress)
{
    m_scriptDebugServer.setSuppressAllPauses(suppress);
}

void InspectorDebuggerAgent::updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason reason, RefPtr<JSON::Object>&& data)
{
    if (m_pauseReason != DebuggerFrontendDispatcher::Reason::BlackboxedScript) {
        m_preBlackboxPauseReason = m_pauseReason;
        m_preBlackboxPauseData = WTFMove(m_pauseData);
    }

    m_pauseReason = reason;
    m_pauseData = WTFMove(data);
}

static RefPtr<JSON::Object> buildAssertPauseReason(const String& message)
{
    auto reason = Protocol::Debugger::AssertPauseReason::create().release();
    if (!message.isNull())
        reason->setMessage(message);
    return reason->openAccessors();
}

static RefPtr<JSON::Object> buildCSPViolationPauseReason(const String& directiveText)
{
    auto reason = Protocol::Debugger::CSPViolationPauseReason::create()
        .setDirective(directiveText)
        .release();
    return reason->openAccessors();
}

RefPtr<JSON::Object> InspectorDebuggerAgent::buildBreakpointPauseReason(JSC::BreakpointID debuggerBreakpointIdentifier)
{
    ASSERT(debuggerBreakpointIdentifier != JSC::noBreakpointID);
    auto it = m_debuggerBreakpointIdentifierToInspectorBreakpointIdentifier.find(debuggerBreakpointIdentifier);
    if (it == m_debuggerBreakpointIdentifierToInspectorBreakpointIdentifier.end())
        return nullptr;

    auto reason = Protocol::Debugger::BreakpointPauseReason::create()
        .setBreakpointId(it->value)
        .release();
    return reason->openAccessors();
}

RefPtr<JSON::Object> InspectorDebuggerAgent::buildExceptionPauseReason(JSC::JSValue exception, const InjectedScript& injectedScript)
{
    ASSERT(exception);
    if (!exception)
        return nullptr;

    ASSERT(!injectedScript.hasNoValue());
    if (injectedScript.hasNoValue())
        return nullptr;

    return injectedScript.wrapObject(exception, InspectorDebuggerAgent::backtraceObjectGroup)->openAccessors();
}

void InspectorDebuggerAgent::handleConsoleAssert(const String& message)
{
    if (!m_scriptDebugServer.breakpointsActive())
        return;

    if (m_pauseOnAssertionFailures)
        breakProgram(DebuggerFrontendDispatcher::Reason::Assert, buildAssertPauseReason(message));
}

InspectorDebuggerAgent::AsyncCallIdentifier InspectorDebuggerAgent::asyncCallIdentifier(AsyncCallType asyncCallType, int callbackId)
{
    return std::make_pair(static_cast<unsigned>(asyncCallType), callbackId);
}

void InspectorDebuggerAgent::didScheduleAsyncCall(JSC::JSGlobalObject* globalObject, AsyncCallType asyncCallType, int callbackId, bool singleShot)
{
    if (!m_asyncStackTraceDepth)
        return;

    if (!m_scriptDebugServer.breakpointsActive())
        return;

    Ref<ScriptCallStack> callStack = createScriptCallStack(globalObject, m_asyncStackTraceDepth);
    ASSERT(callStack->size());
    if (!callStack->size())
        return;

    RefPtr<AsyncStackTrace> parentStackTrace;
    if (m_currentAsyncCallIdentifier) {
        auto it = m_pendingAsyncCalls.find(m_currentAsyncCallIdentifier.value());
        ASSERT(it != m_pendingAsyncCalls.end());
        parentStackTrace = it->value;
    }

    auto identifier = asyncCallIdentifier(asyncCallType, callbackId);
    auto asyncStackTrace = AsyncStackTrace::create(WTFMove(callStack), singleShot, WTFMove(parentStackTrace));

    m_pendingAsyncCalls.set(identifier, WTFMove(asyncStackTrace));
}

void InspectorDebuggerAgent::didCancelAsyncCall(AsyncCallType asyncCallType, int callbackId)
{
    if (!m_asyncStackTraceDepth)
        return;

    auto identifier = asyncCallIdentifier(asyncCallType, callbackId);
    auto it = m_pendingAsyncCalls.find(identifier);
    if (it == m_pendingAsyncCalls.end())
        return;

    auto& asyncStackTrace = it->value;
    asyncStackTrace->didCancelAsyncCall();

    if (m_currentAsyncCallIdentifier && m_currentAsyncCallIdentifier.value() == identifier)
        return;

    m_pendingAsyncCalls.remove(identifier);
}

void InspectorDebuggerAgent::willDispatchAsyncCall(AsyncCallType asyncCallType, int callbackId)
{
    if (!m_asyncStackTraceDepth)
        return;

    if (m_currentAsyncCallIdentifier)
        return;

    // A call can be scheduled before the Inspector is opened, or while async stack
    // traces are disabled. If no call data exists, do nothing.
    auto identifier = asyncCallIdentifier(asyncCallType, callbackId);
    auto it = m_pendingAsyncCalls.find(identifier);
    if (it == m_pendingAsyncCalls.end())
        return;

    auto& asyncStackTrace = it->value;
    asyncStackTrace->willDispatchAsyncCall(m_asyncStackTraceDepth);

    m_currentAsyncCallIdentifier = identifier;
}

void InspectorDebuggerAgent::didDispatchAsyncCall()
{
    if (!m_asyncStackTraceDepth)
        return;

    if (!m_currentAsyncCallIdentifier)
        return;

    auto identifier = m_currentAsyncCallIdentifier.value();
    auto it = m_pendingAsyncCalls.find(identifier);
    ASSERT(it != m_pendingAsyncCalls.end());

    auto& asyncStackTrace = it->value;
    asyncStackTrace->didDispatchAsyncCall();

    m_currentAsyncCallIdentifier = WTF::nullopt;

    if (!asyncStackTrace->isPending())
        m_pendingAsyncCalls.remove(identifier);
}

static Ref<JSON::Object> buildObjectForBreakpointCookie(const String& url, int lineNumber, int columnNumber, const String& condition, RefPtr<JSON::Array>& actions, bool isRegex, bool autoContinue, unsigned ignoreCount)
{
    Ref<JSON::Object> breakpointObject = JSON::Object::create();
    breakpointObject->setString("url"_s, url);
    breakpointObject->setInteger("lineNumber"_s, lineNumber);
    breakpointObject->setInteger("columnNumber"_s, columnNumber);
    breakpointObject->setString("condition"_s, condition);
    breakpointObject->setBoolean("isRegex"_s, isRegex);
    breakpointObject->setBoolean("autoContinue"_s, autoContinue);
    breakpointObject->setInteger("ignoreCount"_s, ignoreCount);

    if (actions)
        breakpointObject->setArray("actions"_s, actions);

    return breakpointObject;
}

static bool matches(const String& url, const String& pattern, bool isRegex)
{
    if (isRegex) {
        JSC::Yarr::RegularExpression regex(pattern);
        return regex.match(url) != -1;
    }
    return url == pattern;
}

static bool breakpointActionTypeForString(const String& typeString, ScriptBreakpointActionType* output)
{
    if (typeString == Protocol::InspectorHelpers::getEnumConstantValue(Protocol::Debugger::BreakpointAction::Type::Log)) {
        *output = ScriptBreakpointActionTypeLog;
        return true;
    }
    if (typeString == Protocol::InspectorHelpers::getEnumConstantValue(Protocol::Debugger::BreakpointAction::Type::Evaluate)) {
        *output = ScriptBreakpointActionTypeEvaluate;
        return true;
    }
    if (typeString == Protocol::InspectorHelpers::getEnumConstantValue(Protocol::Debugger::BreakpointAction::Type::Sound)) {
        *output = ScriptBreakpointActionTypeSound;
        return true;
    }
    if (typeString == Protocol::InspectorHelpers::getEnumConstantValue(Protocol::Debugger::BreakpointAction::Type::Probe)) {
        *output = ScriptBreakpointActionTypeProbe;
        return true;
    }

    return false;
}

bool InspectorDebuggerAgent::breakpointActionsFromProtocol(ErrorString& errorString, RefPtr<JSON::Array>& actions, BreakpointActions* result)
{
    if (!actions)
        return true;

    unsigned actionsLength = actions->length();
    if (!actionsLength)
        return true;

    result->reserveCapacity(actionsLength);
    for (unsigned i = 0; i < actionsLength; ++i) {
        RefPtr<JSON::Value> value = actions->get(i);
        RefPtr<JSON::Object> object;
        if (!value->asObject(object)) {
            errorString = "Unexpected non-object item in given actions"_s;
            return false;
        }

        String typeString;
        if (!object->getString("type"_s, typeString)) {
            errorString = "Missing type for item in given actions"_s;
            return false;
        }

        ScriptBreakpointActionType type;
        if (!breakpointActionTypeForString(typeString, &type)) {
            errorString = "Non-string type for item in given actions"_s;
            return false;
        }

        // Specifying an identifier is optional. They are used to correlate probe samples
        // in the frontend across multiple backend probe actions and segregate object groups.
        int identifier = 0;
        object->getInteger("id"_s, identifier);

        String data;
        object->getString("data"_s, data);

        result->append(ScriptBreakpointAction(type, identifier, data));
    }

    return true;
}

static RefPtr<Protocol::Debugger::Location> buildDebuggerLocation(const JSC::Breakpoint& breakpoint)
{
    ASSERT(breakpoint.resolved);

    auto location = Protocol::Debugger::Location::create()
        .setScriptId(String::number(breakpoint.sourceID))
        .setLineNumber(breakpoint.line)
        .release();
    location->setColumnNumber(breakpoint.column);

    return location;
}

static bool parseLocation(ErrorString& errorString, const JSON::Object& location, JSC::SourceID& sourceID, unsigned& lineNumber, unsigned& columnNumber)
{
    if (!location.getInteger("lineNumber"_s, lineNumber)) {
        errorString = "Unexpected non-integer lineNumber in given location"_s;
        sourceID = JSC::noSourceID;
        return false;
    }

    String scriptIDStr;
    if (!location.getString("scriptId"_s, scriptIDStr)) {
        sourceID = JSC::noSourceID;
        errorString = "Unexepcted non-string scriptId in given location"_s;
        return false;
    }

    sourceID = scriptIDStr.toIntPtr();
    columnNumber = 0;
    location.getInteger("columnNumber"_s, columnNumber);
    return true;
}

void InspectorDebuggerAgent::setBreakpointByUrl(ErrorString& errorString, int lineNumber, const String* optionalURL, const String* optionalURLRegex, const int* optionalColumnNumber, const JSON::Object* options, Protocol::Debugger::BreakpointId* outBreakpointIdentifier, RefPtr<JSON::ArrayOf<Protocol::Debugger::Location>>& locations)
{
    locations = JSON::ArrayOf<Protocol::Debugger::Location>::create();
    if (!optionalURL == !optionalURLRegex) {
        errorString = "Either url or urlRegex must be specified"_s;
        return;
    }

    String url = optionalURL ? *optionalURL : *optionalURLRegex;
    int columnNumber = optionalColumnNumber ? *optionalColumnNumber : 0;
    bool isRegex = optionalURLRegex;

    String breakpointIdentifier = makeString(isRegex ? "/" : "", url, isRegex ? "/:" : ":", lineNumber, ':', columnNumber);
    if (m_javaScriptBreakpoints.contains(breakpointIdentifier)) {
        errorString = "Breakpoint for given location already exists."_s;
        return;
    }

    String condition = emptyString();
    bool autoContinue = false;
    unsigned ignoreCount = 0;
    RefPtr<JSON::Array> actions;
    if (options) {
        options->getString("condition"_s, condition);
        options->getBoolean("autoContinue"_s, autoContinue);
        options->getArray("actions"_s, actions);
        options->getInteger("ignoreCount"_s, ignoreCount);
    }

    BreakpointActions breakpointActions;
    if (!breakpointActionsFromProtocol(errorString, actions, &breakpointActions))
        return;

    m_javaScriptBreakpoints.set(breakpointIdentifier, buildObjectForBreakpointCookie(url, lineNumber, columnNumber, condition, actions, isRegex, autoContinue, ignoreCount));

    for (auto& entry : m_scripts) {
        Script& script = entry.value;
        String scriptURLForBreakpoints = !script.sourceURL.isEmpty() ? script.sourceURL : script.url;
        if (!matches(scriptURLForBreakpoints, url, isRegex))
            continue;

        JSC::SourceID sourceID = entry.key;
        JSC::Breakpoint breakpoint(sourceID, lineNumber, columnNumber, condition, autoContinue, ignoreCount);
        resolveBreakpoint(script, breakpoint);
        if (!breakpoint.resolved)
            continue;

        bool existing;
        setBreakpoint(breakpoint, existing);
        if (existing)
            continue;

        ScriptBreakpoint scriptBreakpoint(breakpoint.line, breakpoint.column, condition, breakpointActions, autoContinue, ignoreCount);
        didSetBreakpoint(breakpoint, breakpointIdentifier, scriptBreakpoint);

        locations->addItem(buildDebuggerLocation(breakpoint));
    }

    *outBreakpointIdentifier = breakpointIdentifier;
}

void InspectorDebuggerAgent::setBreakpoint(ErrorString& errorString, const JSON::Object& location, const JSON::Object* options, Protocol::Debugger::BreakpointId* outBreakpointIdentifier, RefPtr<Protocol::Debugger::Location>& actualLocation)
{
    JSC::SourceID sourceID;
    unsigned lineNumber;
    unsigned columnNumber;
    if (!parseLocation(errorString, location, sourceID, lineNumber, columnNumber))
        return;

    String condition = emptyString();
    bool autoContinue = false;
    unsigned ignoreCount = 0;
    RefPtr<JSON::Array> actions;
    if (options) {
        options->getString("condition"_s, condition);
        options->getBoolean("autoContinue"_s, autoContinue);
        options->getArray("actions"_s, actions);
        options->getInteger("ignoreCount"_s, ignoreCount);
    }

    BreakpointActions breakpointActions;
    if (!breakpointActionsFromProtocol(errorString, actions, &breakpointActions))
        return;

    auto scriptIterator = m_scripts.find(sourceID);
    if (scriptIterator == m_scripts.end()) {
        errorString = "Missing script for scriptId in given location"_s;
        return;
    }

    Script& script = scriptIterator->value;
    JSC::Breakpoint breakpoint(sourceID, lineNumber, columnNumber, condition, autoContinue, ignoreCount);
    resolveBreakpoint(script, breakpoint);
    if (!breakpoint.resolved) {
        errorString = "Could not resolve breakpoint"_s;
        return;
    }

    bool existing;
    setBreakpoint(breakpoint, existing);
    if (existing) {
        errorString = "Breakpoint for given location already exists"_s;
        return;
    }

    String breakpointIdentifier = makeString(sourceID, ':', breakpoint.line, ':', breakpoint.column);
    ScriptBreakpoint scriptBreakpoint(breakpoint.line, breakpoint.column, condition, breakpointActions, autoContinue, ignoreCount);
    didSetBreakpoint(breakpoint, breakpointIdentifier, scriptBreakpoint);

    actualLocation = buildDebuggerLocation(breakpoint);
    *outBreakpointIdentifier = breakpointIdentifier;
}

void InspectorDebuggerAgent::didSetBreakpoint(const JSC::Breakpoint& breakpoint, const String& breakpointIdentifier, const ScriptBreakpoint& scriptBreakpoint)
{
    JSC::BreakpointID id = breakpoint.id;
    m_scriptDebugServer.setBreakpointActions(id, scriptBreakpoint);

    auto debugServerBreakpointIDsIterator = m_breakpointIdentifierToDebugServerBreakpointIDs.find(breakpointIdentifier);
    if (debugServerBreakpointIDsIterator == m_breakpointIdentifierToDebugServerBreakpointIDs.end())
        debugServerBreakpointIDsIterator = m_breakpointIdentifierToDebugServerBreakpointIDs.set(breakpointIdentifier, Vector<JSC::BreakpointID>()).iterator;
    debugServerBreakpointIDsIterator->value.append(id);

    m_debuggerBreakpointIdentifierToInspectorBreakpointIdentifier.set(id, breakpointIdentifier);
}

void InspectorDebuggerAgent::resolveBreakpoint(const Script& script, JSC::Breakpoint& breakpoint)
{
    if (breakpoint.line < static_cast<unsigned>(script.startLine) || static_cast<unsigned>(script.endLine) < breakpoint.line)
        return;

    m_scriptDebugServer.resolveBreakpoint(breakpoint, script.sourceProvider.get());
}

void InspectorDebuggerAgent::setBreakpoint(JSC::Breakpoint& breakpoint, bool& existing)
{
    JSC::JSLockHolder locker(m_scriptDebugServer.vm());
    m_scriptDebugServer.setBreakpoint(breakpoint, existing);
}

void InspectorDebuggerAgent::removeBreakpoint(ErrorString&, const String& breakpointIdentifier)
{
    m_javaScriptBreakpoints.remove(breakpointIdentifier);

    for (JSC::BreakpointID breakpointID : m_breakpointIdentifierToDebugServerBreakpointIDs.take(breakpointIdentifier)) {
        m_debuggerBreakpointIdentifierToInspectorBreakpointIdentifier.remove(breakpointID);

        const BreakpointActions& breakpointActions = m_scriptDebugServer.getActionsForBreakpoint(breakpointID);
        for (auto& action : breakpointActions)
            m_injectedScriptManager.releaseObjectGroup(objectGroupForBreakpointAction(action));

        JSC::JSLockHolder locker(m_scriptDebugServer.vm());
        m_scriptDebugServer.removeBreakpointActions(breakpointID);
        m_scriptDebugServer.removeBreakpoint(breakpointID);
    }
}

void InspectorDebuggerAgent::continueUntilNextRunLoop(ErrorString& errorString)
{
    if (!assertPaused(errorString))
        return;

    resume(errorString);

    m_enablePauseWhenIdle = true;

    registerIdleHandler();
}

void InspectorDebuggerAgent::continueToLocation(ErrorString& errorString, const JSON::Object& location)
{
    if (!assertPaused(errorString))
        return;

    if (m_continueToLocationBreakpointID != JSC::noBreakpointID) {
        m_scriptDebugServer.removeBreakpoint(m_continueToLocationBreakpointID);
        m_continueToLocationBreakpointID = JSC::noBreakpointID;
    }

    JSC::SourceID sourceID;
    unsigned lineNumber;
    unsigned columnNumber;
    if (!parseLocation(errorString, location, sourceID, lineNumber, columnNumber))
        return;

    auto scriptIterator = m_scripts.find(sourceID);
    if (scriptIterator == m_scripts.end()) {
        m_scriptDebugServer.continueProgram();
        m_frontendDispatcher->resumed();
        errorString = "Missing script for scriptId in given location"_s;
        return;
    }

    String condition;
    bool autoContinue = false;
    unsigned ignoreCount = 0;
    JSC::Breakpoint breakpoint(sourceID, lineNumber, columnNumber, condition, autoContinue, ignoreCount);
    Script& script = scriptIterator->value;
    resolveBreakpoint(script, breakpoint);
    if (!breakpoint.resolved) {
        m_scriptDebugServer.continueProgram();
        m_frontendDispatcher->resumed();
        errorString = "Could not resolve breakpoint"_s;
        return;
    }

    bool existing;
    setBreakpoint(breakpoint, existing);
    if (existing) {
        // There is an existing breakpoint at this location. Instead of
        // acting like a series of steps, just resume and we will either
        // hit this new breakpoint or not.
        m_scriptDebugServer.continueProgram();
        m_frontendDispatcher->resumed();
        return;
    }

    m_continueToLocationBreakpointID = breakpoint.id;

    // Treat this as a series of steps until reaching the new breakpoint.
    // So don't issue a resumed event unless we exit the VM without pausing.
    willStepAndMayBecomeIdle();
    m_scriptDebugServer.continueProgram();
}

void InspectorDebuggerAgent::searchInContent(ErrorString& errorString, const String& scriptIDStr, const String& query, const bool* optionalCaseSensitive, const bool* optionalIsRegex, RefPtr<JSON::ArrayOf<Protocol::GenericTypes::SearchMatch>>& results)
{
    JSC::SourceID sourceID = scriptIDStr.toIntPtr();
    auto it = m_scripts.find(sourceID);
    if (it == m_scripts.end()) {
        errorString = "Missing script for given scriptId";
        return;
    }

    bool isRegex = optionalIsRegex ? *optionalIsRegex : false;
    bool caseSensitive = optionalCaseSensitive ? *optionalCaseSensitive : false;
    results = ContentSearchUtilities::searchInTextByLines(it->value.source, query, caseSensitive, isRegex);
}

void InspectorDebuggerAgent::getScriptSource(ErrorString& errorString, const String& scriptIDStr, String* scriptSource)
{
    JSC::SourceID sourceID = scriptIDStr.toIntPtr();
    auto it = m_scripts.find(sourceID);
    if (it != m_scripts.end())
        *scriptSource = it->value.source;
    else
        errorString = "Missing script for given scriptId";
}

void InspectorDebuggerAgent::getFunctionDetails(ErrorString& errorString, const String& functionId, RefPtr<Protocol::Debugger::FunctionDetails>& details)
{
    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptForObjectId(functionId);
    if (injectedScript.hasNoValue()) {
        errorString = "Missing injected script for given functionId"_s;
        return;
    }

    injectedScript.getFunctionDetails(errorString, functionId, details);
}

void InspectorDebuggerAgent::schedulePauseOnNextStatement(DebuggerFrontendDispatcher::Reason reason, RefPtr<JSON::Object>&& data)
{
    if (m_javaScriptPauseScheduled)
        return;

    m_javaScriptPauseScheduled = true;

    updatePauseReasonAndData(reason, WTFMove(data));

    JSC::JSLockHolder locker(m_scriptDebugServer.vm());
    m_scriptDebugServer.setPauseOnNextStatement(true);
}

void InspectorDebuggerAgent::cancelPauseOnNextStatement()
{
    if (!m_javaScriptPauseScheduled)
        return;

    m_javaScriptPauseScheduled = false;

    clearPauseDetails();
    m_scriptDebugServer.setPauseOnNextStatement(false);
    m_enablePauseWhenIdle = false;
}

void InspectorDebuggerAgent::pause(ErrorString&)
{
    schedulePauseOnNextStatement(DebuggerFrontendDispatcher::Reason::PauseOnNextStatement, nullptr);
}

void InspectorDebuggerAgent::resume(ErrorString& errorString)
{
    if (!m_pausedGlobalObject && !m_javaScriptPauseScheduled) {
        errorString = "Must be paused or waiting to pause"_s;
        return;
    }

    cancelPauseOnNextStatement();
    m_scriptDebugServer.continueProgram();
    m_conditionToDispatchResumed = ShouldDispatchResumed::WhenContinued;
}

void InspectorDebuggerAgent::stepNext(ErrorString& errorString)
{
    if (!assertPaused(errorString))
        return;

    willStepAndMayBecomeIdle();
    m_scriptDebugServer.stepNextExpression();
}

void InspectorDebuggerAgent::stepOver(ErrorString& errorString)
{
    if (!assertPaused(errorString))
        return;

    willStepAndMayBecomeIdle();
    m_scriptDebugServer.stepOverStatement();
}

void InspectorDebuggerAgent::stepInto(ErrorString& errorString)
{
    if (!assertPaused(errorString))
        return;

    willStepAndMayBecomeIdle();
    m_scriptDebugServer.stepIntoStatement();
}

void InspectorDebuggerAgent::stepOut(ErrorString& errorString)
{
    if (!assertPaused(errorString))
        return;

    willStepAndMayBecomeIdle();
    m_scriptDebugServer.stepOutOfFunction();
}

void InspectorDebuggerAgent::registerIdleHandler()
{
    if (!m_registeredIdleCallback) {
        m_registeredIdleCallback = true;
        JSC::VM& vm = m_scriptDebugServer.vm();
        vm.whenIdle([this]() {
            didBecomeIdle();
        });
    }
}

void InspectorDebuggerAgent::willStepAndMayBecomeIdle()
{
    // When stepping the backend must eventually trigger a "paused" or "resumed" event.
    // If the step causes us to exit the VM, then we should issue "resumed".
    m_conditionToDispatchResumed = ShouldDispatchResumed::WhenIdle;

    registerIdleHandler();
}

void InspectorDebuggerAgent::didBecomeIdle()
{
    m_registeredIdleCallback = false;

    if (m_conditionToDispatchResumed == ShouldDispatchResumed::WhenIdle) {
        cancelPauseOnNextStatement();
        m_scriptDebugServer.continueProgram();
        m_frontendDispatcher->resumed();
    }

    m_conditionToDispatchResumed = ShouldDispatchResumed::No;

    if (m_enablePauseWhenIdle) {
        ErrorString ignored;
        pause(ignored);
    }
}

void InspectorDebuggerAgent::setPauseOnDebuggerStatements(ErrorString&, bool enabled)
{
    m_scriptDebugServer.setPauseOnDebuggerStatements(enabled);
}

void InspectorDebuggerAgent::setPauseOnExceptions(ErrorString& errorString, const String& stringPauseState)
{
    JSC::Debugger::PauseOnExceptionsState pauseState;
    if (stringPauseState == "none")
        pauseState = JSC::Debugger::DontPauseOnExceptions;
    else if (stringPauseState == "all")
        pauseState = JSC::Debugger::PauseOnAllExceptions;
    else if (stringPauseState == "uncaught")
        pauseState = JSC::Debugger::PauseOnUncaughtExceptions;
    else {
        errorString = makeString("Unknown state: "_s, stringPauseState);
        return;
    }

    m_scriptDebugServer.setPauseOnExceptionsState(static_cast<JSC::Debugger::PauseOnExceptionsState>(pauseState));
    if (m_scriptDebugServer.pauseOnExceptionsState() != pauseState)
        errorString = "Internal error. Could not change pause on exceptions state"_s;
}

void InspectorDebuggerAgent::setPauseOnAssertions(ErrorString&, bool enabled)
{
    m_pauseOnAssertionFailures = enabled;
}

void InspectorDebuggerAgent::setPauseOnMicrotasks(ErrorString&, bool enabled)
{
    m_pauseOnMicrotasks = enabled;
}

void InspectorDebuggerAgent::evaluateOnCallFrame(ErrorString& errorString, const String& callFrameId, const String& expression, const String* objectGroup, const bool* includeCommandLineAPI, const bool* doNotPauseOnExceptionsAndMuteConsole, const bool* returnByValue, const bool* generatePreview, const bool* saveResult, const bool* /* emulateUserGesture */, RefPtr<Protocol::Runtime::RemoteObject>& result, Optional<bool>& wasThrown, Optional<int>& savedResultIndex)
{
    if (!assertPaused(errorString))
        return;

    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptForObjectId(callFrameId);
    if (injectedScript.hasNoValue()) {
        errorString = "Missing injected script for given callFrameId"_s;
        return;
    }

    auto pauseState = m_scriptDebugServer.pauseOnExceptionsState();
    bool pauseAndMute = doNotPauseOnExceptionsAndMuteConsole && *doNotPauseOnExceptionsAndMuteConsole;
    if (pauseAndMute) {
        if (pauseState != JSC::Debugger::DontPauseOnExceptions)
            m_scriptDebugServer.setPauseOnExceptionsState(JSC::Debugger::DontPauseOnExceptions);
        muteConsole();
    }

    injectedScript.evaluateOnCallFrame(errorString, m_currentCallStack.get(), callFrameId, expression,
        objectGroup ? *objectGroup : emptyString(), includeCommandLineAPI && *includeCommandLineAPI, returnByValue && *returnByValue, generatePreview && *generatePreview, saveResult && *saveResult,
        result, wasThrown, savedResultIndex);

    if (pauseAndMute) {
        unmuteConsole();
        m_scriptDebugServer.setPauseOnExceptionsState(pauseState);
    }
}

void InspectorDebuggerAgent::setShouldBlackboxURL(ErrorString& errorString, const String& url, bool shouldBlackbox, const bool* optionalCaseSensitive, const bool* optionalIsRegex)
{
    if (url.isEmpty()) {
        errorString = "URL must not be empty"_s;
        return;
    }

    bool caseSensitive = optionalCaseSensitive && *optionalCaseSensitive;
    bool isRegex = optionalIsRegex && *optionalIsRegex;

    if (!caseSensitive && !isRegex && isWebKitInjectedScript(url)) {
        errorString = "Blackboxing of internal scripts is controlled by 'Debugger.setPauseForInternalScripts'"_s;
        return;
    }

    BlackboxConfig config { url, caseSensitive, isRegex };
    if (shouldBlackbox)
        m_blackboxedURLs.appendIfNotContains(config);
    else
        m_blackboxedURLs.removeAll(config);

    for (auto& [sourceID, script] : m_scripts) {
        if (isWebKitInjectedScript(script.sourceURL))
            continue;

        Optional<JSC::Debugger::BlackboxType> blackboxType;
        if (shouldBlackboxURL(script.sourceURL) || shouldBlackboxURL(script.url))
            blackboxType = JSC::Debugger::BlackboxType::Deferred;
        m_scriptDebugServer.setBlackboxType(sourceID, blackboxType);
    }
}

bool InspectorDebuggerAgent::shouldBlackboxURL(const String& url) const
{
    if (!url.isEmpty()) {
        for (const auto& blackboxConfig : m_blackboxedURLs) {
            auto searchStringType = blackboxConfig.isRegex ? ContentSearchUtilities::SearchStringType::Regex : ContentSearchUtilities::SearchStringType::ExactString;
            auto regex = ContentSearchUtilities::createRegularExpressionForSearchString(blackboxConfig.url, blackboxConfig.caseSensitive, searchStringType);
            if (regex.match(url) != -1)
                return true;
        }
    }
    return false;
}

void InspectorDebuggerAgent::scriptExecutionBlockedByCSP(const String& directiveText)
{
    if (m_scriptDebugServer.pauseOnExceptionsState() != JSC::Debugger::DontPauseOnExceptions)
        breakProgram(DebuggerFrontendDispatcher::Reason::CSPViolation, buildCSPViolationPauseReason(directiveText));
}

Ref<JSON::ArrayOf<Protocol::Debugger::CallFrame>> InspectorDebuggerAgent::currentCallFrames(const InjectedScript& injectedScript)
{
    ASSERT(!injectedScript.hasNoValue());
    if (injectedScript.hasNoValue())
        return JSON::ArrayOf<Protocol::Debugger::CallFrame>::create();

    return injectedScript.wrapCallFrames(m_currentCallStack.get());
}

String InspectorDebuggerAgent::sourceMapURLForScript(const Script& script)
{
    return script.sourceMappingURL;
}

void InspectorDebuggerAgent::setPauseForInternalScripts(ErrorString&, bool shouldPause)
{
    if (shouldPause == m_pauseForInternalScripts)
        return;

    m_pauseForInternalScripts = shouldPause;

    auto blackboxType = !m_pauseForInternalScripts ? Optional<JSC::Debugger::BlackboxType>(JSC::Debugger::BlackboxType::Ignored) : WTF::nullopt;
    for (auto& [sourceID, script] : m_scripts) {
        if (!isWebKitInjectedScript(script.sourceURL))
            continue;
        m_scriptDebugServer.setBlackboxType(sourceID, blackboxType);
    }
}

void InspectorDebuggerAgent::didParseSource(JSC::SourceID sourceID, const Script& script)
{
    String scriptIDStr = String::number(sourceID);
    bool hasSourceURL = !script.sourceURL.isEmpty();
    String sourceURL = script.sourceURL;
    String sourceMappingURL = sourceMapURLForScript(script);

    const bool isModule = script.sourceProvider->sourceType() == JSC::SourceProviderSourceType::Module;
    const bool* isContentScript = script.isContentScript ? &script.isContentScript : nullptr;
    String* sourceURLParam = hasSourceURL ? &sourceURL : nullptr;
    String* sourceMapURLParam = sourceMappingURL.isEmpty() ? nullptr : &sourceMappingURL;

    m_frontendDispatcher->scriptParsed(scriptIDStr, script.url, script.startLine, script.startColumn, script.endLine, script.endColumn, isContentScript, sourceURLParam, sourceMapURLParam, isModule ? &isModule : nullptr);

    m_scripts.set(sourceID, script);

    if (isWebKitInjectedScript(sourceURL)) {
        if (!m_pauseForInternalScripts)
            m_scriptDebugServer.setBlackboxType(sourceID, JSC::Debugger::BlackboxType::Ignored);
    } else if (shouldBlackboxURL(sourceURL) || shouldBlackboxURL(script.url))
        m_scriptDebugServer.setBlackboxType(sourceID, JSC::Debugger::BlackboxType::Deferred);

    String scriptURLForBreakpoints = hasSourceURL ? script.sourceURL : script.url;
    if (scriptURLForBreakpoints.isEmpty())
        return;

    for (auto& entry : m_javaScriptBreakpoints) {
        RefPtr<JSON::Object> breakpointObject = entry.value;

        bool isRegex;
        String url;
        breakpointObject->getBoolean("isRegex"_s, isRegex);
        breakpointObject->getString("url"_s, url);
        if (!matches(scriptURLForBreakpoints, url, isRegex))
            continue;

        ScriptBreakpoint scriptBreakpoint;
        breakpointObject->getInteger("lineNumber"_s, scriptBreakpoint.lineNumber);
        breakpointObject->getInteger("columnNumber"_s, scriptBreakpoint.columnNumber);
        breakpointObject->getString("condition"_s, scriptBreakpoint.condition);
        breakpointObject->getBoolean("autoContinue"_s, scriptBreakpoint.autoContinue);
        breakpointObject->getInteger("ignoreCount"_s, scriptBreakpoint.ignoreCount);
        ErrorString errorString;
        RefPtr<JSON::Array> actions;
        breakpointObject->getArray("actions"_s, actions);
        if (!breakpointActionsFromProtocol(errorString, actions, &scriptBreakpoint.actions)) {
            ASSERT_NOT_REACHED();
            continue;
        }

        JSC::Breakpoint breakpoint(sourceID, scriptBreakpoint.lineNumber, scriptBreakpoint.columnNumber, scriptBreakpoint.condition, scriptBreakpoint.autoContinue, scriptBreakpoint.ignoreCount);
        resolveBreakpoint(script, breakpoint);
        if (!breakpoint.resolved)
            continue;

        bool existing;
        setBreakpoint(breakpoint, existing);
        if (existing)
            continue;

        String breakpointIdentifier = entry.key;
        didSetBreakpoint(breakpoint, breakpointIdentifier, scriptBreakpoint);

        m_frontendDispatcher->breakpointResolved(breakpointIdentifier, buildDebuggerLocation(breakpoint));
    }
}

void InspectorDebuggerAgent::failedToParseSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage)
{
    m_frontendDispatcher->scriptFailedToParse(url, data, firstLine, errorLine, errorMessage);
}

void InspectorDebuggerAgent::willRunMicrotask()
{
    if (!m_scriptDebugServer.breakpointsActive())
        return;

    if (m_pauseOnMicrotasks)
        schedulePauseOnNextStatement(DebuggerFrontendDispatcher::Reason::Microtask, nullptr);
}

void InspectorDebuggerAgent::didRunMicrotask()
{
    if (!m_scriptDebugServer.breakpointsActive())
        return;

    if (m_pauseOnMicrotasks)
        cancelPauseOnNextStatement();
}

void InspectorDebuggerAgent::didPause(JSC::JSGlobalObject* globalObject, JSC::JSValue callFrames, JSC::JSValue exceptionOrCaughtValue)
{
    ASSERT(!m_pausedGlobalObject);
    m_pausedGlobalObject = globalObject;
    m_currentCallStack = { globalObject->vm(), callFrames };

    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptFor(globalObject);

    // If a high level pause pause reason is not already set, try to infer a reason from the debugger.
    if (m_pauseReason == DebuggerFrontendDispatcher::Reason::Other) {
        switch (m_scriptDebugServer.reasonForPause()) {
        case JSC::Debugger::PausedForBreakpoint: {
            auto debuggerBreakpointId = m_scriptDebugServer.pausingBreakpointID();
            if (debuggerBreakpointId != m_continueToLocationBreakpointID)
                updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason::Breakpoint, buildBreakpointPauseReason(debuggerBreakpointId));
            break;
        }
        case JSC::Debugger::PausedForDebuggerStatement:
            updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason::DebuggerStatement, nullptr);
            break;
        case JSC::Debugger::PausedForException:
            updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason::Exception, buildExceptionPauseReason(exceptionOrCaughtValue, injectedScript));
            break;
        case JSC::Debugger::PausedAfterBlackboxedScript: {
            // There should be no break data, as we would've already continued past the breakpoint.
            ASSERT(!m_pauseData);

            // Don't call `updatePauseReasonAndData` so as to not override `m_preBlackboxPauseData`.
            if (m_pauseReason != DebuggerFrontendDispatcher::Reason::BlackboxedScript)
                m_preBlackboxPauseReason = m_pauseReason;
            m_pauseReason = DebuggerFrontendDispatcher::Reason::BlackboxedScript;
            break;
        }
        case JSC::Debugger::PausedAtStatement:
        case JSC::Debugger::PausedAtExpression:
        case JSC::Debugger::PausedBeforeReturn:
        case JSC::Debugger::PausedAtEndOfProgram:
            // Pause was just stepping. Nothing to report.
            break;
        case JSC::Debugger::NotPaused:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    if (m_scriptDebugServer.reasonForPause() == JSC::Debugger::PausedAfterBlackboxedScript) {
        // Ensure that `m_preBlackboxPauseReason` is populated with the most recent data.
        updatePauseReasonAndData(m_pauseReason, nullptr);

        RefPtr<JSON::Object> data;
        if (auto debuggerBreakpointId = m_scriptDebugServer.pausingBreakpointID()) {
            ASSERT(debuggerBreakpointId != m_continueToLocationBreakpointID);
            data = JSON::Object::create();
            data->setString("originalReason"_s, Protocol::InspectorHelpers::getEnumConstantValue(DebuggerFrontendDispatcher::Reason::Breakpoint));
            data->setValue("originalData"_s, buildBreakpointPauseReason(debuggerBreakpointId));
        } else if (m_preBlackboxPauseData) {
            data = JSON::Object::create();
            data->setString("originalReason"_s, Protocol::InspectorHelpers::getEnumConstantValue(m_preBlackboxPauseReason));
            data->setValue("originalData"_s, m_preBlackboxPauseData);
        }
        updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason::BlackboxedScript, WTFMove(data));
    }

    // Set $exception to the exception or caught value.
    if (exceptionOrCaughtValue && !injectedScript.hasNoValue()) {
        injectedScript.setExceptionValue(exceptionOrCaughtValue);
        m_hasExceptionValue = true;
    }

    m_conditionToDispatchResumed = ShouldDispatchResumed::No;
    m_enablePauseWhenIdle = false;

    RefPtr<Protocol::Console::StackTrace> asyncStackTrace;
    if (m_currentAsyncCallIdentifier) {
        auto it = m_pendingAsyncCalls.find(m_currentAsyncCallIdentifier.value());
        if (it != m_pendingAsyncCalls.end())
            asyncStackTrace = it->value->buildInspectorObject();
    }

    m_frontendDispatcher->paused(currentCallFrames(injectedScript), m_pauseReason, m_pauseData, asyncStackTrace);

    m_javaScriptPauseScheduled = false;

    if (m_continueToLocationBreakpointID != JSC::noBreakpointID) {
        m_scriptDebugServer.removeBreakpoint(m_continueToLocationBreakpointID);
        m_continueToLocationBreakpointID = JSC::noBreakpointID;
    }

    auto& stopwatch = m_injectedScriptManager.inspectorEnvironment().executionStopwatch();
    if (stopwatch.isActive()) {
        stopwatch.stop();
        m_didPauseStopwatch = true;
    }
}

void InspectorDebuggerAgent::breakpointActionSound(int breakpointActionIdentifier)
{
    m_frontendDispatcher->playBreakpointActionSound(breakpointActionIdentifier);
}

void InspectorDebuggerAgent::breakpointActionProbe(JSC::JSGlobalObject* globalObject, const ScriptBreakpointAction& action, unsigned batchId, unsigned sampleId, JSC::JSValue sample)
{
    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptFor(globalObject);
    auto payload = injectedScript.wrapObject(sample, objectGroupForBreakpointAction(action), true);
    auto result = Protocol::Debugger::ProbeSample::create()
        .setProbeId(action.identifier)
        .setBatchId(batchId)
        .setSampleId(sampleId)
        .setTimestamp(m_injectedScriptManager.inspectorEnvironment().executionStopwatch().elapsedTime().seconds())
        .setPayload(WTFMove(payload))
        .release();
    m_frontendDispatcher->didSampleProbe(WTFMove(result));
}

void InspectorDebuggerAgent::didContinue()
{
    if (m_didPauseStopwatch) {
        m_didPauseStopwatch = false;
        m_injectedScriptManager.inspectorEnvironment().executionStopwatch().start();
    }

    m_pausedGlobalObject = nullptr;
    m_currentCallStack = { };
    m_injectedScriptManager.releaseObjectGroup(InspectorDebuggerAgent::backtraceObjectGroup);
    clearPauseDetails();
    clearExceptionValue();

    if (m_conditionToDispatchResumed == ShouldDispatchResumed::WhenContinued)
        m_frontendDispatcher->resumed();
}

void InspectorDebuggerAgent::breakProgram(DebuggerFrontendDispatcher::Reason reason, RefPtr<JSON::Object>&& data)
{
    updatePauseReasonAndData(reason, WTFMove(data));

    m_scriptDebugServer.breakProgram();
}

void InspectorDebuggerAgent::clearInspectorBreakpointState()
{
    ErrorString ignored;
    for (const String& identifier : copyToVector(m_breakpointIdentifierToDebugServerBreakpointIDs.keys()))
        removeBreakpoint(ignored, identifier);

    m_javaScriptBreakpoints.clear();

    clearDebuggerBreakpointState();
}

void InspectorDebuggerAgent::clearDebuggerBreakpointState()
{
    {
        JSC::JSLockHolder holder(m_scriptDebugServer.vm());
        m_scriptDebugServer.clearBreakpointActions();
        m_scriptDebugServer.clearBreakpoints();
        m_scriptDebugServer.clearBlackbox();
    }

    m_pausedGlobalObject = nullptr;
    m_currentCallStack = { };
    m_scripts.clear();
    m_breakpointIdentifierToDebugServerBreakpointIDs.clear();
    m_debuggerBreakpointIdentifierToInspectorBreakpointIdentifier.clear();
    m_continueToLocationBreakpointID = JSC::noBreakpointID;
    clearPauseDetails();
    m_javaScriptPauseScheduled = false;
    m_hasExceptionValue = false;

    if (isPaused()) {
        m_scriptDebugServer.continueProgram();
        m_frontendDispatcher->resumed();
    }
}

void InspectorDebuggerAgent::didClearGlobalObject()
{
    // Clear breakpoints from the debugger, but keep the inspector's model of which
    // pages have what breakpoints, as the mapping is only sent to DebuggerAgent once.
    clearDebuggerBreakpointState();

    clearAsyncStackTraceData();

    m_frontendDispatcher->globalObjectCleared();
}

bool InspectorDebuggerAgent::assertPaused(ErrorString& errorString)
{
    if (!m_pausedGlobalObject) {
        errorString = "Must be paused"_s;
        return false;
    }

    return true;
}

void InspectorDebuggerAgent::clearPauseDetails()
{
    updatePauseReasonAndData(DebuggerFrontendDispatcher::Reason::Other, nullptr);
}

void InspectorDebuggerAgent::clearExceptionValue()
{
    if (m_hasExceptionValue) {
        m_injectedScriptManager.clearExceptionValue();
        m_hasExceptionValue = false;
    }
}

void InspectorDebuggerAgent::clearAsyncStackTraceData()
{
    m_pendingAsyncCalls.clear();
    m_currentAsyncCallIdentifier = WTF::nullopt;

    didClearAsyncStackTraceData();
}

} // namespace Inspector
