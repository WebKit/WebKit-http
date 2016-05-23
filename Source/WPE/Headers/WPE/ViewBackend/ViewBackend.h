/*
 * Copyright (C) 2015 Igalia S.L.
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

#ifndef WPE_ViewBackend_ViewBackend_h
#define WPE_ViewBackend_ViewBackend_h

#include <WPE/WPE.h>
#include <memory>
#include <utility>

namespace WPE {

namespace Input {
class Client;
struct KeyboardEvent;
struct PointerEvent;
struct AxisEvent;
}

namespace ViewBackend {

class Client {
public:
    virtual void releaseBuffer(uint32_t handle) = 0;
    virtual void frameComplete() = 0;
    virtual void setSize(uint32_t width, uint32_t height) = 0;
};

class ViewBackend {
public:
    static WPE_EXPORT std::unique_ptr<ViewBackend> create();
    virtual ~ViewBackend();

    virtual void setClient(Client*);
    virtual std::pair<const uint8_t*, size_t> authenticate() = 0;
    virtual uint32_t constructRenderingTarget(uint32_t, uint32_t) = 0;
    virtual void commitBuffer(int, const uint8_t*, size_t) = 0;
    virtual void destroyBuffer(uint32_t) = 0;

    virtual void handleKeyboardEvent(const Input::KeyboardEvent& event);
    virtual void handlePointerEvent(const Input::PointerEvent& event);
    virtual void handleAxisEvent(const Input::AxisEvent& event);

    virtual void setInputClient(Input::Client*);
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_ViewBackend_ViewBackend_h
