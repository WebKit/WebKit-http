/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef CCRenderPassDrawQuad_h
#define CCRenderPassDrawQuad_h

#include "cc/CCResourceProvider.h"
#include <public/WebCompositorQuad.h>
#include <wtf/PassOwnPtr.h>

namespace WebCore {

class CCRenderPass;

class CCRenderPassDrawQuad : public WebKit::WebCompositorQuad {
    WTF_MAKE_NONCOPYABLE(CCRenderPassDrawQuad);
public:
    static PassOwnPtr<CCRenderPassDrawQuad> create(const WebKit::WebCompositorSharedQuadState*, const IntRect&, int renderPassId, bool isReplica, CCResourceProvider::ResourceId maskResourceId, const IntRect& contentsChangedSinceLastFrame);

    int renderPassId() const { return m_renderPassId; }
    bool isReplica() const { return m_isReplica; }
    CCResourceProvider::ResourceId maskResourceId() const { return m_maskResourceId; }
    const IntRect& contentsChangedSinceLastFrame() const { return m_contentsChangedSinceLastFrame; }

    static const CCRenderPassDrawQuad* materialCast(const WebKit::WebCompositorQuad*);
private:
    CCRenderPassDrawQuad(const WebKit::WebCompositorSharedQuadState*, const IntRect&, int renderPassId, bool isReplica, CCResourceProvider::ResourceId maskResourceId, const IntRect& contentsChangedSinceLastFrame);

    int m_renderPassId;
    bool m_isReplica;
    CCResourceProvider::ResourceId m_maskResourceId;
    IntRect m_contentsChangedSinceLastFrame;
};

}

#endif
