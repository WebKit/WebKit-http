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


#include "CDMPrivateEncKeyWPE.h"
#if ENABLE(ENCRYPTED_MEDIA_V2)

#include "CDM.h"
#include "CDMSession.h"
#include "MediaKeyError.h"
#include "CDMSessionEncKeyWPE.h"

namespace WebCore {

String CDMPrivateEncKey::m_openCdmKeySystem("\0");
unique_ptr<OpenCdm> CDMPrivateEncKey::m_openCdm(nullptr);

bool CDMPrivateEncKey::supportsKeySystem(const String& keySystem)
{
    m_openCdmKeySystem = keySystem;
    printf ("This is file %s --function (%s)--%d \n: key system  = %s\n",__FILE__,__func__, __LINE__,keySystem.utf8().data());
    return CDMPrivateEncKey::getOpenCdmInstance()->IsTypeSupported(keySystem.utf8().data(),"");
}

bool CDMPrivateEncKey::supportsKeySystemAndMimeType(const String& keySystem, const String& mimeType)
{
    if (!supportsKeySystem(keySystem))
        return false;

    printf ("This is file %s --function (%s)--%d \n",__FILE__,__func__, __LINE__);
    return CDMPrivateEncKey::getOpenCdmInstance()->IsTypeSupported(keySystem.utf8().data(),
                                                                   mimeType.utf8().data());
}

bool CDMPrivateEncKey::supportsMIMEType(const String& mimeType)
{
    printf ("This is file %s --function (%s)--%d \n: key system  = %s ,mimeType = %s \n",__FILE__,__func__, __LINE__, m_cdm->keySystem().utf8().data(), mimeType.utf8().data());
    return CDMPrivateEncKey::getOpenCdmInstance()->IsTypeSupported(m_cdm->keySystem().utf8().data(), mimeType.utf8().data());
}

std::unique_ptr<CDMSession> CDMPrivateEncKey::createSession(CDMSessionClient* client)
{
    CDMPrivateEncKey::getOpenCdmInstance()->SelectKeySystem(CDMPrivateEncKey::m_openCdmKeySystem.utf8().data());
    return std::make_unique<CDMSessionEncKey>(client,  CDMPrivateEncKey::getOpenCdmInstance());
}
OpenCdm* CDMPrivateEncKey::getOpenCdmInstance()
{
    if (NULL == CDMPrivateEncKey::m_openCdm)
        CDMPrivateEncKey::m_openCdm = std::make_unique<OpenCdm>();
    return CDMPrivateEncKey::m_openCdm.get();
}

}
#endif // ENABLE(ENCRYPTED_MEDIA_V2)
