#ifndef wpe_input_h
#define wpe_input_h

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum wpe_input_keyboard_modifier {
    wpe_input_keyboard_modifier_control = 1 << 0,
    wpe_input_keyboard_modifier_shift   = 1 << 1,
    wpe_input_keyboard_modifier_alt     = 1 << 2,
    wpe_input_keyboard_modifier_meta    = 1 << 3,
};

struct wpe_input_keyboard_event {
    uint32_t time;
    uint32_t keyCode;
    uint32_t unicode;
    bool pressed;
    uint8_t modifiers;
};


enum wpe_input_pointer_event_type {
    wpe_input_pointer_event_type_null,
    wpe_input_pointer_event_type_motion,
    wpe_input_pointer_event_type_button,
};

struct wpe_input_pointer_event {
    enum wpe_input_pointer_event_type type;
    uint32_t time;
    int x;
    int y;
    uint32_t button;
    uint32_t state;
};


enum wpe_input_axis_event_type {
    wpe_input_axis_event_type_null,
    wpe_input_axis_event_type_motion,
};

struct wpe_input_axis_event {
    enum wpe_input_axis_event_type type;
    uint32_t time;
    int x;
    int y;
    uint32_t axis;
    int32_t value;
};


enum wpe_input_touch_event_type {
    wpe_input_touch_event_type_null,
    wpe_input_touch_event_type_down,
    wpe_input_touch_event_type_motion,
    wpe_input_touch_event_type_up,
};

struct wpe_input_touch_event_raw {
    enum wpe_input_touch_event_type type;
    uint32_t time;
    int id;
    int32_t x;
    int32_t y;
};

struct wpe_input_touch_event {
    const struct wpe_input_touch_event_raw* touchpoints;
    uint64_t touchpoints_length;
    enum wpe_input_touch_event_type type;
    int32_t id;
    uint32_t time;
};


const char*
wpe_input_identifier_for_key_event(struct wpe_input_keyboard_event*);

int
wpe_input_windows_key_code_for_key_event(struct wpe_input_keyboard_event*);

const char*
wpe_input_single_character_for_key_event(struct wpe_input_keyboard_event*);

#ifdef __cplusplus
}
#endif

#endif // wpe_input_h
