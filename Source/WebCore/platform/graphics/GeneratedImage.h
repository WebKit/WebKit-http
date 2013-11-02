/*
 * Copyright (C) 2008, 2013 Apple Computer, Inc.  All rights reserved.
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

#ifndef GeneratedImage_h
#define GeneratedImage_h

#include "Image.h"

#include "IntSize.h"
#include <wtf/RefPtr.h>

namespace WebCore {

class GeneratedImage : public Image {
public:
    virtual bool hasSingleSecurityOrigin() const OVERRIDE { return true; }

    virtual void setContainerSize(const IntSize& size) OVERRIDE { m_size = size; }
    virtual bool usesContainerSize() const OVERRIDE { return true; }
    virtual bool hasRelativeWidth() const OVERRIDE { return true; }
    virtual bool hasRelativeHeight() const OVERRIDE { return true; }
    virtual void computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio) OVERRIDE;

    virtual IntSize size() const OVERRIDE { return m_size; }

    // Assume that generated content has no decoded data we need to worry about
    virtual void destroyDecodedData(bool /*destroyAll*/ = true) OVERRIDE { }
    virtual unsigned decodedSize() const OVERRIDE { return 0; }

protected:
    virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode, ImageOrientationDescription) OVERRIDE = 0;
    virtual void drawPattern(GraphicsContext*, const FloatRect& srcRect, const AffineTransform& patternTransform,
        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator, const FloatRect& destRect, BlendMode) OVERRIDE = 0;

    // FIXME: Implement this to be less conservative.
    virtual bool currentFrameKnownToBeOpaque() OVERRIDE { return false; }

    GeneratedImage() { }

private:
    IntSize m_size;
};

}

#endif
