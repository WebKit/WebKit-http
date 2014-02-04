/*
 * Copyright (C) 2014 Haiku, Inc.
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
#include "ProgressTrackerHaiku.h"

#include "Document.h"
#include "Frame.h"
#include "NotImplemented.h"
#include "Page.h"
#include "ProgressTracker.h"
#include "WebPage.h"
#include "WebViewConstants.h"


namespace WebCore {

ProgressTrackerClientHaiku::ProgressTrackerClientHaiku(BWebPage* view)
    : m_view(view)
{
    ASSERT(m_view);
}
    
void ProgressTrackerClientHaiku::progressTrackerDestroyed()
{
    delete this;
}
    
void ProgressTrackerClientHaiku::progressStarted(Frame& originatingProgressFrame)
{
    BMessage message(LOAD_STARTED);
    message.AddString("url", originatingProgressFrame.document()->url().string());
    dispatchMessage(message);

    progressEstimateChanged(originatingProgressFrame);
#if 0
    if (!m_webFrame || m_webFrame->Frame()->tree().parent())
        return;
    triggerNavigationHistoryUpdate();
#endif
}

void ProgressTrackerClientHaiku::progressEstimateChanged(Frame& originatingProgressFrame)
{
    m_view->setLoadingProgress(originatingProgressFrame.page()->progress().estimatedProgress() * 100);

#if 0
    // Triggering this continually during loading progress makes stopping more reliably available.
    triggerNavigationHistoryUpdate();
#endif
}

void ProgressTrackerClientHaiku::progressFinished(Frame& frame)
{
    BMessage message(LOAD_DL_COMPLETED);
    message.AddString("url", frame.document()->url().string());
    dispatchMessage(message);
}

status_t ProgressTrackerClientHaiku::dispatchMessage(BMessage& message) const
{
    message.AddPointer("view", m_view->WebView());
    message.AddPointer("frame", m_view->MainFrame());
        // FIXME do we get messages from subframes as well? If so, we need to
        // find the BWebFrame matching the Frame we get in notifications.

    return m_messenger.SendMessage(&message);
}

} // namespace WebCore
