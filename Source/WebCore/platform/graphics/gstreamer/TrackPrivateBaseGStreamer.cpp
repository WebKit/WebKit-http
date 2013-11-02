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

#include "TrackPrivateBaseGStreamer.h"

#include "GStreamerUtilities.h"
#include "Logging.h"
#include <glib-object.h>
#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

namespace WebCore {

static void trackPrivateActiveChangedCallback(GObject*, GParamSpec*, TrackPrivateBaseGStreamer* track)
{
    track->activeChanged();
}

static void trackPrivateTagsChangedCallback(GObject*, GParamSpec*, TrackPrivateBaseGStreamer* track)
{
    track->tagsChanged();
}

static gboolean trackPrivateActiveChangeTimeoutCallback(TrackPrivateBaseGStreamer* track)
{
    track->notifyTrackOfActiveChanged();
    return FALSE;
}

static gboolean trackPrivateTagsChangeTimeoutCallback(TrackPrivateBaseGStreamer* track)
{
    track->notifyTrackOfTagsChanged();
    return FALSE;
}

TrackPrivateBaseGStreamer::TrackPrivateBaseGStreamer(const char* notifyActiveSignal, GRefPtr<GstElement> playbin, gint index, GRefPtr<GstPad> pad)
    : m_index(index)
    , m_playbin(playbin)
    , m_pad(pad)
    , m_activeTimerHandler(0)
    , m_tagTimerHandler(0)
{
    ASSERT(m_pad);
    g_signal_connect(m_playbin.get(), notifyActiveSignal, G_CALLBACK(trackPrivateActiveChangedCallback), this);
    g_signal_connect(m_pad.get(), "notify::tags", G_CALLBACK(trackPrivateTagsChangedCallback), this);
}

TrackPrivateBaseGStreamer::~TrackPrivateBaseGStreamer()
{
    disconnect();
}

void TrackPrivateBaseGStreamer::disconnect()
{
    if (!m_pad)
        return;

    g_signal_handlers_disconnect_by_func(m_pad.get(),
        reinterpret_cast<gpointer>(trackPrivateActiveChangedCallback), this);
    g_signal_handlers_disconnect_by_func(m_pad.get(),
        reinterpret_cast<gpointer>(trackPrivateTagsChangedCallback), this);

    if (m_activeTimerHandler)
        g_source_remove(m_activeTimerHandler);

    if (m_tagTimerHandler)
        g_source_remove(m_tagTimerHandler);

    m_pad.clear();
    m_playbin.clear();
}

void TrackPrivateBaseGStreamer::activeChanged()
{
    if (m_activeTimerHandler)
        g_source_remove(m_activeTimerHandler);
    m_activeTimerHandler = g_timeout_add(0,
        reinterpret_cast<GSourceFunc>(trackPrivateActiveChangeTimeoutCallback), this);
}

void TrackPrivateBaseGStreamer::tagsChanged()
{
    if (m_tagTimerHandler)
        g_source_remove(m_tagTimerHandler);
    m_tagTimerHandler = g_timeout_add(0,
        reinterpret_cast<GSourceFunc>(trackPrivateTagsChangeTimeoutCallback), this);
}

void TrackPrivateBaseGStreamer::notifyTrackOfActiveChanged()
{
    if (!m_pad)
        return;

    gboolean active = false;
    if (m_pad)
        g_object_get(m_pad.get(), "active", &active, NULL);

    setActive(active);
}

void TrackPrivateBaseGStreamer::notifyTrackOfTagsChanged()
{
    m_tagTimerHandler = 0;
    if (!m_pad)
        return;

    String label;
    String language;
    GRefPtr<GstTagList> tags;
    g_object_get(m_pad.get(), "tags", &tags.outPtr(), NULL);
    if (tags) {
        gchar* tagValue;
        if (gst_tag_list_get_string(tags.get(), GST_TAG_TITLE, &tagValue)) {
            INFO_MEDIA_MESSAGE("Video track %d got title %s.", m_index, tagValue);
            label = tagValue;
            g_free(tagValue);
        }

        if (gst_tag_list_get_string(tags.get(), GST_TAG_LANGUAGE_CODE, &tagValue)) {
            INFO_MEDIA_MESSAGE("Video track %d got language %s.", m_index, tagValue);
            language = tagValue;
            g_free(tagValue);
        }
    }

    if (m_label != label) {
        m_label = label;
        labelChanged(m_label);
    }

    if (m_language != language) {
        m_language = language;
        languageChanged(m_language);
    }
}

} // namespace WebCore

#endif // ENABLE(VIDEO) && USE(GSTREAMER) && ENABLE(VIDEO_TRACK) && defined(GST_API_VERSION_1)
