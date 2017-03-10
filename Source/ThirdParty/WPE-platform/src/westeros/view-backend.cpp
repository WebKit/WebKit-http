/*
 * Copyright (C) 2015, 2016 Igalia S.L.
 * Copyright (C) 2015, 2016 Metrological
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

#include <wpe/view-backend.h>

#include "WesterosViewbackendInput.h"
#include "WesterosViewbackendOutput.h"
#include <cstdio>
#include <cstdlib>
#include <westeros-compositor.h>

namespace Westeros {

struct ViewBackend
{
    ViewBackend(struct wpe_view_backend*);
    virtual ~ViewBackend();

    void initialize();

    struct wpe_view_backend* backend;
    WstCompositor* compositor;

    WesterosViewbackendInput* input_handler;
    WesterosViewbackendOutput* output_handler;
};

ViewBackend::ViewBackend(struct wpe_view_backend* backend)
    : backend(backend)
    , input_handler(nullptr)
    , output_handler(nullptr)
{
    compositor = WstCompositorCreate();
    if (!compositor)
        return;

    input_handler = new WesterosViewbackendInput(backend);
    output_handler = new WesterosViewbackendOutput(backend);
    const char* nestedTargetDisplay = std::getenv("WAYLAND_DISPLAY");
    if (nestedTargetDisplay)
    {
        fprintf(stderr, "ViewBackendWesteros: running as the nested compositor\n");
        WstCompositorSetIsNested(compositor, true);
        WstCompositorSetIsRepeater(compositor, true);
        WstCompositorSetNestedDisplayName(compositor, nestedTargetDisplay);
        //Register for all the necessary callback before starting the compositor
        input_handler->initializeNestedInputHandler(compositor);
        output_handler->initializeNestedOutputHandler(compositor);
        const char * nestedDisplayName = WstCompositorGetDisplayName(compositor);
        setenv("WAYLAND_DISPLAY", nestedDisplayName, 1);
    }

    if (!WstCompositorStart(compositor))
    {
        fprintf(stderr, "ViewBackendWesteros: failed to start the compositor: %s\n",
            WstCompositorGetLastErrorDetail(compositor));
        input_handler->deinitialize();
        output_handler->deinitialize();
        WstCompositorDestroy(compositor);
        compositor = nullptr;
        delete input_handler;
        delete output_handler;
        input_handler = nullptr;
        output_handler = nullptr;
    }
}

ViewBackend::~ViewBackend()
{
    if(input_handler)
        input_handler->deinitialize();
    if(output_handler)
        output_handler->deinitialize();

    if (compositor)
    {
        WstCompositorStop(compositor);
        WstCompositorDestroy(compositor);
        compositor = nullptr;
    }

    if(input_handler)
        delete input_handler;
    if(output_handler)
        delete output_handler;
}

void ViewBackend::initialize()
{
    if(output_handler)
    {
        output_handler->initializeClient();
    }
}

} // namespace Westeros

extern "C" {

struct wpe_view_backend_interface westeros_view_backend_interface = {
    // create
    [](void*, struct wpe_view_backend* backend) -> void*
    {
        return new Westeros::ViewBackend(backend);
    },
    // destroy
    [](void* data)
    {
        auto* backend = static_cast<Westeros::ViewBackend*>(data);
        delete backend;
    },
    // initialize
    [](void* data)
    {
        auto& backend = *static_cast<Westeros::ViewBackend*>(data);
        backend.initialize();
    },
    // get_renderer_host_fd
    [](void* data) -> int
    {
        return -1;
    },
};

}
