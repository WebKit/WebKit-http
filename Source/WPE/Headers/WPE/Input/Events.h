/*
 * Copyright (C) 2014 Igalia S.L.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WPE_Input_Events_h
#define WPE_Input_Events_h

#include <array>

namespace IPC {
    class ArgumentDecoder;
    class ArgumentEncoder;
}

namespace WPE {

namespace Input {

struct KeyboardEvent {
    struct Raw {
        uint32_t time;
        uint32_t key;
        uint32_t state;
    };

    enum Modifiers {
        Control = 1 << 0,
        Shift   = 1 << 1,
        Alt     = 1 << 2,
        Meta    = 1 << 3
    };

    uint32_t time;
    uint32_t keyCode;
    uint32_t unicode;
    bool pressed;
    uint8_t modifiers;

    KeyboardEvent(){}
    KeyboardEvent(uint32_t in_time, uint32_t in_keyCode, uint32_t in_unicode, bool in_pressed, uint8_t in_modifiers)
    : time(in_time)
    , keyCode(in_keyCode)
    , unicode(in_unicode)
    , pressed(in_pressed)
    , modifiers(in_modifiers)
    {
    }

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, KeyboardEvent&);
};

struct PointerEvent {
    enum Type : uint32_t {
        Null,
        Motion,
        Button
    };

    struct Raw {
        Type type;
        uint32_t time;
        int x;
        int y;
        uint32_t button;
        uint32_t state;
    };

    uint32_t type;
    uint32_t time;
    int x;
    int y;
    uint32_t button;
    uint32_t state;
    PointerEvent() {}
    PointerEvent(uint32_t in_type, uint32_t in_time, int in_x, int in_y, uint32_t in_button, uint32_t in_state)
    : type(in_type)
    , time(in_time)
    , x(in_x)
    , y(in_y)
    , button(in_button)
    , state(in_state)
    {
    }

    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, PointerEvent&);
};

struct AxisEvent {
    enum Type : uint32_t {
        Null,
        Motion
    };

    struct Raw {
        Type type;
        uint32_t time;
        uint32_t axis;
        int32_t value;
    };

    uint32_t type;
    uint32_t time;
    int x;
    int y;
    uint32_t axis;
    int32_t value;
    AxisEvent() {}
    AxisEvent(uint32_t in_type, uint32_t in_time, int in_x, int in_y, uint32_t in_axis, int32_t in_value)
    : type(in_type)
    , time(in_time)
    , x(in_x)
    , y(in_y)
    , axis(in_axis)
    , value(in_value)
    {
    }
    void encode(IPC::ArgumentEncoder&) const;
    static bool decode(IPC::ArgumentDecoder&, AxisEvent&);
};

struct TouchEvent {
    enum Type : uint32_t {
        Null,
        Down,
        Motion,
        Up
    };

    struct Raw {
        Type type;
        uint32_t time;
        int id;
        int32_t x;
        int32_t y;
    };

    const std::array<Raw, 10>& touchPoints;
    Type type;
    int32_t id;
    uint32_t time;
};

} // namespace Input

} // namespace WPE

#endif // WPE_Input_Events_h
