/*
 * Copyright (C) 2009 Dirk Schulze <krit@webkit.org>
 * Copyright (C) 2013 Google Inc. All rights reserved.
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

#ifndef SVGFilter_h
#define SVGFilter_h

#if ENABLE(FILTERS)
#include "AffineTransform.h"
#include "Filter.h"
#include "FilterEffect.h"
#include "FloatRect.h"
#include "FloatSize.h"

#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

class SVGFilter : public Filter {
public:
    static PassRefPtr<SVGFilter> create(const AffineTransform&, const FloatRect&, const FloatRect&, const FloatRect&, bool);

    FloatRect filterRegionInUserSpace() const { return m_filterRegion; }
    virtual FloatRect filterRegion() const override { return m_absoluteFilterRegion; }

    virtual float applyHorizontalScale(float value) const override;
    virtual float applyVerticalScale(float value) const override;

    virtual FloatRect sourceImageRect() const override { return m_absoluteSourceDrawingRegion; }
    FloatRect targetBoundingBox() const { return m_targetBoundingBox; }

    virtual bool isSVGFilter() const override final { return true; }

private:
    SVGFilter(const AffineTransform& absoluteTransform, const FloatRect& absoluteSourceDrawingRegion, const FloatRect& targetBoundingBox, const FloatRect& filterRegion, bool effectBBoxMode);

    FloatRect m_absoluteSourceDrawingRegion;
    FloatRect m_targetBoundingBox;
    FloatRect m_absoluteFilterRegion;
    FloatRect m_filterRegion;
    bool m_effectBBoxMode;
};

FILTER_TYPE_CASTS(SVGFilter, isSVGFilter())

} // namespace WebCore

#endif // ENABLE(FILTERS)

#endif // SVGFilter_h
