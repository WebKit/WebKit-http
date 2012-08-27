/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#if USE(ACCELERATED_COMPOSITING)

#include "CCSolidColorLayerImpl.h"

#include "CCQuadSink.h"
#include "CCSolidColorDrawQuad.h"
#include <wtf/MathExtras.h>
#include <wtf/text/WTFString.h>

using namespace std;
using WebKit::WebTransformationMatrix;

namespace WebCore {

CCSolidColorLayerImpl::CCSolidColorLayerImpl(int id)
    : CCLayerImpl(id)
    , m_tileSize(256)
{
}

CCSolidColorLayerImpl::~CCSolidColorLayerImpl()
{
}

void CCSolidColorLayerImpl::appendQuads(CCQuadSink& quadSink, CCAppendQuadsData& appendQuadsData)
{
    CCSharedQuadState* sharedQuadState = quadSink.useSharedQuadState(createSharedQuadState());
    appendDebugBorderQuad(quadSink, sharedQuadState, appendQuadsData);

    // We create a series of smaller quads instead of just one large one so that the
    // culler can reduce the total pixels drawn.
    int width = contentBounds().width();
    int height = contentBounds().height();
    for (int x = 0; x < width; x += m_tileSize) {
        for (int y = 0; y < height; y += m_tileSize) {
            IntRect solidTileRect(x, y, min(width - x, m_tileSize), min(height - y, m_tileSize));
            quadSink.append(CCSolidColorDrawQuad::create(sharedQuadState, solidTileRect, backgroundColor()), appendQuadsData);
        }
    }
}

} // namespace WebCore
#endif // USE(ACCELERATED_COMPOSITING)
