#ifndef wpe_metrological_input_linuxinput_h
#define wpe_metrological_input_linuxinput_h

#ifdef KEY_INPUT_HANDLING_LINUX_INPUT

#include <wpe/input.h>

#ifdef __cplusplus
extern "C" {
#endif

extern struct wpe_input_key_mapper_interface linuxinput_input_key_mapper_interface;

#ifdef __cplusplus
}
#endif

#endif // KEY_INPUT_HANDLING_LINUX_INPUT

#endif // wpe_metrological_input_linuxinput_h
