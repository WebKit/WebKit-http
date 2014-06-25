/*
    Copyright (C) 2012 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"

#include "AcceleratedCompositingContext.h"
#include "Bitmap.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "GraphicsLayerTextureMapper.h"
#include "MainFrame.h"
#include "NotImplemented.h"
#include "TextureMapperLayer.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebView.h"

const double compositingFrameRate = 60;

namespace WebCore {

AcceleratedCompositingContext::AcceleratedCompositingContext(BWebView* view)
    : m_view(view)
    , m_rootLayer(nullptr)
    , m_syncTimer(this, &AcceleratedCompositingContext::syncLayers)
{
    ASSERT(m_view);

    m_textureMapper = TextureMapper::create(TextureMapper::SoftwareMode);
}

AcceleratedCompositingContext::~AcceleratedCompositingContext()
{
    m_syncTimer.stop();
    m_textureMapper = nullptr;
}

void AcceleratedCompositingContext::syncLayers(Timer<AcceleratedCompositingContext>*)
{
    flushAndRenderLayers();
}

void AcceleratedCompositingContext::flushAndRenderLayers()
{
    MainFrame& frame = *(MainFrame*)(m_view->WebPage()->MainFrame()->Frame());
    if (!frame.contentRenderer() || !frame.view())
        return;
    frame.view()->updateLayoutAndStyleIfNeededRecursive();

    if (flushPendingLayerChanges())
        paintToGraphicsContext();
}

bool AcceleratedCompositingContext::flushPendingLayerChanges()
{
    if (m_rootLayer)
        m_rootLayer->flushCompositingStateForThisLayerOnly();
    return m_view->WebPage()->MainFrame()->Frame()->view()->flushCompositingStateIncludingSubframes();
}

void AcceleratedCompositingContext::paintToGraphicsContext()
{
    BView* target = m_view->OffscreenView();
    GraphicsContext context(target);

    m_textureMapper->setGraphicsContext(&context);

    if(target->LockLooper()) {
        compositeLayers(target->Bounds());
        target->Sync();
        target->UnlockLooper();
    }

    if(m_view->LockLooper()) {
        m_view->Invalidate();
        m_view->UnlockLooper();
    }
}

void AcceleratedCompositingContext::compositeLayers(BRect updateRect)
{
    TextureMapperLayer* currentRootLayer = toTextureMapperLayer(m_rootLayer);
    if (!currentRootLayer)
        return;

    currentRootLayer->setTextureMapper(m_textureMapper.get());
    currentRootLayer->applyAnimationsRecursively();

    m_textureMapper->beginPainting();
    m_textureMapper->beginClip(TransformationMatrix(), updateRect);

    currentRootLayer->paint();
    m_fpsCounter.updateFPSAndDisplay(m_textureMapper.get());
    m_textureMapper->endClip();
    m_textureMapper->endPainting();

    if (currentRootLayer->descendantsOrSelfHaveRunningAnimations() && !m_syncTimer.isActive())
        m_syncTimer.startOneShot(1 / compositingFrameRate);
}

void AcceleratedCompositingContext::setRootGraphicsLayer(GraphicsLayer* rootLayer)
{
    m_rootLayer = rootLayer;

    if (!m_syncTimer.isActive())
        m_syncTimer.startOneShot(0);
}

} // namespace WebCore
