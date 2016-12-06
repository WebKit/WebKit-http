/*
 * Copyright (C) 2016 TATA ELXSI
 * Copyright (C) 2016 Metrological
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


#include "CDMSessionEncKeyWPE.h"

#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(OCDM)
#include "CDM.h"
#include "CDMSession.h"
#include "MediaKeyError.h"


namespace WebCore {

CDMSessionEncKey::CDMSessionEncKey(CDMSessionClient* client ,OpenCdm* openCdm)
   : m_client(client),
     m_openCdmSession(openCdm),
     m_msgLength(0),
     m_destUrlLength(0)
{
}

RefPtr<Uint8Array> CDMSessionEncKey::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destUrl, unsigned short& errorCode, uint32_t& sysCode)
{
    UNUSED_PARAM(sysCode);

    int ret = 0;
    string sessionId;

    unsigned char tmpUrl[100] = "\0";
    unsigned char initDataValue[initData->length()] = "\0";

    memcpy(initDataValue, initData->data(), initData->length());

    m_openCdmSession->CreateSession(mimeType.utf8().data(), initDataValue,
                                   initData->length(), sessionId);
    if (!sessionId.size()) {
        printf("SessionId is empty \n");
        return nullptr;
    }
    m_sessionId = String::fromUTF8(sessionId.c_str());

    ret = m_openCdmSession->GetKeyMessage(m_message,
                                   &m_msgLength, tmpUrl, &m_destUrlLength);

    if ( (ret != 0) || (m_msgLength == 0) || (m_destUrlLength == 0) ) {
        errorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
        return nullptr;
    } else {
        destUrl = String::fromUTF8(tmpUrl);
        return (Uint8Array::create((unsigned char*)m_message.c_str(), m_msgLength));
   }
}

void CDMSessionEncKey::releaseKeys()
{
    // no-op
}

bool CDMSessionEncKey::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t& sysytemCode)
{
    UNUSED_PARAM(nextMessage);
    UNUSED_PARAM(errorCode);
    UNUSED_PARAM(sysytemCode);

    int ret = 0;
    std::string responseMsg;

    {//TODO: remove after testing
       char lic[3096];
       memcpy(lic, key->data(), key->length());

       int i;
       for (i = 0; i < key->length(); i++ )
          printf(" %02X",lic[i]);

       std::cout << key->data() << "\n";
    }

    ret = m_openCdmSession->Update(key->data(), key->length(), responseMsg);
    if (ret) {
       string rspMsg = "UpdateStatus:" + responseMsg;
       RefPtr<Uint8Array> tmpMsg = Uint8Array::create((unsigned char*)rspMsg.c_str(), rspMsg.length());
       nextMessage = tmpMsg;
       errorCode = MediaKeyError::MEDIA_KEYERR_CLIENT;
       return false;
    }

    return true;
}

CDMSessionEncKey::~CDMSessionEncKey()
{
}
}

#endif // ENABLE(ENCRYPTED_MEDIA_V2) && USE(OCDM)
