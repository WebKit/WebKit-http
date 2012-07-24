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
#include "RenderObject.h"
#include "StyleResolver.h"

namespace WebCore {
    
StyleGeneratedImage::StyleGeneratedImage(PassRefPtr<CSSImageGeneratorValue> value)
    : m_imageGeneratorValue(value)  
    , m_fixedSize(m_imageGeneratorValue->isFixedSize())
{
    m_isGeneratedImage = true;
}

PassRefPtr<CSSValue> StyleGeneratedImage::cssValue() const
{
    return m_imageGeneratorValue;
}

IntSize StyleGeneratedImage::imageSize(const RenderObject* renderer, float multiplier) const
{
    if (m_fixedSize) {
        IntSize fixedSize = m_imageGeneratorValue->fixedSize(renderer);
        if (multiplier == 1.0f)
            return fixedSize;

        int width = fixedSize.width() * multiplier;
        int height = fixedSize.height() * multiplier;

        // Don't let images that have a width/height >= 1 shrink below 1 when zoomed.
        if (fixedSize.width() > 0)
            width = max(1, width);

        if (fixedSize.height() > 0)
            height = max(1, height);

        return IntSize(width, height);
    }
    
    return m_containerSize;
}

void StyleGeneratedImage::computeIntrinsicDimensions(const RenderObject* renderer, Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio)
{
    IntSize size = imageSize(renderer, 1);
    intrinsicWidth = Length(size.width(), Fixed);
    intrinsicHeight = Length(size.height(), Fixed);
    intrinsicRatio = size;
}

void StyleGeneratedImage::addClient(RenderObject* renderer)
{
    m_imageGeneratorValue->addClient(renderer, IntSize());
}

void StyleGeneratedImage::removeClient(RenderObject* renderer)
{
    m_imageGeneratorValue->removeClient(renderer);
}

PassRefPtr<Image> StyleGeneratedImage::image(RenderObject* renderer, const IntSize& size) const
{
    return m_imageGeneratorValue->image(renderer, size);
}

}
