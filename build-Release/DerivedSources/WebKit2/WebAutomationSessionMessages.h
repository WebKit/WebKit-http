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

#pragma once

#include "ArgumentCoders.h"
#include "ShareableBitmap.h"
#include <WebCore/Cookie.h>
#include <wtf/Vector.h>

namespace WTF {
    class String;
}

namespace WebCore {
    class IntRect;
}

namespace Messages {
namespace WebAutomationSession {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebAutomationSession");
}

class DidEvaluateJavaScriptFunction {
public:
    typedef std::tuple<uint64_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidEvaluateJavaScriptFunction"); }
    static const bool isSync = false;

    DidEvaluateJavaScriptFunction(uint64_t callbackID, const String& result, const String& errorType)
        : m_arguments(callbackID, result, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidResolveChildFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidResolveChildFrame"); }
    static const bool isSync = false;

    DidResolveChildFrame(uint64_t callbackID, uint64_t frameID, const String& errorType)
        : m_arguments(callbackID, frameID, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidResolveParentFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidResolveParentFrame"); }
    static const bool isSync = false;

    DidResolveParentFrame(uint64_t callbackID, uint64_t frameID, const String& errorType)
        : m_arguments(callbackID, frameID, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidComputeElementLayout {
public:
    typedef std::tuple<uint64_t, const WebCore::IntRect&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidComputeElementLayout"); }
    static const bool isSync = false;

    DidComputeElementLayout(uint64_t callbackID, const WebCore::IntRect& rect, const String& errorType)
        : m_arguments(callbackID, rect, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidTakeScreenshot {
public:
    typedef std::tuple<uint64_t, const WebKit::ShareableBitmap::Handle&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidTakeScreenshot"); }
    static const bool isSync = false;

    DidTakeScreenshot(uint64_t callbackID, const WebKit::ShareableBitmap::Handle& imageDataHandle, const String& errorType)
        : m_arguments(callbackID, imageDataHandle, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidGetCookiesForFrame {
public:
    typedef std::tuple<uint64_t, const Vector<WebCore::Cookie>&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetCookiesForFrame"); }
    static const bool isSync = false;

    DidGetCookiesForFrame(uint64_t callbackID, const Vector<WebCore::Cookie>& cookies, const String& errorType)
        : m_arguments(callbackID, cookies, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDeleteCookie {
public:
    typedef std::tuple<uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteCookie"); }
    static const bool isSync = false;

    DidDeleteCookie(uint64_t callbackID, const String& errorType)
        : m_arguments(callbackID, errorType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebAutomationSession
} // namespace Messages
