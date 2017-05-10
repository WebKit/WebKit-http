/* GStreamer PlayReady decryptor
 *
 * Copyright (C) 2015, 2016 Igalia S.L
 * Copyright (C) 2015, 2016 Metrological
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

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)) && USE(GSTREAMER) && USE(PLAYREADY)
#include "WebKitPlayReadyDecryptorGStreamer.h"
#include "PlayreadySession.h"
#include <gst/base/gstbytereader.h>

#define WEBKIT_MEDIA_PLAYREADY_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_PLAYREADY_DECRYPT, WebKitMediaPlayReadyDecryptPrivate))
struct _WebKitMediaPlayReadyDecryptPrivate {
    WebCore::PlayreadySession* sessionMetaData;
};

static void webKitMediaPlayReadyDecryptorFinalize(GObject*);
static gboolean webKitMediaPlayReadyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent*);
static gboolean webKitMediaPlayReadyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);

GST_DEBUG_CATEGORY(webkit_media_playready_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_playready_decrypt_debug_category

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" PLAYREADY_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" PLAYREADY_PROTECTION_SYSTEM_UUID
        ));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/x-h264; audio/mpeg"));

#define webkit_media_playready_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaPlayReadyDecrypt, webkit_media_playready_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_playready_decrypt_class_init(WebKitMediaPlayReadyDecryptClass* klass)
{
    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->finalize = webKitMediaPlayReadyDecryptorFinalize;

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using PlayReady Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using PlayReady Encryption.",
        "Philippe Normand <philn@igalia.com>");

    GST_DEBUG_CATEGORY_INIT(webkit_media_playready_decrypt_debug_category,
        "webkitplayready", 0, "PlayReady decryptor");

    cencClass->protectionSystemId = PLAYREADY_PROTECTION_SYSTEM_UUID;
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaPlayReadyDecryptorHandleKeyResponse);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaPlayReadyDecryptorDecrypt);

    g_type_class_add_private(klass, sizeof(WebKitMediaPlayReadyDecryptPrivate));
}

static void webkit_media_playready_decrypt_init(WebKitMediaPlayReadyDecrypt*)
{
}

static void webKitMediaPlayReadyDecryptorFinalize(GObject* object)
{
    WebKitMediaPlayReadyDecrypt* self = WEBKIT_MEDIA_PLAYREADY_DECRYPT(object);
    WebKitMediaPlayReadyDecryptPrivate* priv = self->priv;

    priv->~WebKitMediaPlayReadyDecryptPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static gboolean webKitMediaPlayReadyDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    WebKitMediaPlayReadyDecryptPrivate* priv = WEBKIT_MEDIA_PLAYREADY_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_PLAYREADY_DECRYPT(self));

    const GstStructure* structure = gst_event_get_structure(event);
    const char* label = "playready-session";
    if (!gst_structure_has_name(structure, label))
        return FALSE;

    GST_INFO_OBJECT(self, "received %s", label);

    const GValue* value = gst_structure_get_value(structure, "session");
    priv->sessionMetaData = reinterpret_cast<WebCore::PlayreadySession*>(g_value_get_pointer(value));
    return TRUE;
}

static gboolean webKitMediaPlayReadyDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    WebKitMediaPlayReadyDecryptPrivate* priv = WEBKIT_MEDIA_PLAYREADY_DECRYPT_GET_PRIVATE(WEBKIT_MEDIA_PLAYREADY_DECRYPT(self));
    GstMapInfo map, ivMap, subSamplesMap;
    unsigned position = 0;
    GstByteReader* reader = nullptr;
    gboolean bufferMapped, subsamplesBufferMapped;
    int errorCode;
    guint16 inClear = 0;
    guint32 inEncrypted = 0;
    guint32 totalEncrypted = 0;
    uint8_t* encryptedData;
    uint8_t* fEncryptedData;
    unsigned index = 0;
    unsigned total = 0;

    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    bufferMapped = gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE));
    if (!bufferMapped) {
        gst_buffer_unmap(ivBuffer, &ivMap);
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    if (subSamplesBuffer) {
        subsamplesBufferMapped = gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ);
        if (!subsamplesBufferMapped) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            gst_buffer_unmap(ivBuffer, &ivMap);
            gst_buffer_unmap(buffer, &map);
            return false;
        }

        reader = gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size);

        // Find out the total size of the encrypted data.
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader, &inClear);
            gst_byte_reader_get_uint32_be(reader, &inEncrypted);
            totalEncrypted += inEncrypted;
        }
        gst_byte_reader_set_pos(reader, 0);

        // Build a new buffer storing the entire encrypted cipher.
        encryptedData = (uint8_t*) g_malloc(totalEncrypted);
        fEncryptedData = encryptedData;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader, &inClear);
            gst_byte_reader_get_uint32_be(reader, &inEncrypted);
            memcpy(encryptedData, map.data + index + inClear, inEncrypted);
            index += inClear + inEncrypted;
            encryptedData += inEncrypted;
        }
        gst_byte_reader_set_pos(reader, 0);

        // Decrypt cipher.
        ASSERT(priv->sessionMetaData);
        if ((errorCode = priv->sessionMetaData->processPayload(static_cast<const void*>(ivMap.data), static_cast<uint32_t>(ivMap.size), static_cast<void*>(fEncryptedData), static_cast<uint32_t>(totalEncrypted)))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            g_free(fEncryptedData);
            gst_byte_reader_free(reader);
            gst_buffer_unmap(buffer, &map);
            gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
            gst_buffer_unmap(ivBuffer, &ivMap);
            return false;
        }

        // Re-build sub-sample data.
        index = 0;
        encryptedData = fEncryptedData;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader, &inClear);
            gst_byte_reader_get_uint32_be(reader, &inEncrypted);

            memcpy(map.data + total + inClear, encryptedData + index, inEncrypted);
            index += inEncrypted;
            total += inClear + inEncrypted;
        }

        g_free(fEncryptedData);
        gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
    } else {
        // Decrypt cipher.
        ASSERT(priv->sessionMetaData);
        if ((errorCode = priv->sessionMetaData->processPayload(static_cast<const void*>(ivMap.data), static_cast<uint32_t>(ivMap.size), static_cast<void*>(map.data), static_cast<uint32_t>(map.size)))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            g_free(fEncryptedData);
            gst_buffer_unmap(buffer, &map);
            gst_buffer_unmap(ivBuffer, &ivMap);
            return false;
        }
    }

    if (reader)
        gst_byte_reader_free(reader);
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(ivBuffer, &ivMap);
    return true;
}

bool webkit_media_playready_decrypt_is_playready_key_system_id(const gchar* keySystemId)
{
    return g_strcmp0(keySystemId, PLAYREADY_PROTECTION_SYSTEM_UUID) == 0;
}

#endif
