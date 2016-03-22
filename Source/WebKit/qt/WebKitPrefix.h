#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif

#include "config.h"

#if OS(WINDOWS)
#undef WEBCORE_EXPORT
#define WEBCORE_EXPORT WTF_EXPORT_DECLARATION
#endif
