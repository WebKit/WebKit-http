/*
    Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "PlatformVideoWindow.h"

#include "HTMLVideoElement.h"
#include "PlatformVideoWindowPrivate.h"

#include <QGuiApplication>
#include <QDesktopWidget>
#include <QKeyEvent>
#include <QPalette>
using namespace WebCore;

#ifndef QT_NO_CURSOR
static const int gHideMouseCursorDelay = 3000;
#endif

FullScreenVideoWindow::FullScreenVideoWindow()
    : QWidget(0, Qt::Window)
    , m_mediaElement(0)
{
    setAttribute(Qt::WA_NativeWindow);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_NoSystemBackground, true);
    setAttribute(Qt::WA_PaintOnScreen, true);

#ifndef QT_NO_CURSOR
    m_cursorTimer.setSingleShot(true);
    connect(&m_cursorTimer, SIGNAL(timeout()), this, SLOT(hideCursor()));
#endif
}

void FullScreenVideoWindow::setVideoElement(HTMLVideoElement* element)
{
    m_mediaElement = element;
}

void FullScreenVideoWindow::closeEvent(QCloseEvent*)
{
#ifndef QT_NO_CURSOR
    m_cursorTimer.stop();
#endif
    setMouseTracking(false);
    releaseMouse();
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
#endif
}

void FullScreenVideoWindow::keyPressEvent(QKeyEvent* ev)
{
    if (m_mediaElement && ev->key() == Qt::Key_Space) {
        if (!m_mediaElement->paused())
            m_mediaElement->pause();
        else
            m_mediaElement->play();
    } else if (ev->key() == Qt::Key_Escape)
        emit closed();
    QWidget::keyPressEvent(ev);
}

bool FullScreenVideoWindow::event(QEvent* ev)
{
    switch (ev->type()) {
    case QEvent::MouseMove:
        showCursor();
        ev->accept();
        return true;
    case QEvent::MouseButtonDblClick:
        emit closed();
        ev->accept();
        return true;
    default:
        return QWidget::event(ev);
    }
}

void FullScreenVideoWindow::showFullScreen()
{
    QWidget::showFullScreen();
    setMouseTracking(true);
    raise();
    setFocus();
    hideCursor();
}

void FullScreenVideoWindow::hideCursor()
{
#ifndef QT_NO_CURSOR
    QGuiApplication::setOverrideCursor(QCursor(Qt::BlankCursor));
#endif
}

void FullScreenVideoWindow::showCursor()
{
#ifndef QT_NO_CURSOR
    QGuiApplication::restoreOverrideCursor();
    m_cursorTimer.start(gHideMouseCursorDelay);
#endif
}


PlatformVideoWindow::PlatformVideoWindow()
{
    m_window = new FullScreenVideoWindow();
    m_window->setWindowFlags(m_window->windowFlags() | Qt::FramelessWindowHint);
    QPalette p;
    p.setColor(QPalette::Base, Qt::black);
    p.setColor(QPalette::Window, Qt::black);
    m_window->setPalette(p);
    m_window->showFullScreen();
    m_videoWindowId = m_window->winId();
}

PlatformVideoWindow::~PlatformVideoWindow()
{
    delete m_window;
    m_videoWindowId = 0;
}

void PlatformVideoWindow::prepareForOverlay(GstMessage*)
{
}
