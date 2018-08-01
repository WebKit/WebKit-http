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
#include "MediaKeyMessageType.h"
#include "MediaKeysRequirement.h"

#include <gst/gst.h>
#include <wtf/text/Base64.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

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
    bool supportsRobustness(const String& robustness) const final { return !robustness.isEmpty(); }
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
    std::string emptyString;

    return m_openCDM.IsTypeSupported(keySystem.utf8().data(), emptyString);
}

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

static CDMInstance::SessionLoadFailure sessionLoadFailureFromOpenCDM(const std::string& loadStatus)
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

static MediaKeyStatus mediaKeyStatusFromOpenCDM(const std::string& keyStatus)
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

static RefPtr<SharedBuffer> cleanResponseMessage(const std::string& message)
{
    // If message does not begin with "message:", we bail out empty.
    if (message.compare(0, 8, "message:"))
        return { };

    // Check the following 5 characters, if they are Type: we skip them as well.
    unsigned offset = !message.compare(8, 5, "Type:") ? 13 : 8;

    RefPtr<SharedBuffer> cleanResponseMessage = SharedBuffer::create(message.c_str() + offset, message.size() - offset);
    return cleanResponseMessage;
}

Ref<CDMInstanceOpenCDM::Session> CDMInstanceOpenCDM::Session::create(const media::OpenCdm& source, Ref<WebCore::SharedBuffer>&& initData)
{
    return adoptRef(*new Session(source, WTFMove(initData)));
}

CDMInstanceOpenCDM::Session::Session(const media::OpenCdm& source, Ref<WebCore::SharedBuffer>&& initData)
    : m_session(source)
    , m_initData(WTFMove(initData))
{
    // FIXME: This can be a source of overflows and other big errors. This should be fixed in the framework and then fixed here.
    uint8_t temporaryURL[1024];
    uint16_t temporaryURLLength = sizeof(temporaryURL);

    std::string message;
    m_session.GetKeyMessage(message, temporaryURL, temporaryURLLength);

    if (message.empty() || !temporaryURLLength)
        return;

    m_isValid = temporaryURLLength;

    // We could do all operations with String but this way we can save some copies.
    size_t typePosition = message.find(":Type:");
    String requestType(message.c_str(), typePosition != std::string::npos ? typePosition : 0);
    unsigned offset = 0;
    if (!requestType.isEmpty() && requestType.length() != message.size())
        offset = typePosition + 6;

    m_message = SharedBuffer::create(message.c_str() + offset, message.size() - offset);
    m_needsIndividualization = requestType.length() == 1 && static_cast<WebCore::MediaKeyMessageType>(requestType.toInt()) == CDMInstance::MessageType::IndividualizationRequest;
}

CDMInstanceOpenCDM::CDMInstanceOpenCDM(media::OpenCdm& system, const String& keySystem)
    : m_keySystem(keySystem)
    , m_mimeType("video/x-h264")
    , m_openCDM(system)
{
    m_openCDM.SelectKeySystem(keySystem.utf8().data());
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    return m_openCDM.SetServerCertificate(reinterpret_cast<unsigned char*>(const_cast<char*>(certificate->data())), certificate->size())
        ? WebCore::CDMInstance::SuccessValue::Succeeded : WebCore::CDMInstance::SuccessValue::Failed;
}

void CDMInstanceOpenCDM::requestLicense(LicenseType licenseType, const AtomicString&, Ref<SharedBuffer>&& rawInitData, LicenseCallback callback)
{
    InitData initData = InitData(reinterpret_cast<const uint8_t*>(rawInitData->data()), rawInitData->size());

    GST_TRACE("Going to request a new session id, init data size %u and MD5 %s", initData.sizeInBytes(), GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    GST_MEMDUMP("init data", initData.characters8(), initData.sizeInBytes());

    String sessionIdAsString = sessionIdByInitData(initData);
    if (!sessionIdAsString.isEmpty()) {
        GST_DEBUG("session %s already created, bailing out", sessionIdAsString.utf8().data());
        callback(WTFMove(rawInitData), sessionIdAsString, false, Failed);
        return;
    }

    // FIXME: Why do we have this weirdness here? It looks like this is a way to reference count on the OpenCDM object.
    media::OpenCdm openCDM(m_openCDM);
    std::string sessionId = openCDM.CreateSession(m_mimeType, reinterpret_cast<const uint8_t*>(rawInitData->data()), rawInitData->size(), openCDMLicenseType(licenseType));

    if (sessionId.empty()) {
        GST_ERROR("could not create session id");
        callback(WTFMove(rawInitData), { }, false, Failed);
        return;
    }

    sessionIdAsString = String::fromUTF8(sessionId.c_str());
    Ref<Session> newSession = Session::create(openCDM, WTFMove(rawInitData));

    if (!newSession->isValid()) {
        GST_WARNING("created invalid session %s", sessionIdAsString.utf8().data());
        callback(WTFMove(rawInitData), sessionIdAsString, false, Failed);
        return;
    }

    GST_DEBUG("created valid session %s", sessionIdAsString.utf8().data());
    callback(newSession->message(), sessionIdAsString, newSession->needsIndividualization(), Succeeded);

    if (!addSession(sessionIdAsString, newSession.ptr()))
        GST_WARNING("Failed to add session %s, the session might already exist, or the allocation failed", sessionId.c_str());
}

void CDMInstanceOpenCDM::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot update the session %s cause we can't find it", sessionId.utf8().data());
        return;
    }

    std::string responseMessage;
    media::OpenCdm::KeyStatus keyStatus = session->update(reinterpret_cast<const uint8_t*>(response.data()), response.size(), responseMessage);
    GST_DEBUG("session id %s, key status is %d (usable: %s)", sessionId.utf8().data(), keyStatus, boolForPrinting(keyStatus == media::OpenCdm::KeyStatus::Usable));

    if (keyStatus == media::OpenCdm::KeyStatus::Usable) {
        KeyStatusVector changedKeys;
        SharedBuffer& initData(session->initData());

        GST_TRACE("OpenCDM::update returned key is usable");

        // FIXME: Why are we passing initData here, we are supposed to be passing key ids.
        changedKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatusFromOpenCDM(keyStatus)});
        callback(false, WTFMove(changedKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded);
    } else if (keyStatus != media::OpenCdm::KeyStatus::InternalError) {
        // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
        // will even be other parts of the spec where not having structured data will be bad.
        RefPtr<SharedBuffer> cleanMessage = cleanResponseMessage(responseMessage);
        if (cleanMessage) {
            GST_MEMDUMP("OpenCDM::update got this message for us", reinterpret_cast<const uint8_t*>(cleanMessage->data()), cleanMessage->size());
            callback(false, std::nullopt, std::nullopt, std::make_pair(MediaKeyMessageType::LicenseRequest, cleanMessage.releaseNonNull()), SuccessValue::Succeeded);
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
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot load the session %s cause we can't find it", sessionId.utf8().data());
        return;
    }
    std::string responseMessage;
    SessionLoadFailure sessionFailure = SessionLoadFailure::None;

    if (!session->load(responseMessage)) {
        if (!responseMessage.compare(0, 8, "message:")) {
            GST_TRACE("message length %u", responseMessage.size());
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
        } else {
            SharedBuffer& initData(session->initData());
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
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot remove session's data of %s cause we can't find it", sessionId.utf8().data());
        return;
    }

    std::string responseMessage;
    KeyStatusVector keys;

    if (!session->remove(responseMessage)) {
        RefPtr<SharedBuffer> cleanMessage = cleanResponseMessage(responseMessage);
        if (cleanMessage) {
            GST_TRACE("message length %u", cleanMessage->size());
            SharedBuffer& initData = session->initData();
            keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, MediaKeyStatus::Released});
            callback(WTFMove(keys), cleanMessage.releaseNonNull(), SuccessValue::Succeeded);
        } else {
            GST_ERROR("OpenCDM::update claimed to have a message, but it was not correctly formatted");
            callback(WTFMove(keys), std::nullopt, SuccessValue::Failed);
        }
    } else {
        SharedBuffer& initData(session->initData());
        MediaKeyStatus keyStatus = mediaKeyStatusFromOpenCDM(responseMessage);
        keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{initData, keyStatus});
        callback(WTFMove(keys), std::nullopt, SuccessValue::Failed);
    }

    if (!removeSession(sessionId))
        GST_WARNING("Failed to remove session %s", sessionId.utf8().data());
}

void CDMInstanceOpenCDM::closeSession(const String& sessionId, CloseSessionCallback callback)
{
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot close session %s because it does not exist", sessionId.utf8().data());
        return;
    }
    session->close();

    if (!removeSession(sessionId))
        GST_WARNING("Failed to remove session %s", sessionId.utf8().data());

    callback();
}

String CDMInstanceOpenCDM::sessionIdByInitData(const InitData& initData) const
{
    LockHolder locker(m_sessionMapMutex);

    if (!m_sessionsMap.size()) {
        GST_WARNING("no sessions");
        return { };
    }

    GST_TRACE("init data MD5 %s", GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    GST_MEMDUMP("init data", reinterpret_cast<const uint8_t*>(initData.characters8()), initData.length());

    String result;

    for (const auto& pair : m_sessionsMap) {
        const String& sessionId = pair.key;
        const RefPtr<Session>& session = pair.value;
        if (session->containsInitData(initData)) {
            result = sessionId;
            break;
        }
    }

    if (result.isEmpty())
        GST_WARNING("Unknown session, nothing will be returned");
    else
        GST_DEBUG("Found session for initdata: %s", result.utf8().data());

    return result;
}

bool CDMInstanceOpenCDM::isSessionIdUsable(const String& sessionId) const
{
    auto session = lookupSession(sessionId);
    return session && session->lastStatus() == media::OpenCdm::KeyStatus::Usable;
}

bool CDMInstanceOpenCDM::addSession(const String& sessionId, Session* session)
{
    LockHolder locker(m_sessionMapMutex);
    ASSERT(session);
    return m_sessionsMap.set(sessionId, session).isNewEntry;
}

bool CDMInstanceOpenCDM::removeSession(const String& sessionId)
{
    LockHolder locker(m_sessionMapMutex);
    return m_sessionsMap.remove(sessionId);
}

RefPtr<CDMInstanceOpenCDM::Session> CDMInstanceOpenCDM::lookupSession(const String& sessionId) const
{
    LockHolder locker(m_sessionMapMutex);
    auto session = m_sessionsMap.find(sessionId);
    return session == m_sessionsMap.end() ? nullptr : session->value;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
