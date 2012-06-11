/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
#include "StyleCachedImageSet.h"

#if ENABLE(CSS_IMAGE_SET)

#include "CSSImageSetValue.h"
#include "CachedImage.h"
#include "RenderObject.h"

namespace WebCore {

StyleCachedImageSet::StyleCachedImageSet(CachedImage* image, float imageScaleFactor, CSSImageSetValue* value)
    : m_bestFitImage(image)
    , m_imageScaleFactor(imageScaleFactor)
    , m_imageSetValue(value)
{
    m_isCachedImageSet = true;
    m_bestFitImage->addClient(this);
}


StyleCachedImageSet::~StyleCachedImageSet()
{
    m_bestFitImage->removeClient(this);
}

PassRefPtr<CSSValue> StyleCachedImageSet::cssValue() const
{
    return m_imageSetValue;
}

bool StyleCachedImageSet::canRender(const RenderObject* renderer, float multiplier) const
{
    return m_bestFitImage->canRender(renderer, multiplier);
}

bool StyleCachedImageSet::isLoaded() const
{
    return m_bestFitImage->isLoaded();
}

bool StyleCachedImageSet::errorOccurred() const
{
    return m_bestFitImage->errorOccurred();
}

IntSize StyleCachedImageSet::imageSize(const RenderObject* renderer, float multiplier) const
{
    IntSize scaledImageSize = m_bestFitImage->imageSizeForRenderer(renderer, multiplier);
    scaledImageSize.scale(1 / m_imageScaleFactor);
    return scaledImageSize;
}

bool StyleCachedImageSet::imageHasRelativeWidth() const
{
    return m_bestFitImage->imageHasRelativeWidth();
}

bool StyleCachedImageSet::imageHasRelativeHeight() const
{
    return m_bestFitImage->imageHasRelativeHeight();
}

void StyleCachedImageSet::computeIntrinsicDimensions(const RenderObject*, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    m_bestFitImage->computeIntrinsicDimensions(intrinsicWidth, intrinsicHeight, intrinsicRatio);
}

bool StyleCachedImageSet::usesImageContainerSize() const
{
    return m_bestFitImage->usesImageContainerSize();
}

void StyleCachedImageSet::setContainerSizeForRenderer(const RenderObject* renderer, const IntSize& imageContainerSize, float imageContainerZoomFactor)
{
    m_bestFitImage->setContainerSizeForRenderer(renderer, imageContainerSize, imageContainerZoomFactor);
}

void StyleCachedImageSet::addClient(RenderObject* renderer)
{
    m_bestFitImage->addClient(renderer);
}

void StyleCachedImageSet::removeClient(RenderObject* renderer)
{
    m_bestFitImage->removeClient(renderer);
}

PassRefPtr<Image> StyleCachedImageSet::image(RenderObject* renderer, const IntSize&) const
{
    return m_bestFitImage->imageForRenderer(renderer);
}

} // namespace WebCore

#endif // ENABLE(CSS_IMAGE_SET)
