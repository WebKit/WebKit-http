/*
 * Copyright (C) 2012 Apple Inc.  All rights reserved.
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

#ifndef PlatformClockCM_h
#define PlatformClockCM_h

#if USE(COREMEDIA)

#include "Clock.h"
#include <wtf/MediaTime.h>
#include <wtf/RetainPtr.h>

typedef struct OpaqueCMTimebase* CMTimebaseRef;
typedef struct OpaqueCMClock* CMClockRef;

namespace WebCore {

class PlatformClockCM FINAL : public Clock {
public:
    PlatformClockCM();
    PlatformClockCM(CMClockRef);

    virtual void setCurrentTime(double) OVERRIDE;
    virtual double currentTime() const OVERRIDE;

    void setCurrentMediaTime(const MediaTime&);
    MediaTime currentMediaTime() const;

    virtual void setPlayRate(double) OVERRIDE;
    virtual double playRate() const OVERRIDE { return m_rate; }

    virtual void start() OVERRIDE;
    virtual void stop() OVERRIDE;
    virtual bool isRunning() const OVERRIDE { return m_running; }

private:
    void initializeWithTimingSource(CMClockRef);

    RetainPtr<CMTimebaseRef> m_timebase;
    double m_rate;
    bool m_running;
};

}

#endif

#endif
