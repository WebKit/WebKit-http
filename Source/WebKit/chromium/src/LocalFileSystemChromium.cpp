/*
 * Copyright (C) 2010, 2012 Google Inc. All rights reserved.
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
#include "LocalFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "CrossThreadTask.h"
#include "Document.h"
#include "ErrorCallback.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "FileSystemType.h"
#include "WebFileError.h"
#include "WebFileSystemCallbacksImpl.h"
#include "WebFrameClient.h"
#include "WebFrameImpl.h"
#include "WebPermissionClient.h"
#include "WebViewImpl.h"
#include "WebWorkerBase.h"
#include "WorkerContext.h"
#include "WorkerFileSystemCallbacksBridge.h"
#include "WorkerThread.h"
#include <public/WebFileSystem.h>
#include <wtf/Threading.h>
#include <wtf/text/WTFString.h>

using namespace WebKit;

namespace WebCore {

LocalFileSystem& LocalFileSystem::localFileSystem()
{
    AtomicallyInitializedStatic(LocalFileSystem*, localFileSystem = new LocalFileSystem(""));
    return *localFileSystem;
}

namespace {

enum CreationFlag {
    OpenExisting,
    CreateIfNotPresent
};

#if ENABLE(WORKERS)

static const char allowFileSystemMode[] = "allowFileSystemMode";
static const char openFileSystemMode[] = "openFileSystemMode";

// This class is used to route the result of the WebWorkerBase::allowFileSystem
// call back to the worker context.
class AllowFileSystemMainThreadBridge : public ThreadSafeRefCounted<AllowFileSystemMainThreadBridge> {
public:
    static PassRefPtr<AllowFileSystemMainThreadBridge> create(WebCore::WorkerLoaderProxy* workerLoaderProxy, const String& mode, WebCommonWorkerClient* commonClient)
    {
        return adoptRef(new AllowFileSystemMainThreadBridge(workerLoaderProxy, mode, commonClient));
    }

    // These methods are invoked on the worker context.
    void cancel()
    {
        MutexLocker locker(m_mutex);
        m_workerLoaderProxy = 0;
    }

    bool result()
    {
        return m_result;
    }

    // This method is invoked on the main thread.
    void signalCompleted(const String& mode, bool result)
    {
        MutexLocker locker(m_mutex);
        if (!m_workerLoaderProxy)
            return;
        m_workerLoaderProxy->postTaskForModeToWorkerContext(
            createCallbackTask(&didComplete, AllowCrossThreadAccess(this), result), mode);
    }

private:
    AllowFileSystemMainThreadBridge(WebCore::WorkerLoaderProxy* workerLoaderProxy, const String& mode, WebCommonWorkerClient* commonClient)
        : m_result(false)
        , m_workerLoaderProxy(workerLoaderProxy)
    {
        WebWorkerBase::dispatchTaskToMainThread(createCallbackTask(&allowFileSystemTask, mode, AllowCrossThreadAccess(commonClient), this));
    }

    static void allowFileSystemTask(WebCore::ScriptExecutionContext* context, const String& mode, WebCommonWorkerClient* commonClient, PassRefPtr<AllowFileSystemMainThreadBridge> bridge)
    {
        bool allowFileSystem = commonClient ? commonClient->allowFileSystem() : false;
        bridge->signalCompleted(mode, allowFileSystem);
    }

    static void didComplete(WebCore::ScriptExecutionContext* context, PassRefPtr<AllowFileSystemMainThreadBridge> bridge, bool result)
    {
        bridge->m_result = result;
    }

    bool m_result;
    Mutex m_mutex;
    WebCore::WorkerLoaderProxy* m_workerLoaderProxy;
};

bool allowFileSystemForWorker(WebCommonWorkerClient* commonClient)
{
    WorkerScriptController* controller = WorkerScriptController::controllerForContext();
    WorkerContext* workerContext = controller->workerContext();
    WebCore::WorkerThread* workerThread = workerContext->thread();
    WorkerRunLoop& runLoop = workerThread->runLoop();
    WebCore::WorkerLoaderProxy* workerLoaderProxy =  &workerThread->workerLoaderProxy();

    // Create a unique mode just for this synchronous call.
    String mode = allowFileSystemMode;
    mode.append(String::number(runLoop.createUniqueId()));

    RefPtr<AllowFileSystemMainThreadBridge> bridge = AllowFileSystemMainThreadBridge::create(workerLoaderProxy, mode, commonClient);

    // Either the bridge returns, or the queue gets terminated.
    if (runLoop.runInMode(workerContext, mode) == MessageQueueTerminated) {
        bridge->cancel();
        return false;
    }

    return bridge->result();
}

void openFileSystemForWorker(WebCommonWorkerClient* commonClient, WebFileSystem::Type type, long long size, bool create, WebFileSystemCallbacks* callbacks, FileSystemSynchronousType synchronousType)
{
    WorkerScriptController* controller = WorkerScriptController::controllerForContext();
    WorkerContext* workerContext = controller->workerContext();
    WebCore::WorkerThread* workerThread = workerContext->thread();
    WorkerRunLoop& runLoop = workerThread->runLoop();
    WebCore::WorkerLoaderProxy* workerLoaderProxy =  &workerThread->workerLoaderProxy();

    // Create a unique mode for this openFileSystem call.
    String mode = openFileSystemMode;
    mode.append(String::number(runLoop.createUniqueId()));

    RefPtr<WorkerFileSystemCallbacksBridge> bridge = WorkerFileSystemCallbacksBridge::create(workerLoaderProxy, workerContext, callbacks);
    bridge->postOpenFileSystemToMainThread(commonClient, type, size, create, mode);

    if (synchronousType == SynchronousFileSystem) {
        if (runLoop.runInMode(workerContext, mode) == MessageQueueTerminated)
            bridge->stop();
    }
}

#endif // ENABLE(WORKERS)

} // namespace

static void openFileSystemNotAllowed(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    callbacks->didFail(WebKit::WebFileErrorAbort);
}

static void deleteFileSystemNotAllowed(ScriptExecutionContext*, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    callbacks->didFail(WebKit::WebFileErrorAbort);
}

static void openFileSystemHelper(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType, long long size, CreationFlag create)
{
    bool allowed = true;
    ASSERT(context);
    if (context->isDocument()) {
        Document* document = static_cast<Document*>(context);
        WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
        WebKit::WebViewImpl* webView = webFrame->viewImpl();
        if (webView->permissionClient() && !webView->permissionClient()->allowFileSystem(webFrame))
            allowed = false;
        else
            webFrame->client()->openFileSystem(webFrame, static_cast<WebFileSystem::Type>(type), size, create == CreateIfNotPresent, new WebFileSystemCallbacksImpl(callbacks));
    } else {
#if ENABLE(WORKERS)
        WorkerContext* workerContext = static_cast<WorkerContext*>(context);
        WorkerLoaderProxy* workerLoaderProxy = &workerContext->thread()->workerLoaderProxy();
        WebWorkerBase* webWorker = static_cast<WebWorkerBase*>(workerLoaderProxy);
        if (!allowFileSystemForWorker(webWorker->commonClient()))
            allowed = false;
        else
            openFileSystemForWorker(webWorker->commonClient(), static_cast<WebFileSystem::Type>(type), size, create == CreateIfNotPresent, new WebFileSystemCallbacksImpl(callbacks, context, synchronousType), synchronousType);

#else
        ASSERT_NOT_REACHED();
#endif
    }

    if (!allowed) {
        // The tasks are expected to be called asynchronously.
        context->postTask(createCallbackTask(&openFileSystemNotAllowed, callbacks));
    }
}

void LocalFileSystem::readFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystemHelper(context, type, callbacks, synchronousType, 0, OpenExisting);
}

void LocalFileSystem::requestFileSystem(ScriptExecutionContext* context, FileSystemType type, long long size, PassOwnPtr<AsyncFileSystemCallbacks> callbacks, FileSystemSynchronousType synchronousType)
{
    openFileSystemHelper(context, type, callbacks, synchronousType, size, CreateIfNotPresent);
}

void LocalFileSystem::deleteFileSystem(ScriptExecutionContext* context, FileSystemType type, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    ASSERT(context);
    ASSERT(context->isDocument());

    Document* document = static_cast<Document*>(context);
    WebFrameImpl* webFrame = WebFrameImpl::fromFrame(document->frame());
    WebKit::WebViewImpl* webView = webFrame->viewImpl();
    if (webView->permissionClient() && !webView->permissionClient()->allowFileSystem(webFrame)) {
        context->postTask(createCallbackTask(&deleteFileSystemNotAllowed, callbacks));
        return;
    }

    webFrame->client()->deleteFileSystem(webFrame, static_cast<WebFileSystem::Type>(type), new WebFileSystemCallbacksImpl(callbacks));
}

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
