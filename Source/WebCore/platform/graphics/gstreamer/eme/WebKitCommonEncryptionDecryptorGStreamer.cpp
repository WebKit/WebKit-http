/* GStreamer ClearKey common encryption decryptor
 *
 * Copyright (C) 2013 YouView TV Ltd. <alex.ashley@youview.com>
 * Copyright (C) 2016 Metrological
 * Copyright (C) 2016 Igalia S.L
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
#include "WebKitCommonEncryptionDecryptorGStreamer.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

#include "GStreamerEMEUtilities.h"
#include <CDMInstance.h>
#include <wtf/Condition.h>
#include <wtf/PrintStream.h>
#include <wtf/RunLoop.h>

#define WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), WEBKIT_TYPE_MEDIA_CENC_DECRYPT, WebKitMediaCommonEncryptionDecryptPrivate))
struct _WebKitMediaCommonEncryptionDecryptPrivate {
    bool m_keyReceived;
    Lock m_mutex;
    Condition m_condition;
    RefPtr<WebCore::CDMInstance> m_cdmInstance;
    WebCore::InitData m_initData;
    GRefPtr<GstEvent> m_pendingProtectionEvent;
};

static GstStateChangeReturn webKitMediaCommonEncryptionDecryptorChangeState(GstElement*, GstStateChange transition);
static void webKitMediaCommonEncryptionDecryptorFinalize(GObject*);
static GstCaps* webkitMediaCommonEncryptionDecryptTransformCaps(GstBaseTransform*, GstPadDirection, GstCaps*, GstCaps*);
static GstFlowReturn webkitMediaCommonEncryptionDecryptTransformInPlace(GstBaseTransform*, GstBuffer*);
static gboolean webkitMediaCommonEncryptionDecryptSinkEventHandler(GstBaseTransform*, GstEvent*);

GST_DEBUG_CATEGORY_STATIC(webkit_media_common_encryption_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_common_encryption_decrypt_debug_category

#define webkit_media_common_encryption_decrypt_parent_class parent_class
G_DEFINE_TYPE(WebKitMediaCommonEncryptionDecrypt, webkit_media_common_encryption_decrypt, GST_TYPE_BASE_TRANSFORM);

static void webkit_media_common_encryption_decrypt_class_init(WebKitMediaCommonEncryptionDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaCommonEncryptionDecryptorFinalize;

    GST_DEBUG_CATEGORY_INIT(webkit_media_common_encryption_decrypt_debug_category,
        "webkitcenc", 0, "Common Encryption base class");

    GstElementClass* elementClass = GST_ELEMENT_CLASS(klass);
    elementClass->change_state = GST_DEBUG_FUNCPTR(webKitMediaCommonEncryptionDecryptorChangeState);

    GstBaseTransformClass* baseTransformClass = GST_BASE_TRANSFORM_CLASS(klass);
    baseTransformClass->transform_ip = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptTransformInPlace);
    baseTransformClass->transform_caps = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptTransformCaps);
    baseTransformClass->transform_ip_on_passthrough = FALSE;
    baseTransformClass->sink_event = GST_DEBUG_FUNCPTR(webkitMediaCommonEncryptionDecryptSinkEventHandler);

    klass->setupCipher = [](WebKitMediaCommonEncryptionDecrypt*, GstBuffer*) { return true; };
    klass->releaseCipher = [](WebKitMediaCommonEncryptionDecrypt*) { };
    klass->handleInitData = [](WebKitMediaCommonEncryptionDecrypt*, const WebCore::InitData&) { return true; };
    klass->attemptToDecryptWithLocalInstance = [](WebKitMediaCommonEncryptionDecrypt*, const WebCore::InitData&) { return false; };

    g_type_class_add_private(klass, sizeof(WebKitMediaCommonEncryptionDecryptPrivate));
}

static void webkit_media_common_encryption_decrypt_init(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    self->priv = priv;
    new (priv) WebKitMediaCommonEncryptionDecryptPrivate();

    GstBaseTransform* base = GST_BASE_TRANSFORM(self);
    gst_base_transform_set_in_place(base, TRUE);
    gst_base_transform_set_passthrough(base, FALSE);
    gst_base_transform_set_gap_aware(base, FALSE);
}

static void webKitMediaCommonEncryptionDecryptorFinalize(GObject* object)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(object);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = self->priv;

    priv->~WebKitMediaCommonEncryptionDecryptPrivate();
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void webKitMediaCommonEncryptionDecryptorPrivClearStateWithInstance(WebKitMediaCommonEncryptionDecryptPrivate* priv, WebCore::CDMInstance* cdmInstance)
{
    ASSERT(priv);
    ASSERT(priv->m_mutex.isLocked());
    priv->m_cdmInstance = cdmInstance;
    priv->m_keyReceived = false;
    priv->m_initData = String();
}

static GstCaps* webkitMediaCommonEncryptionDecryptTransformCaps(GstBaseTransform* base, GstPadDirection direction, GstCaps* caps, GstCaps* filter)
{
    if (direction == GST_PAD_UNKNOWN)
        return nullptr;

    GST_LOG_OBJECT(base, "direction: %s, caps: %" GST_PTR_FORMAT " filter: %" GST_PTR_FORMAT, (direction == GST_PAD_SRC) ? "src" : "sink", caps, filter);

    GstCaps* transformedCaps = gst_caps_new_empty();
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(base);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);

    unsigned size = gst_caps_get_size(caps);
    for (unsigned i = 0; i < size; ++i) {
        GstStructure* incomingStructure = gst_caps_get_structure(caps, i);
        GUniquePtr<GstStructure> outgoingStructure = nullptr;

        if (direction == GST_PAD_SINK) {
            if (!gst_structure_has_field(incomingStructure, "original-media-type"))
                continue;

            outgoingStructure = GUniquePtr<GstStructure>(gst_structure_copy(incomingStructure));
            gst_structure_set_name(outgoingStructure.get(), gst_structure_get_string(outgoingStructure.get(), "original-media-type"));

            // Filter out the DRM related fields from the down-stream caps.
            for (int j = 0; j < gst_structure_n_fields(incomingStructure); ++j) {
                const gchar* fieldName = gst_structure_nth_field_name(incomingStructure, j);

                if (g_str_has_prefix(fieldName, "protection-system")
                    || g_str_has_prefix(fieldName, "original-media-type"))
                    gst_structure_remove_field(outgoingStructure.get(), fieldName);
            }
        } else {
            outgoingStructure = GUniquePtr<GstStructure>(gst_structure_copy(incomingStructure));
            // Filter out the video related fields from the up-stream caps,
            // because they are not relevant to the input caps of this element and
            // can cause caps negotiation failures with adaptive bitrate streams.
            for (int index = gst_structure_n_fields(outgoingStructure.get()) - 1; index >= 0; --index) {
                const gchar* fieldName = gst_structure_nth_field_name(outgoingStructure.get(), index);
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
                    gst_structure_remove_field(outgoingStructure.get(), fieldName);
                    GST_TRACE("Removing field %s", fieldName);
                }
            }

            gst_structure_set(outgoingStructure.get(), "protection-system", G_TYPE_STRING, klass->protectionSystemId,
                "original-media-type", G_TYPE_STRING, gst_structure_get_name(incomingStructure), nullptr);

            gst_structure_set_name(outgoingStructure.get(), "application/x-cenc");
        }

        bool duplicate = false;
        unsigned size = gst_caps_get_size(transformedCaps);

        for (unsigned index = 0; !duplicate && index < size; ++index) {
            GstStructure* structure = gst_caps_get_structure(transformedCaps, index);
            if (gst_structure_is_equal(structure, outgoingStructure.get()))
                duplicate = true;
        }

        if (!duplicate)
            gst_caps_append_structure(transformedCaps, outgoingStructure.release());
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

static GstFlowReturn webkitMediaCommonEncryptionDecryptTransformInPlace(GstBaseTransform* base, GstBuffer* buffer)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(base);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    GstProtectionMeta* protectionMeta = reinterpret_cast<GstProtectionMeta*>(gst_buffer_get_protection_meta(buffer));
    if (!protectionMeta) {
        GST_DEBUG_OBJECT(self, "Failed to get GstProtection metadata from buffer %p, assuming it's not encrypted", buffer);
        return GST_FLOW_OK;
    }

    LockHolder locker(priv->m_mutex);

    // The key might not have been received yet. Wait for it.
    if (!priv->m_keyReceived) {
        GST_DEBUG_OBJECT(self, "key not available yet, waiting for it");
        if (GST_STATE(GST_ELEMENT(self)) < GST_STATE_PAUSED || (GST_STATE_TARGET(GST_ELEMENT(self)) != GST_STATE_VOID_PENDING && GST_STATE_TARGET(GST_ELEMENT(self)) < GST_STATE_PAUSED)) {
            GST_ERROR_OBJECT(self, "can't process key requests in less than PAUSED state");
            return GST_FLOW_NOT_SUPPORTED;
        }
        if (!priv->m_condition.waitFor(priv->m_mutex, Seconds(5), [priv] { return priv->m_keyReceived; })) {
            GST_ERROR_OBJECT(self, "key not available");
            return GST_FLOW_NOT_SUPPORTED;
        }
        GST_DEBUG_OBJECT(self, "key received, continuing");
    }

    unsigned ivSize;
    if (!gst_structure_get_uint(protectionMeta->info, "iv_size", &ivSize)) {
        GST_ERROR_OBJECT(self, "Failed to get iv_size");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    gboolean encrypted;
    if (!gst_structure_get_boolean(protectionMeta->info, "encrypted", &encrypted)) {
        GST_ERROR_OBJECT(self, "Failed to get encrypted flag");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    if (!ivSize || !encrypted) {
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_OK;
    }

    GST_TRACE_OBJECT(base, "protection meta: %" GST_PTR_FORMAT, protectionMeta->info);

    unsigned subSampleCount;
    if (!gst_structure_get_uint(protectionMeta->info, "subsample_count", &subSampleCount)) {
        GST_ERROR_OBJECT(self, "Failed to get subsample_count");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    const GValue* value;
    GstBuffer* subSamplesBuffer = nullptr;
    if (subSampleCount) {
        value = gst_structure_get_value(protectionMeta->info, "subsamples");
        if (!value) {
            GST_ERROR_OBJECT(self, "Failed to get subsamples");
            gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
            return GST_FLOW_NOT_SUPPORTED;
        }
        subSamplesBuffer = gst_value_get_buffer(value);
    }

    value = gst_structure_get_value(protectionMeta->info, "kid");
    GstBuffer* keyIDBuffer = nullptr;
    if (value)
        keyIDBuffer = gst_value_get_buffer(value);

    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);
    if (!klass->setupCipher(self, keyIDBuffer)) {
        GST_ERROR_OBJECT(self, "Failed to configure cipher");
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    value = gst_structure_get_value(protectionMeta->info, "iv");
    if (!value) {
        GST_ERROR_OBJECT(self, "Failed to get IV for sample");
        klass->releaseCipher(self);
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    GstBuffer* ivBuffer = gst_value_get_buffer(value);
    GST_TRACE_OBJECT(self, "decrypting");
    if (!klass->decrypt(self, ivBuffer, buffer, subSampleCount, subSamplesBuffer)) {
        GST_ERROR_OBJECT(self, "Decryption failed");
        klass->releaseCipher(self);
        gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
        return GST_FLOW_NOT_SUPPORTED;
    }

    klass->releaseCipher(self);
    gst_buffer_remove_meta(buffer, reinterpret_cast<GstMeta*>(protectionMeta));
    return GST_FLOW_OK;
}

static void webkitMediaCommonEncryptionDecryptProcessPendingProtectionEvent(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);

    ASSERT(priv->m_mutex.isLocked());

    GRefPtr<GstEvent> event = WTFMove(priv->m_pendingProtectionEvent);
    GstBuffer* buffer = nullptr;
    gst_event_parse_protection(event.get(), nullptr, &buffer, nullptr);

    GST_TRACE_OBJECT(self, "current init data size %u, MD5 %s", priv->m_initData.sizeInBytes(), WebCore::GStreamerEMEUtilities::initDataMD5(priv->m_initData).utf8().data());
    GST_MEMDUMP_OBJECT(self, "init data", reinterpret_cast<const uint8_t*>(priv->m_initData.characters8()), priv->m_initData.sizeInBytes());
    if (priv->m_initData.isEmpty() || gst_buffer_memcmp(buffer, 0, priv->m_initData.characters8(), priv->m_initData.sizeInBytes())) {
        GstMapInfo mapInfo;
        if (!gst_buffer_map(buffer, &mapInfo, GST_MAP_READ)) {
            GST_WARNING_OBJECT(self, "cannot map protection data");
            return;
        }

        priv->m_initData = WebCore::InitData(reinterpret_cast<const uint8_t*>(mapInfo.data), mapInfo.size);
        GST_DEBUG_OBJECT(self, "accepting init data of size %u", mapInfo.size);
        GST_TRACE_OBJECT(self, "init data MD5 %s", WebCore::GStreamerEMEUtilities::initDataMD5(priv->m_initData).utf8().data());
        GST_MEMDUMP_OBJECT(self, "init data", reinterpret_cast<const uint8_t*>(mapInfo.data), mapInfo.size);
        gst_buffer_unmap(buffer, &mapInfo);

        priv->m_keyReceived = !klass->handleInitData(self, priv->m_initData);
        if (!priv->m_keyReceived) {
            GST_DEBUG_OBJECT(self, "posting event and considering key not received");
            gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), gst_structure_new("drm-initialization-data-encountered", "event", GST_TYPE_EVENT, event.get(), nullptr)));
        } else {
            GST_DEBUG_OBJECT(self, "key is already usable");
            priv->m_condition.notifyOne();
        }
    } else
        GST_DEBUG_OBJECT(self, "init data already present");
}

static gboolean webkitMediaCommonEncryptionDecryptSinkEventHandler(GstBaseTransform* trans, GstEvent* event)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(trans);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    WebKitMediaCommonEncryptionDecryptClass* klass = WEBKIT_MEDIA_CENC_DECRYPT_GET_CLASS(self);
    gboolean result = FALSE;

    switch (GST_EVENT_TYPE(event)) {
    case GST_EVENT_PROTECTION: {
        const char* systemId = nullptr;

        gst_event_parse_protection(event, &systemId, nullptr, nullptr);
        GST_TRACE_OBJECT(self, "received protection event %u for %s", GST_EVENT_SEQNUM(event), systemId);

        if (!g_strcmp0(systemId, klass->protectionSystemId)) {
            LockHolder locker(priv->m_mutex);
            priv->m_pendingProtectionEvent = event;
            if (priv->m_cdmInstance)
                webkitMediaCommonEncryptionDecryptProcessPendingProtectionEvent(self);
            else {
                GST_DEBUG_OBJECT(self, "protection event buffer kept for later because we have no CDMInstance yet, requesting");
                gst_element_post_message(GST_ELEMENT(self), gst_message_new_element(GST_OBJECT(self), gst_structure_new_empty("drm-cdm-instance-needed")));
            }
        } else
            GST_TRACE_OBJECT(self, "protection event for a different key system");

        result = TRUE;
        gst_event_unref(event);
        break;
    }
    case GST_EVENT_CUSTOM_DOWNSTREAM_OOB: {
        const GstStructure* structure = gst_event_get_structure(event);
        if (gst_structure_has_name(structure, "drm-cdm-instance-attached")) {
            WebCore::CDMInstance* cdmInstance = nullptr;
            gst_structure_get(structure, "cdm-instance", G_TYPE_POINTER, &cdmInstance, nullptr);
            gst_event_unref(event);
            result = TRUE;
            ASSERT(cdmInstance);
            LockHolder locker(priv->m_mutex);
            if (priv->m_cdmInstance != cdmInstance) {
                GST_INFO_OBJECT(self, "got new CDMInstance %p attached (ours was %p), clearing state", cdmInstance, priv->m_cdmInstance.get());
                webKitMediaCommonEncryptionDecryptorPrivClearStateWithInstance(priv, cdmInstance);
                if (priv->m_pendingProtectionEvent)
                    webkitMediaCommonEncryptionDecryptProcessPendingProtectionEvent(self);
            } else
                GST_TRACE_OBJECT(self, "got attach CDMInstance for the same instance %p we already have", cdmInstance);
        } else if (gst_structure_has_name(structure, "drm-cdm-instance-detached")) {
            WebCore::CDMInstance* cdmInstance = nullptr;
            gst_structure_get(structure, "cdm-instance", G_TYPE_POINTER, &cdmInstance, nullptr);
            gst_event_unref(event);
            result = TRUE;
            ASSERT(cdmInstance);
            LockHolder locker(priv->m_mutex);
            if (priv->m_cdmInstance == cdmInstance) {
                webKitMediaCommonEncryptionDecryptorPrivClearStateWithInstance(priv, nullptr);
                GST_INFO_OBJECT(self, "got CDMInstance %p detached and cleared state", cdmInstance);
            } else
                GST_TRACE_OBJECT(self, "got detaching message for CDMInstance %p but ours is %p", cdmInstance, priv->m_cdmInstance.get());
        } else if (gst_structure_has_name(structure, "drm-attempt-to-decrypt-with-local-instance")) {
            gst_event_unref(event);
            result = TRUE;
            LockHolder locker(priv->m_mutex);
            priv->m_keyReceived = klass->attemptToDecryptWithLocalInstance(self, priv->m_initData);
            GST_DEBUG_OBJECT(self, "attempted to decrypt with local instance %p, key received %s", priv->m_cdmInstance.get(), boolForPrinting(priv->m_keyReceived));
            if (priv->m_keyReceived)
                priv->m_condition.notifyOne();
        }
        break;
    }
    default:
        result = GST_BASE_TRANSFORM_CLASS(parent_class)->sink_event(trans, event);
        break;
    }

    return result;
}

static GstStateChangeReturn webKitMediaCommonEncryptionDecryptorChangeState(GstElement* element, GstStateChange transition)
{
    WebKitMediaCommonEncryptionDecrypt* self = WEBKIT_MEDIA_CENC_DECRYPT(element);
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);

    switch (transition) {
    case GST_STATE_CHANGE_PAUSED_TO_READY:
        GST_DEBUG_OBJECT(self, "PAUSED->READY");
        priv->m_condition.notifyOne();
        break;
    default:
        break;
    }

    GstStateChangeReturn result = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);

    // Add post-transition code here.

    return result;
}

RefPtr<WebCore::CDMInstance> webKitMediaCommonEncryptionDecryptCDMInstance(WebKitMediaCommonEncryptionDecrypt* self)
{
    WebKitMediaCommonEncryptionDecryptPrivate* priv = WEBKIT_MEDIA_CENC_DECRYPT_GET_PRIVATE(self);
    ASSERT(priv->m_mutex.isLocked());
    return priv->m_cdmInstance;
}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
