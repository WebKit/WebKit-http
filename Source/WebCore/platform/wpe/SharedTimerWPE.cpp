/*
 * Copyright (C) 2006 Apple Inc.  All rights reserved.
 * Copyright (C) 2006 Michael Emmel mike.emmel@gmail.com
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "SharedTimer.h"

#include <wtf/gobject/GSourceWrap.h>

namespace WebCore {

static void (*sharedTimerFiredFunction)() = nullptr;
static void sharedTimerFire()
{
    sharedTimerFiredFunction();
}
static GSourceWrap::Static gSharedTimer("[WebKit] sharedTimerTimeoutCallback", std::function<void ()>(sharedTimerFire), G_PRIORITY_HIGH + 40);

void setSharedTimerFiredFunction(void (*f)())
{
    sharedTimerFiredFunction = f;
    if (!sharedTimerFiredFunction)
        gSharedTimer.cancel();
}

void setSharedTimerFireInterval(double interval)
{
    ASSERT(sharedTimerFiredFunction);

    auto intervalDuration = std::chrono::duration<double>(interval);
    auto delay = std::chrono::microseconds::max();
    if (intervalDuration < delay)
        delay = std::chrono::duration_cast<std::chrono::microseconds>(intervalDuration);
    gSharedTimer.schedule(delay);
}

void stopSharedTimer()
{
    gSharedTimer.cancel();
}

void invalidateSharedTimer()
{
}

} // namespace WebCore
