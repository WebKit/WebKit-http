/*
 * Copyright (C) 2013 Cable Television Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS'' AND ANY
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

#ifndef VideoTrackPrivateGStreamer_h
#define VideoTrackPrivateGStreamer_h

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)

#include "GRefPtrGStreamer.h"
#include "TrackPrivateBaseGStreamer.h"
#include "VideoTrackPrivate.h"

namespace WebCore {

class VideoTrackPrivateGStreamer FINAL : public VideoTrackPrivate, public TrackPrivateBaseGStreamer {
public:
    static PassRefPtr<VideoTrackPrivateGStreamer> create(GRefPtr<GstElement> playbin, gint index, GRefPtr<GstPad> pad)
    {
        return adoptRef(new VideoTrackPrivateGStreamer(playbin, index, pad));
    }

    virtual void setSelected(bool) OVERRIDE;
    virtual void setActive(bool enabled) OVERRIDE { setSelected(enabled); }

    virtual int videoTrackIndex() const OVERRIDE { return m_index; }
    virtual void labelChanged(const String&) OVERRIDE;
    virtual void languageChanged(const String&) OVERRIDE;

private:
    VideoTrackPrivateGStreamer(GRefPtr<GstElement> playbin, gint index, GRefPtr<GstPad>);
};

} // namespace WebCore

#endif // ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)

#endif // VideoTrackPrivateGStreamer_h
