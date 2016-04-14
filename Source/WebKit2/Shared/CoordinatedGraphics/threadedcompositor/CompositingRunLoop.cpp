/*
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
#include "CompositingRunLoop.h"

#if USE(COORDINATED_GRAPHICS_THREADED)

namespace WebKit {

CompositingRunLoop::CompositingRunLoop(std::function<void ()>&& updateFunction)
    : m_runLoop(RunLoop::current())
    , m_updateTimer(m_runLoop, this, &CompositingRunLoop::updateTimerFired)
    , m_updateFunction(WTFMove(updateFunction))
{
    m_updateState.store(UpdateState::Completed);
}

void CompositingRunLoop::callOnCompositingRunLoop(std::function<void ()>&& function)
{
    if (&m_runLoop == &RunLoop::current()) {
        function();
        return;
    }

    m_runLoop.dispatch(WTFMove(function));
}

bool CompositingRunLoop::isActive()
{
    return m_updateState.load() != UpdateState::Completed;
}

void CompositingRunLoop::scheduleUpdate()
{
    if (m_updateState.compareExchangeStrong(UpdateState::Completed, UpdateState::InProgress)) {
        m_updateTimer.startOneShot(0);
        return;
    }

    if (m_updateState.compareExchangeStrong(UpdateState::InProgress, UpdateState::PendingAfterCompletion))
        return;
}

void CompositingRunLoop::stopUpdates()
{
    m_updateTimer.stop();
    m_updateState.store(UpdateState::Completed);
}

void CompositingRunLoop::updateCompleted()
{
    if (m_updateState.compareExchangeStrong(UpdateState::InProgress, UpdateState::Completed))
        return;

    if (m_updateState.compareExchangeStrong(UpdateState::PendingAfterCompletion, UpdateState::InProgress)) {
        m_updateTimer.startOneShot(0);
        return;
    }

    ASSERT_NOT_REACHED();
}

void CompositingRunLoop::updateTimerFired()
{
    m_updateFunction();
}

} // namespace WebKit

#endif // USE(COORDINATED_GRAPHICS_THREADED)
