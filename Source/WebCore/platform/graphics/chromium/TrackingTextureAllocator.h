/*
 * Copyright (C) 2011, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TrackingTextureAllocator_h
#define TrackingTextureAllocator_h

#include "TextureAllocator.h"
#include <wtf/HashSet.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {
class WebGraphicsContext3D;
}

namespace WebCore {

class TrackingTextureAllocator : public TextureAllocator {
    WTF_MAKE_NONCOPYABLE(TrackingTextureAllocator);
public:
    static PassOwnPtr<TrackingTextureAllocator> create(WebKit::WebGraphicsContext3D* context, int maxTextureSize)
    {
        return adoptPtr(new TrackingTextureAllocator(context, maxTextureSize));
    }
    virtual ~TrackingTextureAllocator();

    virtual unsigned createTexture(const IntSize&, GC3Denum format) OVERRIDE;
    virtual void deleteTexture(unsigned texture, const IntSize&, GC3Denum format) OVERRIDE;
    virtual void deleteAllTextures() OVERRIDE;

    size_t currentMemoryUseBytes() const { return m_currentMemoryUseBytes; }

    enum TextureUsageHint { Any, FramebufferAttachment };

    void setTextureUsageHint(TextureUsageHint hint) { m_textureUsageHint = hint; }
    void setUseTextureStorageExt(bool useStorageExt) { m_useTextureStorageExt = useStorageExt; }

protected:
    TrackingTextureAllocator(WebKit::WebGraphicsContext3D*, int maxTextureSize);

    WebKit::WebGraphicsContext3D* m_context;
    int m_maxTextureSize;
    size_t m_currentMemoryUseBytes;
    TextureUsageHint m_textureUsageHint;
    bool m_useTextureStorageExt;
    HashSet<unsigned> m_allocatedTextureIds;
};

}

#endif
