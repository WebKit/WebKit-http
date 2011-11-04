/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#include "config.h"

#if ENABLE(VIDEO_TRACK)

#include "V8TrackEvent.h"

#include "TrackBase.h"
#include "TrackEvent.h"
#include "V8Proxy.h"
#include "V8TextTrack.h"

namespace WebCore {

v8::Handle<v8::Value> V8TrackEvent::trackAccessorGetter(v8::Local<v8::String> name, const v8::AccessorInfo& info)
{
    INC_STATS("DOM.TrackEvent.track");
    TrackEvent* trackEvent = V8TrackEvent::toNative(info.Holder());
    TrackBase* track = trackEvent->track();
    
    if (!track)
        return v8::Null();

    switch (track->type()) {
    case TrackBase::BaseTrack:
        // This should never happen.
        ASSERT_NOT_REACHED();
        break;
        
    case TrackBase::TextTrack:
        return toV8(static_cast<TextTrack*>(track));
        break;

    case TrackBase::AudioTrack:
    case TrackBase::VideoTrack:
        // This should not happen until VideoTrack and AudioTrack are implemented.
        ASSERT_NOT_REACHED();
        break;
    }

    return v8::Null();
}

} // namespace WebCore

#endif
