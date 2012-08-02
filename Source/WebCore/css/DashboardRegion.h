/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008 Apple Inc. All rights reserved.
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

#ifndef DashboardRegion_h
#define DashboardRegion_h

#include "Rect.h"

#if ENABLE(DASHBOARD_SUPPORT) || ENABLE(WIDGET_REGION)

namespace WebCore {

class DashboardRegion : public RectBase, public RefCounted<DashboardRegion> {
public:
    static PassRefPtr<DashboardRegion> create() { return adoptRef(new DashboardRegion); }

    RefPtr<DashboardRegion> m_next;
    String m_label;
    String m_geometryType;
    bool m_isCircle : 1;
    bool m_isRectangle : 1;

#if ENABLE(DASHBOARD_SUPPORT) && ENABLE(WIDGET_REGION)
    // Used to tell different CSS function name when both features are enabled.
    String m_cssFunctionName;
#endif

private:
    DashboardRegion() : m_isCircle(false), m_isRectangle(false) { }
};

} // namespace

#endif

#endif
