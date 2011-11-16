/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef CCTextureUpdater_h
#define CCTextureUpdater_h

#include "IntRect.h"
#include <wtf/Vector.h>

namespace WebCore {

class GraphicsContext3D;
class LayerTextureUpdater;
class ManagedTexture;
class TextureAllocator;

class CCTextureUpdater {
public:
    CCTextureUpdater(TextureAllocator*);
    ~CCTextureUpdater();

    void append(ManagedTexture*, LayerTextureUpdater*, const IntRect& sourceRect, const IntRect& destRect);

    bool hasMoreUpdates() const;

    // Update some textures. Returns true if more textures left to process.
    bool update(GraphicsContext3D*, size_t count);

    void clear();

    TextureAllocator* allocator() { return m_allocator; }

private:
    struct UpdateEntry {
        ManagedTexture* m_texture;
        LayerTextureUpdater* m_updater;
        IntRect m_sourceRect;
        IntRect m_destRect;
    };

    TextureAllocator* m_allocator;
    size_t m_entryIndex;
    Vector<UpdateEntry> m_entries;
};

}

#endif // CCTextureUpdater_h
