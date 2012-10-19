/*
 * Copyright (C) 2010, 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedWorkerProcessManager.h"

#if ENABLE(SHARED_WORKER_PROCESS)

#include "SharedWorkerProcessProxy.h"
#include "WebContext.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/WTFString.h>

namespace WebKit {

SharedWorkerProcessManager& SharedWorkerProcessManager::shared()
{
    DEFINE_STATIC_LOCAL(SharedWorkerProcessManager, manager, ());
    return manager;
}

SharedWorkerProcessManager::SharedWorkerProcessManager()
{
}

void SharedWorkerProcessManager::getSharedWorkerProcessConnection(const String& url, const String& name, PassRefPtr<Messages::WebProcessProxy::GetSharedWorkerProcessConnection::DelayedReply> reply)
{
    SharedWorkerProcessProxy* sharedWorkerProcess = getOrCreateSharedWorkerProcess(url, name);
    sharedWorkerProcess->getSharedWorkerProcessConnection(reply);
}

void SharedWorkerProcessManager::removeSharedWorkerProcessProxy(SharedWorkerProcessProxy* sharedWorkerProcessProxy)
{
    size_t vectorIndex = m_sharedWorkerProcesses.find(sharedWorkerProcessProxy);
    ASSERT(vectorIndex != notFound);

    m_sharedWorkerProcesses.remove(vectorIndex);
}

SharedWorkerProcessProxy* SharedWorkerProcessManager::getOrCreateSharedWorkerProcess(const String& url, const String& name)
{
    // FIXME: Find an existing shared worker that matches.

    RefPtr<SharedWorkerProcessProxy> sharedWorkerProcess = SharedWorkerProcessProxy::create(this, url, name);
    SharedWorkerProcessProxy* sharedWorkerProcessPtr = sharedWorkerProcess.get();

    m_sharedWorkerProcesses.append(sharedWorkerProcess.release());

    return sharedWorkerProcessPtr;
}

} // namespace WebKit

#endif // ENABLE(SHARED_WORKER_PROCESS)
