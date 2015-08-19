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

#include <wayland-egl.h>
#include <EGL/egl.h>
#include <WPE/ViewBackend/Wayland/wayland-drm-client-protocol.h>
#include <WPE/ViewBackend/Wayland/xdg-shell-client-protocol.h>

namespace WPE {

namespace ViewBackend {

class WaylandDisplay {
public:
    static WaylandDisplay& singleton();

    struct wl_display* display() const { return m_display; }
    struct wl_compositor* compositor() const { return m_compositor; }
    struct wl_drm* drm() const { return m_drm; }
    struct xdg_shell* xdg() const { return m_xdg; }

    EGLDisplay eglDisplay() const { return m_eglDisplay; }

private:
    WaylandDisplay();

    static const struct wl_registry_listener m_registryListener;
    static const struct xdg_shell_listener m_xdgShellListener;

    struct wl_display* m_display;
    struct wl_registry* m_registry;
    struct wl_compositor* m_compositor;
    struct wl_drm* m_drm;
    struct xdg_shell* m_xdg;

    EGLDisplay m_eglDisplay;
};

} // namespace ViewBackend

} // namespace WPE

#endif // ViewBackend_Wayland_WaylandDisplay_h
