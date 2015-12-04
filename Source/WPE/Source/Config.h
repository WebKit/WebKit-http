#ifndef WPE_Config_h
#define WPE_Config_h

#include <cstddef>

#define WPE_BACKEND(BACKEND) (defined WPE_BACKEND_##BACKEND && WPE_BACKEND_##BACKEND)
#define WPE_BUFFER_MANAGEMENT(BACKEND) (defined WPE_BUFFER_MANAGEMENT_##BACKEND && WPE_BUFFER_MANAGEMENT_##BACKEND)

#endif // WPE_Config_h
