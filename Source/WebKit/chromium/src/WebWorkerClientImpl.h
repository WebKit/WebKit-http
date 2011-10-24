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

#ifndef WebWorkerClientImpl_h
#define WebWorkerClientImpl_h

#if ENABLE(WORKERS)

#include "ScriptExecutionContext.h"
#include "WorkerContextProxy.h"
#include "WorkerLoaderProxy.h"
#include "WorkerMessagingProxy.h"
#include "WorkerObjectProxy.h"

#include "WebFileSystem.h"
#include "WebWorkerBase.h"
#include "WebWorkerClient.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>


namespace WebKit {
class WebWorker;
class WebFrameImpl;

// This class provides chromium implementation for WorkerContextProxy, WorkerObjectProxy amd WorkerLoaderProxy
// for in-proc dedicated workers. It also acts as a bridge for workers to chromium implementation of file systems,
// databases and other related functionality.
//
// In essence, this class wraps WorkerMessagingProxy.
class WebWorkerClientImpl : public WebCore::WorkerContextProxy
                          , public WebCore::WorkerObjectProxy
                          , public NewWebWorkerBase
                          , public NewWebCommonWorkerClient {
public:
    // WebCore::WorkerContextProxy Factory.
    static WebCore::WorkerContextProxy* createWorkerContextProxy(WebCore::Worker*);

    // WebCore::WorkerContextProxy methods:
    // These are called on the thread that created the worker.  In the renderer
    // process, this will be the main WebKit thread.
    virtual void startWorkerContext(const WebCore::KURL&,
                                    const WTF::String&,
                                    const WTF::String&,
                                    WebCore::WorkerThreadStartMode);
    virtual void terminateWorkerContext();
    virtual void postMessageToWorkerContext(
        PassRefPtr<WebCore::SerializedScriptValue> message,
        PassOwnPtr<WebCore::MessagePortChannelArray> channels);
    virtual bool hasPendingActivity() const;
    virtual void workerObjectDestroyed();

#if ENABLE(INSPECTOR)
    virtual void connectToInspector(WebCore::WorkerContextProxy::PageInspector*);
    virtual void disconnectFromInspector();
    virtual void sendMessageToInspector(const String&);
    virtual void postMessageToPageInspector(const String&);
    virtual void updateInspectorStateCookie(const String&);
#endif
    // WebCore::WorkerLoaderProxy methods:
    virtual void postTaskToLoader(PassOwnPtr<WebCore::ScriptExecutionContext::Task>);
    virtual void postTaskForModeToWorkerContext(PassOwnPtr<WebCore::ScriptExecutionContext::Task>, const String& mode);

    // WebCore::WorkerObjectProxy methods:
    virtual void postMessageToWorkerObject(PassRefPtr<WebCore::SerializedScriptValue>, PassOwnPtr<WebCore::MessagePortChannelArray>);
    virtual void postExceptionToWorkerObject(const String& errorMessage, int lineNumber, const String& sourceURL);

    virtual void postConsoleMessageToWorkerObject(WebCore::MessageSource, WebCore::MessageType, WebCore::MessageLevel,
                                                  const String& message, int lineNumber, const String& sourceURL);
    virtual void confirmMessageFromWorkerObject(bool);
    virtual void reportPendingActivity(bool);
    virtual void workerContextClosed();
    virtual void workerContextDestroyed();

    // WebWorkerClientBase methods:
    virtual bool allowDatabase(WebFrame*, const WebString& name, const WebString& displayName, unsigned long estimatedSize);
    virtual bool allowFileSystem();
    virtual void openFileSystem(WebFileSystem::Type, long long size, bool create,
                                WebFileSystemCallbacks*);

    // WebCommentWorkerBase methods:
    virtual NewWebCommonWorkerClient* newCommonClient() { return this; }
    virtual WebView* view() const;

private:
    WebWorkerClientImpl(WebCore::Worker*, WebFrameImpl*);
    virtual ~WebWorkerClientImpl();

    WebCore::WorkerMessagingProxy* m_proxy;
    // Guard against context from being destroyed before a worker exits.
    RefPtr<WebCore::ScriptExecutionContext> m_scriptExecutionContext;
    WebFrameImpl* m_webFrame;
};

} // namespace WebKit;

#endif // ENABLE(WORKERS)

#endif
