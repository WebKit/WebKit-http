/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
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
#include "LayoutTestController.h"

#include "InjectedBundle.h"
#include <glib.h>

namespace WTR {

static gboolean waitToDumpWatchdogTimerCallback(gpointer)
{
    InjectedBundle::shared().layoutTestController()->waitToDumpWatchdogTimerFired();
    return FALSE;
}

void LayoutTestController::platformInitialize()
{
    m_waitToDumpWatchdogTimer = 0;
}

void LayoutTestController::invalidateWaitToDumpWatchdogTimer()
{
    if (!m_waitToDumpWatchdogTimer)
        return;
    g_source_remove(m_waitToDumpWatchdogTimer);
    m_waitToDumpWatchdogTimer = 0;
}

void LayoutTestController::initializeWaitToDumpWatchdogTimerIfNeeded()
{
    if (m_waitToDumpWatchdogTimer)
        return;

    m_waitToDumpWatchdogTimer = g_timeout_add(waitToDumpWatchdogTimerInterval * 1000,
                                              waitToDumpWatchdogTimerCallback, 0);
}

JSRetainPtr<JSStringRef> LayoutTestController::pathToLocalResource(JSStringRef url)
{
    return url;
}

} // namespace WTR
