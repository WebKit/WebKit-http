/*
 * Copyright (C) 2014 Haiku, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "GraphicsLayer.h"

#include "GraphicsLayerFactory.h"


namespace WebCore {

class GraphicsLayerHaiku: public GraphicsLayer {
public:
    GraphicsLayerHaiku(GraphicsLayerClient* client);

    void setNeedsDisplay();
    void setNeedsDisplayInRect(const FloatRect&, ShouldClipToLayer);
};

GraphicsLayerHaiku::GraphicsLayerHaiku(GraphicsLayerClient* client)
    : GraphicsLayer(client)
{
}

void GraphicsLayerHaiku::setNeedsDisplay()
{
    puts(__func__);
}

void GraphicsLayerHaiku::setNeedsDisplayInRect(const FloatRect&, ShouldClipToLayer)
{
    puts(__func__);
}

std::unique_ptr<GraphicsLayer> GraphicsLayer::create(GraphicsLayerFactory* factory, GraphicsLayerClient* client)
{
    std::unique_ptr<GraphicsLayer> graphicsLayer;
    if (!factory)
        graphicsLayer = std::make_unique<GraphicsLayerHaiku>(client);
    else
        graphicsLayer = factory->createGraphicsLayer(client);

    graphicsLayer->initialize();

    return std::move(graphicsLayer);
}

}
