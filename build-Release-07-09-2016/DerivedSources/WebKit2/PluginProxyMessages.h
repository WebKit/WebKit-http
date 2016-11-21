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

#ifndef PluginProxyMessages_h
#define PluginProxyMessages_h

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"
#include <wtf/Vector.h>

namespace WTF {
    class String;
}

namespace WebCore {
    class HTTPHeaderMap;
    class IntRect;
    class ProtectionSpace;
}

namespace WebKit {
    class NPVariantData;
}

namespace Messages {
namespace PluginProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("PluginProxy");
}

class LoadURL {
public:
    typedef std::tuple<uint64_t, String, String, String, WebCore::HTTPHeaderMap, Vector<uint8_t>, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadURL"); }
    static const bool isSync = false;

    LoadURL(uint64_t requestID, const String& method, const String& urlString, const String& target, const WebCore::HTTPHeaderMap& headerFields, const Vector<uint8_t>& httpBody, bool allowPopups)
        : m_arguments(requestID, method, urlString, target, headerFields, httpBody, allowPopups)
    {
    }

    const std::tuple<uint64_t, const String&, const String&, const String&, const WebCore::HTTPHeaderMap&, const Vector<uint8_t>&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const String&, const String&, const String&, const WebCore::HTTPHeaderMap&, const Vector<uint8_t>&, bool> m_arguments;
};

class Update {
public:
    typedef std::tuple<WebCore::IntRect> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Update"); }
    static const bool isSync = false;

    explicit Update(const WebCore::IntRect& paintedRect)
        : m_arguments(paintedRect)
    {
    }

    const std::tuple<const WebCore::IntRect&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::IntRect&> m_arguments;
};

class ProxiesForURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ProxiesForURL"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    explicit ProxiesForURL(const String& urlString)
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

class CookiesForURL {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CookiesForURL"); }
    static const bool isSync = true;

    typedef IPC::Arguments<String&> Reply;
    explicit CookiesForURL(const String& urlString)
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

class SetCookiesForURL {
public:
    typedef std::tuple<String, String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCookiesForURL"); }
    static const bool isSync = false;

    SetCookiesForURL(const String& urlString, const String& cookieString)
        : m_arguments(urlString, cookieString)
    {
    }

    const std::tuple<const String&, const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&, const String&> m_arguments;
};

class GetAuthenticationInfo {
public:
    typedef std::tuple<WebCore::ProtectionSpace> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetAuthenticationInfo"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, String&, String&> Reply;
    explicit GetAuthenticationInfo(const WebCore::ProtectionSpace& protectionSpace)
        : m_arguments(protectionSpace)
    {
    }

    const std::tuple<const WebCore::ProtectionSpace&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::ProtectionSpace&> m_arguments;
};

class GetPluginElementNPObject {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPluginElementNPObject"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class Evaluate {
public:
    typedef std::tuple<WebKit::NPVariantData, String, bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Evaluate"); }
    static const bool isSync = true;

    typedef IPC::Arguments<bool&, WebKit::NPVariantData&> Reply;
    Evaluate(const WebKit::NPVariantData& npObjectAsVariantData, const String& scriptString, bool allowPopups)
        : m_arguments(npObjectAsVariantData, scriptString, allowPopups)
    {
    }

    const std::tuple<const WebKit::NPVariantData&, const String&, bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebKit::NPVariantData&, const String&, bool> m_arguments;
};

class CancelStreamLoad {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelStreamLoad"); }
    static const bool isSync = false;

    explicit CancelStreamLoad(uint64_t streamID)
        : m_arguments(streamID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class ContinueStreamLoad {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContinueStreamLoad"); }
    static const bool isSync = false;

    explicit ContinueStreamLoad(uint64_t streamID)
        : m_arguments(streamID)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};

class CancelManualStreamLoad {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelManualStreamLoad"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class SetStatusbarText {
public:
    typedef std::tuple<String> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetStatusbarText"); }
    static const bool isSync = false;

    explicit SetStatusbarText(const String& statusbarText)
        : m_arguments(statusbarText)
    {
    }

    const std::tuple<const String&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const String&> m_arguments;
};

#if PLATFORM(COCOA)
class PluginFocusOrWindowFocusChanged {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PluginFocusOrWindowFocusChanged"); }
    static const bool isSync = false;

    explicit PluginFocusOrWindowFocusChanged(bool pluginHasFocusAndWindowHasFocus)
        : m_arguments(pluginHasFocusAndWindowHasFocus)
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
class SetComplexTextInputState {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetComplexTextInputState"); }
    static const bool isSync = false;

    explicit SetComplexTextInputState(uint64_t complexTextInputState)
        : m_arguments(complexTextInputState)
    {
    }

    const std::tuple<uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetLayerHostingContextID {
public:
    typedef std::tuple<uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetLayerHostingContextID"); }
    static const bool isSync = false;

    explicit SetLayerHostingContextID(uint32_t layerHostingContextID)
        : m_arguments(layerHostingContextID)
    {
    }

    const std::tuple<uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint32_t> m_arguments;
};
#endif

#if PLUGIN_ARCHITECTURE(X11)
class CreatePluginContainer {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreatePluginContainer"); }
    static const bool isSync = true;

    typedef IPC::Arguments<uint64_t&> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

#if PLUGIN_ARCHITECTURE(X11)
class WindowedPluginGeometryDidChange {
public:
    typedef std::tuple<WebCore::IntRect, WebCore::IntRect, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowedPluginGeometryDidChange"); }
    static const bool isSync = false;

    WindowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID)
        : m_arguments(frameRect, clipRect, windowID)
    {
    }

    const std::tuple<const WebCore::IntRect&, const WebCore::IntRect&, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::IntRect&, const WebCore::IntRect&, uint64_t> m_arguments;
};
#endif

#if PLUGIN_ARCHITECTURE(X11)
class WindowedPluginVisibilityDidChange {
public:
    typedef std::tuple<bool, uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowedPluginVisibilityDidChange"); }
    static const bool isSync = false;

    WindowedPluginVisibilityDidChange(bool isVisible, uint64_t windowID)
        : m_arguments(isVisible, windowID)
    {
    }

    const std::tuple<bool, uint64_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool, uint64_t> m_arguments;
};
#endif

class DidCreatePlugin {
public:
    typedef std::tuple<bool, uint32_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreatePlugin"); }
    static const bool isSync = true;

    typedef IPC::Arguments<> Reply;
    DidCreatePlugin(bool wantsWheelEvents, uint32_t remoteLayerClientID)
        : m_arguments(wantsWheelEvents, remoteLayerClientID)
    {
    }

    const std::tuple<bool, uint32_t>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool, uint32_t> m_arguments;
};

class DidFailToCreatePlugin {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailToCreatePlugin"); }
    static const bool isSync = true;

    typedef IPC::Arguments<> Reply;
    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};

class SetPluginIsPlayingAudio {
public:
    typedef std::tuple<bool> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPluginIsPlayingAudio"); }
    static const bool isSync = false;

    explicit SetPluginIsPlayingAudio(bool pluginIsPlayingAudio)
        : m_arguments(pluginIsPlayingAudio)
    {
    }

    const std::tuple<bool>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<bool> m_arguments;
};

} // namespace PluginProxy
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)

#endif // PluginProxyMessages_h
