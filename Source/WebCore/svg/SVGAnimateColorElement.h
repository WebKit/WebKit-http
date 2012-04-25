/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Eric Seidel <eric@webkit.org>
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

#ifndef SVGAnimateColorElement_h
#define SVGAnimateColorElement_h

#if ENABLE(SVG)
#include "SVGAnimateElement.h"

namespace WebCore {

class SVGAnimateColorElement : public SVGAnimateElement {
public:
    static PassRefPtr<SVGAnimateColorElement> create(const QualifiedName&, Document*);

private:
    SVGAnimateColorElement(const QualifiedName&, Document*);
    virtual void determinePropertyValueTypes(const String& from, const String& to);
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif // SVGAnimateColorElement_h
