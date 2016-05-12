#ifndef NavigatorBeacon_h
#define NavigatorBeacon_h

#include "DOMWindowProperty.h"
#include "Supplementable.h"
#include "ExceptionCode.h"
#include "ScriptExecutionContext.h"

namespace JSC {
class ArrayBufferView;
}

namespace WebCore {

class Navigator;
class ScriptExecutionContext;
class Blob;
class DOMFormData;

class NavigatorBeacon : public Supplement<Navigator>, public DOMWindowProperty {

public:
    NavigatorBeacon(Frame*);
    virtual ~NavigatorBeacon();

    static NavigatorBeacon& from(Navigator&);

    static bool sendBeacon(Navigator&, ScriptExecutionContext&, const String&, Blob*, ExceptionCode&);
    static bool sendBeacon(Navigator&, ScriptExecutionContext&, const String&, const RefPtr<JSC::ArrayBufferView>&, ExceptionCode&);
    static bool sendBeacon(Navigator&, ScriptExecutionContext&, const String&, DOMFormData*, ExceptionCode&);
    static bool sendBeacon(Navigator&, ScriptExecutionContext&, const String&, const String&, ExceptionCode&);

private:
    static const char* supplementName();

    bool canSendBeacon(ScriptExecutionContext&, const URL&, ExceptionCode&);
    int maxAllowance() const;
    bool beaconResult(ScriptExecutionContext& context, bool allowed, int sentBytes);

    int m_transmittedBytes;
};

} // namespace WebCore

#endif // NavigatorBeacon_h
