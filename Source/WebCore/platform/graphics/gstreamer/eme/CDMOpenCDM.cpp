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
#include "GenericTaskQueue.h"
#include "MediaKeyMessageType.h"
#include "MediaKeysRequirement.h"
#include "MediaPlayerPrivateGStreamerBase.h"

#include <gst/gst.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/Base64.h>

GST_DEBUG_CATEGORY_EXTERN(webkit_media_opencdm_decrypt_debug_category);
#define GST_CAT_DEFAULT webkit_media_opencdm_decrypt_debug_category

namespace {

using OCDMKeyStatus = KeyStatus;

}

namespace WebCore {

class CDMPrivateOpenCDM : public CDMPrivate {
private:
    CDMPrivateOpenCDM() = delete;
    CDMPrivateOpenCDM(const CDMPrivateOpenCDM&) = delete;
    CDMPrivateOpenCDM& operator=(const CDMPrivateOpenCDM&) = delete;

public:
    CDMPrivateOpenCDM(const String& keySystem)
        : m_openCDMKeySystem(keySystem)
        , m_openCDMSystem(opencdm_create_system(keySystem.utf8().data()))
    {
    }
    virtual ~CDMPrivateOpenCDM() = default;

public:
    bool supportsInitDataType(const AtomicString& initDataType) const final { return equalLettersIgnoringASCIICase(initDataType, "cenc") || equalLettersIgnoringASCIICase(initDataType, "webm") || equalLettersIgnoringASCIICase(initDataType, "keyids"); }
    bool supportsConfiguration(const MediaKeySystemConfiguration& config) const final;
    bool supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration& config, const MediaKeysRestrictions&) const final { return supportsConfiguration(config); }
    bool supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration& config) const final { return supportsConfiguration(config); }
    bool supportsRobustness(const String& robustness) const final { return !robustness.isEmpty(); }
    MediaKeysRequirement distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const final { return MediaKeysRequirement::Optional; }
    MediaKeysRequirement persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const final { return MediaKeysRequirement::Optional; }
    bool distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const final { return false; }
    RefPtr<CDMInstance> createInstance() final { return adoptRef(new CDMInstanceOpenCDM(*m_openCDMSystem, m_openCDMKeySystem)); }
    void loadAndInitialize() final { }
    bool supportsServerCertificates() const final { return true; }
    bool supportsSessions() const final { return true; }
    bool supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const final { return supportsInitDataType(initDataType); }
    RefPtr<SharedBuffer> sanitizeResponse(const SharedBuffer& response) const final { return response.copy(); }
    std::optional<String> sanitizeSessionId(const String& sessionId) const final { return sessionId; }

private:
    String m_openCDMKeySystem;
    // This owns OCDM System and passes a bare pointer further because it's owned by CDMInstance
    // which lives as long as any MediaKeySession lives.
    ScopedOCDMSystem m_openCDMSystem;
};

class CDMInstanceOpenCDM::Session : public ThreadSafeRefCounted<CDMInstanceOpenCDM::Session> {
public:
    using Notification = void (Session::*)(RefPtr<WebCore::SharedBuffer>&&);
    using ChallengeGeneratedCallback = Function<void(Session*)>;
    using SessionChangedCallback = Function<void(Session*, bool, RefPtr<SharedBuffer>&&, KeyStatusVector&)>;

    static Ref<Session> create(CDMInstanceOpenCDM*, OpenCDMSystem&, const String&, const AtomicString&, Ref<WebCore::SharedBuffer>&&, LicenseType, Ref<WebCore::SharedBuffer>&&);
    ~Session();

    bool isValid() const { return m_session.get() && m_message && !m_message->isEmpty(); }
    const String& id() const { return m_id; }
    Ref<SharedBuffer> message() const { ASSERT(m_message); return Ref<SharedBuffer>(*m_message.get()); }
    bool needsIndividualization() const { return m_needsIndividualization; }
    const Ref<WebCore::SharedBuffer>& initData() const { return m_initData; }
    void generateChallenge(ChallengeGeneratedCallback&&);
    void update(const uint8_t*, unsigned, SessionChangedCallback&&);
    void load(SessionChangedCallback&&);
    void remove(SessionChangedCallback&&);
    bool close() { return m_session && !id().isEmpty() ? !opencdm_session_close(m_session.get()) : true; }
    OCDMKeyStatus status(const SharedBuffer& keyId) const
    {
        return m_session && !id().isEmpty() ? opencdm_session_status(m_session.get(), reinterpret_cast<const uint8_t*>(keyId.data()), keyId.size()) : StatusPending;
    }

    bool containsKeyId(const SharedBuffer& keyId) const
    {
        if (!keyId.data())
            return false;

        auto index = m_keyStatuses.findMatching([&keyId](const std::pair<Ref<SharedBuffer>, KeyStatus>& item) {
            return memmem(keyId.data(), keyId.size(), item.first->data(), item.first->size());
        });

        if (index == notFound && GStreamerEMEUtilities::isPlayReadyKeySystem(m_keySystem)) {
            GST_DEBUG("Trying to match the playready session indirectly");
            // PlayReady corner case: It happens that the keyid required by the stream and reported by the CDM
            // have different endianness of the 4-2-2 GUID components. OCDM caters for that so ask it if it knows a session
            // with the given key id and compare session ids if it matches the session owned by this instance.
            WebCore::ScopedSession session { opencdm_get_system_session(&m_ocdmSystem, reinterpret_cast<const uint8_t*>(keyId.data()), keyId.size(), 0) };
            GST_TRACE("Session %p with id %s", session.get(), session.get() ? opencdm_session_id(session.get()) : "NA");
            if (session.get() && !strcmp(opencdm_session_id(session.get()), m_id.utf8().data()))
                return true;
        }

        return index != notFound;
    }

    static void openCDMNotification(const OpenCDMSession*, void*, Notification, const char* name, const uint8_t[], uint16_t);

private:
    Session() = delete;
    Session(CDMInstanceOpenCDM*, OpenCDMSystem&, const String&, const AtomicString&, Ref<WebCore::SharedBuffer>&&, LicenseType, Ref<WebCore::SharedBuffer>&&);
    void challengeGeneratedCallback(RefPtr<SharedBuffer>&&);
    void keyUpdatedCallback(RefPtr<SharedBuffer>&& = nullptr);
    // This doesn't need any params but it's made like this to fit the notification mechanism in place.
    void keysUpdateDoneCallback(RefPtr<SharedBuffer>&& = nullptr);
    void errorCallback(RefPtr<SharedBuffer>&&);
    void loadFailure() { updateFailure(); }
    void removeFailure() { updateFailure(); }
    void updateFailure()
    {
        for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
            sessionChangedCallback(this, false, nullptr, m_keyStatuses);
        m_sessionChangedCallbacks.clear();
    }

    WTF_MAKE_NONCOPYABLE(Session);

    ScopedSession m_session;
    RefPtr<SharedBuffer> m_message;
    String m_id;
    bool m_needsIndividualization { false };
    Ref<WebCore::SharedBuffer> m_initData;
    OpenCDMSessionCallbacks m_openCDMSessionCallbacks { };
    Vector<ChallengeGeneratedCallback> m_challengeCallbacks;
    Vector<SessionChangedCallback> m_sessionChangedCallbacks;
    String m_keySystem;
    OpenCDMSystem& m_ocdmSystem;
    CDMInstanceOpenCDM* m_parent;
    // Accessed only on the main thread allowing to track if the Session is still valid and could be used.
    // Needed due to the fact the Session pointer is passed to the OCDM as the userData for notifications which are no
    // warranted to be called on the main thread the Session lives on.
    static HashSet<Session*> m_validSessions;
    KeyStatusVector m_keyStatuses;
};

HashSet<CDMInstanceOpenCDM::Session*> CDMInstanceOpenCDM::Session::m_validSessions;

bool CDMPrivateOpenCDM::supportsConfiguration(const MediaKeySystemConfiguration& config) const
{
    for (auto& audioCapability : config.audioCapabilities)
        if (opencdm_is_type_supported(m_openCDMKeySystem.utf8().data(), audioCapability.contentType.utf8().data()))
            return false;
    for (auto& videoCapability : config.videoCapabilities)
        if (opencdm_is_type_supported(m_openCDMKeySystem.utf8().data(), videoCapability.contentType.utf8().data()))
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

    return !opencdm_is_type_supported(keySystem.utf8().data(), emptyString.c_str());
}

static LicenseType openCDMLicenseType(CDMInstance::LicenseType licenseType)
{
    switch (licenseType) {
    case CDMInstance::LicenseType::Temporary:
        return Temporary;
    case CDMInstance::LicenseType::PersistentUsageRecord:
        return PersistentUsageRecord;
    case CDMInstance::LicenseType::PersistentLicense:
        return PersistentLicense;
    default:
        ASSERT_NOT_REACHED();
        return Temporary;
    }
}

static CDMInstance::KeyStatus keyStatusFromOpenCDM(KeyStatus keyStatus)
{
    switch (keyStatus) {
    case Usable:
        return CDMInstance::KeyStatus::Usable;
    case Expired:
        return CDMInstance::KeyStatus::Expired;
    case Released:
        return CDMInstance::KeyStatus::Released;
    case OutputRestricted:
        return CDMInstance::KeyStatus::OutputRestricted;
    case OutputDownscaled:
        return CDMInstance::KeyStatus::OutputDownscaled;
    case StatusPending:
        return CDMInstance::KeyStatus::StatusPending;
    case InternalError:
        return CDMInstance::KeyStatus::InternalError;
    default:
        ASSERT_NOT_REACHED();
        return CDMInstance::KeyStatus::InternalError;
    }
}

static CDMInstance::SessionLoadFailure sessionLoadFailureFromOpenCDM(const String& loadStatus)
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

static MediaKeyStatus mediaKeyStatusFromOpenCDM(const String& keyStatus)
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

static MediaKeyStatus mediaKeyStatusFromOpenCDM(const SharedBuffer& keyStatusBuffer)
{
    String keyStatus(StringImpl::createWithoutCopying(reinterpret_cast<const LChar*>(keyStatusBuffer.data()), keyStatusBuffer.size()));
    return mediaKeyStatusFromOpenCDM(keyStatus);
}

static CDMInstance::KeyStatusVector copyAndMaybeReplaceValue(CDMInstance::KeyStatusVector& keyStatuses, std::optional<MediaKeyStatus> newStatus = std::nullopt)
{
    CDMInstance::KeyStatusVector copy;
    for (auto& keyStatus : keyStatuses) {
        keyStatus.second = newStatus.value_or(keyStatus.second);
        copy.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> { keyStatus.first.copyRef(), keyStatus.second });
    }

    return copy;
}

static RefPtr<SharedBuffer> parseResponseMessage(const SharedBuffer& buffer, std::optional<WebCore::MediaKeyMessageType>& messageType)
{
    String message(StringImpl::createWithoutCopying(reinterpret_cast<const LChar*>(buffer.data()), buffer.size()));
    size_t typePosition = message.find(":Type:");
    String requestType(message.characters8(), typePosition != notFound ? typePosition : 0);
    unsigned offset = 0u;
    if (!requestType.isEmpty() && requestType.length() != message.length())
        offset = typePosition + 6;

    if (requestType.length() == 1)
        messageType = static_cast<WebCore::MediaKeyMessageType>(requestType.toInt());
    return SharedBuffer::create(message.characters8() + offset, message.sizeInBytes() - offset);
}


Ref<CDMInstanceOpenCDM::Session> CDMInstanceOpenCDM::Session::create(CDMInstanceOpenCDM* parent, OpenCDMSystem& source, const String& keySystem, const AtomicString& initDataType, Ref<WebCore::SharedBuffer>&& initData, LicenseType licenseType, Ref<WebCore::SharedBuffer>&& customData)
{
    return adoptRef(*new Session(parent, source, keySystem, initDataType, WTFMove(initData), licenseType, WTFMove(customData)));
}

void CDMInstanceOpenCDM::Session::openCDMNotification(const OpenCDMSession*, void* userData, Notification method, const char* name, const uint8_t message[], uint16_t messageLength)
{
    GST_DEBUG("Got '%s' OCDM notification", name);
    Session* session = reinterpret_cast<Session*>(userData);
    RefPtr<WebCore::SharedBuffer> sharedBuffer = WebCore::SharedBuffer::create(message, messageLength);
    if (!isMainThread()) {
        // Make sure all happens on the main thread to avoid locking.
        callOnMainThread([session, method, buffer = WTFMove(sharedBuffer)]() mutable {
            if (!Session::m_validSessions.contains(session)) {
                // Became invalid in the meantime. It's possible due to leaping through the different threads.
                return;
            }
            (session->*method)(WTFMove(buffer));
        });
        return;
    }

    (session->*method)(WTFMove(sharedBuffer));
}

CDMInstanceOpenCDM::Session::Session(CDMInstanceOpenCDM* parent, OpenCDMSystem& source, const String& keySystem, const AtomicString& initDataType, Ref<WebCore::SharedBuffer>&& initData, LicenseType licenseType, Ref<WebCore::SharedBuffer>&& customData)
    : m_initData(WTFMove(initData))
    , m_keySystem(keySystem)
    , m_ocdmSystem(source)
    , m_parent(parent)
{
    OpenCDMSession* session = nullptr;
    m_openCDMSessionCallbacks.process_challenge_callback = [](OpenCDMSession* session, void* userData, const char[], const uint8_t challenge[], const uint16_t challengeLength) {
        Session::openCDMNotification(session, userData, &Session::challengeGeneratedCallback, "challenge", challenge, challengeLength);
    };
    m_openCDMSessionCallbacks.key_update_callback = [](OpenCDMSession* session, void* userData, const uint8_t key[], const uint8_t keyLength) {
        Session::openCDMNotification(session, userData, &Session::keyUpdatedCallback, "key updated", key, keyLength);
    };
    m_openCDMSessionCallbacks.keys_updated_callback = [](const OpenCDMSession* session, void* userData) {
        Session::openCDMNotification(session, userData, &Session::keysUpdateDoneCallback, "all keys updated", nullptr, 0);
    };
    m_openCDMSessionCallbacks.error_message_callback = [](OpenCDMSession* session, void* userData, const char message[]) {
        Session::openCDMNotification(session, userData, &Session::errorCallback, "error", reinterpret_cast<const uint8_t*>(message), strlen(message));
    };

    GST_DEBUG("Creating session for '%s' keySystem and '%s' initDataType\n", keySystem.utf8().data(), initDataType.string().utf8().data());
    opencdm_construct_session(&source, openCDMLicenseType(licenseType), initDataType.string().utf8().data(), reinterpret_cast<const uint8_t*>(m_initData->data()), m_initData->size(),
        !customData->isEmpty() ? reinterpret_cast<const uint8_t*>(customData->data()) : nullptr, customData->size(), &m_openCDMSessionCallbacks, this, &session);
    if (!session) {
        GST_ERROR("Could not create session");
        return;
    }
    m_session.reset(session);
    m_id = String::fromUTF8(opencdm_session_id(m_session.get()));
    Session::m_validSessions.add(this);
}

CDMInstanceOpenCDM::Session::~Session()
{
    close();
    Session::m_validSessions.remove(this);
}

void CDMInstanceOpenCDM::Session::challengeGeneratedCallback(RefPtr<SharedBuffer>&& buffer)
{
    std::optional<WebCore::MediaKeyMessageType> requestType;
    auto message = buffer ? parseResponseMessage(*buffer, requestType) : nullptr;

    // This can be called as a result of e.g. requestLicense() but update() or remove() as well.
    // This called not as a response to API call is also possible.
    if (!m_challengeCallbacks.isEmpty()) {
        std::optional<WebCore::MediaKeyMessageType> requestType;
        m_message = WTFMove(message);
        m_needsIndividualization = requestType == CDMInstance::MessageType::IndividualizationRequest;

        for (const auto& challengeCallback : m_challengeCallbacks)
            challengeCallback(this);
        m_challengeCallbacks.clear();
    } else if (!m_sessionChangedCallbacks.isEmpty()) {
        for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
            sessionChangedCallback(this, true, message.copyRef(), m_keyStatuses);
        m_sessionChangedCallbacks.clear();
    } else {
        if (m_parent->client() && requestType.has_value())
            m_parent->client()->issueMessage(static_cast<CDMInstanceClient::MessageType>(requestType.value()), message.releaseNonNull());
    }
}

void CDMInstanceOpenCDM::Session::keyUpdatedCallback(RefPtr<SharedBuffer>&& buffer)
{
    GST_MEMDUMP("Updated key", reinterpret_cast<const guint8*>(buffer->data()), buffer->size());
    auto index = m_keyStatuses.findMatching([&buffer](const std::pair<Ref<SharedBuffer>, KeyStatus>& item) {
        return memmem(buffer->data(), buffer->size(), item.first->data(), item.first->size());
    });

    auto keyStatus = keyStatusFromOpenCDM(status(*buffer));
    if (index != notFound)
        m_keyStatuses[index].second = keyStatus;
    else
        m_keyStatuses.append(std::pair<Ref<SharedBuffer>, MediaKeyStatus> { buffer.releaseNonNull(), keyStatus });
}

void CDMInstanceOpenCDM::Session::keysUpdateDoneCallback(RefPtr<SharedBuffer>&&)
{
    bool appliesToApiCall = !m_sessionChangedCallbacks.isEmpty();
    if (!appliesToApiCall && m_parent && m_parent->client()) {
        m_parent->client()->updateKeyStatuses(copyAndMaybeReplaceValue(m_keyStatuses));
        return;
    }

    for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
        sessionChangedCallback(this, true, nullptr, m_keyStatuses);
    m_sessionChangedCallbacks.clear();
}

void CDMInstanceOpenCDM::Session::errorCallback(RefPtr<SharedBuffer>&& message)
{
    for (const auto& challengeCallback : m_challengeCallbacks)
        challengeCallback(this);
    m_challengeCallbacks.clear();

    for (auto& sessionChangedCallback : m_sessionChangedCallbacks)
        sessionChangedCallback(this, false, WTFMove(message), m_keyStatuses);
    m_sessionChangedCallbacks.clear();
}

void CDMInstanceOpenCDM::Session::generateChallenge(ChallengeGeneratedCallback&& callback)
{
    if (isValid()) {
        callback(this);
        return;
    }

    m_challengeCallbacks.append(WTFMove(callback));
}

void CDMInstanceOpenCDM::Session::update(const uint8_t* data, const unsigned length, SessionChangedCallback&& callback)
{
    m_keyStatuses.clear();
    m_sessionChangedCallbacks.append(WTFMove(callback));
    if (!m_session || id().isEmpty() || opencdm_session_update(m_session.get(), data, length))
        updateFailure();

    // Assumption: should report back either with a message to be sent to the license server or key statuses updates.
}

void CDMInstanceOpenCDM::Session::load(SessionChangedCallback&& callback)
{
    m_keyStatuses.clear();
    m_sessionChangedCallbacks.append(WTFMove(callback));
    if (!m_session || id().isEmpty() || opencdm_session_load(m_session.get()))
        loadFailure();

    // Assumption: should report back either with a message to be sent to the license server or key status updates.
}

void CDMInstanceOpenCDM::Session::remove(SessionChangedCallback&& callback)
{
    // m_keyStatuses are not cleared here not to rely on CDM callbacks with Released status.
    m_sessionChangedCallbacks.append(WTFMove(callback));
    if (!m_session || id().isEmpty() || opencdm_session_remove(m_session.get()))
        removeFailure();

    // Assumption: should report back either with a message to be sent to the license server or key updates with "KeyReleased" status.
}

CDMInstanceOpenCDM::CDMInstanceOpenCDM(OpenCDMSystem& system, const String& keySystem)
    : m_keySystem(keySystem)
    , m_openCDMSystem(system)
{
    MediaPlayerPrivateGStreamerBase::ensureWebKitGStreamerElements();
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setServerCertificate(Ref<SharedBuffer>&& certificate)
{
    return !opencdm_system_set_server_certificate(&m_openCDMSystem, reinterpret_cast<unsigned char*>(const_cast<char*>(certificate->data())), certificate->size())
        ? WebCore::CDMInstance::SuccessValue::Succeeded
        : WebCore::CDMInstance::SuccessValue::Failed;
}

CDMInstance::SuccessValue CDMInstanceOpenCDM::setStorageDirectory(const String&)
{
    return WebCore::CDMInstance::SuccessValue::Succeeded;
}

void CDMInstanceOpenCDM::requestLicense(LicenseType licenseType, const AtomicString& initDataType, Ref<SharedBuffer>&& rawInitData, Ref<SharedBuffer>&& rawCustomData, LicenseCallback callback)
{
    InitData initData = InitData(reinterpret_cast<const uint8_t*>(rawInitData->data()), rawInitData->size());

    GST_TRACE("Going to request a new session id, init data size %u and MD5 %s", initData.sizeInBytes(), GStreamerEMEUtilities::initDataMD5(initData).utf8().data());
    GST_MEMDUMP("init data", initData.characters8(), initData.sizeInBytes());
    auto generateChallenge = [this, callback = WTFMove(callback)](Session* session) {
        auto sessionId = session->id();
        if (sessionId.isEmpty()) {
            GST_ERROR("could not create session id");
            callback(session->initData().copyRef(), { }, false, Failed);
            return;
        }

        if (!session->isValid()) {
            GST_WARNING("created invalid session %s", sessionId.utf8().data());
            callback(session->initData().copyRef(), session->id(), false, Failed);
            removeSession(sessionId);
            return;
        }

        GST_DEBUG("created valid session %s", sessionId.utf8().data());
        callback(session->message(), sessionId, session->needsIndividualization(), Succeeded);
    };

    Ref<Session> newSession = Session::create(this, m_openCDMSystem, m_keySystem, initDataType, WTFMove(rawInitData), licenseType, WTFMove(rawCustomData));
    String sessionId = newSession->id();
    if (sessionId.isEmpty()) {
        generateChallenge(newSession.ptr());
        return;
    }
    GST_DEBUG("Created session with id %s", sessionId.utf8().data());
    newSession->generateChallenge(WTFMove(generateChallenge));

    if (!addSession(sessionId, WTFMove(newSession)))
        GST_WARNING("Failed to add session %s, the session might already exist, or the allocation failed", sessionId.utf8().data());
}

void CDMInstanceOpenCDM::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback callback)
{
    GST_TRACE("Updating session %s", sessionId.utf8().data());
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot update the session %s cause we can't find it", sessionId.utf8().data());
        callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        return;
    }

    session->update(reinterpret_cast<const uint8_t*>(response.data()), response.size(), [callback = WTFMove(callback)](Session* session, bool success, RefPtr<SharedBuffer>&& buffer, KeyStatusVector& keyStatuses) {
        if (success) {
            if (!buffer) {
                ASSERT(!keyStatuses.isEmpty());
                callback(false, copyAndMaybeReplaceValue(keyStatuses), std::nullopt, std::nullopt, SuccessValue::Succeeded);
            } else {
                // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
                // will even be other parts of the spec where not having structured data will be bad.
                std::optional<WebCore::MediaKeyMessageType> requestType;
                RefPtr<SharedBuffer> cleanMessage = parseResponseMessage(*buffer, requestType);
                if (cleanMessage) {
                    GST_DEBUG("got message of size %u", cleanMessage->size());
                    GST_MEMDUMP("message", reinterpret_cast<const uint8_t*>(cleanMessage->data()), cleanMessage->size());
                    callback(false, std::nullopt, std::nullopt, std::make_pair(requestType.value_or(MediaKeyMessageType::LicenseRequest), cleanMessage.releaseNonNull()), SuccessValue::Succeeded);
                } else {
                    GST_ERROR("message of size %u incorrectly formatted", buffer ? buffer->size() : 0);
                    callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
                }
            }
        } else {
            GST_ERROR("update license reported error state");
            callback(false, std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed);
        }
    });
}

void CDMInstanceOpenCDM::loadSession(LicenseType, const String& sessionId, const String&, LoadSessionCallback callback)
{
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot load the session %s cause we can't find it", sessionId.utf8().data());
        callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, SessionLoadFailure::NoSessionData);
        return;
    }
    session->load([callback = WTFMove(callback)](Session* session, bool success, RefPtr<SharedBuffer>&& buffer, KeyStatusVector& keyStatuses) {
        if (success) {
            if (!buffer)
                callback(copyAndMaybeReplaceValue(keyStatuses), std::nullopt, std::nullopt, SuccessValue::Succeeded, SessionLoadFailure::None);
            else {
                // FIXME: Using JSON reponse messages is much cleaner than using string prefixes, I believe there
                // will even be other parts of the spec where not having structured data will be bad.
                std::optional<WebCore::MediaKeyMessageType> requestType;
                RefPtr<SharedBuffer> cleanMessage = parseResponseMessage(*buffer, requestType);
                if (cleanMessage) {
                    GST_DEBUG("got message of size %u", cleanMessage->size());
                    GST_MEMDUMP("message", reinterpret_cast<const uint8_t*>(cleanMessage->data()), cleanMessage->size());
                    callback(std::nullopt, std::nullopt, std::make_pair(requestType.value_or(MediaKeyMessageType::LicenseRequest), cleanMessage.releaseNonNull()), SuccessValue::Succeeded, SessionLoadFailure::None);
                } else {
                    GST_ERROR("message of size %u incorrectly formatted", buffer ? buffer->size() : 0);
                    callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, SessionLoadFailure::Other);
                }
            }
        } else {
            auto bufferData = buffer ? buffer->data() : nullptr;
            auto bufferSize = buffer ? buffer->size() : 0;
            String response(StringImpl::createWithoutCopying(reinterpret_cast<const LChar*>(bufferData), bufferSize));
            GST_ERROR("session %s not loaded, reason %s", session->id().utf8().data(), response.utf8().data());
            callback(std::nullopt, std::nullopt, std::nullopt, SuccessValue::Failed, sessionLoadFailureFromOpenCDM(response));
        }
    });
}

void CDMInstanceOpenCDM::removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback callback)
{
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot remove non-existing session %s data", sessionId.utf8().data());
        callback(KeyStatusVector(), std::nullopt, SuccessValue::Failed);
        return;
    }

    session->remove([callback = WTFMove(callback)](Session* session, bool success, RefPtr<SharedBuffer>&& buffer, KeyStatusVector& keys) {
        if (success) {
            if (!buffer)
                callback(copyAndMaybeReplaceValue(keys, MediaKeyStatus::Released), std::nullopt, SuccessValue::Succeeded);
            else {
                std::optional<WebCore::MediaKeyMessageType> requestType;
                RefPtr<SharedBuffer> cleanMessage = buffer ? parseResponseMessage(*buffer, requestType) : nullptr;
                if (cleanMessage) {
                    GST_DEBUG("session %s removed, message length %u", session->id().utf8().data(), cleanMessage->size());
                    callback(copyAndMaybeReplaceValue(keys, MediaKeyStatus::Released), cleanMessage.releaseNonNull(), SuccessValue::Succeeded);
                } else {
                    GST_WARNING("message of size %u incorrectly formatted as session %s removal answer", buffer ? buffer->size() : 0, session->id().utf8().data());
                    callback(copyAndMaybeReplaceValue(keys, MediaKeyStatus::InternalError), std::nullopt, SuccessValue::Failed);
                }
            }
        } else {
            GST_WARNING("could not remove session %s", session->id().utf8().data());
            callback(copyAndMaybeReplaceValue(keys, MediaKeyStatus::InternalError), std::nullopt, SuccessValue::Failed);
        }
    });

#ifndef NDEBUG
    bool removeSessionResult =
#endif
        removeSession(sessionId);
    ASSERT(removeSessionResult);
}

void CDMInstanceOpenCDM::closeSession(const String& sessionId, CloseSessionCallback callback)
{
    auto session = lookupSession(sessionId);
    if (!session) {
        GST_WARNING("cannot close non-existing session %s", sessionId.utf8().data());
        return;
    }
    session->close();

#ifndef NDEBUG
    bool removeSessionResult =
#endif
        removeSession(sessionId);
    ASSERT(removeSessionResult);

    callback();
}

String CDMInstanceOpenCDM::sessionIdByKeyId(const SharedBuffer& keyId) const
{
    LockHolder locker(m_sessionMapMutex);

    GST_MEMDUMP("kid", reinterpret_cast<const uint8_t*>(keyId.data()), keyId.size());
    if (!m_sessionsMap.size() || !keyId.data()) {
        GST_WARNING("no sessions");
        return { };
    }

    String result;

    for (const auto& pair : m_sessionsMap) {
        const String& sessionId = pair.key;
        const RefPtr<Session>& session = pair.value;
        if (session->containsKeyId(keyId)) {
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

bool CDMInstanceOpenCDM::isKeyIdInSessionUsable(const SharedBuffer& keyId, const String& sessionId) const
{
    auto session = lookupSession(sessionId);
    return session && session->status(keyId) == Usable;
}

bool CDMInstanceOpenCDM::addSession(const String& sessionId, RefPtr<Session>&& session)
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
