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

#include "Config.h"
#include <WPE/Pasteboard/Pasteboard.h>

#include "PasteboardGeneric.h"
#if WPE_BACKEND(WAYLAND)
#include "PasteboardWayland.h"
#endif
#include <cstring>
#include <cstdio>
#include <cstdlib>

namespace WPE {

namespace Pasteboard {

std::shared_ptr<Pasteboard> Pasteboard::singleton()
{
    static std::shared_ptr<Pasteboard> pasteboard = nullptr;

    if (pasteboard)
        return pasteboard;

#if WPE_BACKEND(WAYLAND)
    auto* backendEnv = std::getenv("WPE_BACKEND");
    if (std::getenv("WAYLAND_DISPLAY") || (backendEnv && !std::strcmp(backendEnv, "wayland"))) {
        pasteboard = std::shared_ptr<PasteboardWayland>(new PasteboardWayland);
        return pasteboard;
    }
#endif

    pasteboard = std::shared_ptr<PasteboardGeneric>(new PasteboardGeneric);
    return pasteboard;
}

} // namespace Pasteboard

} // namespace WPE
