/*
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2008 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef SVGGlyphElement_h
#define SVGGlyphElement_h

#if ENABLE(SVG_FONTS)
#include "SVGElement.h"
#include "SVGGlyph.h"

namespace WebCore {

class SVGFontData;

class SVGGlyphElement FINAL : public SVGElement {
public:
    static PassRefPtr<SVGGlyphElement> create(const QualifiedName&, Document&);

    SVGGlyph buildGlyphIdentifier() const;

    // Helper function used by SVGFont
    static void inheritUnspecifiedAttributes(SVGGlyph&, const SVGFontData*);
    static String querySVGFontLanguage(const SVGElement*);

    // Helper function shared between SVGGlyphElement & SVGMissingGlyphElement
    static SVGGlyph buildGenericGlyphIdentifier(const SVGElement*);

private:
    SVGGlyphElement(const QualifiedName&, Document&);

    // FIXME: svgAttributeChanged missing.
    virtual void parseAttribute(const QualifiedName&, const AtomicString&) OVERRIDE;

    virtual InsertionNotificationRequest insertedInto(ContainerNode&) OVERRIDE;
    virtual void removedFrom(ContainerNode&) OVERRIDE;

    virtual bool rendererIsNeeded(const RenderStyle&) { return false; }

    void invalidateGlyphCache();
};

NODE_TYPE_CASTS(SVGGlyphElement)

} // namespace WebCore

#endif // ENABLE(SVG_FONTS)
#endif
