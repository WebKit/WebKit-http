/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
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
#include "WebKitOpenCDMDecryptorGStreamer.h"

#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OCDM)
#include <gcrypt.h>
#include <gst/base/gstbytereader.h>
#include <gst/gst.h>
#include <open_cdm.h>
#include <stdio.h>
#include <string>
#include <wtf/glib/GUniquePtr.h>

#define GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_OPENCDM_TYPE_DECRYPT, WebKitMediaOpenCDMDecryptPrivate))

struct _WebKitMediaOpenCDMDecryptPrivate {
    GUniquePtr<char>            m_session;
    GUniquePtr<OpenCdm>         m_openCdm;
};

static void webKitMediaOpenCDMDecryptorFinalize(GObject*);
static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt*, GstEvent*);
static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*, GstBuffer*, unsigned, GstBuffer*);

GST_DEBUG_CATEGORY(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

#define WIDEVINE_PROTECTION_SYSTEM_UUID "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)video/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID ";"
    "application/x-cenc, original-media-type=(string)audio/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID ";"
    "application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID ";"
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID ";"));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/webm; audio/webm; video/mp4; audio/mp4; audio/mpeg; video/x-h264"));

#define webkit_media_opencdm_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaOpenCDMDecrypt, webkit_media_opencdm_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_opencdm_decrypt_class_init(WebKitMediaOpenCDMDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaOpenCDMDecryptorFinalize;

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using OpenCMDi Encryption",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using OpenCDMi Encryption.",
        "TataElxsi");

    GST_DEBUG_CATEGORY_INIT(webkit_media_opencdm_decrypt_debug_category,
        "webkitopencdmi", 0, "OpenCDMi decryptor");

    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    cencClass->protectionSystemId = WIDEVINE_PROTECTION_SYSTEM_UUID;
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorHandleKeyResponse);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorDecrypt);

    g_type_class_add_private(klass, sizeof(WebKitMediaOpenCDMDecryptPrivate));
}

static void webkit_media_opencdm_decrypt_init(WebKitMediaOpenCDMDecrypt* self)
{
    WebKitMediaOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);

    if (!gcry_check_version(GCRYPT_VERSION))
        GST_ERROR_OBJECT(self, "Libgcrypt failed to initialize");

    // Allocate a pool of 16k secure memory. This make the secure memory
    // available and also drops privileges where needed.
    gcry_control(GCRYCTL_INIT_SECMEM, 16384, 0);

    gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);

    self->priv = priv;
    new (priv) WebKitMediaOpenCDMDecryptPrivate();
}

static void webKitMediaOpenCDMDecryptorFinalize(GObject* object)
{
    WebKitMediaOpenCDMDecrypt* self = WEBKIT_OPENCDM_DECRYPT(object);
    WebKitMediaOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));
    priv->m_openCdm->ReleaseMem();
    priv->~WebKitMediaOpenCDMDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    const GstStructure* structure = gst_event_get_structure(event);

    if (!gst_structure_has_name(structure, "drm-session"))
        return false;

    char* tempSession;
    gst_structure_get(structure, "session", G_TYPE_STRING, &tempSession, nullptr);
    WebKitMediaOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));
    priv->m_session.reset(tempSession);
    if (priv->m_session) {
        priv->m_openCdm.reset(new OpenCdm());
        priv->m_openCdm->SelectSession(priv->m_session.get());
    }

    return true;
}

static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    GstMapInfo ivMap;
    int errorCode;
    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    WebKitMediaOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));
    GstMapInfo map;
    gboolean bufferMapped;
    bufferMapped = gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE));
    if (!bufferMapped) {
        gst_buffer_unmap(ivBuffer, &ivMap);
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    if (subSamplesBuffer) {
        GstMapInfo subSamplesMap;
        gboolean subsamplesBufferMapped = gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ);
        if (!subsamplesBufferMapped) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            gst_buffer_unmap(ivBuffer, &ivMap);
            gst_buffer_unmap(buffer, &map);
            return false;
        }

        GstByteReader* reader = gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size);
        uint16_t inClear = 0;
        uint32_t inEncrypted = 0;
        uint32_t totalEncrypted = 0;
        unsigned position = 0;
        // Find out the total size of the encrypted data.
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader, &inClear);
            gst_byte_reader_get_uint32_be(reader, &inEncrypted);
            totalEncrypted += inEncrypted;
        }
        gst_byte_reader_set_pos(reader, 0);

        // Build a new buffer storing the entire encrypted cipher.
        GUniquePtr<uint8_t> holdEncryptedData(reinterpret_cast<uint8_t*>(malloc(totalEncrypted)));
        uint8_t* encryptedData = holdEncryptedData.get();
        unsigned index = 0;
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
        if ((errorCode = priv->m_openCdm->Decrypt(holdEncryptedData.get(), static_cast<uint32_t>(totalEncrypted),
            ivMap.data, static_cast<uint32_t>(ivMap.size)))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            gst_byte_reader_free(reader);
            gst_buffer_unmap(buffer, &map);
            gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
            gst_buffer_unmap(ivBuffer, &ivMap);
            return false;
        }

        // Re-build sub-sample data.
        index = 0;
        encryptedData = holdEncryptedData.get();
        unsigned total = 0;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader, &inClear);
            gst_byte_reader_get_uint32_be(reader, &inEncrypted);

            memcpy(map.data + total + inClear, encryptedData + index, inEncrypted);
            index += inEncrypted;
            total += inClear + inEncrypted;
        }

        gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
        if (reader)
            gst_byte_reader_free(reader);
    } else {
        // Decrypt cipher.
        ASSERT(priv->sessionMetaData);
        if ((errorCode = priv->m_openCdm->Decrypt(map.data, static_cast<uint32_t>(map.size),
            ivMap.data, static_cast<uint32_t>(ivMap.size)))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            gst_buffer_unmap(buffer, &map);
            gst_buffer_unmap(ivBuffer, &ivMap);
            return false;
        }
    }
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(ivBuffer, &ivMap);

    return true;
}

#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OCDM)
