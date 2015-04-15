/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Portions Copyright (c) 2010 Motorola Mobility, Inc.  All rights reserved.
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
#include "RunLoop.h"

#include <glib.h>
#include <wtf/MainThread.h>

namespace WTF {

static GMainContext* threadDefaultContext()
{
    if (GMainContext* context = g_main_context_get_thread_default())
        return context;
    return g_main_context_default();
}

RunLoop::RunLoop()
{
    // g_main_context_default() doesn't add an extra reference.
    if (!isMainThread()) {
        m_runLoopContext = adoptGRef(g_main_context_new());
        g_main_context_push_thread_default(m_runLoopContext.get());
    } else
        m_runLoopContext = threadDefaultContext();

    ASSERT(m_runLoopContext);
    GRefPtr<GMainLoop> innermostLoop = adoptGRef(g_main_loop_new(m_runLoopContext.get(), FALSE));
    ASSERT(innermostLoop);
    m_runLoopMainLoops.append(innermostLoop);

    m_workSource.initialize("[WebKit] RunLoop work", std::bind(&RunLoop::performWork, this),
        G_PRIORITY_DEFAULT, m_runLoopContext.get());
}

RunLoop::~RunLoop()
{
    for (int i = m_runLoopMainLoops.size() - 1; i >= 0; --i) {
        if (!g_main_loop_is_running(m_runLoopMainLoops[i].get()))
            continue;
        g_main_loop_quit(m_runLoopMainLoops[i].get());
    }
}

void RunLoop::run()
{
    RunLoop& mainRunLoop = RunLoop::current();
    GMainLoop* innermostLoop = mainRunLoop.innermostLoop();
    if (!g_main_loop_is_running(innermostLoop)) {
        g_main_loop_run(innermostLoop);
        return;
    }

    // Create and run a nested loop if the innermost one was already running.
    GMainLoop* nestedMainLoop = g_main_loop_new(mainRunLoop.m_runLoopContext.get(), FALSE);
    mainRunLoop.pushNestedMainLoop(nestedMainLoop);
    g_main_loop_run(nestedMainLoop);
    mainRunLoop.popNestedMainLoop();
}

GMainLoop* RunLoop::innermostLoop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    return m_runLoopMainLoops[0].get();
}

void RunLoop::pushNestedMainLoop(GMainLoop* nestedLoop)
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    m_runLoopMainLoops.append(adoptGRef(nestedLoop));
}

void RunLoop::popNestedMainLoop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    m_runLoopMainLoops.removeLast();
}

void RunLoop::stop()
{
    // The innermost main loop should always be there.
    ASSERT(!m_runLoopMainLoops.isEmpty());
    GRefPtr<GMainLoop> lastMainLoop = m_runLoopMainLoops.last();
    if (g_main_loop_is_running(lastMainLoop.get()))
        g_main_loop_quit(lastMainLoop.get());
}

void RunLoop::wakeUp()
{
    m_workSource.schedule();
    g_main_context_wakeup(m_runLoopContext.get());
}

RunLoop::TimerBase::TimerBase(RunLoop& runLoop)
    : m_runLoop(runLoop)
    , m_fireInterval(0)
    , m_repeating(false)
    , m_timerSource("[WebKit] RunLoop::Timer", std::bind(&RunLoop::TimerBase::timerFired, this), G_PRIORITY_DEFAULT, m_runLoop.m_runLoopContext.get())
{
}

RunLoop::TimerBase::~TimerBase()
{
    stop();
}

void RunLoop::TimerBase::start(double fireInterval, bool repeating)
{
    m_fireInterval = fireInterval;
    m_repeating = repeating;
    m_timerSource.schedule(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(m_fireInterval)));
}

void RunLoop::TimerBase::stop()
{
    m_fireInterval = 0;
    m_repeating = false;
    m_timerSource.cancel();
}

bool RunLoop::TimerBase::isActive() const
{
    return m_timerSource.isActive();
}

void RunLoop::TimerBase::timerFired()
{
    fired();
    if (m_repeating)
        m_timerSource.schedule(std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::duration<double>(m_fireInterval)));
}

} // namespace WTF
