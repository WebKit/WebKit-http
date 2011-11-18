/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
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

#ifndef ComplexTextControllerLinux_h
#define ComplexTextControllerLinux_h

#include "HarfbuzzSkia.h"
#include "SkPoint.h"
#include "SkScalar.h"
#include "TextRun.h"

extern "C" {
#include "harfbuzz-shaper.h"
}

#include <unicode/uchar.h>
#include <wtf/OwnArrayPtr.h>
#include <wtf/OwnPtr.h>

namespace WebCore {

class Font;
class FontPlatformData;
class SimpleFontData;

// ComplexTextController walks a TextRun and presents each script run in sequence. A
// TextRun is a sequence of code-points with the same embedding level (i.e. they
// are all left-to-right or right-to-left). A script run is a subsequence where
// all the characters have the same script (e.g. Arabic, Thai etc). Shaping is
// only ever done with script runs since the shapers only know how to deal with
// a single script.
//
// Iteration is always in logical (aka reading) order.  For RTL text that means
// the rightmost part of the text will be first.
//
// Once you have setup the object, call |nextScriptRun| to get the first script
// run. This will return false when the iteration is complete. At any time you
// can call |reset| to start over again.
class ComplexTextController {
public:
    ComplexTextController(const TextRun&, int startingX, int startingY, unsigned wordSpacing, unsigned letterSpacing, unsigned padding, const Font*);
    ~ComplexTextController();

    bool isWordBreak(unsigned);
    int determineWordBreakSpacing(unsigned);
    // setPadding sets a number of pixels to be distributed across the TextRun.
    // WebKit uses this to justify text.
    void setPadding(int);
    void reset(int offset);
    // Advance to the next script run, returning false when the end of the
    // TextRun has been reached.
    bool nextScriptRun();
    float widthOfFullRun();

    // setWordSpacingAdjustment sets a delta (in pixels) which is applied at
    // each word break in the TextRun.
    void setWordSpacingAdjustment(int wordSpacingAdjustment) { m_wordSpacingAdjustment = wordSpacingAdjustment; }

    // setLetterSpacingAdjustment sets an additional number of pixels that is
    // added to the advance after each output cluster. This matches the behaviour
    // of WidthIterator::advance.
    void setLetterSpacingAdjustment(int letterSpacingAdjustment) { m_letterSpacing = letterSpacingAdjustment; }
    int letterSpacing() const { return m_letterSpacing; }

    void setupForRTL();
    bool rtl() const { return m_run.rtl(); }
    const uint16_t* glyphs() const { return m_glyphs16; }

    // Return the start index and length of glyphs for the range [from, to) in the current script run.
    void glyphsForRange(int from, int to, int& fromGlyph, int& glyphLength);

    // Return the length of the array returned by |glyphs|
    const unsigned length() const { return m_item.num_glyphs; }

    // Return the offset for each of the glyphs. Note that this is translated
    // by the current x offset and that the x offset is updated for each script
    // run.
    const SkPoint* positions() const { return m_positions; }

    // return the number of code points in the current script run
    const unsigned numCodePoints() const { return m_item.item.length; }

    // Return the current pixel position of the controller.
    const unsigned offsetX() const { return m_offsetX; }

    const FontPlatformData* fontPlatformDataForScriptRun() { return reinterpret_cast<FontPlatformData*>(m_item.font->userData); }

    int offsetForPosition(int);
    FloatRect selectionRect(const FloatPoint&, int height, int from , int to);

private:
    // Return the width (in px) of the current script run.
    unsigned width() const { return m_pixelWidth; }

    void setupFontForScriptRun();
    void deleteGlyphArrays();
    void createGlyphArrays(int);
    void resetGlyphArrays();
    void shapeGlyphs();
    void setGlyphPositions(bool);

    static void normalizeSpacesAndMirrorChars(const UChar* source, bool rtl, UChar* destination, int length);
    static const TextRun& getNormalizedTextRun(const TextRun& originalRun, OwnPtr<TextRun>& normalizedRun, OwnArrayPtr<UChar>& normalizedBuffer);

    // This matches the logic in RenderBlock::findNextLineBreak
    static bool isCodepointSpace(HB_UChar16 c) { return c == ' ' || c == '\t'; }

    int glyphIndexForXPositionInScriptRun(int targetX) const;

    const Font* const m_font;
    const SimpleFontData* m_currentFontData;
    HB_ShaperItem m_item;
    uint16_t* m_glyphs16; // A vector of 16-bit glyph ids.
    SkPoint* m_positions; // A vector of positions for each glyph.
    ssize_t m_indexOfNextScriptRun; // Indexes the script run in |m_run|.
    int m_offsetX; // Offset in pixels to the start of the next script run.
    int m_startingY; // The Y starting point of the script run.
    unsigned m_pixelWidth; // Width (in px) of the current script run.
    unsigned m_glyphsArrayCapacity; // Current size of all the Harfbuzz arrays.

    OwnPtr<TextRun> m_normalizedRun;
    OwnArrayPtr<UChar> m_normalizedBuffer; // A buffer for normalized run.
    const TextRun& m_run;
    int m_wordSpacingAdjustment; // delta adjustment (pixels) for each word break.
    float m_padding; // pixels to be distributed over the line at word breaks.
    float m_padPerWordBreak; // pixels to be added to each word break.
    float m_padError; // |m_padPerWordBreak| might have a fractional component.
                      // Since we only add a whole number of padding pixels at
                      // each word break we accumulate error. This is the
                      // number of pixels that we are behind so far.
    int m_letterSpacing; // pixels to be added after each glyph.
    String m_smallCapsString; // substring of m_run converted to small caps.
};

} // namespace WebCore

#endif // ComplexTextControllerLinux_h
