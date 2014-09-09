/*
 * Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
 * Copyright (C) 2004, 2005, 2006, 2013 Apple Inc.  All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef Image_h
#define Image_h

#include "Color.h"
#include "ColorSpace.h"
#include "FloatRect.h"
#include "FloatSize.h"
#include "GraphicsTypes.h"
#include "ImageOrientation.h"
#include "NativeImagePtr.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>
#include <wtf/text/WTFString.h>

#if USE(APPKIT)
OBJC_CLASS NSImage;
#endif

#if USE(CG)
struct CGContext;
#endif

#if PLATFORM(WIN)
typedef struct tagSIZE SIZE;
typedef SIZE* LPSIZE;
typedef struct HBITMAP__ *HBITMAP;
#endif

#if PLATFORM(GTK)
typedef struct _GdkPixbuf GdkPixbuf;
#endif

namespace WebCore {

class AffineTransform;
class FloatPoint;
class FloatSize;
class GraphicsContext;
class SharedBuffer;
struct Length;

// This class gets notified when an image creates or destroys decoded frames and when it advances animation frames.
class ImageObserver;

class Image : public RefCounted<Image> {
    friend class GraphicsContext;
public:
    virtual ~Image();
    
    static PassRefPtr<Image> create(ImageObserver* = 0);
    static PassRefPtr<Image> loadPlatformResource(const char* name);
    static bool supportsType(const String&); 

    virtual bool isSVGImage() const { return false; }
    virtual bool isBitmapImage() const { return false; }
    virtual bool isPDFDocumentImage() const { return false; }
    virtual bool currentFrameKnownToBeOpaque() = 0;

    // Derived classes should override this if they can assure that 
    // the image contains only resources from its own security origin.
    virtual bool hasSingleSecurityOrigin() const { return false; }

    static Image* nullImage();
    bool isNull() const { return size().isEmpty(); }

    virtual void setContainerSize(const FloatSize&) { }
    virtual bool usesContainerSize() const { return false; }
    virtual bool hasRelativeWidth() const { return false; }
    virtual bool hasRelativeHeight() const { return false; }
    virtual void computeIntrinsicDimensions(Length& intrinsicWidth, Length& intrinsicHeight, FloatSize& intrinsicRatio);

    virtual FloatSize size() const = 0;
    FloatRect rect() const { return FloatRect(FloatPoint(), size()); }
    float width() const { return size().width(); }
    float height() const { return size().height(); }
    virtual bool getHotSpot(IntPoint&) const { return false; }

#if PLATFORM(IOS)
    virtual FloatSize originalSize() const { return size(); }
#endif

    bool setData(PassRefPtr<SharedBuffer> data, bool allDataReceived);
    virtual bool dataChanged(bool /*allDataReceived*/) { return false; }
    
    virtual String filenameExtension() const { return String(); } // null string if unknown

    virtual void destroyDecodedData(bool destroyAll = true) = 0;

    SharedBuffer* data() { return m_encodedImageData.get(); }

    // Animation begins whenever someone draws the image, so startAnimation() is not normally called.
    // It will automatically pause once all observers no longer want to render the image anywhere.
    enum CatchUpAnimation { DoNotCatchUp, CatchUp };
    virtual void startAnimation(CatchUpAnimation = CatchUp) { }
    virtual void stopAnimation() {}
    virtual void resetAnimation() {}
    
    // Typically the CachedImage that owns us.
    ImageObserver* imageObserver() const { return m_imageObserver; }
    void setImageObserver(ImageObserver* observer) { m_imageObserver = observer; }

    enum TileRule { StretchTile, RoundTile, SpaceTile, RepeatTile };

    virtual PassNativeImagePtr nativeImageForCurrentFrame() { return 0; }
    virtual ImageOrientation orientationForCurrentFrame() { return ImageOrientation(); }

    // Accessors for native image formats.

#if USE(APPKIT)
    virtual NSImage* getNSImage() { return 0; }
#endif

#if PLATFORM(COCOA)
    virtual CFDataRef getTIFFRepresentation() { return 0; }
#endif

#if USE(CG)
    virtual CGImageRef getCGImageRef() { return 0; }
    virtual CGImageRef getFirstCGImageRefOfSize(const IntSize&) { return 0; }
    virtual RetainPtr<CFArrayRef> getCGImageArray() { return 0; }
    static RetainPtr<CGImageRef> imageWithColorSpace(CGImageRef originalImage, ColorSpace);
#endif

#if PLATFORM(WIN)
    virtual bool getHBITMAP(HBITMAP) { return false; }
    virtual bool getHBITMAPOfSize(HBITMAP, const IntSize*) { return false; }
#endif

#if PLATFORM(GTK)
    virtual GdkPixbuf* getGdkPixbuf() { return 0; }
#endif

#if PLATFORM(EFL)
    virtual Evas_Object* getEvasObject(Evas*) { return 0; }
#endif

    virtual void drawPattern(GraphicsContext*, const FloatRect& srcRect, const AffineTransform& patternTransform,
        const FloatPoint& phase, ColorSpace styleColorSpace, CompositeOperator, const FloatRect& destRect, BlendMode = BlendModeNormal);

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    FloatRect adjustSourceRectForDownSampling(const FloatRect& srcRect, const IntSize& scaledSize) const;
#endif

#if !ASSERT_DISABLED
    virtual bool notSolidColor() { return true; }
#endif

    FloatSize spaceSize() const { return m_space; }
    void setSpaceSize(const FloatSize& space)
    {
        m_space = space;
    }
protected:
    Image(ImageObserver* = 0);

    static void fillWithSolidColor(GraphicsContext*, const FloatRect& dstRect, const Color&, ColorSpace styleColorSpace, CompositeOperator);

    // The ColorSpace parameter will only be used for untagged images.
#if PLATFORM(WIN)
    virtual void drawFrameMatchingSourceSize(GraphicsContext*, const FloatRect& dstRect, const IntSize& srcSize, ColorSpace styleColorSpace, CompositeOperator) { }
#endif
    virtual void draw(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, ColorSpace styleColorSpace, CompositeOperator, BlendMode, ImageOrientationDescription);
    void drawTiled(GraphicsContext*, const FloatRect& dstRect, const FloatPoint& srcPoint, const FloatSize& tileSize, ColorSpace styleColorSpace,
        CompositeOperator , BlendMode);
    void drawTiled(GraphicsContext*, const FloatRect& dstRect, const FloatRect& srcRect, const FloatSize& tileScaleFactor, TileRule hRule, TileRule vRule, ColorSpace styleColorSpace, CompositeOperator);

    // Supporting tiled drawing
    virtual bool mayFillWithSolidColor() { return false; }
    virtual Color solidColor() const { return Color(); }
    
private:
    RefPtr<SharedBuffer> m_encodedImageData;
    ImageObserver* m_imageObserver;
    FloatSize m_space;
};

#define IMAGE_TYPE_CASTS(ToClassName) \
    TYPE_CASTS_BASE(ToClassName, Image, image, image->is##ToClassName(), image.is##ToClassName())

}

#endif
