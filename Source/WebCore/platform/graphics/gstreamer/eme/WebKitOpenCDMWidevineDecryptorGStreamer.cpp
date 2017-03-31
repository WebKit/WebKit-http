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

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)) && USE(GSTREAMER) && USE(OCDM)

static void webKitMediaOpenCDMWidevineDecryptorFinalize(GObject*);

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
G_DEFINE_TYPE(WebKitOpenCDMWidevineDecrypt, webkit_media_opencdm_widevine_decrypt, WEBKIT_TYPE_OPENCDM_DECRYPT);

static void webkit_media_opencdm_widevine_decrypt_class_init(WebKitOpenCDMWidevineDecryptClass* klass)
{
    GObjectClass* gobjectClass = G_OBJECT_CLASS(klass);
    gobjectClass->finalize = webKitMediaOpenCDMWidevineDecryptorFinalize;

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
}

static void webkit_media_opencdm_widevine_decrypt_init(WebKitOpenCDMWidevineDecrypt*)
{
}

static void webKitMediaOpenCDMWidevineDecryptorFinalize(GObject* object)
{
    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)) && USE(GSTREAMER) && USE(OCDM)
