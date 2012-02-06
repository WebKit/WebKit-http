/*
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#ifndef SVGFEDropShadowElement_h
#define SVGFEDropShadowElement_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FEDropShadow.h"
#include "SVGAnimatedNumber.h"
#include "SVGFilterPrimitiveStandardAttributes.h"

namespace WebCore {
    
class SVGFEDropShadowElement : public SVGFilterPrimitiveStandardAttributes {
public:
    static PassRefPtr<SVGFEDropShadowElement> create(const QualifiedName&, Document*);
    
    void setStdDeviation(float stdDeviationX, float stdDeviationY);
    
private:
    SVGFEDropShadowElement(const QualifiedName&, Document*);
    
    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(Attribute*) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);
    virtual PassRefPtr<FilterEffect> build(SVGFilterBuilder*, Filter*);
    
    static const AtomicString& stdDeviationXIdentifier();
    static const AtomicString& stdDeviationYIdentifier();
    
    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGFEDropShadowElement)
        DECLARE_ANIMATED_STRING(In1, in1)
        DECLARE_ANIMATED_NUMBER(Dx, dx)
        DECLARE_ANIMATED_NUMBER(Dy, dy)
        DECLARE_ANIMATED_NUMBER(StdDeviationX, stdDeviationX)
        DECLARE_ANIMATED_NUMBER(StdDeviationY, stdDeviationY)
    END_DECLARE_ANIMATED_PROPERTIES
};
    
} // namespace WebCore

#endif // ENABLE(SVG)
#endif
