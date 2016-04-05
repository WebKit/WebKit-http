#ifndef BeaconLoader_h
#define BeaconLoader_h

#include "Frame.h"
#include "PingLoader.h"

namespace JSC {
class ArrayBufferView;
}

namespace WebCore {

class Frame;
class Blob;
class DOMFormData;

class WEBCORE_EXPORT BeaconLoader final : public RefCounted<BeaconLoader>, public PingLoader {
    WTF_MAKE_NONCOPYABLE(BeaconLoader);

public:
    virtual ~BeaconLoader() { }

    static bool sendBeacon(Frame&, int, const URL&, Blob*, int&);
    static bool sendBeacon(Frame&, int, const URL&, JSC::ArrayBufferView*, int&);
    static bool sendBeacon(Frame&, int, const URL&, DOMFormData*, int&);
    static bool sendBeacon(Frame&, int, const URL&, const String&, int&);

private:
    class Sender;

    BeaconLoader(Frame&, ResourceRequest&);
};

} // namespace WebCore

#endif // BeaconLoader_h
