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


#ifndef CDM_PRIVATE_ENC_KEY_WPE_H_
#define CDM_PRIVATE_ENC_KEY_WPE_H_
#include "config.h"
#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(OCDM)
#include "CDMPrivate.h"
#include "CDMSession.h"
#include <open_cdm.h>

#include <runtime/JSCInlines.h>
#include <runtime/TypedArrayInlines.h>
#include <runtime/Uint8Array.h>

namespace WebCore {

class CDM;
class CDMSession;
class CDMPrivateEncKey : public RefCounted<CDMPrivateEncKey> {
public:
    explicit CDMPrivateEncKey(CDM* cdm)
         :m_cdm(cdm)
    { }
    explicit CDMPrivateEncKey() {};

    // CDMFactory support:
    static bool supportsKeySystem(const String&);
    static bool supportsKeySystemAndMimeType(const String& keySystem, const String& mimeType);
    static std::unique_ptr<CDMSession> createSession(CDMSessionClient*);
    ~CDMPrivateEncKey(){};
    static OpenCdm* getOpenCdmInstance();

private:
    static String m_openCdmKeySystem;
    static unique_ptr<OpenCdm> m_openCdm;
protected:
    CDM* m_cdm;
};

}//WebCore
#endif //ENABLE(ENCRYPTED_MEDIA_V2) && USE(OCDM)

#endif //CDM_PRIVATE_ENC_KEY_H_
