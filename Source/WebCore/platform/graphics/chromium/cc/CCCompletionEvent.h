/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CCCompletionEvent_h
#define CCCompletionEvent_h

#include <wtf/ThreadingPrimitives.h>

namespace WebCore {

// Used for making blocking calls from one thread to another. Use only when
// absolutely certain that doing-so will not lead to a deadlock.
//
// It is safe to destroy this object as soon as wait() returns.
class CCCompletionEvent {
public:
    CCCompletionEvent()
    {
#ifndef NDEBUG
        m_waited = false;
        m_signaled = false;
#endif
        m_mutex.lock();
    }

    ~CCCompletionEvent()
    {
        m_mutex.unlock();
        ASSERT(m_waited);
        ASSERT(m_signaled);
    }

    void wait()
    {
        ASSERT(!m_waited);
#ifndef NDEBUG
        m_waited = true;
#endif
        m_condition.wait(m_mutex);
    }

    void signal()
    {
        MutexLocker lock(m_mutex);
        ASSERT(!m_signaled);
#ifndef NDEBUG
        m_signaled = true;
#endif
        m_condition.signal();
    }

private:
    Mutex m_mutex;
    ThreadCondition m_condition;
#ifndef NDEBUG
    // Used to assert that wait() and signal() are each called exactly once.
    bool m_waited;
    bool m_signaled;
#endif
};

}

#endif
