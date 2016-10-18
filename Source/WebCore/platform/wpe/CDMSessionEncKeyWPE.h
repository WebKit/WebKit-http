
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


#ifndef CDM_SESSION_ENC_KEY_H_
#define CDM_SESSION_ENC_KEY_H_

#include "config.h"
#if ENABLE(ENCRYPTED_MEDIA_V2)
#include "CDMPrivate.h"
#include "CDMPrivateEncKeyWPE.h"
#include "CDMSession.h"

namespace WebCore {

class CDM;
class CDMSession;

class CDMSessionEncKey : public CDMSession {
public:
    CDMPrivateEncKey* m_cdmEncryptedKey;
    CDMSessionEncKey(CDMSessionClient*, OpenCdm* );
    ~CDMSessionEncKey();

    void setClient(CDMSessionClient* client) override { m_client = client; }
    const String& sessionId() const override { return m_sessionId; }
    void releaseKeys() override;
    RefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData,
                                          String& destinationURL, unsigned short& errorCode,
                                          uint32_t& systemCode) override;
    bool update(Uint8Array*, RefPtr<Uint8Array>& nextMessage,
                unsigned short& errorCode, uint32_t& systemCode) override;

private:
    CDMSessionClient* m_client;
    OpenCdm* m_openCdmSession;
    String m_sessionId;
    Uint8Array* m_message;
    int m_msgLength;
    int m_destUrlLength;

};

}//WebCore
#endif // ENABLE(ENCRYPTED_MEDIA)

#endif //DM_SESSION_ENC_KEY_H_
