#include "config.h"
#include "CDMClearKey.h"

#if ENABLE(ENCRYPTED_MEDIA)

#include "CDMPrivate.h"
#include "inspector/InspectorValues.h"
#include <wtf/UUID.h>
#include <wtf/text/Base64.h>

using namespace Inspector;

namespace WebCore {

class CDMPrivateClearKey : public CDMPrivate {
public:
    CDMPrivateClearKey();
    virtual ~CDMPrivateClearKey();

    bool supportsInitDataType(const AtomicString&) const override;
    bool supportsConfiguration(const MediaKeySystemConfiguration&) const override;
    bool supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const override;
    bool supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration&) const override;
    bool supportsRobustness(const String&) const override;
    MediaKeysRequirement distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const override;
    MediaKeysRequirement persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const override;
    bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const override;
    RefPtr<CDMInstance> createInstance() override;
    void loadAndInitialize() override;
    bool supportsServerCertificates() const override;
    bool supportsSessions() const override;
    bool supportsInitData(const AtomicString&, const SharedBuffer&) const override;
    RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer&) const override;
    std::optional<String> sanitizeSessionId(const String&) const override;
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

private:
    struct Key {
        Key() = default;
        Key(Key&&) = default;
        Key& operator=(Key&&) = default;
        String keyID;
        KeyStatus status;
        RefPtr<SharedBuffer> keyIDData;
        RefPtr<SharedBuffer> keyValueData;
    };

    // HashMap<String, Key> m_keys;
    HashMap<String, Vector<Key>> m_what;
};

CDMPrivateClearKey::CDMPrivateClearKey() = default;

CDMPrivateClearKey::~CDMPrivateClearKey() = default;

bool CDMPrivateClearKey::supportsInitDataType(const AtomicString& initDataType) const
{
    return equalLettersIgnoringASCIICase(initDataType, "keyids");
}

bool CDMPrivateClearKey::supportsConfiguration(const MediaKeySystemConfiguration&) const
{
    return true;
}

bool CDMPrivateClearKey::supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    return true;
}

bool CDMPrivateClearKey::supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration&) const
{
    return true;
}

bool CDMPrivateClearKey::supportsRobustness(const String&) const
{
    return false;
}

MediaKeysRequirement CDMPrivateClearKey::distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    return MediaKeysRequirement::Optional;
}

MediaKeysRequirement CDMPrivateClearKey::persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    return MediaKeysRequirement::Optional;
}

bool CDMPrivateClearKey::distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const
{
    return false;
}

RefPtr<CDMInstance> CDMPrivateClearKey::createInstance()
{
    return adoptRef(new CDMInstanceClearKey);
}

void CDMPrivateClearKey::loadAndInitialize()
{
}

bool CDMPrivateClearKey::supportsServerCertificates() const
{
    return false;
}

bool CDMPrivateClearKey::supportsSessions() const
{
    return true;
}

bool CDMPrivateClearKey::supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const
{
    return equalLettersIgnoringASCIICase(initDataType, "keyids");
}

RefPtr<SharedBuffer> CDMPrivateClearKey::sanitizeResponse(const SharedBuffer& response) const
{
    return response.copy();
}

std::optional<String> CDMPrivateClearKey::sanitizeSessionId(const String& sessionId) const
{
    return sessionId;
}

CDMFactoryClearKey::CDMFactoryClearKey() = default;

CDMFactoryClearKey::~CDMFactoryClearKey() = default;

std::unique_ptr<CDMPrivate> CDMFactoryClearKey::createCDM(CDM&, const String&)
{
    return std::unique_ptr<CDMPrivate>(new CDMPrivateClearKey);
}

bool CDMFactoryClearKey::supportsKeySystem(const String& keySystem)
{
    return equalLettersIgnoringASCIICase(keySystem, "org.w3.clearkey");
}

CDMInstanceClearKey::CDMInstanceClearKey() = default;
CDMInstanceClearKey::~CDMInstanceClearKey() = default;

CDMInstance::SuccessValue CDMInstanceClearKey::initializeWithConfiguration(const MediaKeySystemConfiguration&)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceClearKey::setDistinctiveIdentifiersAllowed(bool)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceClearKey::setPersistentStateAllowed(bool)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceClearKey::setServerCertificate(Ref<SharedBuffer>&&)
{
    return Failed;
}

void CDMInstanceClearKey::requestLicense(LicenseType, const AtomicString&, Ref<SharedBuffer>&& initData, LicenseCallback callback)
{
    callback(WTFMove(initData), createCanonicalUUIDString(), false, Succeeded);
}

void CDMInstanceClearKey::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    // https://w3c.github.io/encrypted-media/#dom-mediakeysession-generaterequest
    // W3C Editor's Draft 09 November 2016

    // 6.7.1. If the format of sanitized response is invalid in any way, reject promise with a newly created TypeError.
    // 6.7.2. Process sanitized response, following the stipulation for the first matching condition from the following list:
    //   ↳ If sanitized response contains a license or key(s)
    //     Process sanitized response, following the stipulation for the first matching condition from the following list:
    //     ↳ If sessionType is "temporary" and sanitized response does not specify that session data, including any license, key(s), or similar session data it contains, should be stored
    //       Process sanitized response, not storing any session data.
    //     ↳ If sessionType is "persistent-license" and sanitized response contains a persistable license
    //       Process sanitized response, storing the license/key(s) and related session data contained in sanitized response. Such data must be stored such that only the origin of this object's Document can access it.
    //     ↳ If sessionType is "persistent-usage-record"
    //       ??
    //     ↳ Otherwise
    //       Reject promise with a newly created TypeError.
    //   ↳ If sanitized response contains a record of license destruction acknowledgement and sessionType is "persistent-license"
    //     Run the following steps:
    //       6.7.2.1. Close the key session and clear all stored session data associated with this object, including the sessionId and record of license destruction.
    //       6.7.2.2. Set session closed to true.
    //   ↳ Otherwise
    //     Process sanitized response, not storing any session data.

    String json { response.data(), response.size() };

    auto keysArray =
        [&json] () -> RefPtr<InspectorArray> {
            RefPtr<InspectorValue> value;
            if (!InspectorValue::parseJSON(json, value))
                return nullptr;

            RefPtr<InspectorObject> object;
            if (!value->asObject(object))
                return nullptr;

            RefPtr<InspectorArray> array;
            object->getArray("keys", array);
            return array;
        }();
    if (!keysArray) {
        callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        return;
    }

    Vector<Key> updatedKeys;
    bool validFormat =
        [&updatedKeys, &keysArray] {
            for (auto& value : *keysArray) {
                RefPtr<InspectorObject> keyObject;
                if (!value->asObject(keyObject))
                    return false;

                String keyType;
                if (!keyObject->getString("kty", keyType) || !equalLettersIgnoringASCIICase(keyType, "oct"))
                    return false;

                String keyID, keyValue;
                if (!keyObject->getString("kid", keyID) || !keyObject->getString("k", keyValue))
                    return false;

                Vector<char> keyIDData;
                if (!WTF::base64URLDecode(keyID, { keyIDData }))
                    return false;

                Vector<char> keyValueData;
                if (!WTF::base64URLDecode(keyValue, { keyValueData }))
                    return false;

                updatedKeys.append(Key{ keyID, KeyStatus::Usable, SharedBuffer::create(WTFMove(keyIDData)), SharedBuffer::create(WTFMove(keyValueData)) });
            }

            return true;
        }();
    if (!validFormat) {
        callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        return;
    }

#if 0
    String sessionType;
    if (!object->getString("type", sessionType))
        sessionType = "temporary";
    // FIXME: Check that session type is valid.
#endif

    auto& keyVector = m_what.ensure(sessionId, [] { return Vector<Key>{ }; }).iterator->value;

    bool keysChanged = false;
    for (auto& key : updatedKeys) {
        auto it = std::find_if(keyVector.begin(), keyVector.end(),
            [&key] (const Key& containedKey) { return containedKey.keyID == key.keyID; });
        if (it != keyVector.end()) {
            auto& existingKey = it->keyValueData;
            auto& proposedKey = key.keyValueData;

            if (existingKey->size() != proposedKey->size() || std::memcmp(existingKey->data(), proposedKey->data(), existingKey->size())) {
                *it = WTFMove(key);
                keysChanged = true;
            }
        } else {
            keyVector.append(WTFMove(key));
            keysChanged = true;
        }
    }

    std::optional<KeyStatusVector> changedKeys;
    if (keysChanged) {
        Vector<std::pair<RefPtr<SharedBuffer>, KeyStatus>> keys;
        keys.reserveInitialCapacity(keyVector.size());
        for (auto& it : keyVector)
            keys.uncheckedAppend(std::pair<RefPtr<SharedBuffer>, KeyStatus>{ it.keyIDData, it.status });

        std::sort(keys.begin(), keys.end(),
            [] (const auto& a, const auto& b)
            {
                if (a.first->size() != b.first->size())
                    return a.first->size() < b.first->size();

                return std::memcmp(a.first->data(), b.first->data(), a.first->size()) < 0;
            });

        // Sorting Ref<> objects is hard. Such is life.
        KeyStatusVector keyStatusVector;
        keyStatusVector.reserveInitialCapacity(keys.size());
        for (auto& it : keys)
            keyStatusVector.uncheckedAppend(std::pair<Ref<SharedBuffer>, KeyStatus>{ *it.first, it.second });

        changedKeys = WTFMove(keyStatusVector);
    }

    callback(false, WTFMove(changedKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded);
}

void CDMInstanceClearKey::loadSession(LicenseType, const String&, const String&, LoadSessionCallback)
{
}

void CDMInstanceClearKey::closeSession(const String&, CloseSessionCallback callback)
{
    callback();
}

void CDMInstanceClearKey::removeSessionData(const String&, LicenseType, RemoveSessionDataCallback)
{
}

void CDMInstanceClearKey::storeRecordOfKeyUsage(const String&)
{
}

void CDMInstanceClearKey::gatherAvailableKeys(AvailableKeysCallback callback)
{
    KeyVector vector;
    for (auto& it : m_what) {
        for (auto& key : it.value) {
            if (key.status == KeyStatus::Usable)
                vector.append({ key.keyIDData->copy(), key.keyValueData->copy() });
        }
    }
    if (!vector.isEmpty())
        callback(WTFMove(vector));
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
