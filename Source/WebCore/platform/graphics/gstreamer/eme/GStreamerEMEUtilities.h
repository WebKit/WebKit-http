/* GStreamer EME Utilities class
 *
 * Copyright (C) 2017 Metrological
 * Copyright (C) 2017 Igalia S.L
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

#include "GStreamerCommon.h"
#include <gst/gst.h>
#include <wtf/text/WTFString.h>
#include <wtf/Seconds.h>

#define WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID "58147ec8-0423-4659-92e6-f52c5ce8c3cc"
#if USE(OPENCDM)
#define WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID "9a04f079-9840-4286-ab92-e65be0885f95"
#endif
#if USE(OPENCDM)
#define WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID "edef8ba9-79d6-4ace-a3c8-27dcd51d21ed"
#endif

// NOTE: YouTube 2018 EME conformance tests expect this to be >=5s.
const WTF::Seconds WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT = WTF::Seconds(6);

namespace WebCore {

using InitData = String;

class GStreamerEMEUtilities {

public:
    static const char* s_ClearKeyUUID;
    static const char* s_ClearKeyKeySystem;
    static const char* s_UnspecifiedUUID;
    static const char* s_UnspecifiedKeySystem;

#if USE(OPENCDM)
    static const char* s_PlayReadyUUID;
    static std::array<const char*, 2> s_PlayReadyKeySystems;
#endif

#if USE(OPENCDM)
    static const char* s_WidevineUUID;
    static const char* s_WidevineKeySystem;
#endif

    static bool isClearKeyKeySystem(const String& keySystem)
    {
        return equalIgnoringASCIICase(keySystem, s_ClearKeyKeySystem);
    }

    static bool isUnspecifiedKeySystem(const String& keySystem)
    {
        return equalIgnoringASCIICase(keySystem, s_UnspecifiedKeySystem);
    }

#if USE(OPENCDM)
    static bool isPlayReadyKeySystem(const String& keySystem)
    {
        return equalIgnoringASCIICase(keySystem, s_PlayReadyKeySystems[0])
            || equalIgnoringASCIICase(keySystem, s_PlayReadyKeySystems[1]);
    }

    static bool isWidevineKeySystem(const String& keySystem)
    {
        return equalIgnoringASCIICase(keySystem, s_WidevineKeySystem);
    }
#endif

    static const char* keySystemToUuid(const String& keySystem)
    {
        if (isClearKeyKeySystem(keySystem))
            return s_ClearKeyUUID;

        if (isUnspecifiedKeySystem(keySystem))
            return s_UnspecifiedUUID;

#if USE(OPENCDM)
        if (isPlayReadyKeySystem(keySystem))
            return s_PlayReadyUUID;

        if (isWidevineKeySystem(keySystem))
            return s_WidevineUUID;
#endif

        ASSERT_NOT_REACHED();
        return nullptr;
    }

    static const char* uuidToKeySystem(const String& uuid)
    {
        if (uuid == s_ClearKeyUUID)
            return s_ClearKeyKeySystem;

        if (uuid == s_UnspecifiedUUID)
            return s_UnspecifiedKeySystem;

#if USE(OPENCDM)
        if (uuid == s_PlayReadyUUID)
            return s_PlayReadyKeySystems[0];

        if (uuid == s_WidevineUUID)
            return s_WidevineKeySystem;
#endif

        ASSERT_NOT_REACHED();
        return nullptr;
    }

#if (!defined(GST_DISABLE_GST_DEBUG))
    static String initDataMD5(const InitData&);
#endif
};

}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
