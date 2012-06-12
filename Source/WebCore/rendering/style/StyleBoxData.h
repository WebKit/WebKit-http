/*
 * Copyright (C) 2000 Lars Knoll (knoll@kde.org)
 *           (C) 2000 Antti Koivisto (koivisto@kde.org)
 *           (C) 2000 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Graham Dennis (graham.dennis@gmail.com)
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

#ifndef StyleBoxData_h
#define StyleBoxData_h

#include "Length.h"
#include "RenderStyleConstants.h"
#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>

namespace WebCore {

class StyleBoxData : public RefCounted<StyleBoxData> {
public:
    static PassRefPtr<StyleBoxData> create() { return adoptRef(new StyleBoxData); }
    PassRefPtr<StyleBoxData> copy() const { return adoptRef(new StyleBoxData(*this)); }

    bool operator==(const StyleBoxData& o) const;
    bool operator!=(const StyleBoxData& o) const
    {
        return !(*this == o);
    }

    Length width() const { return m_width; }
    Length height() const { return m_height; }
    
    Length minWidth() const { return m_minWidth; }
    Length minHeight() const { return m_minHeight; }
    
    Length maxWidth() const { return m_maxWidth; }
    Length maxHeight() const { return m_maxHeight; }
    
    Length verticalAlign() const { return m_verticalAlign; }
    
    int zIndex() const { return m_zIndex; }
    bool hasAutoZIndex() const { return m_hasAutoZIndex; }
    
    EBoxSizing boxSizing() const { return static_cast<EBoxSizing>(m_boxSizing); }
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    EBoxDecorationBreak boxDecorationBreak() const { return static_cast<EBoxDecorationBreak>(m_boxDecorationBreak); }
#endif

private:
    friend class RenderStyle;

    StyleBoxData();
    StyleBoxData(const StyleBoxData&);

    Length m_width;
    Length m_height;

    Length m_minWidth;
    Length m_maxWidth;

    Length m_minHeight;
    Length m_maxHeight;

    Length m_verticalAlign;

    int m_zIndex;
    unsigned m_hasAutoZIndex : 1;
    unsigned m_boxSizing : 1; // EBoxSizing
#if ENABLE(CSS_BOX_DECORATION_BREAK)
    unsigned m_boxDecorationBreak : 1; // EBoxDecorationBreak
#endif
};

} // namespace WebCore

#endif // StyleBoxData_h
