/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Brent Fulgham <bfulgham@webkit.org>
 * Copyright (C) 2011 Igalia S.L.
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

#include "config.h"
#include "ShareableBitmap.h"

#include <WebCore/BitmapImage.h>
#include <WebCore/CairoUtilities.h>
#include <WebCore/GraphicsContext.h>
#include <WebCore/PlatformContextCairo.h>
#include <WebCore/NotImplemented.h>

using namespace WebCore;

namespace WebKit {

PassOwnPtr<GraphicsContext> ShareableBitmap::createGraphicsContext()
{
    RefPtr<cairo_surface_t> image = createCairoSurface();
    RefPtr<cairo_t> bitmapContext = adoptRef(cairo_create(image.get()));
    return adoptPtr(new GraphicsContext(bitmapContext.get()));
}

void ShareableBitmap::paint(GraphicsContext& context, const IntPoint& dstPoint, const IntRect& srcRect)
{
    RefPtr<cairo_surface_t> surface = adoptRef(cairo_image_surface_create_for_data(static_cast<unsigned char*>(data()),
                                                                                   CAIRO_FORMAT_ARGB32,
                                                                                   m_size.width(), m_size.height(),
                                                                                   m_size.width() * 4));
    FloatRect destRect(dstPoint, srcRect.size());
    context.platformContext()->drawSurfaceToContext(surface.get(), destRect, srcRect, &context);
}

void ShareableBitmap::paint(GraphicsContext&, float /*scaleFactor*/, const IntPoint& /*dstPoint*/, const IntRect& /*srcRect*/)
{
    // See <https://bugs.webkit.org/show_bug.cgi?id=64665>.
    notImplemented();
}

PassRefPtr<cairo_surface_t> ShareableBitmap::createCairoSurface()
{
    RefPtr<cairo_surface_t> image = adoptRef(cairo_image_surface_create_for_data(static_cast<unsigned char*>(data()),
                                                                                 CAIRO_FORMAT_ARGB32,
                                                                                 m_size.width(), m_size.height(),
                                                                                 m_size.width() * 4));

    ref(); // Balanced by deref in releaseSurfaceData.
    static cairo_user_data_key_t dataKey;
    cairo_surface_set_user_data(image.get(), &dataKey, this, releaseSurfaceData);
    return image.release();
}

void ShareableBitmap::releaseSurfaceData(void* typelessBitmap)
{
    static_cast<ShareableBitmap*>(typelessBitmap)->deref(); // Balanced by ref in createCairoSurface.
}

PassRefPtr<Image> ShareableBitmap::createImage()
{
    RefPtr<cairo_surface_t> surface = createCairoSurface();
    if (!surface)
        return 0;

    // BitmapImage::create adopts the cairo_surface_t that's passed in, which is why we need to leakRef here.
    return BitmapImage::create(surface.release().leakRef());
}

} // namespace WebKit
