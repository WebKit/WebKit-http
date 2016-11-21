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

#if ENABLE(NETSCAPE_PLUGIN_API)

#include "ArgumentCoders.h"
#include "ShareableBitmap.h"

namespace IPC {
    class DataReference;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class IntRect;
    class AffineTransform;
    class IntSize;
}

namespace WebKit {
    class WebWheelEvent;
    class WebMouseEvent;
    class WebKeyboardEvent;
}

namespace Messages {
namespace PluginControllerProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("PluginControllerProxy");
}

class GeometryDidChange {
public:
    typedef std::tuple<const WebCore::IntSize&, const WebCore::IntRect&, const WebCore::AffineTransform&, float, const WebKit::ShareableBitmap::Handle&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GeometryDidChange"); }
    static const bool isSync = false;

    GeometryDidChange(const WebCore::IntSize& pluginSize, const WebCore::IntRect& clipRect, const WebCore::AffineTransform& pluginToRootViewTransform, float scaleFactor, const WebKit::ShareableBitmap::Handle& backingStoreHandle)
        : m_arguments(pluginSize, clipRect, pluginToRootViewTransform, scaleFactor, backingStoreHandle)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class VisibilityDidChange {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("VisibilityDidChange"); }
    static const bool isSync = false;

    explicit VisibilityDidChange(bool isVisible)
        : m_arguments(isVisible)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FrameDidFinishLoading {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FrameDidFinishLoading"); }
    static const bool isSync = false;

    explicit FrameDidFinishLoading(uint64_t requestID)
        : m_arguments(requestID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FrameDidFail {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FrameDidFail"); }
    static const bool isSync = false;

    FrameDidFail(uint64_t requestID, bool wasCancelled)
        : m_arguments(requestID, wasCancelled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidEvaluateJavaScript {
public:
    typedef std::tuple<uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidEvaluateJavaScript"); }
    static const bool isSync = false;

    DidEvaluateJavaScript(uint64_t requestID, const String& result)
        : m_arguments(requestID, result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StreamWillSendRequest {
public:
    typedef std::tuple<uint64_t, const String&, const String&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StreamWillSendRequest"); }
    static const bool isSync = false;

    StreamWillSendRequest(uint64_t streamID, const String& requestURLString, const String& redirectResponseURLString, uint32_t redirectResponseStatusCode)
        : m_arguments(streamID, requestURLString, redirectResponseURLString, redirectResponseStatusCode)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StreamDidReceiveResponse {
public:
    typedef std::tuple<uint64_t, const String&, uint32_t, uint32_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StreamDidReceiveResponse"); }
    static const bool isSync = false;

    StreamDidReceiveResponse(uint64_t streamID, const String& responseURLString, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers)
        : m_arguments(streamID, responseURLString, streamLength, lastModifiedTime, mimeType, headers)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StreamDidReceiveData {
public:
    typedef std::tuple<uint64_t, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StreamDidReceiveData"); }
    static const bool isSync = false;

    StreamDidReceiveData(uint64_t streamID, const IPC::DataReference& data)
        : m_arguments(streamID, data)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StreamDidFinishLoading {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StreamDidFinishLoading"); }
    static const bool isSync = false;

    explicit StreamDidFinishLoading(uint64_t streamID)
        : m_arguments(streamID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StreamDidFail {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StreamDidFail"); }
    static const bool isSync = false;

    StreamDidFail(uint64_t streamID, bool wasCancelled)
        : m_arguments(streamID, wasCancelled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ManualStreamDidReceiveResponse {
public:
    typedef std::tuple<const String&, uint32_t, uint32_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ManualStreamDidReceiveResponse"); }
    static const bool isSync = false;

    ManualStreamDidReceiveResponse(const String& responseURLString, uint32_t streamLength, uint32_t lastModifiedTime, const String& mimeType, const String& headers)
        : m_arguments(responseURLString, streamLength, lastModifiedTime, mimeType, headers)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ManualStreamDidReceiveData {
public:
    typedef std::tuple<const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ManualStreamDidReceiveData"); }
    static const bool isSync = false;

    explicit ManualStreamDidReceiveData(const IPC::DataReference& data)
        : m_arguments(data)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ManualStreamDidFinishLoading {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ManualStreamDidFinishLoading"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ManualStreamDidFail {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ManualStreamDidFail"); }
    static const bool isSync = false;

    explicit ManualStreamDidFail(bool wasCancelled)
        : m_arguments(wasCancelled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleMouseEvent {
public:
    typedef std::tuple<const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMouseEvent"); }
    static const bool isSync = false;

    explicit HandleMouseEvent(const WebKit::WebMouseEvent& mouseEvent)
        : m_arguments(mouseEvent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleWheelEvent {
public:
    typedef std::tuple<const WebKit::WebWheelEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleWheelEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit HandleWheelEvent(const WebKit::WebWheelEvent& wheelEvent)
        : m_arguments(wheelEvent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleMouseEnterEvent {
public:
    typedef std::tuple<const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMouseEnterEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit HandleMouseEnterEvent(const WebKit::WebMouseEvent& mouseEvent)
        : m_arguments(mouseEvent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleMouseLeaveEvent {
public:
    typedef std::tuple<const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMouseLeaveEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit HandleMouseLeaveEvent(const WebKit::WebMouseEvent& mouseEvent)
        : m_arguments(mouseEvent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleKeyboardEvent {
public:
    typedef std::tuple<const WebKit::WebKeyboardEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleKeyboardEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit HandleKeyboardEvent(const WebKit::WebKeyboardEvent& keyboardEvent)
        : m_arguments(keyboardEvent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleEditingCommand {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleEditingCommand"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    HandleEditingCommand(const String& commandName, const String& argument)
        : m_arguments(commandName, argument)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class IsEditingCommandEnabled {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("IsEditingCommandEnabled"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit IsEditingCommandEnabled(const String& commandName)
        : m_arguments(commandName)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandlesPageScaleFactor {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandlesPageScaleFactor"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RequiresUnifiedScaleFactor {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequiresUnifiedScaleFactor"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetFocus {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetFocus"); }
    static const bool isSync = false;

    explicit SetFocus(bool isFocused)
        : m_arguments(isFocused)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidUpdate {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdate"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PaintEntirePlugin {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PaintEntirePlugin"); }
    static const bool isSync = true;

    typedef std::tuple<> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetPluginScriptableNPObject {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPluginScriptableNPObject"); }
    static const bool isSync = true;

    typedef std::tuple<uint64_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class WindowFocusChanged {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowFocusChanged"); }
    static const bool isSync = false;

    explicit WindowFocusChanged(bool hasFocus)
        : m_arguments(hasFocus)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class WindowVisibilityChanged {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowVisibilityChanged"); }
    static const bool isSync = false;

    explicit WindowVisibilityChanged(bool isVisible)
        : m_arguments(isVisible)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(COCOA)
class SendComplexTextInput {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SendComplexTextInput"); }
    static const bool isSync = false;

    explicit SendComplexTextInput(const String& textInput)
        : m_arguments(textInput)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class WindowAndViewFramesChanged {
public:
    typedef std::tuple<const WebCore::IntRect&, const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowAndViewFramesChanged"); }
    static const bool isSync = false;

    WindowAndViewFramesChanged(const WebCore::IntRect& windowFrameInScreenCoordinates, const WebCore::IntRect& viewFrameInWindowCoordinates)
        : m_arguments(windowFrameInScreenCoordinates, viewFrameInWindowCoordinates)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SetLayerHostingMode {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetLayerHostingMode"); }
    static const bool isSync = false;

    explicit SetLayerHostingMode(uint32_t layerHostingMode)
        : m_arguments(layerHostingMode)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class SupportsSnapshotting {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SupportsSnapshotting"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class Snapshot {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Snapshot"); }
    static const bool isSync = true;

    typedef std::tuple<WebKit::ShareableBitmap::Handle&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StorageBlockingStateChanged {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StorageBlockingStateChanged"); }
    static const bool isSync = false;

    explicit StorageBlockingStateChanged(bool storageBlockingEnabled)
        : m_arguments(storageBlockingEnabled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PrivateBrowsingStateChanged {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrivateBrowsingStateChanged"); }
    static const bool isSync = false;

    explicit PrivateBrowsingStateChanged(bool isPrivateBrowsingEnabled)
        : m_arguments(isPrivateBrowsingEnabled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetFormValue {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetFormValue"); }
    static const bool isSync = true;

    typedef std::tuple<bool&, String&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class MutedStateChanged {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MutedStateChanged"); }
    static const bool isSync = false;

    explicit MutedStateChanged(bool muted)
        : m_arguments(muted)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

} // namespace PluginControllerProxy
} // namespace Messages

#endif // ENABLE(NETSCAPE_PLUGIN_API)
