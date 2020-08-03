/*
 * Copyright (C) 2006, 2009, 2011, 2016 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2008 Torch Mobile Inc.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
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

#pragma once

#include "FloatSize.h"
#include "Glyph.h"
#include <climits>
#include <wtf/Vector.h>

#if USE(CG)
#include <CoreGraphics/CGGeometry.h>
#endif

namespace WebCore {

class Font;

#if USE(WINGDI)
typedef wchar_t GlyphBufferGlyph;
#else
typedef Glyph GlyphBufferGlyph;
#endif

// CG uses CGSize instead of FloatSize so that the result of advances()
// can be passed directly to CGContextShowGlyphsWithAdvances in FontMac.mm
#if USE(CG)
struct GlyphBufferAdvance : CGSize {
public:
    GlyphBufferAdvance() : CGSize(CGSizeZero) { }
    GlyphBufferAdvance(CGSize size)
        : CGSize(size)
    {
    }
    GlyphBufferAdvance(FloatSize size)
        : CGSize(size)
    {
    }
    GlyphBufferAdvance(float width, float height)
        : CGSize(CGSizeMake(width, height))
    {
    }

    template<class Encoder> void encode(Encoder&) const;
    template<class Decoder> static Optional<GlyphBufferAdvance> decode(Decoder&);

    operator FloatSize() { return { static_cast<float>(this->CGSize::width), static_cast<float>(this->CGSize::height) }; }

    void setWidth(CGFloat width) { this->CGSize::width = width; }
    void setHeight(CGFloat height) { this->CGSize::height = height; }
    CGFloat width() const { return this->CGSize::width; }
    CGFloat height() const { return this->CGSize::height; }
};

template<class Encoder>
void GlyphBufferAdvance::encode(Encoder& encoder) const
{
    encoder << width();
    encoder << height();
}

template<class Decoder>
Optional<GlyphBufferAdvance> GlyphBufferAdvance::decode(Decoder& decoder)
{
    Optional<CGFloat> width;
    decoder >> width;
    if (!width)
        return WTF::nullopt;

    Optional<CGFloat> height;
    decoder >> height;
    if (!height)
        return WTF::nullopt;

    return GlyphBufferAdvance(CGSizeMake(*width, *height));
}
#else
typedef FloatSize GlyphBufferAdvance;
#endif

inline FloatSize toFloatSize(const GlyphBufferAdvance& a)
{
    return FloatSize(a.width(), a.height());
}

class GlyphBuffer {
public:
    bool isEmpty() const { return m_fonts.isEmpty(); }
    unsigned size() const { return m_fonts.size(); }
    
    void clear()
    {
        m_fonts.clear();
        m_glyphs.clear();
        m_advances.clear();
        if (m_offsetsInString)
            m_offsetsInString->clear();
    }

    GlyphBufferGlyph* glyphs(unsigned from) { return m_glyphs.data() + from; }
    GlyphBufferAdvance* advances(unsigned from) { return m_advances.data() + from; }
    const GlyphBufferGlyph* glyphs(unsigned from) const { return m_glyphs.data() + from; }
    const GlyphBufferAdvance* advances(unsigned from) const { return m_advances.data() + from; }

    const Font* fontAt(unsigned index) const { return m_fonts[index]; }

    void setInitialAdvance(GlyphBufferAdvance initialAdvance) { m_initialAdvance = initialAdvance; }
    const GlyphBufferAdvance& initialAdvance() const { return m_initialAdvance; }
    
    Glyph glyphAt(unsigned index) const
    {
        return m_glyphs[index];
    }

    GlyphBufferAdvance advanceAt(unsigned index) const
    {
        return m_advances[index];
    }
    
    static const unsigned noOffset = UINT_MAX;
    void add(Glyph glyph, const Font* font, float width, unsigned offsetInString = noOffset)
    {
        GlyphBufferAdvance advance;
        advance.setWidth(width);
        advance.setHeight(0);

        add(glyph, font, advance, offsetInString);
    }

    void add(Glyph glyph, const Font* font, GlyphBufferAdvance advance, unsigned offsetInString)
    {
        m_fonts.append(font);
        m_glyphs.append(glyph);

        m_advances.append(advance);
        
        if (offsetInString != noOffset && m_offsetsInString)
            m_offsetsInString->append(offsetInString);
    }

    void remove(unsigned location, unsigned length)
    {
        m_fonts.remove(location, length);
        m_glyphs.remove(location, length);
        m_advances.remove(location, length);
        if (m_offsetsInString)
            m_offsetsInString->remove(location, length);
    }

    void makeHole(unsigned location, unsigned length, const Font* font)
    {
        ASSERT(location <= size());

        m_fonts.insertVector(location, Vector<const Font*>(length, font));
        m_glyphs.insertVector(location, Vector<GlyphBufferGlyph>(length, 0xFFFF));
        m_advances.insertVector(location, Vector<GlyphBufferAdvance>(length, GlyphBufferAdvance(0, 0)));
        if (m_offsetsInString)
            m_offsetsInString->insertVector(location, Vector<unsigned>(length, 0));
    }

    void reverse(unsigned from, unsigned length)
    {
        for (unsigned i = from, end = from + length - 1; i < end; ++i, --end)
            swap(i, end);
    }

    void expandLastAdvance(float width)
    {
        ASSERT(!isEmpty());
        GlyphBufferAdvance& lastAdvance = m_advances.last();
        lastAdvance.setWidth(lastAdvance.width() + width);
    }

    void expandLastAdvance(GlyphBufferAdvance expansion)
    {
        ASSERT(!isEmpty());
        GlyphBufferAdvance& lastAdvance = m_advances.last();
        lastAdvance.setWidth(lastAdvance.width() + expansion.width());
        lastAdvance.setHeight(lastAdvance.height() + expansion.height());
    }
    
    void saveOffsetsInString()
    {
        m_offsetsInString.reset(new Vector<unsigned, 2048>());
    }

    int offsetInString(unsigned index) const
    {
        ASSERT(m_offsetsInString);
        return (*m_offsetsInString)[index];
    }

    void shrink(unsigned truncationPoint)
    {
        m_fonts.shrink(truncationPoint);
        m_glyphs.shrink(truncationPoint);
        m_advances.shrink(truncationPoint);
        if (m_offsetsInString)
            m_offsetsInString->shrink(truncationPoint);
    }

private:
    void swap(unsigned index1, unsigned index2)
    {
        const Font* f = m_fonts[index1];
        m_fonts[index1] = m_fonts[index2];
        m_fonts[index2] = f;

        GlyphBufferGlyph g = m_glyphs[index1];
        m_glyphs[index1] = m_glyphs[index2];
        m_glyphs[index2] = g;

        GlyphBufferAdvance s = m_advances[index1];
        m_advances[index1] = m_advances[index2];
        m_advances[index2] = s;
    }

    Vector<const Font*, 2048> m_fonts;
    Vector<GlyphBufferGlyph, 2048> m_glyphs;
    Vector<GlyphBufferAdvance, 2048> m_advances;
    GlyphBufferAdvance m_initialAdvance;
    std::unique_ptr<Vector<unsigned, 2048>> m_offsetsInString;
};

}
