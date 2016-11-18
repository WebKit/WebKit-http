#ifndef GST_OPENCDMI_WIDEVINE_DECRYPTOR_H
#define GST_OPENCDMI_WIDEVINE_DECRYPTOR_H

#include <stdint.h>

#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(GSTREAMER) && USE(OCDM)

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
