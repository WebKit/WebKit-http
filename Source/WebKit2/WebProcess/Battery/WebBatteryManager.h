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

#ifndef WebBatteryManager_h
#define WebBatteryManager_h

#if ENABLE(BATTERY_STATUS)

#include "MessageID.h"
#include "WebBatteryStatus.h"
#include "WebCoreArgumentCoders.h"
#include <wtf/HashSet.h>
#include <wtf/Noncopyable.h>
#include <wtf/text/AtomicString.h>

namespace CoreIPC {
class ArgumentDecoder;
class Connection;
}

namespace WebKit {

class WebPage;
class WebProcess;

class WebBatteryManager {
    WTF_MAKE_NONCOPYABLE(WebBatteryManager);

public:
    explicit WebBatteryManager(WebProcess*);
    ~WebBatteryManager();

    void registerWebPage(WebPage*);
    void unregisterWebPage(WebPage*);

    void didChangeBatteryStatus(const WTF::AtomicString& eventType, const WebBatteryStatus::Data&);
    void updateBatteryStatus(const WebBatteryStatus::Data&);

    void didReceiveMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

private:
    // Implemented in generated WebBatteryManagerMessageReceiver.cpp
    void didReceiveWebBatteryManagerMessage(CoreIPC::Connection*, CoreIPC::MessageID, CoreIPC::ArgumentDecoder*);

    WebProcess* m_process;
    HashSet<WebPage*> m_pageSet;
};

} // namespace WebKit

#endif // ENABLE(BATTERY_STATUS)

#endif // WebBatteryManager_h
