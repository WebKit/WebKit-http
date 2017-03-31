#pragma once

#if ENABLE(ENCRYPTED_MEDIA)

#include "CDM.h"

namespace WebCore {

class CDMFactoryClearKey : public CDMFactory {
public:
    CDMFactoryClearKey();
    virtual ~CDMFactoryClearKey();

    std::unique_ptr<CDMPrivate> createCDM(CDM&, const String&) override;
    bool supportsKeySystem(const String&) override;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
