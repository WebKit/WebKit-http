/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2015 The Qt Company Ltd
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */
#pragma once

#if USE(TEXTURE_MAPPER)

#include <WebCore/GraphicsLayer.h>
#include <WebCore/TextureMapper.h>
#include <WebCore/TextureMapperFPSCounter.h>
#include <WebCore/Timer.h>

class QWebFrameAdapter;
class QWebPageClient;

namespace WebCore {

class TextureMapperLayer;

class TextureMapperLayerClientQt final : public GraphicsLayerClient {
public:
    TextureMapperLayerClientQt(QWebFrameAdapter&);
    ~TextureMapperLayerClientQt();
    void syncRootLayer();
    TextureMapperLayer* rootLayer();

    void markForSync(bool scheduleSync);

    void setRootGraphicsLayer(GraphicsLayer*);

    void syncLayers();

    void renderCompositedLayers(GraphicsContext&, const IntRect& clip);

private:
    QWebPageClient* pageClient() const;

    QWebFrameAdapter& m_frame;
    RefPtr<GraphicsLayer> m_rootGraphicsLayer;
    Timer m_syncTimer;
    WebCore::TextureMapperLayer* m_rootTextureMapperLayer;
    std::unique_ptr<WebCore::TextureMapper> m_textureMapper;
    WebCore::TextureMapperFPSCounter m_fpsCounter;
};

}

#endif
