/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
 * Copyright (C) 2009, 2011 Google Inc. All Rights Reserved.
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
#include "WorkerContextFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystem.h"
#include "DOMFileSystem.h"
#include "DOMFileSystemBase.h"
#include "DOMFileSystemSync.h"
#include "DirectoryEntrySync.h"
#include "ErrorCallback.h"
#include "FileEntrySync.h"
#include "FileError.h"
#include "FileException.h"
#include "FileSystemCallback.h"
#include "FileSystemCallbacks.h"
#include "LocalFileSystem.h"
#include "SecurityOrigin.h"
#include "SyncCallbackHelper.h"
#include "WorkerContext.h"

namespace WebCore {

void WorkerContextFileSystem::webkitRequestFileSystem(WorkerContext* worker, int type, long long size, PassRefPtr<FileSystemCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem()) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    AsyncFileSystem::Type fileSystemType = static_cast<AsyncFileSystem::Type>(type);
    if (!AsyncFileSystem::isValidType(fileSystemType)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::INVALID_MODIFICATION_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().requestFileSystem(worker, fileSystemType, size, FileSystemCallbacks::create(successCallback, errorCallback, worker), false);
}

PassRefPtr<DOMFileSystemSync> WorkerContextFileSystem::webkitRequestFileSystemSync(WorkerContext* worker, int type, long long size, ExceptionCode& ec)
{
    ec = 0;
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem()) {
        ec = FileException::SECURITY_ERR;
        return 0;
    }

    AsyncFileSystem::Type fileSystemType = static_cast<AsyncFileSystem::Type>(type);
    if (!AsyncFileSystem::isValidType(fileSystemType)) {
        ec = FileException::INVALID_MODIFICATION_ERR;
        return 0;
    }

    FileSystemSyncCallbackHelper helper;
    LocalFileSystem::localFileSystem().requestFileSystem(worker, fileSystemType, size, FileSystemCallbacks::create(helper.successCallback(), helper.errorCallback(), worker), true);
    return helper.getResult(ec);
}

void WorkerContextFileSystem::webkitResolveLocalFileSystemURL(WorkerContext* worker, const String& url, PassRefPtr<EntryCallback> successCallback, PassRefPtr<ErrorCallback> errorCallback)
{
    KURL completedURL = worker->completeURL(url);
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem() || !secureContext->securityOrigin()->canRequest(completedURL)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::SECURITY_ERR));
        return;
    }

    AsyncFileSystem::Type type;
    String filePath;
    if (!completedURL.isValid() || !AsyncFileSystem::crackFileSystemURL(completedURL, type, filePath)) {
        DOMFileSystem::scheduleCallback(worker, errorCallback, FileError::create(FileError::ENCODING_ERR));
        return;
    }

    LocalFileSystem::localFileSystem().readFileSystem(worker, type, ResolveURICallbacks::create(successCallback, errorCallback, worker, filePath));
}

PassRefPtr<EntrySync> WorkerContextFileSystem::webkitResolveLocalFileSystemSyncURL(WorkerContext* worker, const String& url, ExceptionCode& ec)
{
    ec = 0;
    KURL completedURL = worker->completeURL(url);
    ScriptExecutionContext* secureContext = worker->scriptExecutionContext();
    if (!AsyncFileSystem::isAvailable() || !secureContext->securityOrigin()->canAccessFileSystem() || !secureContext->securityOrigin()->canRequest(completedURL)) {
        ec = FileException::SECURITY_ERR;
        return 0;
    }

    AsyncFileSystem::Type type;
    String filePath;
    if (!completedURL.isValid() || !AsyncFileSystem::crackFileSystemURL(completedURL, type, filePath)) {
        ec = FileException::ENCODING_ERR;
        return 0;
    }

    FileSystemSyncCallbackHelper readFileSystemHelper;
    LocalFileSystem::localFileSystem().readFileSystem(worker, type, FileSystemCallbacks::create(readFileSystemHelper.successCallback(), readFileSystemHelper.errorCallback(), worker), true);
    RefPtr<DOMFileSystemSync> fileSystem = readFileSystemHelper.getResult(ec);
    if (!fileSystem)
        return 0;

    RefPtr<EntrySync> entry = fileSystem->root()->getDirectory(filePath, 0, ec);
    if (ec == FileException::TYPE_MISMATCH_ERR)
        return fileSystem->root()->getFile(filePath, 0, ec);

    return entry.release();
}

COMPILE_ASSERT(static_cast<int>(WorkerContextFileSystem::TEMPORARY) == static_cast<int>(AsyncFileSystem::Temporary), enum_mismatch);
COMPILE_ASSERT(static_cast<int>(WorkerContextFileSystem::PERSISTENT) == static_cast<int>(AsyncFileSystem::Persistent), enum_mismatch);

} // namespace WebCore

#endif // ENABLE(FILE_SYSTEM)
