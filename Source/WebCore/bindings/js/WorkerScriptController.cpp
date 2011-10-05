/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2011 Google Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */

#include "config.h"

#if ENABLE(WORKERS)

#include "WorkerScriptController.h"

#include "JSDedicatedWorkerContext.h"
#include "ScriptSourceCode.h"
#include "ScriptValue.h"
#include "WebCoreJSClientData.h"
#include "WorkerContext.h"
#include "WorkerObjectProxy.h"
#include "WorkerThread.h"
#include <heap/StrongInlines.h>
#include <interpreter/Interpreter.h>
#include <runtime/Completion.h>
#include <runtime/ExceptionHelpers.h>
#include <runtime/Error.h>
#include <runtime/JSLock.h>

#if ENABLE(SHARED_WORKERS)
#include "JSSharedWorkerContext.h"
#endif

using namespace JSC;

namespace WebCore {

WorkerScriptController::WorkerScriptController(WorkerContext* workerContext)
    : m_globalData(JSGlobalData::create(ThreadStackTypeSmall))
    , m_workerContext(workerContext)
    , m_workerContextWrapper(*m_globalData)
    , m_executionForbidden(false)
{
    initNormalWorldClientData(m_globalData.get());
}

WorkerScriptController::~WorkerScriptController()
{
    m_workerContextWrapper.clear(); // Unprotect the global object.
    m_globalData->clearBuiltinStructures();
    m_globalData->heap.destroy();
}

void WorkerScriptController::initScript()
{
    ASSERT(!m_workerContextWrapper);

    JSLock lock(SilenceAssertionsOnly);

    // Explicitly protect the global object's prototype so it isn't collected
    // when we allocate the global object. (Once the global object is fully
    // constructed, it can mark its own prototype.)
    Structure* workerContextPrototypeStructure = JSWorkerContextPrototype::createStructure(*m_globalData, 0, jsNull());
    Strong<JSWorkerContextPrototype> workerContextPrototype(*m_globalData, JSWorkerContextPrototype::create(*m_globalData, 0, workerContextPrototypeStructure));

    if (m_workerContext->isDedicatedWorkerContext()) {
        Structure* dedicatedContextPrototypeStructure = JSDedicatedWorkerContextPrototype::createStructure(*m_globalData, 0, workerContextPrototype.get());
        Strong<JSDedicatedWorkerContextPrototype> dedicatedContextPrototype(*m_globalData, JSDedicatedWorkerContextPrototype::create(*m_globalData, 0, dedicatedContextPrototypeStructure));
        Structure* structure = JSDedicatedWorkerContext::createStructure(*m_globalData, 0, dedicatedContextPrototype.get());

        m_workerContextWrapper.set(*m_globalData, JSDedicatedWorkerContext::create(*m_globalData, structure, m_workerContext->toDedicatedWorkerContext()));
        workerContextPrototypeStructure->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
        dedicatedContextPrototypeStructure->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
        ASSERT(structure->globalObject() == m_workerContextWrapper);
        ASSERT(m_workerContextWrapper->structure()->globalObject() == m_workerContextWrapper);
        workerContextPrototype->structure()->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
        dedicatedContextPrototype->structure()->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
#if ENABLE(SHARED_WORKERS)
    } else {
        ASSERT(m_workerContext->isSharedWorkerContext());
        Structure* sharedContextPrototypeStructure = JSSharedWorkerContextPrototype::createStructure(*m_globalData, 0, workerContextPrototype.get());
        Strong<JSSharedWorkerContextPrototype> sharedContextPrototype(*m_globalData, JSSharedWorkerContextPrototype::create(*m_globalData, 0, sharedContextPrototypeStructure));
        Structure* structure = JSSharedWorkerContext::createStructure(*m_globalData, 0, sharedContextPrototype.get());

        m_workerContextWrapper.set(*m_globalData, JSSharedWorkerContext::create(*m_globalData, structure, m_workerContext->toSharedWorkerContext()));
        workerContextPrototype->structure()->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
        sharedContextPrototype->structure()->setGlobalObject(*m_globalData, m_workerContextWrapper.get());
#endif
    }
    ASSERT(m_workerContextWrapper->globalObject() == m_workerContextWrapper);
    ASSERT(asObject(m_workerContextWrapper->prototype())->globalObject() == m_workerContextWrapper);
}

ScriptValue WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode)
{
    if (isExecutionForbidden())
        return ScriptValue();

    ScriptValue exception;
    ScriptValue result(evaluate(sourceCode, &exception));
    if (exception.jsValue()) {
        JSLock lock(SilenceAssertionsOnly);
        reportException(m_workerContextWrapper->globalExec(), exception.jsValue());
    }
    return result;
}

ScriptValue WorkerScriptController::evaluate(const ScriptSourceCode& sourceCode, ScriptValue* exception)
{
    if (isExecutionForbidden())
        return ScriptValue();

    initScriptIfNeeded();
    JSLock lock(SilenceAssertionsOnly);

    ExecState* exec = m_workerContextWrapper->globalExec();

    m_workerContextWrapper->globalData().timeoutChecker.start();

    JSValue evaluationException;
    JSValue returnValue = JSC::evaluate(exec, exec->dynamicGlobalObject()->globalScopeChain(), sourceCode.jsSourceCode(), m_workerContextWrapper.get(), &evaluationException);

    m_workerContextWrapper->globalData().timeoutChecker.stop();

    if ((evaluationException && isTerminatedExecutionException(evaluationException)) ||  m_workerContextWrapper->globalData().terminator.shouldTerminate()) {
        forbidExecution();
        return ScriptValue();
    }

    if (evaluationException) {
        String errorMessage;
        int lineNumber = 0;
        String sourceURL = sourceCode.url().string();
        if (m_workerContext->sanitizeScriptError(errorMessage, lineNumber, sourceURL))
            *exception = ScriptValue(*m_globalData, throwError(exec, createError(exec, errorMessage.impl())));
        else
            *exception = ScriptValue(*m_globalData, evaluationException);
    }

    return ScriptValue(*m_globalData, returnValue);

}

void WorkerScriptController::setException(ScriptValue exception)
{
    throwError(m_workerContextWrapper->globalExec(), exception.jsValue());
}

void WorkerScriptController::scheduleExecutionTermination()
{
    m_globalData->terminator.terminateSoon();
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

void WorkerScriptController::disableEval()
{
    initScriptIfNeeded();
    JSLock lock(SilenceAssertionsOnly);

    m_workerContextWrapper->setEvalEnabled(false);
}

} // namespace WebCore

#endif // ENABLE(WORKERS)
