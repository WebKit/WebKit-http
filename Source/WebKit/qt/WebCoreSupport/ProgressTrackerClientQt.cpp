/*
 * Copyright (C) 2016 Konstantin Tokavev <annulen@yandex.ru>
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
#include "ProgressTrackerClientQt.h"

#include "EventHandler.h"
#include "Frame.h"
#include "ProgressTracker.h"
#include "QWebFrameAdapter.h"
#include "QWebPageAdapter.h"
#include "WebEventConversion.h"

namespace WebCore {

bool ProgressTrackerClientQt::dumpProgressFinishedCallback = false;

ProgressTrackerClientQt::ProgressTrackerClientQt(QWebPageAdapter* webPageAdapter)
    : m_webPage(webPageAdapter)
{
    connect(this, SIGNAL(loadProgress(int)),
        m_webPage->handle(), SIGNAL(loadProgress(int)));
}

void ProgressTrackerClientQt::progressTrackerDestroyed()
{
    delete this;
}

void ProgressTrackerClientQt::progressStarted(Frame& originatingProgressFrame)
{
    QWebFrameAdapter* frame = QWebFrameAdapter::kit(&originatingProgressFrame);
    ASSERT(m_webPage == frame->pageAdapter);

    static_cast<FrameLoaderClientQt&>(originatingProgressFrame.loader().client()).originatingLoadStarted();

    if (!originatingProgressFrame.tree().parent())
        m_webPage->updateNavigationActions();
}

void ProgressTrackerClientQt::progressEstimateChanged(Frame& originatingProgressFrame)
{
    ASSERT(m_webPage == QWebFrameAdapter::kit(&originatingProgressFrame)->pageAdapter);
    emit loadProgress(qRound(originatingProgressFrame.page()->progress().estimatedProgress() * 100));
}

void ProgressTrackerClientQt::progressFinished(Frame& originatingProgressFrame)
{
    if (dumpProgressFinishedCallback)
        printf("postProgressFinishedNotification\n");

    QWebFrameAdapter* frame = QWebFrameAdapter::kit(&originatingProgressFrame);
    ASSERT(m_webPage == frame->pageAdapter);

    // Send a mousemove event to:
    // (1) update the cursor to change according to whatever is underneath the mouse cursor right now;
    // (2) display the tool tip if the mouse hovers a node which has a tool tip.
    QPoint localPos;
    if (frame->handleProgressFinished(&localPos)) {
        QMouseEvent event(QEvent::MouseMove, localPos, Qt::NoButton, Qt::NoButton, Qt::NoModifier);
        originatingProgressFrame.eventHandler().mouseMoved(convertMouseEvent(&event, 0));
    }
}

}
