/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef WebVibrationProxy_h
#define WebVibrationProxy_h

#if ENABLE(VIBRATION)

#include "APIObject.h"
#include "MessageID.h"
#include "WebVibrationProvider.h"
#include <wtf/Forward.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
}

namespace WebKit {

class WebContext;

class WebVibrationProxy : public APIObject {
public:
    static const Type APIType = TypeVibration;

    static PassRefPtr<WebVibrationProxy> create(WebContext*);
    virtual ~WebVibrationProxy();

    void invalidate();
    void clearContext() { m_context = 0; }

    void initializeProvider(const WKVibrationProvider*);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    explicit WebVibrationProxy(WebContext*);

    virtual Type type() const { return APIType; }

    // Implemented in generated WebVibrationProxyMessageReceiver.cpp
    void didReceiveWebVibrationProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    void vibrate(uint64_t vibrationTime);
    void cancelVibration();

    WebContext* m_context;
    WebVibrationProvider m_provider;
};

} // namespace WebKit

#endif // ENABLE(VIBRATION)

#endif // WebVibrationProxy_h
