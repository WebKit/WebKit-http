/*
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
#include "PageRuntimeAgent.h"

#if ENABLE(INSPECTOR)

#include "Document.h"
#include "Frame.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "JSDOMWindowBase.h"
#include "MainFrame.h"
#include "Page.h"
#include "PageConsole.h"
#include "ScriptController.h"
#include "ScriptState.h"
#include "SecurityOrigin.h"
#include <inspector/InjectedScript.h>
#include <inspector/InjectedScriptManager.h>

using Inspector::TypeBuilder::Runtime::ExecutionContextDescription;

using namespace Inspector;

namespace WebCore {

PageRuntimeAgent::PageRuntimeAgent(InjectedScriptManager* injectedScriptManager, Page* page, InspectorPageAgent* pageAgent)
    : InspectorRuntimeAgent(injectedScriptManager)
    , m_inspectedPage(page)
    , m_pageAgent(pageAgent)
    , m_mainWorldContextCreated(false)
{
}

void PageRuntimeAgent::didCreateFrontendAndBackend(Inspector::InspectorFrontendChannel* frontendChannel, InspectorBackendDispatcher* backendDispatcher)
{
    m_frontendDispatcher = std::make_unique<InspectorRuntimeFrontendDispatcher>(frontendChannel);
    m_backendDispatcher = InspectorRuntimeBackendDispatcher::create(backendDispatcher, this);
}

void PageRuntimeAgent::willDestroyFrontendAndBackend(InspectorDisconnectReason)
{
    m_frontendDispatcher = nullptr;
    m_backendDispatcher.clear();

    String errorString;
    disable(&errorString);
}

void PageRuntimeAgent::enable(ErrorString* errorString)
{
    if (enabled())
        return;

    InspectorRuntimeAgent::enable(errorString);

    // Only report existing contexts if the page did commit load, otherwise we may
    // unintentionally initialize contexts in the frames which may trigger some listeners
    // that are expected to be triggered only after the load is committed, see http://crbug.com/131623
    if (m_mainWorldContextCreated)
        reportExecutionContextCreation();
}

void PageRuntimeAgent::disable(ErrorString* errorString)
{
    if (!enabled())
        return;

    InspectorRuntimeAgent::disable(errorString);
}

void PageRuntimeAgent::didCreateMainWorldContext(Frame* frame)
{
    m_mainWorldContextCreated = true;

    if (!enabled())
        return;

    ASSERT(m_frontendDispatcher);
    String frameId = m_pageAgent->frameId(frame);
    JSC::ExecState* scriptState = mainWorldExecState(frame);
    notifyContextCreated(frameId, scriptState, nullptr, true);
}

void PageRuntimeAgent::didCreateIsolatedContext(Frame* frame, JSC::ExecState* scriptState, SecurityOrigin* origin)
{
    if (!enabled())
        return;

    ASSERT(m_frontendDispatcher);
    String frameId = m_pageAgent->frameId(frame);
    notifyContextCreated(frameId, scriptState, origin, false);
}

JSC::VM& PageRuntimeAgent::globalVM()
{
    return JSDOMWindowBase::commonVM();
}

InjectedScript PageRuntimeAgent::injectedScriptForEval(ErrorString* errorString, const int* executionContextId)
{
    if (!executionContextId) {
        JSC::ExecState* scriptState = mainWorldExecState(&m_inspectedPage->mainFrame());
        InjectedScript result = injectedScriptManager()->injectedScriptFor(scriptState);
        if (result.hasNoValue())
            *errorString = ASCIILiteral("Internal error: main world execution context not found.");
        return result;
    }

    InjectedScript injectedScript = injectedScriptManager()->injectedScriptForId(*executionContextId);
    if (injectedScript.hasNoValue())
        *errorString = ASCIILiteral("Execution context with given id not found.");
    return injectedScript;
}

void PageRuntimeAgent::muteConsole()
{
    PageConsole::mute();
}

void PageRuntimeAgent::unmuteConsole()
{
    PageConsole::unmute();
}

void PageRuntimeAgent::reportExecutionContextCreation()
{
    Vector<std::pair<JSC::ExecState*, SecurityOrigin*>> isolatedContexts;
    for (Frame* frame = &m_inspectedPage->mainFrame(); frame; frame = frame->tree().traverseNext()) {
        if (!frame->script().canExecuteScripts(NotAboutToExecuteScript))
            continue;
        String frameId = m_pageAgent->frameId(frame);

        JSC::ExecState* scriptState = mainWorldExecState(frame);
        notifyContextCreated(frameId, scriptState, nullptr, true);
        frame->script().collectIsolatedContexts(isolatedContexts);
        if (isolatedContexts.isEmpty())
            continue;
        for (size_t i = 0; i< isolatedContexts.size(); i++)
            notifyContextCreated(frameId, isolatedContexts[i].first, isolatedContexts[i].second, false);
        isolatedContexts.clear();
    }
}

void PageRuntimeAgent::notifyContextCreated(const String& frameId, JSC::ExecState* scriptState, SecurityOrigin* securityOrigin, bool isPageContext)
{
    ASSERT(securityOrigin || isPageContext);
    int executionContextId = injectedScriptManager()->injectedScriptIdFor(scriptState);
    String name = securityOrigin ? securityOrigin->toRawString() : "";
    m_frontendDispatcher->executionContextCreated(ExecutionContextDescription::create()
        .setId(executionContextId)
        .setIsPageContext(isPageContext)
        .setName(name)
        .setFrameId(frameId)
        .release());
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
