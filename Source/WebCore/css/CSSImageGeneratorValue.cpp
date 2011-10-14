/*
 * Copyright (C) 2008 Apple Inc.  All rights reserved.
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
#include "CSSImageGeneratorValue.h"

#include "Image.h"
#include "IntSize.h"
#include "IntSizeHash.h"
#include "PlatformString.h"
#include "RenderObject.h"
#include "StyleGeneratedImage.h"

namespace WebCore {

CSSImageGeneratorValue::CSSImageGeneratorValue()
: m_accessedImage(false)
{
}

CSSImageGeneratorValue::~CSSImageGeneratorValue()
{
}

void CSSImageGeneratorValue::addClient(RenderObject* renderer, const IntSize& size)
{
    ref();
    m_imageCache.addClient(renderer, size);
}

void CSSImageGeneratorValue::removeClient(RenderObject* renderer)
{
    m_imageCache.removeClient(renderer);
    deref();
}

Image* CSSImageGeneratorValue::getImage(RenderObject* renderer, const IntSize& size)
{
    // If renderer is the only client, make sure we don't delete this, if the size changes (as this will result in addClient/removeClient calls).
    RefPtr<CSSImageGeneratorValue> protect(this);
    return m_imageCache.getImage(renderer, size);
}

void CSSImageGeneratorValue::putImage(const IntSize& size, PassRefPtr<Image> image)
{
    m_imageCache.putImage(size, image);
}

StyleGeneratedImage* CSSImageGeneratorValue::generatedImage()
{
    if (!m_accessedImage) {
        m_accessedImage = true;
        m_image = StyleGeneratedImage::create(this, isFixedSize());
    }
    return m_image.get();
}

} // namespace WebCore
