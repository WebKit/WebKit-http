/*
 * Copyright (C) 2004, 2005, 2006, 2007 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
 * Copyright (C) 2005 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) Research In Motion Limited 2010. All rights reserved.
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

#ifndef RenderSVGResourceFilter_h
#define RenderSVGResourceFilter_h

#if ENABLE(SVG) && ENABLE(FILTERS)
#include "FloatRect.h"
#include "ImageBuffer.h"
#include "RenderSVGResourceContainer.h"
#include "SVGFilter.h"
#include "SVGFilterBuilder.h"
#include "SVGFilterElement.h"
#include "SVGUnitTypes.h"

#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

struct FilterData {
    WTF_MAKE_FAST_ALLOCATED;
    WTF_MAKE_NONCOPYABLE(FilterData);
public:
    enum FilterDataState { PaintingSource, Applying, Built, CycleDetected, MarkedForRemoval };

    FilterData()
        : savedContext(0)
        , state(PaintingSource)
    {
    }

    RefPtr<SVGFilter> filter;
    std::unique_ptr<SVGFilterBuilder> builder;
    OwnPtr<ImageBuffer> sourceGraphicBuffer;
    GraphicsContext* savedContext;
    AffineTransform shearFreeAbsoluteTransform;
    FloatRect boundaries;
    FloatRect drawingRegion;
    FloatSize scale;
    FilterDataState state;
};

class GraphicsContext;

class RenderSVGResourceFilter FINAL : public RenderSVGResourceContainer {
public:
    RenderSVGResourceFilter(SVGFilterElement&, PassRef<RenderStyle>);
    virtual ~RenderSVGResourceFilter();

    SVGFilterElement& filterElement() const { return toSVGFilterElement(RenderSVGResourceContainer::element()); }

    virtual void removeAllClientsFromCache(bool markForInvalidation = true);
    virtual void removeClientFromCache(RenderObject*, bool markForInvalidation = true);

    virtual bool applyResource(RenderElement&, const RenderStyle&, GraphicsContext*&, unsigned short resourceMode) OVERRIDE;
    virtual void postApplyResource(RenderElement&, GraphicsContext*&, unsigned short resourceMode, const Path*, const RenderSVGShape*) OVERRIDE;

    virtual FloatRect resourceBoundingBox(const RenderObject&) OVERRIDE;

    std::unique_ptr<SVGFilterBuilder> buildPrimitives(SVGFilter*);

    SVGUnitTypes::SVGUnitType filterUnits() const { return filterElement().filterUnits(); }
    SVGUnitTypes::SVGUnitType primitiveUnits() const { return filterElement().primitiveUnits(); }

    void primitiveAttributeChanged(RenderObject*, const QualifiedName&);

    virtual RenderSVGResourceType resourceType() const { return s_resourceType; }
    static RenderSVGResourceType s_resourceType;

    FloatRect drawingRegion(RenderObject*) const;
private:
    void element() const WTF_DELETED_FUNCTION;

    virtual const char* renderName() const OVERRIDE { return "RenderSVGResourceFilter"; }
    virtual bool isSVGResourceFilter() const OVERRIDE { return true; }

    bool fitsInMaximumImageSize(const FloatSize&, FloatSize&);

    HashMap<RenderObject*, std::unique_ptr<FilterData>> m_filter;
};

RENDER_OBJECT_TYPE_CASTS(RenderSVGResourceFilter, isSVGResourceFilter())

}

#endif
#endif
