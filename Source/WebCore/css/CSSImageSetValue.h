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

#ifndef CSSImageSetValue_h
#define CSSImageSetValue_h

#if ENABLE(CSS_IMAGE_SET)

#include "CSSValueList.h"

namespace WebCore {

class CachedResourceLoader;
class Document;
class StyleCachedImageSet;
class StyleImage;

class CSSImageSetValue : public CSSValueList {
public:

    static PassRefPtr<CSSImageSetValue> create()
    {
        return adoptRef(new CSSImageSetValue());
    }
    ~CSSImageSetValue();

    StyleCachedImageSet* cachedImageSet(CachedResourceLoader*);

    // Returns a StyleCachedImageSet if the best fit image has been cached already, otherwise a StylePendingImage.
    StyleImage* cachedOrPendingImageSet(Document*);

    String customCssText() const;

    bool isPending() const { return !m_accessedBestFitImage; }

    struct ImageWithScale {
        String imageURL;
        float scaleFactor;
        void reportMemoryUsage(MemoryObjectInfo*) const;
    };

    bool hasFailedOrCanceledSubresources() const;

    PassRefPtr<CSSImageSetValue> cloneForCSSOM() const;

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

protected:
    ImageWithScale bestImageForScaleFactor();

private:
    CSSImageSetValue();
    CSSImageSetValue(const CSSImageSetValue& cloneFrom);

    void fillImageSet();
    static inline bool compareByScaleFactor(ImageWithScale first, ImageWithScale second) { return first.scaleFactor < second.scaleFactor; }

    RefPtr<StyleImage> m_imageSet;
    bool m_accessedBestFitImage;

    // This represents the scale factor that we used to find the best fit image. It does not necessarily
    // correspond to the scale factor of the best fit image.
    float m_scaleFactor;

    Vector<ImageWithScale> m_imagesInSet;
};

} // namespace WebCore

#endif // ENABLE(CSS_IMAGE_SET)

#endif // CSSImageSetValue_h
