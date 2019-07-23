/*
 * Copyright (C) 2006 Friedemann Kleint <fkleint@trolltech.com>
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies)
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

#ifndef ImageDecoderQt_h
#define ImageDecoderQt_h

#include "ImageDecoder.h"
#include "ScalableImageDecoderFrame.h"
#include "ImageSource.h"
#include <QtCore/QBuffer>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtGui/QImageReader>
#include <QtGui/QPixmap>

namespace WebCore {


class ImageDecoderQt final : public ImageDecoder {
    WTF_MAKE_FAST_ALLOCATED;
public:
    ImageDecoderQt(AlphaOption alphaOption, GammaAndColorProfileOption gammaAndColorProfileOption);
    ~ImageDecoderQt();

    static Ref<ImageDecoderQt> create(SharedBuffer&, AlphaOption alphaOption, GammaAndColorProfileOption gammaAndColorProfileOption)
    {
        return adoptRef(*new ImageDecoderQt(alphaOption, gammaAndColorProfileOption));
    }

    String filenameExtension() const final;
    EncodedDataStatus encodedDataStatus() const final;
    bool isSizeAvailable() const final;

    // Always original size, without subsampling.
    IntSize size() const final;
    size_t frameCount() const final;

    RepetitionCount repetitionCount() const final;
    Optional<IntPoint> hotSpot() const final;

    IntSize frameSizeAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default) const final;
    bool frameIsCompleteAtIndex(size_t) const final;
    ImageOrientation frameOrientationAtIndex(size_t) const final;

    Seconds frameDurationAtIndex(size_t) const final;
    bool frameHasAlphaAtIndex(size_t) const final;
    bool frameAllowSubsamplingAtIndex(size_t) const final;
    unsigned frameBytesAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default) const final;

    NativeImagePtr createFrameImageAtIndex(size_t, SubsamplingLevel = SubsamplingLevel::Default, const DecodingOptions& = DecodingOptions(DecodingMode::Synchronous)) final;

    void setData(SharedBuffer& data, bool allDataReceived) final;
    bool isAllDataReceived() const final { return m_isAllDataReceived; }
    void clearFrameBufferCache(size_t clearBeforeFrame) final;

    ScalableImageDecoderFrame* frameBufferAtIndex(size_t index) const;

    size_t bytesDecodedToDetermineProperties() const final;

    bool failed() const { return m_encodedDataStatus == EncodedDataStatus::Error; }

protected:
    bool m_isAllDataReceived { false };
    bool m_premultiplyAlpha;
    bool m_ignoreGammaAndColorProfile;
    mutable bool m_scaled { false };
    RefPtr<SharedBuffer> m_data; // The encoded data.
    mutable Vector<ScalableImageDecoderFrame, 1> m_frameBufferCache;
    mutable EncodedDataStatus m_encodedDataStatus { EncodedDataStatus::TypeAvailable };

    // Sets the "decode failure" flag. For caller convenience (since so
    // many callers want to return false after calling this), returns false
    // to enable easy tailcalling. Subclasses may override this to also
    // clean up any local data.
    bool setFailed() const
    {
        m_encodedDataStatus = EncodedDataStatus::Error;
        return false;
    }

    // Returns whether the size is legal (i.e. not going to result in
    // overflow elsewhere). If not, marks decoding as failed.
    bool setSize(const IntSize& size) const
    {
        if (ImageBackingStore::isOverSize(size))
            return setFailed();
        m_size = size;
        m_encodedDataStatus = EncodedDataStatus::SizeAvailable;
        return true;
    }

    IntSize scaledSize() const;

private:
    ImageDecoderQt(const ImageDecoderQt&);
    ImageDecoderQt &operator=(const ImageDecoderQt&);

    void internalDecodeSize() const;
    void internalReadImage(size_t) const;
    bool internalHandleCurrentImage(size_t) const;
    void forceLoadEverything() const;
    void clearPointers() const;
    void prepareScaleDataIfNecessary() const;

    QByteArray m_format;
    mutable std::unique_ptr<QBuffer> m_buffer;
    mutable std::unique_ptr<QImageReader> m_reader;
    mutable RepetitionCount m_repetitionCount;
    mutable IntSize m_size;
};



}

#endif
