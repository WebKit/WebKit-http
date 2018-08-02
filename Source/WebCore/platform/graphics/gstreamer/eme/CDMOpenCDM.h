/* GStreamer OpenCDM decryptor
 *
 * Copyright (C) 2016-2017 TATA ELXSI
 * Copyright (C) 2016-2017 Metrological
 * Copyright (C) 2016-2017 Igalia S.L
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

#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)

#include "CDM.h"
#include "CDMFactory.h"
#include "CDMInstance.h"
#include "GStreamerEMEUtilities.h"
#include "MediaKeyStatus.h"
#include <open_cdm.h>
#include <wtf/HashMap.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

class CDMFactoryOpenCDM : public CDMFactory {
private:
    CDMFactoryOpenCDM() = default;
    CDMFactoryOpenCDM(const CDMFactoryOpenCDM&) = delete;
    CDMFactoryOpenCDM& operator=(const CDMFactoryOpenCDM&) = delete;

public:
    static CDMFactoryOpenCDM& singleton();

    virtual ~CDMFactoryOpenCDM() = default;

    virtual std::unique_ptr<CDMPrivate> createCDM(const String&) final;
    virtual bool supportsKeySystem(const String&) final;

private:
    media::OpenCdm m_openCDM;
};

class CDMInstanceOpenCDM final : public CDMInstance {
private:
    CDMInstanceOpenCDM() = delete;
    CDMInstanceOpenCDM(const CDMInstanceOpenCDM&) = delete;
    CDMInstanceOpenCDM& operator=(const CDMInstanceOpenCDM&) = delete;

    class Session;

public:
    CDMInstanceOpenCDM(media::OpenCdm&, const String&);
    virtual ~CDMInstanceOpenCDM() = default;

    // Metadata getters, just for some DRM characteristics.
    const String& keySystem() const final { return m_keySystem; }
    ImplementationType implementationType() const final { return ImplementationType::OpenCDM; }
    SuccessValue initializeWithConfiguration(const MediaKeySystemConfiguration&) final { return Succeeded; }
    SuccessValue setDistinctiveIdentifiersAllowed(bool) final { return Succeeded; }
    SuccessValue setPersistentStateAllowed(bool) final { return Succeeded; }

    // Operations on the DRM system.
    SuccessValue setServerCertificate(Ref<SharedBuffer>&&) final;

    // Request License will automagically create a Session. The session is later on referred to with its session id.
    void requestLicense(LicenseType, const AtomicString&, Ref<SharedBuffer>&&, LicenseCallback) final;

    // Operations on the DRM system -> Session.
    void updateLicense(const String&, LicenseType, const SharedBuffer&, LicenseUpdateCallback) final;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback) final;
    void closeSession(const String&, CloseSessionCallback) final;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback) final;
    void storeRecordOfKeyUsage(const String&) final { }

    // FIXME: For now, the init data is the only way to find a proper session id.
    String sessionIdByInitData(const InitData&) const;
    bool isSessionIdUsable(const String&) const;

private:
    bool addSession(const String& sessionId, Session* session);
    bool removeSession(const String& sessionId);
    RefPtr<Session> lookupSession(const String& sessionId) const;

    String m_keySystem;
    const char* m_mimeType;
    media::OpenCdm m_openCDM;
    // Protects against concurrent access to m_sessionsMap. In addition to the main thread
    // the GStreamer decryptor elements running in the streaming threads have a need to
    // lookup values in this map.
    mutable Lock m_sessionMapMutex;
    HashMap<String, RefPtr<Session>> m_sessionsMap;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceOpenCDM, WebCore::CDMInstance::ImplementationType::OpenCDM);

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
