/*
 * Copyright (C) 2015 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ViewBackend_Wayland_WaylandDisplay_h
#define ViewBackend_Wayland_WaylandDisplay_h

#if WPE_BACKEND(WAYLAND)

struct wl_compositor;
struct wl_display;
struct wl_drm;
struct wl_registry;
struct wl_seat;
struct xdg_shell;

typedef struct _GSource GSource;

namespace WPE {

namespace ViewBackend {

class WaylandDisplay {
public:
    static const WaylandDisplay& singleton();

    struct wl_display* display() const { return m_display; }

    struct Interfaces {
        struct wl_compositor* compositor;
        struct wl_drm* drm;
        struct wl_seat* seat;
        struct xdg_shell* xdg;
    };
    const Interfaces& interfaces() const { return m_interfaces; }

private:
    WaylandDisplay();
    ~WaylandDisplay();

    struct wl_display* m_display;
    struct wl_registry* m_registry;
    Interfaces m_interfaces;

    GSource* m_eventSource;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(WAYLAND)

#endif // ViewBackend_Wayland_WaylandDisplay_h
