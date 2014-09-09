/*
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#include "config.h"
#include "ImageSource.h"

#if USE(CG)
#include "ImageSourceCG.h"

#include "ImageOrientation.h"
#include "IntPoint.h"
#include "IntSize.h"
#include "MIMETypeRegistry.h"
#include "SharedBuffer.h"
#if !PLATFORM(IOS)
#include <ApplicationServices/ApplicationServices.h>
#else
#include <CoreGraphics/CGImagePrivate.h>
#include <ImageIO/ImageIO.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>
#endif

#if __has_include(<ImageIO/CGImageSourcePrivate.h>)
#import <ImageIO/CGImageSourcePrivate.h>
#else
const CFStringRef kCGImageSourceSubsampleFactor = CFSTR("kCGImageSourceSubsampleFactor");
#endif

namespace WebCore {

const CFStringRef WebCoreCGImagePropertyAPNGUnclampedDelayTime = CFSTR("UnclampedDelayTime");
const CFStringRef WebCoreCGImagePropertyAPNGDelayTime = CFSTR("DelayTime");
const CFStringRef WebCoreCGImagePropertyAPNGLoopCount = CFSTR("LoopCount");

const CFStringRef kCGImageSourceShouldPreferRGB32 = CFSTR("kCGImageSourceShouldPreferRGB32");
const CFStringRef kCGImageSourceSkipMetadata = CFSTR("kCGImageSourceSkipMetadata");

#if !PLATFORM(COCOA)
size_t sharedBufferGetBytesAtPosition(void* info, void* buffer, off_t position, size_t count)
{
    SharedBuffer* sharedBuffer = static_cast<SharedBuffer*>(info);
    size_t sourceSize = sharedBuffer->size();
    if (position >= sourceSize)
        return 0;

    const char* source = sharedBuffer->data() + position;
    size_t amount = std::min<size_t>(count, sourceSize - position);
    memcpy(buffer, source, amount);
    return amount;
}

void sharedBufferRelease(void* info)
{
    SharedBuffer* sharedBuffer = static_cast<SharedBuffer*>(info);
    sharedBuffer->deref();
}
#endif

ImageSource::ImageSource(ImageSource::AlphaOption, ImageSource::GammaAndColorProfileOption)
    : m_decoder(0)
{
    // FIXME: AlphaOption and GammaAndColorProfileOption are ignored.
}

ImageSource::~ImageSource()
{
    clear(true);
}

void ImageSource::clear(bool destroyAllFrames, size_t, SharedBuffer* data, bool allDataReceived)
{
    // Recent versions of ImageIO discard previously decoded image frames if the client
    // application no longer holds references to them, so there's no need to throw away
    // the decoder unless we're explicitly asked to destroy all of the frames.
    if (!destroyAllFrames)
        return;

    if (m_decoder) {
        CFRelease(m_decoder);
        m_decoder = 0;
    }
    if (data)
        setData(data, allDataReceived);
}

static CFDictionaryRef createImageSourceOptions(ImageSource::ShouldSkipMetadata skipMetaData, SubsamplingLevel subsamplingLevel)
{
    const CFBooleanRef imageSourceSkipMetadata = (skipMetaData == ImageSource::SkipMetadata) ? kCFBooleanTrue : kCFBooleanFalse;
    
    if (!subsamplingLevel) {
        const unsigned numOptions = 3;
        const void* keys[numOptions] = { kCGImageSourceShouldCache, kCGImageSourceShouldPreferRGB32, kCGImageSourceSkipMetadata };
        const void* values[numOptions] = { kCFBooleanTrue, kCFBooleanTrue, imageSourceSkipMetadata };
        return CFDictionaryCreate(nullptr, keys, values, numOptions, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
    }

    short constrainedSubsamplingLevel = std::min<short>(3, std::max<short>(0, subsamplingLevel));
    int subsampleInt = 1 << constrainedSubsamplingLevel; // [0..3] => [1, 2, 4, 8]

    RetainPtr<CFNumberRef> subsampleNumber = adoptCF(CFNumberCreate(nullptr,  kCFNumberIntType,  &subsampleInt));
    const CFIndex numOptions = 4;
    const void* keys[numOptions] = { kCGImageSourceShouldCache, kCGImageSourceShouldPreferRGB32, kCGImageSourceSkipMetadata, kCGImageSourceSubsampleFactor };
    const void* values[numOptions] = { kCFBooleanTrue, kCFBooleanTrue, imageSourceSkipMetadata, subsampleNumber.get() };
    return CFDictionaryCreate(nullptr, keys, values, numOptions, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}

static CFDictionaryRef imageSourceOptions(ImageSource::ShouldSkipMetadata skipMetadata = ImageSource::SkipMetadata, SubsamplingLevel subsamplingLevel = 0)
{
    if (subsamplingLevel)
        return createImageSourceOptions(skipMetadata, subsamplingLevel);

    static CFDictionaryRef options = createImageSourceOptions(skipMetadata, 0);
    return options;
}

bool ImageSource::initialized() const
{
    return m_decoder;
}

void ImageSource::setData(SharedBuffer* data, bool allDataReceived)
{
#if PLATFORM(COCOA)
    if (!m_decoder)
        m_decoder = CGImageSourceCreateIncremental(0);
    // On Mac the NSData inside the SharedBuffer can be secretly appended to without the SharedBuffer's knowledge.  We use SharedBuffer's ability
    // to wrap itself inside CFData to get around this, ensuring that ImageIO is really looking at the SharedBuffer.
    CGImageSourceUpdateData(m_decoder, data->createCFData().get(), allDataReceived);
#else
    if (!m_decoder)
        m_decoder = CGImageSourceCreateIncremental(0);
    // Create a CGDataProvider to wrap the SharedBuffer.
    data->ref();
    // We use the GetBytesAtPosition callback rather than the GetBytePointer one because SharedBuffer
    // does not provide a way to lock down the byte pointer and guarantee that it won't move, which
    // is a requirement for using the GetBytePointer callback.
    CGDataProviderDirectCallbacks providerCallbacks = { 0, 0, 0, sharedBufferGetBytesAtPosition, sharedBufferRelease };
    RetainPtr<CGDataProviderRef> dataProvider = adoptCF(CGDataProviderCreateDirect(data, data->size(), &providerCallbacks));
    CGImageSourceUpdateDataProvider(m_decoder, dataProvider.get(), allDataReceived);
#endif
}

String ImageSource::filenameExtension() const
{
    if (!m_decoder)
        return String();
    CFStringRef imageSourceType = CGImageSourceGetType(m_decoder);
    return WebCore::preferredExtensionForImageSourceType(imageSourceType);
}

SubsamplingLevel ImageSource::subsamplingLevelForScale(float scale) const
{
    // There are four subsampling levels: 0 = 1x, 1 = 0.5x, 2 = 0.25x, 3 = 0.125x.
    float clampedScale = std::max<float>(0.125, std::min<float>(1, scale));
    int result = ceilf(log2f(1 / clampedScale));
    ASSERT(result >=0 && result <= 3);
    return result;
}

bool ImageSource::isSizeAvailable()
{
    bool result = false;
    CGImageSourceStatus imageSourceStatus = CGImageSourceGetStatus(m_decoder);

    // Ragnaros yells: TOO SOON! You have awakened me TOO SOON, Executus!
    if (imageSourceStatus >= kCGImageStatusIncomplete) {
        RetainPtr<CFDictionaryRef> image0Properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, 0, imageSourceOptions()));
        if (image0Properties) {
            CFNumberRef widthNumber = (CFNumberRef)CFDictionaryGetValue(image0Properties.get(), kCGImagePropertyPixelWidth);
            CFNumberRef heightNumber = (CFNumberRef)CFDictionaryGetValue(image0Properties.get(), kCGImagePropertyPixelHeight);
            result = widthNumber && heightNumber;
        }
    }
    
    return result;
}

static ImageOrientation orientationFromProperties(CFDictionaryRef imageProperties)
{
    ASSERT(imageProperties);
    CFNumberRef orientationProperty = (CFNumberRef)CFDictionaryGetValue(imageProperties, kCGImagePropertyOrientation);
    if (!orientationProperty)
        return DefaultImageOrientation;

    int exifValue;
    CFNumberGetValue(orientationProperty, kCFNumberIntType, &exifValue);
    return ImageOrientation::fromEXIFValue(exifValue);
}

bool ImageSource::allowSubsamplingOfFrameAtIndex(size_t) const
{
    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, 0, imageSourceOptions()));
    if (!properties)
        return false;

    CFDictionaryRef jfifProperties = static_cast<CFDictionaryRef>(CFDictionaryGetValue(properties.get(), kCGImagePropertyJFIFDictionary));
    if (jfifProperties) {
        CFBooleanRef isProgCFBool = static_cast<CFBooleanRef>(CFDictionaryGetValue(jfifProperties, kCGImagePropertyJFIFIsProgressive));
        if (isProgCFBool) {
            bool isProgressive = CFBooleanGetValue(isProgCFBool);
            // Workaround for <rdar://problem/5184655> - Hang rendering very large progressive JPEG. Decoding progressive
            // images hangs for a very long time right now. Until this is fixed, don't sub-sample progressive images. This
            // will cause them to fail our large image check and they won't be decoded.
            // FIXME: Remove once underlying issue is fixed (<rdar://problem/5191418>)
            return !isProgressive;
        }
    }

    return true;
}

IntSize ImageSource::frameSizeAtIndex(size_t index, SubsamplingLevel subsamplingLevel, ImageOrientationDescription description) const
{
    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, index, imageSourceOptions(SkipMetadata, subsamplingLevel)));

    if (!properties)
        return IntSize();

    int width = 0;
    int height = 0;
    CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPixelWidth);
    if (num)
        CFNumberGetValue(num, kCFNumberIntType, &width);
    num = (CFNumberRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPixelHeight);
    if (num)
        CFNumberGetValue(num, kCFNumberIntType, &height);

    if ((description.respectImageOrientation() == RespectImageOrientation) && orientationFromProperties(properties.get()).usesWidthAsHeight())
        return IntSize(height, width);

    return IntSize(width, height);
}

ImageOrientation ImageSource::orientationAtIndex(size_t index) const
{
    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, index, imageSourceOptions()));
    if (!properties)
        return DefaultImageOrientation;

    return orientationFromProperties(properties.get());
}

IntSize ImageSource::size(ImageOrientationDescription description) const
{
    return frameSizeAtIndex(0, 0, description);
}

bool ImageSource::getHotSpot(IntPoint& hotSpot) const
{
    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, 0, imageSourceOptions()));
    if (!properties)
        return false;

    int x = -1, y = -1;
    CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(properties.get(), CFSTR("hotspotX"));
    if (!num || !CFNumberGetValue(num, kCFNumberIntType, &x))
        return false;

    num = (CFNumberRef)CFDictionaryGetValue(properties.get(), CFSTR("hotspotY"));
    if (!num || !CFNumberGetValue(num, kCFNumberIntType, &y))
        return false;

    if (x < 0 || y < 0)
        return false;

    hotSpot = IntPoint(x, y);
    return true;
}

size_t ImageSource::bytesDecodedToDetermineProperties() const
{
    // Measured by tracing malloc/calloc calls on Mac OS 10.6.6, x86_64.
    // A non-zero value ensures cached images with no decoded frames still enter
    // the live decoded resources list when the CGImageSource decodes image
    // properties, allowing the cache to prune the partially decoded image.
    // This value is likely to be inaccurate on other platforms, but the overall
    // behavior is unchanged.
    return 13088;
}
    
int ImageSource::repetitionCount()
{
    if (!initialized())
        return cAnimationLoopOnce;

    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyProperties(m_decoder, imageSourceOptions()));
    if (!properties)
        return cAnimationLoopOnce;

    CFDictionaryRef gifProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyGIFDictionary);
    if (gifProperties) {
        CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(gifProperties, kCGImagePropertyGIFLoopCount);

        // No property means loop once.
        if (!num)
            return cAnimationLoopOnce;

        int loopCount;
        CFNumberGetValue(num, kCFNumberIntType, &loopCount);

        // A property with value 0 means loop forever.
        return loopCount ? loopCount : cAnimationLoopInfinite;
    }

    CFDictionaryRef pngProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPNGDictionary);
    if (pngProperties) {
        CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(pngProperties, WebCoreCGImagePropertyAPNGLoopCount);
        if (!num)
            return cAnimationLoopOnce;

        int loopCount;
        CFNumberGetValue(num, kCFNumberIntType, &loopCount);
        return loopCount ? loopCount : cAnimationLoopInfinite;
    }

    // Turns out we're not an animated image after all, so we don't animate.
    return cAnimationNone;
}

size_t ImageSource::frameCount() const
{
    return m_decoder ? CGImageSourceGetCount(m_decoder) : 0;
}

CGImageRef ImageSource::createFrameAtIndex(size_t index, SubsamplingLevel subsamplingLevel)
{
    if (!initialized())
        return 0;

    RetainPtr<CGImageRef> image = adoptCF(CGImageSourceCreateImageAtIndex(m_decoder, index, imageSourceOptions(SkipMetadata, subsamplingLevel)));

#if PLATFORM(IOS)
    // <rdar://problem/7371198> - CoreGraphics changed the default caching behaviour in iOS 4.0 to kCGImageCachingTransient
    // which caused a performance regression for us since the images had to be resampled/recreated every time we called
    // CGContextDrawImage. We now tell CG to cache the drawn images. See also <rdar://problem/14366755> -
    // CoreGraphics needs to un-deprecate kCGImageCachingTemporary since it's still not the default.
#if COMPILER(CLANG)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#endif
    CGImageSetCachingFlags(image.get(), kCGImageCachingTemporary);
#if COMPILER(CLANG)
#pragma clang diagnostic pop
#endif
#endif // !PLATFORM(IOS)

    CFStringRef imageUTI = CGImageSourceGetType(m_decoder);
    static const CFStringRef xbmUTI = CFSTR("public.xbitmap-image");
    if (!imageUTI || !CFEqual(imageUTI, xbmUTI))
        return image.leakRef();
    
    // If it is an xbm image, mask out all the white areas to render them transparent.
    const CGFloat maskingColors[6] = {255, 255,  255, 255, 255, 255};
    RetainPtr<CGImageRef> maskedImage = adoptCF(CGImageCreateWithMaskingColors(image.get(), maskingColors));
    if (!maskedImage)
        return image.leakRef();

    return maskedImage.leakRef();
}

bool ImageSource::frameIsCompleteAtIndex(size_t index)
{
    ASSERT(frameCount());

    // CGImageSourceGetStatusAtIndex claims that all frames of a multi-frame image are incomplete
    // when we've not yet received the complete data for an image that is using an incremental data
    // source (<rdar://problem/7679174>). We work around this by special-casing all frames except the
    // last in an image and treating them as complete if they are present and reported as being
    // incomplete. We do this on the assumption that loading new data can only modify the existing last
    // frame or append new frames. The last frame is only treated as being complete if the image source
    // reports it as such. This ensures that it is truly the last frame of the image rather than just
    // the last that we currently have data for.

    CGImageSourceStatus frameStatus = CGImageSourceGetStatusAtIndex(m_decoder, index);
    if (index < frameCount() - 1)
        return frameStatus >= kCGImageStatusIncomplete;

    return frameStatus == kCGImageStatusComplete;
}

float ImageSource::frameDurationAtIndex(size_t index)
{
    if (!initialized())
        return 0;

    float duration = 0;
    RetainPtr<CFDictionaryRef> properties = adoptCF(CGImageSourceCopyPropertiesAtIndex(m_decoder, index, imageSourceOptions(SkipMetadata)));
    if (properties) {
        CFDictionaryRef gifProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyGIFDictionary);
        if (gifProperties) {
            if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(gifProperties, kCGImagePropertyGIFUnclampedDelayTime)) {
                // Use the unclamped frame delay if it exists.
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
            } else if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(gifProperties, kCGImagePropertyGIFDelayTime)) {
                // Fall back to the clamped frame delay if the unclamped frame delay does not exist.
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
            }
        }

        CFDictionaryRef pngProperties = (CFDictionaryRef)CFDictionaryGetValue(properties.get(), kCGImagePropertyPNGDictionary);
        if (pngProperties) {
            if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(pngProperties, WebCoreCGImagePropertyAPNGUnclampedDelayTime))
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
            else if (CFNumberRef num = (CFNumberRef)CFDictionaryGetValue(pngProperties, WebCoreCGImagePropertyAPNGDelayTime))
                CFNumberGetValue(num, kCFNumberFloatType, &duration);
        }
    }

    // Many annoying ads specify a 0 duration to make an image flash as quickly as possible.
    // We follow Firefox's behavior and use a duration of 100 ms for any frames that specify
    // a duration of <= 10 ms. See <rdar://problem/7689300> and <http://webkit.org/b/36082>
    // for more information.
    if (duration < 0.011f)
        return 0.1f;
    return duration;
}

bool ImageSource::frameHasAlphaAtIndex(size_t index)
{
    if (!m_decoder)
        return false; // FIXME: why doesn't this return true?

    if (!frameIsCompleteAtIndex(index))
        return true;

    CFStringRef imageType = CGImageSourceGetType(m_decoder);

    // Return false if there is no image type or the image type is JPEG, because
    // JPEG does not support alpha transparency.
    if (!imageType || CFEqual(imageType, CFSTR("public.jpeg")))
        return false;

    // FIXME: Could return false for other non-transparent image formats.
    // FIXME: Could maybe return false for a GIF Frame if we have enough info in the GIF properties dictionary
    // to determine whether or not a transparent color was defined.
    return true;
}

unsigned ImageSource::frameBytesAtIndex(size_t index, SubsamplingLevel subsamplingLevel) const
{
    IntSize frameSize = frameSizeAtIndex(index, subsamplingLevel, ImageOrientationDescription(RespectImageOrientation));
    return frameSize.width() * frameSize.height() * 4;
}

}

#endif // USE(CG)
