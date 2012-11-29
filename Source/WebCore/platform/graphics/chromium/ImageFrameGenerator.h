/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef ImageFrameGenerator_h
#define ImageFrameGenerator_h

#include "SkTypes.h"
#include "SkBitmap.h"
#include "SkSize.h"
#include <wtf/PassOwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/ThreadSafeRefCounted.h>
#include <wtf/ThreadingPrimitives.h>

namespace WebCore {

class ImageDecoder;
class ScaledImageFragment;
class SharedBuffer;

class ImageDecoderFactory {
public:
    virtual ~ImageDecoderFactory() { }
    virtual PassOwnPtr<ImageDecoder> create() = 0;
};

class ImageFrameGenerator : public ThreadSafeRefCounted<ImageFrameGenerator> {
public:
    static PassRefPtr<ImageFrameGenerator> create(const SkISize& fullSize, PassRefPtr<SharedBuffer> data, bool allDataReceived)
    {
        return adoptRef(new ImageFrameGenerator(fullSize, data, allDataReceived));
    }

    ImageFrameGenerator(const SkISize& fullSize, PassRefPtr<SharedBuffer>, bool allDataReceived);
    ~ImageFrameGenerator();

    const ScaledImageFragment* decodeAndScale(const SkISize& scaledSize);

    void setData(PassRefPtr<SharedBuffer>, bool allDataReceived);

    void setImageDecoderFactoryForTesting(PassOwnPtr<ImageDecoderFactory> factory) { m_imageDecoderFactory = factory; }

private:
    // These methods are called while m_decodeMutex is locked.
    const ScaledImageFragment* tryToLockCache(const SkISize& scaledSize);
    const ScaledImageFragment* tryToScale(const ScaledImageFragment* fullSizeImage, const SkISize& scaledSize);
    const ScaledImageFragment* tryToDecodeAndScale(const SkISize& scaledSize);

    SkISize m_fullSize;
    RefPtr<SharedBuffer> m_data;
    bool m_allDataReceived;

    OwnPtr<ImageDecoderFactory> m_imageDecoderFactory;

    // Prevents multiple decode operations on the same data.
    Mutex m_decodeMutex;

    // Prevents concurrent access to m_data.
    Mutex m_dataMutex;
};

} // namespace WebCore

#endif
