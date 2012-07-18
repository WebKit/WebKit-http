/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 University of Szeged. All rights reserved.
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
#include "TestController.h"

#include "PlatformWebView.h"
#include "WKStringQt.h"

#include <cstdlib>
#include <QCoreApplication>
#include <QElapsedTimer>
#include <QEventLoop>
#include <QFileInfo>
#include <QLibrary>
#include <QObject>
#include <qquickwebview_p.h>
#include <QtGlobal>
#include <wtf/Platform.h>
#include <wtf/text/WTFString.h>

namespace WTR {

void TestController::notifyDone()
{
}

void TestController::platformInitialize()
{
    QQuickWebView::platformInitialize();
}

void TestController::platformRunUntil(bool& condition, double timeout)
{
    if (qgetenv("QT_WEBKIT2_DEBUG") == "1" || timeout == m_noTimeout) {
        // Never timeout if we are debugging or not meant to timeout.
        while (!condition)
            QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, 50);
        return;
    }

    int timeoutInMSecs = timeout * 1000;
    QElapsedTimer timer;
    timer.start();
    while (!condition) {
        if (timer.elapsed() > timeoutInMSecs)
            return;
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents, timeoutInMSecs - timer.elapsed());
    }
}

static bool isExistingLibrary(const QString& path)
{
#if OS(WINDOWS)
    const char* librarySuffixes[] = { ".dll" };
#elif OS(MAC_OS_X)
    const char* librarySuffixes[] = { ".bundle", ".dylib", ".so" };
#elif OS(UNIX)
    const char* librarySuffixes[] = { ".so" };
#else
#error Library path suffix should be specified for this platform
#endif
    for (unsigned i = 0; i < sizeof(librarySuffixes) / sizeof(const char*); ++i) {
        if (QLibrary::isLibrary(path + librarySuffixes[i]))
            return true;
    }

    return false;
}

void TestController::initializeInjectedBundlePath()
{
    QString path = QLatin1String(getenv("WTR_INJECTEDBUNDLE_PATH"));
    if (path.isEmpty())
        path = QFileInfo(QCoreApplication::applicationDirPath() + "/../lib/libWTRInjectedBundle").absoluteFilePath();
    if (!isExistingLibrary(path))
        qFatal("Cannot find the injected bundle at %s\n", qPrintable(path));

    m_injectedBundlePath = WKStringCreateWithQString(path);
}

void TestController::initializeTestPluginDirectory()
{
    // FIXME: the test plugin cause some trouble for us, so we don't load it for the time being.
    // See: https://bugs.webkit.org/show_bug.cgi?id=86620

    // m_testPluginDirectory = WKStringCreateWithUTF8CString(qgetenv("QTWEBKIT_PLUGIN_PATH").constData());
}

void TestController::platformInitializeContext()
{
}

void TestController::runModal(PlatformWebView* view)
{
    QEventLoop eventLoop;
    view->setModalEventLoop(&eventLoop);
    eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
}

const char* TestController::platformLibraryPathForTesting()
{
    return 0;
}

} // namespace WTR
