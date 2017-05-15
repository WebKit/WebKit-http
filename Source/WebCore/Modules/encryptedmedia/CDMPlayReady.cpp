#include "config.h"
#include "CDMPlayReady.h"

#if ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)

#include "CDMInstancePlayReady.h"
#include "CDMPrivate.h"
#include <wtf/UUID.h>

namespace WebCore {

class CDMPrivatePlayReady : public CDMPrivate {
public:
    CDMPrivatePlayReady();
    virtual ~CDMPrivatePlayReady();

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

// CDMFactoryPlayReady

CDMFactoryPlayReady::CDMFactoryPlayReady() = default;
CDMFactoryPlayReady::~CDMFactoryPlayReady() = default;

std::unique_ptr<CDMPrivate> CDMFactoryPlayReady::createCDM(CDM&, const String&)
{
    return std::unique_ptr<CDMPrivate>(new CDMPrivatePlayReady);
}

bool CDMFactoryPlayReady::supportsKeySystem(const String& keySystem)
{
    return equalLettersIgnoringASCIICase(keySystem, "com.microsoft.playready") 
        || equalLettersIgnoringASCIICase(keySystem, "com.youtube.playready");
}

// CDMPrivatePlayReady

CDMPrivatePlayReady::CDMPrivatePlayReady() = default;
CDMPrivatePlayReady::~CDMPrivatePlayReady() = default;

bool CDMPrivatePlayReady::supportsInitDataType(const AtomicString& initDataType) const
{
    return equalLettersIgnoringASCIICase(initDataType, "cenc");
}

bool CDMPrivatePlayReady::supportsConfiguration(const MediaKeySystemConfiguration&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return true;
}

bool CDMPrivatePlayReady::supportsConfigurationWithRestrictions(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return true;
}

bool CDMPrivatePlayReady::supportsSessionTypeWithConfiguration(MediaKeySessionType&, const MediaKeySystemConfiguration&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return true;
}

bool CDMPrivatePlayReady::supportsRobustness(const String&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return false;
}

MediaKeysRequirement CDMPrivatePlayReady::distinctiveIdentifiersRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return MediaKeysRequirement::Optional;
}

MediaKeysRequirement CDMPrivatePlayReady::persistentStateRequirement(const MediaKeySystemConfiguration&, const MediaKeysRestrictions&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return MediaKeysRequirement::Optional;
}

bool CDMPrivatePlayReady::distinctiveIdentifiersAreUniquePerOriginAndClearable(const MediaKeySystemConfiguration&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return false;
}

RefPtr<CDMInstance> CDMPrivatePlayReady::createInstance()
{
    return adoptRef(new CDMInstancePlayReady);
}

void CDMPrivatePlayReady::loadAndInitialize()
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
}

bool CDMPrivatePlayReady::supportsServerCertificates() const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return false;
}

bool CDMPrivatePlayReady::supportsSessions() const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return true;
}

bool CDMPrivatePlayReady::supportsInitData(const AtomicString& initDataType, const SharedBuffer&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return supportsInitDataType(initDataType);
}

RefPtr<SharedBuffer> CDMPrivatePlayReady::sanitizeResponse(const SharedBuffer& response) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return response.copy();
}

std::optional<String> CDMPrivatePlayReady::sanitizeSessionId(const String&) const
{
    fprintf(stderr, "NotImplemented: CDMPrivatePlayReady::%s()\n", __func__);
    return { };
}

// CDMInstancePlayReady

CDMInstancePlayReady::CDMInstancePlayReady()
    : m_prSession(std::make_unique<PlayreadySession>())
{
}

CDMInstancePlayReady::~CDMInstancePlayReady() = default;

CDMInstance::SuccessValue CDMInstancePlayReady::initializeWithConfiguration(const MediaKeySystemConfiguration&)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstancePlayReady::setDistinctiveIdentifiersAllowed(bool)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstancePlayReady::setPersistentStateAllowed(bool)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
    return Succeeded;
}

CDMInstance::SuccessValue CDMInstancePlayReady::setServerCertificate(Ref<SharedBuffer>&&)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
    return Failed;
}

void CDMInstancePlayReady::requestLicense(LicenseType, const AtomicString& initDataType, Ref<SharedBuffer>&& initData, LicenseCallback callback)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
    callback(initData->copy(), createCanonicalUUIDString(), false, Succeeded);
}

void CDMInstancePlayReady::updateLicense(const String& sessionId, LicenseType, const SharedBuffer& response, LicenseUpdateCallback)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
}

void CDMInstancePlayReady::loadSession(LicenseType, const String& sessionId, const String& origin, LoadSessionCallback)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
}

void CDMInstancePlayReady::closeSession(const String& sessionId, CloseSessionCallback)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
}

void CDMInstancePlayReady::removeSessionData(const String& sessionId, LicenseType, RemoveSessionDataCallback)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
}

void CDMInstancePlayReady::storeRecordOfKeyUsage(const String& sessionId)
{
    fprintf(stderr, "NotImplemented: CDMInstancePlayReady::%s()\n", __func__);
}

void CDMInstancePlayReady::gatherAvailableKeys(AvailableKeysCallback)
{
}

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)
