/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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

#include "config.h"
#if ENABLE(SVG_ANIMATION)
#include "SVGSetElement.h"
#include "SVGNames.h"

namespace WebCore {
    
inline SVGSetElement::SVGSetElement(const QualifiedName& tagName, Document* document)
    : SVGAnimateElement(tagName, document)
{
    ASSERT(hasTagName(SVGNames::setTag));
}

PassRefPtr<SVGSetElement> SVGSetElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGSetElement(tagName, document));
}

}

// vim:ts=4:noet
#endif // ENABLE(SVG_ANIMATION)

