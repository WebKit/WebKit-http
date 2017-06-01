#pragma once

#if ENABLE(ENCRYPTED_MEDIA)

#include <utility>
#include <wtf/Forward.h>

namespace WebCore {

class CDMInstance;
class SharedBuffer;

class CDMClient {
public:
    virtual ~CDMClient() { }

    virtual void cdmClientAttemptToResumePlaybackIfNecessary() = 0;
    virtual void cdmClientAttemptToDecryptWithInstance(const CDMInstance&) = 0;

#if USE(OCDM)
    virtual void receivedGenerateKeyRequest(String&) = 0;
    virtual void emitSession(String&) = 0;
#endif
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
