/*
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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

#ifndef StillImageHaiku_h
#define StillImageHaiku_h

#include "Image.h"
#include <Bitmap.h>

namespace WebCore {

class StillImage : public Image {
public:
    static PassRefPtr<StillImage> create(const BBitmap& bitmap)
    {
        return adoptRef(new StillImage(bitmap));
    }

    static PassRefPtr<StillImage> createForRendering(const BBitmap* bitmap)
    {
        return adoptRef(new StillImage(bitmap));
    }

    virtual bool currentFrameKnownToBeOpaque() override;

    virtual void destroyDecodedData(bool = true) override;

    virtual FloatSize size() const override;
    virtual NativeImagePtr nativeImageForCurrentFrame() override;
    virtual void draw(GraphicsContext&, const FloatRect& dstRect, const FloatRect& srcRect, CompositeOperator, BlendMode, ImageOrientationDescription) override;

private:
    StillImage(const BBitmap& bitmap);
    StillImage(const BBitmap* bitmap);
    ~StillImage();

    const BBitmap* m_bitmap;
    bool m_ownsBitmap;
};

} // namespace WebCore

#endif // StillImageHaiku_h
