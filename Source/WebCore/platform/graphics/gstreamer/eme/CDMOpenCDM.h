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
#include "MediaKeyStatus.h"
#include <wtf/HashMap.h>

namespace media {
class OpenCdm;
};

namespace WebCore {

class CDMFactoryOpenCDM : public CDMFactory {
public:
    static CDMFactoryOpenCDM& singleton();

    virtual ~CDMFactoryOpenCDM();

    std::unique_ptr<CDMPrivate> createCDM(const String&) override;
    bool supportsKeySystem(const String&) override;

private:
    CDMFactoryOpenCDM();
};

class CDMInstanceOpenCDM final : public CDMInstance {
public:
    CDMInstanceOpenCDM(media::OpenCdm*, const String&);
    virtual ~CDMInstanceOpenCDM();

    ImplementationType implementationType() const { return  ImplementationType::OpenCDM; }
    SuccessValue initializeWithConfiguration(const MediaKeySystemConfiguration&) override;
    SuccessValue setDistinctiveIdentifiersAllowed(bool) override;
    SuccessValue setPersistentStateAllowed(bool) override;
    SuccessValue setServerCertificate(Ref<SharedBuffer>&&) override;

    void requestLicense(LicenseType, const AtomicString&, Ref<SharedBuffer>&&, LicenseCallback) override;
    void updateLicense(const String&, LicenseType, const SharedBuffer&, LicenseUpdateCallback) override;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback) override;
    void closeSession(const String&, CloseSessionCallback) override;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback) override;
    void storeRecordOfKeyUsage(const String&) override;

    const String& keySystem() const override { return m_keySystem; }

    // FIXME: Session handling needs a lot of love here.
    String getCurrentSessionId() const;

private:
    MediaKeyStatus getKeyStatus(std::string &);
    SessionLoadFailure getSessionLoadStatus(std::string &);
    size_t checkMessageLength(std::string &, std::string &);

    media::OpenCdm* m_openCdmSession;
    HashMap<String, Ref<SharedBuffer>> sessionIdMap;
    String m_keySystem;
};

} // namespace WebCore

SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceOpenCDM, WebCore::CDMInstance::ImplementationType::OpenCDM);

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
