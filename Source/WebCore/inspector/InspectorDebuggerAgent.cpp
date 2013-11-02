/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010-2011 Google Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
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

#if ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
#include "InspectorDebuggerAgent.h"

#include "CachedResource.h"
#include "ContentSearchUtils.h"
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "RegularExpression.h"
#include "ScriptDebugServer.h"
#include "ScriptObject.h"
#include <wtf/text/WTFString.h>

using WebCore::TypeBuilder::Array;
using WebCore::TypeBuilder::Debugger::FunctionDetails;
using WebCore::TypeBuilder::Debugger::ScriptId;
using WebCore::TypeBuilder::Runtime::RemoteObject;

namespace WebCore {

const char* InspectorDebuggerAgent::backtraceObjectGroup = "backtrace";

InspectorDebuggerAgent::InspectorDebuggerAgent(InstrumentingAgents* instrumentingAgents, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorDebuggerAgent>("Debugger", instrumentingAgents)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_pausedScriptState(0)
    , m_enabled(false)
    , m_javaScriptPauseScheduled(false)
    , m_listener(0)
{
    // FIXME: make breakReason optional so that there was no need to init it with "other".
    clearBreakDetails();
}

InspectorDebuggerAgent::~InspectorDebuggerAgent()
{
    ASSERT(!m_instrumentingAgents->inspectorDebuggerAgent());
}

void InspectorDebuggerAgent::enable()
{
    m_instrumentingAgents->setInspectorDebuggerAgent(this);

    // FIXME(WK44513): breakpoints activated flag should be synchronized between all front-ends
    scriptDebugServer().setBreakpointsActivated(true);
    startListeningScriptDebugServer();

    if (m_listener)
        m_listener->debuggerWasEnabled();

    m_enabled = true;
}

void InspectorDebuggerAgent::disable()
{
    m_javaScriptBreakpoints.clear();
    m_instrumentingAgents->setInspectorDebuggerAgent(0);

    stopListeningScriptDebugServer();
    scriptDebugServer().clearBreakpoints();
    scriptDebugServer().clearCompiledScripts();
    scriptDebugServer().continueProgram();
    clear();

    if (m_listener)
        m_listener->debuggerWasDisabled();

    m_enabled = false;
}

void InspectorDebuggerAgent::causesRecompilation(ErrorString*, bool* result)
{
    *result = scriptDebugServer().causesRecompilation();
}

void InspectorDebuggerAgent::canSetScriptSource(ErrorString*, bool* result)
{
    *result = scriptDebugServer().canSetScriptSource();
}

void InspectorDebuggerAgent::supportsSeparateScriptCompilationAndExecution(ErrorString*, bool* result)
{
    *result = scriptDebugServer().supportsSeparateScriptCompilationAndExecution();
}

void InspectorDebuggerAgent::enable(ErrorString*)
{
    if (enabled())
        return;

    enable();

    ASSERT(m_frontend);
}

void InspectorDebuggerAgent::disable(ErrorString*)
{
    if (!enabled())
        return;

    disable();
}

void InspectorDebuggerAgent::setFrontend(InspectorFrontend* frontend)
{
    m_frontend = frontend->debugger();
}

void InspectorDebuggerAgent::clearFrontend()
{
    m_frontend = 0;

    if (!enabled())
        return;

    disable();
}

void InspectorDebuggerAgent::setBreakpointsActive(ErrorString*, bool active)
{
    if (active)
        scriptDebugServer().activateBreakpoints();
    else
        scriptDebugServer().deactivateBreakpoints();
}

bool InspectorDebuggerAgent::isPaused()
{
    return scriptDebugServer().isPaused();
}

bool InspectorDebuggerAgent::runningNestedMessageLoop()
{
    return scriptDebugServer().runningNestedMessageLoop();
}

void InspectorDebuggerAgent::addMessageToConsole(MessageSource source, MessageType type)
{
    if (scriptDebugServer().pauseOnExceptionsState() != ScriptDebugServer::DontPauseOnExceptions && source == ConsoleAPIMessageSource && type == AssertMessageType)
        breakProgram(InspectorFrontend::Debugger::Reason::Assert, 0);
}

static PassRefPtr<InspectorObject> buildObjectForBreakpointCookie(const String& url, int lineNumber, int columnNumber, const String& condition, RefPtr<InspectorArray>& actions, bool isRegex, bool autoContinue)
{
    RefPtr<InspectorObject> breakpointObject = InspectorObject::create();
    breakpointObject->setString("url", url);
    breakpointObject->setNumber("lineNumber", lineNumber);
    breakpointObject->setNumber("columnNumber", columnNumber);
    breakpointObject->setString("condition", condition);
    breakpointObject->setBoolean("isRegex", isRegex);
    breakpointObject->setBoolean("autoContinue", autoContinue);
    if (actions)
        breakpointObject->setArray("actions", actions);

    return breakpointObject;
}

static bool matches(const String& url, const String& pattern, bool isRegex)
{
    if (isRegex) {
        RegularExpression regex(pattern, TextCaseSensitive);
        return regex.match(url) != -1;
    }
    return url == pattern;
}

static bool breakpointActionTypeForString(const String& typeString, ScriptBreakpointActionType* output)
{
    if (typeString == TypeBuilder::getEnumConstantValue(TypeBuilder::Debugger::BreakpointAction::Type::Log)) {
        *output = ScriptBreakpointActionTypeLog;
        return true;
    }
    if (typeString == TypeBuilder::getEnumConstantValue(TypeBuilder::Debugger::BreakpointAction::Type::Evaluate)) {
        *output = ScriptBreakpointActionTypeEvaluate;
        return true;
    }
    if (typeString == TypeBuilder::getEnumConstantValue(TypeBuilder::Debugger::BreakpointAction::Type::Sound)) {
        *output = ScriptBreakpointActionTypeSound;
        return true;
    }

    return false;
}

static bool breakpointActionsFromProtocol(ErrorString* errorString, RefPtr<InspectorArray>& actions, Vector<ScriptBreakpointAction>* result)
{
    if (!actions)
        return true;

    unsigned actionsLength = actions->length();
    if (!actionsLength)
        return true;

    result->reserveCapacity(actionsLength);
    for (unsigned i = 0; i < actionsLength; ++i) {
        RefPtr<InspectorValue> value = actions->get(i);
        RefPtr<InspectorObject> object;
        if (!value->asObject(&object)) {
            *errorString = "BreakpointAction of incorrect type, expected object";
            return false;
        }

        String typeString;
        if (!object->getString("type", &typeString)) {
            *errorString = "BreakpointAction had type missing";
            return false;
        }

        ScriptBreakpointActionType type;
        if (!breakpointActionTypeForString(typeString, &type)) {
            *errorString = "BreakpointAction had unknown type";
            return false;
        }

        String data;
        object->getString("data", &data);

        result->append(ScriptBreakpointAction(type, data));
    }

    return true;
}

void InspectorDebuggerAgent::setBreakpointByUrl(ErrorString* errorString, int lineNumber, const String* const optionalURL, const String* const optionalURLRegex, const int* const optionalColumnNumber, const RefPtr<InspectorObject>* options, TypeBuilder::Debugger::BreakpointId* outBreakpointId, RefPtr<TypeBuilder::Array<TypeBuilder::Debugger::Location>>& locations)
{
    locations = Array<TypeBuilder::Debugger::Location>::create();
    if (!optionalURL == !optionalURLRegex) {
        *errorString = "Either url or urlRegex must be specified.";
        return;
    }

    String url = optionalURL ? *optionalURL : *optionalURLRegex;
    int columnNumber = optionalColumnNumber ? *optionalColumnNumber : 0;
    bool isRegex = optionalURLRegex;

    String breakpointId = (isRegex ? "/" + url + "/" : url) + ':' + String::number(lineNumber) + ':' + String::number(columnNumber);
    if (m_javaScriptBreakpoints.contains(breakpointId)) {
        *errorString = "Breakpoint at specified location already exists.";
        return;
    }

    String condition = emptyString();
    bool autoContinue = false;
    RefPtr<InspectorArray> actions;
    if (options) {
        (*options)->getString("condition", &condition);
        (*options)->getBoolean("autoContinue", &autoContinue);
        actions = (*options)->getArray("actions");
    }

    Vector<ScriptBreakpointAction> breakpointActions;
    if (!breakpointActionsFromProtocol(errorString, actions, &breakpointActions))
        return;

    m_javaScriptBreakpoints.set(breakpointId, buildObjectForBreakpointCookie(url, lineNumber, columnNumber, condition, actions, isRegex, autoContinue));

    ScriptBreakpoint breakpoint(lineNumber, columnNumber, condition, breakpointActions, autoContinue);
    for (ScriptsMap::iterator it = m_scripts.begin(); it != m_scripts.end(); ++it) {
        String scriptURL = !it->value.sourceURL.isEmpty() ? it->value.sourceURL : it->value.url;
        if (!matches(scriptURL, url, isRegex))
            continue;

        RefPtr<TypeBuilder::Debugger::Location> location = resolveBreakpoint(breakpointId, it->key, breakpoint);
        if (location)
            locations->addItem(location);
    }
    *outBreakpointId = breakpointId;
}

static bool parseLocation(ErrorString* errorString, RefPtr<InspectorObject> location, String* scriptId, int* lineNumber, int* columnNumber)
{
    if (!location->getString("scriptId", scriptId) || !location->getNumber("lineNumber", lineNumber)) {
        // FIXME: replace with input validation.
        *errorString = "scriptId and lineNumber are required.";
        return false;
    }
    *columnNumber = 0;
    location->getNumber("columnNumber", columnNumber);
    return true;
}

void InspectorDebuggerAgent::setBreakpoint(ErrorString* errorString, const RefPtr<InspectorObject>& location, const RefPtr<InspectorObject>* options, TypeBuilder::Debugger::BreakpointId* outBreakpointId, RefPtr<TypeBuilder::Debugger::Location>& actualLocation)
{
    String scriptId;
    int lineNumber;
    int columnNumber;

    if (!parseLocation(errorString, location, &scriptId, &lineNumber, &columnNumber))
        return;

    String condition = emptyString();
    bool autoContinue = false;
    RefPtr<InspectorArray> actions;
    if (options) {
        (*options)->getString("condition", &condition);
        (*options)->getBoolean("autoContinue", &autoContinue);
        actions = (*options)->getArray("actions");
    }

    Vector<ScriptBreakpointAction> breakpointActions;
    if (!breakpointActionsFromProtocol(errorString, actions, &breakpointActions))
        return;

    String breakpointId = scriptId + ':' + String::number(lineNumber) + ':' + String::number(columnNumber);
    if (m_breakpointIdToDebugServerBreakpointIds.find(breakpointId) != m_breakpointIdToDebugServerBreakpointIds.end()) {
        *errorString = "Breakpoint at specified location already exists.";
        return;
    }
    ScriptBreakpoint breakpoint(lineNumber, columnNumber, condition, breakpointActions, autoContinue);
    actualLocation = resolveBreakpoint(breakpointId, scriptId, breakpoint);
    if (actualLocation)
        *outBreakpointId = breakpointId;
    else
        *errorString = "Could not resolve breakpoint";
}

void InspectorDebuggerAgent::removeBreakpoint(ErrorString*, const String& breakpointId)
{
    m_javaScriptBreakpoints.remove(breakpointId);

    BreakpointIdToDebugServerBreakpointIdsMap::iterator debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.find(breakpointId);
    if (debugServerBreakpointIdsIterator == m_breakpointIdToDebugServerBreakpointIds.end())
        return;
    for (size_t i = 0; i < debugServerBreakpointIdsIterator->value.size(); ++i)
        scriptDebugServer().removeBreakpoint(debugServerBreakpointIdsIterator->value[i]);
    m_breakpointIdToDebugServerBreakpointIds.remove(debugServerBreakpointIdsIterator);
}

void InspectorDebuggerAgent::continueToLocation(ErrorString* errorString, const RefPtr<InspectorObject>& location)
{
    if (!m_continueToLocationBreakpointId.isEmpty()) {
        scriptDebugServer().removeBreakpoint(m_continueToLocationBreakpointId);
        m_continueToLocationBreakpointId = "";
    }

    String scriptId;
    int lineNumber;
    int columnNumber;

    if (!parseLocation(errorString, location, &scriptId, &lineNumber, &columnNumber))
        return;

    ScriptBreakpoint breakpoint(lineNumber, columnNumber, "", false);
    m_continueToLocationBreakpointId = scriptDebugServer().setBreakpoint(scriptId, breakpoint, &lineNumber, &columnNumber);
    resume(errorString);
}

PassRefPtr<TypeBuilder::Debugger::Location> InspectorDebuggerAgent::resolveBreakpoint(const String& breakpointId, const String& scriptId, const ScriptBreakpoint& breakpoint)
{
    ScriptsMap::iterator scriptIterator = m_scripts.find(scriptId);
    if (scriptIterator == m_scripts.end())
        return 0;
    Script& script = scriptIterator->value;
    if (breakpoint.lineNumber < script.startLine || script.endLine < breakpoint.lineNumber)
        return 0;

    int actualLineNumber;
    int actualColumnNumber;
    String debugServerBreakpointId = scriptDebugServer().setBreakpoint(scriptId, breakpoint, &actualLineNumber, &actualColumnNumber);
    if (debugServerBreakpointId.isEmpty())
        return 0;

    BreakpointIdToDebugServerBreakpointIdsMap::iterator debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.find(breakpointId);
    if (debugServerBreakpointIdsIterator == m_breakpointIdToDebugServerBreakpointIds.end())
        debugServerBreakpointIdsIterator = m_breakpointIdToDebugServerBreakpointIds.set(breakpointId, Vector<String>()).iterator;
    debugServerBreakpointIdsIterator->value.append(debugServerBreakpointId);

    RefPtr<TypeBuilder::Debugger::Location> location = TypeBuilder::Debugger::Location::create()
        .setScriptId(scriptId)
        .setLineNumber(actualLineNumber);
    location->setColumnNumber(actualColumnNumber);
    return location;
}

static PassRefPtr<InspectorObject> scriptToInspectorObject(ScriptObject scriptObject)
{
    if (scriptObject.hasNoValue())
        return 0;
    RefPtr<InspectorValue> value = scriptObject.toInspectorValue(scriptObject.scriptState());
    if (!value)
        return 0;
    return value->asObject();
}

void InspectorDebuggerAgent::searchInContent(ErrorString* error, const String& scriptId, const String& query, const bool* const optionalCaseSensitive, const bool* const optionalIsRegex, RefPtr<Array<WebCore::TypeBuilder::Page::SearchMatch>>& results)
{
    bool isRegex = optionalIsRegex ? *optionalIsRegex : false;
    bool caseSensitive = optionalCaseSensitive ? *optionalCaseSensitive : false;

    ScriptsMap::iterator it = m_scripts.find(scriptId);
    if (it != m_scripts.end())
        results = ContentSearchUtils::searchInTextByLines(it->value.source, query, caseSensitive, isRegex);
    else
        *error = "No script for id: " + scriptId;
}

void InspectorDebuggerAgent::setScriptSource(ErrorString* error, const String& scriptId, const String& newContent, const bool* const preview, RefPtr<Array<TypeBuilder::Debugger::CallFrame>>& newCallFrames, RefPtr<InspectorObject>& result)
{
    bool previewOnly = preview && *preview;
    ScriptObject resultObject;
    if (!scriptDebugServer().setScriptSource(scriptId, newContent, previewOnly, error, &m_currentCallStack, &resultObject))
        return;
    newCallFrames = currentCallFrames();
    RefPtr<InspectorObject> object = scriptToInspectorObject(resultObject);
    if (object)
        result = object;
}

void InspectorDebuggerAgent::getScriptSource(ErrorString* error, const String& scriptId, String* scriptSource)
{
    ScriptsMap::iterator it = m_scripts.find(scriptId);
    if (it != m_scripts.end())
        *scriptSource = it->value.source;
    else
        *error = "No script for id: " + scriptId;
}

void InspectorDebuggerAgent::getFunctionDetails(ErrorString* errorString, const String& functionId, RefPtr<TypeBuilder::Debugger::FunctionDetails>& details)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(functionId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Function object id is obsolete";
        return;
    }
    injectedScript.getFunctionDetails(errorString, functionId, &details);
}

void InspectorDebuggerAgent::schedulePauseOnNextStatement(InspectorFrontend::Debugger::Reason::Enum breakReason, PassRefPtr<InspectorObject> data)
{
    if (m_javaScriptPauseScheduled)
        return;
    m_breakReason = breakReason;
    m_breakAuxData = data;
    scriptDebugServer().setPauseOnNextStatement(true);
}

void InspectorDebuggerAgent::cancelPauseOnNextStatement()
{
    if (m_javaScriptPauseScheduled)
        return;
    clearBreakDetails();
    scriptDebugServer().setPauseOnNextStatement(false);
}

void InspectorDebuggerAgent::pause(ErrorString*)
{
    if (m_javaScriptPauseScheduled)
        return;
    clearBreakDetails();
    scriptDebugServer().setPauseOnNextStatement(true);
    m_javaScriptPauseScheduled = true;
}

void InspectorDebuggerAgent::resume(ErrorString* errorString)
{
    if (!assertPaused(errorString))
        return;
    m_injectedScriptManager->releaseObjectGroup(InspectorDebuggerAgent::backtraceObjectGroup);
    scriptDebugServer().continueProgram();
}

void InspectorDebuggerAgent::stepOver(ErrorString* errorString)
{
    if (!assertPaused(errorString))
        return;
    m_injectedScriptManager->releaseObjectGroup(InspectorDebuggerAgent::backtraceObjectGroup);
    scriptDebugServer().stepOverStatement();
}

void InspectorDebuggerAgent::stepInto(ErrorString* errorString)
{
    if (!assertPaused(errorString))
        return;
    m_injectedScriptManager->releaseObjectGroup(InspectorDebuggerAgent::backtraceObjectGroup);
    scriptDebugServer().stepIntoStatement();
    m_listener->stepInto();
}

void InspectorDebuggerAgent::stepOut(ErrorString* errorString)
{
    if (!assertPaused(errorString))
        return;
    m_injectedScriptManager->releaseObjectGroup(InspectorDebuggerAgent::backtraceObjectGroup);
    scriptDebugServer().stepOutOfFunction();
}

void InspectorDebuggerAgent::setPauseOnExceptions(ErrorString* errorString, const String& stringPauseState)
{
    ScriptDebugServer::PauseOnExceptionsState pauseState;
    if (stringPauseState == "none")
        pauseState = ScriptDebugServer::DontPauseOnExceptions;
    else if (stringPauseState == "all")
        pauseState = ScriptDebugServer::PauseOnAllExceptions;
    else if (stringPauseState == "uncaught")
        pauseState = ScriptDebugServer::PauseOnUncaughtExceptions;
    else {
        *errorString = "Unknown pause on exceptions mode: " + stringPauseState;
        return;
    }
    setPauseOnExceptionsImpl(errorString, pauseState);
}

void InspectorDebuggerAgent::setPauseOnExceptionsImpl(ErrorString* errorString, int pauseState)
{
    scriptDebugServer().setPauseOnExceptionsState(static_cast<ScriptDebugServer::PauseOnExceptionsState>(pauseState));
    if (scriptDebugServer().pauseOnExceptionsState() != pauseState)
        *errorString = "Internal error. Could not change pause on exceptions state";
}

void InspectorDebuggerAgent::evaluateOnCallFrame(ErrorString* errorString, const String& callFrameId, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, const bool* const doNotPauseOnExceptionsAndMuteConsole, const bool* const returnByValue, const bool* generatePreview, RefPtr<TypeBuilder::Runtime::RemoteObject>& result, TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(callFrameId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }

    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = scriptDebugServer().pauseOnExceptionsState();
    if (doNotPauseOnExceptionsAndMuteConsole ? *doNotPauseOnExceptionsAndMuteConsole : false) {
        if (previousPauseOnExceptionsState != ScriptDebugServer::DontPauseOnExceptions)
            scriptDebugServer().setPauseOnExceptionsState(ScriptDebugServer::DontPauseOnExceptions);
        muteConsole();
    }

    injectedScript.evaluateOnCallFrame(errorString, m_currentCallStack, callFrameId, expression, objectGroup ? *objectGroup : "", includeCommandLineAPI ? *includeCommandLineAPI : false, returnByValue ? *returnByValue : false, generatePreview ? *generatePreview : false, &result, wasThrown);

    if (doNotPauseOnExceptionsAndMuteConsole ? *doNotPauseOnExceptionsAndMuteConsole : false) {
        unmuteConsole();
        if (scriptDebugServer().pauseOnExceptionsState() != previousPauseOnExceptionsState)
            scriptDebugServer().setPauseOnExceptionsState(previousPauseOnExceptionsState);
    }
}

void InspectorDebuggerAgent::compileScript(ErrorString* errorString, const String& expression, const String& sourceURL, TypeBuilder::OptOutput<ScriptId>* scriptId, TypeBuilder::OptOutput<String>* syntaxErrorMessage)
{
    InjectedScript injectedScript = injectedScriptForEval(errorString, 0);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }

    String scriptIdValue;
    String exceptionMessage;
    scriptDebugServer().compileScript(injectedScript.scriptState(), expression, sourceURL, &scriptIdValue, &exceptionMessage);
    if (!scriptIdValue && !exceptionMessage) {
        *errorString = "Script compilation failed";
        return;
    }
    *syntaxErrorMessage = exceptionMessage;
    *scriptId = scriptIdValue;
}

void InspectorDebuggerAgent::runScript(ErrorString* errorString, const ScriptId& scriptId, const int* executionContextId, const String* const objectGroup, const bool* const doNotPauseOnExceptionsAndMuteConsole, RefPtr<TypeBuilder::Runtime::RemoteObject>& result, TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = injectedScriptForEval(errorString, executionContextId);
    if (injectedScript.hasNoValue()) {
        *errorString = "Inspected frame has gone";
        return;
    }

    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = scriptDebugServer().pauseOnExceptionsState();
    if (doNotPauseOnExceptionsAndMuteConsole && *doNotPauseOnExceptionsAndMuteConsole) {
        if (previousPauseOnExceptionsState != ScriptDebugServer::DontPauseOnExceptions)
            scriptDebugServer().setPauseOnExceptionsState(ScriptDebugServer::DontPauseOnExceptions);
        muteConsole();
    }

    ScriptValue value;
    bool wasThrownValue;
    String exceptionMessage;
    scriptDebugServer().runScript(injectedScript.scriptState(), scriptId, &value, &wasThrownValue, &exceptionMessage);
    *wasThrown = wasThrownValue;
    if (value.hasNoValue()) {
        *errorString = "Script execution failed";
        return;
    }
    result = injectedScript.wrapObject(value, objectGroup ? *objectGroup : "");
    if (wasThrownValue)
        result->setDescription(exceptionMessage);

    if (doNotPauseOnExceptionsAndMuteConsole && *doNotPauseOnExceptionsAndMuteConsole) {
        unmuteConsole();
        if (scriptDebugServer().pauseOnExceptionsState() != previousPauseOnExceptionsState)
            scriptDebugServer().setPauseOnExceptionsState(previousPauseOnExceptionsState);
    }
}

void InspectorDebuggerAgent::setOverlayMessage(ErrorString*, const String*)
{
}

void InspectorDebuggerAgent::scriptExecutionBlockedByCSP(const String& directiveText)
{
    if (scriptDebugServer().pauseOnExceptionsState() != ScriptDebugServer::DontPauseOnExceptions) {
        RefPtr<InspectorObject> directive = InspectorObject::create();
        directive->setString("directiveText", directiveText);
        breakProgram(InspectorFrontend::Debugger::Reason::CSPViolation, directive.release());
    }
}

PassRefPtr<Array<TypeBuilder::Debugger::CallFrame>> InspectorDebuggerAgent::currentCallFrames()
{
    if (!m_pausedScriptState)
        return Array<TypeBuilder::Debugger::CallFrame>::create();
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(m_pausedScriptState);
    if (injectedScript.hasNoValue()) {
        ASSERT_NOT_REACHED();
        return Array<TypeBuilder::Debugger::CallFrame>::create();
    }
    return injectedScript.wrapCallFrames(m_currentCallStack);
}

String InspectorDebuggerAgent::sourceMapURLForScript(const Script& script)
{
    DEFINE_STATIC_LOCAL(String, sourceMapHTTPHeader, (ASCIILiteral("SourceMap")));
    DEFINE_STATIC_LOCAL(String, sourceMapHTTPHeaderDeprecated, (ASCIILiteral("X-SourceMap")));

    if (!script.url.isEmpty()) {
        if (InspectorPageAgent* pageAgent = m_instrumentingAgents->inspectorPageAgent()) {
            CachedResource* resource = pageAgent->cachedResource(pageAgent->mainFrame(), URL(ParsedURLString, script.url));
            if (resource) {
                String sourceMapHeader = resource->response().httpHeaderField(sourceMapHTTPHeader);
                if (!sourceMapHeader.isEmpty())
                    return sourceMapHeader;

                sourceMapHeader = resource->response().httpHeaderField(sourceMapHTTPHeaderDeprecated);
                if (!sourceMapHeader.isEmpty())
                    return sourceMapHeader;
            }
        }
    }

    return ContentSearchUtils::findScriptSourceMapURL(script.source);
}

// JavaScriptDebugListener functions

void InspectorDebuggerAgent::didParseSource(const String& scriptId, const Script& inScript)
{
    Script script = inScript;
    if (!script.startLine && !script.startColumn)
        script.sourceURL = ContentSearchUtils::findScriptSourceURL(script.source);
    script.sourceMappingURL = sourceMapURLForScript(script);

    bool hasSourceURL = !script.sourceURL.isEmpty();
    String scriptURL = hasSourceURL ? script.sourceURL : script.url;
    bool* hasSourceURLParam = hasSourceURL ? &hasSourceURL : 0;
    String* sourceMapURLParam = script.sourceMappingURL.isNull() ? 0 : &script.sourceMappingURL;
    const bool* isContentScript = script.isContentScript ? &script.isContentScript : 0;
    m_frontend->scriptParsed(scriptId, scriptURL, script.startLine, script.startColumn, script.endLine, script.endColumn, isContentScript, sourceMapURLParam, hasSourceURLParam);

    m_scripts.set(scriptId, script);

    if (scriptURL.isEmpty())
        return;

    for (auto it = m_javaScriptBreakpoints.begin(), end = m_javaScriptBreakpoints.end(); it != end; ++it) {
        RefPtr<InspectorObject> breakpointObject = it->value->asObject();
        bool isRegex;
        breakpointObject->getBoolean("isRegex", &isRegex);
        String url;
        breakpointObject->getString("url", &url);
        if (!matches(scriptURL, url, isRegex))
            continue;
        ScriptBreakpoint breakpoint;
        breakpointObject->getNumber("lineNumber", &breakpoint.lineNumber);
        breakpointObject->getNumber("columnNumber", &breakpoint.columnNumber);
        breakpointObject->getString("condition", &breakpoint.condition);
        breakpointObject->getBoolean("autoContinue", &breakpoint.autoContinue);
        ErrorString errorString;
        RefPtr<InspectorArray> actions = breakpointObject->getArray("actions");
        if (!breakpointActionsFromProtocol(&errorString, actions, &breakpoint.actions)) {
            ASSERT_NOT_REACHED();
            continue;
        }

        RefPtr<TypeBuilder::Debugger::Location> location = resolveBreakpoint(it->key, scriptId, breakpoint);
        if (location)
            m_frontend->breakpointResolved(it->key, location);
    }
}

void InspectorDebuggerAgent::failedToParseSource(const String& url, const String& data, int firstLine, int errorLine, const String& errorMessage)
{
    m_frontend->scriptFailedToParse(url, data, firstLine, errorLine, errorMessage);
}

void InspectorDebuggerAgent::didPause(JSC::ExecState* scriptState, const ScriptValue& callFrames, const ScriptValue& exception)
{
    ASSERT(scriptState && !m_pausedScriptState);
    m_pausedScriptState = scriptState;
    m_currentCallStack = callFrames;

    if (!exception.hasNoValue()) {
        InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(scriptState);
        if (!injectedScript.hasNoValue()) {
            m_breakReason = InspectorFrontend::Debugger::Reason::Exception;
            m_breakAuxData = injectedScript.wrapObject(exception, "backtrace")->openAccessors();
            // m_breakAuxData might be null after this.
        }
    }

    m_frontend->paused(currentCallFrames(), m_breakReason, m_breakAuxData);
    m_javaScriptPauseScheduled = false;

    if (!m_continueToLocationBreakpointId.isEmpty()) {
        scriptDebugServer().removeBreakpoint(m_continueToLocationBreakpointId);
        m_continueToLocationBreakpointId = "";
    }
    if (m_listener)
        m_listener->didPause();
}

void InspectorDebuggerAgent::didContinue()
{
    m_pausedScriptState = 0;
    m_currentCallStack = ScriptValue();
    clearBreakDetails();
    m_frontend->resumed();
}

void InspectorDebuggerAgent::breakProgram(InspectorFrontend::Debugger::Reason::Enum breakReason, PassRefPtr<InspectorObject> data)
{
    m_breakReason = breakReason;
    m_breakAuxData = data;
    scriptDebugServer().breakProgram();
}

void InspectorDebuggerAgent::clear()
{
    m_pausedScriptState = 0;
    m_currentCallStack = ScriptValue();
    m_scripts.clear();
    m_breakpointIdToDebugServerBreakpointIds.clear();
    m_continueToLocationBreakpointId = String();
    clearBreakDetails();
    m_javaScriptPauseScheduled = false;
    ErrorString error;
    setOverlayMessage(&error, 0);
}

bool InspectorDebuggerAgent::assertPaused(ErrorString* errorString)
{
    if (!m_pausedScriptState) {
        *errorString = "Can only perform operation while paused.";
        return false;
    }
    return true;
}

void InspectorDebuggerAgent::clearBreakDetails()
{
    m_breakReason = InspectorFrontend::Debugger::Reason::Other;
    m_breakAuxData = 0;
}

void InspectorDebuggerAgent::reset()
{
    scriptDebugServer().clearBreakpoints();
    m_scripts.clear();
    m_breakpointIdToDebugServerBreakpointIds.clear();
    if (m_frontend)
        m_frontend->globalObjectCleared();
}

} // namespace WebCore

#endif // ENABLE(JAVASCRIPT_DEBUGGER) && ENABLE(INSPECTOR)
