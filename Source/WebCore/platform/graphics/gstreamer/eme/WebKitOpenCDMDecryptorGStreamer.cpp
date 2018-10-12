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
#include "WebKitOpenCDMDecryptorGStreamer.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OPENCDM)

#include "CDMOpenCDM.h"
#include <GStreamerCommon.h>
#include <open_cdm.h>
#include <wtf/text/WTFString.h>
#include <wtf/Lock.h>
#include <wtf/PrintStream.h>

#define GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_OPENCDM_DECRYPT, WebKitOpenCDMDecryptPrivate))

struct _WebKitOpenCDMDecryptPrivate {
    String m_session;
    std::unique_ptr<media::OpenCdm> m_openCdm;
    Lock m_mutex;
};

static void webKitMediaOpenCDMDecryptorFinalize(GObject*);
static bool webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* keyIDBuffer, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);
static bool webKitMediaOpenCDMDecryptorHandleInitData(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::InitData& initData);
static bool webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::InitData&);

static const char* supportedMediaTypes[] = { "video/webm", "video/mp4", "audio/webm", "audio/mp4", "video/x-h264", "audio/mpeg", nullptr };

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
    "video/webm; "
    "audio/webm; "
    "video/mp4; "
    "audio/mp4; "
    "audio/mpeg; "
    "video/x-h264"));

GST_DEBUG_CATEGORY(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

#define webkit_media_opencdm_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitOpenCDMDecrypt, webkit_media_opencdm_decrypt, WEBKIT_TYPE_MEDIA_CENC_DECRYPT);

enum SessionResult {
    InvalidSession,
    NewSession,
    OldSession
};

static void addKeySystemToSinkPadCaps(GRefPtr<GstCaps>& caps, const char* uuid)
{
    GST_INFO("adding sink pad template caps for %s", uuid);
    for (int i = 0; supportedMediaTypes[i]; ++i)
        gst_caps_append_structure(caps.get(), gst_structure_new("application/x-cenc", "original-media-type", G_TYPE_STRING, supportedMediaTypes[i], "protection-system", G_TYPE_STRING, uuid, nullptr));
}

static GRefPtr<GstCaps> createSinkPadTemplateCaps()
{
    media::OpenCdm& openCdm = media::OpenCdm::Instance();
    std::string emptyString;
    GRefPtr<GstCaps> caps = adoptGRef(gst_caps_new_empty());

    if (openCdm.IsTypeSupported(WebCore::GStreamerEMEUtilities::s_ClearKeyKeySystem, emptyString))
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID);

    if (openCdm.IsTypeSupported(WebCore::GStreamerEMEUtilities::s_PlayReadyKeySystems[0], emptyString))
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID);

    if (openCdm.IsTypeSupported(WebCore::GStreamerEMEUtilities::s_WidevineKeySystem, emptyString))
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID);

    return caps;
}

static void webkit_media_opencdm_decrypt_class_init(WebKitOpenCDMDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaOpenCDMDecryptorFinalize;

    GST_DEBUG_CATEGORY_INIT(webkit_media_opencdm_decrypt_debug_category,
        "webkitopencdm", 0, "OpenCDM decryptor");

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    GRefPtr<GstCaps> gstSinkPadTemplateCaps = createSinkPadTemplateCaps();
    gst_element_class_add_pad_template(elementClass, gst_pad_template_new("sink", GST_PAD_SINK, GST_PAD_ALWAYS, gstSinkPadTemplateCaps.get()));
    gst_element_class_add_pad_template(elementClass, gst_static_pad_template_get(&srcTemplate));

    gst_element_class_set_static_metadata(elementClass,
        "Decrypt content with OpenCDM support",
        GST_ELEMENT_FACTORY_KLASS_DECRYPTOR,
        "Decrypts media with OpenCDM support",
        "Metrological");

    WebKitMediaCommonEncryptionDecryptClass* cencClass = WEBKIT_MEDIA_CENC_DECRYPT_CLASS(klass);
    cencClass->handleInitData = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorHandleInitData);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorDecrypt);
    cencClass->attemptToDecryptWithLocalInstance = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance);

    g_type_class_add_private(klass, sizeof(WebKitOpenCDMDecryptPrivate));
}

static void webkit_media_opencdm_decrypt_init(WebKitOpenCDMDecrypt* self)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);
    self->priv = priv;
    new (priv) WebKitOpenCDMDecryptPrivate();
    self->priv->m_openCdm = std::make_unique<media::OpenCdm>();
    GST_TRACE_OBJECT(self, "created");
}

static void webKitMediaOpenCDMDecryptorFinalize(GObject* object)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(object));
    priv->~WebKitOpenCDMDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static SessionResult webKitMediaOpenCDMDecryptorResetSessionFromInitDataIfNeeded(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::InitData& initData)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));

    LockHolder locker(priv->m_mutex);
    RefPtr<WebCore::CDMInstance> cdmInstance = webKitMediaCommonEncryptionDecryptCDMInstance(self);
    ASSERT(cdmInstance && is<WebCore::CDMInstanceOpenCDM>(*cdmInstance));
    auto& cdmInstanceOpenCDM = downcast<WebCore::CDMInstanceOpenCDM>(*cdmInstance);

    SessionResult returnValue = InvalidSession;
    String session = cdmInstanceOpenCDM.sessionIdByInitData(initData);
    if (session.isEmpty() || !cdmInstanceOpenCDM.isSessionIdUsable(session)) {
        GST_DEBUG_OBJECT(self, "session %s is empty or unusable, resetting", session.utf8().data());
        priv->m_session = String();
        priv->m_openCdm = nullptr;
    } else if (session != priv->m_session) {
        priv->m_session = session;
        priv->m_openCdm = std::make_unique<media::OpenCdm>(priv->m_session.utf8().data());
        GST_DEBUG_OBJECT(self, "new session %s is usable", session.utf8().data());
        returnValue = NewSession;
    } else {
        GST_DEBUG_OBJECT(self, "same session %s", session.utf8().data());
        returnValue = OldSession;
    }

    return returnValue;
}

static bool webKitMediaOpenCDMDecryptorHandleInitData(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::InitData& initData)
{
    GST_TRACE_OBJECT(self, "handling init data of size %u and MD5 %s", initData.sizeInBytes(), WebCore::GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    return webKitMediaOpenCDMDecryptorResetSessionFromInitDataIfNeeded(self, initData) == InvalidSession;
}

static bool webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::InitData& initData)
{
    GST_TRACE_OBJECT(self, "attempting to decrypt with local instance and init data of size %u and MD5 %s", initData.sizeInBytes(), WebCore::GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    return webKitMediaOpenCDMDecryptorResetSessionFromInitDataIfNeeded(self, initData) != InvalidSession;
}

static bool webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* keyIDBuffer, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);

    GstMappedBuffer mappedIV(ivBuffer, GST_MAP_READ);
    if (!mappedIV) {
        GST_ERROR_OBJECT(self, "Failed to map IV");
        return false;
    }

    GST_MEMDUMP_OBJECT(self, "IV for sample", mappedIV.data(), mappedIV.size());

    GstMappedBuffer mappedBuffer(buffer, GST_MAP_READWRITE);
    if (!mappedBuffer) {
        GST_ERROR_OBJECT(self, "Failed to map buffer");
        return false;
    }

    GstMappedBuffer mappedKeyID(keyIDBuffer, GST_MAP_READ);
    if (!mappedKeyID) {
        GST_ERROR_OBJECT(self, "Failed to map key ID buffer");
        return false;
    }

    int errorCode;
    if (subSamplesBuffer) {
        GstMappedBuffer mappedSubSamples(subSamplesBuffer, GST_MAP_READ);
        if (!mappedSubSamples) {
            GST_ERROR_OBJECT(self, "Failed to map subsample buffer");
            return false;
        }

        GUniquePtr<GstByteReader> reader(gst_byte_reader_new(mappedSubSamples.data(), mappedSubSamples.size()));
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
            memcpy(encryptedData, mappedBuffer.data() + index + inClear, inEncrypted);
            index += inClear + inEncrypted;
            encryptedData += inEncrypted;
        }
        gst_byte_reader_set_pos(reader.get(), 0);

        // Decrypt cipher.
        GST_TRACE_OBJECT(self, "decrypting (subsample)");
        if ((errorCode = priv->m_openCdm->Decrypt(holdEncryptedData.get(), static_cast<uint32_t>(totalEncrypted), mappedIV.data(), static_cast<uint32_t>(mappedIV.size()), mappedKeyID.size(), mappedKeyID.data(), WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT.millisecondsAs<uint32_t>()))) {
            GST_ERROR_OBJECT(self, "subsample decryption failed, error code %d", errorCode);
            return false;
        }

        // Re-build sub-sample data.
        index = 0;
        encryptedData = holdEncryptedData.get();
        unsigned total = 0;
        for (position = 0; position < subSampleCount; position++) {
            gst_byte_reader_get_uint16_be(reader.get(), &inClear);
            gst_byte_reader_get_uint32_be(reader.get(), &inEncrypted);

            memcpy(mappedBuffer.data() + total + inClear, encryptedData + index, inEncrypted);
            index += inEncrypted;
            total += inClear + inEncrypted;
        }
    } else {
        GST_TRACE_OBJECT(self, "decrypting (no subsamples)");

        if ((errorCode = priv->m_openCdm->Decrypt(mappedBuffer.data(), static_cast<uint32_t>(mappedBuffer.size()), mappedIV.data(), static_cast<uint32_t>(mappedIV.size()), mappedKeyID.size(), mappedKeyID.data(), WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT.millisecondsAs<uint32_t>()))) {
            GST_ERROR_OBJECT(self, "decryption failed, error code %d", errorCode);
            return false;
        }
    }

    return true;
}
#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OPENCDM)
