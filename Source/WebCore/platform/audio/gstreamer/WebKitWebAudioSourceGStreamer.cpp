/*
 *  Copyright (C) 2011 Igalia S.L
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

#include "WebKitWebAudioSourceGStreamer.h"

#if ENABLE(WEB_AUDIO) && USE(GSTREAMER)

#include "AudioBus.h"
#include "AudioIOCallback.h"
#include <wtf/gobject/GOwnPtr.h>
#include "GRefPtrGStreamer.h"
#include <gst/audio/multichannel.h>
#include <gst/pbutils/pbutils.h>

using namespace WebCore;

typedef struct _WebKitWebAudioSrcClass   WebKitWebAudioSrcClass;
typedef struct _WebKitWebAudioSourcePrivate WebKitWebAudioSourcePrivate;

struct _WebKitWebAudioSrc {
    GstBin parent;

    WebKitWebAudioSourcePrivate* priv;
};

struct _WebKitWebAudioSrcClass {
    GstBinClass parentClass;
};

struct _WebKitWebAudioSourcePrivate {
    gfloat sampleRate;
    AudioBus* bus;
    AudioIOCallback* provider;
    guint framesToPull;
    guint64 currentBufferOffset;

    GRefPtr<GstElement> interleave;
    GRefPtr<GstElement> wavEncoder;

    GRefPtr<GstTask> task;
    GStaticRecMutex* mutex;

    GSList* pads; // List of queue sink pads. One queue for each planar audio channel.
    GstPad* sourcePad; // src pad of the element, interleaved wav data is pushed to it.
};

enum {
    PROP_RATE = 1,
    PROP_BUS,
    PROP_PROVIDER,
    PROP_FRAMES
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_ALWAYS,
                                                                  GST_STATIC_CAPS("audio/x-wav"));

GST_DEBUG_CATEGORY_STATIC(webkit_web_audio_src_debug);
#define GST_CAT_DEFAULT webkit_web_audio_src_debug

static void webKitWebAudioSrcConstructed(GObject*);
static void webKitWebAudioSrcFinalize(GObject*);
static void webKitWebAudioSrcSetProperty(GObject*, guint propertyId, const GValue*, GParamSpec*);
static void webKitWebAudioSrcGetProperty(GObject*, guint propertyId, GValue*, GParamSpec*);
static GstStateChangeReturn webKitWebAudioSrcChangeState(GstElement*, GstStateChange);
static void webKitWebAudioSrcLoop(WebKitWebAudioSrc*);

static GstCaps* getGStreamerMonoAudioCaps(float sampleRate)
{
    return gst_caps_new_simple("audio/x-raw-float", "rate", G_TYPE_INT, static_cast<int>(sampleRate),
                               "channels", G_TYPE_INT, 1,
                               "endianness", G_TYPE_INT, G_BYTE_ORDER,
                               "width", G_TYPE_INT, 32, NULL);
}

static GstAudioChannelPosition webKitWebAudioGStreamerChannelPosition(int channelIndex)
{
    GstAudioChannelPosition position = GST_AUDIO_CHANNEL_POSITION_NONE;

    switch (channelIndex) {
    case AudioBus::ChannelLeft:
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_LEFT;
        break;
    case AudioBus::ChannelRight:
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_RIGHT;
        break;
    case AudioBus::ChannelCenter:
        // Center and mono are the same.
        position = GST_AUDIO_CHANNEL_POSITION_FRONT_MONO;
        break;
    case AudioBus::ChannelLFE:
        position = GST_AUDIO_CHANNEL_POSITION_LFE;
        break;
    case AudioBus::ChannelSurroundLeft:
        position = GST_AUDIO_CHANNEL_POSITION_REAR_LEFT;
        break;
    case AudioBus::ChannelSurroundRight:
        position = GST_AUDIO_CHANNEL_POSITION_REAR_RIGHT;
        break;
    default:
        break;
    };

    return position;
}

#define webkit_web_audio_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(WebKitWebAudioSrc, webkit_web_audio_src, GST_TYPE_BIN, GST_DEBUG_CATEGORY_INIT(webkit_web_audio_src_debug, \
                            "webkitwebaudiosrc", \
                            0, \
                            "webaudiosrc element"));

static void webkit_web_audio_src_class_init(WebKitWebAudioSrcClass* webKitWebAudioSrcClass)
{
    GObjectClass* objectClass = G_OBJECT_CLASS(webKitWebAudioSrcClass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(webKitWebAudioSrcClass);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));
    gst_element_class_set_details_simple(elementClass,
                                         "WebKit WebAudio source element",
                                         "Source",
                                         "Handles WebAudio data from WebCore",
                                         "Philippe Normand <pnormand@igalia.com>");

    objectClass->constructed = webKitWebAudioSrcConstructed;
    objectClass->finalize = webKitWebAudioSrcFinalize;
    elementClass->change_state = webKitWebAudioSrcChangeState;

    objectClass->set_property = webKitWebAudioSrcSetProperty;
    objectClass->get_property = webKitWebAudioSrcGetProperty;

    GParamFlags flags = static_cast<GParamFlags>(G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);
    g_object_class_install_property(objectClass,
                                    PROP_RATE,
                                    g_param_spec_float("rate", "rate",
                                                       "Sample rate", G_MINDOUBLE, G_MAXDOUBLE,
                                                       44100.0, flags));

    g_object_class_install_property(objectClass,
                                    PROP_BUS,
                                    g_param_spec_pointer("bus", "bus",
                                                         "Bus", flags));

    g_object_class_install_property(objectClass,
                                    PROP_PROVIDER,
                                    g_param_spec_pointer("provider", "provider",
                                                         "Provider", flags));

    g_object_class_install_property(objectClass,
                                    PROP_FRAMES,
                                    g_param_spec_uint("frames", "frames",
                                                      "Number of audio frames to pull at each iteration",
                                                      0, G_MAXUINT8, 128, flags));

    g_type_class_add_private(webKitWebAudioSrcClass, sizeof(WebKitWebAudioSourcePrivate));
}

static void webkit_web_audio_src_init(WebKitWebAudioSrc* src)
{
    WebKitWebAudioSourcePrivate* priv = G_TYPE_INSTANCE_GET_PRIVATE(src, WEBKIT_TYPE_WEB_AUDIO_SRC, WebKitWebAudioSourcePrivate);
    src->priv = priv;
    new (priv) WebKitWebAudioSourcePrivate();

    GRefPtr<GstPadTemplate> padTemplate = adoptGRef(gst_static_pad_template_get(&srcTemplate));
    priv->sourcePad = gst_ghost_pad_new_no_target_from_template("src", padTemplate.get());
    gst_element_add_pad(GST_ELEMENT(src), priv->sourcePad);

    priv->provider = 0;
    priv->bus = 0;
    priv->currentBufferOffset = 0;

    priv->mutex = g_new(GStaticRecMutex, 1);
    g_static_rec_mutex_init(priv->mutex);

    priv->task = gst_task_create(reinterpret_cast<GstTaskFunction>(webKitWebAudioSrcLoop), src);
    gst_task_set_lock(priv->task.get(), priv->mutex);
}

static void webKitWebAudioSrcConstructed(GObject* object)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    ASSERT(priv->bus);
    ASSERT(priv->provider);
    ASSERT(priv->sampleRate);

    priv->interleave = gst_element_factory_make("interleave", 0);
    priv->wavEncoder = gst_element_factory_make("wavenc", 0);

    if (!priv->interleave) {
        GST_ERROR_OBJECT(src, "Failed to create interleave");
        return;
    }

    if (!priv->wavEncoder) {
        GST_ERROR_OBJECT(src, "Failed to create wavenc");
        return;
    }

    gst_bin_add_many(GST_BIN(src), priv->interleave.get(), priv->wavEncoder.get(), NULL);
    gst_element_link_pads_full(priv->interleave.get(), "src", priv->wavEncoder.get(), "sink", GST_PAD_LINK_CHECK_NOTHING);


    // For each channel of the bus create a new upstream branch for interleave, like:
    // queue ! capsfilter ! audioconvert. which is plugged to a new interleave request sinkpad.
    for (unsigned channelIndex = 0; channelIndex < priv->bus->numberOfChannels(); channelIndex++) {
        GstElement* queue = gst_element_factory_make("queue", 0);
        GstElement* capsfilter = gst_element_factory_make("capsfilter", 0);
        GstElement* audioconvert = gst_element_factory_make("audioconvert", 0);

        GRefPtr<GstCaps> monoCaps = adoptGRef(getGStreamerMonoAudioCaps(priv->sampleRate));
        g_object_set(capsfilter, "caps", monoCaps.get(), NULL);

        // Configure the queue for minimal latency.
        g_object_set(queue, "max-size-buffers", static_cast<guint>(1), NULL);

        GstPad* pad = gst_element_get_static_pad(queue, "sink");
        priv->pads = g_slist_prepend(priv->pads, pad);

        gst_bin_add_many(GST_BIN(src), queue, capsfilter, audioconvert, NULL);
        gst_element_link_pads_full(queue, "src", capsfilter, "sink", GST_PAD_LINK_CHECK_NOTHING);
        gst_element_link_pads_full(capsfilter, "src", audioconvert, "sink", GST_PAD_LINK_CHECK_NOTHING);

        GRefPtr<GstPad> srcPad = adoptGRef(gst_element_get_static_pad(audioconvert, "src"));
        GRefPtr<GstPad> sinkPad = adoptGRef(gst_element_get_request_pad(priv->interleave.get(), "sink%d"));
        gst_pad_link(srcPad.get(), sinkPad.get());

    }
    priv->pads = g_slist_reverse(priv->pads);


    // wavenc's src pad is the only visible pad of our element.
    GRefPtr<GstPad> targetPad = adoptGRef(gst_element_get_static_pad(priv->wavEncoder.get(), "src"));
    gst_ghost_pad_set_target(GST_GHOST_PAD(priv->sourcePad), targetPad.get());
}

static void webKitWebAudioSrcFinalize(GObject* object)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    g_static_rec_mutex_free(priv->mutex);

    g_slist_free_full(priv->pads, reinterpret_cast<GDestroyNotify>(gst_object_unref));

    priv->~WebKitWebAudioSourcePrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, ((GObject* )(src)));
}

static void webKitWebAudioSrcSetProperty(GObject* object, guint propertyId, const GValue* value, GParamSpec* pspec)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    switch (propertyId) {
    case PROP_RATE:
        priv->sampleRate = g_value_get_float(value);
        break;
    case PROP_BUS:
        priv->bus = static_cast<AudioBus*>(g_value_get_pointer(value));
        break;
    case PROP_PROVIDER:
        priv->provider = static_cast<AudioIOCallback*>(g_value_get_pointer(value));
        break;
    case PROP_FRAMES:
        priv->framesToPull = g_value_get_uint(value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static void webKitWebAudioSrcGetProperty(GObject* object, guint propertyId, GValue* value, GParamSpec* pspec)
{
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(object);
    WebKitWebAudioSourcePrivate* priv = src->priv;

    switch (propertyId) {
    case PROP_RATE:
        g_value_set_float(value, priv->sampleRate);
        break;
    case PROP_BUS:
        g_value_set_pointer(value, priv->bus);
        break;
    case PROP_PROVIDER:
        g_value_set_pointer(value, priv->provider);
        break;
    case PROP_FRAMES:
        g_value_set_uint(value, priv->framesToPull);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, propertyId, pspec);
        break;
    }
}

static void webKitWebAudioSrcLoop(WebKitWebAudioSrc* src)
{
    WebKitWebAudioSourcePrivate* priv = src->priv;

    ASSERT(priv->bus);
    ASSERT(priv->provider);
    if (!priv->provider || !priv->bus)
        return;

    // FIXME: Add support for local/live audio input.
    priv->provider->render(0, priv->bus, priv->framesToPull);

    unsigned bufferSize = priv->framesToPull * sizeof(float);
    for (unsigned index = 0; index < g_slist_length(priv->pads); index++) {
        GstPad* pad = static_cast<GstPad*>(g_slist_nth_data(priv->pads, index));

        GstBuffer* buffer = gst_buffer_new();
        ASSERT(buffer);
        ASSERT(!GST_BUFFER_MALLOCDATA(buffer));

        GST_BUFFER_DATA(buffer) = reinterpret_cast<guint8*>(const_cast<float*>(priv->bus->channel(index)->data()));
        GST_BUFFER_SIZE(buffer) = bufferSize;
        GST_BUFFER_OFFSET(buffer) = priv->currentBufferOffset;
        GST_BUFFER_OFFSET_END(buffer) = priv->currentBufferOffset + priv->framesToPull;

        GRefPtr<GstCaps> monoCaps = adoptGRef(getGStreamerMonoAudioCaps(priv->sampleRate));
        GstStructure* structure = gst_caps_get_structure(monoCaps.get(), 0);
        GstAudioChannelPosition channelPosition = webKitWebAudioGStreamerChannelPosition(index);
        gst_audio_set_channel_positions(structure, &channelPosition);
        gst_buffer_set_caps(buffer, monoCaps.get());

        gst_pad_chain(pad, buffer);
    }

    priv->currentBufferOffset += priv->framesToPull;
}

static GstStateChangeReturn webKitWebAudioSrcChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn returnValue = GST_STATE_CHANGE_SUCCESS;
    WebKitWebAudioSrc* src = WEBKIT_WEB_AUDIO_SRC(element);

    switch (transition) {
    case GST_STATE_CHANGE_NULL_TO_READY:
        if (!src->priv->interleave) {
            gst_element_post_message(element, gst_missing_element_message_new(element, "interleave"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no interleave"));
            return GST_STATE_CHANGE_FAILURE;
        }
        if (!src->priv->wavEncoder) {
            gst_element_post_message(element, gst_missing_element_message_new(element, "wavenc"));
            GST_ELEMENT_ERROR(src, CORE, MISSING_PLUGIN, (0), ("no wavenc"));
            return GST_STATE_CHANGE_FAILURE;
        }
        break;
    default:
        break;
    }

    returnValue = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    if (UNLIKELY(returnValue == GST_STATE_CHANGE_FAILURE)) {
        GST_DEBUG_OBJECT(src, "State change failed");
        return returnValue;
    }

    switch (transition) {
    case GST_STATE_CHANGE_READY_TO_PAUSED:
        GST_DEBUG_OBJECT(src, "READY->PAUSED");
        if (!gst_task_start(src->priv->task.get()))
            returnValue = GST_STATE_CHANGE_FAILURE;
        break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(src, "PAUSED->READY");
        if (!gst_task_join(src->priv->task.get()))
            returnValue = GST_STATE_CHANGE_FAILURE;
        break;
    default:
        break;
    }

    return returnValue;
}

#endif // ENABLE(WEB_AUDIO) && USE(GSTREAMER)
