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

#pragma once

#if (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OCDM)

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include "LegacyCDMPrivate.h"
#else
#include "LegacyCDMSession.h"
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
#include <open_cdm.h>

namespace WebCore {

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
class CDM;
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
class MediaPlayerPrivateGStreamerBase;

class CDMPrivateOpenCDM : public RefCounted<CDMPrivateOpenCDM> {
public:
#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
    explicit CDMPrivateOpenCDM(CDM*);
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
    explicit CDMPrivateOpenCDM() = default;

    static bool supportsKeySystem(const String&);
    static bool supportsKeySystemAndMimeType(const String&, const String&);
    static std::unique_ptr<CDMSession> createSession(CDMSessionClient*, MediaPlayerPrivateGStreamerBase*);
    
    static media::OpenCdm* getOpenCdmInstance();

private:
    static String s_openCdmKeySystem;
    static std::unique_ptr<media::OpenCdm> s_openCdm;

#if ENABLE(LEGACY_ENCRYPTED_MEDIA)
protected:
    CDM* m_cdm;
#endif // ENABLE(LEGACY_ENCRYPTED_MEDIA)
};

} // namespace WebCore

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OCDM)
