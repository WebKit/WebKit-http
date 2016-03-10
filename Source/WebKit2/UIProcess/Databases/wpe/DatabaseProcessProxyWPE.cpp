#include "config.h"
#include "DatabaseProcessProxy.h"

#if ENABLE(DATABASE_PROCESS)

namespace WebKit {

void DatabaseProcessProxy::platformGetLaunchOptions(ProcessLauncher::LaunchOptions&)
{
}

} // namespace WebKit

#endif // ENABLE(DATABASE_PROCESS)
