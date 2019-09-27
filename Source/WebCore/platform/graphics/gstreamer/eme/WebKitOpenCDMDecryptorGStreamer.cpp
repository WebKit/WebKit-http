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
#include <open_cdm_adapter.h>
#include <wtf/Lock.h>
#include <wtf/PrintStream.h>
#include <wtf/text/WTFString.h>

#define GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_OPENCDM_DECRYPT, WebKitOpenCDMDecryptPrivate))

using WebCore::GstMappedBuffer;

struct _WebKitOpenCDMDecryptPrivate {
    String m_session;
    WebCore::ScopedSession m_openCdmSession;
    Lock m_mutex;
};

static void webKitMediaOpenCDMDecryptorFinalize(GObject*);
static bool webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt*, GstBuffer* keyIDBuffer, GstBuffer* iv, GstBuffer* sample, unsigned subSamplesCount, GstBuffer* subSamples);
static bool webKitMediaOpenCDMDecryptorHandleKeyId(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::SharedBuffer&);
static bool webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::SharedBuffer&);

static const char* cencEncryptionMediaTypes[] = { "video/mp4", "audio/mp4", "video/x-h264", "audio/mpeg", nullptr };
static const char* webmEncryptionMediaTypes[] = { "video/webm", "audio/webm", "video/x-vp9", nullptr };

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src",
    GST_PAD_SRC,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS(
        "video/webm; "
        "audio/webm; "
        "video/mp4; "
        "audio/mp4; "
        "audio/mpeg; "
        "video/x-h264; "
        "video/x-vp9; "));

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
    for (int i = 0; cencEncryptionMediaTypes[i]; ++i)
        gst_caps_append_structure(caps.get(), gst_structure_new("application/x-cenc", "original-media-type", G_TYPE_STRING, cencEncryptionMediaTypes[i], "protection-system", G_TYPE_STRING, uuid, nullptr));
}

static GRefPtr<GstCaps> createSinkPadTemplateCaps()
{
    std::string emptyString;
    GRefPtr<GstCaps> caps = adoptGRef(gst_caps_new_empty());

    if (!opencdm_is_type_supported(WebCore::GStreamerEMEUtilities::s_ClearKeyKeySystem, emptyString.c_str()))
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID);

    if (!opencdm_is_type_supported(WebCore::GStreamerEMEUtilities::s_PlayReadyKeySystems[0], emptyString.c_str()))
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID);

    if (!opencdm_is_type_supported(WebCore::GStreamerEMEUtilities::s_WidevineKeySystem, emptyString.c_str())) {
        addKeySystemToSinkPadCaps(caps, WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID);
        // No key system UUID for webm. It's not set in caps for it.
        for (int i = 0; webmEncryptionMediaTypes[i]; ++i)
            gst_caps_append_structure(caps.get(), gst_structure_new("application/x-webm-enc", "original-media-type", G_TYPE_STRING, webmEncryptionMediaTypes[i], nullptr));
    }

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
    cencClass->handleKeyId = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorHandleKeyId);
    cencClass->decrypt = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorDecrypt);
    cencClass->attemptToDecryptWithLocalInstance = GST_DEBUG_FUNCPTR(webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance);

    g_type_class_add_private(klass, sizeof(WebKitOpenCDMDecryptPrivate));
}

static void webkit_media_opencdm_decrypt_init(WebKitOpenCDMDecrypt* self)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);
    priv->m_openCdmSession = nullptr;
    self->priv = priv;
    new (priv) WebKitOpenCDMDecryptPrivate();
    GST_TRACE_OBJECT(self, "created");
}

static void webKitMediaOpenCDMDecryptorFinalize(GObject* object)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(object));
    priv->~WebKitOpenCDMDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static SessionResult webKitMediaOpenCDMDecryptorResetSessionFromKeyIdIfNeeded(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::SharedBuffer& keyId)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(WEBKIT_OPENCDM_DECRYPT(self));

    LockHolder locker(priv->m_mutex);
    RefPtr<WebCore::CDMInstance> cdmInstance = webKitMediaCommonEncryptionDecryptCDMInstance(self);
    ASSERT(cdmInstance && is<WebCore::CDMInstanceOpenCDM>(*cdmInstance));
    auto& cdmInstanceOpenCDM = downcast<WebCore::CDMInstanceOpenCDM>(*cdmInstance);

    SessionResult returnValue = InvalidSession;
    String session = cdmInstanceOpenCDM.sessionIdByKeyId(keyId);
    if (session.isEmpty() || !cdmInstanceOpenCDM.isKeyIdInSessionUsable(keyId, session)) {
        GST_DEBUG_OBJECT(self, "session %s is empty or unusable, resetting", session.utf8().data());
        priv->m_session = String();
        priv->m_openCdmSession = nullptr;
    } else if (session != priv->m_session) {
        priv->m_session = session;
        priv->m_openCdmSession = nullptr;
        GST_DEBUG_OBJECT(self, "new session %s is usable", session.utf8().data());
        returnValue = NewSession;
    } else {
        GST_DEBUG_OBJECT(self, "same session %s", session.utf8().data());
        returnValue = OldSession;
    }

    return returnValue;
}

static bool webKitMediaOpenCDMDecryptorHandleKeyId(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::SharedBuffer& keyId)
{
    return webKitMediaOpenCDMDecryptorResetSessionFromKeyIdIfNeeded(self, keyId) == InvalidSession;
}

static bool webKitMediaOpenCDMDecryptorAttemptToDecryptWithLocalInstance(WebKitMediaCommonEncryptionDecrypt* self, const WebCore::SharedBuffer& keyId)
{
    return webKitMediaOpenCDMDecryptorResetSessionFromKeyIdIfNeeded(self, keyId) != InvalidSession;
}

static bool webKitMediaOpenCDMDecryptorDecrypt(WebKitMediaCommonEncryptionDecrypt* self, GstBuffer* keyIDBuffer, GstBuffer* ivBuffer, GstBuffer* buffer, unsigned subSampleCount, GstBuffer* subSamplesBuffer)
{
    WebKitOpenCDMDecryptPrivate* priv = GST_WEBKIT_OPENCDM_DECRYPT_GET_PRIVATE(self);

    GstMappedBuffer mappedKeyID(keyIDBuffer, GST_MAP_READ);
    if (!mappedKeyID) {
        GST_ERROR_OBJECT(self, "Failed to map key ID buffer");
        return false;
    }

    if (!priv->m_openCdmSession) {
        LockHolder locker(priv->m_mutex);
        RefPtr<WebCore::CDMInstance> cdmInstance = webKitMediaCommonEncryptionDecryptCDMInstance(self);
        ASSERT(cdmInstance && is<WebCore::CDMInstanceOpenCDM>(*cdmInstance));
        auto& cdmInstanceOpenCDM = downcast<WebCore::CDMInstanceOpenCDM>(*cdmInstance);
        priv->m_openCdmSession.reset(opencdm_get_system_session(cdmInstanceOpenCDM.ocdmSystem(), mappedKeyID.data(), mappedKeyID.size(), WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT.millisecondsAs<uint32_t>()));
        if (!priv->m_openCdmSession) {
            GST_ERROR_OBJECT(self, "session is empty or unusable");
            return false;
        }
    }

    // Decrypt cipher.
    GST_TRACE_OBJECT(self, "decrypting");
    if (int errorCode = opencdm_gstreamer_session_decrypt(priv->m_openCdmSession.get(), buffer, subSamplesBuffer, subSampleCount, ivBuffer, keyIDBuffer, 0)) {
        GST_ERROR_OBJECT(self, "subsample decryption failed, error code %d", errorCode);
        return false;
    }

    return true;
}
#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OPENCDM)
