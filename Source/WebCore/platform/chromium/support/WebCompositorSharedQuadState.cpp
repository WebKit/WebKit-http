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

#include <public/WebCompositorSharedQuadState.h>

#include "FloatQuad.h"
#include "cc/CCMathUtil.h"

using WebCore::FloatQuad;
using WebCore::IntRect;

namespace WebKit {

WebCompositorSharedQuadState::WebCompositorSharedQuadState()
    : id(0)
    , opacity(0)
    , opaque(false)
{
}

PassOwnPtr<WebCompositorSharedQuadState> WebCompositorSharedQuadState::create(int id, const WebTransformationMatrix& quadTransform, const IntRect& visibleContentRect, const IntRect& scissorRect, float opacity, bool opaque)
{
    return adoptPtr(new WebCompositorSharedQuadState(id, quadTransform, visibleContentRect, scissorRect, opacity, opaque));
}

WebCompositorSharedQuadState::WebCompositorSharedQuadState(int id, const WebTransformationMatrix& quadTransform, const IntRect& visibleContentRect, const IntRect& scissorRect, float opacity, bool opaque)
    : id(id)
    , quadTransform(quadTransform)
    , visibleContentRect(visibleContentRect)
    , scissorRect(scissorRect)
    , opacity(opacity)
    , opaque(opaque)
{
}

bool WebCompositorSharedQuadState::isLayerAxisAlignedIntRect() const
{
    // Note: this doesn't consider window or projection matrices.
    // Assume that they're orthonormal and have integer scales and translations.
    bool clipped = false;
    FloatQuad quad = WebCore::CCMathUtil::mapQuad(quadTransform, FloatQuad(IntRect(visibleContentRect)), clipped);
    return !clipped && quad.isRectilinear() && quad.boundingBox().isExpressibleAsIntRect();
}

}
