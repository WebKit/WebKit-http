/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
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
#include "WebWorkerClientImpl.h"

#if ENABLE(WORKERS)

#include "CrossThreadTask.h"
#include "DedicatedWorkerThread.h"
#include "Document.h"
#include "ErrorEvent.h"
#include "Frame.h"
#include "FrameLoaderClient.h"
#include "InspectorInstrumentation.h"
#include "MessageEvent.h"
#include "MessagePort.h"
#include "MessagePortChannel.h"
#include "ScriptCallStack.h"
#include "ScriptExecutionContext.h"
#include "Worker.h"
#include "WorkerContext.h"
#include "WorkerContextExecutionProxy.h"
#include "WorkerScriptController.h"
#include "WorkerMessagingProxy.h"
#include <wtf/Threading.h>

#include "FrameLoaderClientImpl.h"
#include "PlatformMessagePortChannel.h"
#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "WebKit.h"
#include "WebKitClient.h"
#include "WebMessagePortChannel.h"
#include "WebString.h"
#include "WebURL.h"
#include "WebViewImpl.h"
#include "WebWorker.h"
#include "WebWorkerImpl.h"

using namespace WebCore;

namespace WebKit {

// When WebKit creates a WorkerContextProxy object, we check if we're in the
// renderer or worker process.  If the latter, then we just use
// WorkerMessagingProxy.
//
// If we're in the renderer process, then we need use the glue provided
// WebWorker object to talk to the worker process over IPC.  The worker process
// talks to Worker* using WorkerObjectProxy, which we implement on
// WebWorkerClientImpl.
//
// Note that if we're running each worker in a separate process, then nested
// workers end up using the same codepath as the renderer process.

// static
WorkerContextProxy* WebWorkerClientImpl::createWorkerContextProxy(Worker* worker)
{
    // Special behavior for multiple workers per process.
    // FIXME: v8 doesn't support more than one workers per process.
    // if (!worker->scriptExecutionContext()->isDocument())
    //     return new WorkerMessagingProxy(worker);

    WebWorker* webWorker = 0;
    WebWorkerClientImpl* proxy = new WebWorkerClientImpl(worker);

    if (worker->scriptExecutionContext()->isDocument()) {
        Document* document = static_cast<Document*>(
            worker->scriptExecutionContext());
        WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
        webWorker = webFrame->client()->createWorker(webFrame, proxy);
    } else {
        WorkerScriptController* controller = WorkerScriptController::controllerForContext();
        if (!controller) {
            ASSERT_NOT_REACHED();
            return 0;
        }

        DedicatedWorkerThread* thread = static_cast<DedicatedWorkerThread*>(controller->workerContext()->thread());
        WorkerObjectProxy* workerObjectProxy = &thread->workerObjectProxy();
        WebWorkerImpl* impl = reinterpret_cast<WebWorkerImpl*>(workerObjectProxy);
        webWorker = impl->client()->createWorker(proxy);
    }

    proxy->setWebWorker(webWorker);
    return proxy;
}

WebWorkerClientImpl::WebWorkerClientImpl(Worker* worker)
    : m_scriptExecutionContext(worker->scriptExecutionContext())
    , m_worker(worker)
    , m_askedToTerminate(false)
    , m_unconfirmedMessageCount(0)
    , m_workerContextHadPendingActivity(false)
    , m_workerThreadId(currentThread())
    , m_pageInspector(0)
{
}

WebWorkerClientImpl::~WebWorkerClientImpl()
{
}

void WebWorkerClientImpl::setWebWorker(WebWorker* webWorker)
{
    m_webWorker = webWorker;
}

void WebWorkerClientImpl::startWorkerContext(const KURL& scriptURL,
                                             const String& userAgent,
                                             const String& sourceCode)
{
    // Worker.terminate() could be called from JS before the context is started.
    if (m_askedToTerminate)
        return;
    if (!isMainThread()) {
        WebWorkerBase::dispatchTaskToMainThread(createCallbackTask(
            &startWorkerContextTask,
            AllowCrossThreadAccess(this),
            scriptURL.string(),
            userAgent,
            sourceCode));
        return;
    }
    startWorkerContextInternal(scriptURL, userAgent, sourceCode);
}

void WebWorkerClientImpl::startWorkerContextInternal(const KURL& scriptURL, const WTF::String& userAgent, const WTF::String& sourceCode)
{
    m_webWorker->startWorkerContext(scriptURL, userAgent, sourceCode);
    m_workerContextHadPendingActivity = true; // Worker initialization means a pending activity.
}

void WebWorkerClientImpl::terminateWorkerContext()
{
    if (m_askedToTerminate)
        return;
    m_askedToTerminate = true;
    if (!isMainThread()) {
        WebWorkerBase::dispatchTaskToMainThread(
            createCallbackTask(&terminateWorkerContextTask, AllowCrossThreadAccess(this)));
        return;
    }
    m_webWorker->terminateWorkerContext();
    InspectorInstrumentation::workerContextTerminated(m_scriptExecutionContext.get(), this);
}

void WebWorkerClientImpl::postMessageToWorkerContext(
    PassRefPtr<SerializedScriptValue> message,
    PassOwnPtr<MessagePortChannelArray> channels)
{
    // Worker.terminate() could be called from JS before the context is started.
    if (m_askedToTerminate)
        return;
    ++m_unconfirmedMessageCount;
    if (!isMainThread()) {
        WebWorkerBase::dispatchTaskToMainThread(createCallbackTask(&postMessageToWorkerContextTask,
                                                                   AllowCrossThreadAccess(this),
                                                                   message->toWireString(),
                                                                   channels));
        return;
    }
    WebMessagePortChannelArray webChannels(channels.get() ? channels->size() : 0);
    for (size_t i = 0; i < webChannels.size(); ++i) {
        WebMessagePortChannel* webchannel =
                        (*channels)[i]->channel()->webChannelRelease();
        webchannel->setClient(0);
        webChannels[i] = webchannel;
    }
    m_webWorker->postMessageToWorkerContext(message->toWireString(), webChannels);
}

bool WebWorkerClientImpl::hasPendingActivity() const
{
    return !m_askedToTerminate
           && (m_unconfirmedMessageCount || m_workerContextHadPendingActivity);
}

void WebWorkerClientImpl::workerObjectDestroyed()
{
    if (isMainThread()) {
        m_webWorker->workerObjectDestroyed();
        m_worker = 0;
    }
    // Even if this is called on the main thread, there could be a queued task for
    // this object, so don't delete it right away.
    WebWorkerBase::dispatchTaskToMainThread(createCallbackTask(&workerObjectDestroyedTask,
                                                               AllowCrossThreadAccess(this)));
}

void WebWorkerClientImpl::connectToInspector(WorkerContextProxy::PageInspector* pageInspector)
{
    ASSERT(!m_pageInspector);
    m_pageInspector = pageInspector;
    m_webWorker->attachDevTools();
}

void WebWorkerClientImpl::disconnectFromInspector()
{
    if (!m_askedToTerminate)
        m_webWorker->detachDevTools();
    m_pageInspector = 0;
}

void WebWorkerClientImpl::sendMessageToInspector(const String& message)
{
    m_webWorker->dispatchDevToolsMessage(message);
}

void WebWorkerClientImpl::postMessageToWorkerObject(const WebString& message,
                                                    const WebMessagePortChannelArray& channels)
{
    OwnPtr<MessagePortChannelArray> channels2;
    if (channels.size()) {
        channels2 = adoptPtr(new MessagePortChannelArray(channels.size()));
        for (size_t i = 0; i < channels.size(); ++i) {
            RefPtr<PlatformMessagePortChannel> platform_channel =
                            PlatformMessagePortChannel::create(channels[i]);
            channels[i]->setClient(platform_channel.get());
            (*channels2)[i] = MessagePortChannel::create(platform_channel);
        }
    }

    if (currentThread() != m_workerThreadId) {
        m_scriptExecutionContext->postTask(createCallbackTask(&postMessageToWorkerObjectTask,
                                                              AllowCrossThreadAccess(this),
                                                              String(message),
                                                              channels2.release()));
        return;
    }

    postMessageToWorkerObjectTask(m_scriptExecutionContext.get(), this,
                                  message, channels2.release());
}

void WebWorkerClientImpl::postExceptionToWorkerObject(const WebString& errorMessage,
                                                      int lineNumber,
                                                      const WebString& sourceURL)
{
    if (currentThread() != m_workerThreadId) {
        m_scriptExecutionContext->postTask(createCallbackTask(&postExceptionToWorkerObjectTask,
                                                              AllowCrossThreadAccess(this),
                                                              String(errorMessage),
                                                              lineNumber,
                                                              String(sourceURL)));
        return;
    }

    bool unhandled = m_worker->dispatchEvent(ErrorEvent::create(errorMessage,
                                                                sourceURL,
                                                                lineNumber));
    if (unhandled)
        m_scriptExecutionContext->reportException(errorMessage, lineNumber, sourceURL, 0);
}

void WebWorkerClientImpl::postConsoleMessageToWorkerObject(int destination,
                                                           int sourceId,
                                                           int messageType,
                                                           int messageLevel,
                                                           const WebString& message,
                                                           int lineNumber,
                                                           const WebString& sourceURL)
{
    if (currentThread() != m_workerThreadId) {
        m_scriptExecutionContext->postTask(createCallbackTask(&postConsoleMessageToWorkerObjectTask,
                                                              AllowCrossThreadAccess(this),
                                                              sourceId,
                                                              messageType,
                                                              messageLevel,
                                                              String(message),
                                                              lineNumber,
                                                              String(sourceURL)));
        return;
    }

    m_scriptExecutionContext->addMessage(static_cast<MessageSource>(sourceId),
                                         static_cast<MessageType>(messageType),
                                         static_cast<MessageLevel>(messageLevel),
                                         String(message), lineNumber,
                                         String(sourceURL), 0);
}

void WebWorkerClientImpl::postConsoleMessageToWorkerObject(int sourceId,
                                                           int messageType,
                                                           int messageLevel,
                                                           const WebString& message,
                                                           int lineNumber,
                                                           const WebString& sourceURL)
{
    postConsoleMessageToWorkerObject(0, sourceId, messageType, messageLevel, message, lineNumber, sourceURL);
}

void WebWorkerClientImpl::confirmMessageFromWorkerObject(bool hasPendingActivity)
{
    // unconfirmed_message_count_ can only be updated on the thread where it's
    // accessed.  Otherwise there are race conditions with v8's garbage
    // collection.
    m_scriptExecutionContext->postTask(createCallbackTask(&confirmMessageFromWorkerObjectTask,
                                                          AllowCrossThreadAccess(this),
                                                          hasPendingActivity));
}

void WebWorkerClientImpl::reportPendingActivity(bool hasPendingActivity)
{
    // See above comment in confirmMessageFromWorkerObject.
    m_scriptExecutionContext->postTask(createCallbackTask(&reportPendingActivityTask,
                                                          AllowCrossThreadAccess(this),
                                                          hasPendingActivity));
}

void WebWorkerClientImpl::workerContextDestroyed()
{
    InspectorInstrumentation::workerContextTerminated(m_scriptExecutionContext.get(), this);
}

void WebWorkerClientImpl::workerContextClosed()
{
}

void WebWorkerClientImpl::dispatchDevToolsMessage(const WebString& message)
{
    if (m_pageInspector)
        m_pageInspector->dispatchMessageFromWorker(message);
}

void WebWorkerClientImpl::startWorkerContextTask(ScriptExecutionContext* context,
                                                 WebWorkerClientImpl* thisPtr,
                                                 const String& scriptURL,
                                                 const String& userAgent,
                                                 const String& sourceCode)
{
    thisPtr->startWorkerContextInternal(KURL(ParsedURLString, scriptURL), userAgent, sourceCode);
}

void WebWorkerClientImpl::terminateWorkerContextTask(ScriptExecutionContext* context,
                                                     WebWorkerClientImpl* thisPtr)
{
    thisPtr->m_webWorker->terminateWorkerContext();
}

void WebWorkerClientImpl::postMessageToWorkerContextTask(ScriptExecutionContext* context,
                                                         WebWorkerClientImpl* thisPtr,
                                                         const String& message,
                                                         PassOwnPtr<MessagePortChannelArray> channels)
{
    WebMessagePortChannelArray webChannels(channels.get() ? channels->size() : 0);

    for (size_t i = 0; i < webChannels.size(); ++i) {
        webChannels[i] = (*channels)[i]->channel()->webChannelRelease();
        webChannels[i]->setClient(0);
    }

    thisPtr->m_webWorker->postMessageToWorkerContext(message, webChannels);
}

void WebWorkerClientImpl::workerObjectDestroyedTask(ScriptExecutionContext* context,
                                                    WebWorkerClientImpl* thisPtr)
{
    if (thisPtr->m_worker) // Check we haven't alread called this.
        thisPtr->m_webWorker->workerObjectDestroyed();
    delete thisPtr;
}

void WebWorkerClientImpl::postMessageToWorkerObjectTask(
                                                        ScriptExecutionContext* context,
                                                        WebWorkerClientImpl* thisPtr,
                                                        const String& message,
                                                        PassOwnPtr<MessagePortChannelArray> channels)
{

    if (thisPtr->m_worker) {
        OwnPtr<MessagePortArray> ports =
            MessagePort::entanglePorts(*context, channels);
        RefPtr<SerializedScriptValue> serializedMessage =
            SerializedScriptValue::createFromWire(message);
        thisPtr->m_worker->dispatchEvent(MessageEvent::create(ports.release(),
                                                              serializedMessage.release()));
    }
}

void WebWorkerClientImpl::postExceptionToWorkerObjectTask(
                                                          ScriptExecutionContext* context,
                                                          WebWorkerClientImpl* thisPtr,
                                                          const String& errorMessage,
                                                          int lineNumber,
                                                          const String& sourceURL)
{
    bool handled = false;
    if (thisPtr->m_worker)
        handled = thisPtr->m_worker->dispatchEvent(ErrorEvent::create(errorMessage,
                                                                      sourceURL,
                                                                      lineNumber));
    if (!handled)
        thisPtr->m_scriptExecutionContext->reportException(errorMessage, lineNumber, sourceURL, 0);
}

void WebWorkerClientImpl::postConsoleMessageToWorkerObjectTask(ScriptExecutionContext* context,
                                                               WebWorkerClientImpl* thisPtr,
                                                               int sourceId,
                                                               int messageType,
                                                               int messageLevel,
                                                               const String& message,
                                                               int lineNumber,
                                                               const String& sourceURL)
{
    thisPtr->m_scriptExecutionContext->addMessage(static_cast<MessageSource>(sourceId),
                                                  static_cast<MessageType>(messageType),
                                                  static_cast<MessageLevel>(messageLevel),
                                                  message, lineNumber, sourceURL, 0);
}

void WebWorkerClientImpl::confirmMessageFromWorkerObjectTask(ScriptExecutionContext* context,
                                                             WebWorkerClientImpl* thisPtr,
                                                             bool hasPendingActivity)
{
    thisPtr->m_unconfirmedMessageCount--;
    thisPtr->m_workerContextHadPendingActivity = hasPendingActivity;
}

void WebWorkerClientImpl::reportPendingActivityTask(ScriptExecutionContext* context,
                                                    WebWorkerClientImpl* thisPtr,
                                                    bool hasPendingActivity)
{
    thisPtr->m_workerContextHadPendingActivity = hasPendingActivity;
}

} // namespace WebKit

#endif
