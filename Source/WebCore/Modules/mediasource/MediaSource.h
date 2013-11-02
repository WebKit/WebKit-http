/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
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

#ifndef MediaSource_h
#define MediaSource_h

#if ENABLE(MEDIA_SOURCE)

#include "MediaSourceBase.h"
#include "ScriptWrappable.h"
#include "SourceBuffer.h"
#include "SourceBufferList.h"
#include <wtf/RefCounted.h>

namespace WebCore {

class MediaSource : public MediaSourceBase, public ScriptWrappable {
public:
    static PassRefPtr<MediaSource> create(ScriptExecutionContext&);
    virtual ~MediaSource();

    // MediaSource.idl methods
    SourceBufferList* sourceBuffers() { return m_sourceBuffers.get(); }
    SourceBufferList* activeSourceBuffers() { return m_activeSourceBuffers.get(); }
    SourceBuffer* addSourceBuffer(const String& type, ExceptionCode&);
    void removeSourceBuffer(SourceBuffer*, ExceptionCode&);
    static bool isTypeSupported(const String& type);

    // EventTarget interface
    virtual EventTargetInterface eventTargetInterface() const OVERRIDE;

    using RefCounted<MediaSourceBase>::ref;
    using RefCounted<MediaSourceBase>::deref;

private:
    explicit MediaSource(ScriptExecutionContext&);

    // MediaSourceBase interface
    virtual void onReadyStateChange(const AtomicString&, const AtomicString&) OVERRIDE;
    virtual Vector<RefPtr<TimeRanges>> activeRanges() const OVERRIDE;

    RefPtr<SourceBufferList> m_sourceBuffers;
    RefPtr<SourceBufferList> m_activeSourceBuffers;
};

} // namespace WebCore

#endif

#endif
