/*
 * Copyright (C) 2015 Igalia S.L.
 * Copyright (C) 2015 Metrological
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

#if WPE_BACKEND(BCM_RPI)

#include <WPE/ViewBackend/ViewBackend.h>

#include <WPE/Input/Handling.h>
#include <bcm_host.h>
#include <memory>
#include <utility>

typedef struct _GSource GSource;

namespace WPE {

namespace ViewBackend {

class ViewBackendBCMRPi final : public ViewBackend {
public:
    ViewBackendBCMRPi();
    virtual ~ViewBackendBCMRPi();

    void setClient(Client*) override;
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    void commitBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t) override;

    void setInputClient(Input::Client*) override;

    void handleUpdate();

private:
    Client* m_client;

    DISPMANX_DISPLAY_HANDLE_T m_displayHandle;
    DISPMANX_ELEMENT_HANDLE_T m_elementHandle;

    int m_updateFd;
    GSource* m_updateSource;

    uint32_t m_width;
    uint32_t m_height;

    class Cursor : public Input::Client {
    public:
        Cursor(Input::Client*, DISPMANX_DISPLAY_HANDLE_T, uint32_t, uint32_t);
        virtual ~Cursor();

        void handleKeyboardEvent(Input::KeyboardEvent&&) override;
        void handlePointerEvent(Input::PointerEvent&&) override;
        void handleAxisEvent(Input::AxisEvent&&) override;
        void handleTouchEvent(Input::TouchEvent&&) override;

        static const uint32_t cursorWidth = 16;
        static const uint32_t cursorHeight = 16;

        struct Data {
            static const uint32_t width = 48;
            static const uint32_t height = 48;
            static uint8_t data[width * height * 4 + 1];
        };

    private:
        Input::Client* m_targetClient;
        DISPMANX_ELEMENT_HANDLE_T m_cursorHandle;
        std::pair<uint32_t, uint32_t> m_position;
        std::pair<uint32_t, uint32_t> m_displaySize;
    };
    std::unique_ptr<Cursor> m_cursor;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(BCM_RPI)
