/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
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

#ifndef SVGTextContentElement_h
#define SVGTextContentElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGAnimatedEnumeration.h"
#include "SVGAnimatedLength.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledElement.h"
#include "SVGTests.h"

namespace WebCore {

enum SVGLengthAdjustType {
    SVGLengthAdjustUnknown,
    SVGLengthAdjustSpacing,
    SVGLengthAdjustSpacingAndGlyphs
};

template<>
struct SVGPropertyTraits<SVGLengthAdjustType> {
    static SVGLengthAdjustType highestEnumValue() { return SVGLengthAdjustSpacingAndGlyphs; }

    static String toString(SVGLengthAdjustType type)
    {
        switch (type) {
        case SVGLengthAdjustUnknown:
            return emptyString();
        case SVGLengthAdjustSpacing:
            return "spacing";
        case SVGLengthAdjustSpacingAndGlyphs:
            return "spacingAndGlyphs";
        }

        ASSERT_NOT_REACHED();
        return emptyString();
    }

    static SVGLengthAdjustType fromString(const String& value)
    {
        if (value == "spacingAndGlyphs")
            return SVGLengthAdjustSpacingAndGlyphs;
        if (value == "spacing")
            return SVGLengthAdjustSpacing;
        return SVGLengthAdjustUnknown;
    }
};

class SVGTextContentElement : public SVGStyledElement,
                              public SVGTests,
                              public SVGLangSpace,
                              public SVGExternalResourcesRequired {
public:
    // Forward declare enumerations in the W3C naming scheme, for IDL generation.
    enum {
        LENGTHADJUST_UNKNOWN = SVGLengthAdjustUnknown,
        LENGTHADJUST_SPACING = SVGLengthAdjustSpacing,
        LENGTHADJUST_SPACINGANDGLYPHS = SVGLengthAdjustSpacingAndGlyphs
    };

    unsigned getNumberOfChars();
    float getComputedTextLength();
    float getSubStringLength(unsigned charnum, unsigned nchars, ExceptionCode&);
    FloatPoint getStartPositionOfChar(unsigned charnum, ExceptionCode&);
    FloatPoint getEndPositionOfChar(unsigned charnum, ExceptionCode&);
    FloatRect getExtentOfChar(unsigned charnum, ExceptionCode&);
    float getRotationOfChar(unsigned charnum, ExceptionCode&);
    int getCharNumAtPosition(const FloatPoint&);
    void selectSubString(unsigned charnum, unsigned nchars, ExceptionCode&);

    static SVGTextContentElement* elementFromRenderer(RenderObject*);

    // textLength is not declared using the standard DECLARE_ANIMATED_LENGTH macro
    // as its getter needs special handling (return getComputedTextLength(), instead of m_textLength).
    SVGLength& specifiedTextLength() { return m_specifiedTextLength; }
    PassRefPtr<SVGAnimatedLength> textLengthAnimated();
    static const SVGPropertyInfo* textLengthPropertyInfo();

protected:
    SVGTextContentElement(const QualifiedName&, Document*);

    virtual bool isValid() const { return SVGTests::isValid(); }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(Attribute*) OVERRIDE;
    virtual bool isPresentationAttribute(Attribute*) const OVERRIDE;
    virtual void collectStyleForAttribute(Attribute*, StylePropertySet*) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual bool selfHasRelativeLengths() const;
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);

private:
    virtual bool isTextContent() const { return true; }

    // Custom 'textLength' property
    static void synchronizeTextLength(void* contextElement);
    static PassRefPtr<SVGAnimatedProperty> lookupOrCreateTextLengthWrapper(void* contextElement);
    mutable SVGSynchronizableAnimatedProperty<SVGLength> m_textLength;
    SVGLength m_specifiedTextLength;
  
    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGTextContentElement)
        DECLARE_ANIMATED_ENUMERATION(LengthAdjust, lengthAdjust, SVGLengthAdjustType)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired) 
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() { SVGTests::synchronizeSystemLanguage(this); }
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
