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

#pragma once

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)) && USE(GSTREAMER) && USE(OCDM)

#include "WebKitCommonEncryptionDecryptorGStreamer.h"

G_BEGIN_DECLS

#define WEBKIT_TYPE_OPENCDM_DECRYPT          (webkit_media_opencdm_decrypt_get_type())
#define WEBKIT_OPENCDM_DECRYPT(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), WEBKIT_TYPE_OPENCDM_DECRYPT, WebKitOpenCDMDecrypt))
#define WEBKIT_OPENCDM_DECRYPT_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), WEBKIT_TYPE_OPENCDM_DECRYPT, WebKitOpenCDMDecryptClass))

typedef struct _WebKitOpenCDMDecrypt           WebKitOpenCDMDecrypt;
typedef struct _WebKitOpenCDMDecryptClass      WebKitOpenCDMDecryptClass;
typedef struct _WebKitOpenCDMDecryptPrivate    WebKitOpenCDMDecryptPrivate;

GType webkit_media_opencdm_decrypt_get_type(void);

struct _WebKitOpenCDMDecrypt {
    WebKitMediaCommonEncryptionDecrypt parent;
    WebKitOpenCDMDecryptPrivate* priv;
};

struct _WebKitOpenCDMDecryptClass {
    WebKitMediaCommonEncryptionDecryptClass parentClass;
};

G_END_DECLS

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA_V1) || ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(ENCRYPTED_MEDIA)) && USE(GSTREAMER) && USE(OCDM)
