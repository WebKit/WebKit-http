#pragma once

#if ENABLE(ENCRYPTED_MEDIA)

#include "CDM.h"
#include "CDMInstance.h"

namespace WebCore {

class CDMFactoryClearKey : public CDMFactory {
public:
    CDMFactoryClearKey();
    virtual ~CDMFactoryClearKey();

    std::unique_ptr<CDMPrivate> createCDM(CDM&, const String&) override;
    bool supportsKeySystem(const String&) override;
};

class CDMInstanceClearKey;

} // namespace WebCore


SPECIALIZE_TYPE_TRAITS_CDM_INSTANCE(WebCore::CDMInstanceClearKey, WebCore::CDMInstance::ImplementationType::ClearKey);

#endif // ENABLE(ENCRYPTED_MEDIA)
