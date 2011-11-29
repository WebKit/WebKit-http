/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebNotificationManagerProxy_h
#define WebNotificationManagerProxy_h

#include "APIObject.h"
#include "MessageID.h"
#include "WebNotificationProvider.h"
#include <wtf/PassRefPtr.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
}

namespace WebKit {

class WebContext;

class WebNotificationManagerProxy : public APIObject {
public:
    static const Type APIType = TypeNotificationManager;
    
    static PassRefPtr<WebNotificationManagerProxy> create(WebContext*);
    virtual ~WebNotificationManagerProxy();
    
    void invalidate();
    void clearContext() { m_context = 0; }

    void initializeProvider(const WKNotificationProvider*);
    
    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    explicit WebNotificationManagerProxy(WebContext*);
    
    virtual Type type() const { return APIType; }
    
    void didReceiveWebNotificationManagerProxyMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);
    
    // Message handlers
    void show(const WTF::String& title, const WTF::String& body, uint64_t notificationID);
    void cancel(uint64_t notificationID);
    
    WebContext* m_context;
    WebNotificationProvider m_provider;
};

} // namespace WebKit

#endif // WebNotificationManagerProxy_h
