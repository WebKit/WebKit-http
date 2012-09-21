/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#if ENABLE(INSPECTOR)

#include "InjectedScriptCanvasModule.h"

#include "InjectedScript.h"
#include "InjectedScriptCanvasModuleSource.h"
#include "InjectedScriptManager.h"
#include "ScriptFunctionCall.h"
#include "ScriptObject.h"

namespace WebCore {

InjectedScriptCanvasModule::InjectedScriptCanvasModule()
    : InjectedScriptModule("InjectedScriptCanvasModule")
{
}

InjectedScriptCanvasModule InjectedScriptCanvasModule::moduleForState(InjectedScriptManager* injectedScriptManager, ScriptState* scriptState)
{
    InjectedScriptCanvasModule result;
    result.ensureInjected(injectedScriptManager, scriptState);
    return result;
}

String InjectedScriptCanvasModule::source() const
{
    return String(reinterpret_cast<const char*>(InjectedScriptCanvasModuleSource_js), sizeof(InjectedScriptCanvasModuleSource_js));
}

#if ENABLE(WEBGL)
ScriptObject InjectedScriptCanvasModule::wrapWebGLContext(const ScriptObject& glContext)
{
    ScriptFunctionCall function(injectedScriptObject(), "wrapWebGLContext");
    function.appendArgument(glContext);
    bool hadException = false;
    ScriptValue resultValue = callFunctionWithEvalEnabled(function, hadException);
    if (hadException || resultValue.hasNoValue() || !resultValue.isObject()) {
        ASSERT_NOT_REACHED();
        return ScriptObject();
    }
    return ScriptObject(glContext.scriptState(), resultValue);
}
#endif // ENABLE(WEBGL)

void InjectedScriptCanvasModule::captureFrame(ErrorString* errorString, String* traceLogId)
{
    ScriptFunctionCall function(injectedScriptObject(), "captureFrame");
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeString || !resultValue->asString(traceLogId))
        *errorString = "Internal error: captureFrame";
}

void InjectedScriptCanvasModule::dropTraceLog(ErrorString* errorString, const String& traceLogId)
{
    ScriptFunctionCall function(injectedScriptObject(), "dropTraceLog");
    function.appendArgument(traceLogId);
    bool hadException = false;
    callFunctionWithEvalEnabled(function, hadException);
    ASSERT(!hadException);
    if (hadException)
        *errorString = "Internal error: dropTraceLog";
}

void InjectedScriptCanvasModule::traceLog(ErrorString* errorString, const String& traceLogId, RefPtr<TypeBuilder::Canvas::TraceLog>* traceLog)
{
    ScriptFunctionCall function(injectedScriptObject(), "traceLog");
    function.appendArgument(traceLogId);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeObject) {
        if (!resultValue->asString(errorString))
            *errorString = "Internal error: traceLog";
        return;
    }
    *traceLog = TypeBuilder::Canvas::TraceLog::runtimeCast(resultValue);
}

void InjectedScriptCanvasModule::replayTraceLog(ErrorString* errorString, const String& traceLogId, int stepNo, String* result)
{
    ScriptFunctionCall function(injectedScriptObject(), "replayTraceLog");
    function.appendArgument(traceLogId);
    function.appendArgument(stepNo);
    RefPtr<InspectorValue> resultValue;
    makeCall(function, &resultValue);
    if (!resultValue || resultValue->type() != InspectorValue::TypeString || !resultValue->asString(result))
        *errorString = "Internal error: replayTraceLog";
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
