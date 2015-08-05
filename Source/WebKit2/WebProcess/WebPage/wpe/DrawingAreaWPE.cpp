

#include "config.h"
#include "DrawingAreaWPE.h"

#include "WebPage.h"
#include "WebPreferencesKeys.h"
#include "WebPreferencesStore.h"
#include <WebCore/DisplayRefreshMonitor.h>
#include <WebCore/MainFrame.h>
#include <WebCore/Settings.h>
#include <cstdio>

using namespace WebCore;

namespace WebKit {

DrawingAreaWPE::DrawingAreaWPE(WebPage& webPage, const WebPageCreationParameters&)
    : DrawingArea(DrawingAreaTypeWPE, webPage)
{
    fprintf(stderr, "DrawingAreaWPE: ctor\n");
    webPage.corePage()->settings().setForceCompositingMode(true);
    enterAcceleratedCompositingMode(0);
}

DrawingAreaWPE::~DrawingAreaWPE()
{
    fprintf(stderr, "DrawingAreaWPE: ctor\n");
}

void DrawingAreaWPE::setNeedsDisplay()
{
    ASSERT(m_layerTreeHost);
    m_layerTreeHost->setNonCompositedContentsNeedDisplay();
}

void DrawingAreaWPE::setNeedsDisplayInRect(const IntRect& rect)
{
    ASSERT(m_layerTreeHost);
    m_layerTreeHost->setNonCompositedContentsNeedDisplayInRect(rect);
}

void DrawingAreaWPE::scroll(const IntRect&, const IntSize&)
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
}

void DrawingAreaWPE::pageBackgroundTransparencyChanged()
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
    if (m_layerTreeHost)
        m_layerTreeHost->pageBackgroundTransparencyChanged();
}

void DrawingAreaWPE::forceRepaint()
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
}

bool DrawingAreaWPE::forceRepaintAsync(uint64_t callbackID)
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
    return m_layerTreeHost && m_layerTreeHost->forceRepaintAsync(callbackID);
}

void DrawingAreaWPE::setLayerTreeStateIsFrozen(bool frozen)
{
    fprintf(stderr, "DrawingAreaWPE: %s, frozen %d\n", __func__, frozen);
}

void DrawingAreaWPE::setPaintingEnabled(bool)
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
}

void DrawingAreaWPE::updatePreferences(const WebPreferencesStore& store)
{
    m_webPage.corePage()->settings().setForceCompositingMode(store.getBoolValueForKey(WebPreferencesKey::forceCompositingModeKey()));
}

void DrawingAreaWPE::mainFrameContentSizeChanged(const WebCore::IntSize& size)
{
    if (m_layerTreeHost)
        m_layerTreeHost->sizeDidChange(size);
}

GraphicsLayerFactory* DrawingAreaWPE::graphicsLayerFactory()
{
    if (m_layerTreeHost)
        return m_layerTreeHost->graphicsLayerFactory();
    return nullptr;
}

void DrawingAreaWPE::setRootCompositingLayer(GraphicsLayer* graphicsLayer)
{
    ASSERT(m_layerTreeHost);
    m_layerTreeHost->setRootCompositingLayer(graphicsLayer);
}

void DrawingAreaWPE::scheduleCompositingLayerFlush()
{
    if (m_layerTreeHost)
        m_layerTreeHost->scheduleLayerFlush();
}

void DrawingAreaWPE::scheduleCompositingLayerFlushImmediately()
{
    scheduleCompositingLayerFlush();
}

#if USE(REQUEST_ANIMATION_FRAME_DISPLAY_MONITOR)
RefPtr<WebCore::DisplayRefreshMonitor> DrawingAreaWPE::createDisplayRefreshMonitor(PlatformDisplayID displayID)
{
    fprintf(stderr, "DrawingAreaWPE: %s\n", __func__);
    if (m_layerTreeHost)
        return m_layerTreeHost->createDisplayRefreshMonitor(displayID);

    return nullptr;
}
#endif

void DrawingAreaWPE::attachViewOverlayGraphicsLayer(WebCore::Frame* frame, WebCore::GraphicsLayer* viewOverlayRootLayer)
{
    if (!frame->isMainFrame())
        return;

    ASSERT(m_layerTreeHost);
    m_layerTreeHost->setViewOverlayRootLayer(viewOverlayRootLayer);
}

void DrawingAreaWPE::enterAcceleratedCompositingMode(GraphicsLayer* graphicsLayer)
{
    ASSERT(!m_layerTreeHost);
    m_layerTreeHost = LayerTreeHost::create(&m_webPage);
    m_layerTreeHost->setRootCompositingLayer(graphicsLayer);
}

} // namespace WebKit
