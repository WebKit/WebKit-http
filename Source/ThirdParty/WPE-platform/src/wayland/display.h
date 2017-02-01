/*
 * Copyright (C) 2015, 2016 Igalia S.L.
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

#ifndef wpe_view_backend_wayland_display_h
#define wpe_view_backend_wayland_display_h

#include <array>
#include <unordered_map>
#include <utility>
#include <wpe/input.h>
#include <xkbcommon/xkbcommon-compose.h>
#include <xkbcommon/xkbcommon.h>
#include "ipc.h"

struct wpe_view_backend;

struct wl_compositor;
struct wl_display;
struct wl_keyboard;
#ifdef BACKEND_BCM_NEXUS_WAYLAND
struct wl_nsc;
#endif
struct wl_pointer;
struct wl_registry;
struct wl_seat;
struct wl_surface;
struct wl_touch;
struct xdg_shell;
struct wl_shell;

typedef struct _GSource GSource;

namespace Wayland {

class EventDispatcher
{
public:
    static EventDispatcher& singleton();
    void sendEvent( wpe_input_axis_event& event );
    void sendEvent( wpe_input_pointer_event& event );
    void sendEvent( wpe_input_touch_event& event );
    void sendEvent( wpe_input_keyboard_event& event );
    void setIPC( IPC::Client& ipcClient );
    enum MsgType
    {
	AXIS = 0x30,
	POINTER,
	TOUCH,
	KEYBOARD
    };
private:
    EventDispatcher() {};
    ~EventDispatcher() {};
    IPC::Client * m_ipc;
};

class Display {
public:
    static Display& singleton();

    struct wl_display* display() const { return m_display; }

    uint32_t serial() const { return m_seatData.serial; }

    struct Interfaces {
        struct wl_compositor* compositor;
#ifdef BACKEND_BCM_NEXUS_WAYLAND
        struct wl_nsc* nsc;
#endif
        struct wl_seat* seat;
        struct xdg_shell* xdg;
        struct wl_shell* shell;
    };
    const Interfaces& interfaces() const { return m_interfaces; }

    struct SeatData {
        std::unordered_map<struct wl_surface*, struct wpe_view_backend*> inputClients;

        struct {
            struct wl_pointer* object;
            std::pair<struct wl_surface*, struct wpe_view_backend*> target;
            std::pair<int, int> coords;
            uint32_t button;
            uint32_t state;
        } pointer { nullptr, { }, { 0, 0 }, 0, 0 };
        struct {
            struct wl_keyboard* object;
            std::pair<struct wl_surface*, struct wpe_view_backend*> target;
        } keyboard { nullptr, { } };
        struct {
            struct wl_touch* object;
            std::array<std::pair<struct wl_surface*, struct wpe_view_backend*>, 10> targets;
            std::array<struct wpe_input_touch_event_raw, 10> touchPoints;
        } touch { nullptr, { }, { } };

        struct {
            struct xkb_context* context;
            struct xkb_keymap* keymap;
            struct xkb_state* state;
            struct {
                xkb_mod_index_t control;
                xkb_mod_index_t alt;
                xkb_mod_index_t shift;
            } indexes;
            uint8_t modifiers;
            struct xkb_compose_table* composeTable;
            struct xkb_compose_state* composeState;
        } xkb { nullptr, nullptr, nullptr, { 0, 0, 0 }, 0, nullptr, nullptr };

        struct {
            int32_t rate;
            int32_t delay;
        } repeatInfo { 0, 0 };

        struct {
            uint32_t key;
            uint32_t time;
            uint32_t state;
            uint32_t eventSource;
        } repeatData { 0, 0, 0, 0 };

        uint32_t serial;
    };

    void registerInputClient(struct wl_surface*, struct wpe_view_backend*);
    void unregisterInputClient(struct wl_surface*);

private:
    Display();
    ~Display();

    struct wl_display* m_display;
    struct wl_registry* m_registry;
    Interfaces m_interfaces;

    SeatData m_seatData;

    GSource* m_eventSource;
};

} // namespace Wayland

#endif // wpe_view_backend_wayland_display_h
