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

    virtual void cdmClientInstanceAttached(const CDMInstance&) = 0;
    virtual void cdmClientInstanceDetached(const CDMInstance&) = 0;

    virtual void cdmClientAttemptToResumePlaybackIfNecessary() = 0;
    virtual void cdmClientAttemptToDecryptWithInstance(const CDMInstance&) = 0;

#if USE(OPENCDM)
    virtual void emitSession(const String&) = 0;
#endif
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
