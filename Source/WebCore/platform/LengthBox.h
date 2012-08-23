/*
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
    Copyright (c) 2012, Google Inc. All rights reserved.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef LengthBox_h
#define LengthBox_h

#include "Length.h"
#include "TextDirection.h"
#include "WritingMode.h"

namespace WebCore {

class RenderStyle;

struct LengthBox {
    LengthBox()
    {
    }

    LengthBox(LengthType t)
        : m_left(t)
        , m_right(t)
        , m_top(t)
        , m_bottom(t)
    {
    }

    LengthBox(int v)
        : m_left(Length(v, Fixed))
        , m_right(Length(v, Fixed))
        , m_top(Length(v, Fixed))
        , m_bottom(Length(v, Fixed))
    {
    }

    LengthBox(Length t, Length r, Length b, Length l)
        : m_left(l)
        , m_right(r)
        , m_top(t)
        , m_bottom(b)
    {
    }
    
    LengthBox(int t, int r, int b, int l)
        : m_left(Length(l, Fixed))
        , m_right(Length(r, Fixed))
        , m_top(Length(t, Fixed))
        , m_bottom(Length(b, Fixed))
    {
    }

    Length left() const { return m_left; }
    Length right() const { return m_right; }
    Length top() const { return m_top; }
    Length bottom() const { return m_bottom; }

    Length logicalLeft(WritingMode) const;
    Length logicalRight(WritingMode) const;

    Length before(WritingMode) const;
    Length after(WritingMode) const;
    Length start(WritingMode, TextDirection) const;
    Length end(WritingMode, TextDirection) const;

    bool operator==(const LengthBox& o) const
    {
        return m_left == o.m_left && m_right == o.m_right && m_top == o.m_top && m_bottom == o.m_bottom;
    }

    bool operator!=(const LengthBox& o) const
    {
        return !(*this == o);
    }

    bool nonZero() const
    {
        return !(m_left.isZero() && m_right.isZero() && m_top.isZero() && m_bottom.isZero());
    }

    Length m_left;
    Length m_right;
    Length m_top;
    Length m_bottom;
};

} // namespace WebCore

#endif // LengthBox_h
