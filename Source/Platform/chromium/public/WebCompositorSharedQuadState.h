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

#ifndef WebCompositorSharedQuadState_h
#define WebCompositorSharedQuadState_h

#include "WebCommon.h"

#if WEBKIT_IMPLEMENTATION
#include "IntRect.h"
#include <wtf/PassOwnPtr.h>
#endif
#include "WebRect.h"
#include "WebTransformationMatrix.h"

namespace WebKit {

class WebCompositorSharedQuadState {
public:
    int id;

    // Transforms from quad's original content space to its target content space.
    WebTransformationMatrix quadTransform;
    // This rect lives in the content space for the quad's originating layer.
    WebRect visibleContentRect;
    // This rect lives in the quad's target content space.
    WebRect scissorRect;
    float opacity;
    bool opaque;

    WebCompositorSharedQuadState();

#if WEBKIT_IMPLEMENTATION
    static PassOwnPtr<WebCompositorSharedQuadState> create(int id, const WebTransformationMatrix& quadTransform, const WebCore::IntRect& visibleContentRect, const WebCore::IntRect& scissorRect, float opacity, bool opaque);
    WebCompositorSharedQuadState(int id, const WebTransformationMatrix& quadTransform, const WebCore::IntRect& visibleContentRect, const WebCore::IntRect& scissorRect, float opacity, bool opaque);
    bool isLayerAxisAlignedIntRect() const;
#endif
};

}

#endif
