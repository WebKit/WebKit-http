/*
 * Copyright (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
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
 *
 */

#include "config.h"
#include "ShadowData.h"

using namespace std;

namespace WebCore {

ShadowData::ShadowData(const ShadowData& o)
    : m_x(o.m_x)
    , m_y(o.m_y)
    , m_blur(o.m_blur)
    , m_spread(o.m_spread)
    , m_color(o.m_color)
    , m_style(o.m_style)
    , m_isWebkitBoxShadow(o.m_isWebkitBoxShadow)
    , m_next(o.m_next ? adoptPtr(new ShadowData(*o.m_next)) : nullptr)
{
}

bool ShadowData::operator==(const ShadowData& o) const
{
    if ((m_next && !o.m_next) || (!m_next && o.m_next)
        || (m_next && o.m_next && *m_next != *o.m_next))
        return false;
    
    return m_x == o.m_x
        && m_y == o.m_y
        && m_blur == o.m_blur
        && m_spread == o.m_spread
        && m_style == o.m_style
        && m_color == o.m_color
        && m_isWebkitBoxShadow == o.m_isWebkitBoxShadow;
}

static inline void calculateShadowExtent(const ShadowData* shadow, int additionalOutlineSize, LayoutUnit& shadowLeft, LayoutUnit& shadowRight, LayoutUnit& shadowTop, LayoutUnit& shadowBottom)
{
    do {
        int blurAndSpread = shadow->blur() + shadow->spread() + additionalOutlineSize;
        if (shadow->style() == Normal) {
            shadowLeft = min(shadow->x() - blurAndSpread, shadowLeft);
            shadowRight = max(shadow->x() + blurAndSpread, shadowRight);
            shadowTop = min(shadow->y() - blurAndSpread, shadowTop);
            shadowBottom = max(shadow->y() + blurAndSpread, shadowBottom);
        }

        shadow = shadow->next();
    } while (shadow);
}

void ShadowData::adjustRectForShadow(IntRect& rect, int additionalOutlineSize) const
{
    LayoutUnit shadowLeft = 0;
    LayoutUnit shadowRight = 0;
    LayoutUnit shadowTop = 0;
    LayoutUnit shadowBottom = 0;
    calculateShadowExtent(this, additionalOutlineSize, shadowLeft, shadowRight, shadowTop, shadowBottom);

    rect.move(shadowLeft, shadowTop);
    rect.setWidth(rect.width() - shadowLeft + shadowRight);
    rect.setHeight(rect.height() - shadowTop + shadowBottom);
}

void ShadowData::adjustRectForShadow(FloatRect& rect, int additionalOutlineSize) const
{
    LayoutUnit shadowLeft = 0;
    LayoutUnit shadowRight = 0;
    LayoutUnit shadowTop = 0;
    LayoutUnit shadowBottom = 0;
    calculateShadowExtent(this, additionalOutlineSize, shadowLeft, shadowRight, shadowTop, shadowBottom);

    rect.move(shadowLeft, shadowTop);
    rect.setWidth(rect.width() - shadowLeft + shadowRight);
    rect.setHeight(rect.height() - shadowTop + shadowBottom);
}

} // namespace WebCore
