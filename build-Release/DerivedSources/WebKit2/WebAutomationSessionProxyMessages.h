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
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace WTF {
    class String;
}

namespace Messages {
namespace WebAutomationSessionProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebAutomationSessionProxy");
}

class EvaluateJavaScriptFunction {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, const Vector<String>&, bool, const int&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EvaluateJavaScriptFunction"); }
    static const bool isSync = false;

    EvaluateJavaScriptFunction(uint64_t pageID, uint64_t frameID, const String& function, const Vector<String>& arguments, bool expectsImplicitCallbackArgument, const int& callbackTimeout, uint64_t callbackID)
        : m_arguments(pageID, frameID, function, arguments, expectsImplicitCallbackArgument, callbackTimeout, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ResolveChildFrameWithOrdinal {
public:
    typedef std::tuple<uint64_t, uint64_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResolveChildFrameWithOrdinal"); }
    static const bool isSync = false;

    ResolveChildFrameWithOrdinal(uint64_t pageID, uint64_t frameID, uint32_t ordinal, uint64_t callbackID)
        : m_arguments(pageID, frameID, ordinal, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ResolveChildFrameWithNodeHandle {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResolveChildFrameWithNodeHandle"); }
    static const bool isSync = false;

    ResolveChildFrameWithNodeHandle(uint64_t pageID, uint64_t frameID, const String& nodeHandle, uint64_t callbackID)
        : m_arguments(pageID, frameID, nodeHandle, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ResolveChildFrameWithName {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResolveChildFrameWithName"); }
    static const bool isSync = false;

    ResolveChildFrameWithName(uint64_t pageID, uint64_t frameID, const String& name, uint64_t callbackID)
        : m_arguments(pageID, frameID, name, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ResolveParentFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResolveParentFrame"); }
    static const bool isSync = false;

    ResolveParentFrame(uint64_t pageID, uint64_t frameID, uint64_t callbackID)
        : m_arguments(pageID, frameID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FocusFrame {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FocusFrame"); }
    static const bool isSync = false;

    FocusFrame(uint64_t pageID, uint64_t frameID)
        : m_arguments(pageID, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ComputeElementLayout {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, bool, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ComputeElementLayout"); }
    static const bool isSync = false;

    ComputeElementLayout(uint64_t pageID, uint64_t frameID, const String& nodeHandle, bool scrollIntoViewIfNeeded, bool useViewportCoordinates, uint64_t callbackID)
        : m_arguments(pageID, frameID, nodeHandle, scrollIntoViewIfNeeded, useViewportCoordinates, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class TakeScreenshot {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TakeScreenshot"); }
    static const bool isSync = false;

    TakeScreenshot(uint64_t pageID, uint64_t callbackID)
        : m_arguments(pageID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetCookiesForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetCookiesForFrame"); }
    static const bool isSync = false;

    GetCookiesForFrame(uint64_t pageID, uint64_t frameID, uint64_t callbackID)
        : m_arguments(pageID, frameID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DeleteCookie {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DeleteCookie"); }
    static const bool isSync = false;

    DeleteCookie(uint64_t pageID, uint64_t frameID, const String& cookieName, uint64_t callbackID)
        : m_arguments(pageID, frameID, cookieName, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace WebAutomationSessionProxy
} // namespace Messages
