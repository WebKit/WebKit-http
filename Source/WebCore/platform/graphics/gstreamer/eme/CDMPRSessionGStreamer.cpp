/*
 * Copyright (C) 2014-2015 FLUENDO S.A. All rights reserved.
 * Copyright (C) 2014-2015 METROLOGICAL All rights reserved.
 * Copyright (C) 2015 IGALIA S.L All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY FLUENDO S.A. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL FLUENDO S.A. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CDMPRSessionGStreamer.h"

#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(GSTREAMER) && USE(PLAYREADY)

#include "CDM.h"
#include "MediaPlayerPrivateGStreamerBase.h"
#include "UUID.h"
#include "WebKitMediaKeyError.h"

#include <gst/gst.h>
#include <wtf/text/CString.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_playready_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_playready_decrypt_debug_category

namespace WebCore {

CDMPRSessionGStreamer::CDMPRSessionGStreamer(CDMSessionClient* client, MediaPlayerPrivateGStreamerBase* playerPrivate)
    // FIXME: Create PlayreadySession with actual initData and pipeline values.
    : PlayreadySession(Vector<uint8_t>(), nullptr)
    , m_client(client)
    , m_sessionId(createCanonicalUUIDString())
    , m_playerPrivate(playerPrivate)
{
}

CDMPRSessionGStreamer::~CDMPRSessionGStreamer()
{
}

CDMSessionType CDMPRSessionGStreamer::type()
{
    return CDMSessionTypePlayReady;
}

void CDMPRSessionGStreamer::setClient(CDMSessionClient* client)
{
    ASSERT((m_client == nullptr) ^ (client == nullptr));

    m_client = client;
}

const String& CDMPRSessionGStreamer::sessionId() const
{
    return m_sessionId;
}

RefPtr<Uint8Array> CDMPRSessionGStreamer::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, uint32_t& systemCode)
{
    GST_DEBUG("got request for %s", mimeType.utf8().data());
    m_playerPrivate->receivedGenerateKeyRequest(PLAYREADY_PROTECTION_SYSTEM_ID);
    return playreadyGenerateKeyRequest(initData, String(), destinationURL, errorCode, systemCode);
}

bool CDMPRSessionGStreamer::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t& systemCode)
{
    GST_DEBUG("processing key");
    return playreadyProcessKey(key, nextMessage, errorCode, systemCode);
}

void CDMPRSessionGStreamer::releaseKeys()
{
}

RefPtr<ArrayBuffer> CDMPRSessionGStreamer::cachedKeyForKeyID(const String& sessionId) const
{
    return (sessionId == m_sessionId ? m_key : nullptr);
}

}

#endif
