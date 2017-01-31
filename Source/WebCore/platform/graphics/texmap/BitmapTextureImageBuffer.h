/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2014 Igalia S.L.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef BitmapTextureImageBuffer_h
#define BitmapTextureImageBuffer_h

#include "BitmapTexture.h"
#include "ImageBuffer.h"
#include "IntRect.h"
#include "IntSize.h"

namespace WebCore {

class GraphicsContext;

class BitmapTextureImageBuffer final : public BitmapTexture {
public:
    static PassRefPtr<BitmapTexture> create() { return adoptRef(new BitmapTextureImageBuffer); }
    IntSize size() const final { return m_image->internalSize(); }
    void didReset() final;
    bool isValid() const final { return m_image.get(); }
    void updateContents(Image*, const IntRect&, const IntPoint&, UpdateContentsFlag) final;
    void updateContents(TextureMapper&, GraphicsLayer*, const IntRect& target, const IntPoint& offset, UpdateContentsFlag, float scale) final;
    void updateContents(const void*, const IntRect& target, const IntPoint& sourceOffset, int bytesPerLine, UpdateContentsFlag) final;
    PassRefPtr<BitmapTexture> applyFilters(TextureMapper&, const FilterOperations&) final;

    inline GraphicsContext* graphicsContext() { return m_image ? &m_image->context() : nullptr; }
    ImageBuffer* image() const { return m_image.get(); }

private:
    BitmapTextureImageBuffer() { }
    std::unique_ptr<ImageBuffer> m_image;
};

}

#endif // BitmapTextureImageBuffer_h
