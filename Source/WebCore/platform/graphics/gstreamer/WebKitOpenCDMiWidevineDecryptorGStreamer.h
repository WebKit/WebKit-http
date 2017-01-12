/* GStreamer Widevine decryptor
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

#ifndef GST_OPENCDMI_WIDEVINE_DECRYPTOR_H
#define GST_OPENCDMI_WIDEVINE_DECRYPTOR_H

#include <stdint.h>

#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(OCDM)

#include "WebKitCommonEncryptionDecryptorGStreamer.h"

G_BEGIN_DECLS

#define OPENCDMI_TYPE_WIDEVINE_DECRYPT          (opencdmi_widevine_decrypt_get_type())
#define OPENCDMI_WIDEVINE_DECRYPT(obj)          (G_TYPE_CHECK_INSTANCE_CAST((obj), OPENCDMI_TYPE_WIDEVINE_DECRYPT, OpenCDMiWideVineDecrypt))
#define OPENCDMI_WIDEVINE_DECRYPT_CLASS(klass)  (G_TYPE_CHECK_CLASS_CAST((klass), OPENCDMI_TYPE_WIDEVINE_DECRYPT, OpenCDMiWideVineDecryptClass))
#define OPENCDMI_IS_WIDEVINE_DECRYPT(obj)       (G_TYPE_CHECK_INSTANCE_TYPE((obj), OPENCDMI_TYPE_WIDEVINE_DECRYPT))
#define OPENCDMI_IS_WIDEVINE_DECRYPT_CLASS(obj) (G_TYPE_CHECK_CLASS_TYPE((klass), OPENCDMI_TYPE_WIDEVINE_DECRYPT))

typedef struct _OpenCDMiWideVineDecrypt        OpenCDMiWideVineDecrypt;
typedef struct _OpenCDMiWideVineDecryptClass   OpenCDMiWideVineDecryptClass;
typedef struct _OpenCDMiWideVineDecryptPrivate OpenCDMiWideVineDecryptPrivate;

GType opencdmi_widevine_decrypt_get_type(void);

struct _OpenCDMiWideVineDecrypt {
    WebKitMediaCommonEncryptionDecrypt parent;

    OpenCDMiWideVineDecryptPrivate* priv;
};

struct _OpenCDMiWideVineDecryptClass {
    WebKitMediaCommonEncryptionDecryptClass parentClass;
};

G_END_DECLS

#endif
#endif
