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

#include "config.h"
#include "GStreamerEMEUtilities.h"

#include <wtf/MD5.h>
#include <wtf/text/Base64.h>

#if ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)

namespace WebCore {

const char* GStreamerEMEUtilities::s_ClearKeyUUID = WEBCORE_GSTREAMER_EME_UTILITIES_CLEARKEY_UUID;
const char* GStreamerEMEUtilities::s_ClearKeyKeySystem = "org.w3.clearkey";
const char* GStreamerEMEUtilities::s_UnspecifiedUUID = GST_PROTECTION_UNSPECIFIED_SYSTEM_ID;
const char* GStreamerEMEUtilities::s_UnspecifiedKeySystem = "org.webkit.unspecifiedkeysystem";

#if USE(OPENCDM)
const char* GStreamerEMEUtilities::s_PlayReadyUUID = WEBCORE_GSTREAMER_EME_UTILITIES_PLAYREADY_UUID;
std::array<const char*,2> GStreamerEMEUtilities::s_PlayReadyKeySystems = { "com.microsoft.playready", "com.youtube.playready" };
#endif

#if USE(OPENCDM)
const char* GStreamerEMEUtilities::s_WidevineUUID = WEBCORE_GSTREAMER_EME_UTILITIES_WIDEVINE_UUID;
const char* GStreamerEMEUtilities::s_WidevineKeySystem = "com.widevine.alpha";
#endif

#if (!defined(GST_DISABLE_GST_DEBUG))
String GStreamerEMEUtilities::initDataMD5(const InitData& initData)
{
    WTF::MD5 md5;
    md5.addBytes(static_cast<const uint8_t*>(initData.characters8()), initData.length());

    WTF::MD5::Digest digest;
    md5.checksum(digest);

    return WTF::base64URLEncode(&digest[0], WTF::MD5::hashSize);
}
#endif

}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
