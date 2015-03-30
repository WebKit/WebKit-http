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
#include "AtholShell.h"

#include <WPE/Input/Handling.h>
#include <WebKit/WKContextConfigurationRef.h>
#include <WebKit/WKContext.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>

namespace WPE {

class InputClient final : public API::InputClient {
public:
    InputClient(AtholShell&);

private:
    virtual void handleKeyboardEvent(uint32_t, uint32_t, uint32_t) override;
    virtual void handlePointerMotion(uint32_t, double, double) override;
    virtual void handlePointerButton(uint32_t, uint32_t, uint32_t) override;

    AtholShell& m_shell;
};

InputClient::InputClient(AtholShell& shell)
    : m_shell(shell)
{
}

void InputClient::handleKeyboardEvent(uint32_t time, uint32_t key, uint32_t state)
{
    WPE::Input::Server::singleton().serveKeyboardEvent({ time, key, state });
}

void InputClient::handlePointerMotion(uint32_t time, double dx, double dy)
{
    WPE::Input::Server::singleton().servePointerEvent({
        WPE::Input::PointerEvent::Motion,
        time, static_cast<int>(dx), static_cast<int>(dy), 0, 0
    });
}

void InputClient::handlePointerButton(uint32_t time, uint32_t button, uint32_t state)
{
    WPE::Input::Server::singleton().servePointerEvent({
        WPE::Input::PointerEvent::Button,
        time, 0, 0, button, state
    });
}

AtholShell::AtholShell(API::Compositor* compositor)
    : m_compositor(compositor)
{
    wl_global_create(m_compositor->display(),
        &wl_wpe_interface, wl_wpe_interface.version, this,
        [](struct wl_client* client, void* data, uint32_t version, uint32_t id) {
            ASSERT(version == wl_wpe_interface.version);
            auto* shell = static_cast<AtholShell*>(data);
            struct wl_resource* resource = wl_resource_create(client, &wl_wpe_interface, version, id);
            wl_resource_set_implementation(resource, &m_wpeInterface, shell, nullptr);
        });

    m_compositor->initializeInput(std::make_unique<InputClient>(*this));
}

const struct wl_wpe_interface AtholShell::m_wpeInterface = {
    // register_surface
    [](struct wl_client*, struct wl_resource* shellResource, struct wl_resource* surfaceResource) { }
};

gpointer AtholShell::launchWPE(gpointer data)
{
    auto& shell = *static_cast<AtholShell*>(data);

    GMainContext* threadContext = g_main_context_new();
    GMainLoop* threadLoop = g_main_loop_new(threadContext, FALSE);

    g_main_context_push_thread_default(threadContext);

    auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
    auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));

    auto context = adoptWK(WKContextCreate());

    shell.m_view = adoptWK(WKViewCreate(context.get(), pageGroup.get()));
    auto* view = shell.m_view.get();
    WKViewResize(view, WKSizeMake(1280, 720));
    WKViewMakeWPEInputTarget(view);

    const char* url = g_getenv("WPE_SHELL_URL");
    if (!url)
        url = "https://www.youtube.com/tv/";
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(view), shellURL.get());

    g_main_loop_run(threadLoop);
    return nullptr;
}

} // namespace WPE
