/* GStreamer ClearKey common encryption decryptor
 *
 * Copyright (C) 2015 Igalia S.L
 * Copyright (C) 2015 Metrological
 * Copyright (C) 2013 YouView TV Ltd. <alex.ashley@youview.com>
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

#if (ENABLE(ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA_V2)) && USE(GSTREAMER)
#include "WebKitCommonEncryptionDecryptorGStreamer.h"

#include <gst/base/gstbasetransform.h>
#include <gst/base/gstbytereader.h>
#include "WebKitMediaAesCtr.h"

struct _WebKitMediaCommonEncryptionDecrypt {
    GstBaseTransform parent;
    GBytes* key;
};

struct _WebKitMediaCommonEncryptionDecryptClass {
    GstBaseTransformClass parentClass;
};

static GstCaps* webkitMediaCommonEncryptionDecryptTransformCaps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps* filter);
static GstFlowReturn webkitMediaCommonEncryptionDecryptTransformInPlace(GstBaseTransform*, GstBuffer*);
static gboolean webkitMediaCommonEncryptionDecryptSinkEventHandler(GstBaseTransform*, GstEvent*);

GST_DEBUG_CATEGORY_STATIC(webkit_media_common_encryption_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_common_encryption_decrypt_debug_category

#define CLEAR_KEY_PROTECTION_SYSTEM_ID "58147ec8-0423-4659-92e6-f52c5ce8c3cc"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_ID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" CLEAR_KEY_PROTECTION_SYSTEM_ID));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264; audio/mpeg"));

#define webkit_media_common_encryption_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaCommonEncryptionDecrypt, webkit_media_common_encryption_decrypt, GST_TYPE_BASE_TRANSFORM);

static void webkit_media_common_encryption_decrypt_class_init(WebKitMediaCommonEncryptionDecryptClass* klass) {
    GstBaseTransformClass* baseTransformClass = GST_BASE_TRANSFORM_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using ISOBMFF ClearKey Common Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using ISOBMFF ClearKey Common Encryption.",
        "Alex Ashley <alex.ashley@youview.com>"
        "and Philippe Normand <philn@igalia.com>");

    GST_DEBUG_CATEGORY_INIT(webkit_media_common_encryption_decrypt_debug_category,
        "webkitcenc", 0, "ClearKey CENC decryptor");

    baseTransformClass->transform_ip = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptTransformInPlace);
    baseTransformClass->transform_caps = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptTransformCaps);
    baseTransformClass->sink_event = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptSinkEventHandler);
    baseTransformClass->transform_ip_on_passthrough = FALSE;
}

static void webkit_media_common_encryption_decrypt_init(WebKitMediaCommonEncryptionDecrypt* self)
{
    GstBaseTransform* base = GST_BASE_TRANSFORM(self);

    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);
}

/*
  Given the pad in this direction and the given caps, what caps are allowed on
  the other pad in this element ?
*/
static GstCaps* webkitMediaCommonEncryptionDecryptTransformCaps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    g_return_val_if_fail(direction != GST_PAD_UNKNOWN, nullptr);
    GstCaps* transformedCaps = gst_caps_new_empty();

    GST_DEBUG_OBJECT(base, "direction: %s, caps: %" GST_PTR_FORMAT " filter:"
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
            gst_structure_set(out, "protection-system", G_TYPE_STRING, CLEAR_KEY_PROTECTION_SYSTEM_ID,
                "original-media-type", G_TYPE_STRING, gst_structure_get_name(in), nullptr);

            gst_structure_set_name(out, "application/x-cenc");
            gst_structure_free(tmp);
        }

        bool duplicate = false;
        unsigned size = gst_caps_get_size(transformedCaps);

        for (unsigned index = 0; !duplicate && index < size; ++index) {
            GstStructure* s = gst_caps_get_structure(transformedCaps, index);
            if (gst_structure_is_equal(s, out))
                duplicate = true;
        }

        if (!duplicate)
            gst_caps_append_structure(transformedCaps, out);
        else
            gst_structure_free(out);
    }

    if (filter) {
        GstCaps* intersection;

        GST_DEBUG_OBJECT(base, "Using filter caps %" GST_PTR_FORMAT, filter);
        intersection = gst_caps_intersect_full(transformedCaps, filter, GST_CAPS_INTERSECT_FIRST);
        gst_caps_unref(transformedCaps);
        transformedCaps = intersection;
    }

    GST_DEBUG_OBJECT(base, "returning %" GST_PTR_FORMAT, transformedCaps);
    return transformedCaps;
}

static GstFlowReturn webkitMediaCommonEncryptionDecryptTransformInPlace(GstBaseTransform* base, GstBuffer* buffer)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(base);
    GstFlowReturn result = GST_FLOW_OK;
    GstMapInfo map, ivMap;
    unsigned position = 0;
    unsigned sampleIndex = 0;
    guint subSampleCount;
    AesCtrState* state = nullptr;
    guint ivSize;
    gboolean encrypted;
    const GValue* value;
    GstBuffer* ivBuffer = nullptr;
    GBytes* ivBytes = nullptr;
    GstBuffer* subsamplesBuffer = nullptr;
    GstMapInfo subSamplesMap;
    GstByteReader* reader = nullptr;

    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
    if (!protectionMeta || !buffer) {
        if (!protectionMeta)
            GST_ERROR_OBJECT(self, "Failed to get GstProtection metadata from buffer %p", buffer);

        if (!buffer)
            GST_ERROR_OBJECT(self, "Failed to get writable buffer");

        return GST_FLOW_NOT_SUPPORTED;
    }

    if (!gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE))) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    GST_TRACE_OBJECT(self, "decrypt sample %d", map.size);
    if (!gst_structure_get_uint(protectionMeta->info, "iv_size", &ivSize)) {
        GST_ERROR_OBJECT(self, "failed to get iv_size");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    if (!gst_structure_get_boolean(protectionMeta->info, "encrypted", &encrypted)) {
        GST_ERROR_OBJECT(self, "failed to get encrypted flag");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    // Unencrypted sample.
    if (!ivSize || !encrypted)
        goto beach;

    GST_DEBUG_OBJECT(base, "protection meta: %" GST_PTR_FORMAT, protectionMeta->info);
    if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subSampleCount)) {
        GST_ERROR_OBJECT(self, "failed to get subsample_count");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    value = gst_structure_get_value(protectionMeta->info, "iv");
    if (!value) {
        GST_ERROR_OBJECT(self, "Failed to get IV for sample");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    ivBuffer = gst_value_get_buffer(value);
    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    ivBytes = g_bytes_new(ivMap.data, ivMap.size);
    gst_buffer_unmap(ivBuffer, &ivMap);
    if (subSampleCount) {
        value = gst_structure_get_value(protectionMeta->info, "subsamples");
        if (!value) {
            GST_ERROR_OBJECT(self, "Failed to get subsamples");
            result = GST_FLOW_NOT_SUPPORTED;
            goto release;
        }
        subsamplesBuffer = gst_value_get_buffer(value);
        if (!gst_buffer_map(subsamplesBuffer, &subSamplesMap, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            result = GST_FLOW_NOT_SUPPORTED;
            goto release;
        }
    }

    if (!self->key) {
        GST_ERROR_OBJECT(self, "Decryption key not provided");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    state = webkit_media_aes_ctr_decrypt_new(self->key, ivBytes);
    if (!state) {
        GST_ERROR_OBJECT(self, "Failed to init AES cipher");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    reader = gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size);
    if (!reader) {
        GST_ERROR_OBJECT(self, "Failed to allocate subsample reader");
        result = GST_FLOW_NOT_SUPPORTED;
        goto release;
    }

    GST_DEBUG_OBJECT(self, "position: %d, size: %d", position, map.size);
    while (position < map.size) {
        guint16 nBytesClear = 0;
        guint32 nBytesEncrypted = 0;

        if (sampleIndex < subSampleCount) {
            if (!gst_byte_reader_get_uint16_be(reader, &nBytesClear)
                || !gst_byte_reader_get_uint32_be(reader, &nBytesEncrypted)) {
                result = GST_FLOW_NOT_SUPPORTED;
                GST_DEBUG_OBJECT(self, "unsupported");
                goto release;
            }

            sampleIndex++;
        } else {
            nBytesClear = 0;
            nBytesEncrypted = map.size - position;
        }

        GST_TRACE_OBJECT(self, "%d bytes clear (todo=%d)", nBytesClear, map.size - position);
        position += nBytesClear;
        if (nBytesEncrypted) {
            GST_TRACE_OBJECT(self, "%d bytes encrypted (todo=%d)", nBytesEncrypted, map.size - position);
            if (!webkit_media_aes_ctr_decrypt_ip(state, map.data + position, nBytesEncrypted)) {
                result = GST_FLOW_NOT_SUPPORTED;
                GST_ERROR_OBJECT(self, "decryption failed");
                goto beach;
            }
            position += nBytesEncrypted;
        }
    }

beach:
    gst_buffer_unmap(buffer, &map);
    if (state)
        webkit_media_aes_ctr_decrypt_unref(state);

release:
    if (reader)
        gst_byte_reader_free(reader);

    if (subsamplesBuffer)
        gst_buffer_unmap(subsamplesBuffer, &subSamplesMap);

    if (protectionMeta)
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));

    if (ivBytes)
        g_bytes_unref(ivBytes);

    return result;
}

static gboolean webkitMediaCommonEncryptionDecryptSinkEventHandler(GstBaseTransform* trans, GstEvent* event)
{
    gboolean result = FALSE;
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(trans);

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        const gchar* systemId;
        GstBuffer* buffer = nullptr;
        const gchar* origin;

        GST_DEBUG_OBJECT(self, "received protection event");
        gst_event_parse_protection(event, &systemId, &buffer, &origin);
        GST_DEBUG_OBJECT(self, "systemId: %s", systemId);
        if (!g_str_equal(systemId, CLEAR_KEY_PROTECTION_SYSTEM_ID)) {
            gst_event_unref(event);
            result = TRUE;
            break;
        }

        if (g_str_has_prefix(origin, "isobmff/")) {
            gst_element_post_message(GST_ELEMENT(self),
                gst_message_new_element(GST_OBJECT(self),
                    gst_structure_new("drm-key-needed", "data", GST_TYPE_BUFFER, buffer,
                        "key-system-id", G_TYPE_STRING, "org.w3.clearkey", nullptr)));
        }

        gst_event_unref(event);
        result = TRUE;
        break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM_OOB: {
        const GstStructure* structure = gst_event_get_structure(event);
        if (gst_structure_has_name(structure, "drm-cipher")) {
            GstBuffer* buffer;
            GstMapInfo info;
            const GValue* value = gst_structure_get_value(structure, "key");
            buffer = gst_value_get_buffer(value);
            if (self->key)
                g_bytes_unref(self->key);
            gst_buffer_map(buffer, &info, GST_MAP_READ);
            self->key = g_bytes_new(info.data, info.size);
            gst_buffer_unmap(buffer, &info);
        }

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

#endif
