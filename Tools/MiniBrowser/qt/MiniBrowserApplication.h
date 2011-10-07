/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2010 University of Szeged
 *
 * All rights reserved.
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
 */

#ifndef MiniBrowserApplication_h
#define MiniBrowserApplication_h

#include <QApplication>
#include <QFileDialog>
#include <QHash>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QMenuBar>
#include <QShortcut>
#include <QStatusBar>
#include <QStringList>
#include <QToolBar>
#include <QTouchEvent>
#include <QUrl>
#include "qwindowsysteminterface_qpa.h"

struct WindowOptions {
    WindowOptions()
        : printLoadedUrls(false)
        , useTouchWebView(false)
        , startMaximized(false)
#if defined(QT_CONFIGURED_WITH_OPENGL)
        , useQGLWidgetViewport(false)
#endif
    {
    }

    bool printLoadedUrls;
    bool useTouchWebView;
    bool startMaximized;
#if defined(QT_CONFIGURED_WITH_OPENGL)
    bool useQGLWidgetViewport;
#endif
};

class MiniBrowserApplication : public QApplication {
    Q_OBJECT

public:
    MiniBrowserApplication(int& argc, char** argv);
    QStringList urls() const { return m_urls; }
    bool isRobotized() const { return m_isRobotized; }
    int robotTimeout() const { return m_robotTimeoutSeconds; }
    int robotExtraTime() const { return m_robotExtraTimeSeconds; }

    WindowOptions m_windowOptions;

    virtual bool notify(QObject*, QEvent*);

private:
    void handleUserOptions();

private:
    bool m_realTouchEventReceived;
    int m_pendingFakeTouchEventCount;
    bool m_isRobotized;
    int m_robotTimeoutSeconds;
    int m_robotExtraTimeSeconds;
    QStringList m_urls;

    QHash<int, QWindowSystemInterface::TouchPoint> m_touchPoints;
};

#endif
