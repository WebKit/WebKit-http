/*
 *  Copyright (C) 2009, 2010 Sebastian Dröge <sebastian.droege@collabora.co.uk>
 *  Copyright (C) 2013 Collabora Ltd.
 *  Copyright (C) 2013 Orange
 *  Copyright (C) 2014, 2015 Sebastian Dröge <sebastian@centricular.com>
 *  Copyright (C) 2015, 2016 Metrological Group B.V.
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
#include <wtf/MainThread.h>
#include <wtf/glib/GMutexLocker.h>
#include <wtf/glib/GUniquePtr.h>
#include <wtf/text/CString.h>

typedef struct _Stream Stream;

struct _Stream {
    // Fields filled when the Stream is created.
    WebKitMediaSrc* parent;

    // AppSrc.
    GstElement* appsrc;
    GstPad* decodebinSinkPad;
    WebCore::SourceBufferPrivateGStreamer* sourceBuffer;

    // Fields filled when the track is attached.
    WebCore::MediaSourceStreamTypeGStreamer type;
    // Might be 0, e.g. for VP8/VP9.
    GstElement* parser;
    GstCaps* caps;
#if ENABLE(VIDEO_TRACK)
    RefPtr<WebCore::AudioTrackPrivateGStreamer> audioTrack;
    RefPtr<WebCore::VideoTrackPrivateGStreamer> videoTrack;
#endif
    WebCore::FloatSize presentationSize;

    // This helps WebKitMediaSrcPrivate.appsrcNeedDataCount, ensuring that needDatas are
    // counted only once per each appsrc.
    bool appsrcNeedDataFlag;

    // Used to enforce continuity in the appended data and avoid breaking the decoder.
    MediaTime lastEnqueuedTime;

    guint notifyReadyForMoreSamplesTag;
};

enum OnSeekDataAction {
    Nothing,
    MediaSourceSeekToTime
};

struct _WebKitMediaSrcPrivate {
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

    Deque<Stream*> streams;
    gchar* location;
    int numberOfAudioStreams;
    int numberOfVideoStreams;
    int numberOfTextStreams;
    bool asyncStart;
    bool allTracksConfigured;
    unsigned numberOfPads;

    MediaTime seekTime;

    // On seek, we wait for all the seekDatas, then for all the needDatas, and then run the nextAction.
    OnSeekDataAction appsrcSeekDataNextAction;
    int appsrcSeekDataCount;
    int appsrcNeedDataCount;

    GRefPtr<GstBus> bus;
    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate;
};

enum {
    PROP_0,
    PROP_LOCATION,
    PROP_N_AUDIO,
    PROP_N_VIDEO,
    PROP_N_TEXT,
    PROP_LAST
};

enum {
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
static void freeStream(WebKitMediaSrc*, Stream*);

static void enabledAppsrcNeedData(GstAppSrc*, guint, gpointer);
static void enabledAppsrcEnoughData(GstAppSrc*, gpointer);
static gboolean enabledAppsrcSeekData(GstAppSrc*, guint64, gpointer);

static void disabledAppsrcNeedData(GstAppSrc*, guint, gpointer) { };
static void disabledAppsrcEnoughData(GstAppSrc*, gpointer) { };
static gboolean disabledAppsrcSeekData(GstAppSrc*, guint64, gpointer)
{
    return FALSE;
};

static GstAppSrcCallbacks enabledAppsrcCallbacks = {
    enabledAppsrcNeedData,
    enabledAppsrcEnoughData,
    enabledAppsrcSeekData,
    { 0 }
};

static GstAppSrcCallbacks disabledAppsrcCallbacks = {
    disabledAppsrcNeedData,
    disabledAppsrcEnoughData,
    disabledAppsrcSeekData,
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
        GParamFlags(G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(oklass,
        PROP_N_AUDIO,
        g_param_spec_int("n-audio", "Number Audio", "Total number of audio streams",
        0, G_MAXINT, 0, GParamFlags(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(oklass,
        PROP_N_VIDEO,
        g_param_spec_int("n-video", "Number Video", "Total number of video streams",
        0, G_MAXINT, 0, GParamFlags(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));
    g_object_class_install_property(oklass,
        PROP_N_TEXT,
        g_param_spec_int("n-text", "Number Text", "Total number of text streams",
        0, G_MAXINT, 0, GParamFlags(G_PARAM_READABLE | G_PARAM_STATIC_STRINGS)));

    webkit_media_src_signals[SIGNAL_VIDEO_CHANGED] =
        g_signal_new("video-changed", G_TYPE_FROM_CLASS(oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitMediaSrcClass, videoChanged), nullptr, nullptr,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);
    webkit_media_src_signals[SIGNAL_AUDIO_CHANGED] =
        g_signal_new("audio-changed", G_TYPE_FROM_CLASS(oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitMediaSrcClass, audioChanged), nullptr, nullptr,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);
    webkit_media_src_signals[SIGNAL_TEXT_CHANGED] =
        g_signal_new("text-changed", G_TYPE_FROM_CLASS(oklass),
        G_SIGNAL_RUN_LAST,
        G_STRUCT_OFFSET(WebKitMediaSrcClass, textChanged), nullptr, nullptr,
        g_cclosure_marshal_generic, G_TYPE_NONE, 0, G_TYPE_NONE);

    eklass->change_state = webKitMediaSrcChangeState;

    g_type_class_add_private(klass, sizeof(WebKitMediaSrcPrivate));
}

static void webkit_media_src_init(WebKitMediaSrc* source)
{
    source->priv = WEBKIT_MEDIA_SRC_GET_PRIVATE(source);
    new (source->priv) WebKitMediaSrcPrivate();
    source->priv->seekTime = MediaTime::invalidTime();
    source->priv->appsrcSeekDataCount = 0;
    source->priv->appsrcNeedDataCount = 0;
    source->priv->appsrcSeekDataNextAction = Nothing;

    // No need to reset Stream.appsrcNeedDataFlag because there are no Streams at this point yet.
}

static void webKitMediaSrcFinalize(GObject* object)
{
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = source->priv;

    ASSERT(WTF::isMainThread());

    Deque<Stream*> oldStreams;
    source->priv->streams.swap(oldStreams);

    for (Stream* stream : oldStreams)
        freeStream(source, stream);

    g_free(priv->location);

    priv->seekTime = MediaTime::invalidTime();

    if (priv->mediaPlayerPrivate)
        webkit_media_src_set_mediaplayerprivate(source, nullptr);

    // We used a placement new for construction, the destructor won't be called automatically.
    priv->~_WebKitMediaSrcPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitMediaSrcSetProperty(GObject* object, guint propId, const GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(object);

    switch (propId) {
    case PROP_LOCATION:
        gst_uri_handler_set_uri(reinterpret_cast<GstURIHandler*>(source), g_value_get_string(value), 0);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
}

static void webKitMediaSrcGetProperty(GObject* object, guint propId, GValue* value, GParamSpec* pspec)
{
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = source->priv;

    GST_OBJECT_LOCK(source);
    switch (propId) {
    case PROP_LOCATION:
        g_value_set_string(value, priv->location);
        break;
    case PROP_N_AUDIO:
        g_value_set_int(value, priv->numberOfAudioStreams);
        break;
    case PROP_N_VIDEO:
        g_value_set_int(value, priv->numberOfVideoStreams);
        break;
    case PROP_N_TEXT:
        g_value_set_int(value, priv->numberOfTextStreams);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propId, pspec);
        break;
    }
    GST_OBJECT_UNLOCK(source);
}

static void webKitMediaSrcDoAsyncStart(WebKitMediaSrc* source)
{
    WebKitMediaSrcPrivate* priv = source->priv;
    priv->asyncStart = true;
    GST_BIN_CLASS(parent_class)->handle_message(GST_BIN(source),
        gst_message_new_async_start(GST_OBJECT(source)));
}

static void webKitMediaSrcDoAsyncDone(WebKitMediaSrc* source)
{
    WebKitMediaSrcPrivate* priv = source->priv;
    if (priv->asyncStart) {
        GST_BIN_CLASS(parent_class)->handle_message(GST_BIN(source),
            gst_message_new_async_done(GST_OBJECT(source), GST_CLOCK_TIME_NONE));
        priv->asyncStart = false;
    }
}

static GstStateChangeReturn webKitMediaSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(element);
    WebKitMediaSrcPrivate* priv = source->priv;

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        priv->allTracksConfigured = false;
        webKitMediaSrcDoAsyncStart(source);
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (G_UNLIKELY(ret == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(source, "State change failed");
        webKitMediaSrcDoAsyncDone(source);
        return ret;
    }

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        ret = GST_STATE_CHANGE_ASYNC;
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        webKitMediaSrcDoAsyncDone(source);
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
    for (Stream* stream : webKitMediaSrc->priv->streams)
        duration = std::max<gint64>(duration, gst_app_src_get_size(GST_APP_SRC(stream->appsrc)));
    return duration;
}

static gboolean webKitMediaSrcQueryWithParent(GstPad* pad, GstObject* parent, GstQuery* query)
{
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(GST_ELEMENT(parent));
    gboolean result = FALSE;

    switch (GST_QUERY_TYPE(query)) {
    case GST_QUERY_DURATION: {
        GstFormat format;
        gst_query_parse_duration(query, &format, nullptr);

        GST_DEBUG_OBJECT(source, "duration query in format %s", gst_format_get_name(format));
        GST_OBJECT_LOCK(source);
        switch (format) {
        case GST_FORMAT_TIME: {
            float duration;
            if (source->priv && source->priv->mediaPlayerPrivate && ((duration = source->priv->mediaPlayerPrivate->duration()) > 0)) {
                gst_query_set_duration(query, format, WebCore::toGstClockTime(duration));
                GST_DEBUG_OBJECT(source, "Answering: duration=%" GST_TIME_FORMAT, GST_TIME_ARGS(WebCore::toGstClockTime(duration)));
                result = TRUE;
            }
            break;
        }
        case GST_FORMAT_BYTES: {
            if (source->priv) {
                gint64 duration = webKitMediaSrcGetSize(source);
                if (duration) {
                    gst_query_set_duration(query, format, duration);
                    GST_DEBUG_OBJECT(source, "size: %" G_GINT64_FORMAT, duration);
                    result = TRUE;
                }
            }
            break;
        }
        default:
            break;
        }

        GST_OBJECT_UNLOCK(source);
        break;
    }
    case GST_QUERY_URI:
        GST_OBJECT_LOCK(source);
        if (source && source->priv)
            gst_query_set_uri(query, source->priv->location);
        GST_OBJECT_UNLOCK(source);
        result = TRUE;
        break;
    default: {
        GRefPtr<GstPad> target = adoptGRef(gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(pad)));
        // Forward the query to the proxy target pad.
        if (target)
            result = gst_pad_query(target.get(), query);
        break;
    }
    }

    return result;
}

static void webKitMediaSrcCheckAllTracksConfigured(WebKitMediaSrc*);

static void webKitMediaSrcUpdatePresentationSize(GstCaps* caps, Stream* stream)
{
    GstStructure* structure = gst_caps_get_structure(caps, 0);
    const gchar* structureName = gst_structure_get_name(structure);
    GstVideoInfo info;

    if (g_str_has_prefix(structureName, "video/") && gst_video_info_from_caps(&info, caps)) {
        float width, height;

        // FIXME: Correct?.
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

static void webKitMediaSrcLinkStreamToSrcPad(GstPad* sourcePad, Stream* stream)
{
    unsigned padId = static_cast<unsigned>(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(sourcePad), "id")));
    GST_DEBUG_OBJECT(stream->parent, "linking stream to src pad (id: %u)", padId);

    GUniquePtr<gchar> padName(g_strdup_printf("src_%u", padId));
    GstPad* ghostpad = WebCore::webkitGstGhostPadFromStaticTemplate(&srcTemplate, padName.get(), sourcePad);

    gst_pad_set_query_function(ghostpad, webKitMediaSrcQueryWithParent);

    gst_pad_set_active(ghostpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(stream->parent), ghostpad);

    if (stream->decodebinSinkPad) {
        GST_DEBUG_OBJECT(stream->parent, "A decodebin was previously used for this source, trying to reuse it.");
        // FIXME: error checking here. Not sure what to do if linking
        // fails though, because decodebin is out of this source
        // element's scope, in theory.
        gst_pad_link(ghostpad, stream->decodebinSinkPad);
    }
}

static void webKitMediaSrcLinkParser(GstPad* sourcePad, GstCaps* caps, Stream* stream)
{
    ASSERT(caps && stream->parent);
    if (!caps || !stream->parent) {
        GST_ERROR("Unable to link parser");
        return;
    }

    webKitMediaSrcUpdatePresentationSize(caps, stream);

    // FIXME: drop webKitMediaSrcLinkStreamToSrcPad() and move its code here...
    if (!gst_pad_is_linked(sourcePad)) {
        GST_DEBUG_OBJECT(stream->parent, "pad not linked yet");
        webKitMediaSrcLinkStreamToSrcPad(sourcePad, stream);
    }

    webKitMediaSrcCheckAllTracksConfigured(stream->parent);
}

static void freeStream(WebKitMediaSrc* source, Stream* stream)
{
    if (stream->appsrc) {
        // Don't trigger callbacks from this appsrc to avoid using the stream anymore.
        gst_app_src_set_callbacks(GST_APP_SRC(stream->appsrc), &disabledAppsrcCallbacks, nullptr, 0);
        gst_app_src_end_of_stream(GST_APP_SRC(stream->appsrc));
    }

    if (stream->type != WebCore::Invalid) {
        GST_DEBUG("Freeing track-related info on stream %p", stream);

        WTF::GMutexLocker<GMutex> lock(source->priv->streamMutex);

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
            g_signal_emit(G_OBJECT(source), webkit_media_src_signals[signal], 0, nullptr);

        g_cond_signal(&source->priv->streamCondition);
    }

    GST_DEBUG("Releasing stream: %p", stream);
    g_free(stream);
}

static void webKitMediaSrcCheckAllTracksConfigured(WebKitMediaSrc* webKitMediaSrc)
{
    bool allTracksConfigured = false;

    GST_OBJECT_LOCK(webKitMediaSrc);
    if (!(webKitMediaSrc->priv->allTracksConfigured)) {
        allTracksConfigured = true;
        for (Stream* stream : webKitMediaSrc->priv->streams) {
            if (stream->type == WebCore::Invalid) {
                allTracksConfigured = false;
                break;
            }
        }
        if (allTracksConfigured)
            webKitMediaSrc->priv->allTracksConfigured = true;
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    if (allTracksConfigured) {
        GST_DEBUG("All tracks attached. Completing async state change operation.");
        gst_element_no_more_pads(GST_ELEMENT(webKitMediaSrc));
        webKitMediaSrcDoAsyncDone(webKitMediaSrc);
    }
}

// uri handler interface.
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
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(handler);
    gchar* ret;

    GST_OBJECT_LOCK(source);
    ret = g_strdup(source->priv->location);
    GST_OBJECT_UNLOCK(source);
    return ret;
}

static gboolean webKitMediaSrcSetUri(GstURIHandler* handler, const gchar* uri, GError**)
{
    WebKitMediaSrc* source = WEBKIT_MEDIA_SRC(handler);
    WebKitMediaSrcPrivate* priv = source->priv;

    if (GST_STATE(source) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(source, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GST_OBJECT_LOCK(source);
    g_free(priv->location);
    priv->location = nullptr;
    if (!uri) {
        GST_OBJECT_UNLOCK(source);
        return TRUE;
    }

    WebCore::URL url(WebCore::URL(), uri);

    priv->location = g_strdup(url.string().utf8().data());
    GST_OBJECT_UNLOCK(source);
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

static Stream* getStreamByTrackId(WebKitMediaSrc* source, AtomicString trackIdString)
{
    // WebKitMediaSrc should be locked at this point.
    for (Stream* stream : source->priv->streams) {
        if (stream->type != WebCore::Invalid
            && ((stream->audioTrack && stream->audioTrack->id() == trackIdString)
                || (stream->videoTrack && stream->videoTrack->id() == trackIdString) ) ) {
            return stream;
        }
    }
    return nullptr;
}

static Stream* getStreamBySourceBufferPrivate(WebKitMediaSrc* source, WebCore::SourceBufferPrivateGStreamer* sourceBufferPrivate)
{
    for (Stream* stream : source->priv->streams) {
        if (stream->sourceBuffer == sourceBufferPrivate)
            return stream;
    }
    return nullptr;
}

static Stream* getStreamByAppsrc(WebKitMediaSrc* source, GstElement* appsrc)
{
    for (Stream* stream : source->priv->streams) {
        if (stream->appsrc == appsrc)
            return stream;
    }
    return nullptr;
}

static gboolean seekNeedsDataMainThread(WebKitMediaSrc* source)
{
    GST_DEBUG("Buffering needed before seek");

    ASSERT(WTF::isMainThread());

    GST_OBJECT_LOCK(source);
    MediaTime seekTime = source->priv->seekTime;
    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate = source->priv->mediaPlayerPrivate;
    GST_OBJECT_UNLOCK(source);

    if (!mediaPlayerPrivate)
        return G_SOURCE_REMOVE;

    for (Stream* stream : source->priv->streams) {
        if (stream->type != WebCore::Invalid)
            stream->sourceBuffer->setReadyForMoreSamples(true);
    }

    mediaPlayerPrivate->notifySeekNeedsDataForTime(seekTime);

    return G_SOURCE_REMOVE;
}

static void enabledAppsrcNeedData(GstAppSrc* appsrc, guint, gpointer userData)
{
    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(userData);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    bool allAppsrcNeedDataAfterSeek = false;

    GST_OBJECT_LOCK(webKitMediaSrc);
    OnSeekDataAction appsrcSeekDataNextAction = webKitMediaSrc->priv->appsrcSeekDataNextAction;
    int numAppsrcs = webKitMediaSrc->priv->streams.size();
    Stream* appsrcStream = getStreamByAppsrc(webKitMediaSrc, GST_ELEMENT(appsrc));

    if (webKitMediaSrc->priv->appsrcSeekDataCount > 0) {
        if (appsrcStream && !appsrcStream->appsrcNeedDataFlag) {
            ++webKitMediaSrc->priv->appsrcNeedDataCount;
            appsrcStream->appsrcNeedDataFlag = true;
        }
        if (webKitMediaSrc->priv->appsrcSeekDataCount == numAppsrcs && webKitMediaSrc->priv->appsrcNeedDataCount == numAppsrcs) {
            GST_DEBUG("All needDatas completed");
            allAppsrcNeedDataAfterSeek = true;
            webKitMediaSrc->priv->appsrcSeekDataCount = 0;
            webKitMediaSrc->priv->appsrcNeedDataCount = 0;
            webKitMediaSrc->priv->appsrcSeekDataNextAction = Nothing;

            for (Stream* stream : webKitMediaSrc->priv->streams)
                stream->appsrcNeedDataFlag = false;
        }
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    if (allAppsrcNeedDataAfterSeek) {
        GST_DEBUG("All expected appsrcSeekData() and appsrcNeedData() calls performed. Running next action (%d)", static_cast<int>(appsrcSeekDataNextAction));

        switch (appsrcSeekDataNextAction) {
        case MediaSourceSeekToTime: {
            GstStructure* structure = gst_structure_new_empty("seek-needs-data");
            GstMessage* message = gst_message_new_application(GST_OBJECT(appsrc), structure);
            gst_bus_post(webKitMediaSrc->priv->bus.get(), message);
            GST_TRACE("seek-needs-data message posted to the bus");
            break;
        }
        case Nothing:
            break;
        }
    } else if (appsrcSeekDataNextAction == Nothing) {
        WTF::GMutexLocker<GMutex> lock(webKitMediaSrc->priv->streamMutex);

        GST_OBJECT_LOCK(webKitMediaSrc);

        // Search again for the Stream, just in case it was removed between the previous lock and this one.
        appsrcStream = getStreamByAppsrc(webKitMediaSrc, GST_ELEMENT(appsrc));

        if (appsrcStream && !appsrcStream->notifyReadyForMoreSamplesTag && appsrcStream->type != WebCore::Invalid) {
            appsrcStream->notifyReadyForMoreSamplesTag =
                g_timeout_add(0, GSourceFunc([](gpointer userData) {
                    // This lambda is referred as "notifyReadyForMoreSamples".
                    Stream* stream = static_cast<Stream*>(userData);
                    WebKitMediaSrc* webKitMediaSrc = WEBKIT_MEDIA_SRC(stream->parent);
                    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

                    WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate = webKitMediaSrc->priv->mediaPlayerPrivate;
                    if (mediaPlayerPrivate && !mediaPlayerPrivate->seeking())
                        stream->sourceBuffer->notifyReadyForMoreSamples();

                    GST_OBJECT_LOCK(webKitMediaSrc);
                    stream->notifyReadyForMoreSamplesTag = 0;
                    GST_OBJECT_UNLOCK(webKitMediaSrc);

                    return G_SOURCE_REMOVE;
                }), appsrcStream);
        }
        GST_OBJECT_UNLOCK(webKitMediaSrc);
    }
}

static void enabledAppsrcEnoughData(GstAppSrc *appsrc, gpointer userData)
{
    // No need to lock on webKitMediaSrc, we're on the main thread and nobody is going to remove the stream in the meantime.
    ASSERT(WTF::isMainThread());

    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(userData);
    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));
    Stream* stream = getStreamByAppsrc(webKitMediaSrc, GST_ELEMENT(appsrc));

    // This callback might have been scheduled from a child thread before the stream was removed.
    // Then, the removal code might have run, and later this callback.
    // This check solves the race condition.
    if (!stream || stream->type == WebCore::Invalid)
        return;

    stream->sourceBuffer->setReadyForMoreSamples(false);
}

static gboolean enabledAppsrcSeekData(GstAppSrc*, guint64, gpointer userData)
{
    ASSERT(WTF::isMainThread());

    WebKitMediaSrc* webKitMediaSrc = static_cast<WebKitMediaSrc*>(userData);

    ASSERT(WEBKIT_IS_MEDIA_SRC(webKitMediaSrc));

    GST_OBJECT_LOCK(webKitMediaSrc);
    webKitMediaSrc->priv->appsrcSeekDataCount++;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    return TRUE;
}

// FIXME: Use gst_app_src_push_sample() instead when we switch to the appropriate GStreamer version.
static GstFlowReturn pushSample(GstAppSrc* appsrc, GstSample* sample)
{
    GstBuffer* buffer;
    GstCaps* caps;

    g_return_val_if_fail(GST_IS_SAMPLE(sample), GST_FLOW_ERROR);

    caps = gst_sample_get_caps(sample);
    if (caps)
        gst_app_src_set_caps(appsrc, caps);
    else
        GST_WARNING_OBJECT(appsrc, "received sample without caps");

    buffer = gst_sample_get_buffer(sample);
    if (!buffer) {
        GST_WARNING_OBJECT(appsrc, "received sample without buffer");
        return GST_FLOW_OK;
    }

    // gst_app_src_push_buffer() steals the reference, we need an additional one.
    gst_buffer_ref(buffer);
    return gst_app_src_push_buffer(appsrc, buffer);
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
    GST_DEBUG("webKitMediaSrc=%p", webKitMediaSrc);
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
    stream->appsrcNeedDataFlag = false;
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

    gst_app_src_set_callbacks(GST_APP_SRC(stream->appsrc), &enabledAppsrcCallbacks, stream->parent, 0);
    gst_app_src_set_emit_signals(GST_APP_SRC(stream->appsrc), FALSE);
    gst_app_src_set_stream_type(GST_APP_SRC(stream->appsrc), GST_APP_STREAM_TYPE_SEEKABLE);

    gst_app_src_set_max_bytes(GST_APP_SRC(stream->appsrc), 2 * WTF::MB);
    g_object_set(G_OBJECT(stream->appsrc),
        "block", FALSE,
        "min-percent", 20, nullptr);

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    priv->streams.prepend(stream);
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    gst_bin_add(GST_BIN(m_webKitMediaSrc.get()), stream->appsrc);
    gst_element_sync_state_with_parent(stream->appsrc);

    return MediaSourcePrivate::Ok;
}

void PlaybackPipeline::removeSourceBuffer(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate)
{
    ASSERT(WTF::isMainThread());

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Element removed from MediaSource");
    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    WebKitMediaSrcPrivate* priv = m_webKitMediaSrc->priv;
    Stream* stream = nullptr;
    Deque<Stream*>::iterator streamPosition = priv->streams.begin();

    while (streamPosition != priv->streams.end()) {
        if ((*streamPosition)->sourceBuffer == sourceBufferPrivate.get()) {
            stream = *streamPosition;
            break;
        }
        ++streamPosition;
    }
    if (stream)
        priv->streams.remove(streamPosition);
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (stream)
        freeStream(m_webKitMediaSrc.get(), stream);
}

void PlaybackPipeline::attachTrack(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, RefPtr<TrackPrivateBase> trackPrivate, GstStructure* structure, GstCaps* caps)
{
    WebKitMediaSrc* webKitMediaSrc = m_webKitMediaSrc.get();
    Stream* stream = nullptr;
    unsigned padId = 0;
    const gchar* mediaType = gst_structure_get_name(structure);

    GST_OBJECT_LOCK(webKitMediaSrc);
    stream = getStreamBySourceBufferPrivate(webKitMediaSrc, sourceBufferPrivate.get());
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    ASSERT(stream);

    GST_OBJECT_LOCK(webKitMediaSrc);
    padId = stream->parent->priv->numberOfPads;
    stream->parent->priv->numberOfPads++;
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    GUniquePtr<gchar> parserBinName(g_strdup_printf("streamparser%u", padId));
    stream->parser = gst_bin_new(parserBinName.get());

    GST_DEBUG_OBJECT(webKitMediaSrc, "Configured track %s: appsrc=%s, padId=%u, mediaType=%s", trackPrivate->id().string().utf8().data(), GST_ELEMENT_NAME(stream->appsrc), padId, mediaType);

    if (!g_strcmp0(mediaType, "video/x-h264")) {
        GstElement* parser;
        GstElement* capsfilter;
        GstPad* pad = nullptr;
        GRefPtr<GstCaps> filterCaps = adoptGRef(gst_caps_new_simple("video/x-h264", "alignment", G_TYPE_STRING, "au", nullptr));

        parser = gst_element_factory_make("h264parse", nullptr);
        capsfilter = gst_element_factory_make("capsfilter", nullptr);
        g_object_set(capsfilter, "caps", filterCaps.get(), nullptr);

        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, nullptr);
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
        GRefPtr<GstCaps> filterCaps = adoptGRef(gst_caps_new_simple("video/x-h265", "alignment", G_TYPE_STRING, "au", nullptr));

        parser = gst_element_factory_make("h265parse", nullptr);
        capsfilter = gst_element_factory_make("capsfilter", nullptr);
        g_object_set(capsfilter, "caps", filterCaps.get(), nullptr);

        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, nullptr);
        gst_element_link_pads(parser, "src", capsfilter, "sink");

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(capsfilter, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else if (!g_strcmp0(mediaType, "audio/mpeg")) {
        gint mpegversion = -1;
        GstElement* parser;
        GstPad* pad = nullptr;

        gst_structure_get_int(structure, "mpegversion", &mpegversion);
        if (mpegversion == 1) {
            parser = gst_element_factory_make("mpegaudioparse", nullptr);
        } else if (mpegversion == 2 || mpegversion == 4)
            parser = gst_element_factory_make("aacparse", nullptr);
        else
            ASSERT_NOT_REACHED();

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

    GstPad* sinkPad = gst_element_get_static_pad(stream->parser, "sink");
    GstPad* sourcePad = gst_element_get_static_pad(stream->appsrc, "src");
    gst_pad_link(sourcePad, sinkPad);
    gst_object_unref(sourcePad);
    sourcePad = nullptr;
    gst_object_unref(sinkPad);
    sinkPad = nullptr;

    sourcePad = gst_element_get_static_pad(stream->parser, "src");
    // FIXME: Is padId the best way to identify the Stream? What about trackId?.
    g_object_set_data(G_OBJECT(sourcePad), "id", GINT_TO_POINTER(padId));
    webKitMediaSrcLinkParser(sourcePad, caps, stream);

    ASSERT(stream->parent->priv->mediaPlayerPrivate);
    int signal = -1;
    if (g_str_has_prefix(mediaType, "audio")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Audio;
        stream->parent->priv->numberOfAudioStreams++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_AUDIO_CHANGED;

        stream->audioTrack = RefPtr<WebCore::AudioTrackPrivateGStreamer>(static_cast<WebCore::AudioTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "video")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Video;
        stream->parent->priv->numberOfVideoStreams++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_VIDEO_CHANGED;

        stream->videoTrack = RefPtr<WebCore::VideoTrackPrivateGStreamer>(static_cast<WebCore::VideoTrackPrivateGStreamer*>(trackPrivate.get()));
    } else if (g_str_has_prefix(mediaType, "text")) {
        GST_OBJECT_LOCK(webKitMediaSrc);
        stream->type = Text;
        stream->parent->priv->numberOfTextStreams++;
        GST_OBJECT_UNLOCK(webKitMediaSrc);
        signal = SIGNAL_TEXT_CHANGED;

        // FIXME: Support text tracks.
    }

    if (signal != -1)
        g_signal_emit(G_OBJECT(stream->parent), webkit_media_src_signals[signal], 0, nullptr);

    gst_object_unref(sourcePad);
    sourcePad = nullptr;
}

void PlaybackPipeline::reattachTrack(RefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, RefPtr<TrackPrivateBase> trackPrivate)
{
    GST_DEBUG("Re-attaching track");

    // FIXME: Maybe remove this method.
    // Now the caps change is managed by gst_appsrc_push_sample()
    // in enqueueSample() and flushAndEnqueueNonDisplayingSamples().

    WebKitMediaSrc* webKitMediaSrc = m_webKitMediaSrc.get();

    GST_OBJECT_LOCK(webKitMediaSrc);
    Stream* stream = getStreamBySourceBufferPrivate(webKitMediaSrc, sourceBufferPrivate.get());
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    ASSERT(stream);
    ASSERT(stream->type != Invalid);

    GstCaps* oldAppsrcCaps = gst_app_src_get_caps(GST_APP_SRC(stream->appsrc));
    // Now the caps change is managed by gst_appsrc_push_sample()
    // in enqueueSample() and flushAndEnqueueNonDisplayingSamples().
    // gst_app_src_set_caps(GST_APP_SRC(stream->appsrc), caps);
    GstCaps* appsrcCaps = gst_app_src_get_caps(GST_APP_SRC(stream->appsrc));
    const gchar* mediaType = gst_structure_get_name(gst_caps_get_structure(appsrcCaps, 0));

    if (!gst_caps_is_equal(oldAppsrcCaps, appsrcCaps)) {
        GST_DEBUG("Caps have changed, but reconstructing the sequence of elements is not supported yet");

#if (!(LOG_DISABLED || GST_DISABLE_GST_DEBUG))
        gchar* stroldcaps = gst_caps_to_string(oldAppsrcCaps);
        gchar* strnewcaps = gst_caps_to_string(appsrcCaps);
        GST_DEBUG("oldcaps: %s", stroldcaps);
        GST_DEBUG("newcaps: %s", strnewcaps);
        g_free(stroldcaps);
        g_free(strnewcaps);
#endif
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

        // FIXME: Support text tracks.
    }
    GST_OBJECT_UNLOCK(webKitMediaSrc);

    gst_caps_unref(appsrcCaps);
    gst_caps_unref(oldAppsrcCaps);

    if (signal != -1)
        g_signal_emit(G_OBJECT(stream->parent), webkit_media_src_signals[signal], 0, nullptr);
}

void PlaybackPipeline::notifyDurationChanged()
{
    gst_element_post_message(GST_ELEMENT(m_webKitMediaSrc.get()), gst_message_new_duration_changed(GST_OBJECT(m_webKitMediaSrc.get())));
    // WebKitMediaSrc will ask MediaPlayerPrivateGStreamerMSE for the new duration later, when somebody asks for it.
}

void PlaybackPipeline::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus)
{
    WebKitMediaSrcPrivate* priv = m_webKitMediaSrc->priv;

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Have EOS");

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    bool allTracksConfigured = priv->allTracksConfigured;
    if (!allTracksConfigured)
        priv->allTracksConfigured = true;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (!allTracksConfigured) {
        gst_element_no_more_pads(GST_ELEMENT(m_webKitMediaSrc.get()));
        webKitMediaSrcDoAsyncDone(m_webKitMediaSrc.get());
    }

    Vector<GstAppSrc*> appsrcs;

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    for (Stream* stream : priv->streams) {
        if (stream->appsrc)
            appsrcs.append(GST_APP_SRC(stream->appsrc));
    }
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    for (Vector<GstAppSrc*>::iterator iterator = appsrcs.begin(); iterator != appsrcs.end(); ++iterator)
        gst_app_src_end_of_stream(*iterator);
}

void PlaybackPipeline::flushAndEnqueueNonDisplayingSamples(Vector<RefPtr<MediaSample>> samples)
{
    ASSERT(WTF::isMainThread());

    if (!samples.size()) {
        GST_DEBUG("No samples, trackId unknown");
        return;
    }

    AtomicString trackId = samples[0]->trackID();
    GST_DEBUG("flushAndEnqueueNonDisplayingSamples: trackId=%s PTS[0]=%f ... PTS[n]=%f", trackId.string().utf8().data(), samples[0]->presentationTime().toFloat(), samples[samples.size()-1]->presentationTime().toFloat());

    GST_DEBUG_OBJECT(m_webKitMediaSrc.get(), "Flushing and re-enqueing %zu samples for stream %s", samples.size(), trackId.string().utf8().data());

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    Stream* stream = getStreamByTrackId(m_webKitMediaSrc.get(), trackId);

    if (!stream) {
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    if (!stream->sourceBuffer->isReadyForMoreSamples(trackId)) {
        GST_DEBUG("flushAndEnqueueNonDisplayingSamples: skip adding new sample for trackId=%s, SB is not ready yet", trackId.string().utf8().data());
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    GstElement* appsrc = stream->appsrc;
    MediaTime lastEnqueuedTime = stream->lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    if (!m_webKitMediaSrc->priv->mediaPlayerPrivate->seeking())
        GST_DEBUG("flushAndEnqueueNonDisplayingSamples: trackId=%s pipeline needs flushing.", trackId.string().utf8().data());

    for (Vector<RefPtr<MediaSample>>::iterator iterator = samples.begin(); iterator != samples.end(); ++iterator) {
        GStreamerMediaSample* sample = static_cast<GStreamerMediaSample*>(iterator->get());
        GstBuffer* buffer = nullptr;
        if (sample->sample())
            buffer = gst_sample_get_buffer(sample->sample());
        if (buffer) {
            GstSample* gstSample = gst_sample_ref(sample->sample());
            lastEnqueuedTime = sample->presentationTime();

            GST_BUFFER_FLAG_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY);
            pushSample(GST_APP_SRC(appsrc), gstSample);
            // gst_app_src_push_sample() uses transfer-none for gstSample.

            gst_sample_unref(gstSample);
        }
    }
    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    stream->lastEnqueuedTime = lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
}

void PlaybackPipeline::enqueueSample(PassRefPtr<MediaSample> prSample)
{
    ASSERT(WTF::isMainThread());

    RefPtr<MediaSample> rSample = prSample;
    AtomicString trackId = rSample->trackID();

    GST_TRACE("enqueing sample trackId=%s PTS=%f presentationSize=%.0fx%.0f at %" GST_TIME_FORMAT " duration: %" GST_TIME_FORMAT, trackId.string().utf8().data(), rSample->presentationTime().toFloat(), rSample->presentationSize().width(), rSample->presentationSize().height(), GST_TIME_ARGS(WebCore::toGstClockTime(rSample->presentationTime().toDouble())), GST_TIME_ARGS(WebCore::toGstClockTime(rSample->duration().toDouble())));

    GST_OBJECT_LOCK(m_webKitMediaSrc.get());
    Stream* stream = getStreamByTrackId(m_webKitMediaSrc.get(), trackId);

    if (!stream) {
        GST_WARNING("No stream!");
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    if (!stream->sourceBuffer->isReadyForMoreSamples(trackId)) {
        GST_DEBUG("enqueueSample: skip adding new sample for trackId=%s, SB is not ready yet", trackId.string().utf8().data());
        GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());
        return;
    }

    GstElement* appsrc = stream->appsrc;
    MediaTime lastEnqueuedTime = stream->lastEnqueuedTime;
    GST_OBJECT_UNLOCK(m_webKitMediaSrc.get());

    GStreamerMediaSample* sample = static_cast<GStreamerMediaSample*>(rSample.get());
    if (sample->sample() && gst_sample_get_buffer(sample->sample())) {
        GstSample* gstSample = gst_sample_ref(sample->sample());
        GstBuffer* buffer = gst_sample_get_buffer(gstSample);
        lastEnqueuedTime = sample->presentationTime();

        GST_BUFFER_FLAG_UNSET(buffer, GST_BUFFER_FLAG_DECODE_ONLY);
        pushSample(GST_APP_SRC(appsrc), gstSample);
        // gst_app_src_push_sample() uses transfer-none for gstSample.

        gst_sample_unref(gstSample);

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

static void applicationMessageCallback(GstBus*, GstMessage* message, WebKitMediaSrc* source)
{
    ASSERT(WTF::isMainThread());
    ASSERT(GST_MESSAGE_TYPE(message) == GST_MESSAGE_APPLICATION);

    const GstStructure* structure = gst_message_get_structure(message);

    if (gst_structure_has_name(structure, "seek-needs-data"))
        seekNeedsDataMainThread(source);
}

void webkit_media_src_set_mediaplayerprivate(WebKitMediaSrc* source, WebCore::MediaPlayerPrivateGStreamerMSE* mediaPlayerPrivate)
{
    GST_OBJECT_LOCK(source);
    if (source->priv->mediaPlayerPrivate && source->priv->mediaPlayerPrivate != mediaPlayerPrivate && source->priv->bus)
        g_signal_handlers_disconnect_by_func(source->priv->bus.get(), gpointer(applicationMessageCallback), source);

    // Set to nullptr on MediaPlayerPrivateGStreamer destruction, never a dangling pointer.
    source->priv->mediaPlayerPrivate = mediaPlayerPrivate;
    source->priv->bus = mediaPlayerPrivate?adoptGRef(gst_pipeline_get_bus(GST_PIPELINE(mediaPlayerPrivate->pipeline()))):nullptr;
    if (source->priv->bus) {
        // MediaPlayerPrivateGStreamer has called gst_bus_add_signal_watch() at this point, so we can subscribe.
        g_signal_connect(source->priv->bus.get(), "message::application", G_CALLBACK(applicationMessageCallback), source);
    }
    GST_OBJECT_UNLOCK(source);
}

void webkit_media_src_set_readyforsamples(WebKitMediaSrc* source, bool isReady)
{
    if (source) {
        GST_OBJECT_LOCK(source);
        for (Stream* stream : source->priv->streams)
            stream->sourceBuffer->setReadyForMoreSamples(isReady);
        GST_OBJECT_UNLOCK(source);
    }
}

void webkit_media_src_prepare_seek(WebKitMediaSrc* source, const MediaTime& time)
{
    GST_OBJECT_LOCK(source);
    source->priv->seekTime = time;
    source->priv->appsrcSeekDataCount = 0;
    source->priv->appsrcNeedDataCount = 0;

    for (Stream* stream : source->priv->streams) {
        stream->appsrcNeedDataFlag = false;
        // Don't allow samples away from the seekTime to be enqueued.
        stream->lastEnqueuedTime = time;
    }

    // The pending action will be performed in enabledAppsrcSeekData().
    source->priv->appsrcSeekDataNextAction = MediaSourceSeekToTime;
    GST_OBJECT_UNLOCK(source);
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

