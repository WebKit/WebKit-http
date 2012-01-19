/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "AsyncFileSystem.h"

#if ENABLE(FILE_SYSTEM)

#include "AsyncFileSystemCallbacks.h"
#include "FileSystem.h"
#include "NotImplemented.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

const char AsyncFileSystem::kPersistentPathPrefix[] = "persistent";
const size_t AsyncFileSystem::kPersistentPathPrefixLength = sizeof(AsyncFileSystem::kPersistentPathPrefix) - 1;
const char AsyncFileSystem::kTemporaryPathPrefix[] = "temporary";
const size_t AsyncFileSystem::kTemporaryPathPrefixLength = sizeof(AsyncFileSystem::kTemporaryPathPrefix) - 1;

String AsyncFileSystem::toURL(const String& originString, const String& fullPath)
{
    StringBuilder result;
    result.append("filesystem:");
    result.append(originString);
    result.append("/");
    switch (type()) {
    case Temporary:
        result.append(kTemporaryPathPrefix);
        break;
    case Persistent:
        result.append(kPersistentPathPrefix);
        break;
    }
    result.append(fullPath);
    return result.toString();
}

#if !PLATFORM(CHROMIUM)
bool AsyncFileSystem::isAvailable()
{
    notImplemented();
    return false;
}

bool AsyncFileSystem::crackFileSystemURL(const KURL& url, AsyncFileSystem::Type& type, String& filePath)
{
    if (!url.protocolIs("filesystem"))
        return false;

    KURL originURL(ParsedURLString, url.path());
    String path = decodeURLEscapeSequences(originURL.path());
    if (path.isEmpty() || path[0] != '/')
        return false;
    path = path.substring(1);

    if (path.startsWith(kTemporaryPathPrefix)) {
        type = Temporary;
        path = path.substring(kTemporaryPathPrefixLength);
    } else if (path.startsWith(kPersistentPathPrefix)) {
        type = Persistent;
        path = path.substring(kPersistentPathPrefixLength);
    } else
        return false;

    if (path.isEmpty() || path[0] != '/')
        return false;

    filePath.swap(path);
    return true;
}

PassOwnPtr<AsyncFileSystem> AsyncFileSystem::create(Type, const String&)
{
    notImplemented();
    return 0;
}

void AsyncFileSystem::openFileSystem(const String& basePath, const String& storageIdentifier, Type type, bool, PassOwnPtr<AsyncFileSystemCallbacks> callbacks)
{
    notImplemented();
    callbacks->didFail(NOT_SUPPORTED_ERR);
}
#endif

} // namespace

#endif // ENABLE(FILE_SYSTEM)
