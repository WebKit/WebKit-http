/*
 * Copyright (C) 2010, Google Inc. All rights reserved.
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

#ifndef TextureManager_h
#define TextureManager_h

#include "GraphicsContext3D.h"
#include "IntRect.h"
#include "IntSize.h"

#include <wtf/FastAllocBase.h>
#include <wtf/HashMap.h>
#include <wtf/ListHashSet.h>

namespace WebCore {

typedef int TextureToken;

class TextureManager {
    WTF_MAKE_NONCOPYABLE(TextureManager);
public:
    static PassOwnPtr<TextureManager> create(size_t memoryLimitBytes, int maxTextureSize)
    {
        return adoptPtr(new TextureManager(memoryLimitBytes, maxTextureSize));
    }

    void setMemoryLimitBytes(size_t);

    TextureToken getToken();
    void releaseToken(TextureToken);
    bool hasTexture(TextureToken);

    bool requestTexture(TextureToken, IntSize, unsigned textureFormat);

    void protectTexture(TextureToken);
    void unprotectTexture(TextureToken);
    void unprotectAllTextures();
    bool isProtected(TextureToken);

    unsigned allocateTexture(GraphicsContext3D*, TextureToken);
    void deleteEvictedTextures(GraphicsContext3D*);

    void reduceMemoryToLimit(size_t);
    size_t currentMemoryUseBytes() const { return m_memoryUseBytes; }

#ifndef NDEBUG
    void setAssociatedContextDebugOnly(GraphicsContext3D* context) { m_associatedContextDebugOnly = context; }
    GraphicsContext3D* associatedContextDebugOnly() const { return m_associatedContextDebugOnly; }
#endif

private:
    TextureManager(size_t memoryLimitBytes, int maxTextureSize);

    struct TextureInfo {
        IntSize size;
        unsigned format;
        unsigned textureId;
        bool isProtected;
    };

    void addTexture(TextureToken, TextureInfo);
    void removeTexture(TextureToken, TextureInfo);

    typedef HashMap<TextureToken, TextureInfo> TextureMap;
    TextureMap m_textures;
    ListHashSet<TextureToken> m_textureLRUSet;

    size_t m_memoryLimitBytes;
    size_t m_memoryUseBytes;
    int m_maxTextureSize;
    TextureToken m_nextToken;

#ifndef NDEBUG
    GraphicsContext3D* m_associatedContextDebugOnly;
#endif

    Vector<unsigned> m_evictedTextureIds;
};

}

#endif
