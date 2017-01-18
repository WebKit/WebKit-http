
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
#include "WebKitOpenCDMSessionEncKey.h"

#if ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(OCDM)

#include "CDM.h"
#include "CDMSession.h"
#include <gst/gst.h>
#include "WebKitMediaKeyError.h"

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

namespace WebCore {

WebKitOpenCDMSessionEncKey::WebKitOpenCDMSessionEncKey(CDMSessionClient* client, OpenCdm* openCdm)
    : m_client(client)
    , m_openCdmSession(openCdm)
    , m_destinationUrlLength(0)
{
}

RefPtr<Uint8Array> WebKitOpenCDMSessionEncKey::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationUrl, unsigned short& errorCode, uint32_t& sysCode)
{
    UNUSED_PARAM(sysCode);
    int returnValue = 0;

    unsigned char initDataValue[initData->length()];

    memcpy(initDataValue, initData->data(), initData->length());

    string sessionId;
    m_openCdmSession->CreateSession(mimeType.utf8().data(), initDataValue,
        initData->length(), sessionId);
    if (!sessionId.size()) {
        GST_WARNING("SessionId is empty\n");
        return nullptr;
    }
    m_sessionId = String::fromUTF8(sessionId.c_str());

    unsigned char temporaryUrl[100] = "\0";
    string message;
    int messageLength = 0;
    returnValue = m_openCdmSession->GetKeyMessage(message,
        &messageLength, temporaryUrl, &m_destinationUrlLength);
    if (returnValue || !messageLength || !m_destinationUrlLength) {
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_UNKNOWN;
        return nullptr;
    }
    else {
        destinationUrl = String::fromUTF8(temporaryUrl);
    }
    return Uint8Array::create(reinterpret_cast<unsigned char*>(const_cast<char*>(message.c_str())), messageLength);
}

void WebKitOpenCDMSessionEncKey::releaseKeys()
{
}

bool WebKitOpenCDMSessionEncKey::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t& sysytemCode)
{
    UNUSED_PARAM(sysytemCode);

    GST_MEMDUMP("License :", key->data(), key->length());

    int returnValue = 0;
    std::string responseMessage;
    returnValue = m_openCdmSession->Update(key->data(), key->length(), responseMessage);
    if (returnValue) {
        responseMessage = "UpdateStatus: " + responseMessage;
        String prettyResponseMessage = responseMessage.c_str();
        RefPtr<Uint8Array> temporaryMessage = Uint8Array::create(prettyResponseMessage.characters8(), prettyResponseMessage.length());
        nextMessage = temporaryMessage;
        errorCode = WebKitMediaKeyError::MEDIA_KEYERR_CLIENT;
        return false;
    }

    return true;
}

} // namespace WebCore

#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA) && USE(OCDM)
