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

#ifndef WebExternalTextureLayer_h
#define WebExternalTextureLayer_h

#include "WebCommon.h"
#include "WebFloatRect.h"
#include "WebLayer.h"

namespace WebKit {

class WebExternalTextureLayerClient;

// This class represents a layer that renders a texture that is generated
// externally (not managed by the WebLayerTreeView).
// The texture will be used by the WebLayerTreeView during compositing passes.
// When in single-thread mode, this means during WebLayerTreeView::composite().
// When using the threaded compositor, this can mean at an arbitrary time until
// the WebLayerTreeView is destroyed.
class WebExternalTextureLayer {
public:
    // The owner of this layer may optionally provide a client. This client will
    // be called whenever the compositor wishes to produce a new frame and can
    // provide a new front buffer texture ID. This is useful if the client wants to
    // implement a double-buffering scheme that is synchronized with the compositor, for instance.
    // The client must outlive the WebExternalTextureLayer.
    WEBKIT_EXPORT static WebExternalTextureLayer* create(WebExternalTextureLayerClient* = 0);

    virtual ~WebExternalTextureLayer() { }

    virtual WebLayer* layer() = 0;

    // Sets the texture id that represents the layer, in the namespace of the
    // compositor context.
    virtual void setTextureId(unsigned) = 0;

    // Sets whether or not the texture should be flipped in the Y direction when
    // rendered.
    virtual void setFlipped(bool) = 0;

    // Sets the rect in UV space of the texture that is mapped to the layer
    // bounds.
    virtual void setUVRect(const WebFloatRect&) = 0;

    // Sets whether every pixel in this layer is opaque. Defaults to false.
    virtual void setOpaque(bool) = 0;

    // Sets whether this layer's texture has premultiplied alpha or not. Defaults to true.
    virtual void setPremultipliedAlpha(bool) = 0;

    // Indicates that the most recently provided texture ID is about to be modified externally.
    virtual void willModifyTexture() = 0;

    // Sets whether this context should be rate limited by the compositor. Rate limiting works by blocking
    // invalidate() and invalidateRect() calls if the compositor is too many frames behind.
    virtual void setRateLimitContext(bool) = 0;
};

} // namespace WebKit

#endif // WebExternalTextureLayer_h
