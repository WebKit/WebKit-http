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
#include <wtf/text/WTFString.h>
#include <wtf/Seconds.h>

// NOTE: YouTube 2018 EME conformance tests expect this to be >=5s.
const WTF::Seconds WEBCORE_GSTREAMER_EME_LICENSE_KEY_RESPONSE_TIMEOUT = WTF::Seconds(6);

namespace WebCore {

using InitData = String;

class GStreamerEMEUtilities {

public:

#if (!defined(GST_DISABLE_GST_DEBUG))
    static String initDataMD5(const InitData&);
#endif
};

}

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(GSTREAMER)
