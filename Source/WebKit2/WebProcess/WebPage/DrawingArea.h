/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DrawingArea_h
#define DrawingArea_h

#include "DrawingAreaInfo.h"
#include "LayerTreeContext.h"
#include <WebCore/FloatRect.h>
#include <WebCore/IntRect.h>
#include <WebCore/LayerFlushThrottleState.h>
#include <WebCore/PlatformScreen.h>
#include <WebCore/ViewState.h>
#include <functional>
#include <wtf/Forward.h>
#include <wtf/Noncopyable.h>

namespace IPC {
class Connection;
class MessageDecoder;
}

namespace WebCore {
class DisplayRefreshMonitor;
class FrameView;
class GraphicsLayer;
class GraphicsLayerFactory;
}

namespace WebKit {

struct ColorSpaceData;
class LayerTreeHost;
class WebPage;
struct WebPageCreationParameters;
struct WebPreferencesStore;

class DrawingArea {
    WTF_MAKE_NONCOPYABLE(DrawingArea);

public:
    static std::unique_ptr<DrawingArea> create(WebPage&, const WebPageCreationParameters&);
    virtual ~DrawingArea();
    
    DrawingAreaType type() const { return m_type; }
    
    void didReceiveDrawingAreaMessage(IPC::Connection*, IPC::MessageDecoder&);

    virtual void setNeedsDisplay() = 0;
    virtual void setNeedsDisplayInRect(const WebCore::IntRect&) = 0;
    virtual void scroll(const WebCore::IntRect& scrollRect, const WebCore::IntSize& scrollDelta) = 0;

    // FIXME: These should be pure virtual.
    virtual void pageBackgroundTransparencyChanged() { }
    virtual void forceRepaint() { }
    virtual bool forceRepaintAsync(uint64_t /*callbackID*/) { return false; }
    virtual void setLayerTreeStateIsFrozen(bool) { }
    virtual bool layerTreeStateIsFrozen() const { return false; }
    virtual LayerTreeHost* layerTreeHost() const { return 0; }

    virtual void setPaintingEnabled(bool) { }
    virtual void updatePreferences(const WebPreferencesStore&) { }
    virtual void mainFrameContentSizeChanged(const WebCore::IntSize&) { }

#if PLATFORM(COCOA)
    virtual void setExposedRect(const WebCore::FloatRect&) = 0;
    virtual WebCore::FloatRect exposedRect() const = 0;
    virtual void acceleratedAnimationDidStart(uint64_t /*layerID*/, const String& /*key*/, double /*startTime*/) { }
#endif
#if PLATFORM(IOS)
    virtual void setExposedContentRect(const WebCore::FloatRect&) = 0;
#endif
    virtual void mainFrameScrollabilityChanged(bool) { }

    virtual bool supportsAsyncScrolling() { return false; }

    virtual bool shouldUseTiledBackingForFrameView(const WebCore::FrameView*) { return false; }

    virtual WebCore::GraphicsLayerFactory* graphicsLayerFactory() { return nullptr; }
    virtual void setRootCompositingLayer(WebCore::GraphicsLayer*) = 0;
    virtual void scheduleCompositingLayerFlush() = 0;
    virtual void scheduleCompositingLayerFlushImmediately() = 0;

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
    virtual PassRefPtr<WebCore::DisplayRefreshMonitor> createDisplayRefreshMonitor(PlatformDisplayID);
#endif

#if USE(COORDINATED_GRAPHICS)
    virtual void didReceiveCoordinatedLayerTreeHostMessage(IPC::Connection*, IPC::MessageDecoder&) = 0;
#endif

    virtual void dispatchAfterEnsuringUpdatedScrollPosition(std::function<void ()>);

    virtual void viewStateDidChange(WebCore::ViewState::Flags, bool /*wantsDidUpdateViewState*/) { }
    virtual void setLayerHostingMode(LayerHostingMode) { }

    virtual bool markLayersVolatileImmediatelyIfPossible() { return true; }

    virtual bool adjustLayerFlushThrottling(WebCore::LayerFlushThrottleState::Flags) { return false; }

protected:
    DrawingArea(DrawingAreaType, WebPage&);

    DrawingAreaType m_type;
    WebPage& m_webPage;

private:
    // Message handlers.
    // FIXME: These should be pure virtual.
    virtual void updateBackingStoreState(uint64_t /*backingStoreStateID*/, bool /*respondImmediately*/, float /*deviceScaleFactor*/, const WebCore::IntSize& /*size*/, 
                                         const WebCore::IntSize& /*scrollOffset*/) { }
    virtual void didUpdate() { }

#if PLATFORM(COCOA)
    // Used by TiledCoreAnimationDrawingArea.
    virtual void updateGeometry(const WebCore::IntSize& viewSize, const WebCore::IntSize& layerPosition) { }
    virtual void setDeviceScaleFactor(float) { }
    virtual void setColorSpace(const ColorSpaceData&) { }

    virtual void adjustTransientZoom(double scale, WebCore::FloatPoint origin) { }
    virtual void commitTransientZoom(double scale, WebCore::FloatPoint origin) { }

    virtual void addTransactionCallbackID(uint64_t callbackID) { ASSERT_NOT_REACHED(); }
#endif
};

#define DRAWING_AREA_TYPE_CASTS(ToValueTypeName, predicate) \
    TYPE_CASTS_BASE(ToValueTypeName, DrawingArea, value, value->predicate, value.predicate)

} // namespace WebKit

#endif // DrawingArea_h
