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

#include "config.h"
#include "StillImageHaiku.h"

#include "GraphicsContext.h"
#include "IntSize.h"
#include <View.h>

namespace WebCore {

StillImage::StillImage(const BBitmap& bitmap)
    : m_bitmap(new BBitmap(bitmap))
    , m_ownsBitmap(true)
{}

StillImage::StillImage(const BBitmap* bitmap)
    : m_bitmap(bitmap)
    , m_ownsBitmap(false)
{}

StillImage::~StillImage()
{
    if (m_ownsBitmap)
        delete m_bitmap;
}


bool StillImage::currentFrameKnownToBeOpaque()
{
    color_space space = m_bitmap->ColorSpace();
    return space != B_RGB32 && space != B_RGBA15;
}

void StillImage::destroyDecodedData(bool destroyAll)
{
    // This is used for "large" animations to free image data.
    // It appears it would not apply to StillImage.
}

FloatSize StillImage::size() const
{
    return FloatSize(m_bitmap->Bounds().Width() + 1., m_bitmap->Bounds().Height() + 1.);
}

NativeImagePtr StillImage::nativeImageForCurrentFrame()
{
    return const_cast<NativeImagePtr>(m_bitmap);
}

void StillImage::draw(GraphicsContext& context, const FloatRect& destRect,
                      const FloatRect& sourceRect, CompositeOperator op, BlendMode, ImageOrientationDescription)
{
    if (!m_bitmap->IsValid())
        return;
    
    context.save();
    context.setCompositeOperation(op);
    context.platformContext()->DrawBitmap(m_bitmap, sourceRect, destRect);
    context.restore();
}

}
