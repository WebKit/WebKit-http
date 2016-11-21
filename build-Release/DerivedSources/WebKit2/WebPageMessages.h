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
#include "GamepadData.h"
#include "SandboxExtension.h"
#include "SessionState.h"
#include "SharedMemory.h"
#include <WebCore/DictationAlternative.h>
#include <WebCore/Editor.h>
#include <WebCore/PageOverlay.h>
#include <WebCore/TextCheckerClient.h>
#include <wtf/Optional.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/Vector.h>
#include <wtf/text/WTFString.h>

namespace IPC {
    class DataReference;
    class Connection;
    class Attachment;
}

namespace WTF {
    class String;
}

namespace WebCore {
    class FloatRect;
    class SessionID;
    class Color;
    struct TextCheckingResult;
    class MediaPlaybackTargetContext;
    class IntRect;
    class DragData;
    class IntPoint;
    class FloatSize;
    class FloatPoint;
    class IntSize;
}

namespace WebKit {
    struct LoadParameters;
    class WebContextMenuItemData;
    class WebTouchEvent;
    struct InteractionInformationAtPosition;
    struct WebPreferencesStore;
    class UserData;
    struct EditingRange;
    struct PrintInfo;
    class DownloadID;
    class WebMouseEvent;
    class WebKeyboardEvent;
}

namespace Messages {
namespace WebPage {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("WebPage");
}

class SetInitialFocus {
public:
    typedef std::tuple<bool, bool, const WebKit::WebKeyboardEvent&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetInitialFocus"); }
    static const bool isSync = false;

    SetInitialFocus(bool forward, bool isKeyboardEventValid, const WebKit::WebKeyboardEvent& event, uint64_t callbackID)
        : m_arguments(forward, isKeyboardEventValid, event, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetViewState {
public:
    typedef std::tuple<const unsigned&, bool, const Vector<uint64_t>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetViewState"); }
    static const bool isSync = false;

    SetViewState(const unsigned& viewState, bool wantsDidUpdateViewState, const Vector<uint64_t>& callbackIDs)
        : m_arguments(viewState, wantsDidUpdateViewState, callbackIDs)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetLayerHostingMode {
public:
    typedef std::tuple<const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetLayerHostingMode"); }
    static const bool isSync = false;

    explicit SetLayerHostingMode(const unsigned& layerHostingMode)
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

class SetSessionID {
public:
    typedef std::tuple<const WebCore::SessionID&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetSessionID"); }
    static const bool isSync = false;

    explicit SetSessionID(const WebCore::SessionID& sessionID)
        : m_arguments(sessionID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetDrawsBackground {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDrawsBackground"); }
    static const bool isSync = false;

    explicit SetDrawsBackground(bool drawsBackground)
        : m_arguments(drawsBackground)
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
class SetTopContentInsetFenced {
public:
    typedef std::tuple<float, const IPC::Attachment&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTopContentInsetFenced"); }
    static const bool isSync = false;

    SetTopContentInsetFenced(float contentInset, const IPC::Attachment& fencePort)
        : m_arguments(contentInset, fencePort)
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

class SetTopContentInset {
public:
    typedef std::tuple<float> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTopContentInset"); }
    static const bool isSync = false;

    explicit SetTopContentInset(float contentInset)
        : m_arguments(contentInset)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetUnderlayColor {
public:
    typedef std::tuple<const WebCore::Color&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetUnderlayColor"); }
    static const bool isSync = false;

    explicit SetUnderlayColor(const WebCore::Color& color)
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

class ViewWillStartLiveResize {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ViewWillStartLiveResize"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ViewWillEndLiveResize {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ViewWillEndLiveResize"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class KeyEvent {
public:
    typedef std::tuple<const WebKit::WebKeyboardEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("KeyEvent"); }
    static const bool isSync = false;

    explicit KeyEvent(const WebKit::WebKeyboardEvent& event)
        : m_arguments(event)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class MouseEvent {
public:
    typedef std::tuple<const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MouseEvent"); }
    static const bool isSync = false;

    explicit MouseEvent(const WebKit::WebMouseEvent& event)
        : m_arguments(event)
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
class SetViewportConfigurationMinimumLayoutSize {
public:
    typedef std::tuple<const WebCore::FloatSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetViewportConfigurationMinimumLayoutSize"); }
    static const bool isSync = false;

    explicit SetViewportConfigurationMinimumLayoutSize(const WebCore::FloatSize& size)
        : m_arguments(size)
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
class SetMaximumUnobscuredSize {
public:
    typedef std::tuple<const WebCore::FloatSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMaximumUnobscuredSize"); }
    static const bool isSync = false;

    explicit SetMaximumUnobscuredSize(const WebCore::FloatSize& size)
        : m_arguments(size)
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
class SetDeviceOrientation {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDeviceOrientation"); }
    static const bool isSync = false;

    explicit SetDeviceOrientation(int32_t deviceOrientation)
        : m_arguments(deviceOrientation)
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
class DynamicViewportSizeUpdate {
public:
    typedef std::tuple<const WebCore::FloatSize&, const WebCore::FloatSize&, const WebCore::FloatRect&, const WebCore::FloatRect&, const WebCore::FloatRect&, double, int32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DynamicViewportSizeUpdate"); }
    static const bool isSync = false;

    DynamicViewportSizeUpdate(const WebCore::FloatSize& minimumLayoutSize, const WebCore::FloatSize& maximumUnobscuredSize, const WebCore::FloatRect& targetExposedContentRect, const WebCore::FloatRect& targetUnobscuredRect, const WebCore::FloatRect& targetUnobscuredRectInScrollViewCoordinates, double scale, int32_t deviceOrientation, uint64_t dynamicViewportSizeUpdateID)
        : m_arguments(minimumLayoutSize, maximumUnobscuredSize, targetExposedContentRect, targetUnobscuredRect, targetUnobscuredRectInScrollViewCoordinates, scale, deviceOrientation, dynamicViewportSizeUpdateID)
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
class SynchronizeDynamicViewportUpdate {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SynchronizeDynamicViewportUpdate"); }
    static const bool isSync = true;

    typedef std::tuple<double&, WebCore::FloatPoint&, uint64_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class HandleTap {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleTap"); }
    static const bool isSync = false;

    HandleTap(const WebCore::IntPoint& point, uint64_t lastLayerTreeTransactionId)
        : m_arguments(point, lastLayerTreeTransactionId)
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
class PotentialTapAtPosition {
public:
    typedef std::tuple<uint64_t, const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PotentialTapAtPosition"); }
    static const bool isSync = false;

    PotentialTapAtPosition(uint64_t requestID, const WebCore::FloatPoint& point)
        : m_arguments(requestID, point)
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
class CommitPotentialTap {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CommitPotentialTap"); }
    static const bool isSync = false;

    explicit CommitPotentialTap(uint64_t lastLayerTreeTransactionId)
        : m_arguments(lastLayerTreeTransactionId)
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
class CancelPotentialTap {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelPotentialTap"); }
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
class TapHighlightAtPosition {
public:
    typedef std::tuple<uint64_t, const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TapHighlightAtPosition"); }
    static const bool isSync = false;

    TapHighlightAtPosition(uint64_t requestID, const WebCore::FloatPoint& point)
        : m_arguments(requestID, point)
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
class InspectorNodeSearchMovedToPosition {
public:
    typedef std::tuple<const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InspectorNodeSearchMovedToPosition"); }
    static const bool isSync = false;

    explicit InspectorNodeSearchMovedToPosition(const WebCore::FloatPoint& point)
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
class InspectorNodeSearchEndedAtPosition {
public:
    typedef std::tuple<const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InspectorNodeSearchEndedAtPosition"); }
    static const bool isSync = false;

    explicit InspectorNodeSearchEndedAtPosition(const WebCore::FloatPoint& point)
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
class BlurAssistedNode {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BlurAssistedNode"); }
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
class SelectWithGesture {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, uint32_t, uint32_t, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectWithGesture"); }
    static const bool isSync = false;

    SelectWithGesture(const WebCore::IntPoint& point, uint32_t granularity, uint32_t gestureType, uint32_t gestureState, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, granularity, gestureType, gestureState, isInteractingWithAssistedNode, callbackID)
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
class UpdateSelectionWithTouches {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateSelectionWithTouches"); }
    static const bool isSync = false;

    UpdateSelectionWithTouches(const WebCore::IntPoint& point, uint32_t touches, bool baseIsStart, uint64_t callbackID)
        : m_arguments(point, touches, baseIsStart, callbackID)
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
class UpdateBlockSelectionWithTouch {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateBlockSelectionWithTouch"); }
    static const bool isSync = false;

    UpdateBlockSelectionWithTouch(const WebCore::IntPoint& point, uint32_t touch, uint32_t handlePosition)
        : m_arguments(point, touch, handlePosition)
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
class SelectWithTwoTouches {
public:
    typedef std::tuple<const WebCore::IntPoint&, const WebCore::IntPoint&, uint32_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectWithTwoTouches"); }
    static const bool isSync = false;

    SelectWithTwoTouches(const WebCore::IntPoint& from, const WebCore::IntPoint& to, uint32_t gestureType, uint32_t gestureState, uint64_t callbackID)
        : m_arguments(from, to, gestureType, gestureState, callbackID)
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
class ExtendSelection {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExtendSelection"); }
    static const bool isSync = false;

    explicit ExtendSelection(uint32_t granularity)
        : m_arguments(granularity)
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
class SelectWordBackward {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectWordBackward"); }
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
class MoveSelectionByOffset {
public:
    typedef std::tuple<int32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MoveSelectionByOffset"); }
    static const bool isSync = false;

    MoveSelectionByOffset(int32_t offset, uint64_t callbackID)
        : m_arguments(offset, callbackID)
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
class SelectTextWithGranularityAtPoint {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectTextWithGranularityAtPoint"); }
    static const bool isSync = false;

    SelectTextWithGranularityAtPoint(const WebCore::IntPoint& point, uint32_t granularity, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, granularity, isInteractingWithAssistedNode, callbackID)
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
class SelectPositionAtBoundaryWithDirection {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, uint32_t, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectPositionAtBoundaryWithDirection"); }
    static const bool isSync = false;

    SelectPositionAtBoundaryWithDirection(const WebCore::IntPoint& point, uint32_t granularity, uint32_t direction, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, granularity, direction, isInteractingWithAssistedNode, callbackID)
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
class MoveSelectionAtBoundaryWithDirection {
public:
    typedef std::tuple<uint32_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("MoveSelectionAtBoundaryWithDirection"); }
    static const bool isSync = false;

    MoveSelectionAtBoundaryWithDirection(uint32_t granularity, uint32_t direction, uint64_t callbackID)
        : m_arguments(granularity, direction, callbackID)
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
class SelectPositionAtPoint {
public:
    typedef std::tuple<const WebCore::IntPoint&, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectPositionAtPoint"); }
    static const bool isSync = false;

    SelectPositionAtPoint(const WebCore::IntPoint& point, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, isInteractingWithAssistedNode, callbackID)
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
class BeginSelectionInDirection {
public:
    typedef std::tuple<uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BeginSelectionInDirection"); }
    static const bool isSync = false;

    BeginSelectionInDirection(uint32_t direction, uint64_t callbackID)
        : m_arguments(direction, callbackID)
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
class UpdateSelectionWithExtentPoint {
public:
    typedef std::tuple<const WebCore::IntPoint&, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateSelectionWithExtentPoint"); }
    static const bool isSync = false;

    UpdateSelectionWithExtentPoint(const WebCore::IntPoint& point, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, isInteractingWithAssistedNode, callbackID)
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
class UpdateSelectionWithExtentPointAndBoundary {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint32_t, bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateSelectionWithExtentPointAndBoundary"); }
    static const bool isSync = false;

    UpdateSelectionWithExtentPointAndBoundary(const WebCore::IntPoint& point, uint32_t granularity, bool isInteractingWithAssistedNode, uint64_t callbackID)
        : m_arguments(point, granularity, isInteractingWithAssistedNode, callbackID)
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
class RequestDictationContext {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestDictationContext"); }
    static const bool isSync = false;

    explicit RequestDictationContext(uint64_t callbackID)
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
#endif

#if PLATFORM(IOS)
class ReplaceDictatedText {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReplaceDictatedText"); }
    static const bool isSync = false;

    ReplaceDictatedText(const String& oldText, const String& newText)
        : m_arguments(oldText, newText)
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
class ReplaceSelectedText {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReplaceSelectedText"); }
    static const bool isSync = false;

    ReplaceSelectedText(const String& oldText, const String& newText)
        : m_arguments(oldText, newText)
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
class RequestAutocorrectionData {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestAutocorrectionData"); }
    static const bool isSync = false;

    RequestAutocorrectionData(const String& textForAutocorrection, uint64_t callbackID)
        : m_arguments(textForAutocorrection, callbackID)
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
class ApplyAutocorrection {
public:
    typedef std::tuple<const String&, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplyAutocorrection"); }
    static const bool isSync = false;

    ApplyAutocorrection(const String& correction, const String& originalText, uint64_t callbackID)
        : m_arguments(correction, originalText, callbackID)
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
class SyncApplyAutocorrection {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SyncApplyAutocorrection"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    SyncApplyAutocorrection(const String& correction, const String& originalText)
        : m_arguments(correction, originalText)
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
class RequestAutocorrectionContext {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestAutocorrectionContext"); }
    static const bool isSync = false;

    explicit RequestAutocorrectionContext(uint64_t callbackID)
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
#endif

#if PLATFORM(IOS)
class GetAutocorrectionContext {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetAutocorrectionContext"); }
    static const bool isSync = true;

    typedef std::tuple<String&, String&, String&, String&, uint64_t&, uint64_t&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(IOS)
class GetPositionInformation {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetPositionInformation"); }
    static const bool isSync = true;

    typedef std::tuple<WebKit::InteractionInformationAtPosition&> Reply;
    explicit GetPositionInformation(const WebCore::IntPoint& point)
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
class RequestPositionInformation {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RequestPositionInformation"); }
    static const bool isSync = false;

    explicit RequestPositionInformation(const WebCore::IntPoint& point)
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
class StartInteractionWithElementAtPosition {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StartInteractionWithElementAtPosition"); }
    static const bool isSync = false;

    explicit StartInteractionWithElementAtPosition(const WebCore::IntPoint& point)
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
class StopInteraction {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopInteraction"); }
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
class PerformActionOnElement {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformActionOnElement"); }
    static const bool isSync = false;

    explicit PerformActionOnElement(uint32_t action)
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
#endif

#if PLATFORM(IOS)
class FocusNextAssistedNode {
public:
    typedef std::tuple<bool, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FocusNextAssistedNode"); }
    static const bool isSync = false;

    FocusNextAssistedNode(bool isForward, uint64_t callbackID)
        : m_arguments(isForward, callbackID)
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
class SetAssistedNodeValue {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAssistedNodeValue"); }
    static const bool isSync = false;

    explicit SetAssistedNodeValue(const String& value)
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
#endif

#if PLATFORM(IOS)
class SetAssistedNodeValueAsNumber {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAssistedNodeValueAsNumber"); }
    static const bool isSync = false;

    explicit SetAssistedNodeValueAsNumber(double value)
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
#endif

#if PLATFORM(IOS)
class SetAssistedNodeSelectedIndex {
public:
    typedef std::tuple<uint32_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAssistedNodeSelectedIndex"); }
    static const bool isSync = false;

    SetAssistedNodeSelectedIndex(uint32_t index, bool allowMultipleSelection)
        : m_arguments(index, allowMultipleSelection)
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
class ApplicationWillResignActive {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplicationWillResignActive"); }
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
class ApplicationDidEnterBackground {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplicationDidEnterBackground"); }
    static const bool isSync = false;

    explicit ApplicationDidEnterBackground(bool isSuspendedUnderLock)
        : m_arguments(isSuspendedUnderLock)
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
class ApplicationDidFinishSnapshottingAfterEnteringBackground {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplicationDidFinishSnapshottingAfterEnteringBackground"); }
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
class ApplicationWillEnterForeground {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplicationWillEnterForeground"); }
    static const bool isSync = false;

    explicit ApplicationWillEnterForeground(bool isSuspendedUnderLock)
        : m_arguments(isSuspendedUnderLock)
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
class ApplicationDidBecomeActive {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ApplicationDidBecomeActive"); }
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
class ContentSizeCategoryDidChange {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContentSizeCategoryDidChange"); }
    static const bool isSync = false;

    explicit ContentSizeCategoryDidChange(const String& contentSizeCategory)
        : m_arguments(contentSizeCategory)
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
class ExecuteEditCommandWithCallback {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExecuteEditCommandWithCallback"); }
    static const bool isSync = false;

    ExecuteEditCommandWithCallback(const String& name, uint64_t callbackID)
        : m_arguments(name, callbackID)
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
class GetSelectionContext {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSelectionContext"); }
    static const bool isSync = false;

    explicit GetSelectionContext(uint64_t callbackID)
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
#endif

#if PLATFORM(IOS)
class SetAllowsMediaDocumentInlinePlayback {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAllowsMediaDocumentInlinePlayback"); }
    static const bool isSync = false;

    explicit SetAllowsMediaDocumentInlinePlayback(bool allows)
        : m_arguments(allows)
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
class HandleTwoFingerTapAtPoint {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleTwoFingerTapAtPoint"); }
    static const bool isSync = false;

    HandleTwoFingerTapAtPoint(const WebCore::IntPoint& point, uint64_t requestID)
        : m_arguments(point, requestID)
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
class SetForceAlwaysUserScalable {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetForceAlwaysUserScalable"); }
    static const bool isSync = false;

    explicit SetForceAlwaysUserScalable(bool userScalable)
        : m_arguments(userScalable)
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

class SetControlledByAutomation {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetControlledByAutomation"); }
    static const bool isSync = false;

    explicit SetControlledByAutomation(bool controlled)
        : m_arguments(controlled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(REMOTE_INSPECTOR)
class SetAllowsRemoteInspection {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAllowsRemoteInspection"); }
    static const bool isSync = false;

    explicit SetAllowsRemoteInspection(bool allow)
        : m_arguments(allow)
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

#if ENABLE(REMOTE_INSPECTOR)
class SetRemoteInspectionNameOverride {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetRemoteInspectionNameOverride"); }
    static const bool isSync = false;

    explicit SetRemoteInspectionNameOverride(const String& name)
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
#endif

#if ENABLE(IOS_TOUCH_EVENTS)
class TouchEventSync {
public:
    typedef std::tuple<const WebKit::WebTouchEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TouchEventSync"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit TouchEventSync(const WebKit::WebTouchEvent& event)
        : m_arguments(event)
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

#if !ENABLE(IOS_TOUCH_EVENTS) && ENABLE(TOUCH_EVENTS)
class TouchEvent {
public:
    typedef std::tuple<const WebKit::WebTouchEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TouchEvent"); }
    static const bool isSync = false;

    explicit TouchEvent(const WebKit::WebTouchEvent& event)
        : m_arguments(event)
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
class DidEndColorPicker {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidEndColorPicker"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if ENABLE(INPUT_TYPE_COLOR)
class DidChooseColor {
public:
    typedef std::tuple<const WebCore::Color&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChooseColor"); }
    static const bool isSync = false;

    explicit DidChooseColor(const WebCore::Color& color)
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

#if ENABLE(CONTEXT_MENUS)
class ContextMenuHidden {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ContextMenuHidden"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class ScrollBy {
public:
    typedef std::tuple<uint32_t, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScrollBy"); }
    static const bool isSync = false;

    ScrollBy(uint32_t scrollDirection, uint32_t scrollGranularity)
        : m_arguments(scrollDirection, scrollGranularity)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CenterSelectionInVisibleArea {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CenterSelectionInVisibleArea"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GoBack {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GoBack"); }
    static const bool isSync = false;

    GoBack(uint64_t navigationID, uint64_t backForwardItemID)
        : m_arguments(navigationID, backForwardItemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GoForward {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GoForward"); }
    static const bool isSync = false;

    GoForward(uint64_t navigationID, uint64_t backForwardItemID)
        : m_arguments(navigationID, backForwardItemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GoToBackForwardItem {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GoToBackForwardItem"); }
    static const bool isSync = false;

    GoToBackForwardItem(uint64_t navigationID, uint64_t backForwardItemID)
        : m_arguments(navigationID, backForwardItemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class TryRestoreScrollPosition {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TryRestoreScrollPosition"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadURLInFrame {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadURLInFrame"); }
    static const bool isSync = false;

    LoadURLInFrame(const String& url, uint64_t frameID)
        : m_arguments(url, frameID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadRequest {
public:
    typedef std::tuple<const WebKit::LoadParameters&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadRequest"); }
    static const bool isSync = false;

    explicit LoadRequest(const WebKit::LoadParameters& loadParameters)
        : m_arguments(loadParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadData {
public:
    typedef std::tuple<const WebKit::LoadParameters&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadData"); }
    static const bool isSync = false;

    explicit LoadData(const WebKit::LoadParameters& loadParameters)
        : m_arguments(loadParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadString {
public:
    typedef std::tuple<const WebKit::LoadParameters&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadString"); }
    static const bool isSync = false;

    explicit LoadString(const WebKit::LoadParameters& loadParameters)
        : m_arguments(loadParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class LoadAlternateHTMLString {
public:
    typedef std::tuple<const WebKit::LoadParameters&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LoadAlternateHTMLString"); }
    static const bool isSync = false;

    explicit LoadAlternateHTMLString(const WebKit::LoadParameters& loadParameters)
        : m_arguments(loadParameters)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class NavigateToPDFLinkWithSimulatedClick {
public:
    typedef std::tuple<const String&, const WebCore::IntPoint&, const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("NavigateToPDFLinkWithSimulatedClick"); }
    static const bool isSync = false;

    NavigateToPDFLinkWithSimulatedClick(const String& url, const WebCore::IntPoint& documentPoint, const WebCore::IntPoint& screenPoint)
        : m_arguments(url, documentPoint, screenPoint)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class Reload {
public:
    typedef std::tuple<uint64_t, bool, bool, const WebKit::SandboxExtension::Handle&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Reload"); }
    static const bool isSync = false;

    Reload(uint64_t navigationID, bool reloadFromOrigin, bool contentBlockersEnabled, const WebKit::SandboxExtension::Handle& sandboxExtensionHandle)
        : m_arguments(navigationID, reloadFromOrigin, contentBlockersEnabled, sandboxExtensionHandle)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StopLoading {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopLoading"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class StopLoadingFrame {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("StopLoadingFrame"); }
    static const bool isSync = false;

    explicit StopLoadingFrame(uint64_t frameID)
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

class RestoreSession {
public:
    typedef std::tuple<const Vector<WebKit::BackForwardListItemState>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RestoreSession"); }
    static const bool isSync = false;

    explicit RestoreSession(const Vector<WebKit::BackForwardListItemState>& itemStates)
        : m_arguments(itemStates)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidRemoveBackForwardItem {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRemoveBackForwardItem"); }
    static const bool isSync = false;

    explicit DidRemoveBackForwardItem(uint64_t backForwardItemID)
        : m_arguments(backForwardItemID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidReceivePolicyDecision {
public:
    typedef std::tuple<uint64_t, uint64_t, uint32_t, uint64_t, const WebKit::DownloadID&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceivePolicyDecision"); }
    static const bool isSync = false;

    DidReceivePolicyDecision(uint64_t frameID, uint64_t listenerID, uint32_t policyAction, uint64_t navigationID, const WebKit::DownloadID& downloadID)
        : m_arguments(frameID, listenerID, policyAction, navigationID, downloadID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ClearSelection {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearSelection"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RestoreSelectionInFocusedEditableElement {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RestoreSelectionInFocusedEditableElement"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetContentsAsString {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetContentsAsString"); }
    static const bool isSync = false;

    explicit GetContentsAsString(uint64_t callbackID)
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

#if ENABLE(MHTML)
class GetContentsAsMHTMLData {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetContentsAsMHTMLData"); }
    static const bool isSync = false;

    explicit GetContentsAsMHTMLData(uint64_t callbackID)
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
#endif

class GetMainResourceDataOfFrame {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetMainResourceDataOfFrame"); }
    static const bool isSync = false;

    GetMainResourceDataOfFrame(uint64_t frameID, uint64_t callbackID)
        : m_arguments(frameID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetResourceDataFromFrame {
public:
    typedef std::tuple<uint64_t, const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetResourceDataFromFrame"); }
    static const bool isSync = false;

    GetResourceDataFromFrame(uint64_t frameID, const String& resourceURL, uint64_t callbackID)
        : m_arguments(frameID, resourceURL, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetRenderTreeExternalRepresentation {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetRenderTreeExternalRepresentation"); }
    static const bool isSync = false;

    explicit GetRenderTreeExternalRepresentation(uint64_t callbackID)
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

class GetSelectionOrContentsAsString {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSelectionOrContentsAsString"); }
    static const bool isSync = false;

    explicit GetSelectionOrContentsAsString(uint64_t callbackID)
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

class GetSelectionAsWebArchiveData {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSelectionAsWebArchiveData"); }
    static const bool isSync = false;

    explicit GetSelectionAsWebArchiveData(uint64_t callbackID)
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

class GetSourceForFrame {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSourceForFrame"); }
    static const bool isSync = false;

    GetSourceForFrame(uint64_t frameID, uint64_t callbackID)
        : m_arguments(frameID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetWebArchiveOfFrame {
public:
    typedef std::tuple<uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetWebArchiveOfFrame"); }
    static const bool isSync = false;

    GetWebArchiveOfFrame(uint64_t frameID, uint64_t callbackID)
        : m_arguments(frameID, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class RunJavaScriptInMainFrame {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RunJavaScriptInMainFrame"); }
    static const bool isSync = false;

    RunJavaScriptInMainFrame(const String& script, uint64_t callbackID)
        : m_arguments(script, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ForceRepaint {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ForceRepaint"); }
    static const bool isSync = false;

    explicit ForceRepaint(uint64_t callbackID)
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

#if PLATFORM(COCOA)
class PerformDictionaryLookupAtLocation {
public:
    typedef std::tuple<const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformDictionaryLookupAtLocation"); }
    static const bool isSync = false;

    explicit PerformDictionaryLookupAtLocation(const WebCore::FloatPoint& point)
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

#if PLATFORM(MAC)
class PerformDictionaryLookupOfCurrentSelection {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformDictionaryLookupOfCurrentSelection"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class SetFont {
public:
    typedef std::tuple<const String&, double, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetFont"); }
    static const bool isSync = false;

    SetFont(const String& fontFamily, double fontSize, uint64_t fontTraits)
        : m_arguments(fontFamily, fontSize, fontTraits)
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

class PreferencesDidChange {
public:
    typedef std::tuple<const WebKit::WebPreferencesStore&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PreferencesDidChange"); }
    static const bool isSync = false;

    explicit PreferencesDidChange(const WebKit::WebPreferencesStore& store)
        : m_arguments(store)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetUserAgent {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetUserAgent"); }
    static const bool isSync = false;

    explicit SetUserAgent(const String& userAgent)
        : m_arguments(userAgent)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetCustomTextEncodingName {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCustomTextEncodingName"); }
    static const bool isSync = false;

    explicit SetCustomTextEncodingName(const String& encodingName)
        : m_arguments(encodingName)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SuspendActiveDOMObjectsAndAnimations {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SuspendActiveDOMObjectsAndAnimations"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ResumeActiveDOMObjectsAndAnimations {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ResumeActiveDOMObjectsAndAnimations"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class Close {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Close"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class TryClose {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TryClose"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetEditable {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEditable"); }
    static const bool isSync = false;

    explicit SetEditable(bool editable)
        : m_arguments(editable)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ValidateCommand {
public:
    typedef std::tuple<const String&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ValidateCommand"); }
    static const bool isSync = false;

    ValidateCommand(const String& name, uint64_t callbackID)
        : m_arguments(name, callbackID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ExecuteEditCommand {
public:
    typedef std::tuple<const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExecuteEditCommand"); }
    static const bool isSync = false;

    ExecuteEditCommand(const String& name, const String& argument)
        : m_arguments(name, argument)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidRemoveEditCommand {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidRemoveEditCommand"); }
    static const bool isSync = false;

    explicit DidRemoveEditCommand(uint64_t commandID)
        : m_arguments(commandID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ReapplyEditCommand {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReapplyEditCommand"); }
    static const bool isSync = false;

    explicit ReapplyEditCommand(uint64_t commandID)
        : m_arguments(commandID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class UnapplyEditCommand {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UnapplyEditCommand"); }
    static const bool isSync = false;

    explicit UnapplyEditCommand(uint64_t commandID)
        : m_arguments(commandID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPageAndTextZoomFactors {
public:
    typedef std::tuple<double, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPageAndTextZoomFactors"); }
    static const bool isSync = false;

    SetPageAndTextZoomFactors(double pageZoomFactor, double textZoomFactor)
        : m_arguments(pageZoomFactor, textZoomFactor)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPageZoomFactor {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPageZoomFactor"); }
    static const bool isSync = false;

    explicit SetPageZoomFactor(double zoomFactor)
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

class SetTextZoomFactor {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTextZoomFactor"); }
    static const bool isSync = false;

    explicit SetTextZoomFactor(double zoomFactor)
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

class WindowScreenDidChange {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowScreenDidChange"); }
    static const bool isSync = false;

    explicit WindowScreenDidChange(uint32_t displayID)
        : m_arguments(displayID)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ScalePage {
public:
    typedef std::tuple<double, const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScalePage"); }
    static const bool isSync = false;

    ScalePage(double scale, const WebCore::IntPoint& origin)
        : m_arguments(scale, origin)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ScalePageInViewCoordinates {
public:
    typedef std::tuple<double, const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScalePageInViewCoordinates"); }
    static const bool isSync = false;

    ScalePageInViewCoordinates(double scale, const WebCore::IntPoint& centerInViewCoordinates)
        : m_arguments(scale, centerInViewCoordinates)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ScaleView {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ScaleView"); }
    static const bool isSync = false;

    explicit ScaleView(double scale)
        : m_arguments(scale)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetUseFixedLayout {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetUseFixedLayout"); }
    static const bool isSync = false;

    explicit SetUseFixedLayout(bool fixed)
        : m_arguments(fixed)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetFixedLayoutSize {
public:
    typedef std::tuple<const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetFixedLayoutSize"); }
    static const bool isSync = false;

    explicit SetFixedLayoutSize(const WebCore::IntSize& size)
        : m_arguments(size)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ListenForLayoutMilestones {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ListenForLayoutMilestones"); }
    static const bool isSync = false;

    explicit ListenForLayoutMilestones(uint32_t milestones)
        : m_arguments(milestones)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetSuppressScrollbarAnimations {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetSuppressScrollbarAnimations"); }
    static const bool isSync = false;

    explicit SetSuppressScrollbarAnimations(bool suppressAnimations)
        : m_arguments(suppressAnimations)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetEnableVerticalRubberBanding {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEnableVerticalRubberBanding"); }
    static const bool isSync = false;

    explicit SetEnableVerticalRubberBanding(bool enableVerticalRubberBanding)
        : m_arguments(enableVerticalRubberBanding)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetEnableHorizontalRubberBanding {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetEnableHorizontalRubberBanding"); }
    static const bool isSync = false;

    explicit SetEnableHorizontalRubberBanding(bool enableHorizontalRubberBanding)
        : m_arguments(enableHorizontalRubberBanding)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetBackgroundExtendsBeyondPage {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetBackgroundExtendsBeyondPage"); }
    static const bool isSync = false;

    explicit SetBackgroundExtendsBeyondPage(bool backgroundExtendsBeyondPage)
        : m_arguments(backgroundExtendsBeyondPage)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPaginationMode {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPaginationMode"); }
    static const bool isSync = false;

    explicit SetPaginationMode(uint32_t mode)
        : m_arguments(mode)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPaginationBehavesLikeColumns {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPaginationBehavesLikeColumns"); }
    static const bool isSync = false;

    explicit SetPaginationBehavesLikeColumns(bool behavesLikeColumns)
        : m_arguments(behavesLikeColumns)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPageLength {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPageLength"); }
    static const bool isSync = false;

    explicit SetPageLength(double pageLength)
        : m_arguments(pageLength)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetGapBetweenPages {
public:
    typedef std::tuple<double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetGapBetweenPages"); }
    static const bool isSync = false;

    explicit SetGapBetweenPages(double gap)
        : m_arguments(gap)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetPaginationLineGridEnabled {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetPaginationLineGridEnabled"); }
    static const bool isSync = false;

    explicit SetPaginationLineGridEnabled(bool lineGridEnabled)
        : m_arguments(lineGridEnabled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class PostInjectedBundleMessage {
public:
    typedef std::tuple<const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PostInjectedBundleMessage"); }
    static const bool isSync = false;

    PostInjectedBundleMessage(const String& messageName, const WebKit::UserData& messageBody)
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

class PostSynchronousInjectedBundleMessage {
public:
    typedef std::tuple<const String&, const WebKit::UserData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PostSynchronousInjectedBundleMessage"); }
    static const bool isSync = true;

    typedef std::tuple<> Reply;
    PostSynchronousInjectedBundleMessage(const String& messageName, const WebKit::UserData& messageBody)
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

class FindString {
public:
    typedef std::tuple<const String&, uint32_t, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FindString"); }
    static const bool isSync = false;

    FindString(const String& string, uint32_t findOptions, const unsigned& maxMatchCount)
        : m_arguments(string, findOptions, maxMatchCount)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class FindStringMatches {
public:
    typedef std::tuple<const String&, uint32_t, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FindStringMatches"); }
    static const bool isSync = false;

    FindStringMatches(const String& string, uint32_t findOptions, const unsigned& maxMatchCount)
        : m_arguments(string, findOptions, maxMatchCount)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetImageForFindMatch {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetImageForFindMatch"); }
    static const bool isSync = false;

    explicit GetImageForFindMatch(uint32_t matchIndex)
        : m_arguments(matchIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SelectFindMatch {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SelectFindMatch"); }
    static const bool isSync = false;

    explicit SelectFindMatch(uint32_t matchIndex)
        : m_arguments(matchIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class HideFindUI {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HideFindUI"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class CountStringMatches {
public:
    typedef std::tuple<const String&, uint32_t, const unsigned&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CountStringMatches"); }
    static const bool isSync = false;

    CountStringMatches(const String& string, uint32_t findOptions, const unsigned& maxMatchCount)
        : m_arguments(string, findOptions, maxMatchCount)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class AddMIMETypeWithCustomContentProvider {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddMIMETypeWithCustomContentProvider"); }
    static const bool isSync = false;

    explicit AddMIMETypeWithCustomContentProvider(const String& mimeType)
        : m_arguments(mimeType)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
class PerformDragControllerAction {
public:
    typedef std::tuple<uint64_t, const WebCore::DragData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformDragControllerAction"); }
    static const bool isSync = false;

    PerformDragControllerAction(uint64_t action, const WebCore::DragData& dragData)
        : m_arguments(action, dragData)
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

#if !PLATFORM(GTK) && ENABLE(DRAG_SUPPORT)
class PerformDragControllerAction {
public:
    typedef std::tuple<uint64_t, const WebCore::IntPoint&, const WebCore::IntPoint&, uint64_t, const String&, uint32_t, const WebKit::SandboxExtension::Handle&, const WebKit::SandboxExtension::HandleArray&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformDragControllerAction"); }
    static const bool isSync = false;

    PerformDragControllerAction(uint64_t action, const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t draggingSourceOperationMask, const String& dragStorageName, uint32_t flags, const WebKit::SandboxExtension::Handle& sandboxExtensionHandle, const WebKit::SandboxExtension::HandleArray& sandboxExtensionsForUpload)
        : m_arguments(action, clientPosition, globalPosition, draggingSourceOperationMask, dragStorageName, flags, sandboxExtensionHandle, sandboxExtensionsForUpload)
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

#if ENABLE(DRAG_SUPPORT)
class DidStartDrag {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidStartDrag"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if ENABLE(DRAG_SUPPORT)
class DragEnded {
public:
    typedef std::tuple<const WebCore::IntPoint&, const WebCore::IntPoint&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DragEnded"); }
    static const bool isSync = false;

    DragEnded(const WebCore::IntPoint& clientPosition, const WebCore::IntPoint& globalPosition, uint64_t operation)
        : m_arguments(clientPosition, globalPosition, operation)
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

class DidChangeSelectedIndexForActivePopupMenu {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChangeSelectedIndexForActivePopupMenu"); }
    static const bool isSync = false;

    explicit DidChangeSelectedIndexForActivePopupMenu(int32_t newIndex)
        : m_arguments(newIndex)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetTextForActivePopupMenu {
public:
    typedef std::tuple<int32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetTextForActivePopupMenu"); }
    static const bool isSync = false;

    explicit SetTextForActivePopupMenu(int32_t index)
        : m_arguments(index)
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
class FailedToShowPopupMenu {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FailedToShowPopupMenu"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if ENABLE(CONTEXT_MENUS)
class DidSelectItemFromActiveContextMenu {
public:
    typedef std::tuple<const WebKit::WebContextMenuItemData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidSelectItemFromActiveContextMenu"); }
    static const bool isSync = false;

    explicit DidSelectItemFromActiveContextMenu(const WebKit::WebContextMenuItemData& menuItem)
        : m_arguments(menuItem)
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
class DidChooseFilesForOpenPanelWithDisplayStringAndIcon {
public:
    typedef std::tuple<const Vector<String>&, const String&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChooseFilesForOpenPanelWithDisplayStringAndIcon"); }
    static const bool isSync = false;

    DidChooseFilesForOpenPanelWithDisplayStringAndIcon(const Vector<String>& fileURLs, const String& displayString, const IPC::DataReference& iconData)
        : m_arguments(fileURLs, displayString, iconData)
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

class DidChooseFilesForOpenPanel {
public:
    typedef std::tuple<const Vector<String>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidChooseFilesForOpenPanel"); }
    static const bool isSync = false;

    explicit DidChooseFilesForOpenPanel(const Vector<String>& fileURLs)
        : m_arguments(fileURLs)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class DidCancelForOpenPanel {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCancelForOpenPanel"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

#if ENABLE(SANDBOX_EXTENSIONS)
class ExtendSandboxForFileFromOpenPanel {
public:
    typedef std::tuple<const WebKit::SandboxExtension::Handle&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExtendSandboxForFileFromOpenPanel"); }
    static const bool isSync = false;

    explicit ExtendSandboxForFileFromOpenPanel(const WebKit::SandboxExtension::Handle& sandboxExtensionHandle)
        : m_arguments(sandboxExtensionHandle)
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

class AdvanceToNextMisspelling {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AdvanceToNextMisspelling"); }
    static const bool isSync = false;

    explicit AdvanceToNextMisspelling(bool startBeforeSelection)
        : m_arguments(startBeforeSelection)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ChangeSpellingToWord {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ChangeSpellingToWord"); }
    static const bool isSync = false;

    explicit ChangeSpellingToWord(const String& word)
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

class DidFinishCheckingText {
public:
    typedef std::tuple<uint64_t, const Vector<WebCore::TextCheckingResult>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidFinishCheckingText"); }
    static const bool isSync = false;

    DidFinishCheckingText(uint64_t requestID, const Vector<WebCore::TextCheckingResult>& result)
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

class DidCancelCheckingText {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCancelCheckingText"); }
    static const bool isSync = false;

    explicit DidCancelCheckingText(uint64_t requestID)
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

#if USE(APPKIT)
class UppercaseWord {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UppercaseWord"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(APPKIT)
class LowercaseWord {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("LowercaseWord"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if USE(APPKIT)
class CapitalizeWord {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CapitalizeWord"); }
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
class SetSmartInsertDeleteEnabled {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetSmartInsertDeleteEnabled"); }
    static const bool isSync = false;

    explicit SetSmartInsertDeleteEnabled(bool isSmartInsertDeleteEnabled)
        : m_arguments(isSmartInsertDeleteEnabled)
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

#if ENABLE(GEOLOCATION)
class DidReceiveGeolocationPermissionDecision {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveGeolocationPermissionDecision"); }
    static const bool isSync = false;

    DidReceiveGeolocationPermissionDecision(uint64_t geolocationID, bool allowed)
        : m_arguments(geolocationID, allowed)
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
class DidReceiveUserMediaPermissionDecision {
public:
    typedef std::tuple<uint64_t, bool, const String&, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveUserMediaPermissionDecision"); }
    static const bool isSync = false;

    DidReceiveUserMediaPermissionDecision(uint64_t userMediaID, bool allowed, const String& audioDeviceUID, const String& videoDeviceUID)
        : m_arguments(userMediaID, allowed, audioDeviceUID, videoDeviceUID)
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
class DidCompleteUserMediaPermissionCheck {
public:
    typedef std::tuple<uint64_t, const String&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidCompleteUserMediaPermissionCheck"); }
    static const bool isSync = false;

    DidCompleteUserMediaPermissionCheck(uint64_t userMediaID, const String& mediaDeviceIdentifierHashSalt, bool allowed)
        : m_arguments(userMediaID, mediaDeviceIdentifierHashSalt, allowed)
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

class DidReceiveNotificationPermissionDecision {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidReceiveNotificationPermissionDecision"); }
    static const bool isSync = false;

    DidReceiveNotificationPermissionDecision(uint64_t notificationID, bool allowed)
        : m_arguments(notificationID, allowed)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class BeginPrinting {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("BeginPrinting"); }
    static const bool isSync = false;

    BeginPrinting(uint64_t frameID, const WebKit::PrintInfo& printInfo)
        : m_arguments(frameID, printInfo)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class EndPrinting {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EndPrinting"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class ComputePagesForPrinting {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ComputePagesForPrinting"); }
    static const bool isSync = false;

    ComputePagesForPrinting(uint64_t frameID, const WebKit::PrintInfo& printInfo, uint64_t callbackID)
        : m_arguments(frameID, printInfo, callbackID)
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
class DrawRectToImage {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&, const WebCore::IntRect&, const WebCore::IntSize&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DrawRectToImage"); }
    static const bool isSync = false;

    DrawRectToImage(uint64_t frameID, const WebKit::PrintInfo& printInfo, const WebCore::IntRect& rect, const WebCore::IntSize& imageSize, uint64_t callbackID)
        : m_arguments(frameID, printInfo, rect, imageSize, callbackID)
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
class DrawPagesToPDF {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&, uint32_t, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DrawPagesToPDF"); }
    static const bool isSync = false;

    DrawPagesToPDF(uint64_t frameID, const WebKit::PrintInfo& printInfo, uint32_t first, uint32_t count, uint64_t callbackID)
        : m_arguments(frameID, printInfo, first, count, callbackID)
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

#if (PLATFORM(COCOA) && PLATFORM(IOS))
class ComputePagesForPrintingAndDrawToPDF {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ComputePagesForPrintingAndDrawToPDF"); }
    static const bool isSync = true;

    struct DelayedReply : public ThreadSafeRefCounted<DelayedReply> {
        DelayedReply(PassRefPtr<IPC::Connection>, std::unique_ptr<IPC::Encoder>);
        ~DelayedReply();

        bool send(uint32_t pageCount);

    private:
        RefPtr<IPC::Connection> m_connection;
        std::unique_ptr<IPC::Encoder> m_encoder;
    };

    typedef std::tuple<uint32_t&> Reply;
    ComputePagesForPrintingAndDrawToPDF(uint64_t frameID, const WebKit::PrintInfo& printInfo, uint64_t callbackID)
        : m_arguments(frameID, printInfo, callbackID)
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
class DrawPagesForPrinting {
public:
    typedef std::tuple<uint64_t, const WebKit::PrintInfo&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DrawPagesForPrinting"); }
    static const bool isSync = false;

    DrawPagesForPrinting(uint64_t frameID, const WebKit::PrintInfo& printInfo, uint64_t callbackID)
        : m_arguments(frameID, printInfo, callbackID)
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

class SetMediaVolume {
public:
    typedef std::tuple<float> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMediaVolume"); }
    static const bool isSync = false;

    explicit SetMediaVolume(float volume)
        : m_arguments(volume)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetMuted {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMuted"); }
    static const bool isSync = false;

    explicit SetMuted(bool muted)
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

class SetMayStartMediaWhenInWindow {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMayStartMediaWhenInWindow"); }
    static const bool isSync = false;

    explicit SetMayStartMediaWhenInWindow(bool mayStartMedia)
        : m_arguments(mayStartMedia)
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
class HandleMediaEvent {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleMediaEvent"); }
    static const bool isSync = false;

    explicit HandleMediaEvent(uint32_t eventType)
        : m_arguments(eventType)
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
class SetVolumeOfMediaElement {
public:
    typedef std::tuple<double, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetVolumeOfMediaElement"); }
    static const bool isSync = false;

    SetVolumeOfMediaElement(double volume, uint64_t elementID)
        : m_arguments(volume, elementID)
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

class Dummy {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Dummy"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetCanRunBeforeUnloadConfirmPanel {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCanRunBeforeUnloadConfirmPanel"); }
    static const bool isSync = false;

    explicit SetCanRunBeforeUnloadConfirmPanel(bool canRunBeforeUnloadConfirmPanel)
        : m_arguments(canRunBeforeUnloadConfirmPanel)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetCanRunModal {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCanRunModal"); }
    static const bool isSync = false;

    explicit SetCanRunModal(bool canRunModal)
        : m_arguments(canRunModal)
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
class SetThemePath {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetThemePath"); }
    static const bool isSync = false;

    explicit SetThemePath(const String& themePath)
        : m_arguments(themePath)
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
class CommitPageTransitionViewport {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CommitPageTransitionViewport"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(GTK)
class SetComposition {
public:
    typedef std::tuple<const String&, const Vector<WebCore::CompositionUnderline>&, uint64_t, uint64_t, uint64_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetComposition"); }
    static const bool isSync = false;

    SetComposition(const String& text, const Vector<WebCore::CompositionUnderline>& underlines, uint64_t selectionStart, uint64_t selectionEnd, uint64_t replacementRangeStart, uint64_t replacementRangeEnd)
        : m_arguments(text, underlines, selectionStart, selectionEnd, replacementRangeStart, replacementRangeEnd)
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
class ConfirmComposition {
public:
    typedef std::tuple<const String&, int64_t, int64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ConfirmComposition"); }
    static const bool isSync = false;

    ConfirmComposition(const String& text, int64_t selectionStart, int64_t selectionLength)
        : m_arguments(text, selectionStart, selectionLength)
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
class CancelComposition {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelComposition"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM (GTK) && HAVE(GTK_GESTURES)
class GetCenterForZoomGesture {
public:
    typedef std::tuple<const WebCore::IntPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetCenterForZoomGesture"); }
    static const bool isSync = true;

    typedef std::tuple<WebCore::IntPoint&> Reply;
    explicit GetCenterForZoomGesture(const WebCore::IntPoint& centerInViewCoordinates)
        : m_arguments(centerInViewCoordinates)
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
class SendComplexTextInputToPlugin {
public:
    typedef std::tuple<uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SendComplexTextInputToPlugin"); }
    static const bool isSync = false;

    SendComplexTextInputToPlugin(uint64_t pluginComplexTextInputIdentifier, const String& textInput)
        : m_arguments(pluginComplexTextInputIdentifier, textInput)
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
    typedef std::tuple<const WebCore::FloatRect&, const WebCore::FloatRect&, const WebCore::FloatRect&, const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WindowAndViewFramesChanged"); }
    static const bool isSync = false;

    WindowAndViewFramesChanged(const WebCore::FloatRect& windowFrameInScreenCoordinates, const WebCore::FloatRect& windowFrameInUnflippedScreenCoordinates, const WebCore::FloatRect& viewFrameInWindowCoordinates, const WebCore::FloatPoint& accessibilityViewCoordinates)
        : m_arguments(windowFrameInScreenCoordinates, windowFrameInUnflippedScreenCoordinates, viewFrameInWindowCoordinates, accessibilityViewCoordinates)
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
class SetMainFrameIsScrollable {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMainFrameIsScrollable"); }
    static const bool isSync = false;

    explicit SetMainFrameIsScrollable(bool isScrollable)
        : m_arguments(isScrollable)
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
class RegisterUIProcessAccessibilityTokens {
public:
    typedef std::tuple<const IPC::DataReference&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("RegisterUIProcessAccessibilityTokens"); }
    static const bool isSync = false;

    RegisterUIProcessAccessibilityTokens(const IPC::DataReference& elemenToken, const IPC::DataReference& windowToken)
        : m_arguments(elemenToken, windowToken)
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
class GetStringSelectionForPasteboard {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetStringSelectionForPasteboard"); }
    static const bool isSync = true;

    typedef std::tuple<String&> Reply;
    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(COCOA)
class GetDataSelectionForPasteboard {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetDataSelectionForPasteboard"); }
    static const bool isSync = true;

    typedef std::tuple<WebKit::SharedMemory::Handle&, uint64_t&> Reply;
    explicit GetDataSelectionForPasteboard(const String& pasteboardType)
        : m_arguments(pasteboardType)
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
class ReadSelectionFromPasteboard {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReadSelectionFromPasteboard"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit ReadSelectionFromPasteboard(const String& pasteboardName)
        : m_arguments(pasteboardName)
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

#if (PLATFORM(COCOA) && ENABLE(SERVICE_CONTROLS))
class ReplaceSelectionWithPasteboardData {
public:
    typedef std::tuple<const Vector<String>&, const IPC::DataReference&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ReplaceSelectionWithPasteboardData"); }
    static const bool isSync = false;

    ReplaceSelectionWithPasteboardData(const Vector<String>& types, const IPC::DataReference& data)
        : m_arguments(types, data)
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
class ShouldDelayWindowOrderingEvent {
public:
    typedef std::tuple<const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ShouldDelayWindowOrderingEvent"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    explicit ShouldDelayWindowOrderingEvent(const WebKit::WebMouseEvent& event)
        : m_arguments(event)
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
class AcceptsFirstMouse {
public:
    typedef std::tuple<const int&, const WebKit::WebMouseEvent&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AcceptsFirstMouse"); }
    static const bool isSync = true;

    typedef std::tuple<bool&> Reply;
    AcceptsFirstMouse(const int& eventNumber, const WebKit::WebMouseEvent& event)
        : m_arguments(eventNumber, event)
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
class InsertTextAsync {
public:
    typedef std::tuple<const String&, const WebKit::EditingRange&, bool, uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InsertTextAsync"); }
    static const bool isSync = false;

    InsertTextAsync(const String& text, const WebKit::EditingRange& replacementRange, bool registerUndoGroup, uint32_t editingRangeIsRelativeTo)
        : m_arguments(text, replacementRange, registerUndoGroup, editingRangeIsRelativeTo)
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
class GetMarkedRangeAsync {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetMarkedRangeAsync"); }
    static const bool isSync = false;

    explicit GetMarkedRangeAsync(uint64_t callbackID)
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
#endif

#if PLATFORM(COCOA)
class GetSelectedRangeAsync {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetSelectedRangeAsync"); }
    static const bool isSync = false;

    explicit GetSelectedRangeAsync(uint64_t callbackID)
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
#endif

#if PLATFORM(COCOA)
class CharacterIndexForPointAsync {
public:
    typedef std::tuple<const WebCore::IntPoint&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CharacterIndexForPointAsync"); }
    static const bool isSync = false;

    CharacterIndexForPointAsync(const WebCore::IntPoint& point, uint64_t callbackID)
        : m_arguments(point, callbackID)
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
class FirstRectForCharacterRangeAsync {
public:
    typedef std::tuple<const WebKit::EditingRange&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FirstRectForCharacterRangeAsync"); }
    static const bool isSync = false;

    FirstRectForCharacterRangeAsync(const WebKit::EditingRange& range, uint64_t callbackID)
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
#endif

#if PLATFORM(COCOA)
class SetCompositionAsync {
public:
    typedef std::tuple<const String&, const Vector<WebCore::CompositionUnderline>&, const WebKit::EditingRange&, const WebKit::EditingRange&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetCompositionAsync"); }
    static const bool isSync = false;

    SetCompositionAsync(const String& text, const Vector<WebCore::CompositionUnderline>& underlines, const WebKit::EditingRange& selectionRange, const WebKit::EditingRange& replacementRange)
        : m_arguments(text, underlines, selectionRange, replacementRange)
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
class ConfirmCompositionAsync {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ConfirmCompositionAsync"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class InsertDictatedTextAsync {
public:
    typedef std::tuple<const String&, const WebKit::EditingRange&, const Vector<WebCore::DictationAlternative>&, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("InsertDictatedTextAsync"); }
    static const bool isSync = false;

    InsertDictatedTextAsync(const String& text, const WebKit::EditingRange& replacementRange, const Vector<WebCore::DictationAlternative>& dictationAlternatives, bool registerUndoGroup)
        : m_arguments(text, replacementRange, dictationAlternatives, registerUndoGroup)
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
class AttributedSubstringForCharacterRangeAsync {
public:
    typedef std::tuple<const WebKit::EditingRange&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AttributedSubstringForCharacterRangeAsync"); }
    static const bool isSync = false;

    AttributedSubstringForCharacterRangeAsync(const WebKit::EditingRange& range, uint64_t callbackID)
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
#endif

#if PLATFORM(MAC)
class FontAtSelection {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FontAtSelection"); }
    static const bool isSync = false;

    explicit FontAtSelection(uint64_t callbackID)
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
#endif

class SetMinimumLayoutSize {
public:
    typedef std::tuple<const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetMinimumLayoutSize"); }
    static const bool isSync = false;

    explicit SetMinimumLayoutSize(const WebCore::IntSize& minimumLayoutSize)
        : m_arguments(minimumLayoutSize)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetAutoSizingShouldExpandToViewHeight {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetAutoSizingShouldExpandToViewHeight"); }
    static const bool isSync = false;

    explicit SetAutoSizingShouldExpandToViewHeight(bool shouldExpand)
        : m_arguments(shouldExpand)
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
class ConfirmComposition {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ConfirmComposition"); }
    static const bool isSync = false;

    explicit ConfirmComposition(const String& compositionString)
        : m_arguments(compositionString)
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

#if PLATFORM(EFL)
class SetComposition {
public:
    typedef std::tuple<const String&, const Vector<WebCore::CompositionUnderline>&, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetComposition"); }
    static const bool isSync = false;

    SetComposition(const String& compositionString, const Vector<WebCore::CompositionUnderline>& underlines, uint64_t cursorPosition)
        : m_arguments(compositionString, underlines, cursorPosition)
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

#if PLATFORM(EFL)
class CancelComposition {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CancelComposition"); }
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
class FindZoomableAreaForPoint {
public:
    typedef std::tuple<const WebCore::IntPoint&, const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("FindZoomableAreaForPoint"); }
    static const bool isSync = false;

    FindZoomableAreaForPoint(const WebCore::IntPoint& point, const WebCore::IntSize& area)
        : m_arguments(point, area)
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
class HandleAlternativeTextUIResult {
public:
    typedef std::tuple<const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleAlternativeTextUIResult"); }
    static const bool isSync = false;

    explicit HandleAlternativeTextUIResult(const String& result)
        : m_arguments(result)
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
class WillStartUserTriggeredZooming {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillStartUserTriggeredZooming"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

class SetScrollPinningBehavior {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetScrollPinningBehavior"); }
    static const bool isSync = false;

    explicit SetScrollPinningBehavior(uint32_t pinning)
        : m_arguments(pinning)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetScrollbarOverlayStyle {
public:
    typedef std::tuple<const Optional<uint32_t>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetScrollbarOverlayStyle"); }
    static const bool isSync = false;

    explicit SetScrollbarOverlayStyle(const Optional<uint32_t>& scrollbarStyle)
        : m_arguments(scrollbarStyle)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class GetBytecodeProfile {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GetBytecodeProfile"); }
    static const bool isSync = false;

    explicit GetBytecodeProfile(uint64_t callbackID)
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

class TakeSnapshot {
public:
    typedef std::tuple<const WebCore::IntRect&, const WebCore::IntSize&, uint32_t, uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("TakeSnapshot"); }
    static const bool isSync = false;

    TakeSnapshot(const WebCore::IntRect& snapshotRect, const WebCore::IntSize& bitmapSize, uint32_t options, uint64_t callbackID)
        : m_arguments(snapshotRect, bitmapSize, options, callbackID)
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
class PerformImmediateActionHitTestAtLocation {
public:
    typedef std::tuple<const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PerformImmediateActionHitTestAtLocation"); }
    static const bool isSync = false;

    explicit PerformImmediateActionHitTestAtLocation(const WebCore::FloatPoint& location)
        : m_arguments(location)
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
class ImmediateActionDidUpdate {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ImmediateActionDidUpdate"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class ImmediateActionDidCancel {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ImmediateActionDidCancel"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class ImmediateActionDidComplete {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ImmediateActionDidComplete"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};
#endif

#if PLATFORM(MAC)
class DataDetectorsDidPresentUI {
public:
    typedef std::tuple<const WebCore::PageOverlay::PageOverlayID&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DataDetectorsDidPresentUI"); }
    static const bool isSync = false;

    explicit DataDetectorsDidPresentUI(const WebCore::PageOverlay::PageOverlayID& pageOverlay)
        : m_arguments(pageOverlay)
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
class DataDetectorsDidChangeUI {
public:
    typedef std::tuple<const WebCore::PageOverlay::PageOverlayID&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DataDetectorsDidChangeUI"); }
    static const bool isSync = false;

    explicit DataDetectorsDidChangeUI(const WebCore::PageOverlay::PageOverlayID& pageOverlay)
        : m_arguments(pageOverlay)
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
class DataDetectorsDidHideUI {
public:
    typedef std::tuple<const WebCore::PageOverlay::PageOverlayID&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DataDetectorsDidHideUI"); }
    static const bool isSync = false;

    explicit DataDetectorsDidHideUI(const WebCore::PageOverlay::PageOverlayID& pageOverlay)
        : m_arguments(pageOverlay)
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
class HandleAcceptedCandidate {
public:
    typedef std::tuple<const WebCore::TextCheckingResult&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("HandleAcceptedCandidate"); }
    static const bool isSync = false;

    explicit HandleAcceptedCandidate(const WebCore::TextCheckingResult& acceptedCandidate)
        : m_arguments(acceptedCandidate)
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

class SetShouldDispatchFakeMouseMoveEvents {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetShouldDispatchFakeMouseMoveEvents"); }
    static const bool isSync = false;

    explicit SetShouldDispatchFakeMouseMoveEvents(bool shouldDispatchFakeMouseMoveEvents)
        : m_arguments(shouldDispatchFakeMouseMoveEvents)
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
class PlaybackTargetSelected {
public:
    typedef std::tuple<uint64_t, const WebCore::MediaPlaybackTargetContext&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PlaybackTargetSelected"); }
    static const bool isSync = false;

    PlaybackTargetSelected(uint64_t contextId, const WebCore::MediaPlaybackTargetContext& target)
        : m_arguments(contextId, target)
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
class PlaybackTargetAvailabilityDidChange {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("PlaybackTargetAvailabilityDidChange"); }
    static const bool isSync = false;

    PlaybackTargetAvailabilityDidChange(uint64_t contextId, bool available)
        : m_arguments(contextId, available)
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
class SetShouldPlayToPlaybackTarget {
public:
    typedef std::tuple<uint64_t, bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetShouldPlayToPlaybackTarget"); }
    static const bool isSync = false;

    SetShouldPlayToPlaybackTarget(uint64_t contextId, bool shouldPlay)
        : m_arguments(contextId, shouldPlay)
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

class ClearWheelEventTestTrigger {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ClearWheelEventTestTrigger"); }
    static const bool isSync = false;

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetShouldScaleViewToFitDocument {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetShouldScaleViewToFitDocument"); }
    static const bool isSync = false;

    explicit SetShouldScaleViewToFitDocument(bool shouldScaleViewToFitDocument)
        : m_arguments(shouldScaleViewToFitDocument)
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
class DidEndRequestInstallMissingMediaPlugins {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidEndRequestInstallMissingMediaPlugins"); }
    static const bool isSync = false;

    explicit DidEndRequestInstallMissingMediaPlugins(uint32_t result)
        : m_arguments(result)
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

class SetResourceCachingDisabled {
public:
    typedef std::tuple<bool> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetResourceCachingDisabled"); }
    static const bool isSync = false;

    explicit SetResourceCachingDisabled(bool disabled)
        : m_arguments(disabled)
    {
    }

    const Arguments& arguments() const
    {
        return m_arguments;
    }

private:
    Arguments m_arguments;
};

class SetUserInterfaceLayoutDirection {
public:
    typedef std::tuple<uint32_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetUserInterfaceLayoutDirection"); }
    static const bool isSync = false;

    explicit SetUserInterfaceLayoutDirection(uint32_t direction)
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

#if ENABLE(GAMEPAD)
class GamepadActivity {
public:
    typedef std::tuple<const Vector<WebKit::GamepadData>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("GamepadActivity"); }
    static const bool isSync = false;

    explicit GamepadActivity(const Vector<WebKit::GamepadData>& gamepadDatas)
        : m_arguments(gamepadDatas)
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

} // namespace WebPage
} // namespace Messages
