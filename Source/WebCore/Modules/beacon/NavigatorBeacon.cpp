#include "config.h"
#include "NavigatorBeacon.h"

#include "Navigator.h"
#include "BeaconLoader.h"
#include "runtime/ArrayBufferView.h"
#include "DOMFormData.h"
#include "ContentSecurityPolicy.h"

namespace WebCore {

NavigatorBeacon::NavigatorBeacon(Frame* frame)
: DOMWindowProperty(frame)
{
}

NavigatorBeacon::~NavigatorBeacon()
{
}

const char* NavigatorBeacon::supplementName()
{
    return "NavigatorBeacon";
}

NavigatorBeacon& NavigatorBeacon::from(Navigator& navigator)
{
    NavigatorBeacon* supplement = static_cast<NavigatorBeacon*>(Supplement<Navigator>::from(&navigator, supplementName()));
    if (!supplement) {
        auto newSupplement = std::make_unique<NavigatorBeacon>(navigator.frame());
        supplement = newSupplement.get();
        provideTo(&navigator, supplementName(), WTFMove(newSupplement));
    }
    return *supplement;
}

bool NavigatorBeacon::canSendBeacon(ScriptExecutionContext& context, const URL& url, ExceptionCode& ec)
{
    if (!url.isValid()) {
        ec = SYNTAX_ERR;
        return false;
    }

    // For now, only support HTTP and related.
    if (!url.protocolIsInHTTPFamily()) {
        ec = SECURITY_ERR;
        return false;
    }

    if (!context.shouldBypassMainWorldContentSecurityPolicy() && !context.contentSecurityPolicy()->allowConnectToSource(url)) {
        ec = SECURITY_ERR;
        return false;
    }

    // If detached from frame, do not allow sending a Beacon.
    if (!frame())
        return false;

    return true;
}

int NavigatorBeacon::maxAllowance() const
{
    return m_transmittedBytes;
}

bool NavigatorBeacon::beaconResult(ScriptExecutionContext&, bool allowed, int sentBytes)
{
    if (allowed) {
        ASSERT(sentBytes >= 0);
        m_transmittedBytes += sentBytes;
    }
    return allowed;
}

bool NavigatorBeacon::sendBeacon(Navigator& navigator, ScriptExecutionContext& context, const String& urlstring, Blob* data, ExceptionCode& ec)
{
    NavigatorBeacon& impl = NavigatorBeacon::from(navigator);

    URL url = context.completeURL(urlstring);
    if (!impl.canSendBeacon(context, url, ec))
        return false;

    int allowance = impl.maxAllowance();
    int bytes = 0;
    bool allowed = BeaconLoader::sendBeacon(*impl.frame(), allowance, url, data, bytes);

    return impl.beaconResult(context, allowed, bytes);
}

bool NavigatorBeacon::sendBeacon(Navigator& navigator, ScriptExecutionContext& context, const String& urlstring, const RefPtr<JSC::ArrayBufferView>& data, ExceptionCode& ec)
{
    NavigatorBeacon& impl = NavigatorBeacon::from(navigator);

    URL url = context.completeURL(urlstring);
    if (!impl.canSendBeacon(context, url, ec))
        return false;

    int allowance = impl.maxAllowance();
    int bytes = 0;
    bool allowed = BeaconLoader::sendBeacon(*impl.frame(), allowance, url, data.get(), bytes);

    return impl.beaconResult(context, allowed, bytes);
}

bool NavigatorBeacon::sendBeacon(Navigator& navigator, ScriptExecutionContext& context, const String& urlstring, DOMFormData* data, ExceptionCode& ec)
{
    NavigatorBeacon& impl = NavigatorBeacon::from(navigator);

    URL url = context.completeURL(urlstring);
    if (!impl.canSendBeacon(context, url, ec))
        return false;

    int allowance = impl.maxAllowance();
    int bytes = 0;
    bool allowed = BeaconLoader::sendBeacon(*impl.frame(), allowance, url, data, bytes);

    return impl.beaconResult(context, allowed, bytes);
}

bool NavigatorBeacon::sendBeacon(Navigator& navigator, ScriptExecutionContext& context, const String& urlstring, const String& data, ExceptionCode& ec)
{
    NavigatorBeacon& impl = NavigatorBeacon::from(navigator);

    URL url = context.completeURL(urlstring);
    if (!impl.canSendBeacon(context, url, ec))
        return false;

    int allowance = impl.maxAllowance();
    int bytes = 0;
    bool allowed = BeaconLoader::sendBeacon(*impl.frame(), allowance, url, data, bytes);

    return impl.beaconResult(context, allowed, bytes);
}

} // namespace WebCore
