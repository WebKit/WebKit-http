#pragma once

#include "CDMInstance.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)

#include "PlayreadySession.h"

namespace WebCore {

class CDMInstancePlayReady : public CDMInstance {
public:
    CDMInstancePlayReady();
    virtual ~CDMInstancePlayReady();

    ImplementationType implementationType() const final { return  ImplementationType::PlayReady; }

    SuccessValue initializeWithConfiguration(const MediaKeySystemConfiguration&) override;
    SuccessValue setDistinctiveIdentifiersAllowed(bool) override;
    SuccessValue setPersistentStateAllowed(bool) override;
    SuccessValue setServerCertificate(Ref<SharedBuffer>&&) override;

    void requestLicense(LicenseType, const AtomicString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback) override;
    void updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback) override;
    void loadSession(LicenseType, const String& sessionId, const String& origin, LoadSessionCallback) override;
    void closeSession(const String& sessionId, CloseSessionCallback) override;
    void removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback) override;
    void storeRecordOfKeyUsage(const String& sessionId) override;
    void gatherAvailableKeys(AvailableKeysCallback) override;

    PlayreadySession& prSession() const { return *m_prSession; }

private:
    std::unique_ptr<PlayreadySession> m_prSession;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)
