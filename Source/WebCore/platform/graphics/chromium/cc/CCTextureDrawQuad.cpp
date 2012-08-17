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

#include "CCTextureDrawQuad.h"

namespace WebCore {

PassOwnPtr<CCTextureDrawQuad> CCTextureDrawQuad::create(const CCSharedQuadState* sharedQuadState, const IntRect& quadRect, unsigned resourceId, bool premultipliedAlpha, const FloatRect& uvRect, bool flipped)
{
    return adoptPtr(new CCTextureDrawQuad(sharedQuadState, quadRect, resourceId, premultipliedAlpha, uvRect, flipped));
}

CCTextureDrawQuad::CCTextureDrawQuad(const CCSharedQuadState* sharedQuadState, const IntRect& quadRect, unsigned resourceId, bool premultipliedAlpha, const FloatRect& uvRect, bool flipped)
    : CCDrawQuad(sharedQuadState, CCDrawQuad::TextureContent, quadRect)
    , m_resourceId(resourceId)
    , m_premultipliedAlpha(premultipliedAlpha)
    , m_uvRect(uvRect)
    , m_flipped(flipped)
{
}

void CCTextureDrawQuad::setNeedsBlending()
{
    m_needsBlending = true;
}

const CCTextureDrawQuad* CCTextureDrawQuad::materialCast(const CCDrawQuad* quad)
{
    ASSERT(quad->material() == CCDrawQuad::TextureContent);
    return static_cast<const CCTextureDrawQuad*>(quad);
}

}
