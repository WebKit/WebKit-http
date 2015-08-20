/* Discretix Session management
 *
 * Copyright (C) 2015 Igalia S.L
 * Copyright (C) 2015 Metrological
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#ifndef DiscretixSession_h
#define DiscretixSession_h

#if USE(DXDRM)

#include <dxdrm/DxDrmClient.h>
#include <runtime/Uint8Array.h>
#include <wtf/Forward.h>

namespace WebCore {

class DiscretixSession {

private:
    enum SessionState {
        PHASE_INITIAL,
        PHASE_ACKNOWLEDGE,
        PHASE_PROVISIONED
    };

public:
    DiscretixSession();
    ~DiscretixSession();

    PassRefPtr<Uint8Array> dxdrmGenerateKeyRequest(Uint8Array* initData, String& destinationURL, unsigned short& errorCode, unsigned long& systemCode);
    bool dxdrmProcessKey(Uint8Array* key, RefPtr<Uint8Array>& nextMessage, unsigned short& errorCode, unsigned long& systemCode);

    bool keyRequested() const { return m_keyRequested; }
    bool ready() const { return m_ready; }
    int decrypt(void* data, uint32_t dataLength, const void* encryptionBoxData, uint32_t encryptionBoxLength, uint32_t sampleIndex, uint32_t trackId);

protected:
    RefPtr<ArrayBuffer> m_key;

private:
    void* m_DxDrmStream;
    SessionState m_state;
    EDxDrmStatus m_status;
    bool m_ready;
    bool m_keyRequested;
};

}
#endif

#endif
