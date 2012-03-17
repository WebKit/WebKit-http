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

#ifndef CCTextureDrawQuad_h
#define CCTextureDrawQuad_h

#include "cc/CCDrawQuad.h"
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class CCLayerImpl;
class CCTextureDrawQuad : public CCDrawQuad {
    WTF_MAKE_NONCOPYABLE(CCTextureDrawQuad);
public:
    static PassOwnPtr<CCTextureDrawQuad> create(const CCSharedQuadState*, const IntRect&, unsigned textureId, bool hasAlpha, bool premultipliedAlpha, const FloatRect& uvRect, bool flipped, const IntSize& ioSurfaceSize, unsigned ioSurfaceTextureId);

    unsigned textureId() const { return  m_textureId; }
    bool hasAlpha() const { return  m_hasAlpha; }
    bool premultipliedAlpha() const { return  m_premultipliedAlpha; }
    FloatRect uvRect() const { return m_uvRect; }
    bool flipped() const { return m_flipped; }

    const IntSize& ioSurfaceSize() const { return m_ioSurfaceSize; }
    unsigned ioSurfaceTextureId() const { return m_ioSurfaceTextureId; }
 
private:
    CCTextureDrawQuad(const CCSharedQuadState*, const IntRect&, unsigned texture_id, bool hasAlpha, bool premultipliedAlpha, const FloatRect& uvRect, bool flipped, const IntSize& ioSurfaceSize, unsigned ioSurfaceTextureId);
    
    unsigned m_textureId;
    bool m_hasAlpha;
    bool m_premultipliedAlpha;
    FloatRect m_uvRect;
    bool m_flipped;
    IntSize m_ioSurfaceSize;
    unsigned m_ioSurfaceTextureId;
};

}

#endif
