/*
 * Copyright (C) 2016 Metrological Group B.V.
 * Copyright (C) 2016 Igalia S.L.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials provided
 *    with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "CDMClearKey.h"

#if ENABLE(ENCRYPTED_MEDIA)

#include "CDMKeySystemConfiguration.h"
#include "CDMRestrictions.h"
#include "CDMSessionType.h"
#include "SharedBuffer.h"
#include "inspector/InspectorValues.h"
#include <wtf/UUID.h>
#include <wtf/text/Base64.h>

using namespace Inspector;

namespace WebCore {

static struct {
    HashMap<String, Vector<CDMInstanceClearKey::Key>> keys;
    HashSet<String> persistentSessions;
} s_clearKey;

CDMFactoryClearKey& CDMFactoryClearKey::singleton()
{
    static CDMFactoryClearKey s_factory;
    return s_factory;
}

CDMFactoryClearKey::CDMFactoryClearKey() = default;
CDMFactoryClearKey::~CDMFactoryClearKey() = default;

std::unique_ptr<CDMPrivate> CDMFactoryClearKey::createCDM(const String& keySystem)
{
#ifdef NDEBUG
    UNUSED_PARAM(keySystem);
#else
    ASSERT(supportsKeySystem(keySystem));
#endif
    return std::unique_ptr<CDMPrivate>(new CDMPrivateClearKey);
}

bool CDMFactoryClearKey::supportsKeySystem(const String& keySystem)
{
    // `org.w3.clearkey` is the only supported key system.
    return equalLettersIgnoringASCIICase(keySystem, "org.w3.clearkey");
}

CDMPrivateClearKey::CDMPrivateClearKey() = default;
CDMPrivateClearKey::~CDMPrivateClearKey() = default;

bool CDMPrivateClearKey::supportsInitDataType(const AtomicString& initDataType) const
{
    // `keyids` is the only supported init data type.
    return equalLettersIgnoringASCIICase(initDataType, "keyids");
}

bool CDMPrivateClearKey::supportsConfiguration(const CDMKeySystemConfiguration& configuration) const
{
    // Reject any configuration that marks distinctive identifier or persistent state as required.
    if (configuration.distinctiveIdentifier == CDMRequirement::Required
        || configuration.persistentState == CDMRequirement::Required)
        return false;
    return true;
}

bool CDMPrivateClearKey::supportsConfigurationWithRestrictions(const CDMKeySystemConfiguration& configuration, const CDMRestrictions& restrictions) const
{
    // Reject any configuration that marks distincitive identifier as required, or that marks
    // distinctive identifier as optional even when restrictions mark it as denied.
    if ((configuration.distinctiveIdentifier == CDMRequirement::Optional && restrictions.distinctiveIdentifierDenied)
        || configuration.distinctiveIdentifier == CDMRequirement::Required)
        return false;

    // Ditto for persistent state.
    if ((configuration.persistentState == CDMRequirement::Optional && restrictions.persistentStateDenied)
        || configuration.persistentState == CDMRequirement::Required)
        return false;

    return true;
}

bool CDMPrivateClearKey::supportsSessionTypeWithConfiguration(CDMSessionType& sessionType, const CDMKeySystemConfiguration& configuration) const
{
    // Only support the temporary session type.
    if (sessionType != CDMSessionType::Temporary)
        return false;
    return supportsConfiguration(configuration);
}

bool CDMPrivateClearKey::supportsRobustness(const String& robustness) const
{
    // Only empty `robustness` string is supported.
    return robustness.isEmpty();
}

CDMRequirement CDMPrivateClearKey::distinctiveIdentifiersRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions& restrictions) const
{
    // Distinctive identifier is not allowed if it's been denied, otherwise it's optional.
    if (restrictions.distinctiveIdentifierDenied)
        return CDMRequirement::NotAllowed;
    return CDMRequirement::Optional;
}

CDMRequirement CDMPrivateClearKey::persistentStateRequirement(const CDMKeySystemConfiguration&, const CDMRestrictions& restrictions) const
{
    // Persistent state is not allowed if it's been denied, otherwise it's optional.
    if (restrictions.persistentStateDenied)
        return CDMRequirement::NotAllowed;
    return CDMRequirement::Optional;
}

bool CDMPrivateClearKey::distinctiveIdentifiersAreUniquePerOriginAndClearable(const CDMKeySystemConfiguration&) const
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

CDMInstanceClearKey::CDMInstanceClearKey() = default;
CDMInstanceClearKey::~CDMInstanceClearKey() = default;

CDMInstance::SuccessValue CDMInstanceClearKey::initializeWithConfiguration(const CDMKeySystemConfiguration&)
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

    auto& keyVector = s_clearKey.keys.ensure(sessionId, [] { return Vector<Key>{ }; }).iterator->value;

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
    for (auto& it : s_clearKey.keys) {
        for (auto& key : it.value) {
            if (key.status == KeyStatus::Usable)
                vector.append({ key.keyIDData->copy(), key.keyValueData->copy() });
        }
    }
    if (!vector.isEmpty())
        callback(WTFMove(vector));
}

const String& CDMInstanceClearKey::keySystem() const
{
    static const String s_keySystem("org.w3.clearkey");

    return s_keySystem;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
