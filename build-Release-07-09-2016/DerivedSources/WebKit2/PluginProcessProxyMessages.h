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

#ifndef PluginProcessProxyMessages_h
#define PluginProcessProxyMessages_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class Attachment;
}

namespace WTF {
    class String;
}

namespace Messages {
namespace PluginProcessProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("PluginProcessProxy");
}

class DidCreateWebProcessConnection {
public:
    typedef std::tuple<IPC::Attachment, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateWebProcessConnection"); }
    static const bool isSync = false;

    DidCreateWebProcessConnection(const IPC::Attachment& connectionIdentifier, bool supportsAsynchronousPluginInitialization)
        : m_arguments(connectionIdentifier, supportsAsynchronousPluginInitialization)
    {
    }

    const std::tuple<const IPC::Attachment&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const IPC::Attachment&, bool> m_arguments;
};

class DidGetSitesWithData {
public:
    typedef std::tuple<Vector<String>, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetSitesWithData"); }
    static const bool isSync = false;

    DidGetSitesWithData(const Vector<String>& sites, uint64_t callbackID)
        : m_arguments(sites, callbackID)
    {
    }

    const std::tuple<const Vector<String>&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const Vector<String>&, uint64_t> m_arguments;
};

class DidDeleteWebsiteData {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteData"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteData(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class DidDeleteWebsiteDataForHostNames {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDeleteWebsiteDataForHostNames"); }
    static const bool isSync = false;

    explicit DidDeleteWebsiteDataForHostNames(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

#if PLATFORM(COCOA)
class SetModalWindowIsShowing {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetModalWindowIsShowing"); }
    static const bool isSync = false;

    explicit SetModalWindowIsShowing(bool modalWindowIsShowing)
        : m_arguments(modalWindowIsShowing)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetFullscreenWindowIsShowing {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetFullscreenWindowIsShowing"); }
    static const bool isSync = false;

    explicit SetFullscreenWindowIsShowing(bool fullscreenWindowIsShowing)
        : m_arguments(fullscreenWindowIsShowing)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class LaunchProcess {
public:
    typedef std::tuple<String, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LaunchProcess"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    LaunchProcess(const String& launchPath, const Vector<String>& arguments)
        : m_arguments(launchPath, arguments)
    {
    }

    const std::tuple<const String&, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const Vector<String>&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class LaunchApplicationAtURL {
public:
    typedef std::tuple<String, Vector<String>> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LaunchApplicationAtURL"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    LaunchApplicationAtURL(const String& url, const Vector<String>& arguments)
        : m_arguments(url, arguments)
    {
    }

    const std::tuple<const String&, const Vector<String>&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const Vector<String>&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class OpenURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenURL"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, int32_t&, String&> Reply;
    explicit OpenURL(const String& urlString)
        : m_arguments(urlString)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class OpenFile {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenFile"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&> Reply;
    explicit OpenFile(const String& fullPath)
        : m_arguments(fullPath)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};
#endif

} // namespace PluginProcessProxy
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // PluginProcessProxyMessages_h
