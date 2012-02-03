/*
 * Copyright (C) 2008 Apple Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "config.h"
#include "SuspendableTimer.h"

#include "ScriptExecutionContext.h"

namespace WebCore {

SuspendableTimer::SuspendableTimer(ScriptExecutionContext* context)
    : ActiveDOMObject(context, this)
    , m_nextFireInterval(0)
    , m_repeatInterval(0)
#if !ASSERT_DISABLED
    , m_active(false)
    , m_suspended(false)
#endif
{
}

SuspendableTimer::~SuspendableTimer()
{
}

bool SuspendableTimer::hasPendingActivity() const
{
    return isActive();
}

void SuspendableTimer::stop()
{
    TimerBase::stop();
}

void SuspendableTimer::suspend(ReasonForSuspension)
{
#if !ASSERT_DISABLED
    ASSERT(!m_suspended);
    m_suspended = true;
#endif
    m_active = isActive();
    if (m_active) {
        m_nextFireInterval = nextFireInterval();
        m_repeatInterval = repeatInterval();
        TimerBase::stop();
    }
}

void SuspendableTimer::resume()
{
#if !ASSERT_DISABLED
    ASSERT(m_suspended);
    m_suspended = false;
#endif
    if (m_active)
        start(m_nextFireInterval, m_repeatInterval);
}

bool SuspendableTimer::canSuspend() const
{
    return true;
}

} // namespace WebCore
