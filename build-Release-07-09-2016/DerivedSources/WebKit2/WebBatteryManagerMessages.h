/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebBatteryManagerMessages_h
#define WebBatteryManagerMessages_h

#if ENABLE(BATTERY_STATUS)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include "WebBatteryStatus.h"


namespace Messages {
namespace WebBatteryManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebBatteryManager");
}

class DidChangeBatteryStatus {
public:
    typedef std::tuple<AtomicString, WebKit::WebBatteryStatus::Data> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeBatteryStatus"); }
    static const bool isSync = false;

    DidChangeBatteryStatus(const AtomicString& eventType, const WebKit::WebBatteryStatus::Data& status)
        : m_arguments(eventType, status)
    {
    }

    const std::tuple<const AtomicString&, const WebKit::WebBatteryStatus::Data&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const AtomicString&, const WebKit::WebBatteryStatus::Data&> m_arguments;
};

class UpdateBatteryStatus {
public:
    typedef std::tuple<WebKit::WebBatteryStatus::Data> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateBatteryStatus"); }
    static const bool isSync = false;

    explicit UpdateBatteryStatus(const WebKit::WebBatteryStatus::Data& status)
        : m_arguments(status)
    {
    }

    const std::tuple<const WebKit::WebBatteryStatus::Data&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::WebBatteryStatus::Data&> m_arguments;
};

} // namespace WebBatteryManager
} // namespace Messages

#endif // ENABLE(BATTERY_STATUS)

#endif // WebBatteryManagerMessages_h
