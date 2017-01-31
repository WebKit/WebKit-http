/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * Copyright (C) 2016-2017 Igalia S.L
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
#include "WebKitOpenCDMWidevineDecryptorGStreamer.h"

#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OCDM)

#include "GUniquePtrGStreamer.h"

#include <open_cdm.h>
#include <wtf/text/WTFString.h>

#define GST_WEBKIT_OPENCDM_WIDEVINE_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_OPENCDM_WIDEVINE_DECRYPT, WebKitOpenCDMWidevineDecryptPrivate))

struct _WebKitOpenCDMWidevineDecryptPrivate {
    String m_session;
    std::unique_ptr<OpenCdm> m_openCdm;
};

static void webKitMediaOpenCDMDecryptorFinalize(GObject*);
static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt*, GstEvent*);
static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer*, GstBuffer*, unsigned, GstBuffer*);

GST_DEBUG_CATEGORY(webkit_media_opencdm_widevine_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_widevine_decrypt_debug_category

static GstStaticPadTemplate sinkTemplate = GST_STATIC_PAD_TEMPLATE("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("application/x-cenc, original-media-type=(string)video/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)video/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/webm, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/mp4, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)video/x-h264, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID "; "
    "application/x-cenc, original-media-type=(string)audio/mpeg, protection-system=(string)" WIDEVINE_PROTECTION_SYSTEM_UUID ";"));

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS("video/webm; audio/webm; video/mp4; audio/mp4; audio/mpeg; video/x-h264"));

#define webkit_media_opencdm_widevine_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitOpenCDMWidevineDecrypt, webkit_media_opencdm_widevine_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

static void webkit_media_opencdm_widevine_decrypt_class_init(WebKitOpenCDMWidevineDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaOpenCDMDecryptorFinalize;

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&sinkTemplate));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content encrypted using Widevine DRM system supported with OpenCDM",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media that has been encrypted using Widevine DRM system supported with OpenCDM",
        "TataElxsi");

    GST_DEBUG_CATEGORY_INIT(webkit_media_opencdm_widevine_decrypt_debug_category,
        "webkitopencdmwidevine", 0, "OpenCDM Widevine decryptor");

    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    cencClass->protectionSystemId = WIDEVINE_PROTECTION_SYSTEM_UUID;
    cencClass->handleKeyResponse = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorHandleKeyResponse);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorDecrypt);

    g_type_class_add_private(klass, sizeof(WebKitOpenCDMWidevineDecryptPrivate));
}

static void webkit_media_opencdm_widevine_decrypt_init(WebKitOpenCDMWidevineDecrypt* self)
{
    WebKitOpenCDMWidevineDecryptPrivate* priv = GST_WEBKIT_OPENCDM_WIDEVINE_DECRYPT_GET_PRIVATE(self);
    self->priv = priv;
    new (priv) WebKitOpenCDMWidevineDecryptPrivate();
}

static void webKitMediaOpenCDMDecryptorFinalize(GObject* object)
{
    WebKitOpenCDMWidevineDecryptPrivate* priv = GST_WEBKIT_OPENCDM_WIDEVINE_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_WIDEVINE_DECRYPT(object));
    priv->m_openCdm->ReleaseMem();
    priv->~WebKitOpenCDMWidevineDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static gboolean webKitMediaOpenCDMDecryptorHandleKeyResponse(WebKitMediaCommonEncryptionDecrypt* self, GstEvent* event)
{
    const GstStructure* structure = gst_event_get_structure(event);
    if (!gst_structure_has_name(structure, "drm-session"))
        return false;

    GUniqueOutPtr<char> temporarySession;
    gst_structure_get(structure, "session", G_TYPE_STRING, &temporarySession.outPtr(), nullptr);
    WebKitOpenCDMWidevineDecryptPrivate* priv = GST_WEBKIT_OPENCDM_WIDEVINE_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_WIDEVINE_DECRYPT(self));
    ASSERT(temporarySession);

    priv->m_session = temporarySession.get();
    priv->m_openCdm = std::make_unique<OpenCdm>();
    priv->m_openCdm->SelectSession(priv->m_session.utf8().data());
    return true;
}

static gboolean webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    GstMapInfo ivMap;
    if (!gst_buffer_map(ivBuffer, &ivMap, GST_MAP_READ)) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    GstMapInfo map;
    if (!gst_buffer_map(buffer, &map, static_cast<GstMapFlags>(GST_MAP_READWRITE))) {
        gst_buffer_unmap(ivBuffer, &ivMap);
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    WebKitOpenCDMWidevineDecryptPrivate* priv = GST_WEBKIT_OPENCDM_WIDEVINE_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_WIDEVINE_DECRYPT(self));
    ASSERT(priv->sessionMetaData);

    int errorCode;
    bool returnValue = true;
    if (subSamplesBuffer) {
        GstMapInfo subSamplesMap;
        if (!gst_buffer_map(subSamplesBuffer, &subSamplesMap, GST_MAP_READ)) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            returnValue = false;
            goto beach;
        }

        GUniquePtr<GstByteReader> reader(gst_byte_reader_new(subSamplesMap.data, subSamplesMap.size));
        uint16_t inClear = 0;
        uint32_t inEncrypted = 0;
        uint32_t totalEncrypted = 0;
        unsigned position;
        // Find out the total size of the encrypted data.
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader.get(), &inClear);
            gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);
            totalEncrypted += inEncrypted;
        }
        gst_byte_reader_set_pos(reader.get(), 0);

        // Build a new buffer storing the entire encrypted cipher.
        GUniquePtr<uint8_t> holdEncryptedData(reinterpret_cast<uint8_t*>(malloc(totalEncrypted)));
        uint8_t* encryptedData = holdEncryptedData.get();
        unsigned index = 0;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader.get(), &inClear);
            gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);
            memcpy(encryptedData, map.data + index + inClear, inEncrypted);
            index += inClear + inEncrypted;
            encryptedData += inEncrypted;
        }
        gst_byte_reader_set_pos(reader.get(), 0);

        // Decrypt cipher.
        if (errorCode = priv->m_openCdm->Decrypt(holdEncryptedData.get(), static_cast<uint32_t>(totalEncrypted),
            ivMap.data, static_cast<uint32_t>(ivMap.size))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
            returnValue = false;
            goto beach;
        }

        // Re-build sub-sample data.
        index = 0;
        encryptedData = holdEncryptedData.get();
        unsigned total = 0;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader.get(), &inClear);
            gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);

            memcpy(map.data + total + inClear, encryptedData + index, inEncrypted);
            index += inEncrypted;
            total += inClear + inEncrypted;
        }

        gst_buffer_unmap(subSamplesBuffer, &subSamplesMap);
    } else {
        // Decrypt cipher.
        if (errorCode = priv->m_openCdm->Decrypt(map.data, static_cast<uint32_t>(map.size),
            ivMap.data, static_cast<uint32_t>(ivMap.size))) {
            GST_WARNING_OBJECT(self, "ERROR - packet decryption failed [%d]", errorCode);
            returnValue = false;
            goto beach;
        }
    }

beach:
    gst_buffer_unmap(buffer, &map);
    gst_buffer_unmap(ivBuffer, &ivMap);
    return returnValue;
}

#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OCDM)
