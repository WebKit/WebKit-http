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

#include "config.h"
#include "Shell.h"

#include "Environment.h"
#include <WebKit/WKContextConfigurationRef.h>
#include <WebKit/WKContext.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>

namespace WPE {

Shell::Shell(Environment& environment)
    : m_environment(environment)
{
    auto* compositor = m_environment.compositor();
    weston_compositor_set_default_pointer_grab(compositor, &m_pgInterface);
    weston_compositor_set_default_keyboard_grab(compositor, &m_kgInterface);
    weston_compositor_set_default_touch_grab(compositor, &m_tgInterface);
}

const struct weston_pointer_grab_interface Shell::m_pgInterface = {
    // focus
    [](struct weston_pointer_grab*) { },

    // motion
    [](struct weston_pointer_grab* grab, uint32_t time, wl_fixed_t x, wl_fixed_t y)
    {
        struct weston_pointer* pointer = grab->pointer;
        weston_pointer_move(pointer, x, y);

        int newX = wl_fixed_to_int(pointer->x);
        int newY = wl_fixed_to_int(pointer->y);

        Shell::instance().m_environment.updateCursorPosition(newX, newY);
        WKInputHandlerNotifyPointerMotion(Shell::instance().m_inputHandler.get(),
            WKPointerMotion{ time, newX, newY });
    },

    // button
    [](struct weston_pointer_grab* grab, uint32_t time, uint32_t button, uint32_t state)
    {
        WKInputHandlerNotifyPointerButton(Shell::instance().m_inputHandler.get(),
            WKPointerButton{ time, button, state });
    },

    // axis
    [](struct weston_pointer_grab* grab, uint32_t time, uint32_t axis, wl_fixed_t value)
    {
        // Multiply the delta value by -4 to turn it into a more acceptable direction,
        // and to increase the pixel step into something more ... efficient.
        WKInputHandlerNotifyAxisMotion(Shell::instance().m_inputHandler.get(),
            WKAxisMotion{ time, axis, -4 * wl_fixed_to_int(value) });
    },

    // cancel
    [](struct weston_pointer_grab*) { }
};

const struct weston_keyboard_grab_interface Shell::m_kgInterface = {
    // key
    [](struct weston_keyboard_grab*, uint32_t time, uint32_t key, uint32_t state)
    {
        WKInputHandlerNotifyKeyboardKey(Shell::instance().m_inputHandler.get(),
            WKKeyboardKey{ time, key, state });
    },

    // modifiers
    [](struct weston_keyboard_grab*, uint32_t serial, uint32_t depressed,
        uint32_t latched, uint32_t locked, uint32_t group)
    { },

    // cancel
    [](struct weston_keyboard_grab*) { }
};

const struct weston_touch_grab_interface Shell::m_tgInterface = {
    // down
    [](struct weston_touch_grab*, uint32_t time, int id, wl_fixed_t x, wl_fixed_t y)
    {
        WKInputHandlerNotifyTouchDown(Shell::instance().m_inputHandler.get(),
            WKTouchDown{ time, id, x, y});
    },
    // up
    [](struct weston_touch_grab*, uint32_t time, int id)
    {
        WKInputHandlerNotifyTouchUp(Shell::instance().m_inputHandler.get(),
            WKTouchUp{ time, id});
    },
    // motion
    [](struct weston_touch_grab*, uint32_t time, int id, wl_fixed_t x, wl_fixed_t y)
    {
        WKInputHandlerNotifyTouchMotion(Shell::instance().m_inputHandler.get(),
            WKTouchMotion{ time, id, x, y });
    },
    // frame
    [](struct weston_touch_grab*) { },
    // cancel
    [](struct weston_touch_grab*) { }
};

Shell* Shell::m_instance = nullptr;

gpointer Shell::launchWPE(gpointer data)
{
    Shell::m_instance = static_cast<Shell*>(data);

    GMainContext* threadContext = g_main_context_new();
    GMainLoop* threadLoop = g_main_loop_new(threadContext, FALSE);

    g_main_context_push_thread_default(threadContext);

    auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
    auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));

    auto context = adoptWK(WKContextCreate());

    Shell::instance().m_view = adoptWK(WKViewCreate(context.get(), pageGroup.get()));
    auto view = Shell::instance().m_view.get();
    WKViewResize(view, Shell::instance().m_environment.outputSize());

    Shell::instance().m_inputHandler = adoptWK(WKInputHandlerCreate(view));

    const char* url = g_getenv("WPE_SHELL_URL");
    if (!url)
        url = "http://www.webkit.org/blog-files/3d-transforms/poster-circle.html";
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(view), shellURL.get());

    g_main_loop_run(threadLoop);
    return nullptr;
}

} // namespace WPE
