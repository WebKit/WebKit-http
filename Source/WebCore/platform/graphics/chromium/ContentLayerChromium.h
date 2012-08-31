/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */


#ifndef ContentLayerChromium_h
#define ContentLayerChromium_h

#if USE(ACCELERATED_COMPOSITING)

#include "LayerPainterChromium.h"
#include "TiledLayerChromium.h"

class SkCanvas;

namespace WebCore {

class ContentLayerChromiumClient;
class FloatRect;
class IntRect;
class LayerTextureUpdater;

class ContentLayerPainter : public LayerPainterChromium {
    WTF_MAKE_NONCOPYABLE(ContentLayerPainter);
public:
    static PassOwnPtr<ContentLayerPainter> create(ContentLayerChromiumClient*);

    virtual void paint(SkCanvas*, const IntRect& contentRect, FloatRect& opaque) OVERRIDE;

private:
    explicit ContentLayerPainter(ContentLayerChromiumClient*);

    ContentLayerChromiumClient* m_client;
};

// A layer that renders its contents into an SkCanvas.
class ContentLayerChromium : public TiledLayerChromium {
public:
    static PassRefPtr<ContentLayerChromium> create(ContentLayerChromiumClient*);

    virtual ~ContentLayerChromium();

    void clearClient() { m_client = 0; }

    virtual bool drawsContent() const OVERRIDE;
    virtual void setTexturePriorities(const CCPriorityCalculator&) OVERRIDE;
    virtual void update(CCTextureUpdateQueue&, const CCOcclusionTracker*, CCRenderingStats&) OVERRIDE;
    virtual bool needMoreUpdates() OVERRIDE;

    virtual void setOpaque(bool) OVERRIDE;

protected:
    explicit ContentLayerChromium(ContentLayerChromiumClient*);


private:
    virtual LayerTextureUpdater* textureUpdater() const OVERRIDE { return m_textureUpdater.get(); }
    virtual void createTextureUpdaterIfNeeded() OVERRIDE;

    ContentLayerChromiumClient* m_client;
    RefPtr<LayerTextureUpdater> m_textureUpdater;
};

}
#endif // USE(ACCELERATED_COMPOSITING)

#endif
