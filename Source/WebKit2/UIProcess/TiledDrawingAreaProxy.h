/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef TiledDrawingAreaProxy_h
#define TiledDrawingAreaProxy_h

#if ENABLE(TILED_BACKING_STORE)

#include "DrawingAreaProxy.h"
#include <WebCore/GraphicsContext.h>
#include <WebCore/IntRect.h>
#include <wtf/HashSet.h>

#include "RunLoop.h"
#include "TiledDrawingAreaTile.h"

#if PLATFORM(MAC)
#include <wtf/RetainPtr.h>
#ifdef __OBJC__
@class WKView;
#else
class WKView;
#endif
#endif

namespace WebCore {
class GraphicsContext;
}

namespace WebKit {

class ShareableBitmap;
class TiledDrawingAreaTileSet;
class WebPageProxy;

#if PLATFORM(MAC)
typedef WKView PlatformWebView;
#elif PLATFORM(WIN)
class WebView;
typedef WebView PlatformWebView;
#elif PLATFORM(QT)
class TouchViewInterface;
typedef TouchViewInterface PlatformWebView;
#endif

class TiledDrawingAreaProxy : public DrawingAreaProxy {
public:
    static PassOwnPtr<TiledDrawingAreaProxy> create(PlatformWebView* webView, WebPageProxy*);

    TiledDrawingAreaProxy(PlatformWebView*, WebPageProxy*);
    virtual ~TiledDrawingAreaProxy();

    void setVisibleContentRect(const WebCore::IntRect&);
    void setContentsScale(float);

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachCompositingContext(uint32_t /* contextID */) { }
    virtual void detachCompositingContext() { }
#endif

    WebCore::IntSize tileSize() { return m_tileSize; }
    void setTileSize(const WebCore::IntSize&);

    double tileCreationDelay() const { return m_tileCreationDelay; }
    void setTileCreationDelay(double delay);

    // Tiled are dropped outside the keep area, and created for cover area. The values a relative to the viewport size.
    void getKeepAndCoverAreaMultipliers(WebCore::FloatSize& keepMultiplier, WebCore::FloatSize& coverMultiplier)
    {
        keepMultiplier = m_keepAreaMultiplier;
        coverMultiplier = m_coverAreaMultiplier;
    }
    void setKeepAndCoverAreaMultipliers(const WebCore::FloatSize& keepMultiplier, const WebCore::FloatSize& coverMultiplier);

    void tileBufferUpdateComplete();

    bool hasPendingUpdates() const;

private:
    WebPageProxy* page();
    void updateWebView(const Vector<WebCore::IntRect>& paintedArea);

    // DrawingAreaProxy
    virtual bool paint(const WebCore::IntRect&, PlatformDrawingContext);
    virtual void sizeDidChange();
    virtual void setPageIsVisible(bool isVisible);

    virtual void didSetSize(const WebCore::IntSize&);
    virtual void invalidate(const WebCore::IntRect& rect);
    virtual void tileUpdated(int tileID, const UpdateInfo& updateInfo, float scale, unsigned pendingUpdateCount);
    virtual void allTileUpdatesProcessed();

    void registerTile(int tileID, TiledDrawingAreaTile*);
    void unregisterTile(int tileID);
    void requestTileUpdate(int tileID, const WebCore::IntRect& dirtyRect);
    void cancelTileUpdate(int tileID);

    void startTileBufferUpdateTimer();
    void startTileCreationTimer();

    void tileBufferUpdateTimerFired();
    void tileCreationTimerFired();

    void updateTileBuffers();
    void createTiles();

    bool resizeEdgeTiles();
    void dropTilesOutsideRect(const WebCore::IntRect&);

    void disableTileSetUpdates(TiledDrawingAreaTileSet*);
    void removeAllTiles();

    void paint(TiledDrawingAreaTileSet*, const WebCore::IntRect&, WebCore::GraphicsContext&);
    float coverageRatio(TiledDrawingAreaTileSet*, const WebCore::IntRect&);

    WebCore::IntRect contentsRect() const;
    WebCore::IntRect visibleRect() const;

    WebCore::IntRect calculateKeepRect(const WebCore::IntRect& visibleRect) const;
    WebCore::IntRect calculateCoverRect(const WebCore::IntRect& visibleRect) const;

    WebCore::IntRect tileRectForCoordinate(const TiledDrawingAreaTile::Coordinate&) const;
    TiledDrawingAreaTile::Coordinate tileCoordinateForPoint(const WebCore::IntPoint&) const;
    double tileDistance(const WebCore::IntRect& viewport, const TiledDrawingAreaTile::Coordinate&);

private:
    bool m_isWaitingForDidSetFrameNotification;
    bool m_isVisible;

    WebCore::IntSize m_viewSize; // Size of the BackingStore as well.
    WebCore::IntSize m_lastSetViewSize;

    PlatformWebView* m_webView;

    OwnPtr<TiledDrawingAreaTileSet> m_currentTileSet;
    OwnPtr<TiledDrawingAreaTileSet> m_previousTileSet;

    WTF::HashMap<int, TiledDrawingAreaTile*> m_tilesByID;

    typedef RunLoop::Timer<TiledDrawingAreaProxy> TileTimer;
    TileTimer m_tileBufferUpdateTimer;
    TileTimer m_tileCreationTimer;

    WebCore::IntSize m_tileSize;
    double m_tileCreationDelay;
    WebCore::FloatSize m_keepAreaMultiplier;
    WebCore::FloatSize m_coverAreaMultiplier;

    WebCore::IntRect m_visibleContentRect;

    friend class TiledDrawingAreaTile;
};

} // namespace WebKit

#endif // TILED_BACKING_STORE

#endif // TiledDrawingAreaProxy_h
