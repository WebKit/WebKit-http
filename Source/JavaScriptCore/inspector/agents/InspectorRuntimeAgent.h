/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef InspectorRuntimeAgent_h
#define InspectorRuntimeAgent_h

#if ENABLE(INSPECTOR)

#include "InspectorJSBackendDispatchers.h"
#include "InspectorJSFrontendDispatchers.h"
#include "inspector/InspectorAgentBase.h"
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace JSC {
class VM;
}

namespace Inspector {

class InjectedScript;
class InjectedScriptManager;
class InspectorArray;
class ScriptDebugServer;
typedef String ErrorString;

class JS_EXPORT_PRIVATE InspectorRuntimeAgent : public InspectorAgentBase, public InspectorRuntimeBackendDispatcherHandler {
    WTF_MAKE_NONCOPYABLE(InspectorRuntimeAgent);
public:
    virtual ~InspectorRuntimeAgent();

    virtual void willDestroyFrontendAndBackend(InspectorDisconnectReason) override;

    virtual void enable(ErrorString*) override { m_enabled = true; }
    virtual void disable(ErrorString*) override { m_enabled = false; }
    virtual void parse(ErrorString*, const String& expression, Inspector::Protocol::Runtime::SyntaxErrorType* result, Inspector::Protocol::OptOutput<String>* message, RefPtr<Inspector::Protocol::Runtime::ErrorRange>&) override final;
    virtual void evaluate(ErrorString*, const String& expression, const String* objectGroup, const bool* includeCommandLineAPI, const bool* doNotPauseOnExceptionsAndMuteConsole, const int* executionContextId, const bool* returnByValue, const bool* generatePreview, RefPtr<Inspector::Protocol::Runtime::RemoteObject>& result, Inspector::Protocol::OptOutput<bool>* wasThrown) override final;
    virtual void callFunctionOn(ErrorString*, const String& objectId, const String& expression, const RefPtr<Inspector::InspectorArray>* optionalArguments, const bool* doNotPauseOnExceptionsAndMuteConsole, const bool* returnByValue, const bool* generatePreview, RefPtr<Inspector::Protocol::Runtime::RemoteObject>& result, Inspector::Protocol::OptOutput<bool>* wasThrown) override final;
    virtual void releaseObject(ErrorString*, const ErrorString& objectId) override final;
    virtual void getProperties(ErrorString*, const String& objectId, const bool* ownProperties, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Runtime::PropertyDescriptor>>& result, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Runtime::InternalPropertyDescriptor>>& internalProperties) override final;
    virtual void releaseObjectGroup(ErrorString*, const String& objectGroup) override final;
    virtual void run(ErrorString*) override;
    virtual void getRuntimeTypesForVariablesAtOffsets(ErrorString*, const RefPtr<Inspector::InspectorArray>& locations, RefPtr<Inspector::Protocol::Array<Inspector::Protocol::Runtime::TypeDescription>>&) override;
    virtual void enableTypeProfiler(ErrorString*) override;
    virtual void disableTypeProfiler(ErrorString*) override;
    
    void setScriptDebugServer(ScriptDebugServer* scriptDebugServer) { m_scriptDebugServer = scriptDebugServer; }

    bool enabled() const { return m_enabled; }

protected:
    InspectorRuntimeAgent(InjectedScriptManager*);

    InjectedScriptManager* injectedScriptManager() { return m_injectedScriptManager; }

    virtual JSC::VM& globalVM() = 0;
    virtual InjectedScript injectedScriptForEval(ErrorString*, const int* executionContextId) = 0;

    virtual void muteConsole() = 0;
    virtual void unmuteConsole() = 0;

private:
    void setTypeProfilerEnabledState(bool);

    InjectedScriptManager* m_injectedScriptManager;
    ScriptDebugServer* m_scriptDebugServer;
    bool m_enabled;
    bool m_isTypeProfilingEnabled;
};

} // namespace Inspector

#endif // ENABLE(INSPECTOR)
#endif // InspectorRuntimeAgent_h
