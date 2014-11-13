/*
 * Copyright (C) 2014 Igalia S.L.
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

#include <DerivedSources/WebCore/WaylandWPEProtocolServer.h>
#include <WebKit/WKGeometry.h>
#include <glib.h>
#include <weston/compositor.h>

namespace WPE {

class View;

class Shell {
public:
    Shell(struct weston_compositor*);

    static Shell& instance() { return *m_instance; }
    static gpointer launchWPE(gpointer);

private:
    static const struct wl_wpe_interface m_wpeInterface;
    static const struct weston_pointer_grab_interface m_pgInterface;
    static const struct weston_keyboard_grab_interface m_kgInterface;

    static Shell* m_instance;

    struct weston_compositor* m_compositor;

    struct weston_layer m_layer;
    struct wl_list m_outputList;
    struct wl_listener m_outputCreatedListener;
    WKSize m_outputSize;

    // FIXME: Properly implement input handling.
    // InputEventHandler m_inputEventHandler;
    // View* m_focusedView;

    void createOutput(struct weston_output*);

    void registerSurface(struct weston_surface*);

    static void outputCreated(struct wl_listener*, void* data);
};

} // namespace WPE
