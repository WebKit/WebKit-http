/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2008 Andrea Anzani <andrea.anzani@gmail.com>
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
#include "Image.h"

#include "BitmapImage.h"
#include "FloatRect.h"
#include "GraphicsContext.h"
#include "ImageObserver.h"
#include "IntRect.h"
#include "NotImplemented.h"
#include "SharedBuffer.h"
#include "TransformationMatrix.h"
#include <Application.h>
#include <Bitmap.h>
#include <View.h>

// This function loads resources from WebKit
Vector<char> loadResourceIntoArray(const char*);


namespace WebCore {

FloatSize nativeImageDrawingScale(GraphicsContext& context, const FloatRect& destRect, const FloatRect& srcRect)
{
    auto transformedDestinationRect = context.getCTM().mapRect(destRect);
    return { static_cast<float>(transformedDestinationRect.width() / srcRect.width()), static_cast<float>(transformedDestinationRect.height() / srcRect.height()) };
}


void drawNativeImage(WTF::RefPtr<WebCore::BitmapRef> const& image,
    WebCore::GraphicsContext& ctxt, WebCore::FloatRect const& dst,
    WebCore::FloatRect const& src, WebCore::IntSize const& size,
    WebCore::ImagePaintingOptions const& options)
{
    ctxt.drawNativeImage(image, size, dst, src, options);
}


NativeImagePtr BitmapImage::getBBitmap()
{
    return frameImageAtIndex(0);
}

NativeImagePtr BitmapImage::getFirstBBitmapOfSize(const IntSize& size)
{
    size_t count = frameCount();
    for (size_t i = 0; i < count; ++i) {
        NativeImagePtr bitmap = frameImageAtIndex(i);
        if (bitmap && bitmap->Bounds().IntegerWidth() + 1 == size.width()
            && bitmap->Bounds().IntegerHeight() + 1 == size.height()) {
            return bitmap;
        }
    }

    // Fallback to the default getBBitmap if we can't find the right size
    return getBBitmap();
}

} // namespace WebCore

