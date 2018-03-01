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

#include "config.h"
#include "CDMOpenCDM.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)

#include "CDMPrivate.h"
#include "GStreamerEMEUtilities.h"
#include "MediaKeyMessageType.h"
#include "MediaKeysRequirement.h"
#include "inspector/InspectorValues.h"

#include <gst/gst.h>
#include <wtf/text/Base64.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

using namespace Inspector;

namespace WebCore {

class CDMPrivateOpenCDM : public CDMPrivate {
private:
    CDMPrivateOpenCDM() = delete;
    CDMPrivateOpenCDM(const CDMPrivateOpenCDM&) = delete;
    CDMPrivateOpenCDM& operator=(const CDMPrivateOpenCDM&) = delete;

public:
    CDMPrivateOpenCDM(const String& keySystem)
        : m_openCDMKeySystem(keySystem)
        , m_openCDM() { }
    virtual ~CDMPrivateOpenCDM() = default;

public:
    bool supportsInitDataType(const AtomicString& initDataType) const final { return equalLettersIgnoringASCIICase(initDataType, "cenc"); }
    bool supportsConfiguration(const MediaKeySystemConfiguration& config) const final;
    bool supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration& config, const MediaKeysRestrictions&) const final { return supportsConfiguration(config); }
    bool supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration& config) const final { return supportsConfiguration(config); }
    bool supportsRobustness(const String&) const final { return false; }
    MediaKeysRequirement distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const final { return MediaKeysRequirement::Optional; }
    MediaKeysRequirement persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const final { return MediaKeysRequirement::Optional; }
    bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const final { return false; }
    RefPtr<CDMInstance> createInstance() final { return adoptRef(new CDMInstanceOpenCDM(m_openCDM, m_openCDMKeySystem)); }
    void loadAndInitialize() final { }
    bool supportsServerCertificates() const final { return true; }
    bool supportsSessions() const final { return true; }
    bool supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const final { return equalLettersIgnoringASCIICase(initDataType, "cenc"); }
    RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer& response) const final { return response.copy(); }
    std::optional<String> sanitizeSessionId(const String& sessionId) const final { return sessionId; }

private:
    String m_openCDMKeySystem;
    media::OpenCdm m_openCDM;
};

bool CDMPrivateOpenCDM::supportsConfiguration(const MediaKeySystemConfiguration& config) const
{
    for (auto& audioCapability : config.audioCapabilities)
        if (!m_openCDM.IsTypeSupported(m_openCDMKeySystem.utf8().data(), audioCapability.contentType.utf8().data()))
            return false;
    for (auto& videoCapability : config.videoCapabilities)
        if (!m_openCDM.IsTypeSupported(m_openCDMKeySystem.utf8().data(), videoCapability.contentType.utf8().data()))
            return false;
    return true;
}

// Class CDMFactory.

CDMFactoryOpenCDM& CDMFactoryOpenCDM::singleton()
{
    static CDMFactoryOpenCDM s_factory;
    return s_factory;
}

std::unique_ptr<CDMPrivate> CDMFactoryOpenCDM::createCDM(const String& keySystem)
{
    return std::unique_ptr<CDMPrivate>(new CDMPrivateOpenCDM(keySystem));
}

bool CDMFactoryOpenCDM::supportsKeySystem(const String& keySystem)
{
    return GStreamerEMEUtilities::isPlayReadyKeySystem(keySystem);
}

// Class CDMInstanceOpenCDM.
// Local helper methods.

static media::OpenCdm::LicenseType openCDMLicenseType(CDMInstance::LicenseType licenseType)
{
    switch (licenseType) {
    case CDMInstance::LicenseType::Temporary:
        return media::OpenCdm::Temporary;
    case CDMInstance::LicenseType::PersistentUsageRecord:
        return media::OpenCdm::PersistentUsageRecord;
    case CDMInstance::LicenseType::PersistentLicense:
        return media::OpenCdm::PersistentLicense;
    default:
        ASSERT_NOT_REACHED();
        return media::OpenCdm::Temporary;
    }
}

static CDMInstance::KeyStatus keyStatusFromOpenCDM(media::OpenCdm::KeyStatus keyStatus)
{
    switch (keyStatus) {
    case media::OpenCdm::KeyStatus::Usable:
        return CDMInstance::KeyStatus::Usable;
    case media::OpenCdm::KeyStatus::Expired:
        return CDMInstance::KeyStatus::Expired;
    case media::OpenCdm::KeyStatus::Released:
        return CDMInstance::KeyStatus::Released;
    case media::OpenCdm::KeyStatus::OutputRestricted:
        return CDMInstance::KeyStatus::OutputRestricted;
    case media::OpenCdm::KeyStatus::OutputDownscaled:
        return CDMInstance::KeyStatus::OutputDownscaled;
    case media::OpenCdm::KeyStatus::StatusPending:
        return CDMInstance::KeyStatus::StatusPending;
    case media::OpenCdm::KeyStatus::InternalError:
        return CDMInstance::KeyStatus::InternalError;
    default:
        ASSERT_NOT_REACHED();
        return CDMInstance::KeyStatus::InternalError;
    }
}

static CDMInstance::SessionLoadFailure sessionLoadFailureFromOpenCDM(std::string& loadStatus)
{
    if (loadStatus != "None")
        return CDMInstance::SessionLoadFailure::None;
    if (loadStatus == "SessionNotFound")
        return CDMInstance::SessionLoadFailure::NoSessionData;
    if (loadStatus == "MismatchedSessionType")
        return CDMInstance::SessionLoadFailure::MismatchedSessionType;
    if (loadStatus == "QuotaExceeded")
        return CDMInstance::SessionLoadFailure::QuotaExceeded;
    return CDMInstance::SessionLoadFailure::Other;
}

static MediaKeyStatus mediaKeyStatusFromOpenCDM(std::string& keyStatus)
{
    if (keyStatus == "KeyUsable")
        return MediaKeyStatus::Usable;
    if (keyStatus == "KeyExpired")
        return MediaKeyStatus::Expired;
    if (keyStatus == "KeyReleased")
        return MediaKeyStatus::Released;
    if (keyStatus == "KeyOutputRestricted")
        return MediaKeyStatus::OutputRestricted;
    if (keyStatus == "KeyOutputDownscaled")
        return MediaKeyStatus::OutputDownscaled;
    if (keyStatus == "KeyStatusPending")
        return MediaKeyStatus::StatusPending;
    return MediaKeyStatus::InternalError;
}

static size_t messageLength(std::string& message, std::string& request)
{
    size_t length = 0;
    std::string delimiter = ":Type:";
    std::string requestType = message.substr(0, message.find(delimiter));
    // FIXME: What is with this weirdness?
    if (requestType.size() && requestType.size() == (request.size() + 1))
        length = requestType.size() + delimiter.size();
    else
        length = request.length();
    GST_TRACE("delimiter.size = %u, delimiter = %s, requestType.size = %u, requestType = %s, request.size = %u, request = %s", delimiter.size(), delimiter.c_str(), requestType.size(), requestType.c_str(), request.size(), request.c_str());
    return length;
}

// ---------------------------------------------------------------------------------------------------------------------------
// Class CDMInstanceOpenCDM
// ---------------------------------------------------------------------------------------------------------------------------
CDMInstanceOpenCDM::Session::Session(const media::OpenCdm& source, Ref<WebCore::SharedBuffer>&& initData)
    : m_session(source)
    , m_initData(WTFMove(initData))
{
    uint8_t temporaryURL[1024];
    uint16_t temporaryURLLength = sizeof(temporaryURL);

    m_session.GetKeyMessage(m_message, temporaryURL, temporaryURLLength);

    if (m_message.empty() || !temporaryURLLength) {
        m_message.clear();
        return;
    }

    std::string delimiter(":Type:");
    std::string requestType(m_message.substr(0, m_message.find(delimiter)));

    m_url = std::string(reinterpret_cast<const char*>(temporaryURL), temporaryURLLength);

    if (requestType.size() && requestType.size() != m_message.size())
        m_message.erase(0, m_message.find(delimiter) + delimiter.length());

    m_needsIndividualization = requestType.size() == 1 && static_cast<WebCore::MediaKeyMessageType>(std::stoi(requestType)) == CDMInstance::MessageType::IndividualizationRequest;
}

CDMInstanceOpenCDM::CDMInstanceOpenCDM(media::OpenCdm& system, const String& keySystem)
    : m_openCDM(system)
    , m_mimeType("video/x-h264")
    , m_keySystem(keySystem)
{
    m_openCDM.SelectKeySystem(keySystem.utf8().data());

    if (GStreamerEMEUtilities::isWidevineKeySystem(keySystem))
        m_mimeType = "video/mp4";
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    return m_openCDM.SetServerCertificate(reinterpret_cast<unsigned char*>(const_cast<char*>(certificate->data())), certificate->size())
        ? WebCore::CDMInstance::SuccessValue::Succeeded : WebCore::CDMInstance::SuccessValue::Failed;
}

void CDMInstanceOpenCDM::requestLicense(LicenseType licenseType, const AtomicString&, Ref<SharedBuffer>&& initData, LicenseCallback callback)
{
    std::string sessionId;

    GST_TRACE("Going to request a new session id, init data size %u", initData->size());
    GST_MEMDUMP("init data", reinterpret_cast<const uint8_t*>(initData->data()), initData->size());

    media::OpenCdm openCDM(m_openCDM);

    sessionId = openCDM.CreateSession(m_mimeType, reinterpret_cast<const uint8_t*>(initData->data()), initData->size(), openCDMLicenseType(licenseType));

    if (sessionId.empty()) {
        GST_ERROR("could not create session id");
        callback(WTFMove(initData), { }, false, Failed);
        return;
    }

    String sessionIdValue = String::fromUTF8(sessionId.c_str());
    std::pair<std::map<std::string, Session>::iterator, bool> newEntry =
        m_sessionsMap.emplace(std::piecewise_construct, std::forward_as_tuple(sessionId), std::forward_as_tuple(openCDM, WTFMove(initData)));

    const Session& newSession(newEntry.first->second);

    if (newSession.isValid()) {
        std::string message = newSession.message();
        Ref<SharedBuffer> licenseRequestMessage = SharedBuffer::create(message.c_str(), message.size());
        callback(WTFMove(licenseRequestMessage), sessionIdValue, newSession.needsIndividualization(), Succeeded);
    } else
        callback(WTFMove(initData), sessionIdValue, false, Failed);
}

void CDMInstanceOpenCDM::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    std::map<std::string, Session>::iterator iterator(m_sessionsMap.find(sessionId.utf8().data()));

    if (iterator == m_sessionsMap.end()) {
        GST_WARNING("cannot update the session %s cause we can't find it", sessionId.utf8().data());
        return;
    }

    std::string responseMessage;
    Session& session(iterator->second);
    media::OpenCdm::KeyStatus keyStatus = session.update(reinterpret_cast<const uint8_t*>(response.data()), response.size(), responseMessage);
    GST_DEBUG("session id %s, key status is %d (usable: %s)", sessionId.utf8().data(), keyStatus, boolForPrinting(keyStatus == media::OpenCdm::KeyStatus::Usable));

    if (keyStatus == media::OpenCdm::KeyStatus::Usable) {
        KeyStatusVector changedKeys;
        SharedBuffer& initData(session.initData());

        GST_TRACE("OpenCDM::update returned key is usable");

        // FIXME: Why are we passing initData here, we are supposed to be passing key ids.
        changedKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatusFromOpenCDM(keyStatus)});
        callback(false, WTFMove(changedKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded);
    } else if (keyStatus != media::OpenCdm::KeyStatus::InternalError) {
        // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
        // will even be other parts of the spec where not having structured data will be bad.
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t offset = messageLength(responseMessage, request);
            const uint8_t* messageStart = reinterpret_cast<const uint8_t*>(responseMessage.c_str() + offset);
            size_t messageLength = responseMessage.length() - offset;
            GST_MEMDUMP("OpenCDM::update got this message for us", messageStart, messageLength);
            Ref<SharedBuffer> nextMessage = SharedBuffer::create(messageStart, messageLength);
            CDMInstance::Message message = std::make_pair(MediaKeyMessageType::LicenseRequest, WTFMove(nextMessage));
            callback(false, std::nullopt, std::nullopt, std::move(message), SuccessValue::Succeeded);
        } else {
            GST_ERROR("OpenCDM::update claimed to have a message, but it was not correctly formatted");
            callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        }
    } else {
        GST_ERROR("OpenCDM::update reported error state, update license failed");
        callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
    }
}

void CDMInstanceOpenCDM::loadSession(LicenseType, const String& sessionId, const String&, LoadSessionCallback callback)
{
    std::map<std::string, Session>::iterator iterator(m_sessionsMap.find(sessionId.utf8().data()));

    if (iterator == m_sessionsMap.end()) {
        GST_WARNING("cannot load the session %s cause we can't find it", sessionId.utf8().data());
        return;
    }

    std::string responseMessage;
    SessionLoadFailure sessionFailure(SessionLoadFailure::None);
    Session& session(iterator->second);

    if (!session.load(responseMessage)) {
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t length = messageLength(responseMessage, request);
            GST_TRACE("message length %u", length);
            auto message = SharedBuffer::create(responseMessage.c_str() + length, responseMessage.length() - length);
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
        } else {
            SharedBuffer& initData(session.initData());
            KeyStatusVector knownKeys;
            MediaKeyStatus keyStatus = mediaKeyStatusFromOpenCDM(responseMessage);
            knownKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatus});
            callback(WTFMove(knownKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
        }
    } else {
        sessionFailure = sessionLoadFailureFromOpenCDM(responseMessage);
        callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, sessionFailure);
    }
}

void CDMInstanceOpenCDM::removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback callback)
{
    std::map<std::string, Session>::iterator iterator(m_sessionsMap.find(sessionId.utf8().data()));

    if (iterator == m_sessionsMap.end()) {
        GST_WARNING("cannot remove session's data of %s cause we can't find it", sessionId.utf8().data());
        return;
    }

    std::string responseMessage;
    KeyStatusVector keys;
    Session& session(iterator->second);

    if (!session.remove(responseMessage)) {
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t length = messageLength(responseMessage, request);
            GST_TRACE("message length %u", length);

            auto message = SharedBuffer::create(responseMessage.c_str() + length, responseMessage.length() - length);
            SharedBuffer& initData = session.initData();
            std::string status = "KeyReleased";
            MediaKeyStatus keyStatus = mediaKeyStatusFromOpenCDM(status);
            keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatus});
            callback(WTFMove(keys), std::move(WTFMove(message)), SuccessValue::Succeeded);
        }
    } else {
        SharedBuffer& initData(session.initData());
        MediaKeyStatus keyStatus = mediaKeyStatusFromOpenCDM(responseMessage);
        keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatus});
        callback(WTFMove(keys), std::nullopt, SuccessValue::Failed);
    }
}

void CDMInstanceOpenCDM::closeSession(const String& session, CloseSessionCallback callback)
{
    std::string sessionId(session.utf8().data());
    std::map<std::string, Session>::iterator iterator(m_sessionsMap.find(sessionId));

    if (iterator != m_sessionsMap.end())
        iterator->second.close();

    if (!m_sessionsMap.erase(sessionId))
        GST_WARNING("%s is an unknown session", sessionId.c_str());

    callback();
}

String CDMInstanceOpenCDM::sessionIdByInitData(const String& initData, bool firstInLine) const
{
    if (!m_sessionsMap.size()) {
        GST_WARNING("no sessions");
        return { };
    }

    String result;
    for (auto iterator = m_sessionsMap.begin(); iterator != m_sessionsMap.end() && result.isEmpty(); ++iterator)
        if (iterator->second == initData)
            result = String::fromUTF8(iterator->first.c_str());

    if (result.isEmpty()) {
        if (firstInLine) {
            result = String::fromUTF8(m_sessionsMap.begin()->first.c_str());
            GST_INFO("Unknown session, returning the first in line: %s", result.utf8().data());
        } else
            GST_WARNING("Unknown session, nothing will be returned");
    } else
        GST_DEBUG("Found session for initdata: %s", result.utf8().data());

    return result;
}

bool CDMInstanceOpenCDM::isSessionIdUsable(const String& sessionId) const
{
    std::string sessionIdAsStdString(sessionId.utf8().data());
    const auto& element = m_sessionsMap.find(sessionIdAsStdString);

    return element != m_sessionsMap.end() && element->second.lastStatus() == media::OpenCdm::KeyStatus::Usable;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
