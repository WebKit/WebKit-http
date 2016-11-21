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

#ifndef WebNotificationManagerMessages_h
#define WebNotificationManagerMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTF {
    class String;
}

namespace Messages {
namespace WebNotificationManager {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebNotificationManager");
}

class DidShowNotification {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidShowNotification"); }
    static const bool isSync = false;

    explicit DidShowNotification(uint64_t notificationID)
        : m_arguments(notificationID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DidClickNotification {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidClickNotification"); }
    static const bool isSync = false;

    explicit DidClickNotification(uint64_t notificationID)
        : m_arguments(notificationID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DidCloseNotifications {
public:
    typedef std::tuple<Vector<uint64_t>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCloseNotifications"); }
    static const bool isSync = false;

    explicit DidCloseNotifications(const Vector<uint64_t>& notificationIDs)
        : m_arguments(notificationIDs)
    {
    }

    const std::tuple<const Vector<uint64_t>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<uint64_t>&> m_arguments;
};

class DidUpdateNotificationDecision {
public:
    typedef std::tuple<String, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateNotificationDecision"); }
    static const bool isSync = false;

    DidUpdateNotificationDecision(const String& originString, bool allowed)
        : m_arguments(originString, allowed)
    {
    }

    const std::tuple<const String&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, bool> m_arguments;
};

class DidRemoveNotificationDecisions {
public:
    typedef std::tuple<Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRemoveNotificationDecisions"); }
    static const bool isSync = false;

    explicit DidRemoveNotificationDecisions(const Vector<String>& originStrings)
        : m_arguments(originStrings)
    {
    }

    const std::tuple<const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&> m_arguments;
};

} // namespace WebNotificationManager
} // namespace Messages

#endif // WebNotificationManagerMessages_h
