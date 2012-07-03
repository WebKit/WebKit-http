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

#ifndef WebBatteryManagerProxy_h
#define WebBatteryManagerProxy_h

#if ENABLE(BATTERY_STATUS)

#include "APIObject.h"
#include "MessageID.h"
#include "WebBatteryProvider.h"
#include <wtf/Forward.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
}

namespace WebKit {

class WebContext;
class WebBatteryStatus;

class WebBatteryManagerProxy : public APIObject {
public:
    static const Type APIType = TypeBatteryManager;

    static PassRefPtr<WebBatteryManagerProxy> create(WebContext*);
    virtual ~WebBatteryManagerProxy();

    void invalidate();
    void clearContext() { m_context = 0; }

    void initializeProvider(const WKBatteryProvider*);

    void providerDidChangeBatteryStatus(const WTF::AtomicString&, WebBatteryStatus*);
    void providerUpdateBatteryStatus(WebBatteryStatus*);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    explicit WebBatteryManagerProxy(WebContext*);

    virtual Type type() const { return APIType; }

    // Implemented in generated WebBatteryManagerProxyMessageReceiver.cpp
    void didReceiveWebBatteryManagerProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    void startUpdating();
    void stopUpdating();

    bool m_isUpdating;

    WebContext* m_context;
    WebBatteryProvider m_provider;
};

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)

#endif // WebBatteryManagerProxy_h
