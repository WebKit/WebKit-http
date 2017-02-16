/*
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CDMSessionOpenCDM.h"

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OCDM)

#include "CDM.h"
#include "MediaPlayerPrivateGStreamerBase.h"
#include "WebKitMediaKeyError.h"
#include <gst/gst.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

namespace WebCore {

CDMSessionOpenCDM::CDMSessionOpenCDM(CDMSessionClient*, media::OpenCdm* openCdm, MediaPlayerPrivateGStreamerBase* playerPrivate, String openCdmKeySystem)
    : m_openCdmSession(openCdm)
    , m_playerPrivate(playerPrivate)
    , m_openCdmKeySystem(openCdmKeySystem)
    , m_eKeyState(KeyInit)
{
}

RefPtr<Uint8Array> CDMSessionOpenCDM::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationUrl, unsigned short& errorCode, uint32_t&)
{
    if (m_eKeyState != KeyInit)
        return nullptr;
    m_playerPrivate->receivedGenerateKeyRequest(m_openCdmKeySystem);

    std::string sessionId;
    m_openCdmSession->CreateSession(mimeType.utf8().data(), reinterpret_cast<unsigned char*>(initData->data()),
        initData->length(), sessionId);
    if (!sessionId.size()) {
        GST_ERROR("SessionId is empty\n");
        return nullptr;
    }
    m_sessionId = String::fromUTF8(sessionId.c_str());

    unsigned char temporaryUrl[1024] = {'\0'};
    std::string message;
    int messageLength = 0;
    int destinationUrlLength = 0;
    int returnValue = m_openCdmSession->GetKeyMessage(message,
        &messageLength, temporaryUrl, &destinationUrlLength);
    if (returnValue || !messageLength || !destinationUrlLength) {
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_UNKNOWN;
        return nullptr;
    }
    printf("Key state set to PENDING \n");
    m_eKeyState = KeyPending;
    destinationUrl = String::fromUTF8(temporaryUrl);
    return Uint8Array::create(reinterpret_cast<unsigned char*>(const_cast<char*>(message.c_str())), messageLength);
}

void CDMSessionOpenCDM::releaseKeys()
{
}

bool CDMSessionOpenCDM::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t&)
{
    if (m_eKeyState != KeyPending)
        return false;

    GST_MEMDUMP("License :", key->data(), key->length());

    std::string responseMessage;
    if (m_openCdmSession->Update(key->data(), key->length(), responseMessage)) {
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
        m_eKeyState = KeyError;
        return false;
    }
    responseMessage = "UpdateStatus: " + responseMessage;
    nextMessage = Uint8Array::create(reinterpret_cast<unsigned char*>(const_cast<char*>(responseMessage.c_str())), responseMessage.length());
    m_eKeyState = KeyReady;
    return true;
}

} // namespace WebCore

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OCDM)
