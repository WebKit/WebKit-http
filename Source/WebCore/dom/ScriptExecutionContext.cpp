/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2012 Google Inc. All Rights Reserved.
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
#include "ScriptExecutionContext.h"

#include "ActiveDOMObject.h"
#include "ContentSecurityPolicy.h"
#include "DOMTimer.h"
#include "ErrorEvent.h"
#include "EventListener.h"
#include "EventTarget.h"
#include "FileThread.h"
#include "MessagePort.h"
#include "PublicURLManager.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"
#include "Settings.h"
#include "WorkerContext.h"
#include "WorkerThread.h"
#include <wtf/MainThread.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

#if USE(JSC)
// FIXME: This is a layering violation.
#include "JSDOMWindow.h"
#endif

namespace WebCore {

class ProcessMessagesSoonTask : public ScriptExecutionContext::Task {
public:
    static PassOwnPtr<ProcessMessagesSoonTask> create()
    {
        return adoptPtr(new ProcessMessagesSoonTask);
    }

    virtual void performTask(ScriptExecutionContext* context)
    {
        context->dispatchMessagePortEvents();
    }
};

class ScriptExecutionContext::PendingException {
    WTF_MAKE_NONCOPYABLE(PendingException);
public:
    PendingException(const String& errorMessage, int lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack> callStack)
        : m_errorMessage(errorMessage)
        , m_lineNumber(lineNumber)
        , m_sourceURL(sourceURL)
        , m_callStack(callStack)
    {
    }
    String m_errorMessage;
    int m_lineNumber;
    String m_sourceURL;
    RefPtr<ScriptCallStack> m_callStack;
};

void ScriptExecutionContext::AddConsoleMessageTask::performTask(ScriptExecutionContext* context)
{
    context->addConsoleMessage(m_source, m_type, m_level, m_message);
}

ScriptExecutionContext::ScriptExecutionContext()
    : m_iteratingActiveDOMObjects(false)
    , m_inDestructor(false)
    , m_inDispatchErrorEvent(false)
    , m_activeDOMObjectsAreSuspended(false)
    , m_reasonForSuspendingActiveDOMObjects(static_cast<ActiveDOMObject::ReasonForSuspension>(-1))
    , m_activeDOMObjectsAreStopped(false)
{
}

ScriptExecutionContext::~ScriptExecutionContext()
{
    m_inDestructor = true;
    for (HashSet<ContextDestructionObserver*>::iterator iter = m_destructionObservers.begin(); iter != m_destructionObservers.end(); iter = m_destructionObservers.begin()) {
        ContextDestructionObserver* observer = *iter;
        m_destructionObservers.remove(observer);
        ASSERT(observer->scriptExecutionContext() == this);
        observer->contextDestroyed();
    }

    HashSet<MessagePort*>::iterator messagePortsEnd = m_messagePorts.end();
    for (HashSet<MessagePort*>::iterator iter = m_messagePorts.begin(); iter != messagePortsEnd; ++iter) {
        ASSERT((*iter)->scriptExecutionContext() == this);
        (*iter)->contextDestroyed();
    }
#if ENABLE(BLOB)
    if (m_fileThread) {
        m_fileThread->stop();
        m_fileThread = 0;
    }
    if (m_publicURLManager)
        m_publicURLManager->contextDestroyed();
#endif
}

void ScriptExecutionContext::processMessagePortMessagesSoon()
{
    postTask(ProcessMessagesSoonTask::create());
}

void ScriptExecutionContext::dispatchMessagePortEvents()
{
    RefPtr<ScriptExecutionContext> protect(this);

    // Make a frozen copy.
    Vector<MessagePort*> ports;
    copyToVector(m_messagePorts, ports);

    unsigned portCount = ports.size();
    for (unsigned i = 0; i < portCount; ++i) {
        MessagePort* port = ports[i];
        // The port may be destroyed, and another one created at the same address, but this is safe, as the worst that can happen
        // as a result is that dispatchMessages() will be called needlessly.
        if (m_messagePorts.contains(port) && port->started())
            port->dispatchMessages();
    }
}

void ScriptExecutionContext::createdMessagePort(MessagePort* port)
{
    ASSERT(port);
#if ENABLE(WORKERS)
    ASSERT((isDocument() && isMainThread())
        || (isWorkerContext() && currentThread() == static_cast<WorkerContext*>(this)->thread()->threadID()));
#endif

    m_messagePorts.add(port);
}

void ScriptExecutionContext::destroyedMessagePort(MessagePort* port)
{
    ASSERT(port);
#if ENABLE(WORKERS)
    ASSERT((isDocument() && isMainThread())
        || (isWorkerContext() && currentThread() == static_cast<WorkerContext*>(this)->thread()->threadID()));
#endif

    m_messagePorts.remove(port);
}

bool ScriptExecutionContext::canSuspendActiveDOMObjects()
{
    // No protection against m_activeDOMObjects changing during iteration: canSuspend() shouldn't execute arbitrary JS.
    m_iteratingActiveDOMObjects = true;
    HashMap<ActiveDOMObject*, void*>::iterator activeObjectsEnd = m_activeDOMObjects.end();
    for (HashMap<ActiveDOMObject*, void*>::iterator iter = m_activeDOMObjects.begin(); iter != activeObjectsEnd; ++iter) {
        ASSERT(iter->first->scriptExecutionContext() == this);
        ASSERT(iter->first->suspendIfNeededCalled());
        if (!iter->first->canSuspend()) {
            m_iteratingActiveDOMObjects = false;
            return false;
        }
    }
    m_iteratingActiveDOMObjects = false;
    return true;
}

void ScriptExecutionContext::suspendActiveDOMObjects(ActiveDOMObject::ReasonForSuspension why)
{
    // No protection against m_activeDOMObjects changing during iteration: suspend() shouldn't execute arbitrary JS.
    m_iteratingActiveDOMObjects = true;
    HashMap<ActiveDOMObject*, void*>::iterator activeObjectsEnd = m_activeDOMObjects.end();
    for (HashMap<ActiveDOMObject*, void*>::iterator iter = m_activeDOMObjects.begin(); iter != activeObjectsEnd; ++iter) {
        ASSERT(iter->first->scriptExecutionContext() == this);
        ASSERT(iter->first->suspendIfNeededCalled());
        iter->first->suspend(why);
    }
    m_iteratingActiveDOMObjects = false;
    m_activeDOMObjectsAreSuspended = true;
    m_reasonForSuspendingActiveDOMObjects = why;
}

void ScriptExecutionContext::resumeActiveDOMObjects()
{
    m_activeDOMObjectsAreSuspended = false;
    // No protection against m_activeDOMObjects changing during iteration: resume() shouldn't execute arbitrary JS.
    m_iteratingActiveDOMObjects = true;
    HashMap<ActiveDOMObject*, void*>::iterator activeObjectsEnd = m_activeDOMObjects.end();
    for (HashMap<ActiveDOMObject*, void*>::iterator iter = m_activeDOMObjects.begin(); iter != activeObjectsEnd; ++iter) {
        ASSERT(iter->first->scriptExecutionContext() == this);
        ASSERT(iter->first->suspendIfNeededCalled());
        iter->first->resume();
    }
    m_iteratingActiveDOMObjects = false;
}

void ScriptExecutionContext::stopActiveDOMObjects()
{
    m_activeDOMObjectsAreStopped = true;
    // No protection against m_activeDOMObjects changing during iteration: stop() shouldn't execute arbitrary JS.
    m_iteratingActiveDOMObjects = true;
    HashMap<ActiveDOMObject*, void*>::iterator activeObjectsEnd = m_activeDOMObjects.end();
    for (HashMap<ActiveDOMObject*, void*>::iterator iter = m_activeDOMObjects.begin(); iter != activeObjectsEnd; ++iter) {
        ASSERT(iter->first->scriptExecutionContext() == this);
        ASSERT(iter->first->suspendIfNeededCalled());
        iter->first->stop();
    }
    m_iteratingActiveDOMObjects = false;

    // Also close MessagePorts. If they were ActiveDOMObjects (they could be) then they could be stopped instead.
    closeMessagePorts();
}

void ScriptExecutionContext::suspendActiveDOMObjectIfNeeded(ActiveDOMObject* object)
{
    ASSERT(m_activeDOMObjects.contains(object));
    // Ensure all ActiveDOMObjects are suspended also newly created ones.
    if (m_activeDOMObjectsAreSuspended)
        object->suspend(m_reasonForSuspendingActiveDOMObjects);
}

void ScriptExecutionContext::didCreateActiveDOMObject(ActiveDOMObject* object, void* upcastPointer)
{
    ASSERT(object);
    ASSERT(upcastPointer);
    ASSERT(!m_inDestructor);
    if (m_iteratingActiveDOMObjects)
        CRASH();
    m_activeDOMObjects.add(object, upcastPointer);
}

void ScriptExecutionContext::willDestroyActiveDOMObject(ActiveDOMObject* object)
{
    ASSERT(object);
    if (m_iteratingActiveDOMObjects)
        CRASH();
    m_activeDOMObjects.remove(object);
}

void ScriptExecutionContext::didCreateDestructionObserver(ContextDestructionObserver* observer)
{
    ASSERT(observer);
    ASSERT(!m_inDestructor);
    m_destructionObservers.add(observer);
}

void ScriptExecutionContext::willDestroyDestructionObserver(ContextDestructionObserver* observer)
{
    ASSERT(observer);
    m_destructionObservers.remove(observer);
}

void ScriptExecutionContext::closeMessagePorts() {
    HashSet<MessagePort*>::iterator messagePortsEnd = m_messagePorts.end();
    for (HashSet<MessagePort*>::iterator iter = m_messagePorts.begin(); iter != messagePortsEnd; ++iter) {
        ASSERT((*iter)->scriptExecutionContext() == this);
        (*iter)->close();
    }
}

bool ScriptExecutionContext::sanitizeScriptError(String& errorMessage, int& lineNumber, String& sourceURL)
{
    KURL targetURL = completeURL(sourceURL);
    if (securityOrigin()->canRequest(targetURL))
        return false;
    errorMessage = "Script error.";
    sourceURL = String();
    lineNumber = 0;
    return true;
}

void ScriptExecutionContext::reportException(const String& errorMessage, int lineNumber, const String& sourceURL, PassRefPtr<ScriptCallStack> callStack)
{
    if (m_inDispatchErrorEvent) {
        if (!m_pendingExceptions)
            m_pendingExceptions = adoptPtr(new Vector<OwnPtr<PendingException> >());
        m_pendingExceptions->append(adoptPtr(new PendingException(errorMessage, lineNumber, sourceURL, callStack)));
        return;
    }

    // First report the original exception and only then all the nested ones.
    if (!dispatchErrorEvent(errorMessage, lineNumber, sourceURL))
        logExceptionToConsole(errorMessage, sourceURL, lineNumber, callStack);

    if (!m_pendingExceptions)
        return;

    for (size_t i = 0; i < m_pendingExceptions->size(); i++) {
        PendingException* e = m_pendingExceptions->at(i).get();
        logExceptionToConsole(e->m_errorMessage, e->m_sourceURL, e->m_lineNumber, e->m_callStack);
    }
    m_pendingExceptions.clear();
}

void ScriptExecutionContext::addConsoleMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, const String& sourceURL, unsigned lineNumber, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(source, type, level, message, sourceURL, lineNumber, callStack);
}

void ScriptExecutionContext::addConsoleMessage(MessageSource source, MessageType type, MessageLevel level, const String& message, PassRefPtr<ScriptCallStack> callStack)
{
    addMessage(source, type, level, message, String(), 0, callStack);
}


bool ScriptExecutionContext::dispatchErrorEvent(const String& errorMessage, int lineNumber, const String& sourceURL)
{
    EventTarget* target = errorEventTarget();
    if (!target)
        return false;

    String message = errorMessage;
    int line = lineNumber;
    String sourceName = sourceURL;
    sanitizeScriptError(message, line, sourceName);

    ASSERT(!m_inDispatchErrorEvent);
    m_inDispatchErrorEvent = true;
    RefPtr<ErrorEvent> errorEvent = ErrorEvent::create(message, sourceName, line);
    target->dispatchEvent(errorEvent);
    m_inDispatchErrorEvent = false;
    return errorEvent->defaultPrevented();
}

void ScriptExecutionContext::addTimeout(int timeoutId, DOMTimer* timer)
{
    ASSERT(!m_timeouts.contains(timeoutId));
    m_timeouts.set(timeoutId, timer);
}

void ScriptExecutionContext::removeTimeout(int timeoutId)
{
    m_timeouts.remove(timeoutId);
}

DOMTimer* ScriptExecutionContext::findTimeout(int timeoutId)
{
    return m_timeouts.get(timeoutId);
}

#if ENABLE(BLOB)
FileThread* ScriptExecutionContext::fileThread()
{
    if (!m_fileThread) {
        m_fileThread = FileThread::create();
        if (!m_fileThread->start())
            m_fileThread = 0;
    }
    return m_fileThread.get();
}

PublicURLManager& ScriptExecutionContext::publicURLManager()
{
    if (!m_publicURLManager)
        m_publicURLManager = PublicURLManager::create();
    return *m_publicURLManager;
}
#endif

void ScriptExecutionContext::adjustMinimumTimerInterval(double oldMinimumTimerInterval)
{
    if (minimumTimerInterval() != oldMinimumTimerInterval) {
        for (TimeoutMap::iterator iter = m_timeouts.begin(); iter != m_timeouts.end(); ++iter) {
            DOMTimer* timer = iter->second;
            timer->adjustMinimumTimerInterval(oldMinimumTimerInterval);
        }
    }
}

double ScriptExecutionContext::minimumTimerInterval() const
{
    // The default implementation returns the DOMTimer's default
    // minimum timer interval. FIXME: to make it work with dedicated
    // workers, we will have to override it in the appropriate
    // subclass, and provide a way to enumerate a Document's dedicated
    // workers so we can update them all.
    return Settings::defaultMinDOMTimerInterval();
}

ScriptExecutionContext::Task::~Task()
{
}

#if USE(JSC)
JSC::JSGlobalData* ScriptExecutionContext::globalData()
{
     if (isDocument())
        return JSDOMWindow::commonJSGlobalData();

#if ENABLE(WORKERS)
    if (isWorkerContext())
        return static_cast<WorkerContext*>(this)->script()->globalData();
#endif

    ASSERT_NOT_REACHED();
    return 0;
}
#endif

} // namespace WebCore
