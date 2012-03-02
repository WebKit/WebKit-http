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

#ifndef WorkerInspectorController_h
#define WorkerInspectorController_h

#if ENABLE(INSPECTOR) && ENABLE(WORKERS)

#include <wtf/FastAllocBase.h>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>
#include <wtf/OwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class InjectedScriptManager;
#if ENABLE(JAVASCRIPT_DEBUGGER)
class InspectorDebuggerAgent;
#endif
class InspectorBackendDispatcher;
class InspectorConsoleAgent;
class InspectorFrontend;
class InspectorFrontendChannel;
class InspectorConsoleAgent;
class InspectorInstrumentation;
class InspectorProfilerAgent;
class InspectorRuntimeAgent;
class InspectorState;
class InspectorStateClient;
class InspectorTimelineAgent;
class InstrumentingAgents;
class WorkerContext;

class WorkerInspectorController {
    WTF_MAKE_NONCOPYABLE(WorkerInspectorController);
    WTF_MAKE_FAST_ALLOCATED;
public:
    WorkerInspectorController(WorkerContext*);
    ~WorkerInspectorController();

    bool hasFrontend() const { return m_frontend; }
    void connectFrontend();
    void disconnectFrontend();
    void restoreInspectorStateFromCookie(const String& inspectorCookie);
    void dispatchMessageFromFrontend(const String&);
#if ENABLE(JAVASCRIPT_DEBUGGER)
    void resume();
#endif

private:
    friend InstrumentingAgents* instrumentationForWorkerContext(WorkerContext*);

    WorkerContext* m_workerContext;
    OwnPtr<InspectorStateClient> m_stateClient;
    OwnPtr<InspectorState> m_state;
    OwnPtr<InstrumentingAgents> m_instrumentingAgents;
    OwnPtr<InjectedScriptManager> m_injectedScriptManager;
#if ENABLE(JAVASCRIPT_DEBUGGER)
    OwnPtr<InspectorDebuggerAgent> m_debuggerAgent;
    OwnPtr<InspectorProfilerAgent> m_profilerAgent;
#endif
    OwnPtr<InspectorRuntimeAgent> m_runtimeAgent;
    OwnPtr<InspectorConsoleAgent> m_consoleAgent;
    OwnPtr<InspectorTimelineAgent> m_timelineAgent;

    OwnPtr<InspectorFrontendChannel> m_frontendChannel;
    OwnPtr<InspectorFrontend> m_frontend;
    RefPtr<InspectorBackendDispatcher> m_backendDispatcher;
};

}

#endif // ENABLE(WORKERS)

#endif // !defined(WorkerInspectorController_h)
