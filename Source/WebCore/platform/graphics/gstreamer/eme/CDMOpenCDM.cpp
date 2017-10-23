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
#include <open_cdm.h>
#include <wtf/text/Base64.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

using namespace Inspector;

namespace WebCore {

class CDMPrivateOpenCDM : public CDMPrivate {

    String m_openCdmKeySystem;
    static std::unique_ptr<media::OpenCdm> s_openCdm;

public:

    CDMPrivateOpenCDM(const String&);
    virtual ~CDMPrivateOpenCDM();

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
    static media::OpenCdm* getOpenCdmInstance();
};

std::unique_ptr<media::OpenCdm> CDMPrivateOpenCDM::s_openCdm;

static media::OpenCdm::LicenseType webKitLicenseTypeToOpenCDM(CDMInstance::LicenseType licenseType)
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

CDMPrivateOpenCDM::CDMPrivateOpenCDM(const String& keySystem)
    : m_openCdmKeySystem(keySystem)
{
}

CDMPrivateOpenCDM::~CDMPrivateOpenCDM() = default;

bool CDMPrivateOpenCDM::supportsInitDataType(const AtomicString& initDataType) const
{
    return equalLettersIgnoringASCIICase(initDataType, "cenc");
}

bool CDMPrivateOpenCDM::supportsConfiguration(const MediaKeySystemConfiguration& config) const
{
    for (auto& audioCapability : config.audioCapabilities) {
        if (!getOpenCdmInstance()->IsTypeSupported(m_openCdmKeySystem.utf8().data(), audioCapability.contentType.utf8().data()))
            return false;
    }
    for (auto& videoCapability : config.videoCapabilities) {
        if (!getOpenCdmInstance()->IsTypeSupported(m_openCdmKeySystem.utf8().data(), videoCapability.contentType.utf8().data()))
            return false;
    }
    return true;
}

bool CDMPrivateOpenCDM::supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration& config, const MediaKeysRestrictions&) const
{
    return supportsConfiguration(config);
}

bool CDMPrivateOpenCDM::supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration& config) const
{
    return supportsConfiguration(config);
}

bool CDMPrivateOpenCDM::supportsRobustness(const String&) const
{
    return false;
}

media::OpenCdm* CDMPrivateOpenCDM::getOpenCdmInstance()
{
    if (!s_openCdm)
        s_openCdm = std::make_unique<media::OpenCdm>();
    return s_openCdm.get();
}

MediaKeysRequirement CDMPrivateOpenCDM::distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    return MediaKeysRequirement::Optional;
}

MediaKeysRequirement CDMPrivateOpenCDM::persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    return MediaKeysRequirement::Optional;
}

bool CDMPrivateOpenCDM::distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const
{
    return false;
}

RefPtr<CDMInstance> CDMPrivateOpenCDM::createInstance()
{
    getOpenCdmInstance()->SelectKeySystem(m_openCdmKeySystem.utf8().data());
    return adoptRef(new CDMInstanceOpenCDM(getOpenCdmInstance(), m_openCdmKeySystem));
}

void CDMPrivateOpenCDM::loadAndInitialize()
{
}

bool CDMPrivateOpenCDM::supportsServerCertificates() const
{
    return true;
}

bool CDMPrivateOpenCDM::supportsSessions() const
{
    return true;
}

bool CDMPrivateOpenCDM::supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const
{
    return equalLettersIgnoringASCIICase(initDataType, "cenc");
}

RefPtr<SharedBuffer> CDMPrivateOpenCDM::sanitizeResponse(const SharedBuffer& response) const
{
    return response.copy();
}

std::optional<String> CDMPrivateOpenCDM::sanitizeSessionId(const String& sessionId) const
{
    return sessionId;
}

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
    return GStreamerEMEUtilities::isPlayReadyKeySystem(keySystem) || GStreamerEMEUtilities::isWidevineKeySystem(keySystem);
}

CDMInstanceOpenCDM::CDMInstanceOpenCDM(media::OpenCdm* session, const String& keySystem)
    : m_openCdmSession(session)
    , m_keySystem(keySystem)
{
}

CDMInstanceOpenCDM::~CDMInstanceOpenCDM() = default;

CDMInstance::SuccessValue CDMInstanceOpenCDM::initializeWithConfiguration(const MediaKeySystemConfiguration&)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setDistinctiveIdentifiersAllowed(bool)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setPersistentStateAllowed(bool)
{
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    CDMInstance::SuccessValue ret = WebCore::CDMInstance::SuccessValue::Failed;
    if (m_openCdmSession->SetServerCertificate(reinterpret_cast<unsigned char*>(const_cast<char*>(certificate->data())), certificate->size()))
        ret = WebCore::CDMInstance::SuccessValue::Succeeded;
    return ret;
}

void CDMInstanceOpenCDM::requestLicense(LicenseType licenseType, const AtomicString&, Ref<SharedBuffer>&& initData, LicenseCallback callback)
{   
    std::string sessionId;
    String sessionIdValue;
    String mimeType = "video/x-h264";
    if (GStreamerEMEUtilities::isPlayReadyKeySystem(m_keySystem))
        mimeType = "video/x-h264";
    else if (GStreamerEMEUtilities::isWidevineKeySystem(m_keySystem))
        mimeType = "video/mp4";

    GST_TRACE("Going to request a new session ID");

    bool createdSession = m_openCdmSession->CreateSession(mimeType.utf8().data(), reinterpret_cast<unsigned char*>(const_cast<char*>(initData->data())),
        initData->size(), sessionId, webKitLicenseTypeToOpenCDM(licenseType));

    if (!createdSession) {
        callback(WTFMove(initData), sessionIdValue, false, Failed);
        return;
    }

    sessionIdValue = String::fromUTF8(sessionId.c_str());
    GST_TRACE("create session succeeded, session ID is %s", sessionIdValue.utf8().data());

    unsigned char temporaryUrl[1024] = { };
    std::string message;
    int messageLength = 0;
    int destinationUrlLength = 0;
    m_openCdmSession->GetKeyMessage(message, &messageLength, temporaryUrl, &destinationUrlLength);
    if (!messageLength || !destinationUrlLength) {
        callback(WTFMove(initData), sessionIdValue, false, Failed);
        return;
    }

    GST_TRACE("successfully called GetKeyMessage");
    bool needIndividualization = false;
    std::string delimiter = ":Type:";
    std::string requestType = message.substr(0, message.find(delimiter));
    size_t previousMessageSize = message.size();
    if (requestType.size() && (requestType.size() !=  message.size()))
        message.erase(0, message.find(delimiter) + delimiter.length());
    GST_TRACE("message.size before erase = %u, message.size after erase = %u, delimiter.size = %u, delimiter = %s, requestType.size = %u, requestType = %s", previousMessageSize, message.size(), delimiter.size(), delimiter.c_str(), requestType.size(), requestType.c_str());
    if ((requestType.size() == 1) && ((WebCore::MediaKeyMessageType)std::stoi(requestType) == CDMInstance::MessageType::IndividualizationRequest))
        needIndividualization = true;

    Ref<SharedBuffer> licenseRequestMessage = SharedBuffer::create(message.c_str(), message.size());
    callback(WTFMove(licenseRequestMessage), sessionIdValue, needIndividualization, Succeeded);
    sessionIdMap.add(sessionIdValue, WTFMove(initData));
}

void CDMInstanceOpenCDM::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    std::string responseMessage;
    // FIXME: This is a realy awful API, why not just take a string& ?
    int haveMessage = m_openCdmSession->Update(reinterpret_cast<unsigned char*>(const_cast<char*>(response.data())), response.size(), responseMessage);
    GST_DEBUG("session id %s, calling callback %s message", sessionId.utf8().data(), haveMessage ? "with" : "without");

    if (haveMessage) {
        GST_TRACE("OpenCDM::Update has a message for us");
        // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
        // will even be other parts of the spec where not having structured data will be bad.
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t offset = checkMessageLength(responseMessage, request);
            unsigned const char* messageStart = reinterpret_cast<unsigned const char*>(responseMessage.c_str() + offset);
            size_t messageLength = responseMessage.length() - offset;
            GST_MEMDUMP("Message received from platform update", messageStart, messageLength);
            Ref<SharedBuffer> nextMessage = SharedBuffer::create(messageStart, messageLength);
            CDMInstance::Message message = std::make_pair(MediaKeyMessageType::LicenseRequest, WTFMove(nextMessage));
            callback(false, std::nullopt, std::nullopt, std::move(message), SuccessValue::Succeeded);
        } else {
            GST_ERROR("OpenCDM::update claimed to have a message, but it was not correctly formatted");
            callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        }
        return;
    }

    GST_TRACE("No message from update, we have a new key status object to communicate");
    SharedBuffer* initData = sessionIdMap.get(sessionId);
    KeyStatusVector changedKeys;
    // FIXME: When Update returns false, the response message is empty
    MediaKeyStatus keyStatus = getKeyStatus(responseMessage);
    // FIXME: Why are we passing initData here, we are supposed to be passing key IDs.
    changedKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus>{*initData, keyStatus});
    callback(false, WTFMove(changedKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded);
}

CDMInstance::SessionLoadFailure CDMInstanceOpenCDM::getSessionLoadStatus(std::string& loadStatus)
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

MediaKeyStatus CDMInstanceOpenCDM::getKeyStatus(std::string& keyStatus)
{
    if (keyStatus == "KeyUsable")
        return MediaKeyStatus::Usable;
    if (keyStatus == "KeyExpired")
        return MediaKeyStatus::Expired;
    if (keyStatus == "KeyOutputRestricted")
        return MediaKeyStatus::OutputRestricted;
    if (keyStatus == "KeyStatusPending")
        return MediaKeyStatus::OutputRestricted;
    if (keyStatus == "KeyInternalError")
        return MediaKeyStatus::InternalError;
    if (keyStatus == "KeyReleased")
        return MediaKeyStatus::Released;
    return MediaKeyStatus::InternalError;
}

size_t CDMInstanceOpenCDM::checkMessageLength(std::string& message, std::string& request)
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

void CDMInstanceOpenCDM::loadSession(LicenseType, const String& sessionId, const String&, LoadSessionCallback callback)
{
    std::string responseMessage;
    SessionLoadFailure sessionFailure = SessionLoadFailure::None;
    int ret = m_openCdmSession->Load(responseMessage);
    if (!ret) {
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t length = checkMessageLength(responseMessage, request);
            GST_TRACE("message length %u", length);
            auto message = SharedBuffer::create((responseMessage.c_str() + length), (responseMessage.length() - length));
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
        } else {
            SharedBuffer* initData = sessionIdMap.get(sessionId);
            KeyStatusVector knownKeys;
            MediaKeyStatus keyStatus = getKeyStatus(responseMessage);
            knownKeys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {*initData, keyStatus});
            callback(WTFMove(knownKeys), std::nullopt, std::nullopt, SuccessValue::Succeeded, sessionFailure);
        }
    } else {
        sessionFailure = getSessionLoadStatus(responseMessage);
        callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, sessionFailure);
    }
}

void CDMInstanceOpenCDM::closeSession(const String&, CloseSessionCallback callback)
{
    m_openCdmSession->Close();
    callback();
}

void CDMInstanceOpenCDM::removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback callback)
{
    std::string responseMessage;
    KeyStatusVector keys;
    int ret = m_openCdmSession->Remove(responseMessage);
    if (!ret) {
        std::string request = "message:";
        if (!responseMessage.compare(0, request.length(), request.c_str())) {
            size_t length = checkMessageLength(responseMessage, request);
            GST_TRACE("message length %u", length);

            auto message = SharedBuffer::create((responseMessage.c_str() + length), (responseMessage.length() - length));
            SharedBuffer* initData = sessionIdMap.get(sessionId);
            std::string status = "KeyReleased";
            MediaKeyStatus keyStatus = getKeyStatus(status);
            keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {*initData, keyStatus});
            callback(WTFMove(keys), std::move(WTFMove(message)), SuccessValue::Succeeded);
        }
    } else {
        SharedBuffer* initData = sessionIdMap.get(sessionId);
        MediaKeyStatus keyStatus = getKeyStatus(responseMessage);
        keys.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> {*initData, keyStatus});
        callback(WTFMove(keys), std::nullopt, SuccessValue::Failed);
    }
}

void CDMInstanceOpenCDM::storeRecordOfKeyUsage(const String&)
{
}

String CDMInstanceOpenCDM::getCurrentSessionId() const
{
    ASSERT(sessionIdMap.size() == 1);

    if (sessionIdMap.size() == 0) {
        GST_WARNING("no sessions");
        return { };
    }
    if (sessionIdMap.size() > 1)
        GST_WARNING("more than one session");

    return sessionIdMap.begin()->key;
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(OPENCDM)
