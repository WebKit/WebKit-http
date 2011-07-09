/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006 Rob Buis <buis@kde.org>
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

#ifndef SVGSwitchElement_h
#define SVGSwitchElement_h

#if ENABLE(SVG)
#include "SVGAnimatedBoolean.h"
#include "SVGExternalResourcesRequired.h"
#include "SVGLangSpace.h"
#include "SVGStyledTransformableElement.h"
#include "SVGTests.h"

namespace WebCore {

class SVGSwitchElement : public SVGStyledTransformableElement,
                         public SVGTests,
                         public SVGLangSpace,
                         public SVGExternalResourcesRequired {
public:
    static PassRefPtr<SVGSwitchElement> create(const QualifiedName&, Document*);

private:
    SVGSwitchElement(const QualifiedName&, Document*);
    
    virtual bool isValid() const { return SVGTests::isValid(); }
    virtual bool supportsFocus() const { return true; }

    virtual bool childShouldCreateRenderer(Node*) const;

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGSwitchElement)
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
