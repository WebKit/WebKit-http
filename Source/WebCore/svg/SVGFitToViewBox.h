/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 Rob Buis <buis@kde.org>
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

#ifndef SVGFitToViewBox_h
#define SVGFitToViewBox_h

#if ENABLE(SVG)
#include "Attribute.h"
#include "FloatRect.h"
#include "QualifiedName.h"
#include "SVGNames.h"
#include "SVGPreserveAspectRatio.h"
#include <wtf/HashSet.h>

namespace WebCore {

class AffineTransform;
class Document;

class SVGFitToViewBox {
public:
    static AffineTransform viewBoxToViewTransform(const FloatRect& viewBoxRect, const SVGPreserveAspectRatio&, float viewWidth, float viewHeight);

    static bool isKnownAttribute(const QualifiedName&);
    static void addSupportedAttributes(HashSet<QualifiedName>&);

    template<class SVGElementTarget>
    static bool parseAttribute(SVGElementTarget* target, const Attribute& attribute)
    {
        ASSERT(target);
        ASSERT(target->document());
        if (attribute.name() == SVGNames::viewBoxAttr) {
            FloatRect viewBox;
            if (!attribute.isNull())
                parseViewBox(target->document(), attribute.value(), viewBox);
            target->setViewBoxBaseValue(viewBox);
            return true;
        }

        if (attribute.name() == SVGNames::preserveAspectRatioAttr) {
            SVGPreserveAspectRatio preserveAspectRatio;
            preserveAspectRatio.parse(attribute.value());
            target->setPreserveAspectRatioBaseValue(preserveAspectRatio);
            return true;
        }

        return false;
    }

    static bool parseViewBox(Document*, const UChar*& start, const UChar* end, FloatRect& viewBox, bool validate = true);

private:
    static bool parseViewBox(Document*, const String&, FloatRect&);
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGFitToViewBox_h
