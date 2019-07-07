#if defined(HAVE_CONFIG_H) && HAVE_CONFIG_H && defined(BUILDING_WITH_CMAKE)
#include "cmakeconfig.h"
#endif


#if OS(WINDOWS)

#ifndef WEBCORE_EXPORT
#define WEBCORE_EXPORT WTF_IMPORT_DECLARATION
#endif // !WEBCORE_EXPORT

#endif
