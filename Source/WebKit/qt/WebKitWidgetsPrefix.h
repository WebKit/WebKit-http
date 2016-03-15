#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif

#include <PlatformExportMacros.h>
#include <wtf/Assertions.h>

#undef WEBCORE_EXPORT
#define WEBCORE_EXPORT
