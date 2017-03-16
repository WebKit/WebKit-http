
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

#include "CDMPrivateOpenCDM.h"
#include "LegacyCDMSession.h"

namespace WebCore {

class CDMSession;
class MediaPlayerPrivateGStreamerBase;

class CDMSessionOpenCDM : public CDMSession {
private:
    enum KeyState {
        // Has been initialized.
        KeyInit = 0,
        // Has a key message pending to be processed.
        KeyPending = 1,
        // Has a usable key.
        KeyReady = 2,
        // Has an error.
        KeyError = 3,
        // Has been closed.
        KeyClosed = 4
    };

public:
    CDMSessionOpenCDM(CDMSessionClient*, media::OpenCdm*, MediaPlayerPrivateGStreamerBase*, String);
    virtual ~CDMSessionOpenCDM() = default;

    void setClient(CDMSessionClient*) override { }
    const String& sessionId() const override { return m_sessionId; }
    void releaseKeys() override;
    RefPtr<Uint8Array> generateKeyRequest(const String&, Uint8Array*,
        String&, unsigned short&, uint32_t&) override;
    bool update(Uint8Array*, RefPtr<Uint8Array>&,
        unsigned short&, uint32_t&) override;
    bool keyRequested() const override { return m_eKeyState == KeyPending; }
    bool ready() const override { return m_eKeyState == KeyReady; }

private:
    media::OpenCdm* m_openCdmSession;
    String m_sessionId;
    MediaPlayerPrivateGStreamerBase* m_playerPrivate;
    String m_openCdmKeySystem;
    KeyState m_eKeyState;
};

} // namespace WebCore

#endif // (ENABLE(LEGACY_ENCRYPTED_MEDIA) || ENABLE(LEGACY_ENCRYPTED_MEDIA_V1)) && USE(OCDM)
