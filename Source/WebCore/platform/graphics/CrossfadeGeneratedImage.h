/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef CrossfadeGeneratedImage_h
#define CrossfadeGeneratedImage_h

#include "GeneratedImage.h"
#include "Image.h"
#include "ImageObserver.h"
#include "IntSize.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class CSSCrossfadeValue;

class CrossfadeGeneratedImage FINAL : public GeneratedImage {
public:
    static PassRefPtr<CrossfadeGeneratedImage> create(Image* fromImage, Image* toImage, float percentage, IntSize crossfadeSize, const IntSize& size)
    {
        return adoptRef(new CrossfadeGeneratedImage(fromImage, toImage, percentage, crossfadeSize, size));
    }

    virtual void setContainerSize(const IntSize&) OVERRIDE { }
    virtual bool usesContainerSize() const OVERRIDE { return false; }
    virtual bool hasRelativeWidth() const OVERRIDE { return false; }
    virtual bool hasRelativeHeight() const OVERRIDE { return false; }

    virtual IntSize size() const OVERRIDE { return m_crossfadeSize; }

protected:
    virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode, ImageOrientationDescription) OVERRIDE;
    virtual void drawPattern(GraphicsContext*, const FloatRect& srcRect, const AffineTransform& patternTransform, const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator, const FloatRect& dstRect, BlendMode) OVERRIDE;

    CrossfadeGeneratedImage(Image* fromImage, Image* toImage, float percentage, IntSize crossfadeSize, const IntSize&);

private:
    void drawCrossfade(GraphicsContext*);

    Image* m_fromImage;
    Image* m_toImage;

    float m_percentage;
    IntSize m_crossfadeSize;
};

}

#endif
