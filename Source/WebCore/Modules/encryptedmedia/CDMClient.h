#pragma once

#if ENABLE(ENCRYPTED_MEDIA)

#include <utility>
#include <wtf/Forward.h>

namespace WebCore {

class SharedBuffer;

class CDMClient {
public:
    virtual ~CDMClient() { }

    virtual void cdmClientAttemptToResumePlaybackIfNecessary() = 0;
    virtual void cdmClientAttemptToDecryptWithKeys(const Vector<std::pair<Ref<SharedBuffer>, Ref<SharedBuffer>>>&) = 0;
};

} // namespace WebCore

#endif // ENABLE(ENCRYPTED_MEDIA)
