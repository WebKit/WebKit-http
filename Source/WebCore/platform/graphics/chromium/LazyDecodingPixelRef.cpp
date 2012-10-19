/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LazyDecodingPixelRef.h"

#include "ImageDecoder.h"
#include "ImageDecodingStore.h"
#include "ImageFrameGenerator.h"
#include <wtf/MainThread.h>

namespace WebCore {

LazyDecodingPixelRef::LazyDecodingPixelRef(PassRefPtr<ImageFrameGenerator> frameGenerator, const SkISize& scaledSize, const SkIRect& scaledSubset)
    : SkPixelRef(0)
    , m_frameGenerator(frameGenerator)
    , m_scaledSize(scaledSize)
    , m_scaledSubset(scaledSubset)
{
}

LazyDecodingPixelRef::~LazyDecodingPixelRef()
{
}

bool LazyDecodingPixelRef::isScaled(const SkISize& fullSize) const
{
    return fullSize != m_scaledSize;
}

bool LazyDecodingPixelRef::isClipped() const
{
    return m_scaledSize.width() != m_scaledSubset.width() || m_scaledSize.height() != m_scaledSubset.height();
}

void* LazyDecodingPixelRef::onLockPixels(SkColorTable**)
{
    ASSERT(isMainThread());
    return ImageDecodingStore::instanceOnMainThread()->lockPixels(m_frameGenerator, m_scaledSize, m_scaledSubset);
}

void LazyDecodingPixelRef::onUnlockPixels()
{
    ASSERT(isMainThread());
    ImageDecodingStore::instanceOnMainThread()->unlockPixels();
}

bool LazyDecodingPixelRef::onLockPixelsAreWritable() const
{
    return false;
}

} // namespace WebKit
