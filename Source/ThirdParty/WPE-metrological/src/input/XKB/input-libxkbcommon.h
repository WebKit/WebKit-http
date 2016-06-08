#ifndef wpe_metrological_input_libxkbcommon_h
#define wpe_metrological_input_libxkbcommon_h

#ifdef KEY_INPUT_HANDLING_XKB

#include <wpe/input.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_input_key_mapper_interface libxkbcommon_input_key_mapper_interface;

#ifdef __cplusplus
}
#endif

#endif // KEY_INPUT_HANDLING_XKB

#endif // wpe_metrological_input_libxkbcommon_h
