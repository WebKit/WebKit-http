/*
 * Copyright (C) 2014 FLUENDO S.A. All rights reserved.
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
#include "CDMSessionGStreamer.h"

#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(GSTREAMER)

#include "CDM.h"
#include "CDMSession.h"
#include "UUID.h"
#include "MediaKeyError.h"
#include "MediaPlayerPrivateGStreamer.h"
#include <wtf/text/StringBuilder.h>
#include <wtf/text/CString.h>

#include "dxdrm/DxDrmDebugApi.h"

GST_DEBUG_CATEGORY_EXTERN(webkit_media_player_debug);
#define GST_CAT_DEFAULT webkit_media_player_debug

#define MAX_CHALLENGE_LEN 100000

namespace WebCore {

CDMSessionGStreamer::CDMSessionGStreamer(MediaPlayerPrivateGStreamer* parent)
    : m_parent(parent)
    , m_client(nullptr)
    , m_sessionId(createCanonicalUUIDString())
#if USE(DXDRM)
    , m_DxDrmStream(nullptr)
#endif
{
}

CDMSessionGStreamer::~CDMSessionGStreamer()
{
#if USE(DXDRM)
    if (m_DxDrmStream) {
        DxDrmStream_Close(&m_DxDrmStream);
        m_DxDrmStream = nullptr;
    }
#endif
}

PassRefPtr<Uint8Array> CDMSessionGStreamer::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode)
{
    UNUSED_PARAM(mimeType);
    UNUSED_PARAM(errorCode);
    UNUSED_PARAM(systemCode);

#if USE(DXDRM)
    // Instantiate Discretix DRM client from init data. This could be the WRMHEADER or a complete ASF header..
    EDxDrmStatus status = DxDrmClient_OpenDrmStreamFromData(&m_DxDrmStream, initData->data(), initData->byteLength());
    if (status != DX_SUCCESS) {
        GST_WARNING("failed creating DxDrmClient from initData (%d)", status);
        errorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
        return nullptr;
    }

    // Set Secure Clock
    status = DxDrmStream_AdjustClock(m_DxDrmStream, DX_AUTO_NO_UI);
    if (status != DX_SUCCESS) {
        GST_WARNING("failed setting secure clock (%d)", status);
        errorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
        systemCode = status;
        return nullptr;
    }

    guint32 challenge_length = MAX_CHALLENGE_LEN;
    gpointer challenge = g_malloc0(challenge_length);

    // Get challenge
    status = DxDrmStream_GetLicenseChallenge(m_DxDrmStream, challenge, (DxUint32 *) &challenge_length);
    if (status != DX_SUCCESS) {
        GST_WARNING("failed to generate challenge request (%d)", status);
        g_free(challenge);
        return nullptr;
    }

    // Get License URL
    destinationURL = (const char *) DxDrmStream_GetTextAttribute(m_DxDrmStream, DX_ATTR_SILENT_URL, DX_ACTIVE_CONTENT);

    GST_DEBUG("destination URL : %s", destinationURL.utf8().data());
    GST_MEMDUMP("generated license request :", (const guint8 *) challenge, challenge_length);

    PassRefPtr<Uint8Array> result = Uint8Array::create(reinterpret_cast<const unsigned char *>(challenge), challenge_length);

    g_free(challenge);

    // This is the first stage of license aquisition.
    m_waitAck = false;

    return result;
#else
    return nullptr;
#endif
}

void CDMSessionGStreamer::releaseKeys()
{
    m_parent->signalDRM();
}

bool CDMSessionGStreamer::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode)
{
    GST_MEMDUMP("response received :", key->data(), key->byteLength());

#if USE(DXDRM)
    bool ret = false;
    DxBool isAckRequired = false;
    HDxResponseResult responseResult = nullptr;
    EDxDrmStatus status;

    if (m_waitAck == false) {
        // Server replied to our license request
        status = DxDrmStream_ProcessLicenseResponse (m_DxDrmStream, key->data (), key->byteLength (), &responseResult, &isAckRequired);
    } else {
        // Server replied to our license response acknowledge
        status = DxDrmClient_ProcessServerResponse (key->data (), key->byteLength (), DX_RESPONSE_LICENSE_ACK, &responseResult, &isAckRequired);
        if (isAckRequired) {
            GST_WARNING ("ack required when processing ack of ack !");
        }
    }

    if (status != DX_SUCCESS) {
        GST_WARNING("failed processing license response (%d)", status);
        goto error;
    }

    if (!m_waitAck && isAckRequired) {
        guint32 challenge_length = MAX_CHALLENGE_LEN;
        gpointer challenge = g_malloc0(challenge_length);

        status = DxDrmClient_GetLicenseAcq_GenerateAck(&responseResult, challenge, (DxUint32 *) &challenge_length);
        if (status != DX_SUCCESS) {
            GST_WARNING("failed generating license ack challenge (%d)", status);
            g_free(challenge);
            goto error;
        }

        GST_MEMDUMP ("generated license ack request :", (const guint8 *) challenge, challenge_length);
        nextMessage = Uint8Array::create(reinterpret_cast<const unsigned char *>(challenge), challenge_length);

        g_free(challenge);
        m_waitAck = true;
    } else {
        // Notify the player instance that a key was added
        m_parent->signalDRM();
        ret = true;
    }
    return true;
error:
    errorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
    systemCode = status;
    // Notify Player that license acquisition failed
    m_parent->signalDRM();
    return ret;
#else
    return false;
#endif
}

}

#endif
