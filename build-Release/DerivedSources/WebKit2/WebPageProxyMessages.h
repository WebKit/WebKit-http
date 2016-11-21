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
#include "SandboxExtension.h"
#include "ShareableBitmap.h"
#include "SharedMemory.h"
#include "WebPopupItem.h"
#include <WebCore/FloatQuad.h>
#include <WebCore/FloatRect.h>
#include <WebCore/IntRect.h>
#include <WebCore/SearchPopupMenu.h>
#include <WebCore/TextCheckerClient.h>
#include <utility>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class DataReference;
    class Connection;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class FloatRect;
    class TextCheckingRequestData;
    class Color;
    class ResourceResponse;
    struct WindowFeatures;
    struct Highlight;
    struct SecurityOriginData;
    class ContentFilterUnblockHandler;
    class IntRect;
    class IntPoint;
    class Cursor;
    class MachSendRight;
    class ResourceError;
    struct GrammarDetail;
    struct ExceptionDetails;
    struct DictionaryPopupInfo;
    class IntSize;
    class FloatSize;
    class FloatPoint;
    struct FileChooserSettings;
    class ResourceRequest;
    struct TextIndicatorData;
    class MediaSessionMetadata;
    class CertificateInfo;
    struct ViewportAttributes;
    class DragData;
    class AuthenticationChallenge;
}

namespace WebKit {
    struct AttributedString;
    struct InteractionInformationAtPosition;
    class ContextMenuContextData;
    struct WebNavigationDataStore;
    struct AssistedNodeInformation;
    class QuickLookDocumentData;
    struct DataDetectionResult;
    struct WebPageCreationParameters;
    class UserData;
    struct EditingRange;
    struct WebHitTestResultData;
    struct EditorState;
    struct PlatformPopupMenuData;
    struct NavigationActionData;
    class DownloadID;
}

namespace Messages {
namespace WebPageProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebPageProxy");
}

class CreateNewPage {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const WebCore::ResourceRequest&, const WebCore::WindowFeatures&, const WebKit::NavigationActionData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreateNewPage"); }
    static const bool isSync = true;

    typedef std::tuple<uint64_t&, WebKit::WebPageCreationParameters&> Reply;
    CreateNewPage(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebCore::ResourceRequest& request, const WebCore::WindowFeatures& windowFeatures, const WebKit::NavigationActionData& navigationActionData)
        : m_arguments(frameID, frameSecurityOrigin, request, windowFeatures, navigationActionData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ShowPage {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowPage"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClosePage {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClosePage"); }
    static const bool isSync = false;

    explicit ClosePage(bool stopResponsivenessTimer)
        : m_arguments(stopResponsivenessTimer)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunJavaScriptAlert {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunJavaScriptAlert"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send();

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<> Reply;
    RunJavaScriptAlert(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const String& message)
        : m_arguments(frameID, frameSecurityOrigin, message)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunJavaScriptConfirm {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunJavaScriptConfirm"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(bool result);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<bool&> Reply;
    RunJavaScriptConfirm(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const String& message)
        : m_arguments(frameID, frameSecurityOrigin, message)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunJavaScriptPrompt {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunJavaScriptPrompt"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(const String& result);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<String&> Reply;
    RunJavaScriptPrompt(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const String& message, const String& defaultValue)
        : m_arguments(frameID, frameSecurityOrigin, message, defaultValue)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class MouseDidMoveOverElement {
public:
    typedef std::tuple<const WebKit::WebHitTestResultData&, uint32_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MouseDidMoveOverElement"); }
    static const bool isSync = false;

    MouseDidMoveOverElement(const WebKit::WebHitTestResultData& hitTestResultData, uint32_t modifiers, const WebKit::UserData& userData)
        : m_arguments(hitTestResultData, modifiers, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(NETSCAPE_PLUGIN_API)
class UnavailablePluginButtonClicked {
public:
    typedef std::tuple<uint32_t, const String&, const String&, const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnavailablePluginButtonClicked"); }
    static const bool isSync = false;

    UnavailablePluginButtonClicked(uint32_t pluginUnavailabilityReason, const String& mimeType, const String& pluginURLString, const String& pluginspageAttributeURLString, const String& frameURLString, const String& pageURLString)
        : m_arguments(pluginUnavailabilityReason, mimeType, pluginURLString, pluginspageAttributeURLString, frameURLString, pageURLString)
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

#if ENABLE(WEBGL)
class WebGLPolicyForURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WebGLPolicyForURL"); }
    static const bool isSync = true;

    typedef std::tuple<uint32_t&> Reply;
    explicit WebGLPolicyForURL(const String& url)
        : m_arguments(url)
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

#if ENABLE(WEBGL)
class ResolveWebGLPolicyForURL {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResolveWebGLPolicyForURL"); }
    static const bool isSync = true;

    typedef std::tuple<uint32_t&> Reply;
    explicit ResolveWebGLPolicyForURL(const String& url)
        : m_arguments(url)
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

class DidChangeViewportProperties {
public:
    typedef std::tuple<const WebCore::ViewportAttributes&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeViewportProperties"); }
    static const bool isSync = false;

    explicit DidChangeViewportProperties(const WebCore::ViewportAttributes& attributes)
        : m_arguments(attributes)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveEvent {
public:
    typedef std::tuple<uint32_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveEvent"); }
    static const bool isSync = false;

    DidReceiveEvent(uint32_t type, bool handled)
        : m_arguments(type, handled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StopResponsivenessTimer {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopResponsivenessTimer"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if !PLATFORM(IOS)
class SetCursor {
public:
    typedef std::tuple<const WebCore::Cursor&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCursor"); }
    static const bool isSync = false;

    explicit SetCursor(const WebCore::Cursor& cursor)
        : m_arguments(cursor)
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

#if !PLATFORM(IOS)
class SetCursorHiddenUntilMouseMoves {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCursorHiddenUntilMouseMoves"); }
    static const bool isSync = false;

    explicit SetCursorHiddenUntilMouseMoves(bool hiddenUntilMouseMoves)
        : m_arguments(hiddenUntilMouseMoves)
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

class SetStatusText {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetStatusText"); }
    static const bool isSync = false;

    explicit SetStatusText(const String& statusText)
        : m_arguments(statusText)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetToolTip {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetToolTip"); }
    static const bool isSync = false;

    explicit SetToolTip(const String& toolTip)
        : m_arguments(toolTip)
    {
    }

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

    explicit SetFocus(bool focused)
        : m_arguments(focused)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class TakeFocus {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TakeFocus"); }
    static const bool isSync = false;

    explicit TakeFocus(uint32_t direction)
        : m_arguments(direction)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FocusedFrameChanged {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FocusedFrameChanged"); }
    static const bool isSync = false;

    explicit FocusedFrameChanged(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FrameSetLargestFrameChanged {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FrameSetLargestFrameChanged"); }
    static const bool isSync = false;

    explicit FrameSetLargestFrameChanged(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetRenderTreeSize {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetRenderTreeSize"); }
    static const bool isSync = false;

    explicit SetRenderTreeSize(uint64_t treeSize)
        : m_arguments(treeSize)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetToolbarsAreVisible {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetToolbarsAreVisible"); }
    static const bool isSync = false;

    explicit SetToolbarsAreVisible(bool toolbarsAreVisible)
        : m_arguments(toolbarsAreVisible)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetToolbarsAreVisible {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetToolbarsAreVisible"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetMenuBarIsVisible {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMenuBarIsVisible"); }
    static const bool isSync = false;

    explicit SetMenuBarIsVisible(bool menuBarIsVisible)
        : m_arguments(menuBarIsVisible)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetMenuBarIsVisible {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetMenuBarIsVisible"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetStatusBarIsVisible {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetStatusBarIsVisible"); }
    static const bool isSync = false;

    explicit SetStatusBarIsVisible(bool statusBarIsVisible)
        : m_arguments(statusBarIsVisible)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetStatusBarIsVisible {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetStatusBarIsVisible"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetIsResizable {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetIsResizable"); }
    static const bool isSync = false;

    explicit SetIsResizable(bool isResizable)
        : m_arguments(isResizable)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetIsResizable {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetIsResizable"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetWindowFrame {
public:
    typedef std::tuple<const WebCore::FloatRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetWindowFrame"); }
    static const bool isSync = false;

    explicit SetWindowFrame(const WebCore::FloatRect& windowFrame)
        : m_arguments(windowFrame)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetWindowFrame {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetWindowFrame"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::FloatRect&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ScreenToRootView {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScreenToRootView"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::IntPoint&> Reply;
    explicit ScreenToRootView(const WebCore::IntPoint& screenPoint)
        : m_arguments(screenPoint)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RootViewToScreen {
public:
    typedef std::tuple<const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RootViewToScreen"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::IntRect&> Reply;
    explicit RootViewToScreen(const WebCore::IntRect& rect)
        : m_arguments(rect)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(IOS)
class AccessibilityScreenToRootView {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AccessibilityScreenToRootView"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::IntPoint&> Reply;
    explicit AccessibilityScreenToRootView(const WebCore::IntPoint& screenPoint)
        : m_arguments(screenPoint)
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

#if PLATFORM(IOS)
class RootViewToAccessibilityScreen {
public:
    typedef std::tuple<const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RootViewToAccessibilityScreen"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::IntRect&> Reply;
    explicit RootViewToAccessibilityScreen(const WebCore::IntRect& rect)
        : m_arguments(rect)
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

class RunBeforeUnloadConfirmPanel {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunBeforeUnloadConfirmPanel"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(bool shouldClose);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<bool&> Reply;
    RunBeforeUnloadConfirmPanel(const String& message, uint64_t frameID)
        : m_arguments(message, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PageDidScroll {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PageDidScroll"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunOpenPanel {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const WebCore::FileChooserSettings&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunOpenPanel"); }
    static const bool isSync = false;

    RunOpenPanel(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebCore::FileChooserSettings& parameters)
        : m_arguments(frameID, frameSecurityOrigin, parameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PrintFrame {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrintFrame"); }
    static const bool isSync = true;

    typedef std::tuple<> Reply;
    explicit PrintFrame(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunModal {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunModal"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class NotifyScrollerThumbIsVisibleInRect {
public:
    typedef std::tuple<const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("NotifyScrollerThumbIsVisibleInRect"); }
    static const bool isSync = false;

    explicit NotifyScrollerThumbIsVisibleInRect(const WebCore::IntRect& scrollerThumb)
        : m_arguments(scrollerThumb)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RecommendedScrollbarStyleDidChange {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RecommendedScrollbarStyleDidChange"); }
    static const bool isSync = false;

    explicit RecommendedScrollbarStyleDidChange(int32_t newStyle)
        : m_arguments(newStyle)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidChangeScrollbarsForMainFrame {
public:
    typedef std::tuple<bool, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeScrollbarsForMainFrame"); }
    static const bool isSync = false;

    DidChangeScrollbarsForMainFrame(bool hasHorizontalScrollbar, bool hasVerticalScrollbar)
        : m_arguments(hasHorizontalScrollbar, hasVerticalScrollbar)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidChangeScrollOffsetPinningForMainFrame {
public:
    typedef std::tuple<bool, bool, bool, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeScrollOffsetPinningForMainFrame"); }
    static const bool isSync = false;

    DidChangeScrollOffsetPinningForMainFrame(bool pinnedToLeftSide, bool pinnedToRightSide, bool pinnedToTopSide, bool pinnedToBottomSide)
        : m_arguments(pinnedToLeftSide, pinnedToRightSide, pinnedToTopSide, pinnedToBottomSide)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidChangePageCount {
public:
    typedef std::tuple<const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangePageCount"); }
    static const bool isSync = false;

    explicit DidChangePageCount(const unsigned& pageCount)
        : m_arguments(pageCount)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PageExtendedBackgroundColorDidChange {
public:
    typedef std::tuple<const WebCore::Color&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PageExtendedBackgroundColorDidChange"); }
    static const bool isSync = false;

    explicit PageExtendedBackgroundColorDidChange(const WebCore::Color& backgroundColor)
        : m_arguments(backgroundColor)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(NETSCAPE_PLUGIN_API)
class DidFailToInitializePlugin {
public:
    typedef std::tuple<const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailToInitializePlugin"); }
    static const bool isSync = false;

    DidFailToInitializePlugin(const String& mimeType, const String& frameURLString, const String& pageURLString)
        : m_arguments(mimeType, frameURLString, pageURLString)
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

#if ENABLE(NETSCAPE_PLUGIN_API)
class DidBlockInsecurePluginVersion {
public:
    typedef std::tuple<const String&, const String&, const String&, const String&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidBlockInsecurePluginVersion"); }
    static const bool isSync = false;

    DidBlockInsecurePluginVersion(const String& mimeType, const String& pluginURLString, const String& frameURLString, const String& pageURLString, bool replacementObscured)
        : m_arguments(mimeType, pluginURLString, frameURLString, pageURLString, replacementObscured)
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

class SetCanShortCircuitHorizontalWheelEvents {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCanShortCircuitHorizontalWheelEvents"); }
    static const bool isSync = false;

    explicit SetCanShortCircuitHorizontalWheelEvents(bool canShortCircuitHorizontalWheelEvents)
        : m_arguments(canShortCircuitHorizontalWheelEvents)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(EFL)
class HandleInputMethodKeydown {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleInputMethodKeydown"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
class PageDidRequestScroll {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PageDidRequestScroll"); }
    static const bool isSync = false;

    explicit PageDidRequestScroll(const WebCore::IntPoint& point)
        : m_arguments(point)
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

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
class PageTransitionViewportReady {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PageTransitionViewportReady"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(COORDINATED_GRAPHICS_MULTIPROCESS)
class DidFindZoomableArea {
public:
    typedef std::tuple<const WebCore::IntPoint&, const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFindZoomableArea"); }
    static const bool isSync = false;

    DidFindZoomableArea(const WebCore::IntPoint& target, const WebCore::IntRect& area)
        : m_arguments(target, area)
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

class DidChangeContentSize {
public:
    typedef std::tuple<const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeContentSize"); }
    static const bool isSync = false;

    explicit DidChangeContentSize(const WebCore::IntSize& newSize)
        : m_arguments(newSize)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(INPUT_TYPE_COLOR)
class ShowColorPicker {
public:
    typedef std::tuple<const WebCore::Color&, const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowColorPicker"); }
    static const bool isSync = false;

    ShowColorPicker(const WebCore::Color& initialColor, const WebCore::IntRect& elementRect)
        : m_arguments(initialColor, elementRect)
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

#if ENABLE(INPUT_TYPE_COLOR)
class SetColorPickerColor {
public:
    typedef std::tuple<const WebCore::Color&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetColorPickerColor"); }
    static const bool isSync = false;

    explicit SetColorPickerColor(const WebCore::Color& color)
        : m_arguments(color)
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

#if ENABLE(INPUT_TYPE_COLOR)
class EndColorPicker {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EndColorPicker"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class WillAddDetailedMessageToConsole {
public:
    typedef std::tuple<const String&, const String&, uint64_t, uint64_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillAddDetailedMessageToConsole"); }
    static const bool isSync = false;

    WillAddDetailedMessageToConsole(const String& src, const String& level, uint64_t line, uint64_t column, const String& message, const String& url)
        : m_arguments(src, level, line, column, message, url)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DecidePolicyForResponseSync {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const WebCore::ResourceResponse&, const WebCore::ResourceRequest&, bool, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DecidePolicyForResponseSync"); }
    static const bool isSync = true;

    typedef std::tuple<bool&, uint64_t&, WebKit::DownloadID&> Reply;
    DecidePolicyForResponseSync(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebCore::ResourceResponse& response, const WebCore::ResourceRequest& request, bool canShowMIMEType, uint64_t listenerID, const WebKit::UserData& userData)
        : m_arguments(frameID, frameSecurityOrigin, response, request, canShowMIMEType, listenerID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DecidePolicyForNavigationAction {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, uint64_t, const WebKit::NavigationActionData&, uint64_t, const WebCore::SecurityOriginData&, const WebCore::ResourceRequest&, const WebCore::ResourceRequest&, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DecidePolicyForNavigationAction"); }
    static const bool isSync = true;

    typedef std::tuple<bool&, uint64_t&, uint64_t&, WebKit::DownloadID&> Reply;
    DecidePolicyForNavigationAction(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const WebKit::NavigationActionData& navigationActionData, uint64_t originatingFrameID, const WebCore::SecurityOriginData& originatingFrameSecurityOrigin, const WebCore::ResourceRequest& originalRequest, const WebCore::ResourceRequest& request, uint64_t listenerID, const WebKit::UserData& userData)
        : m_arguments(frameID, frameSecurityOrigin, navigationID, navigationActionData, originatingFrameID, originatingFrameSecurityOrigin, originalRequest, request, listenerID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DecidePolicyForNewWindowAction {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, const WebKit::NavigationActionData&, const WebCore::ResourceRequest&, const String&, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DecidePolicyForNewWindowAction"); }
    static const bool isSync = false;

    DecidePolicyForNewWindowAction(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, const WebKit::NavigationActionData& navigationActionData, const WebCore::ResourceRequest& request, const String& frameName, uint64_t listenerID, const WebKit::UserData& userData)
        : m_arguments(frameID, frameSecurityOrigin, navigationActionData, request, frameName, listenerID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UnableToImplementPolicy {
public:
    typedef std::tuple<uint64_t, const WebCore::ResourceError&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnableToImplementPolicy"); }
    static const bool isSync = false;

    UnableToImplementPolicy(uint64_t frameID, const WebCore::ResourceError& error, const WebKit::UserData& userData)
        : m_arguments(frameID, error, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidChangeProgress {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeProgress"); }
    static const bool isSync = false;

    explicit DidChangeProgress(double value)
        : m_arguments(value)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishProgress {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishProgress"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidStartProgress {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStartProgress"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetNetworkRequestsInProgress {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetNetworkRequestsInProgress"); }
    static const bool isSync = false;

    explicit SetNetworkRequestsInProgress(bool networkRequestsInProgress)
        : m_arguments(networkRequestsInProgress)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCreateMainFrame {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateMainFrame"); }
    static const bool isSync = false;

    explicit DidCreateMainFrame(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCreateSubframe {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCreateSubframe"); }
    static const bool isSync = false;

    explicit DidCreateSubframe(uint64_t frameID)
        : m_arguments(frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidStartProvisionalLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStartProvisionalLoadForFrame"); }
    static const bool isSync = false;

    DidStartProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& url, const String& unreachableURL, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, url, unreachableURL, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveServerRedirectForProvisionalLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveServerRedirectForProvisionalLoadForFrame"); }
    static const bool isSync = false;

    DidReceiveServerRedirectForProvisionalLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& url, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, url, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidChangeProvisionalURLForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeProvisionalURLForFrame"); }
    static const bool isSync = false;

    DidChangeProvisionalURLForFrame(uint64_t frameID, uint64_t navigationID, const String& url)
        : m_arguments(frameID, navigationID, url)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailProvisionalLoadForFrame {
public:
    typedef std::tuple<uint64_t, const WebCore::SecurityOriginData&, uint64_t, const String&, const WebCore::ResourceError&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailProvisionalLoadForFrame"); }
    static const bool isSync = false;

    DidFailProvisionalLoadForFrame(uint64_t frameID, const WebCore::SecurityOriginData& frameSecurityOrigin, uint64_t navigationID, const String& provisionalURL, const WebCore::ResourceError& error, const WebKit::UserData& userData)
        : m_arguments(frameID, frameSecurityOrigin, navigationID, provisionalURL, error, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCommitLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, bool, uint32_t, const WebCore::CertificateInfo&, bool, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCommitLoadForFrame"); }
    static const bool isSync = false;

    DidCommitLoadForFrame(uint64_t frameID, uint64_t navigationID, const String& mimeType, bool hasCustomContentProvider, uint32_t loadType, const WebCore::CertificateInfo& certificateInfo, bool containsPluginDocument, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, mimeType, hasCustomContentProvider, loadType, certificateInfo, containsPluginDocument, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebCore::ResourceError&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailLoadForFrame"); }
    static const bool isSync = false;

    DidFailLoadForFrame(uint64_t frameID, uint64_t navigationID, const WebCore::ResourceError& error, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, error, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishDocumentLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishDocumentLoadForFrame"); }
    static const bool isSync = false;

    DidFinishDocumentLoadForFrame(uint64_t frameID, uint64_t navigationID, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishLoadForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishLoadForFrame"); }
    static const bool isSync = false;

    DidFinishLoadForFrame(uint64_t frameID, uint64_t navigationID, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFirstLayoutForFrame {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFirstLayoutForFrame"); }
    static const bool isSync = false;

    DidFirstLayoutForFrame(uint64_t frameID, const WebKit::UserData& userData)
        : m_arguments(frameID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFirstVisuallyNonEmptyLayoutForFrame {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFirstVisuallyNonEmptyLayoutForFrame"); }
    static const bool isSync = false;

    DidFirstVisuallyNonEmptyLayoutForFrame(uint64_t frameID, const WebKit::UserData& userData)
        : m_arguments(frameID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReachLayoutMilestone {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReachLayoutMilestone"); }
    static const bool isSync = false;

    explicit DidReachLayoutMilestone(uint32_t type)
        : m_arguments(type)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceiveTitleForFrame {
public:
    typedef std::tuple<uint64_t, const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveTitleForFrame"); }
    static const bool isSync = false;

    DidReceiveTitleForFrame(uint64_t frameID, const String& title, const WebKit::UserData& userData)
        : m_arguments(frameID, title, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDisplayInsecureContentForFrame {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDisplayInsecureContentForFrame"); }
    static const bool isSync = false;

    DidDisplayInsecureContentForFrame(uint64_t frameID, const WebKit::UserData& userData)
        : m_arguments(frameID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidRunInsecureContentForFrame {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRunInsecureContentForFrame"); }
    static const bool isSync = false;

    DidRunInsecureContentForFrame(uint64_t frameID, const WebKit::UserData& userData)
        : m_arguments(frameID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDetectXSSForFrame {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDetectXSSForFrame"); }
    static const bool isSync = false;

    DidDetectXSSForFrame(uint64_t frameID, const WebKit::UserData& userData)
        : m_arguments(frameID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidSameDocumentNavigationForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, uint32_t, const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidSameDocumentNavigationForFrame"); }
    static const bool isSync = false;

    DidSameDocumentNavigationForFrame(uint64_t frameID, uint64_t navigationID, uint32_t type, const String& url, const WebKit::UserData& userData)
        : m_arguments(frameID, navigationID, type, url, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDestroyNavigation {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDestroyNavigation"); }
    static const bool isSync = false;

    explicit DidDestroyNavigation(uint64_t navigationID)
        : m_arguments(navigationID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class MainFramePluginHandlesPageScaleGestureDidChange {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MainFramePluginHandlesPageScaleGestureDidChange"); }
    static const bool isSync = false;

    explicit MainFramePluginHandlesPageScaleGestureDidChange(bool mainFramePluginHandlesPageScaleGesture)
        : m_arguments(mainFramePluginHandlesPageScaleGesture)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FrameDidBecomeFrameSet {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FrameDidBecomeFrameSet"); }
    static const bool isSync = false;

    FrameDidBecomeFrameSet(uint64_t frameID, bool value)
        : m_arguments(frameID, value)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidNavigateWithNavigationData {
public:
    typedef std::tuple<const WebKit::WebNavigationDataStore&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidNavigateWithNavigationData"); }
    static const bool isSync = false;

    DidNavigateWithNavigationData(const WebKit::WebNavigationDataStore& store, uint64_t frameID)
        : m_arguments(store, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidPerformClientRedirect {
public:
    typedef std::tuple<const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPerformClientRedirect"); }
    static const bool isSync = false;

    DidPerformClientRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID)
        : m_arguments(sourceURLString, destinationURLString, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidPerformServerRedirect {
public:
    typedef std::tuple<const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPerformServerRedirect"); }
    static const bool isSync = false;

    DidPerformServerRedirect(const String& sourceURLString, const String& destinationURLString, uint64_t frameID)
        : m_arguments(sourceURLString, destinationURLString, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidUpdateHistoryTitle {
public:
    typedef std::tuple<const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateHistoryTitle"); }
    static const bool isSync = false;

    DidUpdateHistoryTitle(const String& title, const String& url, uint64_t frameID)
        : m_arguments(title, url, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFinishLoadingDataForCustomContentProvider {
public:
    typedef std::tuple<const String&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishLoadingDataForCustomContentProvider"); }
    static const bool isSync = false;

    DidFinishLoadingDataForCustomContentProvider(const String& suggestedFilename, const IPC::DataReference& data)
        : m_arguments(suggestedFilename, data)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class WillSubmitForm {
public:
    typedef std::tuple<uint64_t, uint64_t, const Vector<std::pair<String, String>>&, uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillSubmitForm"); }
    static const bool isSync = false;

    WillSubmitForm(uint64_t frameID, uint64_t sourceFrameID, const Vector<std::pair<String, String>>& textFieldValues, uint64_t listenerID, const WebKit::UserData& userData)
        : m_arguments(frameID, sourceFrameID, textFieldValues, listenerID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class VoidCallback {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("VoidCallback"); }
    static const bool isSync = false;

    explicit VoidCallback(uint64_t callbackID)
        : m_arguments(callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DataCallback {
public:
    typedef std::tuple<const IPC::DataReference&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DataCallback"); }
    static const bool isSync = false;

    DataCallback(const IPC::DataReference& resultData, uint64_t callbackID)
        : m_arguments(resultData, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ImageCallback {
public:
    typedef std::tuple<const WebKit::ShareableBitmap::Handle&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ImageCallback"); }
    static const bool isSync = false;

    ImageCallback(const WebKit::ShareableBitmap::Handle& bitmapHandle, uint64_t callbackID)
        : m_arguments(bitmapHandle, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StringCallback {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StringCallback"); }
    static const bool isSync = false;

    StringCallback(const String& resultString, uint64_t callbackID)
        : m_arguments(resultString, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ScriptValueCallback {
public:
    typedef std::tuple<const IPC::DataReference&, bool, const WebCore::ExceptionDetails&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScriptValueCallback"); }
    static const bool isSync = false;

    ScriptValueCallback(const IPC::DataReference& resultData, bool hadException, const WebCore::ExceptionDetails& details, uint64_t callbackID)
        : m_arguments(resultData, hadException, details, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ComputedPagesCallback {
public:
    typedef std::tuple<const Vector<WebCore::IntRect>&, double, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ComputedPagesCallback"); }
    static const bool isSync = false;

    ComputedPagesCallback(const Vector<WebCore::IntRect>& pageRects, double totalScaleFactorForPrinting, uint64_t callbackID)
        : m_arguments(pageRects, totalScaleFactorForPrinting, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ValidateCommandCallback {
public:
    typedef std::tuple<const String&, bool, int32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ValidateCommandCallback"); }
    static const bool isSync = false;

    ValidateCommandCallback(const String& command, bool isEnabled, int32_t state, uint64_t callbackID)
        : m_arguments(command, isEnabled, state, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class EditingRangeCallback {
public:
    typedef std::tuple<const WebKit::EditingRange&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EditingRangeCallback"); }
    static const bool isSync = false;

    EditingRangeCallback(const WebKit::EditingRange& range, uint64_t callbackID)
        : m_arguments(range, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UnsignedCallback {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnsignedCallback"); }
    static const bool isSync = false;

    UnsignedCallback(uint64_t result, uint64_t callbackID)
        : m_arguments(result, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RectForCharacterRangeCallback {
public:
    typedef std::tuple<const WebCore::IntRect&, const WebKit::EditingRange&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RectForCharacterRangeCallback"); }
    static const bool isSync = false;

    RectForCharacterRangeCallback(const WebCore::IntRect& rect, const WebKit::EditingRange& actualRange, uint64_t callbackID)
        : m_arguments(rect, actualRange, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(MAC)
class AttributedStringForCharacterRangeCallback {
public:
    typedef std::tuple<const WebKit::AttributedString&, const WebKit::EditingRange&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AttributedStringForCharacterRangeCallback"); }
    static const bool isSync = false;

    AttributedStringForCharacterRangeCallback(const WebKit::AttributedString& string, const WebKit::EditingRange& actualRange, uint64_t callbackID)
        : m_arguments(string, actualRange, callbackID)
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

#if PLATFORM(MAC)
class FontAtSelectionCallback {
public:
    typedef std::tuple<const String&, double, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FontAtSelectionCallback"); }
    static const bool isSync = false;

    FontAtSelectionCallback(const String& fontName, double fontSize, bool selectioHasMultipleFonts, uint64_t callbackID)
        : m_arguments(fontName, fontSize, selectioHasMultipleFonts, callbackID)
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

#if PLATFORM(IOS)
class GestureCallback {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GestureCallback"); }
    static const bool isSync = false;

    GestureCallback(const WebCore::IntPoint& point, uint32_t gestureType, uint32_t gestureState, uint32_t flags, uint64_t callbackID)
        : m_arguments(point, gestureType, gestureState, flags, callbackID)
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

#if PLATFORM(IOS)
class TouchesCallback {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TouchesCallback"); }
    static const bool isSync = false;

    TouchesCallback(const WebCore::IntPoint& point, uint32_t touches, uint32_t flags, uint64_t callbackID)
        : m_arguments(point, touches, flags, callbackID)
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

#if PLATFORM(IOS)
class AutocorrectionDataCallback {
public:
    typedef std::tuple<const Vector<WebCore::FloatRect>&, const String&, double, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AutocorrectionDataCallback"); }
    static const bool isSync = false;

    AutocorrectionDataCallback(const Vector<WebCore::FloatRect>& textRects, const String& fontName, double fontSize, uint64_t traits, uint64_t callbackID)
        : m_arguments(textRects, fontName, fontSize, traits, callbackID)
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

#if PLATFORM(IOS)
class AutocorrectionContextCallback {
public:
    typedef std::tuple<const String&, const String&, const String&, const String&, uint64_t, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AutocorrectionContextCallback"); }
    static const bool isSync = false;

    AutocorrectionContextCallback(const String& beforeText, const String& markedText, const String& selectedText, const String& afterText, uint64_t location, uint64_t length, uint64_t callbackID)
        : m_arguments(beforeText, markedText, selectedText, afterText, location, length, callbackID)
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

#if PLATFORM(IOS)
class SelectionContextCallback {
public:
    typedef std::tuple<const String&, const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectionContextCallback"); }
    static const bool isSync = false;

    SelectionContextCallback(const String& selectedText, const String& beforeText, const String& afterText, uint64_t callbackID)
        : m_arguments(selectedText, beforeText, afterText, callbackID)
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

#if PLATFORM(IOS)
class InterpretKeyEvent {
public:
    typedef std::tuple<const WebKit::EditorState&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InterpretKeyEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    InterpretKeyEvent(const WebKit::EditorState& state, bool isCharEvent)
        : m_arguments(state, isCharEvent)
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

#if PLATFORM(IOS)
class DidReceivePositionInformation {
public:
    typedef std::tuple<const WebKit::InteractionInformationAtPosition&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceivePositionInformation"); }
    static const bool isSync = false;

    explicit DidReceivePositionInformation(const WebKit::InteractionInformationAtPosition& information)
        : m_arguments(information)
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

#if PLATFORM(IOS)
class SaveImageToLibrary {
public:
    typedef std::tuple<const WebKit::SharedMemory::Handle&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SaveImageToLibrary"); }
    static const bool isSync = false;

    SaveImageToLibrary(const WebKit::SharedMemory::Handle& handle, uint64_t size)
        : m_arguments(handle, size)
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

#if PLATFORM(IOS)
class DidUpdateBlockSelectionWithTouch {
public:
    typedef std::tuple<uint32_t, uint32_t, float, float> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateBlockSelectionWithTouch"); }
    static const bool isSync = false;

    DidUpdateBlockSelectionWithTouch(uint32_t touch, uint32_t flags, float growThreshold, float shrinkThreshold)
        : m_arguments(touch, flags, growThreshold, shrinkThreshold)
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

#if PLATFORM(IOS)
class ShowPlaybackTargetPicker {
public:
    typedef std::tuple<bool, const WebCore::IntRect&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowPlaybackTargetPicker"); }
    static const bool isSync = false;

    ShowPlaybackTargetPicker(bool hasVideo, const WebCore::IntRect& elementRect)
        : m_arguments(hasVideo, elementRect)
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

#if PLATFORM(IOS)
class ZoomToRect {
public:
    typedef std::tuple<const WebCore::FloatRect&, double, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ZoomToRect"); }
    static const bool isSync = false;

    ZoomToRect(const WebCore::FloatRect& rect, double minimumScale, double maximumScale)
        : m_arguments(rect, minimumScale, maximumScale)
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

#if PLATFORM(IOS)
class CommitPotentialTapFailed {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CommitPotentialTapFailed"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class DidNotHandleTapAsClick {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidNotHandleTapAsClick"); }
    static const bool isSync = false;

    explicit DidNotHandleTapAsClick(const WebCore::IntPoint& point)
        : m_arguments(point)
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

#if PLATFORM(IOS)
class DisableDoubleTapGesturesDuringTapIfNecessary {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DisableDoubleTapGesturesDuringTapIfNecessary"); }
    static const bool isSync = false;

    explicit DisableDoubleTapGesturesDuringTapIfNecessary(uint64_t requestID)
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
#endif

#if PLATFORM(IOS)
class DrawToPDFCallback {
public:
    typedef std::tuple<const IPC::DataReference&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DrawToPDFCallback"); }
    static const bool isSync = false;

    DrawToPDFCallback(const IPC::DataReference& pdfData, uint64_t callbackID)
        : m_arguments(pdfData, callbackID)
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

#if ENABLE(DATA_DETECTION)
class SetDataDetectionResult {
public:
    typedef std::tuple<const WebKit::DataDetectionResult&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDataDetectionResult"); }
    static const bool isSync = false;

    explicit SetDataDetectionResult(const WebKit::DataDetectionResult& dataDetectionResult)
        : m_arguments(dataDetectionResult)
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

#if PLATFORM(GTK)
class PrintFinishedCallback {
public:
    typedef std::tuple<const WebCore::ResourceError&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PrintFinishedCallback"); }
    static const bool isSync = false;

    PrintFinishedCallback(const WebCore::ResourceError& error, uint64_t callbackID)
        : m_arguments(error, callbackID)
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
class MachSendRightCallback {
public:
    typedef std::tuple<const WebCore::MachSendRight&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MachSendRightCallback"); }
    static const bool isSync = false;

    MachSendRightCallback(const WebCore::MachSendRight& sendRight, uint64_t callbackID)
        : m_arguments(sendRight, callbackID)
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

class PageScaleFactorDidChange {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PageScaleFactorDidChange"); }
    static const bool isSync = false;

    explicit PageScaleFactorDidChange(double scaleFactor)
        : m_arguments(scaleFactor)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PluginScaleFactorDidChange {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PluginScaleFactorDidChange"); }
    static const bool isSync = false;

    explicit PluginScaleFactorDidChange(double zoomFactor)
        : m_arguments(zoomFactor)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PluginZoomFactorDidChange {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PluginZoomFactorDidChange"); }
    static const bool isSync = false;

    explicit PluginZoomFactorDidChange(double zoomFactor)
        : m_arguments(zoomFactor)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(GTK)
class BindAccessibilityTree {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BindAccessibilityTree"); }
    static const bool isSync = false;

    explicit BindAccessibilityTree(const String& plugID)
        : m_arguments(plugID)
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

#if PLATFORM(GTK)
class SetInputMethodState {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetInputMethodState"); }
    static const bool isSync = false;

    explicit SetInputMethodState(bool enabled)
        : m_arguments(enabled)
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

class BackForwardAddItem {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardAddItem"); }
    static const bool isSync = false;

    explicit BackForwardAddItem(uint64_t itemID)
        : m_arguments(itemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BackForwardGoToItem {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardGoToItem"); }
    static const bool isSync = true;

    typedef std::tuple<WebKit::SandboxExtension::Handle&> Reply;
    explicit BackForwardGoToItem(uint64_t itemID)
        : m_arguments(itemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BackForwardItemAtIndex {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardItemAtIndex"); }
    static const bool isSync = true;

    typedef std::tuple<uint64_t&> Reply;
    explicit BackForwardItemAtIndex(int32_t itemIndex)
        : m_arguments(itemIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BackForwardBackListCount {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardBackListCount"); }
    static const bool isSync = true;

    typedef std::tuple<int32_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BackForwardForwardListCount {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardForwardListCount"); }
    static const bool isSync = true;

    typedef std::tuple<int32_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BackForwardClear {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BackForwardClear"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class WillGoToBackForwardListItem {
public:
    typedef std::tuple<uint64_t, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillGoToBackForwardListItem"); }
    static const bool isSync = false;

    WillGoToBackForwardListItem(uint64_t itemID, const WebKit::UserData& userData)
        : m_arguments(itemID, userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RegisterEditCommandForUndo {
public:
    typedef std::tuple<uint64_t, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterEditCommandForUndo"); }
    static const bool isSync = false;

    RegisterEditCommandForUndo(uint64_t commandID, uint32_t editAction)
        : m_arguments(commandID, editAction)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClearAllEditCommands {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearAllEditCommands"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RegisterInsertionUndoGrouping {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterInsertionUndoGrouping"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CanUndoRedo {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CanUndoRedo"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit CanUndoRedo(uint32_t action)
        : m_arguments(action)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ExecuteUndoRedo {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExecuteUndoRedo"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit ExecuteUndoRedo(uint32_t action)
        : m_arguments(action)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessage {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessage"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessage(const String& message, const String& description)
        : m_arguments(message, description)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessageWithResult {
public:
    typedef std::tuple<const String&, const String&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessageWithResult"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessageWithResult(const String& message, const String& description, uint32_t result)
        : m_arguments(message, description, result)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LogSampledDiagnosticMessageWithValue {
public:
    typedef std::tuple<const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LogSampledDiagnosticMessageWithValue"); }
    static const bool isSync = false;

    LogSampledDiagnosticMessageWithValue(const String& message, const String& description, const String& value)
        : m_arguments(message, description, value)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class EditorStateChanged {
public:
    typedef std::tuple<const WebKit::EditorState&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EditorStateChanged"); }
    static const bool isSync = false;

    explicit EditorStateChanged(const WebKit::EditorState& editorState)
        : m_arguments(editorState)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CompositionWasCanceled {
public:
    typedef std::tuple<const WebKit::EditorState&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CompositionWasCanceled"); }
    static const bool isSync = false;

    explicit CompositionWasCanceled(const WebKit::EditorState& editorState)
        : m_arguments(editorState)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetHasHadSelectionChangesFromUserInteraction {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetHasHadSelectionChangesFromUserInteraction"); }
    static const bool isSync = false;

    explicit SetHasHadSelectionChangesFromUserInteraction(bool hasHadUserSelectionChanges)
        : m_arguments(hasHadUserSelectionChanges)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCountStringMatches {
public:
    typedef std::tuple<const String&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCountStringMatches"); }
    static const bool isSync = false;

    DidCountStringMatches(const String& string, uint32_t matchCount)
        : m_arguments(string, matchCount)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetTextIndicator {
public:
    typedef std::tuple<const WebCore::TextIndicatorData&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTextIndicator"); }
    static const bool isSync = false;

    SetTextIndicator(const WebCore::TextIndicatorData& indicator, uint64_t lifetime)
        : m_arguments(indicator, lifetime)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClearTextIndicator {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearTextIndicator"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFindString {
public:
    typedef std::tuple<const String&, const Vector<WebCore::IntRect>&, uint32_t, int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFindString"); }
    static const bool isSync = false;

    DidFindString(const String& string, const Vector<WebCore::IntRect>& matchRect, uint32_t matchCount, int32_t matchIndex)
        : m_arguments(string, matchRect, matchCount, matchIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFailToFindString {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFailToFindString"); }
    static const bool isSync = false;

    explicit DidFailToFindString(const String& string)
        : m_arguments(string)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidFindStringMatches {
public:
    typedef std::tuple<const String&, const Vector<Vector<WebCore::IntRect>>&, int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFindStringMatches"); }
    static const bool isSync = false;

    DidFindStringMatches(const String& string, const Vector<Vector<WebCore::IntRect>>& matches, int32_t firstIndexAfterSelection)
        : m_arguments(string, matches, firstIndexAfterSelection)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidGetImageForFindMatch {
public:
    typedef std::tuple<const WebKit::ShareableBitmap::Handle&, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetImageForFindMatch"); }
    static const bool isSync = false;

    DidGetImageForFindMatch(const WebKit::ShareableBitmap::Handle& contentImageHandle, uint32_t matchIndex)
        : m_arguments(contentImageHandle, matchIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ShowPopupMenu {
public:
    typedef std::tuple<const WebCore::IntRect&, uint64_t, const Vector<WebKit::WebPopupItem>&, int32_t, const WebKit::PlatformPopupMenuData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowPopupMenu"); }
    static const bool isSync = false;

    ShowPopupMenu(const WebCore::IntRect& rect, uint64_t textDirection, const Vector<WebKit::WebPopupItem>& items, int32_t selectedIndex, const WebKit::PlatformPopupMenuData& data)
        : m_arguments(rect, textDirection, items, selectedIndex, data)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HidePopupMenu {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HidePopupMenu"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(CONTEXT_MENUS)
class ShowContextMenu {
public:
    typedef std::tuple<const WebKit::ContextMenuContextData&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowContextMenu"); }
    static const bool isSync = false;

    ShowContextMenu(const WebKit::ContextMenuContextData& contextMenuContextData, const WebKit::UserData& userData)
        : m_arguments(contextMenuContextData, userData)
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

class DidReceiveAuthenticationChallenge {
public:
    typedef std::tuple<uint64_t, const WebCore::AuthenticationChallenge&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveAuthenticationChallenge"); }
    static const bool isSync = false;

    DidReceiveAuthenticationChallenge(uint64_t frameID, const WebCore::AuthenticationChallenge& challenge, uint64_t challengeID)
        : m_arguments(frameID, challenge, challengeID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ExceededDatabaseQuota {
public:
    typedef std::tuple<uint64_t, const String&, const String&, const String&, uint64_t, uint64_t, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExceededDatabaseQuota"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(uint64_t newQuota);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<uint64_t&> Reply;
    ExceededDatabaseQuota(uint64_t frameID, const String& originIdentifier, const String& databaseName, const String& databaseDisplayName, uint64_t currentQuota, uint64_t currentOriginUsage, uint64_t currentDatabaseUsage, uint64_t expectedUsage)
        : m_arguments(frameID, originIdentifier, databaseName, databaseDisplayName, currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ReachedApplicationCacheOriginQuota {
public:
    typedef std::tuple<const String&, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReachedApplicationCacheOriginQuota"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(uint64_t newQuota);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<uint64_t&> Reply;
    ReachedApplicationCacheOriginQuota(const String& originIdentifier, uint64_t currentQuota, uint64_t totalBytesNeeded)
        : m_arguments(originIdentifier, currentQuota, totalBytesNeeded)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RequestGeolocationPermissionForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestGeolocationPermissionForFrame"); }
    static const bool isSync = false;

    RequestGeolocationPermissionForFrame(uint64_t geolocationID, uint64_t frameID, const String& originIdentifier)
        : m_arguments(geolocationID, frameID, originIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(MEDIA_STREAM)
class RequestUserMediaPermissionForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, const String&, const Vector<String>&, const Vector<String>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestUserMediaPermissionForFrame"); }
    static const bool isSync = false;

    RequestUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, const String& userMediaDocumentOriginIdentifier, const String& topLevelDocumentOriginIdentifier, const Vector<String>& audioDeviceUIDs, const Vector<String>& videoDeviceUIDs)
        : m_arguments(userMediaID, frameID, userMediaDocumentOriginIdentifier, topLevelDocumentOriginIdentifier, audioDeviceUIDs, videoDeviceUIDs)
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

#if ENABLE(MEDIA_STREAM)
class CheckUserMediaPermissionForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CheckUserMediaPermissionForFrame"); }
    static const bool isSync = false;

    CheckUserMediaPermissionForFrame(uint64_t userMediaID, uint64_t frameID, const String& userMediaDocumentOriginIdentifier, const String& topLevelDocumentOriginIdentifier)
        : m_arguments(userMediaID, frameID, userMediaDocumentOriginIdentifier, topLevelDocumentOriginIdentifier)
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

class RequestNotificationPermission {
public:
    typedef std::tuple<uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestNotificationPermission"); }
    static const bool isSync = false;

    RequestNotificationPermission(uint64_t requestID, const String& originIdentifier)
        : m_arguments(requestID, originIdentifier)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ShowNotification {
public:
    typedef std::tuple<const String&, const String&, const String&, const String&, const String&, const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowNotification"); }
    static const bool isSync = false;

    ShowNotification(const String& title, const String& body, const String& iconURL, const String& tag, const String& lang, const String& dir, const String& originIdentifier, uint64_t notificationID)
        : m_arguments(title, body, iconURL, tag, lang, dir, originIdentifier, notificationID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CancelNotification {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelNotification"); }
    static const bool isSync = false;

    explicit CancelNotification(uint64_t notificationID)
        : m_arguments(notificationID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClearNotifications {
public:
    typedef std::tuple<const Vector<uint64_t>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearNotifications"); }
    static const bool isSync = false;

    explicit ClearNotifications(const Vector<uint64_t>& notificationIDs)
        : m_arguments(notificationIDs)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidDestroyNotification {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidDestroyNotification"); }
    static const bool isSync = false;

    explicit DidDestroyNotification(uint64_t notificationID)
        : m_arguments(notificationID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if USE(UNIFIED_TEXT_CHECKING)
class CheckTextOfParagraph {
public:
    typedef std::tuple<const String&, uint64_t, int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CheckTextOfParagraph"); }
    static const bool isSync = true;

    typedef std::tuple<Vector<WebCore::TextCheckingResult>&> Reply;
    CheckTextOfParagraph(const String& text, uint64_t checkingTypes, int32_t insertionPoint)
        : m_arguments(text, checkingTypes, insertionPoint)
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

class CheckSpellingOfString {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CheckSpellingOfString"); }
    static const bool isSync = true;

    typedef std::tuple<int32_t&, int32_t&> Reply;
    explicit CheckSpellingOfString(const String& text)
        : m_arguments(text)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CheckGrammarOfString {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CheckGrammarOfString"); }
    static const bool isSync = true;

    typedef std::tuple<Vector<WebCore::GrammarDetail>&, int32_t&, int32_t&> Reply;
    explicit CheckGrammarOfString(const String& text)
        : m_arguments(text)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SpellingUIIsShowing {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SpellingUIIsShowing"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UpdateSpellingUIWithMisspelledWord {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateSpellingUIWithMisspelledWord"); }
    static const bool isSync = false;

    explicit UpdateSpellingUIWithMisspelledWord(const String& misspelledWord)
        : m_arguments(misspelledWord)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UpdateSpellingUIWithGrammarString {
public:
    typedef std::tuple<const String&, const WebCore::GrammarDetail&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateSpellingUIWithGrammarString"); }
    static const bool isSync = false;

    UpdateSpellingUIWithGrammarString(const String& badGrammarPhrase, const WebCore::GrammarDetail& grammarDetail)
        : m_arguments(badGrammarPhrase, grammarDetail)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetGuessesForWord {
public:
    typedef std::tuple<const String&, const String&, int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetGuessesForWord"); }
    static const bool isSync = true;

    typedef std::tuple<Vector<String>&> Reply;
    GetGuessesForWord(const String& word, const String& context, int32_t insertionPoint)
        : m_arguments(word, context, insertionPoint)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LearnWord {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LearnWord"); }
    static const bool isSync = false;

    explicit LearnWord(const String& word)
        : m_arguments(word)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class IgnoreWord {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("IgnoreWord"); }
    static const bool isSync = false;

    explicit IgnoreWord(const String& word)
        : m_arguments(word)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RequestCheckingOfString {
public:
    typedef std::tuple<uint64_t, const WebCore::TextCheckingRequestData&, int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestCheckingOfString"); }
    static const bool isSync = false;

    RequestCheckingOfString(uint64_t requestID, const WebCore::TextCheckingRequestData& request, int32_t insertionPoint)
        : m_arguments(requestID, request, insertionPoint)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(DRAG_SUPPORT)
class DidPerformDragControllerAction {
public:
    typedef std::tuple<uint64_t, bool, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPerformDragControllerAction"); }
    static const bool isSync = false;

    DidPerformDragControllerAction(uint64_t dragOperation, bool mouseIsOverFileInput, const unsigned& numberOfItemsToBeAccepted)
        : m_arguments(dragOperation, mouseIsOverFileInput, numberOfItemsToBeAccepted)
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

#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
class SetDragImage {
public:
    typedef std::tuple<const WebCore::IntPoint&, const WebKit::ShareableBitmap::Handle&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDragImage"); }
    static const bool isSync = false;

    SetDragImage(const WebCore::IntPoint& clientPosition, const WebKit::ShareableBitmap::Handle& dragImage, bool linkDrag)
        : m_arguments(clientPosition, dragImage, linkDrag)
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

#if PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)
class SetPromisedDataForImage {
public:
    typedef std::tuple<const String&, const WebKit::SharedMemory::Handle&, uint64_t, const String&, const String&, const String&, const String&, const String&, const WebKit::SharedMemory::Handle&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPromisedDataForImage"); }
    static const bool isSync = false;

    SetPromisedDataForImage(const String& pasteboardName, const WebKit::SharedMemory::Handle& imageHandle, uint64_t imageSize, const String& filename, const String& extension, const String& title, const String& url, const String& visibleURL, const WebKit::SharedMemory::Handle& archiveHandle, uint64_t archiveSize)
        : m_arguments(pasteboardName, imageHandle, imageSize, filename, extension, title, url, visibleURL, archiveHandle, archiveSize)
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

#if ((PLATFORM(COCOA) && ENABLE(DRAG_SUPPORT)) && ENABLE(ATTACHMENT_ELEMENT))
class SetPromisedDataForAttachment {
public:
    typedef std::tuple<const String&, const String&, const String&, const String&, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPromisedDataForAttachment"); }
    static const bool isSync = false;

    SetPromisedDataForAttachment(const String& pasteboardName, const String& filename, const String& extension, const String& title, const String& url, const String& visibleURL)
        : m_arguments(pasteboardName, filename, extension, title, url, visibleURL)
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

#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
class StartDrag {
public:
    typedef std::tuple<const WebCore::DragData&, const WebKit::ShareableBitmap::Handle&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartDrag"); }
    static const bool isSync = false;

    StartDrag(const WebCore::DragData& dragData, const WebKit::ShareableBitmap::Handle& dragImage)
        : m_arguments(dragData, dragImage)
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
class DidPerformDictionaryLookup {
public:
    typedef std::tuple<const WebCore::DictionaryPopupInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPerformDictionaryLookup"); }
    static const bool isSync = false;

    explicit DidPerformDictionaryLookup(const WebCore::DictionaryPopupInfo& dictionaryPopupInfo)
        : m_arguments(dictionaryPopupInfo)
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
class ExecuteSavedCommandBySelector {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExecuteSavedCommandBySelector"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit ExecuteSavedCommandBySelector(const String& selector)
        : m_arguments(selector)
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
class RegisterWebProcessAccessibilityToken {
public:
    typedef std::tuple<const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterWebProcessAccessibilityToken"); }
    static const bool isSync = false;

    explicit RegisterWebProcessAccessibilityToken(const IPC::DataReference& data)
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
#endif

#if PLATFORM(COCOA)
class PluginFocusOrWindowFocusChanged {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PluginFocusOrWindowFocusChanged"); }
    static const bool isSync = false;

    PluginFocusOrWindowFocusChanged(uint64_t pluginComplexTextInputIdentifier, bool pluginHasFocusAndWindowHasFocus)
        : m_arguments(pluginComplexTextInputIdentifier, pluginHasFocusAndWindowHasFocus)
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
class SetPluginComplexTextInputState {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPluginComplexTextInputState"); }
    static const bool isSync = false;

    SetPluginComplexTextInputState(uint64_t pluginComplexTextInputIdentifier, uint64_t complexTextInputState)
        : m_arguments(pluginComplexTextInputIdentifier, complexTextInputState)
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
class GetIsSpeaking {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetIsSpeaking"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class Speak {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Speak"); }
    static const bool isSync = false;

    explicit Speak(const String& string)
        : m_arguments(string)
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
class StopSpeaking {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopSpeaking"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class MakeFirstResponder {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MakeFirstResponder"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class SearchWithSpotlight {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SearchWithSpotlight"); }
    static const bool isSync = false;

    explicit SearchWithSpotlight(const String& string)
        : m_arguments(string)
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
class SearchTheWeb {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SearchTheWeb"); }
    static const bool isSync = false;

    explicit SearchTheWeb(const String& string)
        : m_arguments(string)
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

#if USE(APPKIT)
class SubstitutionsPanelIsShowing {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SubstitutionsPanelIsShowing"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class ShowCorrectionPanel {
public:
    typedef std::tuple<int32_t, const WebCore::FloatRect&, const String&, const String&, const Vector<String>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowCorrectionPanel"); }
    static const bool isSync = false;

    ShowCorrectionPanel(int32_t panelType, const WebCore::FloatRect& boundingBoxOfReplacedString, const String& replacedString, const String& replacementString, const Vector<String>& alternativeReplacementStrings)
        : m_arguments(panelType, boundingBoxOfReplacedString, replacedString, replacementString, alternativeReplacementStrings)
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

#if PLATFORM(MAC)
class DismissCorrectionPanel {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DismissCorrectionPanel"); }
    static const bool isSync = false;

    explicit DismissCorrectionPanel(int32_t reason)
        : m_arguments(reason)
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

#if PLATFORM(MAC)
class DismissCorrectionPanelSoon {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DismissCorrectionPanelSoon"); }
    static const bool isSync = true;

    typedef std::tuple<String&> Reply;
    explicit DismissCorrectionPanelSoon(int32_t reason)
        : m_arguments(reason)
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

#if PLATFORM(MAC)
class RecordAutocorrectionResponse {
public:
    typedef std::tuple<int32_t, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RecordAutocorrectionResponse"); }
    static const bool isSync = false;

    RecordAutocorrectionResponse(int32_t responseType, const String& replacedString, const String& replacementString)
        : m_arguments(responseType, replacedString, replacementString)
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

#if PLATFORM(MAC)
class SetEditableElementIsFocused {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEditableElementIsFocused"); }
    static const bool isSync = false;

    explicit SetEditableElementIsFocused(bool editableElementIsFocused)
        : m_arguments(editableElementIsFocused)
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

#if USE(DICTATION_ALTERNATIVES)
class ShowDictationAlternativeUI {
public:
    typedef std::tuple<const WebCore::FloatRect&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowDictationAlternativeUI"); }
    static const bool isSync = false;

    ShowDictationAlternativeUI(const WebCore::FloatRect& boundingBoxOfDictatedText, uint64_t dictationContext)
        : m_arguments(boundingBoxOfDictatedText, dictationContext)
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

#if USE(DICTATION_ALTERNATIVES)
class RemoveDictationAlternatives {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemoveDictationAlternatives"); }
    static const bool isSync = false;

    explicit RemoveDictationAlternatives(uint64_t dictationContext)
        : m_arguments(dictationContext)
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

#if USE(DICTATION_ALTERNATIVES)
class DictationAlternatives {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DictationAlternatives"); }
    static const bool isSync = true;

    typedef std::tuple<Vector<String>&> Reply;
    explicit DictationAlternatives(uint64_t dictationContext)
        : m_arguments(dictationContext)
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

#if PLUGIN_ARCHITECTURE(X11)
class CreatePluginContainer {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CreatePluginContainer"); }
    static const bool isSync = true;

    typedef std::tuple<uint64_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLUGIN_ARCHITECTURE(X11)
class WindowedPluginGeometryDidChange {
public:
    typedef std::tuple<const WebCore::IntRect&, const WebCore::IntRect&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowedPluginGeometryDidChange"); }
    static const bool isSync = false;

    WindowedPluginGeometryDidChange(const WebCore::IntRect& frameRect, const WebCore::IntRect& clipRect, uint64_t windowID)
        : m_arguments(frameRect, clipRect, windowID)
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

#if PLUGIN_ARCHITECTURE(X11)
class WindowedPluginVisibilityDidChange {
public:
    typedef std::tuple<bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowedPluginVisibilityDidChange"); }
    static const bool isSync = false;

    WindowedPluginVisibilityDidChange(bool isVisible, uint64_t windowID)
        : m_arguments(isVisible, windowID)
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

#if PLATFORM(IOS)
class DynamicViewportUpdateChangedTarget {
public:
    typedef std::tuple<double, const WebCore::FloatPoint&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DynamicViewportUpdateChangedTarget"); }
    static const bool isSync = false;

    DynamicViewportUpdateChangedTarget(double newTargetScale, const WebCore::FloatPoint& newScrollPosition, uint64_t dynamicViewportSizeUpdateID)
        : m_arguments(newTargetScale, newScrollPosition, dynamicViewportSizeUpdateID)
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

#if PLATFORM(IOS)
class CouldNotRestorePageState {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CouldNotRestorePageState"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class RestorePageState {
public:
    typedef std::tuple<const WebCore::FloatPoint&, const WebCore::FloatPoint&, const WebCore::FloatSize&, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RestorePageState"); }
    static const bool isSync = false;

    RestorePageState(const WebCore::FloatPoint& scrollPosition, const WebCore::FloatPoint& scrollOrigin, const WebCore::FloatSize& obscuredInsetOnSave, double scale)
        : m_arguments(scrollPosition, scrollOrigin, obscuredInsetOnSave, scale)
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

#if PLATFORM(IOS)
class RestorePageCenterAndScale {
public:
    typedef std::tuple<const WebCore::FloatPoint&, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RestorePageCenterAndScale"); }
    static const bool isSync = false;

    RestorePageCenterAndScale(const WebCore::FloatPoint& unobscuredCenter, double scale)
        : m_arguments(unobscuredCenter, scale)
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

#if PLATFORM(IOS)
class DidGetTapHighlightGeometries {
public:
    typedef std::tuple<uint64_t, const WebCore::Color&, const Vector<WebCore::FloatQuad>&, const WebCore::IntSize&, const WebCore::IntSize&, const WebCore::IntSize&, const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidGetTapHighlightGeometries"); }
    static const bool isSync = false;

    DidGetTapHighlightGeometries(uint64_t requestID, const WebCore::Color& color, const Vector<WebCore::FloatQuad>& geometries, const WebCore::IntSize& topLeftRadius, const WebCore::IntSize& topRightRadius, const WebCore::IntSize& bottomLeftRadius, const WebCore::IntSize& bottomRightRadius)
        : m_arguments(requestID, color, geometries, topLeftRadius, topRightRadius, bottomLeftRadius, bottomRightRadius)
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

#if PLATFORM(IOS)
class StartAssistingNode {
public:
    typedef std::tuple<const WebKit::AssistedNodeInformation&, bool, bool, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartAssistingNode"); }
    static const bool isSync = false;

    StartAssistingNode(const WebKit::AssistedNodeInformation& information, bool userIsInteracting, bool blurPreviousNode, const WebKit::UserData& userData)
        : m_arguments(information, userIsInteracting, blurPreviousNode, userData)
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

#if PLATFORM(IOS)
class StopAssistingNode {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopAssistingNode"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class OverflowScrollWillStartScroll {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OverflowScrollWillStartScroll"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class OverflowScrollDidEndScroll {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OverflowScrollDidEndScroll"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class ShowInspectorHighlight {
public:
    typedef std::tuple<const WebCore::Highlight&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowInspectorHighlight"); }
    static const bool isSync = false;

    explicit ShowInspectorHighlight(const WebCore::Highlight& highlight)
        : m_arguments(highlight)
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

#if PLATFORM(IOS)
class HideInspectorHighlight {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HideInspectorHighlight"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class ShowInspectorIndication {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowInspectorIndication"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class HideInspectorIndication {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HideInspectorIndication"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class EnableInspectorNodeSearch {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnableInspectorNodeSearch"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class DisableInspectorNodeSearch {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DisableInspectorNodeSearch"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class SaveRecentSearches {
public:
    typedef std::tuple<const String&, const Vector<WebCore::RecentSearch>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SaveRecentSearches"); }
    static const bool isSync = false;

    SaveRecentSearches(const String& name, const Vector<WebCore::RecentSearch>& searchItems)
        : m_arguments(name, searchItems)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadRecentSearches {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadRecentSearches"); }
    static const bool isSync = true;

    typedef std::tuple<Vector<WebCore::RecentSearch>&> Reply;
    explicit LoadRecentSearches(const String& name)
        : m_arguments(name)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SavePDFToFileInDownloadsFolder {
public:
    typedef std::tuple<const String&, const String&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SavePDFToFileInDownloadsFolder"); }
    static const bool isSync = false;

    SavePDFToFileInDownloadsFolder(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference& data)
        : m_arguments(suggestedFilename, originatingURLString, data)
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
class SavePDFToTemporaryFolderAndOpenWithNativeApplication {
public:
    typedef std::tuple<const String&, const String&, const IPC::DataReference&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SavePDFToTemporaryFolderAndOpenWithNativeApplication"); }
    static const bool isSync = false;

    SavePDFToTemporaryFolderAndOpenWithNativeApplication(const String& suggestedFilename, const String& originatingURLString, const IPC::DataReference& data, const String& pdfUUID)
        : m_arguments(suggestedFilename, originatingURLString, data, pdfUUID)
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
class OpenPDFFromTemporaryFolderWithNativeApplication {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("OpenPDFFromTemporaryFolderWithNativeApplication"); }
    static const bool isSync = false;

    explicit OpenPDFFromTemporaryFolderWithNativeApplication(const String& pdfUUID)
        : m_arguments(pdfUUID)
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

#if ENABLE(NETSCAPE_PLUGIN_API)
class FindPlugin {
public:
    typedef std::tuple<const String&, uint32_t, const String&, const String&, const String&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FindPlugin"); }
    static const bool isSync = true;

    typedef std::tuple<uint64_t&, String&, uint32_t&, String&> Reply;
    FindPlugin(const String& mimeType, uint32_t processType, const String& urlString, const String& frameURLString, const String& pageURLString, bool allowOnlyApplicationPlugins)
        : m_arguments(mimeType, processType, urlString, frameURLString, pageURLString, allowOnlyApplicationPlugins)
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

class DidUpdateViewState {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateViewState"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidSaveToPageCache {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidSaveToPageCache"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(SUBTLE_CRYPTO)
class WrapCryptoKey {
public:
    typedef std::tuple<const Vector<uint8_t>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WrapCryptoKey"); }
    static const bool isSync = true;

    typedef std::tuple<bool&, Vector<uint8_t>&> Reply;
    explicit WrapCryptoKey(const Vector<uint8_t>& key)
        : m_arguments(key)
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

#if ENABLE(SUBTLE_CRYPTO)
class UnwrapCryptoKey {
public:
    typedef std::tuple<const Vector<uint8_t>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnwrapCryptoKey"); }
    static const bool isSync = true;

    typedef std::tuple<bool&, Vector<uint8_t>&> Reply;
    explicit UnwrapCryptoKey(const Vector<uint8_t>& wrappedKey)
        : m_arguments(wrappedKey)
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

#if (ENABLE(TELEPHONE_NUMBER_DETECTION) && PLATFORM(MAC))
class ShowTelephoneNumberMenu {
public:
    typedef std::tuple<const String&, const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowTelephoneNumberMenu"); }
    static const bool isSync = false;

    ShowTelephoneNumberMenu(const String& telephoneNumber, const WebCore::IntPoint& point)
        : m_arguments(telephoneNumber, point)
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

#if USE(QUICK_LOOK)
class DidStartLoadForQuickLookDocumentInMainFrame {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStartLoadForQuickLookDocumentInMainFrame"); }
    static const bool isSync = false;

    DidStartLoadForQuickLookDocumentInMainFrame(const String& fileName, const String& uti)
        : m_arguments(fileName, uti)
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

#if USE(QUICK_LOOK)
class DidFinishLoadForQuickLookDocumentInMainFrame {
public:
    typedef std::tuple<const WebKit::QuickLookDocumentData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishLoadForQuickLookDocumentInMainFrame"); }
    static const bool isSync = false;

    explicit DidFinishLoadForQuickLookDocumentInMainFrame(const WebKit::QuickLookDocumentData& data)
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
#endif

#if ENABLE(CONTENT_FILTERING)
class ContentFilterDidBlockLoadForFrame {
public:
    typedef std::tuple<const WebCore::ContentFilterUnblockHandler&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContentFilterDidBlockLoadForFrame"); }
    static const bool isSync = false;

    ContentFilterDidBlockLoadForFrame(const WebCore::ContentFilterUnblockHandler& unblockHandler, uint64_t frameID)
        : m_arguments(unblockHandler, frameID)
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

class IsPlayingMediaDidChange {
public:
    typedef std::tuple<const unsigned&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("IsPlayingMediaDidChange"); }
    static const bool isSync = false;

    IsPlayingMediaDidChange(const unsigned& state, uint64_t sourceElementID)
        : m_arguments(state, sourceElementID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(MEDIA_SESSION)
class HasMediaSessionWithActiveMediaElementsDidChange {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HasMediaSessionWithActiveMediaElementsDidChange"); }
    static const bool isSync = false;

    explicit HasMediaSessionWithActiveMediaElementsDidChange(bool state)
        : m_arguments(state)
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

#if ENABLE(MEDIA_SESSION)
class MediaSessionMetadataDidChange {
public:
    typedef std::tuple<const WebCore::MediaSessionMetadata&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MediaSessionMetadataDidChange"); }
    static const bool isSync = false;

    explicit MediaSessionMetadataDidChange(const WebCore::MediaSessionMetadata& metadata)
        : m_arguments(metadata)
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

#if ENABLE(MEDIA_SESSION)
class FocusedContentMediaElementDidChange {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FocusedContentMediaElementDidChange"); }
    static const bool isSync = false;

    explicit FocusedContentMediaElementDidChange(uint64_t elementID)
        : m_arguments(elementID)
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

#if PLATFORM(MAC)
class DidPerformImmediateActionHitTest {
public:
    typedef std::tuple<const WebKit::WebHitTestResultData&, bool, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidPerformImmediateActionHitTest"); }
    static const bool isSync = false;

    DidPerformImmediateActionHitTest(const WebKit::WebHitTestResultData& result, bool contentPreventsDefault, const WebKit::UserData& userData)
        : m_arguments(result, contentPreventsDefault, userData)
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

class HandleMessage {
public:
    typedef std::tuple<const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMessage"); }
    static const bool isSync = false;

    HandleMessage(const String& messageName, const WebKit::UserData& messageBody)
        : m_arguments(messageName, messageBody)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleSynchronousMessage {
public:
    typedef std::tuple<const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleSynchronousMessage"); }
    static const bool isSync = true;

    typedef std::tuple<WebKit::UserData&> Reply;
    HandleSynchronousMessage(const String& messageName, const WebKit::UserData& messageBody)
        : m_arguments(messageName, messageBody)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HandleAutoFillButtonClick {
public:
    typedef std::tuple<const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleAutoFillButtonClick"); }
    static const bool isSync = false;

    explicit HandleAutoFillButtonClick(const WebKit::UserData& userData)
        : m_arguments(userData)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class AddPlaybackTargetPickerClient {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddPlaybackTargetPickerClient"); }
    static const bool isSync = false;

    explicit AddPlaybackTargetPickerClient(uint64_t contextId)
        : m_arguments(contextId)
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

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class RemovePlaybackTargetPickerClient {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RemovePlaybackTargetPickerClient"); }
    static const bool isSync = false;

    explicit RemovePlaybackTargetPickerClient(uint64_t contextId)
        : m_arguments(contextId)
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

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class ShowPlaybackTargetPicker {
public:
    typedef std::tuple<uint64_t, const WebCore::FloatRect&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShowPlaybackTargetPicker"); }
    static const bool isSync = false;

    ShowPlaybackTargetPicker(uint64_t clientId, const WebCore::FloatRect& pickerLocation, bool hasVideo)
        : m_arguments(clientId, pickerLocation, hasVideo)
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

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class PlaybackTargetPickerClientStateDidChange {
public:
    typedef std::tuple<uint64_t, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PlaybackTargetPickerClientStateDidChange"); }
    static const bool isSync = false;

    PlaybackTargetPickerClientStateDidChange(uint64_t contextId, const unsigned& mediaState)
        : m_arguments(contextId, mediaState)
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

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class SetMockMediaPlaybackTargetPickerEnabled {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMockMediaPlaybackTargetPickerEnabled"); }
    static const bool isSync = false;

    explicit SetMockMediaPlaybackTargetPickerEnabled(bool enabled)
        : m_arguments(enabled)
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

#if ENABLE(WIRELESS_PLAYBACK_TARGET) && !PLATFORM(IOS)
class SetMockMediaPlaybackTargetPickerState {
public:
    typedef std::tuple<const String&, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMockMediaPlaybackTargetPickerState"); }
    static const bool isSync = false;

    SetMockMediaPlaybackTargetPickerState(const String& name, const unsigned& pickerState)
        : m_arguments(name, pickerState)
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

class ImageOrMediaDocumentSizeChanged {
public:
    typedef std::tuple<const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ImageOrMediaDocumentSizeChanged"); }
    static const bool isSync = false;

    explicit ImageOrMediaDocumentSizeChanged(const WebCore::IntSize& newSize)
        : m_arguments(newSize)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UseFixedLayoutDidChange {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UseFixedLayoutDidChange"); }
    static const bool isSync = false;

    explicit UseFixedLayoutDidChange(bool useFixedLayout)
        : m_arguments(useFixedLayout)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FixedLayoutSizeDidChange {
public:
    typedef std::tuple<const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FixedLayoutSizeDidChange"); }
    static const bool isSync = false;

    explicit FixedLayoutSizeDidChange(const WebCore::IntSize& fixedLayoutSize)
        : m_arguments(fixedLayoutSize)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(VIDEO) && USE(GSTREAMER)
class RequestInstallMissingMediaPlugins {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestInstallMissingMediaPlugins"); }
    static const bool isSync = false;

    RequestInstallMissingMediaPlugins(const String& details, const String& description)
        : m_arguments(details, description)
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

class DidRestoreScrollPosition {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRestoreScrollPosition"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(MAC)
class DidHandleAcceptedCandidate {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidHandleAcceptedCandidate"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

} // namespace WebPageProxy
} // namespace Messages
