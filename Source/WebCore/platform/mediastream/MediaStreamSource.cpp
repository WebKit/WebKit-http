/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer
 *    in the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name of Google Inc. nor the names of its contributors
 *    may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "MediaStreamSource.h"

namespace WebCore {

PassRefPtr<MediaStreamSource> MediaStreamSource::create(const String& id, Type type, const String& name, ReadyState readyState)
{
    return adoptRef(new MediaStreamSource(id, type, name, readyState));
}

MediaStreamSource::MediaStreamSource(const String& id, Type type, const String& name, ReadyState readyState)
    : m_id(id)
    , m_type(type)
    , m_name(name)
    , m_readyState(readyState)
{
}

void MediaStreamSource::setReadyState(ReadyState readyState)
{
    ASSERT(m_readyState != ReadyStateEnded);
    if (m_readyState != readyState) {
        m_readyState = readyState;
        for (Vector<Observer*>::iterator i = m_observers.begin(); i != m_observers.end(); ++i)
            (*i)->sourceChangedState();
    }
}

void MediaStreamSource::addObserver(MediaStreamSource::Observer* observer)
{
    m_observers.append(observer);
}

void MediaStreamSource::removeObserver(MediaStreamSource::Observer* observer)
{
    size_t pos = m_observers.find(observer);
    if (pos != notFound)
        m_observers.remove(pos);
}

} // namespace WebCore

#endif // ENABLE(MEDIA_STREAM)
