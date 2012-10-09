/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "RTCStatsReport.h"

namespace WebCore {

PassRefPtr<RTCStatsReport> RTCStatsReport::create()
{
    return adoptRef(new RTCStatsReport());
}

RTCStatsReport::RTCStatsReport()
{
}

void RTCStatsReport::addElement(bool isLocal, double timestamp)
{
    if (isLocal) {
        ASSERT(!m_local);
        m_local = RTCStatsElement::create(timestamp);
    }
    ASSERT(!m_remote);
    m_remote = RTCStatsElement::create(timestamp);
}

void RTCStatsReport::addStatistic(bool isLocal, String name, String value)
{
    if (isLocal) {
        ASSERT(m_local);
        m_local->addStatistic(name, value);
    } else {
        ASSERT(m_remote);
        m_remote->addStatistic(name, value);
    }
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
