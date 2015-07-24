/*
 * Copyright (C) 2014-2015 FLUENDO S.A. All rights reserved.
 * Copyright (C) 2014-2015 METROLOGICAL All rights reserved.
 * Copyright (C) 2015 IGALIA S.L All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY FLUENDO S.A. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL FLUENDO S.A. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CDMPRSessionGStreamer_h
#define CDMPRSessionGStreamer_h

#if ENABLE(ENCRYPTED_MEDIA_V2) && USE(GSTREAMER) && USE(DXDRM)

#include "CDMSession.h"
#include <gst/gst.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

class MediaPlayerPrivateGStreamer;

class CDMPRSessionGStreamer : public CDMSession {
private:
    CDMPRSessionGStreamer();
    CDMPRSessionGStreamer(const CDMPRSessionGStreamer&);

private:
    enum SessionState {
        PHASE_INITIAL,
        PHASE_ACKNOWLEDGE,
        PHASE_PROVISIONED
    };

public:
    CDMPRSessionGStreamer(MediaPlayerPrivateGStreamer* parent);
    virtual ~CDMPRSessionGStreamer() override;

    virtual CDMSessionType type() override;
    virtual void setClient(CDMSessionClient* client) override;
    virtual const String& sessionId() const override;
    virtual PassRefPtr<Uint8Array> generateKeyRequest(const String& mimeType, Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode) override;
    virtual void releaseKeys() override;
    virtual bool update(Uint8Array*, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode) override;
    virtual RefPtr<ArrayBuffer> cachedKeyForKeyID(const String&) const;

    int decrypt(GstMapInfo& map, GstMapInfo& boxMap, const unsigned sampleIndex, const unsigned trackId);

private:
    MediaPlayerPrivateGStreamer* m_player;
    CDMSessionClient* m_client;
    String m_sessionId;
    void* m_DxDrmStream;
    RefPtr<ArrayBuffer> m_key;
    SessionState m_state;
};

}

#endif

#endif // CDMPRSessionGStreamer_h
