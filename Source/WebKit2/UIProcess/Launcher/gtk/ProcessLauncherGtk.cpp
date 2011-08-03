/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY MOTOROLA INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL MOTOROLA INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "ProcessLauncher.h"

#include "Connection.h"
#include "RunLoop.h"
#include <WebCore/FileSystem.h>
#include <WebCore/ResourceHandle.h>
#include <errno.h>
#if OS(LINUX)
#include <sys/prctl.h>
#endif
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/gobject/GOwnPtr.h>

using namespace WebCore;

namespace WebKit {

const char* gWebKitWebProcessName = "WebKitWebProcess";
const char* gWebKitPluginProcessName = "WebKitPluginProcess";

static void childSetupFunction(gpointer userData)
{
    int socket = GPOINTER_TO_INT(userData);
    close(socket);

#if OS(LINUX)
    // Kill child process when parent dies.
    prctl(PR_SET_PDEATHSIG, SIGKILL);
#endif
}

static void childFinishedFunction(GPid, gint status, gpointer userData)
{
    if (WIFEXITED(status) && !WEXITSTATUS(status))
        return;

    close(GPOINTER_TO_INT(userData));
}

void ProcessLauncher::launchProcess()
{
    GPid pid = 0;

    int sockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
        g_printerr("Creation of socket failed: %s.\n", g_strerror(errno));
        ASSERT_NOT_REACHED();
        return;
    }

    const gchar* execDirectory = g_getenv("WEBKIT_EXEC_PATH");
    GOwnPtr<gchar> binaryPath(g_build_filename(execDirectory ? execDirectory : LIBEXECDIR,
                                               m_launchOptions.processType == ProcessLauncher::WebProcess ? gWebKitWebProcessName : gWebKitPluginProcessName, NULL));
    GOwnPtr<gchar> socket(g_strdup_printf("%d", sockets[0]));
    char* argv[3];
    argv[0] = binaryPath.get();
    argv[1] = socket.get();
    argv[2] = 0;

    GOwnPtr<GError> error;
    int spawnFlags = G_SPAWN_LEAVE_DESCRIPTORS_OPEN | G_SPAWN_DO_NOT_REAP_CHILD;
    if (!g_spawn_async(0, argv, 0, static_cast<GSpawnFlags>(spawnFlags), childSetupFunction, GINT_TO_POINTER(sockets[1]), &pid, &error.outPtr())) {
        g_printerr("Unable to fork a new WebProcess: %s.\n", error->message);
        ASSERT_NOT_REACHED();
    }

    close(sockets[0]);
    m_processIdentifier = pid;

    // Monitor the child process, it calls waitpid to prevent the child process from becomming a zombie,
    // and it allows us to close the socket when the child process crashes.
    g_child_watch_add(m_processIdentifier, childFinishedFunction, GINT_TO_POINTER(sockets[1]));

    // We've finished launching the process, message back to the main run loop.
    RunLoop::main()->scheduleWork(WorkItem::create(this, &ProcessLauncher::didFinishLaunchingProcess, m_processIdentifier, sockets[1]));
}

void ProcessLauncher::terminateProcess()
{   
    if (!m_processIdentifier)
        return;

    kill(m_processIdentifier, SIGKILL);
}

void ProcessLauncher::platformInvalidate()
{
}

} // namespace WebKit
