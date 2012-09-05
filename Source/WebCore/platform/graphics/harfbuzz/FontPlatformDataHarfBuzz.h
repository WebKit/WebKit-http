/*
 * Copyright (c) 2006, 2007, 2008, Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef FontPlatformDataHarfBuzz_h
#define FontPlatformDataHarfBuzz_h

#include "FontOrientation.h"
#include "FontRenderStyle.h"
#include "SkPaint.h"
#include "TextOrientation.h"
#include <wtf/Forward.h>
#include <wtf/RefPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringImpl.h>

class SkTypeface;
typedef uint32_t SkFontID;

namespace WebCore {

class FontDescription;

#if USE(HARFBUZZ_NG)
class HarfBuzzNGFace;
#else
class HarfbuzzFace;
#endif

// -----------------------------------------------------------------------------
// FontPlatformData is the handle which WebKit has on a specific face. A face
// is the tuple of (font, size, ...etc). Here we are just wrapping a Skia
// SkTypeface pointer and dealing with the reference counting etc.
// -----------------------------------------------------------------------------
class FontPlatformData {
public:
    // Used for deleted values in the font cache's hash tables. The hash table
    // will create us with this structure, and it will compare other values
    // to this "Deleted" one. It expects the Deleted one to be differentiable
    // from the 0 one (created with the empty constructor), so we can't just
    // set everything to 0.
    FontPlatformData(WTF::HashTableDeletedValueType);
    FontPlatformData();
    FontPlatformData(float textSize, bool fakeBold, bool fakeItalic);
    FontPlatformData(const FontPlatformData&);
    FontPlatformData(SkTypeface*, const char* name, float textSize, bool fakeBold, bool fakeItalic, FontOrientation = Horizontal, TextOrientation = TextOrientationVerticalRight);
    FontPlatformData(const FontPlatformData& src, float textSize);
    ~FontPlatformData();

    // -------------------------------------------------------------------------
    // Return true iff this font is monospaced (i.e. every glyph has an equal x
    // advance)
    // -------------------------------------------------------------------------
    bool isFixedPitch() const;

    // -------------------------------------------------------------------------
    // Setup a Skia painting context to use this font.
    // -------------------------------------------------------------------------
    void setupPaint(SkPaint*) const;

    // -------------------------------------------------------------------------
    // Return Skia's unique id for this font. This encodes both the style and
    // the font's file name so refers to a single face.
    // -------------------------------------------------------------------------
    SkFontID uniqueID() const;

    unsigned hash() const;
    float size() const { return m_textSize; }
    int emSizeInFontUnits() const;

    FontOrientation orientation() const { return m_orientation; }
    void setOrientation(FontOrientation orientation) { m_orientation = orientation; }
    void setFakeBold(bool fakeBold) { m_fakeBold = fakeBold; }
    void setFakeItalic(bool fakeItalic) { m_fakeItalic = fakeItalic; }
    bool operator==(const FontPlatformData&) const;
    FontPlatformData& operator=(const FontPlatformData&);
    bool isHashTableDeletedValue() const { return m_typeface == hashTableDeletedFontValue(); }

#ifndef NDEBUG
    String description() const;
#endif

#if USE(HARFBUZZ_NG)
    HarfBuzzNGFace* harfbuzzFace() const;
#else
    HarfbuzzFace* harfbuzzFace() const;
#endif

    // The returned styles are all actual styles without FontRenderStyle::NoPreference.
    const FontRenderStyle& fontRenderStyle() const { return m_style; }

    // -------------------------------------------------------------------------
    // Global font preferences...

    static void setHinting(SkPaint::Hinting);
    static void setAutoHint(bool);
    static void setUseBitmaps(bool);
    static void setAntiAlias(bool);
    static void setSubpixelRendering(bool);
    static void setSubpixelPositioning(bool);

private:
    void getRenderStyleForStrike(const char*, int);
    void querySystemForRenderStyle();

    // FIXME: Could SkAutoUnref be used here?
    SkTypeface* m_typeface;
    CString m_family;
    float m_textSize;
    mutable int m_emSizeInFontUnits;
    bool m_fakeBold;
    bool m_fakeItalic;
    FontOrientation m_orientation;
    TextOrientation m_textOrientation;
    FontRenderStyle m_style;
#if USE(HARFBUZZ_NG)
    mutable RefPtr<HarfBuzzNGFace> m_harfbuzzFace;
#else
    mutable RefPtr<HarfbuzzFace> m_harfbuzzFace;
#endif

    SkTypeface* hashTableDeletedFontValue() const { return reinterpret_cast<SkTypeface*>(-1); }
};

} // namespace WebCore

#endif // ifdef FontPlatformDataHarfBuzz_h
