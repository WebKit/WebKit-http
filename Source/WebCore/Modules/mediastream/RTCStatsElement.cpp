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

#include "RTCStatsElement.h"

#include <wtf/text/StringHash.h>

namespace WebCore {

PassRefPtr<RTCStatsElement> RTCStatsElement::create(double timestamp)
{
    return adoptRef(new RTCStatsElement(timestamp));
}

RTCStatsElement::RTCStatsElement(double timestamp)
    : m_timestamp(timestamp)
{
}

String RTCStatsElement::stat(const String& name) const
{
    return m_stats.get(name);
}

Vector<String> RTCStatsElement::names() const
{
    Vector<String> result;
    for (HashMap<String, String>::const_iterator it = m_stats.begin();
        it != m_stats.end(); ++it) {
        result.append(it->key);
    }
    return result;
}

void RTCStatsElement::addStatistic(const String& name, const String& value)
{
    m_stats.add(name, value);
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
