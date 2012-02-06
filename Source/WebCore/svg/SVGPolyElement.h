/*
 * Copyright (C) 2004, 2005, 2006, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
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

#ifndef SVGPolyElement_h
#define SVGPolyElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGPointList.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

class SVGPolyElement : public SVGStyledTransformableElement
                     , public SVGTests
                     , public SVGLangSpace
                     , public SVGExternalResourcesRequired {
public:
    SVGListPropertyTearOff<SVGPointList>* points();
    SVGListPropertyTearOff<SVGPointList>* animatedPoints();

    SVGPointList& pointList() const { return m_points.value; }

    static const SVGPropertyInfo* pointsPropertyInfo();

protected:
    SVGPolyElement(const QualifiedName&, Document*);

private:
    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual bool supportsFocus() const { return true; }

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(Attribute*) OVERRIDE; 
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual bool supportsMarkers() const { return true; }

    // Custom 'points' property
    static void synchronizePoints(void* contextElement);
    static PassRefPtr<SVGAnimatedProperty> lookupOrCreatePointsWrapper(void* contextElement);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGPolyElement)
        DECLARE_ANIMATED_BOOLEAN(ExternalResourcesRequired, externalResourcesRequired)
    END_DECLARE_ANIMATED_PROPERTIES

    // SVGTests
    virtual void synchronizeRequiredFeatures() { SVGTests::synchronizeRequiredFeatures(this); }
    virtual void synchronizeRequiredExtensions() { SVGTests::synchronizeRequiredExtensions(this); }
    virtual void synchronizeSystemLanguage() { SVGTests::synchronizeSystemLanguage(this); }

protected:
    mutable SVGSynchronizableAnimatedProperty<SVGPointList> m_points;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
