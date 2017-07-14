#pragma once

#if ENABLE(ENCRYPTED_MEDIA)

#include "CDM.h"
#include "CDMInstance.h"
#include <wtf/HashMap.h>

namespace WebCore {

class CDMFactoryClearKey : public CDMFactory {
public:
    CDMFactoryClearKey();
    virtual ~CDMFactoryClearKey();

    std::unique_ptr<CDMPrivate> createCDM(CDM&, const String&) override;
    bool supportsKeySystem(const String&) override;
};

class CDMInstanceClearKey : public CDMInstance {
public:
    CDMInstanceClearKey();
    virtual ~CDMInstanceClearKey();

    ImplementationType implementationType() const { return ImplementationType::ClearKey; }

    SuccessValue initializeWithConfiguration(const MediaKeySystemConfiguration&) override;
    SuccessValue setDistinctiveIdentifiersAllowed(bool) override;
    SuccessValue setPersistentStateAllowed(bool) override;
    SuccessValue setServerCertificate(Ref<SharedBuffer>&&) override;

    void requestLicense(LicenseType, const AtomicString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback) override;
    void updateLicense(const String&, LicenseType, const SharedBuffer&, LicenseUpdateCallback) override;
    void loadSession(LicenseType, const String&, const String&, LoadSessionCallback) override;
    void closeSession(const String&, CloseSessionCallback) override;
    void removeSessionData(const String&, LicenseType, RemoveSessionDataCallback) override;
    void storeRecordOfKeyUsage(const String&) override;

    void gatherAvailableKeys(AvailableKeysCallback) override;

    struct Key {
        Key() = default;
        Key(Key&&) = default;
        Key& operator=(Key&&) = default;
        String keyID;
        KeyStatus status;
        RefPtr<SharedBuffer> keyIDData;
        RefPtr<SharedBuffer> keyValueData;
    };

    const HashMap<String, Vector<Key>>& keys() const { return m_keys; }

private:
    HashMap<String, Vector<Key>> m_keys;
};

} // namespace WebCore


SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceClearKey, WebCore::CDMInstance::ImplementationType::ClearKey);

#endif // ENABLE(ENCRYPTED_MEDIA)
