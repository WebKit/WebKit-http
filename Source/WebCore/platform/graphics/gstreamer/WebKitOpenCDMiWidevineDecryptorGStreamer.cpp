/* GStreamer Widevine decryptor
 *
 * Copyright (C) 2016 TATA ELXSI
 * Copyright (C) 2016 Metrological
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
#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(GSTREAMER) && USE(OCDM)
#include <string>
#include <stdio.h>
#include <gst/gst.h>
#include <open_cdm.h>
#include <gst/base/gstbytereader.h>
#include "WebKitOpenCDMiWidevineDecryptorGStreamer.h"

#define GST_OPENCDMI_WIDEVINE_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), OPENCDMI_TYPE_WIDEVINE_DECRYPT, OpenCDMiWideVineDecryptPrivate))
struct _OpenCDMiWideVineDecryptPrivate {
    const char *m_session;
    OpenCdm* m_openCdm;
};

static void openCDMiWidevineDecryptorFinalize(GObject*);
static void openCDMiWidevineDecryptorRequestDecryptionKey(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer*);
static gboolean openCDMiWidevineDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent*);
static gboolean openCDMiWidevineDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);

GST_DEBUG_CATEGORY(opencdmi_widevine_decrypt_debug_category);
#define GST_CAT_DEFAULT opencdmi_widevine_decrypt_debug_category

#define WIDEVINE_PROTECTION_SYSTEM_ID "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID "; "
    "application/x-cenc, original-media-type=(string)video/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID "; "
    "application/x-cenc, original-media-type=(string)audio/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID ";"
    "application/x-cenc, original-media-type=(string)audio/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID ";"
    "application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID ";"
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_ID ";"
     ));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/webm; audio/webm; video/mp4; audio/mp4; audio/mpeg; video/x-h264"));

#define opencdmi_widevine_decrypt_parent_class parent_class
G_DEFINE_TYPE(OpenCDMiWideVineDecrypt, opencdmi_widevine_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void opencdmi_widevine_decrypt_class_init(OpenCDMiWideVineDecryptClass* klass)
{
    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);

    gobjectClass->finalize = openCDMiWidevineDecryptorFinalize;

    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using WideVine Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using WideVine Encryption.",
        "TataElxsi");

    GST_DEBUG_CATEGORY_INIT(opencdmi_widevine_decrypt_debug_category,
        "webkitwidevine", 0, "WideVine decryptor");

    cencClass->protectionSystemId = WIDEVINE_PROTECTION_SYSTEM_ID;
    cencClass->requestDecryptionKey = GST_DEBUG_FUNCPTR(openCDMiWidevineDecryptorRequestDecryptionKey);
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(openCDMiWidevineDecryptorHandleKeyResponse);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(openCDMiWidevineDecryptorDecrypt);

    g_type_class_add_private(klass, sizeof(OpenCDMiWideVineDecryptPrivate));

}

static void opencdmi_widevine_decrypt_init(OpenCDMiWideVineDecrypt *decrypt)
{
}

static void openCDMiWidevineDecryptorFinalize(GObject* object)
{
    OpenCDMiWideVineDecrypt* self = OPENCDMI_WIDEVINE_DECRYPT(object);
    OpenCDMiWideVineDecryptPrivate* priv = GST_OPENCDMI_WIDEVINE_DECRYPT_GET_PRIVATE(OPENCDMI_WIDEVINE_DECRYPT(self));
    priv->m_openCdm->ReleaseMem();
    delete priv->m_openCdm;
    priv->m_openCdm = nullptr;
//    priv->~OpenCDMiWideVineDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void openCDMiWidevineDecryptorRequestDecryptionKey(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* initDataBuffer)
{
    gst_element_post_message(GST_ELEMENT(self),
        gst_message_new_element(GST_OBJECT(self),
            gst_structure_new("drm-key-needed", "data", GST_TYPE_BUFFER, initDataBuffer,
                "key-system-id", G_TYPE_STRING, "com.widevine.alpha", NULL)));
}

static gboolean openCDMiWidevineDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    OpenCDMiWideVineDecryptPrivate* priv = GST_OPENCDMI_WIDEVINE_DECRYPT_GET_PRIVATE(OPENCDMI_WIDEVINE_DECRYPT(self));

    const GstStructure* structure = gst_event_get_structure(event);
    const char* label = "drm-session";
    if (!gst_structure_has_name(structure, label))
        return FALSE;

    GST_INFO_OBJECT(self, "received %s", label);

    gst_structure_get(structure, "session", G_TYPE_STRING, &priv->m_session, nullptr);
    if (priv->m_session && strlen(priv->m_session)) {
        priv->m_openCdm = new OpenCdm(); 
        priv->m_openCdm->SelectSession(priv->m_session);
    }

    return TRUE;
}

static gboolean openCDMiWidevineDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    OpenCDMiWideVineDecryptPrivate* priv = GST_OPENCDMI_WIDEVINE_DECRYPT_GET_PRIVATE(OPENCDMI_WIDEVINE_DECRYPT(self));
    GstMapInfo map, ivMap, subSamplesMap;
    unsigned position = 0;
    GstByteReader* reader = NULL;
    gboolean bufferMapped, subsamplesBufferMapped;
    int errorCode;
    guint16 inClear = 0;
    guint32 inEncrypted = 0;
    guint32 totalEncrypted = 0;
    uint8_t* encryptedData;
    uint8_t* fEncryptedData;
    unsigned index = 0;
    unsigned total = 0;
    //Call OpenCDM Decrypt function with args
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
        if ((errorCode = priv->m_openCdm->Decrypt(fEncryptedData, static_cast<uint32_t>(totalEncrypted), 
                                                  ivMap.data, static_cast<uint32_t>(ivMap.size)))) {
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
        if ((errorCode =  priv->m_openCdm->Decrypt(map.data, static_cast<uint32_t>(map.size),
                                                   ivMap.data, static_cast<uint32_t>(ivMap.size)))) {
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
#endif
