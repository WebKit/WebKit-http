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

#ifndef WPE_ViewBackend_ViewBackend_h
#define WPE_ViewBackend_ViewBackend_h

#include <WPE/WPE.h>
#include <memory>

namespace WPE {

namespace Input {
class Client;
}

namespace ViewBackend {

class Client {
public:
    virtual void releaseBuffer(uint32_t handle) = 0;
    virtual void frameComplete() = 0;
};

class ViewBackend {
public:
    static WPE_EXPORT std::unique_ptr<ViewBackend> create();

    virtual void setClient(Client*);
    virtual void commitPrimeBuffer(int, uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);
    virtual void destroyPrimeBuffer(uint32_t);

    virtual void commitBCMBuffer(uint32_t, uint32_t, uint32_t, uint32_t, uint32_t);

    virtual void setInputClient(Input::Client*);
};

} // namespace ViewBackend

} // namespace WPE

#endif // WPE_ViewBackend_ViewBackend_h
