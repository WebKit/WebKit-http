#pragma once

#if ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)

#include "CDM.h"

namespace WebCore {

class CDMFactoryPlayReady : public CDMFactory {
public:
    CDMFactoryPlayReady();
    virtual ~CDMFactoryPlayReady();

    std::unique_ptr<CDMPrivate> createCDM(CDM&, const String&) override;
    bool supportsKeySystem(const String&) override;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA) && USE(PLAYREADY)
