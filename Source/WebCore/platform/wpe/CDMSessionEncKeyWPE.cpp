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

#if ENABLE(ENCRYPTED_MEDIA_V2)
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
    printf ("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
}


RefPtr<Uint8Array> CDMSessionEncKey::generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destUrl, unsigned short& errorCode, uint32_t& sysCode)
{
    UNUSED_PARAM(sysCode);

    int ret = 0;
    unsigned char url[100] = "\0";
    unsigned char message[100] = "\0";

    printf ("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    m_openCdmSession->CreateSession(mimeType.utf8().data(),
                                    reinterpret_cast<unsigned char*>(initData),
                                    initData->length()); //TODO Widevine
    ret = m_openCdmSession->GetKeyMessage(message,
                                          &m_msgLength, url, &m_destUrlLength);

    if ( (ret != 0) || (m_msgLength == 0) || (m_destUrlLength == 0) ) {
        errorCode = MediaKeyError::MEDIA_KEYERR_UNKNOWN;
        return nullptr;

    } else {
        destUrl = String::fromUTF8(url);
        return (Uint8Array::create(message,m_msgLength));
    }

}

void CDMSessionEncKey::releaseKeys()
{
    printf("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    // no-op
}

bool CDMSessionEncKey::update(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, uint32_t& sysytemCode)
{
    UNUSED_PARAM(nextMessage);
    UNUSED_PARAM(errorCode);
    UNUSED_PARAM(sysytemCode);

    String rawKeysString = String::fromUTF8(key->data(), key->length());
    m_openCdmSession->Update(key->data(), key->length());//TODO implement for widevine
    return true;
}

CDMSessionEncKey::~CDMSessionEncKey()
{
    printf ("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
}

}

#endif // ENABLE(ENCRYPTED_MEDIA_V2)
