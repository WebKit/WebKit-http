/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef SVGTRefElement_h
#define SVGTRefElement_h

#if ENABLE(SVG)
#include "SVGTextPositioningElement.h"
#include "SVGURIReference.h"

namespace WebCore {

class TargetListener;

class SVGTRefElement : public SVGTextPositioningElement,
                       public SVGURIReference {
public:
    static PassRefPtr<SVGTRefElement> create(const QualifiedName&, Document*);

private:
    friend class TargetListener;

    SVGTRefElement(const QualifiedName&, Document*);
    virtual ~SVGTRefElement();

    void createShadowSubtree();

    bool isSupportedAttribute(const QualifiedName&);
    virtual void parseAttribute(Attribute*) OVERRIDE;
    virtual void svgAttributeChanged(const QualifiedName&);

    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);
    virtual bool childShouldCreateRenderer(Node*) const;
    virtual bool rendererIsNeeded(const NodeRenderingContext&);

    virtual void insertedIntoDocument();
    virtual void removedFromDocument();

    void clearEventListener();

    void updateReferencedText();

    void detachTarget();

    virtual void buildPendingResource();

    BEGIN_DECLARE_ANIMATED_PROPERTIES(SVGTRefElement)
        DECLARE_ANIMATED_STRING(Href, href)
    END_DECLARE_ANIMATED_PROPERTIES

    RefPtr<TargetListener> m_eventListener;
};

} // namespace WebCore

#endif // ENABLE(SVG)
#endif
