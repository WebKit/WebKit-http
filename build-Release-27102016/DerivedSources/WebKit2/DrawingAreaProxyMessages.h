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

#ifndef DrawingAreaProxyMessages_h
#define DrawingAreaProxyMessages_h

#include "Arguments.h"
#include "MessageEncoder.h"
#include "StringReference.h"

namespace WebCore {
    class IntSize;
}

namespace WebKit {
    class UpdateInfo;
    class LayerTreeContext;
}

namespace Messages {
namespace DrawingAreaProxy {

static inline IPC::StringReference messageReceiverName()
{
    return IPC::StringReference("DrawingAreaProxy");
}

class Update {
public:
    typedef std::tuple<uint64_t, WebKit::UpdateInfo> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("Update"); }
    static const bool isSync = false;

    Update(uint64_t stateID, const WebKit::UpdateInfo& updateInfo)
        : m_arguments(stateID, updateInfo)
    {
    }

    const std::tuple<uint64_t, const WebKit::UpdateInfo&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::UpdateInfo&> m_arguments;
};

class DidUpdateBackingStoreState {
public:
    typedef std::tuple<uint64_t, WebKit::UpdateInfo, WebKit::LayerTreeContext> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateBackingStoreState"); }
    static const bool isSync = false;

    DidUpdateBackingStoreState(uint64_t backingStoreStateID, const WebKit::UpdateInfo& updateInfo, const WebKit::LayerTreeContext& context)
        : m_arguments(backingStoreStateID, updateInfo, context)
    {
    }

    const std::tuple<uint64_t, const WebKit::UpdateInfo&, const WebKit::LayerTreeContext&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::UpdateInfo&, const WebKit::LayerTreeContext&> m_arguments;
};

class EnterAcceleratedCompositingMode {
public:
    typedef std::tuple<uint64_t, WebKit::LayerTreeContext> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("EnterAcceleratedCompositingMode"); }
    static const bool isSync = false;

    EnterAcceleratedCompositingMode(uint64_t backingStoreStateID, const WebKit::LayerTreeContext& context)
        : m_arguments(backingStoreStateID, context)
    {
    }

    const std::tuple<uint64_t, const WebKit::LayerTreeContext&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::LayerTreeContext&> m_arguments;
};

class ExitAcceleratedCompositingMode {
public:
    typedef std::tuple<uint64_t, WebKit::UpdateInfo> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("ExitAcceleratedCompositingMode"); }
    static const bool isSync = false;

    ExitAcceleratedCompositingMode(uint64_t backingStoreStateID, const WebKit::UpdateInfo& updateInfo)
        : m_arguments(backingStoreStateID, updateInfo)
    {
    }

    const std::tuple<uint64_t, const WebKit::UpdateInfo&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::UpdateInfo&> m_arguments;
};

class UpdateAcceleratedCompositingMode {
public:
    typedef std::tuple<uint64_t, WebKit::LayerTreeContext> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("UpdateAcceleratedCompositingMode"); }
    static const bool isSync = false;

    UpdateAcceleratedCompositingMode(uint64_t backingStoreStateID, const WebKit::LayerTreeContext& context)
        : m_arguments(backingStoreStateID, context)
    {
    }

    const std::tuple<uint64_t, const WebKit::LayerTreeContext&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<uint64_t, const WebKit::LayerTreeContext&> m_arguments;
};

class WillEnterAcceleratedCompositingMode {
public:
    typedef std::tuple<uint64_t> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("WillEnterAcceleratedCompositingMode"); }
    static const bool isSync = false;

    explicit WillEnterAcceleratedCompositingMode(uint64_t backingStoreStateID)
        : m_arguments(backingStoreStateID)
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
class DidUpdateGeometry {
public:
    typedef std::tuple<> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("DidUpdateGeometry"); }
    static const bool isSync = false;

    const std::tuple<>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<> m_arguments;
};
#endif

#if PLATFORM(COCOA)
class IntrinsicContentSizeDidChange {
public:
    typedef std::tuple<WebCore::IntSize> DecodeType;

    static IPC::StringReference receiverName() { return messageReceiverName(); }
    static IPC::StringReference name() { return IPC::StringReference("IntrinsicContentSizeDidChange"); }
    static const bool isSync = false;

    explicit IntrinsicContentSizeDidChange(const WebCore::IntSize& newIntrinsicContentSize)
        : m_arguments(newIntrinsicContentSize)
    {
    }

    const std::tuple<const WebCore::IntSize&>& arguments() const
    {
        return m_arguments;
    }

private:
    std::tuple<const WebCore::IntSize&> m_arguments;
};
#endif

} // namespace DrawingAreaProxy
} // namespace Messages

#endif // DrawingAreaProxyMessages_h
