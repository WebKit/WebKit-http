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
#include <WebCore/FloatRect.h>
#include <wtf/Optional.h>

namespace WTF {
    class String;
}

namespace WebCore {
    class FloatPoint;
    class IntSize;
    class MachSendRight;
}

namespace WebKit {
    struct ColorSpaceData;
}

namespace Messages {
namespace DrawingArea {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("DrawingArea");
}

class UpdateBackingStoreState {
public:
    typedef std::tuple<uint64_t, bool, float, const WebCore::IntSize&, const WebCore::IntSize&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateBackingStoreState"); }
    static const bool isSync = false;

    UpdateBackingStoreState(uint64_t backingStoreStateID, bool respondImmediately, float deviceScaleFactor, const WebCore::IntSize& size, const WebCore::IntSize& scrollOffset)
        : m_arguments(backingStoreStateID, respondImmediately, deviceScaleFactor, size, scrollOffset)
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

#if PLATFORM(COCOA)
class UpdateGeometry {
public:
    typedef std::tuple<const WebCore::IntSize&, const WebCore::IntSize&, bool, const WebCore::MachSendRight&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateGeometry"); }
    static const bool isSync = false;

    UpdateGeometry(const WebCore::IntSize& viewSize, const WebCore::IntSize& layerPosition, bool flushSynchronously, const WebCore::MachSendRight& fencePort)
        : m_arguments(viewSize, layerPosition, flushSynchronously, fencePort)
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
class SetDeviceScaleFactor {
public:
    typedef std::tuple<float> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetDeviceScaleFactor"); }
    static const bool isSync = false;

    explicit SetDeviceScaleFactor(float deviceScaleFactor)
        : m_arguments(deviceScaleFactor)
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
class SetColorSpace {
public:
    typedef std::tuple<const WebKit::ColorSpaceData&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetColorSpace"); }
    static const bool isSync = false;

    explicit SetColorSpace(const WebKit::ColorSpaceData& colorSpace)
        : m_arguments(colorSpace)
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
class SetViewExposedRect {
public:
    typedef std::tuple<const Optional<WebCore::FloatRect>&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetViewExposedRect"); }
    static const bool isSync = false;

    explicit SetViewExposedRect(const Optional<WebCore::FloatRect>& viewExposedRect)
        : m_arguments(viewExposedRect)
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
class AdjustTransientZoom {
public:
    typedef std::tuple<double, const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AdjustTransientZoom"); }
    static const bool isSync = false;

    AdjustTransientZoom(double scale, const WebCore::FloatPoint& origin)
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
#endif

#if PLATFORM(COCOA)
class CommitTransientZoom {
public:
    typedef std::tuple<double, const WebCore::FloatPoint&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("CommitTransientZoom"); }
    static const bool isSync = false;

    CommitTransientZoom(double scale, const WebCore::FloatPoint& origin)
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
#endif

#if PLATFORM(COCOA)
class AcceleratedAnimationDidStart {
public:
    typedef std::tuple<uint64_t, const String&, double> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AcceleratedAnimationDidStart"); }
    static const bool isSync = false;

    AcceleratedAnimationDidStart(uint64_t layerID, const String& key, double startTime)
        : m_arguments(layerID, key, startTime)
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
class AcceleratedAnimationDidEnd {
public:
    typedef std::tuple<uint64_t, const String&> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AcceleratedAnimationDidEnd"); }
    static const bool isSync = false;

    AcceleratedAnimationDidEnd(uint64_t layerID, const String& key)
        : m_arguments(layerID, key)
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
class AddTransactionCallbackID {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("AddTransactionCallbackID"); }
    static const bool isSync = false;

    explicit AddTransactionCallbackID(uint64_t callbackID)
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

#if USE(TEXTURE_MAPPER) && PLATFORM(GTK) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
class SetNativeSurfaceHandleForCompositing {
public:
    typedef std::tuple<uint64_t> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("SetNativeSurfaceHandleForCompositing"); }
    static const bool isSync = false;

    explicit SetNativeSurfaceHandleForCompositing(uint64_t handle)
        : m_arguments(handle)
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

#if USE(TEXTURE_MAPPER) && PLATFORM(GTK) && !USE(REDIRECTED_XCOMPOSITE_WINDOW)
class DestroyNativeSurfaceHandleForCompositing {
public:
    typedef std::tuple<> Arguments;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DestroyNativeSurfaceHandleForCompositing"); }
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

} // namespace DrawingArea
} // namespace Messages
