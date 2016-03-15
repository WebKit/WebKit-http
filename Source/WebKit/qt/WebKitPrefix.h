#include "cmakeconfig.h"
#include <wtf/Assertions.h>
#include <WebCore/PlatformExportMacros.h>

// TODO: Define in static builds only?
#if OS(WINDOWS)
#define WEBCORE_EXPORT
#endif
