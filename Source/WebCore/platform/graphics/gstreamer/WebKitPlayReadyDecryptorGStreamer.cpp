/* GStreamer PlayReady decryptor
 *
 * Copyright (C) 2015 Igalia S.L
 * Copyright (C) 2015 Metrological
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include "config.h"

#if (ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)) && USE(GSTREAMER) && USE(DXDRM)
#include "WebKitPlayReadyDecryptorGStreamer.h"

#include "DiscretixSession.h"

#include <gst/base/gstbasetransform.h>
#include <gst/base/gstbytereader.h>

struct _WebKitMediaPlayReadyDecrypt {
    GstBaseTransform parent;
    WebCore::DiscretixSession* sessionMetaData;
    gboolean streamReceived;

    GMutex mutex;
    GCond condition;

    GstEvent* protectionEvent;
    GstBuffer* initDataBuffer;
};

struct _WebKitMediaPlayReadyDecryptClass {
    GstBaseTransformClass parentClass;
};

static GstCaps* webkitMediaPlayReadyDecryptTransformCaps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps* filter);
static GstFlowReturn webkitMediaPlayReadyDecryptTransformInPlace(GstBaseTransform*, GstBuffer*);
static gboolean webkitMediaPlayReadyDecryptSinkEventHandler(GstBaseTransform*, GstEvent*);
static GstStateChangeReturn webKitMediaPlayReadyDecryptChangeState(GstElement* element, GstStateChange transition);

GST_DEBUG_CATEGORY(webkit_media_playready_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_playready_decrypt_debug_category

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" PLAYREADY_PROTECTION_SYSTEM_ID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" PLAYREADY_PROTECTION_SYSTEM_ID
        ));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264; audio/mpeg"));

#define webkit_media_playready_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaPlayReadyDecrypt, webkit_media_playready_decrypt, GST_TYPE_BASE_TRANSFORM);

static void webkit_media_playready_decrypt_dispose(GObject*);

static void webkit_media_playready_decrypt_class_init(WebKitMediaPlayReadyDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    GstBaseTransformClass* baseTransformClass = GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);

    gobjectClass->dispose = webkit_media_playready_decrypt_dispose;
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using PlayReady Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using PlayReady Encryption.",
        "Philippe Normand <philn@igalia.com>");

    GST_DEBUG_CATEGORY_INIT(webkit_media_playready_decrypt_debug_category,
        "webkitplayready", 0, "PlayReady decryptor");

    elementClass->change_state = GST_DEBUG_FUNCPTR(webKitMediaPlayReadyDecryptChangeState);

    baseTransformClass->transform_ip = GST_DEBUG_FUNCPTR(webkitMediaPlayReadyDecryptTransformInPlace);
    baseTransformClass->transform_caps = GST_DEBUG_FUNCPTR(webkitMediaPlayReadyDecryptTransformCaps);
    baseTransformClass->sink_event = GST_DEBUG_FUNCPTR(webkitMediaPlayReadyDecryptSinkEventHandler);
    baseTransformClass->transform_ip_on_passthrough = FALSE;
}

static void webkit_media_playready_decrypt_init(WebKitMediaPlayReadyDecrypt* self)
{
    GstBaseTransform* base = GST_BASE_TRANSFORM(self);

    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);

    g_mutex_init(&self->mutex);
    g_cond_init(&self->condition);
}

static void webkit_media_playready_decrypt_dispose(GObject* object)
{
    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(object);

    if (self->protectionEvent) {
        gst_event_unref(self->protectionEvent);
        self->protectionEvent = nullptr;
    }

    g_mutex_clear(&self->mutex);
    g_cond_clear(&self->condition);

    G_OBJECT_CLASS(parent_class)->dispose(object);
}

/*
  Append new_structure to dest, but only if it does not already exist in res.
  This function takes ownership of new_structure.
*/
static bool webkitMediaPlayReadyDecryptCapsAppendIfNotDuplicate(GstCaps* destination, GstStructure* structure)
{
    bool duplicate = false;
    unsigned size = gst_caps_get_size(destination);
    for (unsigned index = 0; !duplicate && index < size; ++index) {
        GstStructure* s = gst_caps_get_structure(destination, index);
        if (gst_structure_is_equal(s, structure))
            duplicate = true;
    }

    if (!duplicate)
        gst_caps_append_structure(destination, structure);
    else
        gst_structure_free(structure);

    return duplicate;
}

static GstCaps* webkitMediaPlayReadyDecryptTransformCaps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    g_return_val_if_fail(direction != GST_PAD_UNKNOWN, nullptr);
    GstCaps* transformedCaps = gst_caps_new_empty();

    GST_LOG_OBJECT(base, "direction: %s, caps: %" GST_PTR_FORMAT " filter:"
        " %" GST_PTR_FORMAT, (direction == GST_PAD_SRC) ? "src" : "sink", caps, filter);

    unsigned size = gst_caps_get_size(caps);
    for (unsigned i = 0; i < size; ++i) {
        GstStructure* in = gst_caps_get_structure(caps, i);
        GstStructure* out = nullptr;

        if (direction == GST_PAD_SINK) {
            if (!gst_structure_has_field(in, "original-media-type"))
                continue;

            out = gst_structure_copy(in);
            gst_structure_set_name(out, gst_structure_get_string(out, "original-media-type"));

            /* filter out the DRM related fields from the down-stream caps */
            for (int j = 0; j < gst_structure_n_fields(in); ++j) {
                const gchar* fieldName = gst_structure_nth_field_name(in, j);

                if (g_str_has_prefix(fieldName, "protection-system")
                    || g_str_has_prefix(fieldName, "original-media-type"))
                    gst_structure_remove_field(out, fieldName);
            }
        } else {
            GstStructure* tmp = gst_structure_copy(in);
            /* filter out the video related fields from the up-stream caps,
               because they are not relevant to the input caps of this element and
               can cause caps negotiation failures with adaptive bitrate streams */
            for (int index = gst_structure_n_fields(tmp) - 1; index >= 0; --index) {
                const gchar* fieldName = gst_structure_nth_field_name(tmp, index);
                GST_TRACE("Check field \"%s\" for removal", fieldName);

                if (!g_strcmp0(fieldName, "base-profile")
                    || !g_strcmp0(fieldName, "codec_data")
                    || !g_strcmp0(fieldName, "height")
                    || !g_strcmp0(fieldName, "framerate")
                    || !g_strcmp0(fieldName, "level")
                    || !g_strcmp0(fieldName, "pixel-aspect-ratio")
                    || !g_strcmp0(fieldName, "profile")
                    || !g_strcmp0(fieldName, "rate")
                    || !g_strcmp0(fieldName, "width")) {
                    gst_structure_remove_field(tmp, fieldName);
                    GST_TRACE("Removing field %s", fieldName);
                }
            }

            out = gst_structure_copy(tmp);
            gst_structure_set(out, "protection-system", G_TYPE_STRING, PLAYREADY_PROTECTION_SYSTEM_ID,
                "original-media-type", G_TYPE_STRING, gst_structure_get_name(in), nullptr);

            gst_structure_set_name(out, "application/x-cenc");
            gst_structure_free(tmp);
        }

        webkitMediaPlayReadyDecryptCapsAppendIfNotDuplicate(transformedCaps, out);
    }

    if (filter) {
        GstCaps* intersection;

        GST_LOG_OBJECT(base, "Using filter caps %" GST_PTR_FORMAT, filter);
        intersection = gst_caps_intersect_full(transformedCaps, filter, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(transformedCaps);
        transformedCaps = intersection;
    }

    GST_LOG_OBJECT(base, "returning %" GST_PTR_FORMAT, transformedCaps);
    return transformedCaps;
}

static GstFlowReturn webkitMediaPlayReadyDecryptTransformInPlace(GstBaseTransform* base, GstBuffer* buffer)
{
    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(base);
    GstFlowReturn result = GST_FLOW_OK;
    GstMapInfo map;
    const GValue* value;
    guint sampleIndex = 0;
    int errorCode;
    uint32_t trackID = 0;
    GstPad* pad;
    GstCaps* caps;
    GstMapInfo boxMap;
    GstBuffer* box = nullptr;
    GstProtectionMeta* protectionMeta = 0;
    gboolean boxMapped = FALSE;
    gboolean bufferMapped = FALSE;

    GST_TRACE_OBJECT(self, "Processing buffer");
    g_mutex_lock(&self->mutex);
    GST_TRACE_OBJECT(self, "Mutex acquired, stream received: %s", self->streamReceived ? "yes":"no");

    // The key might not have been received yet. Wait for it.
    if (!self->streamReceived)
        g_cond_wait(&self->condition, &self->mutex);

    if (!self->streamReceived) {
        GST_DEBUG_OBJECT(self, "Condition signaled from state change transition. Aborting.");
        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    GST_TRACE_OBJECT(self, "Proceeding with decryption");
    protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
    if (!protectionMeta || !buffer) {
        if (!protectionMeta)
            GST_ERROR_OBJECT(self, "Failed to get GstProtection metadata from buffer %p", buffer);

        if (!buffer)
            GST_ERROR_OBJECT(self, "Failed to get writable buffer");

        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    bufferMapped = gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE));
    if (!bufferMapped) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    pad = gst_element_get_static_pad(GST_ELEMENT(self), "src");
    caps = gst_pad_get_current_caps(pad);
    if (g_str_has_prefix(gst_structure_get_name(gst_caps_get_structure(caps, 0)), "video/"))
        trackID = 1;
    else
        trackID = 2;
    gst_caps_unref(caps);
    gst_object_unref(pad);

    if (!gst_structure_get_uint(protectionMeta->info, "sample-index", &sampleIndex)) {
        GST_ERROR_OBJECT(self, "failed to get sample-index");
        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    value = gst_structure_get_value(protectionMeta->info, "box");
    if (!value) {
        GST_ERROR_OBJECT(self, "Failed to get encryption box for sample");
        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    box = gst_value_get_buffer(value);
    boxMapped = gst_buffer_map(box, &boxMap, GST_MAP_READ);
    if (!boxMapped) {
        GST_ERROR_OBJECT(self, "Failed to map encryption box");
        result = GST_FLOW_NOT_SUPPORTED;
        goto beach;
    }

    GST_TRACE_OBJECT(self, "decrypt sample %u", sampleIndex);
    if ((errorCode = self->sessionMetaData->decrypt(static_cast<void*>(map.data), static_cast<uint32_t>(map.size),
        static_cast<void*>(boxMap.data), static_cast<uint32_t>(boxMap.size), static_cast<uint32_t>(sampleIndex), trackID))) {
        GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
        GST_MEMDUMP_OBJECT(self, "box", boxMap.data, boxMap.size);
        result = GST_FLOW_ERROR;
        goto beach;
    }

beach:
    if (boxMapped)
        gst_buffer_unmap(box, &boxMap);

    if (bufferMapped)
        gst_buffer_unmap(buffer, &map);

    if (protectionMeta)
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));

    GST_TRACE_OBJECT(self, "Unlocking mutex");
    g_mutex_unlock(&self->mutex);
    return result;
}

static gboolean requestKey(gpointer userData)
{
    if (!WEBKIT_IS_MEDIA_PLAYREADY_DECRYPT(userData))
        return G_SOURCE_REMOVE;

    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(userData);
    GST_DEBUG_OBJECT(self, "posting drm-key-needed message");
    gst_element_post_message(GST_ELEMENT(self),
        gst_message_new_element(GST_OBJECT(self),
            gst_structure_new("drm-key-needed", "data", GST_TYPE_BUFFER, self->initDataBuffer,
                "key-system-id", G_TYPE_STRING, "com.microsoft.playready", nullptr)));

    if (self->protectionEvent) {
        gst_event_unref(self->protectionEvent);
        self->protectionEvent = nullptr;
    }
    return G_SOURCE_REMOVE;
}

static gboolean webkitMediaPlayReadyDecryptSinkEventHandler(GstBaseTransform* trans, GstEvent* event)
{
    gboolean result = FALSE;
    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(trans);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        const gchar* systemId;
        const gchar* origin;
        GstBuffer* initdatabuffer;

        GST_INFO_OBJECT(self, "received protection event");
        gst_event_parse_protection(event, &systemId, &initdatabuffer, &origin);
        GST_DEBUG_OBJECT(self, "systemId: %s", systemId);
        if (!g_str_equal(systemId, PLAYREADY_PROTECTION_SYSTEM_ID)
            || !g_str_has_prefix(origin, "smooth-streaming")) {
            gst_event_unref(event);
            result = FALSE;
            break;
        }

        // Keep the event ref around so that the parsed event data
        // remains valid until the drm-key-need message has been sent.
        self->protectionEvent = event;
        self->initDataBuffer = initdatabuffer;
        g_timeout_add(0, requestKey, self);

        result = TRUE;
        break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM_OOB: {
        GST_INFO_OBJECT(self, "received OOB event");
        g_mutex_lock(&self->mutex);
        const GstStructure* structure = gst_event_get_structure(event);
        if (gst_structure_has_name(structure, "dxdrm-session")) {
            GST_INFO_OBJECT(self, "received dxdrm session");

            const GValue* value = gst_structure_get_value(structure, "session");
            self->sessionMetaData = reinterpret_cast<WebCore::DiscretixSession*>(g_value_get_pointer(value));
            self->streamReceived = TRUE;
            g_cond_signal(&self->condition);
        }

        g_mutex_unlock(&self->mutex);
        gst_event_unref(event);
        result = TRUE;
        break;
    }
    default:
        result = GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
        break;
    }

    return result;
}

static GstStateChangeReturn webKitMediaPlayReadyDecryptChangeState(GstElement* element, GstStateChange transition)
{
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;
    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(element);

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(self, "PAUSED->READY");
        g_mutex_lock(&self->mutex);
        g_cond_signal(&self->condition);
        g_mutex_unlock(&self->mutex);
        break;
    default:
        break;
    }

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    // Add post-transition code here.

    return ret;
}
#endif
