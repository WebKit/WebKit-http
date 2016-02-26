/*
 *  Copyright (C) 2009, 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  Copyright (C) 2013 Collabora Ltd.
 *  Copyright (C) 2013 Orange
 *  Copyright (C) 2014 Sebastian Dröge <sebastian@centricular.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "WebKitMediaSourceGStreamer.h"

#if ENABLE(VIDEO) && ENABLE(MEDIA_SOURCE) && USE(GSTREAMER)

#include "AudioTrackPrivateGStreamer.h"
#include "GStreamerUtilities.h"
#include "MediaDescription.h"
#include "MediaPlayerPrivateGStreamerMSE.h"
#include "MediaSample.h"
#include "MediaSourceGStreamer.h"
#include "NotImplemented.h"
#include "SourceBufferPrivateGStreamer.h"
#include "TimeRanges.h"
#include "VideoTrackPrivateGStreamer.h"

#include <gst/app/app.h>
#include <gst/app/gstappsrc.h>
#include <gst/gst.h>
#include <gst/pbutils/missing-plugins.h>
#include <gst/pbutils/pbutils.h>
#include <gst/video/video.h>
#include <wtf/text/CString.h>
#include <wtf/glib/GMutexLocker.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/MainThread.h>

// To make LOG_MEDIA_MESSAGE() work outside the WebCore namespace.
#if !LOG_DISABLED
using WebCore::LogMedia;
#endif

typedef struct _Stream Stream;

struct _Stream
{
    // Fields filled when the Stream is created.
    WebKitMediaSrc* parent;

    // AppSrc
    GstElement* appsrc;
    GstPad* decodebinSinkPad;
    WebCore::SourceBufferPrivateGStreamer* sourceBuffer;

    // Fields filled when the track is attached.
    WebCore::MediaSourceStreamTypeGStreamer type;
    // Might be 0, e.g. for VP8/VP9
    GstElement *parser;
    GstCaps* caps;
#if ENABLE(VIDEO_TRACK)
    RefPtr<WebCore::AudioTrackPrivateGStreamer> audioTrack;
    RefPtr<WebCore::VideoTrackPrivateGStreamer> videoTrack;
#endif
    WebCore::FloatSize presentationSize;

    // This helps WebKitMediaSrcPrivate.appSrcNeedDataCount, ensuring that needDatas are
    // counted only once per each appsrc.
    bool appSrcNeedDataFlag;

    // Used to enforce continuity in the appended data and avoid breaking the decoder.
    MediaTime lastEnqueuedTime;

    guint notifyReadyForMoreSamplesTag;
};

enum OnSeekDataAction {
    Nothing,
    MediaSourceSeekToTime
};

struct ReleaseStreamTrackInfoTimer {
    ReleaseStreamTrackInfoTimer()
        : timer(RunLoop::main(), this, &ReleaseStreamTrackInfoTimer::fired)
    { }

    void fired() { std::exchange(callback, nullptr)(); }

    std::function<void ()> callback;
    RunLoop::Timer<ReleaseStreamTrackInfoTimer> timer;
};

struct _WebKitMediaSrcPrivate
{
    _WebKitMediaSrcPrivate()
    {
        g_mutex_init(&streamMutex);
        g_cond_init(&streamCondition);
    }

    ~_WebKitMediaSrcPrivate()
    {
        g_mutex_clear(&streamMutex);
        g_cond_clear(&streamCondition);
    }

    // Used to coordinate the release of Stream track info.
    GMutex streamMutex;
    GCond streamCondition;
    ReleaseStreamTrackInfoTimer timeoutSource;

    GList* streams;
    gchar* location;
    int nAudio;
    int nVideo;
    int nText;
    bool asyncStart;
    bool allTracksConfigured;
    unsigned numberOfPads;

    MediaTime seekTime;

    // On seek, we wait for all the seekDatas, then for all the needDatas, and then run the nextAction.
    OnSeekDataAction appSrcSeekDataNextAction;
    int appSrcSeekDataCount;
    int appSrcNeedDataCount;

    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate;
};

enum
{
    PROP_0,
    PROP_LOCATION,
    PROP_N_AUDIO,
    PROP_N_VIDEO,
    PROP_N_TEXT,
    PROP_LAST
};

enum
{
    SIGNAL_VIDEO_CHANGED,
    SIGNAL_AUDIO_CHANGED,
    SIGNAL_TEXT_CHANGED,
    LAST_SIGNAL
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src_%u", GST_PAD_SRC,
    GST_PAD_SOMETIMES, GST_STATIC_CAPS_ANY);

#define WEBKIT_MEDIA_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_SRC, WebKitMediaSrcPrivate))

GST_DEBUG_CATEGORY_STATIC(webkit_media_src_debug);
#define GST_CAT_DEFAULT webkit_media_src_debug

static void webKitMediaSrcUriHandlerInit(gpointer gIface, gpointer ifaceData);
static void webKitMediaSrcFinalize(GObject*);
static void webKitMediaSrcSetProperty(GObject*, guint propertyId, const GValue*, GParamSpec*);
static void webKitMediaSrcGetProperty(GObject*, guint propertyId, GValue*, GParamSpec*);
static GstStateChangeReturn webKitMediaSrcChangeState(GstElement*, GstStateChange);
static gboolean webKitMediaSrcQueryWithParent(GstPad*, GstObject*, GstQuery*);

inline static AtomicString getStreamTrackId(Stream*);
static gboolean freeStreamLater(Stream*);
static gboolean releaseStreamTrackInfo(WebKitMediaSrc*, Stream*);

static void app_src_need_data (GstAppSrc *src, guint length, gpointer user_data);
static void app_src_enough_data (GstAppSrc *src, gpointer user_data);
static gboolean app_src_seek_data (GstAppSrc *src, guint64 offset, gpointer user_data);

static GstAppSrcCallbacks appsrcCallbacks = {
    app_src_need_data,
    app_src_enough_data,
    app_src_seek_data,
    { 0 }
};

#define webkit_media_src_parent_class parent_class
// We split this out into another macro to avoid a check-webkit-style error.
#define WEBKIT_MEDIA_SRC_CATEGORY_INIT GST_DEBUG_CATEGORY_INIT(webkit_media_src_debug, "webkitmediasrc", 0, "websrc element");
G_DEFINE_TYPE_WITH_CODE(WebKitMediaSrc, webkit_media_src, GST_TYPE_BIN,
    G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, webKitMediaSrcUriHandlerInit);
    WEBKIT_MEDIA_SRC_CATEGORY_INIT);

static guint webkit_media_src_signals[LAST_SIGNAL] = { 0 };

static void webkit_media_src_class_init(WebKitMediaSrcClass* klass)
{
    GObjectClass* oklass = G_OBJECT_CLASS(klass);
    GstElementClass* eklass = GST_ELEMENT_CLASS(klass);

    oklass->finalize = webKitMediaSrcFinalize;
    oklass->set_property = webKitMediaSrcSetProperty;
    oklass->get_property = webKitMediaSrcGetProperty;

    gst_element_class_add_pad_template(eklass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(eklass, "WebKit Media source element", "Source", "Handles Blob uris", "Stephane Jadaud <sjadaud@sii.fr>, Sebastian Dröge <sebastian@centricular.com>");

    /* Allows setting the uri using the 'location' property, which is used
     * for example by gst_element_make_from_uri() */
    g_object_class_install_property(oklass,
        PROP_LOCATION,
        g_param_spec_string("location", "location", "Location to read from", 0,
        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property (oklass,
        PROP_N_AUDIO,
        g_param_spec_int ("n-audio", "Number Audio", "Total number of audio streams",
        0, G_MAXINT, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property (oklass,
        PROP_N_VIDEO,
        g_param_spec_int ("n-video", "Number Video", "Total number of video streams",
        0, G_MAXINT, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property (oklass,
        PROP_N_TEXT,
        g_param_spec_int ("n-text", "Number Text", "Total number of text streams",
        0, G_MAXINT, 0, (GParamFlags) (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    webkit_media_src_signals[SIGNAL_VIDEO_CHANGED] =
        g_signal_new ("video-changed", G_TYPE_FROM_CLASS (oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitMediaSrcClass, video_changed), NULL, NULL,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);
    webkit_media_src_signals[SIGNAL_AUDIO_CHANGED] =
        g_signal_new ("audio-changed", G_TYPE_FROM_CLASS (oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitMediaSrcClass, audio_changed), NULL, NULL,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);
    webkit_media_src_signals[SIGNAL_TEXT_CHANGED] =
        g_signal_new ("text-changed", G_TYPE_FROM_CLASS (oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET (WebKitMediaSrcClass, text_changed), NULL, NULL,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);

    eklass->change_state = webKitMediaSrcChangeState;

    g_type_class_add_private(klass, sizeof(WebKitMediaSrcPrivate));
}

static void webkit_media_src_init(WebKitMediaSrc* src)
{
    src->priv = WEBKIT_MEDIA_SRC_GET_PRIVATE(src);
    new (src->priv) WebKitMediaSrcPrivate();
    src->priv->seekTime = MediaTime::invalidTime();
    src->priv->appSrcSeekDataCount = 0;
    src->priv->appSrcNeedDataCount = 0;
    src->priv->appSrcSeekDataNextAction = Nothing;

    // No need to reset Stream.appSrcNeedDataFlag because there are no Streams at this point yet.
}

static void webKitMediaSrcFinalize(GObject* object)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = src->priv;

    ASSERT(WTF::isMainThread());

    for (GList* streams = src->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        streams->data = 0;
        if (stream->type != WebCore::Invalid) {
            releaseStreamTrackInfo(src, stream);
        }
        g_free(stream);
    }
    g_list_free(src->priv->streams);
    src->priv->streams = 0;

    g_free(priv->location);

    priv->seekTime = MediaTime::invalidTime();

    if (priv->mediaPlayerPrivate)
        priv->mediaPlayerPrivate = 0;

    // We used a placement new for construction, the destructor won't be called automatically.
    priv->~_WebKitMediaSrcPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitMediaSrcSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);

    switch (propId) {
    case PROP_LOCATION:
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(src), g_value_get_string(value), 0);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webKitMediaSrcGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    switch (propId) {
    case PROP_LOCATION:
        g_value_set_string(value, priv->location);
        break;
    case PROP_N_AUDIO:
        g_value_set_int(value, priv->nAudio);
        break;
    case PROP_N_VIDEO:
        g_value_set_int(value, priv->nVideo);
        break;
    case PROP_N_TEXT:
        g_value_set_int(value, priv->nText);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
    GST_OBJECT_UNLOCK(src);
}

static void webKitMediaSrcDoAsyncStart(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    priv->asyncStart = true;
    GST_BIN_CLASS(parent_class)->handle_message(GST_BIN(src),
        gst_message_new_async_start(GST_OBJECT(src)));
}

static void webKitMediaSrcDoAsyncDone(WebKitMediaSrc* src)
{
    WebKitMediaSrcPrivate* priv = src->priv;
    if (priv->asyncStart) {
        GST_BIN_CLASS(parent_class)->handle_message(GST_BIN(src),
            gst_message_new_async_done(GST_OBJECT(src), GST_CLOCK_TIME_NONE));
        priv->asyncStart = false;
    }
}

static GstStateChangeReturn webKitMediaSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(element);
    WebKitMediaSrcPrivate* priv = src->priv;

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        priv->allTracksConfigured = false;
        webKitMediaSrcDoAsyncStart(src);
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (G_UNLIKELY(ret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(src, "State change failed");
        webKitMediaSrcDoAsyncDone(src);
        return ret;
    }

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        ret = GST_STATE_CHANGE_ASYNC;
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        webKitMediaSrcDoAsyncDone(src);
        priv->allTracksConfigured = false;
        break;
    default:
        break;
    }

    return ret;
}

gint64 webKitMediaSrcGetSize(WebKitMediaSrc* webKitMediaSrc)
{
    gint64 duration = 0;
    for (GList* streams = webKitMediaSrc->priv->streams; streams; streams = streams->next) {
        Stream* s = static_cast<Stream*>(streams->data);
        duration  = MAX(duration, gst_app_src_get_size(GST_APP_SRC(s->appsrc)));
    }
    return duration;
}

static gboolean webKitMediaSrcQueryWithParent(GstPad* pad, GstObject* parent, GstQuery* query)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(GST_ELEMENT(parent));
    gboolean result = FALSE;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_DURATION: {
        GstFormat format;
        gst_query_parse_duration(query, &format, NULL);

        GST_DEBUG_OBJECT(src, "duration query in format %s", gst_format_get_name(format));
        GST_OBJECT_LOCK(src);
        switch (format) {
        case GST_FORMAT_TIME: {
            float duration;
            if (src->priv && src->priv->mediaPlayerPrivate && ((duration = src->priv->mediaPlayerPrivate->duration()) > 0)) {
                gst_query_set_duration(query, format, WebCore::toGstClockTime(duration));
                GST_DEBUG_OBJECT(src, "Answering: duration=%" GST_TIME_FORMAT, GST_TIME_ARGS(WebCore::toGstClockTime(duration)));
                result = TRUE;
            }
            break;
        }
        case GST_FORMAT_BYTES: {
            if (src->priv) {
                gint64 duration = webKitMediaSrcGetSize(src);
                if (duration) {
                    gst_query_set_duration(query, format, duration);
                    GST_DEBUG_OBJECT(src, "size: %" G_GINT64_FORMAT, duration);
                    result = TRUE;
                }
            }
            break;
        }
        default:
            break;
        }

        GST_OBJECT_UNLOCK(src);
        break;
    }
    case GST_QUERY_URI:
        GST_OBJECT_LOCK(src);
        if (src && src->priv)
            gst_query_set_uri(query, src->priv->location);
        GST_OBJECT_UNLOCK(src);
        result = TRUE;
        break;
    default:{
        GRefPtr<GstPad> target = adoptGRef(gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(pad)));
        // Forward the query to the proxy target pad.
        if (target)
            result = gst_pad_query(target.get(), query);
        break;
    }
    }

    return result;
}

static void webKitMediaSrcCheckAllTracksConfigured(WebKitMediaSrc* webKitMediaSrc);

static void webKitMediaSrcUpdatePresentationSize(GstCaps* caps, Stream* stream)
{
    GstStructure* s = gst_caps_get_structure(caps, 0);
    const gchar* structureName = gst_structure_get_name(s);
    GstVideoInfo info;

    if (g_str_has_prefix(structureName, "video/") && gst_video_info_from_caps(&info, caps)) {
        float width, height;

        // TODO: correct?
        width = info.width;
        height = info.height * ((float) info.par_d / (float) info.par_n);

        GST_OBJECT_LOCK(stream->parent);
        stream->presentationSize = WebCore::FloatSize(width, height);
        GST_OBJECT_UNLOCK(stream->parent);
    } else {
        GST_OBJECT_LOCK(stream->parent);
        stream->presentationSize = WebCore::FloatSize();
        GST_OBJECT_UNLOCK(stream->parent);
    }

    gst_caps_ref(caps);
    GST_OBJECT_LOCK(stream->parent);
    if (stream->caps)
        gst_caps_unref(stream->caps);

    stream->caps = caps;
    GST_OBJECT_UNLOCK(stream->parent);
}

static void webKitMediaSrcLinkStreamToSrcPad(GstPad* srcpad, Stream* stream)
{
    unsigned padId = static_cast<unsigned>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(srcpad), "id")));
    GST_DEBUG_OBJECT(stream->parent, "linking stream to src pad (id: %u)", padId);

    GUniquePtr<gchar> padName(g_strdup_printf("src_%u", padId));
    GstPad* ghostpad = WebCore::webkitGstGhostPadFromStaticTemplate(&srcTemplate, padName.get(), srcpad);

    gst_pad_set_query_function(ghostpad, webKitMediaSrcQueryWithParent);

    gst_pad_set_active(ghostpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(stream->parent), ghostpad);

    if (stream->decodebinSinkPad) {
        GST_DEBUG_OBJECT(stream->parent, "A decodebin was previously used for this source, trying to reuse it.");
        // TODO: error checking here. Not sure what to do if linking
        // fails though, because decodebin is out of this src
        // element's scope, in theory.
        gst_pad_link(ghostpad, stream->decodebinSinkPad);
    }
}

static void webKitMediaSrcLinkParser(GstPad* srcpad, GstCaps* caps, Stream* stream)
{
    ASSERT(caps && stream->parent);
    if (!caps || !stream->parent) {
        GST_ERROR("Unable to link parser");
        return;
    }

    webKitMediaSrcUpdatePresentationSize(caps, stream);

    // TODO: drop webKitMediaSrcLinkStreamToSrcPad() and move its code here...
    if (!gst_pad_is_linked(srcpad)) {
        GST_DEBUG_OBJECT(stream->parent, "pad not linked yet");
        webKitMediaSrcLinkStreamToSrcPad(srcpad, stream);
    }

    webKitMediaSrcCheckAllTracksConfigured(stream->parent);
}

static gboolean releaseStreamTrackInfo(WebKitMediaSrc* src, Stream* stream)
{
    GST_DEBUG("Freeing track-related info on stream %p", stream);

    WTF::GMutexLocker<GMutex> lock(src->priv->streamMutex);

    if (stream->notifyReadyForMoreSamplesTag) {
        g_source_remove(stream->notifyReadyForMoreSamplesTag);
        stream->notifyReadyForMoreSamplesTag = 0;
    }

    if (stream->caps) {
        gst_caps_unref(stream->caps);
        stream->caps = nullptr;
    }
#if ENABLE(VIDEO_TRACK)
    if (stream->audioTrack)
        stream->audioTrack = nullptr;
    if (stream->videoTrack)
        stream->videoTrack = nullptr;
#endif

    int signal = -1;
    switch (stream->type) {
    case WebCore::Audio:
        signal = SIGNAL_AUDIO_CHANGED;
        break;
    case WebCore::Video:
        signal = SIGNAL_VIDEO_CHANGED;
        break;
    case WebCore::Text:
        signal = SIGNAL_TEXT_CHANGED;
        break;
    default:
        break;
    }
    stream->type = WebCore::Invalid;

    if (signal != -1)
        g_signal_emit(G_OBJECT(src), webkit_media_src_signals[signal], 0, NULL);

    g_cond_signal(&src->priv->streamCondition);
    return G_SOURCE_REMOVE;
}

static gboolean freeStreamLater(Stream* stream)
{
    GST_DEBUG("Releasing stream: %p", stream);
    g_free(stream);

    return G_SOURCE_REMOVE;
}

static void webKitMediaSrcCheckAllTracksConfigured(WebKitMediaSrc* webKitMediaSrc)
{
    bool allTracksConfigured = false;

    GST_OBJECT_LOCK(webKitMediaSrc);
    if (!(webKitMediaSrc->priv->allTracksConfigured)) {
        allTracksConfigured = true;
        for (GList* streams = webKitMediaSrc->priv->streams; streams; streams = streams->next) {
            Stream* s = static_cast<Stream*>(streams->data);
            if (s->type == WebCore::Invalid) {
                allTracksConfigured = false;
                break;
            }
        }
        if (allTracksConfigured)
            webKitMediaSrc->priv->allTracksConfigured = true;
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    if (allTracksConfigured) {
        LOG_MEDIA_MESSAGE("All tracks attached. Completing async state change operation.");
        gst_element_no_more_pads(GST_ELEMENT(webKitMediaSrc));
        webKitMediaSrcDoAsyncDone(webKitMediaSrc);
    }
}

// uri handler interface
static GstURIType webKitMediaSrcUriGetType(GType)
{
    return GST_URI_SRC;
}

const gchar* const* webKitMediaSrcGetProtocols(GType)
{
    static const char* protocols[] = {"mediasourceblob", 0 };
    return protocols;
}

static gchar* webKitMediaSrcGetUri(GstURIHandler* handler)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(handler);
    gchar* ret;

    GST_OBJECT_LOCK(src);
    ret = g_strdup(src->priv->location);
    GST_OBJECT_UNLOCK(src);
    return ret;
}

static gboolean webKitMediaSrcSetUri(GstURIHandler* handler, const gchar* uri, GError**)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(handler);
    WebKitMediaSrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GST_OBJECT_LOCK(src);
    g_free(priv->location);
    priv->location = 0;
    if (!uri) {
        GST_OBJECT_UNLOCK(src);
        return TRUE;
    }

    WebCore::URL url(WebCore::URL(), uri);

    priv->location = g_strdup(url.string().utf8().data());
    GST_OBJECT_UNLOCK(src);
    return TRUE;
}
static void webKitMediaSrcUriHandlerInit(gpointer gIface, gpointer)
{
    GstURIHandlerInterface* iface = (GstURIHandlerInterface *) gIface;

    iface->get_type = webKitMediaSrcUriGetType;
    iface->get_protocols = webKitMediaSrcGetProtocols;
    iface->get_uri = webKitMediaSrcGetUri;
    iface->set_uri = webKitMediaSrcSetUri;
}

inline static AtomicString getStreamTrackId(Stream* stream)
{
    if (stream->audioTrack)
        return stream->audioTrack->id();
    if (stream->videoTrack)
        return stream->videoTrack->id();
    GST_DEBUG("Stream has no audio and no video track");
    return AtomicString();
}

static Stream* getStreamByTrackId(WebKitMediaSrc* src, AtomicString trackIDString)
{
    // WebKitMediaSrc should be locked at this point.
    for (GList* streams = src->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        if (stream->type != WebCore::Invalid && (
            (stream->audioTrack && stream->audioTrack->id() == trackIDString) ||
            (stream->videoTrack && stream->videoTrack->id() == trackIDString) ) )
            return stream;
    }
    return NULL;
}

static Stream* getStreamBySourceBufferPrivate(WebKitMediaSrc* src, WebCore::SourceBufferPrivateGStreamer* sourceBufferPrivate)
{
    for (GList* streams = src->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        if (stream->sourceBuffer == sourceBufferPrivate)
            return stream;
    }
    return NULL;
}

static Stream* getStreamByAppSrc(WebKitMediaSrc* src, GstElement* appsrc)
{
    for (GList* streams = src->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        if (stream->appsrc == appsrc)
            return stream;
    }
    return NULL;
}

static gboolean notifyReadyForMoreSamples(gpointer user_data)
{
    Stream* stream = static_cast<Stream*>(user_data);
    WebKitMediaSrc* webKitMediaSrc = WEBKIT_MEDIA_SRC(stream->parent);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate = webKitMediaSrc->priv->mediaPlayerPrivate;
    if (mediaPlayerPrivate && !mediaPlayerPrivate->seeking()) {
        stream->sourceBuffer->notifyReadyForMoreSamples();
    }

    GST_OBJECT_LOCK(webKitMediaSrc);
    stream->notifyReadyForMoreSamplesTag = 0;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    return G_SOURCE_REMOVE;
}

static gboolean seekNeedsDataMainThread (gpointer user_data)
{
    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(user_data);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    LOG_MEDIA_MESSAGE("Buffering needed before seek");

    ASSERT(WTF::isMainThread());

    GST_OBJECT_LOCK(webKitMediaSrc);
    MediaTime seekTime = webKitMediaSrc->priv->seekTime;
    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate = webKitMediaSrc->priv->mediaPlayerPrivate;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    if (mediaPlayerPrivate==nullptr)
        return G_SOURCE_REMOVE;

    for (GList* streams = webKitMediaSrc->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        if (stream->type != WebCore::Invalid) {
            stream->sourceBuffer->setReadyForMoreSamples(true);
        }
    }

    mediaPlayerPrivate->notifySeekNeedsData(seekTime);

    return G_SOURCE_REMOVE;
}

static void app_src_need_data (GstAppSrc *src, guint length, gpointer user_data)
{
    UNUSED_PARAM(length);

    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(user_data);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    bool allAppSrcsNeedDataAfterSeek = false;

    GST_OBJECT_LOCK(webKitMediaSrc);
    OnSeekDataAction appSrcSeekDataNextAction = webKitMediaSrc->priv->appSrcSeekDataNextAction;
    int numAppSrcs = g_list_length(webKitMediaSrc->priv->streams);
    Stream* appSrcStream = getStreamByAppSrc(webKitMediaSrc, GST_ELEMENT(src));

    if (webKitMediaSrc->priv->appSrcSeekDataCount > 0) {
        if (appSrcStream && !appSrcStream->appSrcNeedDataFlag) {
            ++webKitMediaSrc->priv->appSrcNeedDataCount;
            appSrcStream->appSrcNeedDataFlag = true;
        }
        if (webKitMediaSrc->priv->appSrcSeekDataCount == numAppSrcs && webKitMediaSrc->priv->appSrcNeedDataCount == numAppSrcs) {
            LOG_MEDIA_MESSAGE("All need_datas completed");
            allAppSrcsNeedDataAfterSeek = true;
            webKitMediaSrc->priv->appSrcSeekDataCount = 0;
            webKitMediaSrc->priv->appSrcNeedDataCount = 0;
            webKitMediaSrc->priv->appSrcSeekDataNextAction = Nothing;

            for (GList* streams = webKitMediaSrc->priv->streams; streams; streams = streams->next) {
                Stream* stream = static_cast<Stream*>(streams->data);
                stream->appSrcNeedDataFlag = false;
            }
        }
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    if (allAppSrcsNeedDataAfterSeek) {
        LOG_MEDIA_MESSAGE("All expected app_src_seek_data() and app_src_need_data() calls performed. Running next action (%d)", static_cast<int>(appSrcSeekDataNextAction));

        switch (appSrcSeekDataNextAction) {
        case MediaSourceSeekToTime:
            if (WTF::isMainThread())
                seekNeedsDataMainThread(user_data);
            else
                g_timeout_add(0, GSourceFunc(seekNeedsDataMainThread), user_data);
            break;
        case Nothing:
            break;
        }
    } else if (appSrcSeekDataNextAction == Nothing) {
        WTF::GMutexLocker<GMutex> lock(webKitMediaSrc->priv->streamMutex);

        GST_OBJECT_LOCK(webKitMediaSrc);
        Stream* stream = getStreamByAppSrc(webKitMediaSrc, GST_ELEMENT(src));
        if (stream && stream->notifyReadyForMoreSamplesTag == 0 && stream->type != WebCore::Invalid) {
            stream->notifyReadyForMoreSamplesTag =
                g_timeout_add(0, GSourceFunc(notifyReadyForMoreSamples), stream);
        }
        GST_OBJECT_UNLOCK(webKitMediaSrc);
    }
}

static void app_src_enough_data (GstAppSrc *src, gpointer user_data)
{
    ASSERT(WTF::isMainThread());
    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(user_data);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));
    Stream* stream = getStreamByAppSrc(webKitMediaSrc, GST_ELEMENT(src));
    ASSERT(stream != NULL);
    ASSERT(stream->type != WebCore::Invalid);
    stream->sourceBuffer->setReadyForMoreSamples(false);
}

static gboolean app_src_seek_data (GstAppSrc *src, guint64 offset, gpointer user_data)
{
    UNUSED_PARAM(src);
    UNUSED_PARAM(offset);

    ASSERT(WTF::isMainThread());

    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(user_data);

    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    GST_OBJECT_LOCK(webKitMediaSrc);
    webKitMediaSrc->priv->appSrcSeekDataCount++;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    return TRUE;
}

// METRO FIXME: Use gst_app_src_push_sample() instead when we switch to the appropriate GStreamer version.
static GstFlowReturn push_sample(GstAppSrc* appsrc, GstSample* sample)
{
    GstBuffer *buffer;
    GstCaps *caps;

    g_return_val_if_fail (GST_IS_SAMPLE (sample), GST_FLOW_ERROR);

    caps = gst_sample_get_caps (sample);
    if (caps != NULL) {
      gst_app_src_set_caps (appsrc, caps);
    } else {
      GST_WARNING_OBJECT (appsrc, "received sample without caps");
    }

    buffer = gst_sample_get_buffer (sample);
    if (buffer == NULL) {
      GST_WARNING_OBJECT (appsrc, "received sample without buffer");
      return GST_FLOW_OK;
    }

    // gst_app_src_push_buffer() steals the reference, we need an additional one.
    gst_buffer_ref(buffer);
    return gst_app_src_push_buffer (appsrc, buffer);
}

namespace WebCore {

PassRefPtr<PlaybackPipeline> PlaybackPipeline::create()
{
    return adoptRef(new PlaybackPipeline());
}

PlaybackPipeline::PlaybackPipeline()
    : RefCounted<PlaybackPipeline>()
{
}

PlaybackPipeline::~PlaybackPipeline()
{
}

void PlaybackPipeline::setWebKitMediaSrc(WebKitMediaSrc* webKitMediaSrc)
{
    LOG_MEDIA_MESSAGE("webKitMediaSrc=%p", webKitMediaSrc);
    if (webKitMediaSrc)
        gst_object_ref(webKitMediaSrc);

    m_webKitMediaSrc = adoptGRef(static_cast<WebKitMediaSrc*>(webKitMediaSrc));
}

WebKitMediaSrc* PlaybackPipeline::webKitMediaSrc()
{
    return m_webKitMediaSrc.get();
}

MediaSourcePrivate::AddStatus PlaybackPipeline::addSourceBuffer(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate)
{
    WebKitMediaSrcPrivate* priv = m_webKitMediaSrc->priv;

    if (priv->allTracksConfigured) {
        GST_ERROR_OBJECT(m_webKitMediaSrc.get(), "Adding new source buffers after first data not supported yet");
        return MediaSourcePrivate::NotSupported;
    }

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "State %d", (int)GST_STATE(m_webKitMediaSrc.get()));

    Stream* stream = g_new0(Stream, 1);
    stream->parent = m_webKitMediaSrc.get();
    stream->appsrc = gst_element_factory_make("appsrc", nullptr);
    stream->appSrcNeedDataFlag = false;
    stream->sourceBuffer = sourceBufferPrivate.get();
    stream->notifyReadyForMoreSamplesTag = 0;

    // No track has been attached yet.
    stream->type = Invalid;
    stream->parser = nullptr;
    stream->caps = nullptr;
#if ENABLE(VIDEO_TRACK)
    stream->audioTrack = nullptr;
    stream->videoTrack = nullptr;
#endif
    stream->presentationSize = WebCore::FloatSize();
    stream->lastEnqueuedTime = MediaTime::invalidTime();

    gst_app_src_set_callbacks(GST_APP_SRC(stream->appsrc), &appsrcCallbacks, stream->parent, 0);
    gst_app_src_set_emit_signals(GST_APP_SRC(stream->appsrc), FALSE);
    gst_app_src_set_stream_type(GST_APP_SRC(stream->appsrc), GST_APP_STREAM_TYPE_SEEKABLE);

    gst_app_src_set_max_bytes(GST_APP_SRC(stream->appsrc), 2 * WTF::MB);
    g_object_set(G_OBJECT(stream->appsrc),
                 "block", FALSE,
                 "min-percent", 20, NULL);

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    priv->streams = g_list_prepend(priv->streams, stream);
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    gst_bin_add(GST_BIN(m_webKitMediaSrc.get()), stream->appsrc);
    gst_element_sync_state_with_parent(stream->appsrc);

    return MediaSourcePrivate::Ok;
}

void PlaybackPipeline::removeSourceBuffer(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate)
{
    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Element removed from MediaSource");
    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    WebKitMediaSrcPrivate* priv = m_webKitMediaSrc->priv;
    Stream* stream = 0;
    GList *l;

    for (l = priv->streams; l; l = l->next) {
        Stream *tmp = static_cast<Stream*>(l->data);
        if (tmp->sourceBuffer == sourceBufferPrivate.get()) {
            stream = tmp;
            priv->streams = g_list_remove(priv->streams, stream);
            break;
        }
    }
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (stream) {
        if (stream->appsrc)
            gst_app_src_end_of_stream(GST_APP_SRC(stream->appsrc));

        if (stream->type != Invalid) {
            if (WTF::isMainThread())
                releaseStreamTrackInfo(stream->parent, stream);
            else {
                WTF::GMutexLocker<GMutex> lock(stream->parent->priv->streamMutex);
                GRefPtr<WebKitMediaSrc> protector(stream->parent);
                stream->parent->priv->timeoutSource.callback = [protector, stream] { releaseStreamTrackInfo(protector.get(), stream); };
                stream->parent->priv->timeoutSource.timer.startOneShot(0);

                g_cond_wait(&stream->parent->priv->streamCondition, &stream->parent->priv->streamMutex);
            }
        }

        g_timeout_add(300, (GSourceFunc)freeStreamLater, stream);
    }
}

void PlaybackPipeline::attachTrack(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, RefPtr<TrackPrivateBase> trackPrivate, GstStructure* s, GstCaps* caps)
{
    WebKitMediaSrc* webKitMediaSrc = m_webKitMediaSrc.get();
    Stream* stream = 0;
    gchar* parserBinName;
    unsigned padId = 0;
    const gchar* mediaType = gst_structure_get_name(s);

    GST_OBJECT_LOCK(webKitMediaSrc);
    stream = getStreamBySourceBufferPrivate(webKitMediaSrc, sourceBufferPrivate.get());
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    ASSERT(stream != 0);

    GST_OBJECT_LOCK(webKitMediaSrc);
    padId = stream->parent->priv->numberOfPads;
    stream->parent->priv->numberOfPads++;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    parserBinName = g_strdup_printf("streamparser%u", padId);

    stream->parser = gst_bin_new(parserBinName);
    g_free(parserBinName);

    GST_DEBUG_OBJECT(webKitMediaSrc, "Configured track %s: appsrc=%s, padId=%u, mediaType=%s", trackPrivate->id().string().utf8().data(), GST_ELEMENT_NAME(stream->appsrc), padId, mediaType);

    if (!g_strcmp0(mediaType, "video/x-h264")) {
        GstElement* parser;
        GstElement* capsfilter;
        GstPad* pad = nullptr;
        GstCaps* filtercaps;

        filtercaps = gst_caps_new_simple("video/x-h264", "alignment", G_TYPE_STRING, "au", NULL);
        parser = gst_element_factory_make("h264parse", 0);
        capsfilter = gst_element_factory_make("capsfilter", 0);
        g_object_set(capsfilter, "caps", filtercaps, NULL);
        gst_caps_unref(filtercaps);

        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, NULL);
        gst_element_link_pads(parser, "src", capsfilter, "sink");

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(capsfilter, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else if (!g_strcmp0(mediaType, "video/x-h265")) {
        GstElement* parser;
        GstElement* capsfilter;
        GstPad* pad = nullptr;
        GstCaps* filtercaps;

        filtercaps = gst_caps_new_simple("video/x-h265", "alignment", G_TYPE_STRING, "au", NULL);
        parser = gst_element_factory_make("h265parse", 0);
        capsfilter = gst_element_factory_make("capsfilter", 0);
        g_object_set(capsfilter, "caps", filtercaps, NULL);
        gst_caps_unref(filtercaps);

        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, NULL);
        gst_element_link_pads(parser, "src", capsfilter, "sink");

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(capsfilter, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else if (!g_strcmp0(mediaType, "audio/mpeg")) {
        gint mpegversion = -1;
        GstElement* parser = nullptr;
        GstPad* pad = nullptr;

        gst_structure_get_int(s, "mpegversion", &mpegversion);
        if (mpegversion == 1) {
            parser = gst_element_factory_make("mpegaudioparse", 0);
        } else if (mpegversion == 2 || mpegversion == 4) {
            parser = gst_element_factory_make("aacparse", 0);
            //g_object_set(parser, "disable-passthrough", TRUE, nullptr);
        } else {
            ASSERT_NOT_REACHED();
        }

        gst_bin_add(GST_BIN(stream->parser), parser);

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(parser, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else {
        GST_ERROR_OBJECT(stream->parent, "Unsupported media format: %s", mediaType);
        gst_object_unref(GST_OBJECT(stream->parser));
        return;
    }

    GST_OBJECT_LOCK(webKitMediaSrc);
    stream->type = Unknown;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    ASSERT(stream->parser);
    gst_bin_add(GST_BIN(stream->parent), stream->parser);
    gst_element_sync_state_with_parent(stream->parser);

    GstPad* sinkpad = gst_element_get_static_pad(stream->parser, "sink");
    GstPad* srcpad = gst_element_get_static_pad(stream->appsrc, "src");
    gst_pad_link(srcpad, sinkpad);
    gst_object_unref(srcpad);
    srcpad = 0;
    gst_object_unref(sinkpad);
    sinkpad = 0;

    srcpad = gst_element_get_static_pad(stream->parser, "src");
    // TODO: Is padId the best way to identify the Stream? What about trackId?
    g_object_set_data(G_OBJECT(srcpad), "id", GINT_TO_POINTER(padId));
    webKitMediaSrcLinkParser(srcpad, caps, stream);

    ASSERT(stream->parent->priv->mediaPlayerPrivate);
    int signal = -1;
    if (g_str_has_prefix(mediaType, "audio")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Audio;
        stream->parent->priv->nAudio++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_AUDIO_CHANGED;

        stream->audioTrack = RefPtr<WebCore::AudioTrackPrivateGStreamer>(static_cast<WebCore::AudioTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "video")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Video;
        stream->parent->priv->nVideo++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_VIDEO_CHANGED;

        stream->videoTrack = RefPtr<WebCore::VideoTrackPrivateGStreamer>(static_cast<WebCore::VideoTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "text")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Text;
        stream->parent->priv->nText++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_TEXT_CHANGED;

        // TODO: Support text tracks.
    }

    if (signal != -1)
        g_signal_emit(G_OBJECT(stream->parent), webkit_media_src_signals[signal], 0, NULL);

    gst_object_unref(srcpad);
    srcpad = 0;
}

void PlaybackPipeline::reattachTrack(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, RefPtr<TrackPrivateBase> trackPrivate)
{
    LOG_MEDIA_MESSAGE("Re-attaching track");

    // TODO: Maybe remove this method.
    // Now the caps change is managed by gst_appsrc_push_sample()
    // in enqueueSample() and flushAndEnqueueNonDisplayingSamples().

    WebKitMediaSrc* webKitMediaSrc = m_webKitMediaSrc.get();

    GST_OBJECT_LOCK(webKitMediaSrc);
    Stream* stream = getStreamBySourceBufferPrivate(webKitMediaSrc, sourceBufferPrivate.get());
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    ASSERT(stream != 0);
    ASSERT(stream->type != Invalid);

    GstCaps* oldAppsrccaps = gst_app_src_get_caps(GST_APP_SRC(stream->appsrc));
    // Now the caps change is managed by gst_appsrc_push_sample()
    // in enqueueSample() and flushAndEnqueueNonDisplayingSamples().
    // gst_app_src_set_caps(GST_APP_SRC(stream->appsrc), caps);
    GstCaps* appsrccaps = gst_app_src_get_caps(GST_APP_SRC(stream->appsrc));
    const gchar* mediaType = gst_structure_get_name(gst_caps_get_structure(appsrccaps, 0));

    if (!gst_caps_is_equal(oldAppsrccaps, appsrccaps)) {
        LOG_MEDIA_MESSAGE("Caps have changed, but reconstructing the sequence of elements is not supported yet");

        gchar* stroldcaps = gst_caps_to_string(oldAppsrccaps);
        gchar* strnewcaps = gst_caps_to_string(appsrccaps);
        LOG_MEDIA_MESSAGE("oldcaps: %s", stroldcaps);
        LOG_MEDIA_MESSAGE("newcaps: %s", strnewcaps);
        g_free(stroldcaps);
        g_free(strnewcaps);
    }

    int signal = -1;

    GST_OBJECT_LOCK(webKitMediaSrc);
    if (g_str_has_prefix(mediaType, "audio")) {
        ASSERT(stream->type == Audio);
        signal = SIGNAL_AUDIO_CHANGED;
        stream->audioTrack = RefPtr<WebCore::AudioTrackPrivateGStreamer>(static_cast<WebCore::AudioTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "video")) {
        ASSERT(stream->type == Video);
        signal = SIGNAL_VIDEO_CHANGED;
        stream->videoTrack = RefPtr<WebCore::VideoTrackPrivateGStreamer>(static_cast<WebCore::VideoTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "text")) {
        ASSERT(stream->type == Text);
        signal = SIGNAL_TEXT_CHANGED;

        // TODO: Support text tracks and mediaTypes related to EME
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    gst_caps_unref(appsrccaps);
    gst_caps_unref(oldAppsrccaps);

    if (signal != -1)
        g_signal_emit(G_OBJECT(stream->parent), webkit_media_src_signals[signal], 0, NULL);
}

void PlaybackPipeline::notifyDurationChanged()
{
    gst_element_post_message(GST_ELEMENT(m_webKitMediaSrc.get()), gst_message_new_duration_changed(GST_OBJECT(m_webKitMediaSrc.get())));
    // WebKitMediaSrc will ask MediaPlayerPrivateGStreamerMSE for the new duration later, when somebody asks for it.
}

void PlaybackPipeline::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus)
{
    WebKitMediaSrcPrivate* priv = m_webKitMediaSrc->priv;
    GList *l;

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Have EOS");

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    bool allTracksConfigured = priv->allTracksConfigured;
    if (!allTracksConfigured) {
        priv->allTracksConfigured = true;
    }
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (!allTracksConfigured) {
        gst_element_no_more_pads(GST_ELEMENT(m_webKitMediaSrc.get()));
        webKitMediaSrcDoAsyncDone(m_webKitMediaSrc.get());
    }

    Vector<GstAppSrc*> appSrcs;

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    for (l = priv->streams; l; l = l->next) {
        Stream *stream = static_cast<Stream*>(l->data);
        if (stream->appsrc)
            appSrcs.append(GST_APP_SRC(stream->appsrc));
    }
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    for (Vector<GstAppSrc*>::iterator it = appSrcs.begin(); it != appSrcs.end(); ++it)
        gst_app_src_end_of_stream(*it);
}

void PlaybackPipeline::flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample> > samples)
{
    ASSERT(WTF::isMainThread());

    if (samples.size() == 0) {
        LOG_MEDIA_MESSAGE("No samples, trackId unknown");
        return;
    }

    AtomicString trackId = samples[0]->trackID();
    LOG_MEDIA_MESSAGE("flushAndEnqueueNonDisplayingSamples: trackId=%s PTS[0]=%f ... PTS[n]=%f", trackId.string().utf8().data(), samples[0]->presentationTime().toFloat(), samples[samples.size()-1]->presentationTime().toFloat());

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Flushing and re-enqueing %d samples for stream %s", samples.size(), trackId.string().utf8().data());

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    Stream* stream = getStreamByTrackId(m_webKitMediaSrc.get(), trackId);

    if (!stream) {
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    if (!stream->sourceBuffer->isReadyForMoreSamples(trackId)) {
        LOG_MEDIA_MESSAGE("flushAndEnqueueNonDisplayingSamples: skip adding new sample for trackId=%s, SB is not ready yet", trackId.string().utf8().data());
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    GstElement* appsrc = stream->appsrc;
    MediaTime lastEnqueuedTime = stream->lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (!m_webKitMediaSrc->priv->mediaPlayerPrivate->seeking()) {
        LOG_MEDIA_MESSAGE("flushAndEnqueueNonDisplayingSamples: trackId=%s pipeline needs flushing.", trackId.string().utf8().data());
    }

    for (Vector<RefPtr<MediaSample> >::iterator it = samples.begin(); it != samples.end(); ++it) {
        GStreamerMediaSample* sample = static_cast<GStreamerMediaSample*>(it->get());
        GstBuffer* buffer = nullptr;
        if (sample->sample())
            buffer = gst_sample_get_buffer(sample->sample());
        if (buffer) {
            GstSample* gstsample = gst_sample_ref(sample->sample());
            lastEnqueuedTime = sample->presentationTime();

            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY);
            push_sample(GST_APP_SRC(appsrc), gstsample);
            // gst_app_src_push_sample() uses transfer-none for gstsample

            gst_sample_unref(gstsample);
        }
    }
    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    stream->lastEnqueuedTime = lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
}

void PlaybackPipeline::enqueueSample(PassRefPtr<MediaSample> prsample)
{
    RefPtr<MediaSample> rsample = prsample;
    AtomicString trackId = rsample->trackID();

    TRACE_MEDIA_MESSAGE("enqueing sample trackId=%s PTS=%f presentationSize=%.0fx%.0f at %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT, trackId.string().utf8().data(), rsample->presentationTime().toFloat(), rsample->presentationSize().width(), rsample->presentationSize().height(), GST_TIME_ARGS(WebCore::toGstClockTime(rsample->presentationTime().toDouble())), GST_TIME_ARGS(WebCore::toGstClockTime(rsample->duration().toDouble())));
    ASSERT(WTF::isMainThread());

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    Stream* stream = getStreamByTrackId(m_webKitMediaSrc.get(), trackId);

    if (!stream) {
        WARN_MEDIA_MESSAGE("No stream!");
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    if (!stream->sourceBuffer->isReadyForMoreSamples(trackId)) {
        LOG_MEDIA_MESSAGE("enqueueSample: skip adding new sample for trackId=%s, SB is not ready yet", trackId.string().utf8().data());
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    GstElement* appsrc = stream->appsrc;
    MediaTime lastEnqueuedTime = stream->lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    GStreamerMediaSample* sample = static_cast<GStreamerMediaSample*>(rsample.get());
    if (sample->sample() && gst_sample_get_buffer(sample->sample())) {
        GstSample* gstsample = gst_sample_ref(sample->sample());
        GstBuffer* buffer = gst_sample_get_buffer(gstsample);
        lastEnqueuedTime = sample->presentationTime();

        GST_BUFFER_FLAG_UNSET(buffer, GST_BUFFER_FLAG_DECODE_ONLY);
        push_sample(GST_APP_SRC(appsrc), gstsample);
        // gst_app_src_push_sample() uses transfer-none for gstsample

        gst_sample_unref(gstsample);

        GST_OBJECT_LOCK(m_webKitMediaSrc.get());
        stream->lastEnqueuedTime = lastEnqueuedTime;
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
    }
}

GstElement* PlaybackPipeline::pipeline()
{
    if (!m_webKitMediaSrc || !GST_ELEMENT_PARENT(GST_ELEMENT(m_webKitMediaSrc.get())))
        return nullptr;

    return GST_ELEMENT_PARENT(GST_ELEMENT_PARENT(GST_ELEMENT(m_webKitMediaSrc.get())));
}

};

void webkit_media_src_set_mediaplayerprivate(WebKitMediaSrc* src, WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate)
{
    GST_OBJECT_LOCK(src);
    // Set to 0 on MediaPlayerPrivateGStreamer destruction, never a dangling pointer
    src->priv->mediaPlayerPrivate = mediaPlayerPrivate;
    GST_OBJECT_UNLOCK(src);
}

void webkit_media_src_set_readyforsamples(WebKitMediaSrc* src, bool isReady)
{
    if (src) {
        GST_OBJECT_LOCK(src);
        for (GList* streams = src->priv->streams; streams; streams = streams->next) {
            Stream* stream = static_cast<Stream*>(streams->data);
            stream->sourceBuffer->setReadyForMoreSamples(isReady);
        }
        GST_OBJECT_UNLOCK(src);
    }
}

void webkit_media_src_prepare_seek(WebKitMediaSrc* src, const MediaTime& time)
{
    GST_OBJECT_LOCK(src);
    src->priv->seekTime = time;
    src->priv->appSrcSeekDataCount = 0;
    src->priv->appSrcNeedDataCount = 0;

    for (GList* streams = src->priv->streams; streams; streams = streams->next) {
        Stream* stream = static_cast<Stream*>(streams->data);
        stream->appSrcNeedDataFlag = false;
        // Don't allow samples away from the seekTime to be enqueued.
        stream->lastEnqueuedTime = time;
    }

    // The pending action will be performed in app_src_seek_data().
    src->priv->appSrcSeekDataNextAction = MediaSourceSeekToTime;
    GST_OBJECT_UNLOCK(src);
}

namespace WTF {
template <> GRefPtr<WebKitMediaSrc> adoptGRef(WebKitMediaSrc* ptr)
{
    ASSERT(!ptr || !g_object_is_floating(G_OBJECT(ptr)));
    return GRefPtr<WebKitMediaSrc>(ptr, GRefPtrAdopt);
}

template <> WebKitMediaSrc* refGPtr<WebKitMediaSrc>(WebKitMediaSrc* ptr)
{
    if (ptr)
        gst_object_ref_sink(GST_OBJECT(ptr));

    return ptr;
}

template <> void derefGPtr<WebKitMediaSrc>(WebKitMediaSrc* ptr)
{
    if (ptr)
        gst_object_unref(ptr);
}
};

#endif // USE(GSTREAMER)

