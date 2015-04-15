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
#include <wtf/gobject/GUniquePtr.h>

namespace WebCore
{
class GStreamerMediaDescription : public MediaDescription {
private:
    GstCaps* m_caps;
public:
    static PassRefPtr<GStreamerMediaDescription> create(GstCaps* caps)
    {
        return adoptRef(new GStreamerMediaDescription(caps));
    }

    virtual ~GStreamerMediaDescription()
    {
        gst_caps_unref(m_caps);
    }

    AtomicString codec() const override
    {
        gchar* description = gst_pb_utils_get_codec_description(m_caps);
        AtomicString codecName(description);
        g_free(description);

        return codecName;
    }

    bool isVideo() const override
    {
        GstStructure* s = gst_caps_get_structure(m_caps, 0);
        const gchar* name = gst_structure_get_name(s);

        return g_str_has_prefix(name, "video/");
    }

    bool isAudio() const override
    {
        GstStructure* s = gst_caps_get_structure(m_caps, 0);
        const gchar* name = gst_structure_get_name(s);

        return g_str_has_prefix(name, "audio/");
    }

    bool isText() const override
    {
        // TODO
        return false;
    }

private:
    GStreamerMediaDescription(GstCaps* caps)
        : MediaDescription()
        , m_caps(gst_caps_ref(caps))
    {
    }
};

class GStreamerMediaSample : public MediaSample
{
private:
    MediaTime m_pts, m_dts, m_duration;
    AtomicString m_trackID;
    size_t m_size;
    FloatSize m_presentationSize;
    MediaSample::SampleFlags m_flags;
    GStreamerMediaSample(GstBuffer* buffer, const FloatSize& presentationSize, const AtomicString& trackID)
        : MediaSample()
        , m_pts(MediaTime::zeroTime())
        , m_dts(MediaTime::zeroTime())
        , m_duration(MediaTime::zeroTime())
        , m_trackID(trackID)
        , m_presentationSize(presentationSize)
        , m_flags(MediaSample::IsSync)
    {
        if (GST_BUFFER_PTS_IS_VALID(buffer))
            m_pts = MediaTime(GST_BUFFER_PTS(buffer), GST_SECOND);
        if (GST_BUFFER_DTS_IS_VALID(buffer))
            m_dts = MediaTime(GST_BUFFER_DTS(buffer), GST_SECOND);
        if (GST_BUFFER_DURATION_IS_VALID(buffer))
            m_duration = MediaTime(GST_BUFFER_DURATION(buffer), GST_SECOND);
        m_size = gst_buffer_get_size(buffer);

        if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DELTA_UNIT))
            m_flags = MediaSample::None;

        if (GST_BUFFER_FLAG_IS_SET(buffer, GST_BUFFER_FLAG_DECODE_ONLY))
            m_flags = (MediaSample::SampleFlags) (m_flags | MediaSample::NonDisplaying);
    }

public:
    static PassRefPtr<GStreamerMediaSample> create(GstBuffer* buffer, const FloatSize& presentationSize, const AtomicString& trackID)
    {
        return adoptRef(new GStreamerMediaSample(buffer, presentationSize, trackID));
    }

    MediaTime presentationTime() const { return m_pts; }
    MediaTime decodeTime() const { return m_dts; }
    MediaTime duration() const { return m_duration; }
    AtomicString trackID() const { return m_trackID; }
    size_t sizeInBytes() const { return m_size; }
    FloatSize presentationSize() const { return m_presentationSize; }
    SampleFlags flags() const { return m_flags; }
    PlatformSample platformSample() { return PlatformSample(); }
    void dump(PrintStream&) const {}
};
};

typedef struct _Stream Stream;
typedef struct _Source Source;

typedef struct {
    GstBuffer* buffer;
    WebCore::FloatSize presentationSize;
} PendingReceiveSample;

typedef enum {STREAM_TYPE_UNKNOWN, STREAM_TYPE_AUDIO, STREAM_TYPE_VIDEO, STREAM_TYPE_TEXT} StreamType;

struct _Stream
{
    Source* parent;

    int id;
    StreamType type;

    // Might be 0, e.g. for VP8/VP9
    GstElement *parser;
    GstPad* srcpad;
    GstPad* demuxersrcpad;
    GstPad* multiqueuesrcpad;
    GstCaps* caps;
    gulong bufferProbeId;
    gulong bufferAfterMultiqueueProbeId;
#if ENABLE(VIDEO_TRACK)
    RefPtr<WebCore::AudioTrackPrivateGStreamer> *audioTrack;
    RefPtr<WebCore::VideoTrackPrivateGStreamer> *videoTrack;
#endif
    WebCore::FloatSize presentationSize;
    GList* pendingReceiveSample;
    bool initSegmentAlreadyProcessed;
};

struct _Source {
    WebKitMediaSrc* parent;
    GstElement* src;
    GstElement* typefind;
    // May be 0 if elementary stream
    GstElement* demuxer;
    GstElement* multiqueue;
    GList* streams;

    // We expose everything when
    // all sources are noMorePads
    bool noMorePads;

    // Just for identification
    WebCore::SourceBufferPrivateGStreamer* sourceBuffer;

    // Some appended data are only headers and don't generate any
    // useful stream data for decoding. This is detected with a
    // timeout and reported to the upper layers, so update/updateend
    // can be generated and the append operation doesn't block.
    guint noDataToDecodeTimeoutTag;

    // Samples coming after the init segment arrive individually,
    // we must detect when no more samples have arrived after a while
    gint64 lastSampleTime;
    guint pendingSamplesAfterInitSegment;
};

struct _WebKitMediaSrcPrivate
{
    GList* sources;
    gchar* location;
    int nAudio;
    int nVideo;
    int nText;
    GstClockTime duration;
    bool haveAppsrc;
    bool asyncStart;
    bool noMorePads;
    int numberOfPads;

    WebCore::MediaSourceClientGStreamer* mediaSourceClient;
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
static gboolean webKitMediaSrcEventWithParent(GstPad*, GstObject*, GstEvent*);

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
}

static void webKitMediaSrcFinalize(GObject* object)
{
    WebKitMediaSrc* src = WEBKIT_MEDIA_SRC(object);
    WebKitMediaSrcPrivate* priv = src->priv;

    // TODO: Free sources
    g_free(priv->location);

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
        priv->noMorePads = false;
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
        priv->noMorePads = false;
        break;
    default:
        break;
    }

    return ret;
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
        if ((format == GST_FORMAT_TIME) && src && src->priv && (src->priv->duration > 0)) {
            gst_query_set_duration(query, format, src->priv->duration);
            result = TRUE;
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

#if ENABLE(VIDEO_TRACK)
static gboolean webKitMediaSrcDidReceiveInitializationSegment(gpointer data);
#endif

static gboolean webKitMediaSrcLastSampleTimeout(Source* source);

static gboolean webKitMediaSrcEventWithParent(GstPad* pad, GstObject* parent, GstEvent* event)
{
    gboolean result = FALSE;

    switch (GST_EVENT_TYPE(event)) {
#if ENABLE(VIDEO_TRACK)
    case GST_EVENT_CUSTOM_UPSTREAM:{
        const GstStructure* s = gst_event_get_structure(event);

        if (gst_structure_has_name(s, "webKitAudioTrack")) {
            Stream* stream = (Stream*) gst_pad_get_element_private(pad);
            if (stream && stream->parent) {
                RefPtr<WebCore::AudioTrackPrivateGStreamer>* audioTrack;

                gst_structure_get(s, "track", G_TYPE_POINTER, &audioTrack, NULL);
                stream->audioTrack = audioTrack;

                g_timeout_add(0, GSourceFunc(webKitMediaSrcDidReceiveInitializationSegment), stream->parent);
            }
            result = TRUE;
            gst_event_unref(event);
        } else if (gst_structure_has_name(s, "webKitVideoTrack")) {
            Stream* stream = (Stream*) gst_pad_get_element_private(pad);
            if (stream && stream->parent) {
                RefPtr<WebCore::VideoTrackPrivateGStreamer>* videoTrack;

                gst_structure_get(s, "track", G_TYPE_POINTER, &videoTrack, NULL);
                stream->videoTrack = videoTrack;

                g_timeout_add(0, GSourceFunc(webKitMediaSrcDidReceiveInitializationSegment), stream->parent);
            }
            result = TRUE;
            gst_event_unref(event);
        } else {
            result = gst_pad_event_default(pad, parent, event);
        }
        break;
    }
#endif
    default:
        result = gst_pad_event_default(pad, parent, event);
        break;
    }

    return result;
}

static GstPad* get_internal_linked_pad(GstPad* pad)
{
    GstIterator* it;
    GstPad* otherpad;
    GValue item = G_VALUE_INIT;

    it = gst_pad_iterate_internal_links(pad);

    if (!it || (gst_iterator_next(it, &item)) != GST_ITERATOR_OK
        || ((otherpad = GST_PAD(g_value_dup_object(&item))) == 0)) {
        return 0;
    }
    g_value_unset(&item);
    if (it)
        gst_iterator_free(it);

    return otherpad;
}

typedef struct {
    Stream* stream;
    WTF::RefPtr<WebCore::GStreamerMediaSample> sample;
} ReceiveSample;

#if ENABLE(VIDEO_TRACK)
static gboolean webKitWebSrcDidReceiveSample(gpointer userdata)
{
    ReceiveSample* sample = (ReceiveSample*)userdata;

    if (sample->stream->parent) {
        sample->stream->parent->parent->priv->mediaSourceClient->didReceiveSample(sample->stream->parent->sourceBuffer, sample->sample);

        GST_OBJECT_LOCK(sample->stream->parent->parent);
        sample->stream->parent->pendingSamplesAfterInitSegment--;
        if (!sample->stream->parent->lastSampleTime) {
            g_timeout_add(100, GSourceFunc(webKitMediaSrcLastSampleTimeout), sample->stream->parent);
        }
        sample->stream->parent->lastSampleTime = g_get_monotonic_time();

        GST_OBJECT_UNLOCK(sample->stream->parent->parent);
    }
    sample->sample.release();
    g_free(sample);

    return G_SOURCE_REMOVE;
}
#endif

static gboolean webKitMediaSrcLastSampleTimeout(Source* source)
{
    bool result;
    bool callDidReceiveAllPendingSamples = false;

    GST_OBJECT_LOCK(source->parent);

    if (source->lastSampleTime) {
        if (source->pendingSamplesAfterInitSegment == 0 && g_get_monotonic_time() - source->lastSampleTime > 250) {
            source->lastSampleTime = 0;
            callDidReceiveAllPendingSamples = true;
            result = G_SOURCE_REMOVE;
        } else
            result = G_SOURCE_CONTINUE;
    } else {
        // The timer has been cancelled
        result = G_SOURCE_REMOVE;
    }
    GST_OBJECT_UNLOCK(source->parent);

    if (callDidReceiveAllPendingSamples) {
        source->parent->priv->mediaSourceClient->didReceiveAllPendingSamples(source->sourceBuffer);
    }
    return result;
}

static GstPadProbeReturn webKitWebSrcBufferProbe(GstPad*, GstPadProbeInfo* info, Stream* stream)
{
    if (!(stream->parent)) return GST_PAD_PROBE_DROP;

    GstBuffer* buffer = GST_BUFFER(info->data);

    GST_OBJECT_LOCK(stream->parent->parent);
    if (stream->parent->noDataToDecodeTimeoutTag) {
        g_source_remove(stream->parent->noDataToDecodeTimeoutTag);
        stream->parent->noDataToDecodeTimeoutTag = 0;
    }

    bool initSegmentAlreadyProcessed = stream->initSegmentAlreadyProcessed;

    if (initSegmentAlreadyProcessed) {
        ReceiveSample* sample = g_new0(ReceiveSample, 1);

        sample->sample = WebCore::GStreamerMediaSample::create(buffer, stream->presentationSize, stream->audioTrack ? stream->audioTrack->get()->id() : stream->videoTrack->get()->id());
        sample->stream = stream;
        stream->parent->pendingSamplesAfterInitSegment++;

        g_timeout_add(0, GSourceFunc(webKitWebSrcDidReceiveSample), sample);
    } else {
        PendingReceiveSample* sample = g_new0(PendingReceiveSample, 1);

        sample->buffer = gst_buffer_ref(buffer);
        sample->presentationSize = stream->presentationSize;
        stream->pendingReceiveSample = g_list_append(stream->pendingReceiveSample, sample);
    }
    GST_OBJECT_UNLOCK(stream->parent->parent);

    return GST_PAD_PROBE_OK;
}

static GstPadProbeReturn webKitWebSrcBufferAfterMultiqueueProbe(GstPad* pad, GstPadProbeInfo* info, Stream* stream)
{
    if (!(stream->parent)) return GST_PAD_PROBE_DROP;

    GstPadProbeReturn result;
    GstBuffer* buffer = GST_BUFFER(info->data);

    GST_OBJECT_LOCK(stream->parent->parent);
    GstClockTime duration = stream->parent->parent->priv->duration;
    GST_OBJECT_UNLOCK(stream->parent->parent);

    // If the presentation time of this buffer is beyond the "logical" duration, synthesize EOS.
    // The "logical" duration may be shorter than the "physical" duration that the buffered data can provide,
    // which would throw a natural EOS anyway
    if (GST_BUFFER_PTS_IS_VALID(buffer) && duration && GST_BUFFER_PTS(buffer) > duration) {
        GRefPtr<GstPad> peerPad = adoptGRef(gst_pad_get_peer(pad));
        gst_pad_send_event(peerPad.get(), gst_event_new_eos());
        result = GST_PAD_PROBE_DROP;
    } else
        result = GST_PAD_PROBE_OK;

    return result;
}

static void webKitMediaSrcDemuxerNoMorePads(GstElement*, Source* source);

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

        GST_OBJECT_LOCK(stream->parent->parent);
        stream->presentationSize = WebCore::FloatSize(width, height);
        GST_OBJECT_UNLOCK(stream->parent->parent);
    } else {
        GST_OBJECT_LOCK(stream->parent->parent);
        stream->presentationSize = WebCore::FloatSize();
        GST_OBJECT_UNLOCK(stream->parent->parent);
    }

    gst_caps_ref(caps);
    GST_OBJECT_LOCK(stream->parent->parent);
    if (stream->caps)
        gst_caps_unref(stream->caps);

    stream->caps = caps;
    GST_OBJECT_UNLOCK(stream->parent->parent);
}

static void webKitMediaSrcLinkStreamToSrcPad(GstPad* srcpad, Stream* stream)
{
    Source* source = stream->parent;

    // TODO: Atomic ints, GRefPtr
    GST_OBJECT_LOCK(stream->parent->parent);
    int numberOfPads = source->parent->priv->numberOfPads++;
    GST_OBJECT_UNLOCK(stream->parent->parent);

    gchar* padName = g_strdup_printf("src_%u", numberOfPads);
    GstPad* ghostpad = gst_ghost_pad_new_from_template(padName, srcpad, gst_static_pad_template_get(&srcTemplate));

    gst_pad_set_query_function(ghostpad, webKitMediaSrcQueryWithParent);
    gst_pad_set_event_function(ghostpad, webKitMediaSrcEventWithParent);

    gst_pad_set_element_private(ghostpad, stream);

    gst_pad_set_active(ghostpad, TRUE);
    gst_element_add_pad(GST_ELEMENT(source->parent), ghostpad);

    GST_OBJECT_LOCK(stream->parent->parent);
    stream->srcpad = ghostpad;
    GST_OBJECT_UNLOCK(stream->parent->parent);
}

static gboolean webKitMediaSrcNoDataToDecodeTimeout(Source* source)
{
    GST_OBJECT_LOCK(source->parent);
    source->noDataToDecodeTimeoutTag = 0;
    GST_OBJECT_UNLOCK(source->parent);

    source->parent->priv->mediaSourceClient->didReceiveAllPendingSamples(source->sourceBuffer);

    return FALSE;
}

static void webKitMediaSrcParserNotifyCaps(GObject* object, GParamSpec*, Stream* stream)
{
    GstPad* srcpad = GST_PAD(object);
    GstCaps* caps = gst_pad_get_current_caps(srcpad);

    if (!caps || !stream->parent) {
        return;
    }

    GST_OBJECT_LOCK(stream->parent->parent);
    if (stream->parent->noDataToDecodeTimeoutTag) {
        g_source_remove(stream->parent->noDataToDecodeTimeoutTag);
        stream->parent->noDataToDecodeTimeoutTag = 0;
    }
    GST_OBJECT_UNLOCK(stream->parent->parent);

    webKitMediaSrcUpdatePresentationSize(caps, stream);
    gst_caps_unref(caps);

    // TODO
    if (!gst_pad_is_linked(srcpad))
        webKitMediaSrcLinkStreamToSrcPad(srcpad, stream);

    webKitMediaSrcDemuxerNoMorePads(NULL, stream->parent);
}

static void webKitMediaSrcDemuxerPadAdded(GstElement* demuxer, GstPad* demuxersrcpad, Source* source)
{
    GstCaps* demuxersrcpadcaps = gst_pad_get_current_caps(demuxersrcpad);
    GstStructure* s = gst_caps_get_structure(demuxersrcpadcaps, 0);
    Stream* stream = g_new0(Stream, 1);
    gchar *parserBinName;
    const gchar* demuxersrcpadtypename = gst_structure_get_name(s);

    GST_OBJECT_LOCK(source->parent);
    stream->id = source->parent->priv->numberOfPads; // Just informative
    GST_OBJECT_UNLOCK(source->parent);
    stream->parent = source;
    stream->initSegmentAlreadyProcessed = false;
    stream->type = STREAM_TYPE_UNKNOWN;
    stream->demuxersrcpad = demuxersrcpad;

    parserBinName = g_strdup_printf("streamparser%d", stream->id);

    g_assert(demuxersrcpadcaps != 0);

    if (gst_structure_has_name(s, "video/x-h264")) {
        GstElement* parser;
        GstElement* capsfilter;
        GstPad* pad;
        GstCaps* filtercaps;

        filtercaps = gst_caps_new_simple("video/x-h264", "alignment", G_TYPE_STRING, "au", NULL);
        parser = gst_element_factory_make("h264parse", 0);
        capsfilter = gst_element_factory_make("capsfilter", 0);
        g_object_set(capsfilter, "caps", filtercaps, NULL);
        gst_caps_unref(filtercaps);

        stream->parser = gst_bin_new(parserBinName);
        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, NULL);
        gst_element_link_pads(parser, "src", capsfilter, "sink");

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(capsfilter, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else if (gst_structure_has_name(s, "video/x-h265")) {
        GstElement* parser;
        GstElement* capsfilter;
        GstPad* pad;
        GstCaps* filtercaps;

        filtercaps = gst_caps_new_simple("video/x-h265", "alignment", G_TYPE_STRING, "au", NULL);
        parser = gst_element_factory_make("h265parse", 0);
        capsfilter = gst_element_factory_make("capsfilter", 0);
        g_object_set(capsfilter, "caps", filtercaps, NULL);
        gst_caps_unref(filtercaps);

        stream->parser = gst_bin_new(parserBinName);
        gst_bin_add_many(GST_BIN(stream->parser), parser, capsfilter, NULL);
        gst_element_link_pads(parser, "src", capsfilter, "sink");

        pad = gst_element_get_static_pad(parser, "sink");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("sink", pad));
        gst_object_unref(pad);

        pad = gst_element_get_static_pad(capsfilter, "src");
        gst_element_add_pad(stream->parser, gst_ghost_pad_new("src", pad));
        gst_object_unref(pad);
    } else if (gst_structure_has_name(s, "audio/mpeg")) {
        gint mpegversion = -1;

        gst_structure_get_int(s, "mpegversion", &mpegversion);
        if (mpegversion == 1) {
            stream->parser = gst_element_factory_make("mpegaudioparse", 0);
        } else if (mpegversion == 2 || mpegversion == 4) {
            stream->parser = gst_element_factory_make("aacparse", 0);
        } else {
            g_assert_not_reached();
        }
    }

    g_free(parserBinName);

    GST_OBJECT_LOCK(source->parent);
    source->streams = g_list_prepend(source->streams, stream);
    GST_OBJECT_UNLOCK(source->parent);

    GstPad* sinkpad;
    GstPad* srcpad;

    sinkpad = gst_element_get_request_pad(source->multiqueue, "sink_%u");
    gst_pad_link(demuxersrcpad, sinkpad);

    srcpad = get_internal_linked_pad(sinkpad);
    g_object_ref(srcpad);
    stream->multiqueuesrcpad = srcpad;
    stream->bufferAfterMultiqueueProbeId = gst_pad_add_probe(srcpad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) webKitWebSrcBufferAfterMultiqueueProbe, stream, NULL);
    gst_object_unref(sinkpad);

    stream->bufferProbeId = gst_pad_add_probe(demuxersrcpad, GST_PAD_PROBE_TYPE_BUFFER, (GstPadProbeCallback) webKitWebSrcBufferProbe, stream, NULL);

    webKitMediaSrcUpdatePresentationSize(demuxersrcpadcaps, stream);

    if (stream->parser) {
        gst_bin_add(GST_BIN(source->parent), stream->parser);
        gst_element_sync_state_with_parent(stream->parser);
        sinkpad = gst_element_get_static_pad(stream->parser, "sink");
        gst_pad_link(srcpad, sinkpad);
        gst_object_unref(srcpad);
        gst_object_unref(sinkpad);
        srcpad = gst_element_get_static_pad(stream->parser, "src");
    } else
        webKitMediaSrcUpdatePresentationSize(demuxersrcpadcaps, stream);

    gst_caps_unref(demuxersrcpadcaps);

    g_signal_connect(srcpad, "notify::caps", G_CALLBACK(webKitMediaSrcParserNotifyCaps), stream);
    webKitMediaSrcLinkStreamToSrcPad(srcpad, stream);

    if (g_str_has_prefix(demuxersrcpadtypename, "audio")) {
        GST_OBJECT_LOCK(source->parent);
        stream->type = STREAM_TYPE_AUDIO;
        source->parent->priv->nAudio++;
        GST_OBJECT_UNLOCK(source->parent);
        g_signal_emit(G_OBJECT(source->parent), webkit_media_src_signals[SIGNAL_AUDIO_CHANGED], 0, NULL);
    } else if (g_str_has_prefix(demuxersrcpadtypename, "video")) {
        GST_OBJECT_LOCK(source->parent);
        stream->type = STREAM_TYPE_VIDEO;
        source->parent->priv->nVideo++;
        GST_OBJECT_UNLOCK(source->parent);
        g_signal_emit(G_OBJECT(source->parent), webkit_media_src_signals[SIGNAL_VIDEO_CHANGED], 0, NULL);
    } else if (g_str_has_prefix(demuxersrcpadtypename, "text")) {
        GST_OBJECT_LOCK(source->parent);
        stream->type = STREAM_TYPE_TEXT;
        source->parent->priv->nText++;
        GST_OBJECT_UNLOCK(source->parent);
        g_signal_emit(G_OBJECT(source->parent), webkit_media_src_signals[SIGNAL_TEXT_CHANGED], 0, NULL);
    }

    gst_object_unref(srcpad);

    gst_debug_bin_to_dot_file(GST_BIN(GST_ELEMENT_PARENT(source->parent)), GST_DEBUG_GRAPH_SHOW_ALL, "demuxer-pad-added");
}

static gboolean freeStreamLater(Stream* stream)
{
    if (stream->caps)
        gst_caps_unref(stream->caps);
#if ENABLE(VIDEO_TRACK)
    if (stream->audioTrack)
        stream->audioTrack->clear();
    if (stream->videoTrack)
        stream->videoTrack->clear();
#endif
    if (stream->multiqueuesrcpad)
        g_object_unref(stream->multiqueuesrcpad);

    g_free(stream);

    return G_SOURCE_REMOVE;
}

static gboolean freeSourceLater(Source* source)
{
    g_free(source);

    return G_SOURCE_REMOVE;
}

static void webKitMediaSrcDemuxerPadRemoved(GstElement* demuxer, GstPad* demuxersrcpad, Source* source)
{
    // Locate the right stream
    Stream* stream = 0;
    GstPad* srcpad = 0;
    GstPad* multiqueuesrcpad = 0;
    gulong bufferProbeId = 0;
    gulong bufferAfterMultiqueueProbeId = 0;
    GST_OBJECT_LOCK(source->parent);
    for (GList *l = source->streams; l; l=l->next) {
        Stream* s = (Stream*)l->data;
        if (demuxersrcpad == s->demuxersrcpad) {
            stream = s;
            srcpad = s->srcpad;
            multiqueuesrcpad = s->multiqueuesrcpad;
            bufferProbeId = s->bufferProbeId;
            bufferAfterMultiqueueProbeId = s->bufferAfterMultiqueueProbeId;
            break;
        }
    }
    GST_OBJECT_UNLOCK(source->parent);

    if (stream) {
        if (srcpad)
            gst_pad_set_element_private(srcpad, NULL);

        if (bufferProbeId)
            gst_pad_remove_probe(demuxersrcpad, stream->bufferProbeId);

        if (multiqueuesrcpad && bufferAfterMultiqueueProbeId)
            gst_pad_remove_probe(multiqueuesrcpad, bufferAfterMultiqueueProbeId);

        GST_OBJECT_LOCK(source->parent);
        source->streams = g_list_remove(source->streams, stream);

        switch (stream->type) {
        case STREAM_TYPE_AUDIO:
            source->parent->priv->nAudio--;
            break;
        case STREAM_TYPE_VIDEO:
            source->parent->priv->nVideo--;
            break;
        case STREAM_TYPE_TEXT:
            source->parent->priv->nText--;
            break;
        default:
            break;
        }

        // Some g_idle_added code out there may still need the stream
        stream->parent = NULL;
        GST_OBJECT_UNLOCK(source->parent);

        g_timeout_add(500, (GSourceFunc)freeStreamLater, stream);
    }
}

#if ENABLE(VIDEO_TRACK)
static gboolean webKitMediaSrcDidReceiveInitializationSegment(gpointer userdata)
{
    Source* source = (Source*)userdata;

    GST_OBJECT_LOCK(source->parent);
    if (source->noDataToDecodeTimeoutTag) {
        g_source_remove(source->noDataToDecodeTimeoutTag);
        source->noDataToDecodeTimeoutTag = 0;
    }

    GList* l;
    bool noData = false;
    for (l = source->streams; l; l = l->next) {
        Stream* stream = (Stream*)l->data;
        if (!stream->audioTrack && !stream->videoTrack) {
            noData = true;
            break;
        }
    }
    GST_OBJECT_UNLOCK(source->parent);

    if (noData) {
        // No useful data, but notify anyway to complete the append operation (webKitMediaSrcLastSampleTimeout is cancelled and won't notify in this case)
        source->parent->priv->mediaSourceClient->didReceiveAllPendingSamples(source->sourceBuffer);
        return G_SOURCE_REMOVE;
    }

    // TODO: Locking
    WebCore::SourceBufferPrivateClient::InitializationSegment initializationSegment;

    GST_OBJECT_LOCK(source->parent);
    initializationSegment.duration = MediaTime(source->parent->priv->duration, GST_SECOND);
    for (l = source->streams; l; l = l->next) {
        Stream* stream = (Stream*)l->data;

        if (stream->audioTrack) {
            WebCore::SourceBufferPrivateClient::InitializationSegment::AudioTrackInformation info;
            info.track = *stream->audioTrack;
            info.description = WebCore::GStreamerMediaDescription::create(stream->caps);
            initializationSegment.audioTracks.append(info);
        } else if (stream->videoTrack) {
            WebCore::SourceBufferPrivateClient::InitializationSegment::VideoTrackInformation info;
            info.track = *stream->videoTrack;
            info.description = WebCore::GStreamerMediaDescription::create(stream->caps);
            initializationSegment.videoTracks.append(info);
        } else {
            g_assert_not_reached();
        }
    }
    GST_OBJECT_UNLOCK(source->parent);

    source->parent->priv->mediaSourceClient->didReceiveInitializationSegment(source->sourceBuffer, initializationSegment);

    Vector<RefPtr<WebCore::GStreamerMediaSample> > samples;

    GST_OBJECT_LOCK(source->parent);
    for (l = source->streams; l; l = l->next) {
        Stream* stream = (Stream*)l->data;
        if (stream->initSegmentAlreadyProcessed) continue;

        GList* m;
        for (m = stream->pendingReceiveSample; m; m = m->next) {
            PendingReceiveSample* pending = (PendingReceiveSample*)m->data;
            RefPtr<WebCore::GStreamerMediaSample> sample = WebCore::GStreamerMediaSample::create(pending->buffer, pending->presentationSize, stream->audioTrack ? stream->audioTrack->get()->id() : stream->videoTrack->get()->id());
            samples.append(sample);
            gst_buffer_unref(pending->buffer);
            g_free(pending);
        }
        g_list_free(stream->pendingReceiveSample);
        stream->pendingReceiveSample = NULL;
        stream->initSegmentAlreadyProcessed = true;
    }
    GST_OBJECT_UNLOCK(source->parent);

    for (Vector<RefPtr<WebCore::GStreamerMediaSample> >::iterator it = samples.begin(); it != samples.end(); ++it) {
        RefPtr<WebCore::GStreamerMediaSample> sample = *it;
        source->parent->priv->mediaSourceClient->didReceiveSample(source->sourceBuffer, sample);
    }

    GST_OBJECT_LOCK(source->parent);
    // The timeout on this timestamp is what helps the append operation to be completed
    if (!source->lastSampleTime) {
        g_timeout_add(100, GSourceFunc(webKitMediaSrcLastSampleTimeout), source);
    }
    source->lastSampleTime = g_get_monotonic_time();
    GST_OBJECT_UNLOCK(source->parent);

    return G_SOURCE_REMOVE;
}
#endif

static void webKitMediaSrcDemuxerNoMorePads(GstElement*, Source* source)
{
    GST_OBJECT_LOCK(source->parent);

    GList* l;
    bool allPadsDone = true;

    source->noMorePads = true;
    // TODO: Locking
    for (l = source->parent->priv->sources; l; l = l->next) {
        Source* tmp = (Source*)l->data;

        allPadsDone = allPadsDone && tmp->noMorePads;
        if (!allPadsDone) break;
    }
    if (allPadsDone)
        source->parent->priv->noMorePads = true;
    GST_OBJECT_UNLOCK(source->parent);

    if (allPadsDone) {
        gst_element_no_more_pads(GST_ELEMENT(source->parent));
        webKitMediaSrcDoAsyncDone(source->parent);
    }
}

static void webKitMediaSrcHaveType(GstElement* typefind, guint probability, GstCaps* caps, Source* source)
{
    GST_OBJECT_LOCK(source->parent);
    bool alreadyProcessed = source->demuxer || source->streams;
    GST_OBJECT_UNLOCK(source->parent);

    if (alreadyProcessed) {
        return;
    }

    GstStructure* s = gst_caps_get_structure(caps, 0);
    GstElement* demuxer = NULL;
    GstElement* multiqueue = NULL;

    if (gst_structure_has_name(s, "video/webm") || gst_structure_has_name(s, "audio/webm")) {
        demuxer = gst_element_factory_make("matroskademux", NULL);
    } else if (gst_structure_has_name(s, "video/quicktime") || gst_structure_has_name(s, "audio/x-m4a")
               || gst_structure_has_name(s, "application/x-3gp")) {
        demuxer = gst_element_factory_make("qtdemux", NULL);
    } else if (gst_structure_has_name(s, "video/mpegts")) {
        demuxer = gst_element_factory_make("tsdemux", NULL);
    } else if (gst_structure_has_name(s, "audio/mpeg")) {
        gint mpegversion = -1;

        gst_structure_get_int(s, "mpegversion", &mpegversion);
        if (mpegversion == 1) {
            // TODO: MP3
            g_assert_not_reached();
        } else if (mpegversion == 2 || mpegversion == 4) {
            // TODO: AAC
            g_assert_not_reached();
        } else {
            g_assert_not_reached();
        }
    } else {
        g_assert_not_reached();
    }

    if (demuxer) {
        multiqueue = gst_element_factory_make("multiqueue", NULL);
        g_object_set(G_OBJECT(multiqueue),
                "low-percent", 0,
                "high-percent", 100,
                "max-size-buffers", 0,
                "max-size-bytes", 0,
                "max-size-time", guint64(0),
                NULL);

        GST_OBJECT_LOCK(source->parent);
        source->demuxer = demuxer;
        source->multiqueue = multiqueue;
        GST_OBJECT_UNLOCK(source->parent);

        gst_bin_add_many(GST_BIN(source->parent), source->demuxer, source->multiqueue, NULL);
        gst_element_sync_state_with_parent(source->demuxer);
        gst_element_sync_state_with_parent(source->multiqueue);

        gst_element_link_pads(typefind, "src", source->demuxer, "sink");

        g_signal_connect(demuxer, "pad-added", G_CALLBACK(webKitMediaSrcDemuxerPadAdded), source);
        g_signal_connect(demuxer, "pad-removed", G_CALLBACK(webKitMediaSrcDemuxerPadRemoved), source);
        g_signal_connect(demuxer, "no-more-pads", G_CALLBACK(webKitMediaSrcDemuxerNoMorePads), source);
    } else {
        g_assert_not_reached();
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

namespace WebCore {
PassRefPtr<MediaSourceClientGStreamer> MediaSourceClientGStreamer::create(WebKitMediaSrc* src)
{
    return adoptRef(new MediaSourceClientGStreamer(src));
}

MediaSourceClientGStreamer::MediaSourceClientGStreamer(WebKitMediaSrc* src)
    : RefCounted<MediaSourceClientGStreamer>()
    , m_src(adoptGRef(static_cast<WebKitMediaSrc*>(gst_object_ref(src))))
{
    m_src->priv->mediaSourceClient = this;
}

MediaSourceClientGStreamer::~MediaSourceClientGStreamer()
{
}

MediaSourcePrivate::AddStatus MediaSourceClientGStreamer::addSourceBuffer(PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, const ContentType&)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;

    if (priv->noMorePads) {
        GST_ERROR_OBJECT(m_src.get(), "Adding new source buffers after first data not supported yet");
        return MediaSourcePrivate::NotSupported;
    }

    GST_DEBUG_OBJECT(m_src.get(), "State %d", (int)GST_STATE(m_src.get()));

    GST_OBJECT_LOCK(m_src.get());
    guint numberOfSources = g_list_length(priv->sources);
    GST_OBJECT_UNLOCK(m_src.get());

    Source* source = g_new0(Source, 1);
    GUniquePtr<gchar> srcName(g_strdup_printf("src%u", numberOfSources));
    GUniquePtr<gchar> typefindName(g_strdup_printf("typefind%u", numberOfSources));
    source->parent = m_src.get();
    source->src = gst_element_factory_make("appsrc", srcName.get());
    source->typefind = gst_element_factory_make("typefind", typefindName.get());
    source->noDataToDecodeTimeoutTag = 0;

    g_signal_connect(source->typefind, "have-type", G_CALLBACK(webKitMediaSrcHaveType), source);
    source->sourceBuffer = sourceBufferPrivate.get();

    GST_OBJECT_LOCK(m_src.get());
    priv->sources = g_list_prepend(priv->sources, source);
    GST_OBJECT_UNLOCK(m_src.get());

    gst_bin_add_many(GST_BIN(m_src.get()), source->src, source->typefind, NULL);
    gst_element_link_pads(source->src, "src", source->typefind, "sink");

    gst_element_sync_state_with_parent(source->src);
    gst_element_sync_state_with_parent(source->typefind);

    return MediaSourcePrivate::Ok;
}

void MediaSourceClientGStreamer::durationChanged(const MediaTime& duration)
{
    if (!duration.isValid() || duration.isPositiveInfinite() || duration.isNegativeInfinite())
        return;

    WebKitMediaSrcPrivate* priv = m_src->priv;
    GstClockTime gstDuration;
    if (duration.hasDoubleValue())
        gstDuration = duration.toFloat() * GST_SECOND;
    else
        gstDuration = gst_util_uint64_scale(duration.timeValue(), GST_SECOND, duration.timeScale());

    GST_DEBUG_OBJECT(m_src.get(), "Received duration: %" GST_TIME_FORMAT, GST_TIME_ARGS(gstDuration));

    GST_OBJECT_LOCK(m_src.get());
    if (gstDuration == priv->duration) {
        GST_OBJECT_UNLOCK(m_src.get());
        return;
    }
    priv->duration = gstDuration;
    GST_OBJECT_UNLOCK(m_src.get());
    gst_element_post_message(GST_ELEMENT(m_src.get()), gst_message_new_duration_changed(GST_OBJECT(m_src.get())));
}

bool MediaSourceClientGStreamer::append(PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate, const unsigned char* data, unsigned length)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GstFlowReturn ret = GST_FLOW_OK;
    GstBuffer* buffer;
    Source* source = 0;
    GList *l;
    bool aborted;

    GST_OBJECT_LOCK(m_src.get());
    for (l = priv->sources; l; l = l->next) {
        Source *tmp = static_cast<Source*>(l->data);
        if (tmp->sourceBuffer == sourceBufferPrivate.get()) {
            source = tmp;
            break;
        }
    }
    aborted = sourceBufferPrivate->isAborted();
    sourceBufferPrivate->resetAborted();
    if (aborted && source && source->src) {
        for (l = source->streams; l; l = l->next) {
            Stream *stream = (Stream*)l->data;
            stream->initSegmentAlreadyProcessed = false;
        }
    }
    GST_OBJECT_UNLOCK(m_src.get());

    if (!source || !source->src)
        return false;

    buffer = gst_buffer_new_and_alloc(length);
    gst_buffer_fill(buffer, 0, data, length);

    // Reset parser state after an abort
    if (aborted) {
        if (source->demuxer) {
            GstState pending;
            gst_element_get_state(GST_ELEMENT(source->demuxer), 0, &pending, 250 * GST_NSECOND);
            gst_element_set_state(GST_ELEMENT(source->demuxer), GST_STATE_READY);
            gst_element_set_state(GST_ELEMENT(source->demuxer), pending);
        }
    }

    GST_OBJECT_LOCK(m_src.get());
    source->lastSampleTime = 0;
    source->pendingSamplesAfterInitSegment = 0;

    ASSERT(source->noDataToDecodeTimeoutTag == 0);

    source->noDataToDecodeTimeoutTag = g_timeout_add(1000, GSourceFunc(webKitMediaSrcNoDataToDecodeTimeout), source);
    GST_OBJECT_UNLOCK(m_src.get());

    ret = gst_app_src_push_buffer(GST_APP_SRC(source->src), buffer);

    return (ret == GST_FLOW_OK);
}

void MediaSourceClientGStreamer::markEndOfStream(MediaSourcePrivate::EndOfStreamStatus status)
{
    WebKitMediaSrcPrivate* priv = m_src->priv;
    GList *l;

    GST_DEBUG_OBJECT(m_src.get(), "Have EOS");

    GST_OBJECT_LOCK(m_src.get());
    bool noMorePads = priv->noMorePads;
    if (!noMorePads) {
        priv->noMorePads = true;
    }
    GST_OBJECT_UNLOCK(m_src.get());

    if (!noMorePads) {
        gst_element_no_more_pads(GST_ELEMENT(m_src.get()));
        webKitMediaSrcDoAsyncDone(m_src.get());
    }

    Vector<GstAppSrc*> appSrcs;

    GST_OBJECT_LOCK(m_src.get());
    for (l = priv->sources; l; l = l->next) {
        Source *source = static_cast<Source*>(l->data);
        if (source->src)
            appSrcs.append(GST_APP_SRC(source->src));
    }
    GST_OBJECT_UNLOCK(m_src.get());

    for (Vector<GstAppSrc*>::iterator it = appSrcs.begin(); it != appSrcs.end(); ++it)
        gst_app_src_end_of_stream(*it);
}

void MediaSourceClientGStreamer::removedFromMediaSource(PassRefPtr<SourceBufferPrivateGStreamer> sourceBufferPrivate)
{
    GST_OBJECT_LOCK(m_src.get());
    WebKitMediaSrcPrivate* priv = m_src->priv;
    Source* source = 0;
    GList *l;

    for (l = priv->sources; l; l = l->next) {
        Source *tmp = static_cast<Source*>(l->data);
        if (tmp->sourceBuffer == sourceBufferPrivate.get()) {
            source = tmp;
            break;
        }
    }
    GST_OBJECT_UNLOCK(m_src.get());

    if (source) {
        if (source->src)
            gst_app_src_end_of_stream(GST_APP_SRC(source->src));

        // Force webKitMediaSrcLastSampleTimeout to cancel itself
        source->lastSampleTime = 0;
        source->pendingSamplesAfterInitSegment = 0;

        if (source->typefind) {
            g_signal_handlers_disconnect_by_func(source->typefind, (gpointer)webKitMediaSrcHaveType, source);
            source->typefind = NULL;
        }

        if (source->demuxer) {
            g_signal_handlers_disconnect_by_func(source->demuxer, (gpointer)webKitMediaSrcDemuxerPadAdded, source);
            g_signal_handlers_disconnect_by_func(source->demuxer, (gpointer)webKitMediaSrcDemuxerPadRemoved, source);
            g_signal_handlers_disconnect_by_func(source->demuxer, (gpointer)webKitMediaSrcDemuxerNoMorePads, source);
            source->demuxer = NULL;
        }

        if (source->noDataToDecodeTimeoutTag) {
            g_source_remove(source->noDataToDecodeTimeoutTag);
            source->noDataToDecodeTimeoutTag = 0;
        }

        g_timeout_add(300, (GSourceFunc)freeSourceLater, source);
    }
}

#if ENABLE(VIDEO_TRACK)
void MediaSourceClientGStreamer::didReceiveInitializationSegment(SourceBufferPrivateGStreamer* sourceBuffer, const SourceBufferPrivateClient::InitializationSegment& initializationSegment)
{
    sourceBuffer->didReceiveInitializationSegment(initializationSegment);
}

void MediaSourceClientGStreamer::didReceiveSample(SourceBufferPrivateGStreamer* sourceBuffer, PassRefPtr<MediaSample> sample)
{
    sourceBuffer->didReceiveSample(sample);
}

void MediaSourceClientGStreamer::didReceiveAllPendingSamples(SourceBufferPrivateGStreamer* sourceBuffer)
{
    sourceBuffer->didReceiveAllPendingSamples();
}
#endif

};

GstPad* webkit_media_src_get_audio_pad(WebKitMediaSrc* src, guint i)
{
    GST_OBJECT_LOCK(src);

    GstPad* result = NULL;
    guint n = 0;
    for (GList* sources = src->priv->sources; sources && !result; sources = sources->next) {
        Source* source = (Source*)sources->data;
        for (GList* streams = source->streams; streams; streams = streams->next) {
            Stream* stream = (Stream*)streams->data;
            if (stream->type == STREAM_TYPE_AUDIO) {
                if (n == i) {
                    result = stream->demuxersrcpad;
                    break;
                } else
                    n++;
            }
        }
    }

    GST_OBJECT_UNLOCK(src);

    return result;
}

GstPad* webkit_media_src_get_video_pad(WebKitMediaSrc* src, guint i)
{
    GST_OBJECT_LOCK(src);

    GstPad* result = NULL;
    guint n = 0;
    for (GList* sources = src->priv->sources; sources && !result; sources = sources->next) {
        Source* source = (Source*)sources->data;
        for (GList* streams = source->streams; streams; streams = streams->next) {
            Stream* stream = (Stream*)streams->data;
            if (stream->type == STREAM_TYPE_VIDEO) {
                if (n == i) {
                    result = stream->demuxersrcpad;
                    break;
                } else
                    n++;
            }
        }
    }

    GST_OBJECT_UNLOCK(src);

    return result;
}

GstPad* webkit_media_src_get_text_pad(WebKitMediaSrc* src, guint i)
{
    GST_OBJECT_LOCK(src);

    GstPad* result = NULL;
    guint n = 0;
    for (GList* sources = src->priv->sources; sources && !result; sources = sources->next) {
        Source* source = (Source*)sources->data;
        for (GList* streams = source->streams; streams; streams = streams->next) {
            Stream* stream = (Stream*)streams->data;
            if (stream->type == STREAM_TYPE_TEXT) {
                if (n == i) {
                    result = stream->demuxersrcpad;
                    break;
                } else
                    n++;
            }
        }
    }

    GST_OBJECT_UNLOCK(src);

    return result;
}

// Pad NUST be the WebKitMediaSrc demuxer pad (aka: stream->demuxersrcpad) associated with the added track
void webkit_media_src_track_added(WebKitMediaSrc* src, GstPad* pad, GstEvent* event)
{
    // Find the stream->srcpad (aka: ghostpad) associated with the provided demuxersrcpad
    GST_OBJECT_LOCK(src);
    GstPad* srcpad = NULL;

    for (GList* sources = src->priv->sources; sources && !srcpad; sources = sources->next) {
        Source* source = (Source*)sources->data;
        for (GList* streams = source->streams; streams; streams = streams->next) {
            Stream* stream = (Stream*)streams->data;
            if (stream->demuxersrcpad == pad) {
                srcpad = stream->srcpad;
                break;
            }
        }
    }
    GST_OBJECT_UNLOCK(src);

    ASSERT(srcpad);

    webKitMediaSrcEventWithParent(srcpad, NULL, event);
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

