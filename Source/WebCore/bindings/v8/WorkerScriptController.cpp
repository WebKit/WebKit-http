/*
 * Copyright (C) 2009, 2012 Google Inc. All rights reserved.
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

#if ENABLE(WORKERS)

#include "WorkerScriptController.h"

#include "DOMTimer.h"
#include "ScriptCallStack.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "V8DOMMap.h"
#include "V8WorkerContext.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"
#include "WorkerObjectProxy.h"
#include "WorkerThread.h"
#include <v8.h>

#if PLATFORM(CHROMIUM)
#include <public/Platform.h>
#include <public/WebWorkerRunLoop.h>
#endif

namespace WebCore {

WorkerScriptController::WorkerScriptController(WorkerContext* workerContext)
    : m_workerContext(workerContext)
    , m_isolate(v8::Isolate::New())
    , m_executionForbidden(false)
    , m_executionScheduledToTerminate(false)
{
    V8PerIsolateData* data = V8PerIsolateData::create(m_isolate);
    data->allStores().append(&m_DOMDataStore);
    data->setDOMDataStore(&m_DOMDataStore);
    m_isolate->Enter();
    m_proxy = adoptPtr(new WorkerContextExecutionProxy(workerContext));
}

WorkerScriptController::~WorkerScriptController()
{
    removeAllDOMObjects();
#if PLATFORM(CHROMIUM)
    // The corresponding call to didStartWorkerRunLoop is in
    // WorkerThread::workerThread().
    // See http://webkit.org/b/83104#c14 for why this is here.
    WebKit::Platform::current()->didStopWorkerRunLoop(WebKit::WebWorkerRunLoop(&m_workerContext->thread()->runLoop()));
#endif
    m_proxy.clear();
    V8PerIsolateData::dispose(m_isolate);
    m_isolate->Exit();
    m_isolate->Dispose();
}

void WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    evaluate(sourceCode, 0);
}

void WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode, ScriptValue* exception)
{
    if (isExecutionForbidden())
        return;

    WorkerContextExecutionState state;
    m_proxy->evaluate(sourceCode.source(), sourceCode.url().string(), sourceCode.startPosition(), &state);
    if (state.hadException) {
        if (exception)
            *exception = state.exception;
        else
            m_workerContext->reportException(state.errorMessage, state.lineNumber, state.sourceURL, 0);
    }
}

void WorkerScriptController::scheduleExecutionTermination()
{
    // The mutex provides a memory barrier to ensure that once
    // termination is scheduled, isExecutionTerminating will
    // accurately reflect that state when called from another thread.
    {
        MutexLocker locker(m_scheduledTerminationMutex);
        m_executionScheduledToTerminate = true;
    }
    v8::V8::TerminateExecution(m_isolate);
}

bool WorkerScriptController::isExecutionTerminating() const
{
    // See comments in scheduleExecutionTermination regarding mutex usage.
    MutexLocker locker(m_scheduledTerminationMutex);
    return m_executionScheduledToTerminate;
}

void WorkerScriptController::forbidExecution()
{
    ASSERT(m_workerContext->isContextThread());
    m_executionForbidden = true;
}

bool WorkerScriptController::isExecutionForbidden() const
{
    ASSERT(m_workerContext->isContextThread());
    return m_executionForbidden;
}

void WorkerScriptController::disableEval(const String& /* errorMessage */)
{
    m_proxy->setEvalAllowed(false);
}

void WorkerScriptController::setException(const ScriptValue& exception)
{
    throwError(*exception.v8Value());
}

WorkerScriptController* WorkerScriptController::controllerForContext()
{
    // Happens on frame destruction, check otherwise GetCurrent() will crash.
    if (!v8::Context::InContext())
        return 0;
    v8::Handle<v8::Context> context = v8::Context::GetCurrent();
    v8::Handle<v8::Object> global = context->Global();
    global = V8DOMWrapper::lookupDOMWrapper(V8WorkerContext::GetTemplate(), global);
    // Return 0 if the current executing context is not the worker context.
    if (global.IsEmpty())
        return 0;
    WorkerContext* workerContext = V8WorkerContext::toNative(global);
    return workerContext->script();
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
