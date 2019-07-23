/*
 * Copyright (C) 2006 Friedemann Kleint <fkleint@trolltech.com>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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
#include "ImageDecoderQt.h"
#include "ImageSource.h"
#include "IntSize.h"
#include "NotImplemented.h"

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtGui/QImageReader>

namespace WebCore {

ImageDecoderQt::ImageDecoderQt(AlphaOption alphaOption, GammaAndColorProfileOption gammaAndColorProfileOption)
    : m_premultiplyAlpha(alphaOption == AlphaOption::Premultiplied)
    , m_ignoreGammaAndColorProfile(gammaAndColorProfileOption == GammaAndColorProfileOption::Ignored)
    , m_repetitionCount(RepetitionCountNone)
{
}

ImageDecoderQt::~ImageDecoderQt()
{
}

static const char* s_formatWhiteList[] = {"png", "jpeg", "gif", "webp", "bmp", "svg", "ico", 0};

static bool isFormatWhiteListed(const QByteArray &format)
{
    static QSet<QByteArray> whiteListSet;
    if (whiteListSet.isEmpty()) {
        QByteArray whiteListEnv = qgetenv("QTWEBKIT_IMAGEFORMAT_WHITELIST");
        if (!whiteListEnv.isEmpty())
            whiteListSet = QSet<QByteArray>::fromList(whiteListEnv.split(','));

        const char **formatIt  = s_formatWhiteList;
        while (*formatIt) {
            whiteListSet.insert(QByteArray(*formatIt));
            ++formatIt;
        }
    }
    return whiteListSet.contains(format);
}

void ImageDecoderQt::setData(SharedBuffer& data, bool allDataReceived)
{
    if (failed())
        return;

    // No progressive loading possible
    if (!allDataReceived)
        return;

    // Cache our own new data.
    ImageDecoder::setData(data, allDataReceived);

    // We expect to be only called once with allDataReceived
    ASSERT(!m_buffer);
    ASSERT(!m_reader);

    // Attempt to load the data
    QByteArray imageData = QByteArray::fromRawData(m_data->data(), m_data->size());
    m_buffer = std::make_unique<QBuffer>();
    m_buffer->setData(imageData);
    m_buffer->open(QIODevice::ReadOnly | QIODevice::Unbuffered);
    m_reader = std::make_unique<QImageReader>(m_buffer.get(), m_format);

    // This will force the JPEG decoder to use JDCT_IFAST
    m_reader->setQuality(49);

    // QImageReader only allows retrieving the format before reading the image
    m_format = m_reader->format();
    if (!m_format.isEmpty() && !isFormatWhiteListed(m_format)) {
        qWarning("Image of format '%s' blocked because it is not considered safe. If you are sure it is safe to do so, you can white-list the format by setting the environment variable QTWEBKIT_IMAGEFORMAT_WHITELIST=%s", m_format.constData(), m_format.constData());
        setFailed();
        m_reader = nullptr;
    }
}

bool ImageDecoderQt::isSizeAvailable() const
{
    if (!ImageDecoder::isSizeAvailable() && m_reader)
        internalDecodeSize();

    return ImageDecoder::isSizeAvailable();
}

size_t ImageDecoderQt::frameCount() const
{
    if (m_frameBufferCache.isEmpty() && m_reader) {
        if (m_reader->supportsAnimation()) {
            int imageCount = m_reader->imageCount();

            // Fixup for Qt decoders... imageCount() is wrong
            // and jumpToNextImage does not work either... so
            // we will have to parse everything...
            if (!imageCount)
                forceLoadEverything();
            else {
                m_frameBufferCache.resize(imageCount);
            }
        } else {
            m_frameBufferCache.resize(1);
        }
    }

    return m_frameBufferCache.size();
}

RepetitionCount ImageDecoderQt::repetitionCount() const
{
    if (m_reader && m_reader->supportsAnimation())
        m_repetitionCount = m_reader->loopCount();
    return m_repetitionCount;
}

String ImageDecoderQt::filenameExtension() const
{
    return String(m_format.constData(), m_format.length());
}

void ImageDecoderQt::clearFrameBufferCache(size_t /*index*/)
{
}

ScalableImageDecoderFrame* ImageDecoderQt::frameBufferAtIndex(size_t index) const
{
    // In case the ImageDecoderQt got recreated we don't know
    // yet how many images we are going to have and need to
    // find that out now.
    size_t count = m_frameBufferCache.size();
    if (!failed() && !count) {
        internalDecodeSize();
        count = frameCount();
    }

    if (index >= count)
        return 0;

    ScalableImageDecoderFrame& frame = m_frameBufferCache[index];
    if (!frame.isComplete() && m_reader)
        internalReadImage(index);
    return &frame;
}

void ImageDecoderQt::internalDecodeSize() const
{
    ASSERT(m_reader);

    // If we have a QSize() something failed
    QSize size = m_reader->size();
    if (size.isEmpty()) {
        setFailed();
        return clearPointers();
    }

    setSize(IntSize(size.width(), size.height()));

    // We don't need the tables set by prepareScaleDataIfNecessary,
    // but their dimensions are used by ImageDecoder::scaledSize().
    prepareScaleDataIfNecessary();
    if (m_scaled)
        m_reader->setScaledSize(scaledSize());
}

void ImageDecoderQt::internalReadImage(size_t frameIndex) const
{
    ASSERT(m_reader);

    if (m_reader->supportsAnimation())
        m_reader->jumpToImage(frameIndex);
    else if (frameIndex) {
        setFailed();
        return clearPointers();
    }

    if (!internalHandleCurrentImage(frameIndex))
      setFailed();

    // Attempt to return some memory
    for (size_t i = 0; i < m_frameBufferCache.size(); ++i) {
        if (!m_frameBufferCache[i].isComplete())
            return;
    }

    clearPointers();
}

bool ImageDecoderQt::internalHandleCurrentImage(size_t frameIndex) const
{
    ScalableImageDecoderFrame* const buffer = &m_frameBufferCache[frameIndex];
    QSize imageSize = m_reader->scaledSize();
    if (imageSize.isEmpty())
        imageSize = m_reader->size();

    if (!buffer->initialize(IntSize(imageSize.width(), imageSize.height()), m_premultiplyAlpha))
        return false;

    QImage image(reinterpret_cast<uchar*>(buffer->backingStore()->pixelAt(0, 0)), imageSize.width(), imageSize.height(), sizeof(RGBA32) * imageSize.width(), m_reader->imageFormat());

    buffer->setDuration(Seconds::fromMilliseconds(m_reader->nextImageDelay()));
    m_reader->read(&image);

    // ScalableImageDecoderFrame expects ARGB32.
    if (m_premultiplyAlpha) {
        if (image.format() != QImage::Format_ARGB32_Premultiplied)
            image = image.convertToFormat(QImage::Format_ARGB32_Premultiplied);
    } else {
        if (image.format() != QImage::Format_ARGB32)
            image = image.convertToFormat(QImage::Format_ARGB32);
    }

    if (reinterpret_cast<const uchar*>(image.constBits()) != reinterpret_cast<const uchar*>(buffer->backingStore()->pixelAt(0, 0))) {
        // The in-buffer was replaced during decoding with another, so copy into it manually.
        memcpy(buffer->backingStore()->pixelAt(0, 0), image.constBits(),  image.sizeInBytes());
    }

    if (image.isNull()) {
        frameCount();
        repetitionCount();
        clearPointers();
        return false;
    }

    buffer->backingStore()->setFrameRect(image.rect());
    buffer->setHasAlpha(image.hasAlphaChannel());
    buffer->setDecodingStatus(DecodingStatus::Complete);

    return true;
}

// The QImageIOHandler is not able to tell us how many frames
// we have and we need to parse every image. We do this by
// increasing the m_frameBufferCache by one and try to parse
// the image. We stop when QImage::read fails and then need
// to resize the m_frameBufferCache to the final size and update
// the failed bit. If we failed to decode the first image
// then we truly failed to decode, otherwise we're OK.

// TODO: Do not increment the m_frameBufferCache.size() by one but more than one
void ImageDecoderQt::forceLoadEverything() const
{
    int imageCount = 0;

    do {
        m_frameBufferCache.resize(++imageCount);
    } while (internalHandleCurrentImage(imageCount - 1));

    // If we failed decoding the first image we actually
    // have no images and need to set the failed bit.
    // Otherwise, we want to forget about
    // the last attempt to decode a image.
    m_frameBufferCache.resize(imageCount - 1);
    if (imageCount == 1)
        setFailed();
}

void ImageDecoderQt::clearPointers() const
{
    m_reader = nullptr;
    m_buffer = nullptr;
}

IntSize ImageDecoderQt::frameSizeAtIndex(size_t index, SubsamplingLevel) const
{
    if (!m_reader || (index > 0 && !m_reader->supportsAnimation()))
        return IntSize();

    m_reader->jumpToImage(index);
    QImage image = m_reader->read();

    if (!image.isNull()) {
        return IntSize(image.width(), image.height());
    } else {
        return IntSize();
    }
}

bool ImageDecoderQt::frameIsCompleteAtIndex(size_t index) const
{
    ScalableImageDecoderFrame* buffer = frameBufferAtIndex(index);
    return buffer && buffer->isComplete();
}

ImageOrientation ImageDecoderQt::frameOrientationAtIndex(size_t index) const
{
    ScalableImageDecoderFrame* buffer = frameBufferAtIndex(index);
    return buffer ? buffer->orientation() : ImageOrientation();
}

Seconds ImageDecoderQt::frameDurationAtIndex(size_t index) const
{
    ScalableImageDecoderFrame* buffer = frameBufferAtIndex(index);
    return buffer ? buffer->duration() : 100_ms;
}

bool ImageDecoderQt::frameAllowSubsamplingAtIndex(size_t) const
{
    notImplemented();
    return true;
}

bool ImageDecoderQt::frameHasAlphaAtIndex(size_t) const
{
    notImplemented();
    return true;
}

unsigned ImageDecoderQt::frameBytesAtIndex(size_t index, SubsamplingLevel subsamplingLevel) const
{
    if (!m_reader)
        return 0;

    auto frameSize = frameSizeAtIndex(index, subsamplingLevel);
    return (frameSize.area() * 4).unsafeGet();
}

size_t ImageDecoderQt::bytesDecodedToDetermineProperties() const
{
    // Set to match value used for CoreGraphics.
    return 13088;
}

EncodedDataStatus ImageDecoderQt::encodedDataStatus() const
{
    notImplemented();
    return EncodedDataStatus::Unknown;
}

IntSize ImageDecoderQt::size() const
{
    if (!m_reader)
        return IntSize();

    m_reader->jumpToImage(0);
    QImage image = m_reader->read();
    if (image.isNull())
        return IntSize();

    return IntSize(image.width(), image.height());
}

Optional<IntPoint> ImageDecoderQt::hotSpot() const
{
    return IntPoint();
}

}

// vim: ts=4 sw=4 et
