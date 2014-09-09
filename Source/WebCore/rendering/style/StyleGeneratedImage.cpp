/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "StyleGeneratedImage.h"

#include "CSSImageGeneratorValue.h"
#include "RenderElement.h"
#include "StyleResolver.h"

namespace WebCore {
    
StyleGeneratedImage::StyleGeneratedImage(PassRef<CSSImageGeneratorValue> value)
    : m_imageGeneratorValue(WTF::move(value))
    , m_fixedSize(m_imageGeneratorValue->isFixedSize())
{
    m_isGeneratedImage = true;
}

PassRefPtr<CSSValue> StyleGeneratedImage::cssValue() const
{
    return &const_cast<CSSImageGeneratorValue&>(m_imageGeneratorValue.get());
}

FloatSize StyleGeneratedImage::imageSize(const RenderElement* renderer, float multiplier) const
{
    if (m_fixedSize) {
        FloatSize fixedSize = const_cast<CSSImageGeneratorValue&>(m_imageGeneratorValue.get()).fixedSize(renderer);
        if (multiplier == 1.0f)
            return fixedSize;

        float width = fixedSize.width() * multiplier;
        float height = fixedSize.height() * multiplier;

        // Don't let images that have a width/height >= 1 shrink below 1 device pixel when zoomed.
        float deviceScaleFactor = renderer ? renderer->document().deviceScaleFactor() : 1;
        if (fixedSize.width() > 0)
            width = std::max<float>(1 / deviceScaleFactor, width);

        if (fixedSize.height() > 0)
            height = std::max<float>(1 / deviceScaleFactor, height);

        return FloatSize(width, height);
    }
    
    return m_containerSize;
}

void StyleGeneratedImage::computeIntrinsicDimensions(const RenderElement* renderer, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    // At a zoom level of 1 the image is guaranteed to have a device pixel size.
    FloatSize size = flooredForPainting(LayoutSize(imageSize(renderer, 1)), renderer ? renderer->document().deviceScaleFactor() : 1);
    intrinsicWidth = Length(size.width(), Fixed);
    intrinsicHeight = Length(size.height(), Fixed);
    intrinsicRatio = size;
}

void StyleGeneratedImage::addClient(RenderElement* renderer)
{
    m_imageGeneratorValue->addClient(renderer);
}

void StyleGeneratedImage::removeClient(RenderElement* renderer)
{
    m_imageGeneratorValue->removeClient(renderer);
}

PassRefPtr<Image> StyleGeneratedImage::image(RenderElement* renderer, const FloatSize& size) const
{
    return const_cast<CSSImageGeneratorValue&>(m_imageGeneratorValue.get()).image(renderer, size);
}

bool StyleGeneratedImage::knownToBeOpaque(const RenderElement* renderer) const
{
    return m_imageGeneratorValue->knownToBeOpaque(renderer);
}

}
