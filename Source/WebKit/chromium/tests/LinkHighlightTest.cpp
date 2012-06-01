/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "LinkHighlight.h"

#include "AnimationIdVendor.h"
#include "GraphicsLayerChromium.h"
#include "GraphicsLayerClient.h"
#include "IntRect.h"
#include "Path.h"
#include <gtest/gtest.h>
#include <public/WebTransformationMatrix.h>
#include <wtf/PassOwnPtr.h>

using namespace WebCore;
using WebKit::WebTransformationMatrix;

namespace {

class MockGraphicsLayerClient : public GraphicsLayerClient {
public:
    virtual void notifyAnimationStarted(const GraphicsLayer*, double time) OVERRIDE { }
    virtual void notifySyncRequired(const GraphicsLayer*) OVERRIDE { }
    virtual void paintContents(const GraphicsLayer*, GraphicsContext&, GraphicsLayerPaintingPhase, const IntRect& inClip) OVERRIDE { }
    virtual bool showDebugBorders(const GraphicsLayer*) const OVERRIDE { return false; }
    virtual bool showRepaintCounter(const GraphicsLayer*) const OVERRIDE { return false; }
};

TEST(LinkHighlightTest, verifyLinkHighlightLayer)
{
    Path highlightPath;
    highlightPath.addRect(FloatRect(5, 6, 12, 8));
    IntRect pathBoundingRect = enclosingIntRect(highlightPath.boundingRect());

    RefPtr<LinkHighlight> highlight = LinkHighlight::create(0, highlightPath, AnimationIdVendor::LinkHighlightAnimationId, AnimationIdVendor::getNextGroupId());
    ASSERT_TRUE(highlight.get());
    ContentLayerChromium* contentLayer = highlight->contentLayer();
    ASSERT_TRUE(contentLayer);

    EXPECT_EQ(pathBoundingRect.size(), contentLayer->bounds());
    EXPECT_TRUE(contentLayer->transform().isIdentityOrTranslation());
    EXPECT_TRUE(contentLayer->transform().isIntegerTranslation());

    float expectedXTranslation = pathBoundingRect.x() + pathBoundingRect.width() / 2;
    float expectedYTranslation = pathBoundingRect.y() + pathBoundingRect.height() / 2;
    EXPECT_FLOAT_EQ(expectedXTranslation, contentLayer->transform().m41());
    EXPECT_FLOAT_EQ(expectedYTranslation, contentLayer->transform().m42());
}

TEST(LinkHighlightTest, verifyGraphicsLayerChromiumEmbedding)
{
    MockGraphicsLayerClient client;
    OwnPtr<GraphicsLayerChromium> graphicsLayer = static_pointer_cast<GraphicsLayerChromium>(GraphicsLayer::create(&client));
    ASSERT_TRUE(graphicsLayer.get());

    Path highlightPath;
    highlightPath.addRect(FloatRect(5, 5, 10, 8));

    // Neither of the following operations should crash.
    graphicsLayer->addLinkHighlight(highlightPath);
    graphicsLayer->didFinishLinkHighlight();
}

} // namespace
