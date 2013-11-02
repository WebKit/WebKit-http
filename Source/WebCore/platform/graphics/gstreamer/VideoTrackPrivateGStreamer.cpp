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

#include "config.h"

#if ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)

#include "VideoTrackPrivateGStreamer.h"

#include <glib-object.h>

namespace WebCore {

VideoTrackPrivateGStreamer::VideoTrackPrivateGStreamer(GRefPtr<GstElement> playbin, gint index, GRefPtr<GstPad> pad)
    : TrackPrivateBaseGStreamer("notify::current-video", playbin, index, pad)
{
    activeChanged();
    tagsChanged();
}

void VideoTrackPrivateGStreamer::setSelected(bool selected)
{
    if (selected == this->selected())
        return;
    VideoTrackPrivate::setSelected(selected);

    if (selected && m_playbin)
        g_object_set(m_playbin.get(), "current-video", m_index, NULL);
}

void VideoTrackPrivateGStreamer::labelChanged(const String& label)
{
    if (client())
        client()->labelChanged(this, label);
}

void VideoTrackPrivateGStreamer::languageChanged(const String& language)
{
    if (client())
        client()->languageChanged(this, language);
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
