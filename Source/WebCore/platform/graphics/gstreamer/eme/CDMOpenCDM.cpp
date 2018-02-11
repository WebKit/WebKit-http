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

// ---------------------------------------------------------------------------------------------------------------------------
// Class CDMPrivate
// ---------------------------------------------------------------------------------------------------------------------------
class CDMPrivateOpenCDM : public CDMPrivate {
private:
    CDMPrivateOpenCDM() = delete;
    CDMPrivateOpenCDM(const CDMPrivateOpenCDM&) = delete;
    CDMPrivateOpenCDM& operator= (const CDMPrivateOpenCDM&) = delete;
    
public:
    CDMPrivateOpenCDM(const String& keySystem)
        : mm_openCdmKeySystem(keySystem)
        , m_openCdm() {
    }
    virtual ~CDMPrivateOpenCDM() = default;

public:
    virtual bool supportsInitDataType(const AtomicString& initDataType) const override {
        return equalLettersIgnoringASCIICase(initDataType, "cenc");
    }
    virtual bool supportsConfiguration(const MediaKeySystemConfiguration& config) const override {
        for (auto& audioCapability : config.audioCapabilities) {
            if (!m_openCdm.IsTypeSupported(mm_openCdmKeySystem.utf8().data(), audioCapability.contentType.utf8().data()))
                return false;
        }
        for (auto& videoCapability : config.videoCapabilities) {
            if (!m_openCdm.IsTypeSupported(mm_openCdmKeySystem.utf8().data(), videoCapability.contentType.utf8().data()))
                return false;
        }
        return true;
    }
    virtual bool supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration& config, const MediaKeysRestrictions&) const override {
        return supportsConfiguration(config);
    }
    virtual bool supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration& config) const override {
        return supportsConfiguration(config);
    }
    virtual bool supportsRobustness(const String&) const {
        return false;
    }
    virtual MediaKeysRequirement distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const override {
        return MediaKeysRequirement::Optional;
    }
    virtual MediaKeysRequirement persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const override {
        return MediaKeysRequirement::Optional;
    }
    virtual bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const override {
        return false;
    }
    virtual RefPtr<CDMInstance> createInstance() override {
        return adoptRef(new CDMInstanceOpenCDM(m_openCdm, mm_openCdmKeySystem));
    }
    virtual void loadAndInitialize() override {
    }
    virtual bool supportsServerCertificates() const override {
        return true;
    }
    virtual bool supportsSessions() const override {
        return true;
    }
    virtual bool supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const override {
        return equalLettersIgnoringASCIICase(initDataType, "cenc");
    }
    virtual RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer& response) const override {
        return response.copy();
    }
    virtual std::optional<String> sanitizeSessionId(const String& sessionId) const override {
        return sessionId;
    }

private:
    String mm_openCdmKeySystem;
    media::OpenCdm m_openCdm;
};

// ---------------------------------------------------------------------------------------------------------------------------
// Class CDMFactory
// ---------------------------------------------------------------------------------------------------------------------------
CDMFactoryOpenCDM& CDMFactoryOpenCDM::singleton()
{
    static CDMFactoryOpenCDM s_factory;
    return s_factory;
}

CDMFactoryOpenCDM::CDMFactoryOpenCDM() = default;
CDMFactoryOpenCDM::~CDMFactoryOpenCDM() = default;

std::unique_ptr<CDMPrivate> CDMFactoryOpenCDM::createCDM(const String& keySystem)
{
    return std::unique_ptr<CDMPrivate>(new CDMPrivateOpenCDM(keySystem));
}

bool CDMFactoryOpenCDM::supportsKeySystem(const String& keySystem)
{
    return GStreamerEMEUtilities::isPlayReadyKeySystem(keySystem);
}

// ---------------------------------------------------------------------------------------------------------------------------
// Class CDMInstanceOpenCDM
// ---------------------------------------------------------------------------------------------------------------------------
// Local helper methods:
// ---------------------------------------------------------------------------------------------------------------------------
static media::OpenCdm::LicenseType Convert(CDMInstance::LicenseType licenseType)
{
    switch (licenseType) {
    case CDMInstance::LicenseType::Temporary:
        return media::OpenCdm::Temporary;
        break;
    case CDMInstance::LicenseType::PersistentUsageRecord:
        return media::OpenCdm::PersistentUsageRecord;
        break;
    case CDMInstance::LicenseType::PersistentLicense:
        return media::OpenCdm::PersistentLicense;
        break;
    default:
        ASSERT_NOT_REACHED();
        return media::OpenCdm::Temporary;
        break;
    }
}

static CDMInstance::KeyStatus Convert(media::OpenCdm::KeyStatus keyStatus)
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

static CDMInstance::SessionLoadFailure ConvertSessionLoadStatus(std::string& loadStatus)
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

static MediaKeyStatus ConvertKeyStatus(std::string& keyStatus)
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

static size_t checkMessageLength(std::string& message, std::string& request)
{
    size_t length = 0;
    std::string delimiter = ":Type:";
    std::string requestType = message.substr(0, message.find(delimiter));
    // FIXME: What is with this weirdness?
    if (requestType.size() && (requestType.size() == (request.size() + 1)))
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
    , m_message()
    , m_url()
    , m_needIndividualisation(false)
    , m_buffer(WTFMove(initData)) {

    uint8_t  temporaryUrl[1024];
    uint16_t temporaryUrlLength = sizeof(temporaryUrl);

    m_session.GetKeyMessage(m_message, temporaryUrl, temporaryUrlLength);

    if ((m_message.empty() == true) || (temporaryUrlLength == 0)) {
        m_message.clear();
    }
    else {
        std::string delimiter (":Type:");
        std::string requestType (m_message.substr(0, m_message.find(delimiter)));

        m_url = std::string(reinterpret_cast<const char*>(temporaryUrl), temporaryUrlLength);

        if ( (requestType.size() != 0) && (requestType.size() !=  m_message.size()) ) {
            m_message.erase(0, m_message.find(delimiter) + delimiter.length());
        }

        m_needIndividualisation = ((requestType.size() == 1) && ((WebCore::MediaKeyMessageType)std::stoi(requestType) == CDMInstance::MessageType::IndividualizationRequest));
    }
}

CDMInstanceOpenCDM::Session::~Session() {
}

CDMInstanceOpenCDM::CDMInstanceOpenCDM(media::OpenCdm& system, const String& keySystem)
    : m_openCdm(system)
    , m_mimeType("video/x-h264")
    , m_keySystem(keySystem) {

    m_openCdm.SelectKeySystem(keySystem.utf8().data());

    if (GStreamerEMEUtilities::isPlayReadyKeySystem(keySystem))
        m_mimeType = "video/x-h264";
    else if (GStreamerEMEUtilities::isWidevineKeySystem(keySystem))
        m_mimeType = "video/mp4";
}

CDMInstanceOpenCDM::~CDMInstanceOpenCDM() = default;

CDMInstance::SuccessValue CDMInstanceOpenCDM::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    CDMInstance::SuccessValue ret = WebCore::CDMInstance::SuccessValue::Failed;
    if (m_openCdm.SetServerCertificate(reinterpret_cast<unsigned char*>(const_cast<char*>(certificate->data())), certificate->size()))
        ret = WebCore::CDMInstance::SuccessValue::Succeeded;
    return ret;
}

void CDMInstanceOpenCDM::requestLicense(LicenseType licenseType, const AtomicString&, Ref<SharedBuffer>&& initData, LicenseCallback callback)
{  
    std::string sessionId;

    GST_TRACE("Going to request a new session ID, init data size %u", initData->size());
    GST_MEMDUMP("init data", reinterpret_cast<const uint8_t*>(initData->data()), initData->size());

    media::OpenCdm openCdm(m_openCdm);

    sessionId = openCdm.CreateSession(m_mimeType, reinterpret_cast<const uint8_t*>(initData->data()), initData->size(), Convert(licenseType));

    if (sessionId.empty() == true) {
        String sessionIdValue;
        GST_ERROR("could not create session Id");
        callback(WTFMove(initData), sessionIdValue, false, Failed);
    }
    else {
        String sessionIdValue = String::fromUTF8(sessionId.c_str());
        std::pair<std::map<std::string, Session>::iterator, bool> newEntry = 
            m_sessionIdMap.emplace (std::piecewise_construct,
                               std::forward_as_tuple(sessionId),
                               std::forward_as_tuple(openCdm, WTFMove(initData)));

        const Session& newSession (newEntry.first->second);
        
        if (newSession.IsValid() == false) {
            callback(WTFMove(initData), sessionIdValue, false, Failed);
        }
        else {
            std::string message = newSession.Message();
            Ref<SharedBuffer> licenseRequestMessage = SharedBuffer::create(message.c_str(), message.size());
            callback(WTFMove(licenseRequestMessage), sessionIdValue, newSession.NeedIndividualization(), Succeeded);
        }
    }
}

void CDMInstanceOpenCDM::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    std::map<std::string, Session>::iterator index (m_sessionIdMap.find(sessionId.utf8().data()));

    if (index != m_sessionIdMap.end()) {

        std::string responseMessage;
        Session& session (index->second);

        media::OpenCdm::KeyStatus keyStatus = session.Update(reinterpret_cast<const uint8_t*>(response.data()), response.size(), responseMessage);

        GST_DEBUG("session id %s, key status is %d (usable: %s)", sessionId.utf8().data(), keyStatus, (keyStatus == media::OpenCdm::KeyStatus::Usable ? "true" : "false"));

        if (keyStatus == media::OpenCdm::KeyStatus::Usable) {

            KeyStatusVector changedKeys;
            SharedBuffer& initData (session.SharedBuffer());

            GST_TRACE("OpenCDM::update returned key is usable");

            // FIXME: Why are we passing initData here, we are supposed to be passing key IDs.
            changedKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {initData, Convert(keyStatus)});
            callback(false, WTFMove(changedKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded);
        } else if (keyStatus != media::OpenCdm::KeyStatus::InternalError) {
            // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
            // will even be other parts of the spec where not having structured data will be bad.
            std::string request = "message:";
            if (!responseMessage.compare(0, request.length(), request.c_str())) {
                size_t offset = checkMessageLength(responseMessage, request);
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
}

void CDMInstanceOpenCDM::loadSession(LicenseType, const String& sessionId, const String&, LoadSessionCallback callback)
{
    std::map<std::string, Session>::iterator index (m_sessionIdMap.find(sessionId.utf8().data()));

    if (index != m_sessionIdMap.end()) {

        std::string responseMessage;
        SessionLoadFailure sessionFailure(SessionLoadFailure::None);
        Session& session (index->second);
 
        if (session.Load(responseMessage) == 0) {

            std::string request = "message:";
            if (!responseMessage.compare(0, request.length(), request.c_str())) {
                size_t length = checkMessageLength(responseMessage, request);
                GST_TRACE("message length %u", length);
                auto message = SharedBuffer::create((responseMessage.c_str() + length), (responseMessage.length() - length));
                callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
            } else {
                SharedBuffer& initData (session.SharedBuffer());
                KeyStatusVector knownKeys;
                MediaKeyStatus keyStatus = ConvertKeyStatus(responseMessage);
                knownKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {initData, keyStatus});
                callback(WTFMove(knownKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
            }
        } else {
            sessionFailure = ConvertSessionLoadStatus(responseMessage);
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, sessionFailure);
        }
    }
}

void CDMInstanceOpenCDM::removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback callback)
{
    std::map<std::string, Session>::iterator index (m_sessionIdMap.find(sessionId.utf8().data()));

    if (index != m_sessionIdMap.end()) {
 
        std::string responseMessage;
        KeyStatusVector keys;
        Session& session (index->second);
    
        if (session.Remove(responseMessage) == 0) {
            std::string request = "message:";
            if (!responseMessage.compare(0, request.length(), request.c_str())) {
                size_t length = checkMessageLength(responseMessage, request);
                GST_TRACE("message length %u", length);

                auto message = SharedBuffer::create((responseMessage.c_str() + length), (responseMessage.length() - length));
                SharedBuffer& initData = session.SharedBuffer();
                std::string status = "KeyReleased";
                MediaKeyStatus keyStatus = ConvertKeyStatus(status);
                keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {initData, keyStatus});
                callback(WTFMove(keys), std::move(WTFMove(message)), SuccessValue::Succeeded);
            }
        } else {
            SharedBuffer& initData (session.SharedBuffer());
            MediaKeyStatus keyStatus = ConvertKeyStatus(responseMessage);
            keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {initData, keyStatus});
            callback(WTFMove(keys), std::nullopt, SuccessValue::Failed);
        }
    }
}

void CDMInstanceOpenCDM::closeSession(const String& session, CloseSessionCallback callback)
{
    std::string sessionId(session.utf8().data());
    std::map<std::string, Session>::iterator index (m_sessionIdMap.find(sessionId));

    if (index != m_sessionIdMap.end()) {
 
        index->second.Close();
    }
 
    if (!m_sessionIdMap.erase(sessionId)) {
        GST_WARNING("%s is an unknown session", sessionId.c_str());
    }

    callback();
}

void CDMInstanceOpenCDM::storeRecordOfKeyUsage(const String&)
{
}



String CDMInstanceOpenCDM::sessionIdByInitData(const String& initData, const bool firstInLine) const
{
    String result;

    if (m_sessionIdMap.size() == 0) {
        GST_WARNING("no sessions");
    }
    else {

        std::map<std::string, Session>::const_iterator index (m_sessionIdMap.begin());

        while ( (index != m_sessionIdMap.end()) && (result.isEmpty() == true) ) {

            if (index->second == initData) {
                result = String::fromUTF8(index->first.c_str());
            }

            index++;
        }
        if (result.isEmpty() == true) {
            if (firstInLine == true) {
                result = String::fromUTF8(m_sessionIdMap.begin()->first.c_str());
                GST_WARNING("Unknown session, grabbing the first in line [%s]!!!", result.utf8().data());
            }
            else {
                GST_WARNING("Unknown session, Nothing will be returned!!!");
            }
        }
        else {
            GST_DEBUG("Found session for initdata: %s", result.utf8().data());
        }
    }

    return result;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
