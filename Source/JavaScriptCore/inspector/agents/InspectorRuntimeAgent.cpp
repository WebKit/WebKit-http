/*
 * Copyright (C) 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
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
#include "InspectorRuntimeAgent.h"

#if ENABLE(INSPECTOR)

#include "Completion.h"
#include "InjectedScript.h"
#include "InjectedScriptManager.h"
#include "InspectorValues.h"
#include "JSLock.h"
#include "ParserError.h"
#include "ScriptDebugServer.h"
#include "SourceCode.h"
#include <wtf/PassRefPtr.h>

using namespace JSC;

namespace Inspector {

static bool asBool(const bool* const b)
{
    return b ? *b : false;
}

InspectorRuntimeAgent::InspectorRuntimeAgent(InjectedScriptManager* injectedScriptManager)
    : InspectorAgentBase(ASCIILiteral("Runtime"))
    , m_injectedScriptManager(injectedScriptManager)
    , m_scriptDebugServer(nullptr)
    , m_enabled(false)
{
}

InspectorRuntimeAgent::~InspectorRuntimeAgent()
{
}

static ScriptDebugServer::PauseOnExceptionsState setPauseOnExceptionsState(ScriptDebugServer* scriptDebugServer, ScriptDebugServer::PauseOnExceptionsState newState)
{
    ASSERT(scriptDebugServer);
    ScriptDebugServer::PauseOnExceptionsState presentState = scriptDebugServer->pauseOnExceptionsState();
    if (presentState != newState)
        scriptDebugServer->setPauseOnExceptionsState(newState);
    return presentState;
}

static PassRefPtr<Inspector::TypeBuilder::Runtime::ErrorRange> buildErrorRangeObject(const JSTokenLocation& tokenLocation)
{
    RefPtr<Inspector::TypeBuilder::Runtime::ErrorRange> result = Inspector::TypeBuilder::Runtime::ErrorRange::create()
        .setStartOffset(tokenLocation.startOffset)
        .setEndOffset(tokenLocation.endOffset);
    return result.release();
}

void InspectorRuntimeAgent::parse(ErrorString*, const String& expression, Inspector::TypeBuilder::Runtime::SyntaxErrorType::Enum* result, Inspector::TypeBuilder::OptOutput<String>* message, RefPtr<Inspector::TypeBuilder::Runtime::ErrorRange>& range)
{
    VM& vm = globalVM();
    JSLockHolder lock(vm);

    ParserError error;
    checkSyntax(vm, JSC::makeSource(expression), error);

    switch (error.m_syntaxErrorType) {
    case ParserError::SyntaxErrorNone:
        *result = Inspector::TypeBuilder::Runtime::SyntaxErrorType::None;
        break;
    case ParserError::SyntaxErrorIrrecoverable:
        *result = Inspector::TypeBuilder::Runtime::SyntaxErrorType::Irrecoverable;
        break;
    case ParserError::SyntaxErrorUnterminatedLiteral:
        *result = Inspector::TypeBuilder::Runtime::SyntaxErrorType::UnterminatedLiteral;
        break;
    case ParserError::SyntaxErrorRecoverable:
        *result = Inspector::TypeBuilder::Runtime::SyntaxErrorType::Recoverable;
        break;
    }

    if (error.m_syntaxErrorType != ParserError::SyntaxErrorNone) {
        *message = error.m_message;
        range = buildErrorRangeObject(error.m_token.m_location);
    }
}

void InspectorRuntimeAgent::evaluate(ErrorString* errorString, const String& expression, const String* const objectGroup, const bool* const includeCommandLineAPI, const bool* const doNotPauseOnExceptionsAndMuteConsole, const int* executionContextId, const bool* const returnByValue, const bool* generatePreview, RefPtr<Inspector::TypeBuilder::Runtime::RemoteObject>& result, Inspector::TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = injectedScriptForEval(errorString, executionContextId);
    if (injectedScript.hasNoValue())
        return;

    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = ScriptDebugServer::DontPauseOnExceptions;
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        muteConsole();

    injectedScript.evaluate(errorString, expression, objectGroup ? *objectGroup : "", asBool(includeCommandLineAPI), asBool(returnByValue), asBool(generatePreview), &result, wasThrown);

    if (asBool(doNotPauseOnExceptionsAndMuteConsole)) {
        unmuteConsole();
        setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
    }
}

void InspectorRuntimeAgent::callFunctionOn(ErrorString* errorString, const String& objectId, const String& expression, const RefPtr<InspectorArray>* const optionalArguments, const bool* const doNotPauseOnExceptionsAndMuteConsole, const bool* const returnByValue, const bool* generatePreview, RefPtr<Inspector::TypeBuilder::Runtime::RemoteObject>& result, Inspector::TypeBuilder::OptOutput<bool>* wasThrown)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = ASCIILiteral("Inspected frame has gone");
        return;
    }

    String arguments;
    if (optionalArguments)
        arguments = (*optionalArguments)->toJSONString();

    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = ScriptDebugServer::DontPauseOnExceptions;
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
    if (asBool(doNotPauseOnExceptionsAndMuteConsole))
        muteConsole();

    injectedScript.callFunctionOn(errorString, objectId, expression, arguments, asBool(returnByValue), asBool(generatePreview), &result, wasThrown);

    if (asBool(doNotPauseOnExceptionsAndMuteConsole)) {
        unmuteConsole();
        setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
    }
}

void InspectorRuntimeAgent::getProperties(ErrorString* errorString, const String& objectId, const bool* const ownProperties, RefPtr<Inspector::TypeBuilder::Array<Inspector::TypeBuilder::Runtime::PropertyDescriptor>>& result, RefPtr<Inspector::TypeBuilder::Array<Inspector::TypeBuilder::Runtime::InternalPropertyDescriptor>>& internalProperties)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue()) {
        *errorString = ASCIILiteral("Inspected frame has gone");
        return;
    }

    ScriptDebugServer::PauseOnExceptionsState previousPauseOnExceptionsState = setPauseOnExceptionsState(m_scriptDebugServer, ScriptDebugServer::DontPauseOnExceptions);
    muteConsole();

    injectedScript.getProperties(errorString, objectId, ownProperties ? *ownProperties : false, &result);
    injectedScript.getInternalProperties(errorString, objectId, &internalProperties);

    unmuteConsole();
    setPauseOnExceptionsState(m_scriptDebugServer, previousPauseOnExceptionsState);
}

void InspectorRuntimeAgent::releaseObject(ErrorString*, const String& objectId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    if (!injectedScript.hasNoValue())
        injectedScript.releaseObject(objectId);
}

void InspectorRuntimeAgent::releaseObjectGroup(ErrorString*, const String& objectGroup)
{
    m_injectedScriptManager->releaseObjectGroup(objectGroup);
}

void InspectorRuntimeAgent::run(ErrorString*)
{
}

void InspectorRuntimeAgent::getRuntimeTypeForVariableInTextRange(ErrorString*, const String& in_variableName, const String& in_id, int in_startLine, int in_startColumn, int in_endLine, int in_endColumn, String* out_types) 
{
    VM& vm = globalVM();
    String types(vm.getTypesForVariableInRange(in_startLine, in_startColumn, in_endLine, in_endColumn, in_variableName, in_id));
    *out_types = types;
}

} // namespace Inspector

#endif // ENABLE(INSPECTOR)
