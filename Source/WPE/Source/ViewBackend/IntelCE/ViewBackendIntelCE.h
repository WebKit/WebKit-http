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

#if WPE_BACKEND(INTEL_CE)

#include <WPE/ViewBackend/ViewBackend.h>

namespace WPE {

namespace ViewBackend {

class ViewBackendIntelCE final : public ViewBackend {
public:
    ViewBackendIntelCE();
    virtual ~ViewBackendIntelCE();

    void setClient(Client*) override;
    std::pair<const uint8_t*, size_t> authenticate() override { return { nullptr, 0 }; };
    uint32_t constructRenderingTarget(uint32_t, uint32_t) override;
    void commitBuffer(int, const uint8_t*, size_t) override;
    void destroyBuffer(uint32_t) override;

    void setInputClient(Input::Client*) override;

private:
    Client* m_client;

    uint32_t m_width;
    uint32_t m_height;
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_BACKEND(INTEL_CE)
