/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged
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

#include <QApplication>

#include <stdio.h>
#if !defined(NDEBUG) && defined(Q_OS_UNIX)
#include <signal.h>
#include <unistd.h>
#endif

namespace WebKit {
Q_DECL_IMPORT int WebProcessMainQt(QGuiApplication*);
#if !defined(QT_NO_WIDGETS)
Q_DECL_IMPORT void initializeWebKit2Theme();
#endif
}

#if !defined(NDEBUG) && defined(Q_OS_UNIX)
static void sigcontHandler(int)
{
}
#endif

static void messageHandler(QtMsgType type, const char* message)
{
    if (type == QtCriticalMsg) {
        fprintf(stderr, "%s\n", message);
        return;
    }

    // Do nothing
}

// The framework entry point.
// We call our platform specific entry point directly rather than WebKitMain because it makes little sense
// to reimplement the handling of command line arguments from QApplication.
int main(int argc, char** argv)
{
#if !defined(NDEBUG) && defined(Q_OS_UNIX)
    if (qgetenv("QT_WEBKIT_PAUSE_WEB_PROCESS") == "1" || qgetenv("QT_WEBKIT2_DEBUG") == "1") {
        struct sigaction newAction, oldAction;
        newAction.sa_handler = sigcontHandler;
        sigemptyset(&newAction.sa_mask);
        newAction.sa_flags = 0;
        sigaction(SIGCONT, &newAction, &oldAction);
        fprintf(stderr, "Pausing UI process, please attach to PID %d and send signal SIGCONT... ", getpid());
        pause();
        sigaction(SIGCONT, &oldAction, 0);
        fprintf(stderr, " OK\n");
    }
#endif

#if !defined(QT_NO_WIDGETS)
    WebKit::initializeWebKit2Theme();
#endif

    // Has to be done before QApplication is constructed in case
    // QApplication itself produces debug output.
    QByteArray suppressOutput = qgetenv("QT_WEBKIT_SUPPRESS_WEB_PROCESS_OUTPUT");
    if (!suppressOutput.isEmpty() && suppressOutput != "0")
        qInstallMsgHandler(messageHandler);

    return WebKit::WebProcessMainQt(new QApplication(argc, argv));
}
