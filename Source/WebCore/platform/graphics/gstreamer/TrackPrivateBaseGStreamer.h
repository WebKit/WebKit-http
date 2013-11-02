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

#ifndef TrackPrivateBaseGStreamer_h
#define TrackPrivateBaseGStreamer_h

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)

#include "GRefPtrGStreamer.h"
#include <wtf/text/WTFString.h>

namespace WebCore {

class TrackPrivateBaseGStreamer {
public:
    virtual ~TrackPrivateBaseGStreamer();

    virtual void labelChanged(const String&) = 0;
    virtual void languageChanged(const String&) = 0;

    GstPad* pad() const { return m_pad.get(); }

    void disconnect();

    virtual void setActive(bool) = 0;

    void setIndex(int index) { m_index =  index; }

    void activeChanged();
    void tagsChanged();

    void notifyTrackOfActiveChanged();
    void notifyTrackOfTagsChanged();

protected:
    TrackPrivateBaseGStreamer(const char* notifyActiveSignal, GRefPtr<GstElement> playbin, gint index, GRefPtr<GstPad>);

    gint m_index;
    GRefPtr<GstElement> m_playbin;

private:
    GRefPtr<GstPad> m_pad;
    String m_label;
    String m_language;
    guint m_activeTimerHandler;
    guint m_tagTimerHandler;
};

} // namespace WebCore

#endif // ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)

#endif // TrackPrivateBaseGStreamer_h
