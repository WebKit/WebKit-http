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
#include "CSSImageSetValue.h"

#if ENABLE(CSS_IMAGE_SET)

#include "CSSImageValue.h"
#include "CSSPrimitiveValue.h"
#include "CachedResourceLoader.h"
#include "Document.h"
#include "Page.h"
#include "StyleCachedImageSet.h"
#include "StylePendingImage.h"

namespace WebCore {

CSSImageSetValue::CSSImageSetValue()
    : CSSValueList(ImageSetClass, CommaSeparator)
    , m_accessedBestFitImage(false)
{
}

CSSImageSetValue::~CSSImageSetValue()
{
}

void CSSImageSetValue::fillImageSet()
{
    size_t length = this->length();
    size_t i = 0;
    while (i < length) {
        CSSValue* imageValue = item(i);
        ASSERT(imageValue->isImageValue());
        String imageURL = static_cast<CSSImageValue*>(imageValue)->url();

        ++i;
        ASSERT(i < length);
        CSSValue* scaleFactorValue = item(i);
        ASSERT(scaleFactorValue->isPrimitiveValue());
        float scaleFactor = static_cast<CSSPrimitiveValue*>(scaleFactorValue)->getFloatValue();

        ImageWithScale image;
        image.imageURL = imageURL;
        image.scaleFactor = scaleFactor;
        m_imagesInSet.append(image);
        ++i;
    }

    // Sort the images so that they are stored in order from lowest resolution to highest.
    std::sort(m_imagesInSet.begin(), m_imagesInSet.end(), CSSImageSetValue::compareByScaleFactor);
}

CSSImageSetValue::ImageWithScale CSSImageSetValue::bestImageForScaleFactor(float scaleFactor)
{
    ImageWithScale image;
    size_t numberOfImages = m_imagesInSet.size();
    for (size_t i = 0; i < numberOfImages; ++i) {
        image = m_imagesInSet.at(i);
        if (image.scaleFactor >= scaleFactor)
            return image;
    }
    return image;
}

StyleCachedImageSet* CSSImageSetValue::cachedImageSet(CachedResourceLoader* loader)
{
    Document* document = loader->document();
    float deviceScaleFactor = 1;
    if (Page* page = document->page())
        deviceScaleFactor = page->deviceScaleFactor();

    if (!m_imagesInSet.size())
        fillImageSet();

    // FIXME: In the future, we want to take much more than deviceScaleFactor into acount here. 
    // All forms of scale should be included: Page::pageScaleFactor(), Frame::pageZoomFactor(), 
    // and any CSS transforms. https://bugs.webkit.org/show_bug.cgi?id=81698
    return cachedImageSet(loader, bestImageForScaleFactor(deviceScaleFactor));
}

StyleCachedImageSet* CSSImageSetValue::cachedImageSet(CachedResourceLoader* loader, ImageWithScale image)
{
    ASSERT(loader);

    if (!m_accessedBestFitImage) {
        ResourceRequest request(loader->document()->completeURL(image.imageURL));
        if (CachedImage* cachedImage = loader->requestImage(request)) {
            m_imageSet = StyleCachedImageSet::create(cachedImage, image.scaleFactor, this);
            m_accessedBestFitImage = true;
        }
    }

    return (m_imageSet && m_imageSet->isCachedImageSet()) ? static_cast<StyleCachedImageSet*>(m_imageSet.get()) : 0;
}

StyleImage* CSSImageSetValue::cachedOrPendingImageSet()
{
    if (!m_imageSet)
        m_imageSet = StylePendingImage::create(this);

    return m_imageSet.get();
}

String CSSImageSetValue::customCssText() const
{
    return "-webkit-image-set(" + CSSValueList::customCssText() + ")";
}

} // namespace WebCore

#endif // ENABLE(CSS_IMAGE_SET)
