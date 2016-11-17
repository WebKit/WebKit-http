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
    int i;
    unsigned char url[100] = "\0";
    unsigned char initial_value[] = {"00000042"
    "70737368"
    "00000000"
    "edef8ba979d64acea3c827dcd51d21ed"
    "00000022"
    "08011a0d7769646576696e655f746573"
    "74220f73747265616d696e675f636c69"
    "7031" }; //TODO remove this and use initData param

    string sessionId; //TODO check sessionId setting sequence and update

    m_openCdmSession->CreateSession(mimeType.utf8().data(), initial_value,
                                    strlen((const char*)initial_value), sessionId);
    if (!sessionId.size()) {
        printf("SessionId is empty \n");
        return nullptr;
    }
    m_sessionId = String::fromUTF8(sessionId.c_str());

    printf ("SessionID = %s\n", sessionId.c_str());
    printf("m_sessionID = %s\n", m_sessionId.utf8().data());
    cout << endl << "ses_id:CDMSessionEncKey:generateKeyRequest: "<< sessionId << endl;

    ret = m_openCdmSession->GetKeyMessage(m_message,
                                   &m_msgLength, url, &m_destUrlLength);

    printf("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    if ( (ret != 0) || (m_msgLength == 0) || (m_destUrlLength == 0) ) {
        printf("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
        errorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
        return nullptr;
    } else {


        destUrl = String::fromUTF8(url);
        return (Uint8Array::create((unsigned char*)m_message.c_str(), m_msgLength));
    }
}

void CDMSessionEncKey::releaseKeys()
{
    printf("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    // no-op
}

bool CDMSessionEncKey::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t& sysytemCode)
{
    printf("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    UNUSED_PARAM(nextMessage);
    UNUSED_PARAM(errorCode);
    UNUSED_PARAM(sysytemCode);

    //TODO : remove debug prints
    char lic[3096];
    String rawKeysString = String::fromUTF8(key->data(), key->length());
    printf("**********LIC UPDTE ***************************\n");
    memcpy(lic,key->data(), key->length());
    printf("**********LIC UPDTE ***************************\n KEY LENGTH = %d ,KEYBYTELENGTH = %d",key->length(),key->byteLength());
    int i;
    printf("**********LIC FROM WEBKIT***************************\n");
    for(i = 0; i< key->length();i++ )
        printf(" %02X",lic[i]);
    printf("\n**********LIC FROM WEBKIT***************************\n");
    std::cout << key->data() << "\n";

    m_openCdmSession->Update(key->data(), key->length());

    return true;
}

CDMSessionEncKey::~CDMSessionEncKey()
{
    printf ("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
}
}

#endif // ENABLE(ENCRYPTED_MEDIA_V2) && USE(OCDM)
