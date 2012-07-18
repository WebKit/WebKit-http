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
#include "SQLiteFileSystem.h"

#include <public/Platform.h>
#include <sqlite3.h>

#include <fcntl.h>
#include <string.h>
#include <unistd.h>

using namespace WebCore;

// Defined in Chromium's codebase in third_party/sqlite/src/os_unix.c
extern "C" {
void chromium_sqlite3_initialize_unix_sqlite3_file(sqlite3_file* file);
int chromium_sqlite3_fill_in_unix_sqlite3_file(sqlite3_vfs* vfs, int fd, int dirfd, sqlite3_file* file, const char* fileName, int noLock);
int chromium_sqlite3_get_reusable_file_handle(sqlite3_file* file, const char* fileName, int flags, int* fd);
void chromium_sqlite3_update_reusable_file_handle(sqlite3_file* file, int fd, int flags);
void chromium_sqlite3_destroy_reusable_file_handle(sqlite3_file* file);
}

// Chromium's Posix implementation of SQLite VFS
namespace {

// Opens a file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// id - the structure that will manipulate the newly opened file.
// desiredFlags - the desired open mode flags.
// usedFlags - the actual open mode flags that were used.
int chromiumOpen(sqlite3_vfs* vfs, const char* fileName,
                 sqlite3_file* id, int desiredFlags, int* usedFlags)
{
    chromium_sqlite3_initialize_unix_sqlite3_file(id);
    int fd = -1;
    int result = chromium_sqlite3_get_reusable_file_handle(id, fileName, desiredFlags, &fd);
    if (result != SQLITE_OK)
        return result;

    if (fd < 0) {
        fd = WebKit::Platform::current()->databaseOpenFile(String(fileName), desiredFlags);
        if ((fd < 0) && (desiredFlags & SQLITE_OPEN_READWRITE)) {
            int newFlags = (desiredFlags & ~(SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE)) | SQLITE_OPEN_READONLY;
            fd = WebKit::Platform::current()->databaseOpenFile(String(fileName), newFlags);
        }
    }
    if (fd < 0) {
        chromium_sqlite3_destroy_reusable_file_handle(id);
        return SQLITE_CANTOPEN;
    }

    if (usedFlags)
        *usedFlags = desiredFlags;
    chromium_sqlite3_update_reusable_file_handle(id, fd, desiredFlags);

    fcntl(fd, F_SETFD, fcntl(fd, F_GETFD) | FD_CLOEXEC);

    // The mask 0x00007F00 gives us the 7 bits that determine the type of the file SQLite is trying to open.
    int fileType = desiredFlags & 0x00007F00;
    int noLock = (fileType != SQLITE_OPEN_MAIN_DB);
    result = chromium_sqlite3_fill_in_unix_sqlite3_file(vfs, fd, -1, id, fileName, noLock);
    if (result != SQLITE_OK)
        chromium_sqlite3_destroy_reusable_file_handle(id);
    return result;
}

// Deletes the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// syncDir - determines if the directory to which this file belongs
//           should be synched after the file is deleted.
int chromiumDelete(sqlite3_vfs*, const char* fileName, int syncDir)
{
    return WebKit::Platform::current()->databaseDeleteFile(String(fileName), syncDir);
}

// Check the existance and status of the given file.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the file.
// flag - the type of test to make on this file.
// res - the result.
int chromiumAccess(sqlite3_vfs*, const char* fileName, int flag, int* res)
{
    int attr = static_cast<int>(WebKit::Platform::current()->databaseGetFileAttributes(String(fileName)));
    if (attr < 0) {
        *res = 0;
        return SQLITE_OK;
    }

    switch (flag) {
    case SQLITE_ACCESS_EXISTS:
        *res = 1;   // if the file doesn't exist, attr < 0
        break;
    case SQLITE_ACCESS_READWRITE:
        *res = (attr & W_OK) && (attr & R_OK);
        break;
    case SQLITE_ACCESS_READ:
        *res = (attr & R_OK);
        break;
    default:
        return SQLITE_ERROR;
    }

    return SQLITE_OK;
}

// Turns a relative pathname into a full pathname.
//
// vfs - pointer to the sqlite3_vfs object.
// relativePath - the relative path.
// bufSize - the size of the output buffer in bytes.
// absolutePath - the output buffer where the absolute path will be stored.
int chromiumFullPathname(sqlite3_vfs* vfs, const char* relativePath,
                         int, char* absolutePath)
{
    // The renderer process doesn't need to know the absolute path of the file
    sqlite3_snprintf(vfs->mxPathname, absolutePath, "%s", relativePath);
    return SQLITE_OK;
}

#ifndef SQLITE_OMIT_LOAD_EXTENSION
// Returns NULL, thus disallowing loading libraries in the renderer process.
//
// vfs - pointer to the sqlite3_vfs object.
// fileName - the name of the shared library file.
void* chromiumDlOpen(sqlite3_vfs*, const char*)
{
    return 0;
}
#else
#define chromiumDlOpen 0
#endif // SQLITE_OMIT_LOAD_EXTENSION

} // namespace

namespace WebCore {

void SQLiteFileSystem::registerSQLiteVFS()
{
    sqlite3_vfs* unix_vfs = sqlite3_vfs_find("unix");
    static sqlite3_vfs chromium_vfs = {
        1,
        unix_vfs->szOsFile,
        unix_vfs->mxPathname,
        0,
        "chromium_vfs",
        unix_vfs->pAppData,
        chromiumOpen,
        chromiumDelete,
        chromiumAccess,
        chromiumFullPathname,
        chromiumDlOpen,
        unix_vfs->xDlError,
        unix_vfs->xDlSym,
        unix_vfs->xDlClose,
        unix_vfs->xRandomness,
        unix_vfs->xSleep,
        unix_vfs->xCurrentTime,
        unix_vfs->xGetLastError
    };
    sqlite3_vfs_register(&chromium_vfs, 0);
}

} // namespace WebCore
