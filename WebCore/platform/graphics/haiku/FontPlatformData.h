/*
 * Copyright (C) 2006 Apple Computer, Inc.
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com>
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
 *
 * All rights reserved.
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
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

#ifndef FontPlatformData_h
#define FontPlatformData_h

#include "FontDescription.h"
#include "GlyphBuffer.h"
#include "RefCounted.h"
#include <interface/Font.h>
#include <stdio.h>

namespace WebCore {

class FontPlatformDataPrivate : public Noncopyable {
public:
    FontPlatformDataPrivate();
    FontPlatformDataPrivate(const float size, const bool bold, const bool oblique);
    FontPlatformDataPrivate(const BFont& font);

    void update()
    {
        size = font.Size();
        bold = font.Flags() & B_BOLD_FACE;
        oblique = font.Flags() & B_ITALIC_FACE;
    }

    void deref()
    {
        --refCount;
        if (!refCount)
            delete this;
    }

    unsigned refCount;
    BFont font;
    float size;
    bool bold : 1;
    bool oblique : 1;

private:
    ~FontPlatformDataPrivate();
};

class FontPlatformData : public FastAllocBase {
public:
    FontPlatformData(WTF::HashTableDeletedValueType)
        : m_data(reinterpret_cast<FontPlatformDataPrivate*>(-1))
    {
   	}

//    FontPlatformData()
//        : m_font(0)
//    {
//    }
    FontPlatformData(const BFont& font)
        : m_data(new FontPlatformDataPrivate(font))
    {}

    FontPlatformData(const FontDescription&, const AtomicString& family);
    FontPlatformData(float size, bool bold, bool oblique);
    FontPlatformData(const FontPlatformData&);

    ~FontPlatformData();

    FontPlatformData& operator=(const FontPlatformData&);
    bool operator==(const FontPlatformData&) const;

    const BFont* font() const { return &(m_data->font); }

    bool isFixedPitch()
    {
        ASSERT(!isHashTableDeletedValue());
        if (m_data)
            return m_data->font.Spacing() == B_FIXED_SPACING;
        return false;
    }
    float size() const
    {
        ASSERT(!isHashTableDeletedValue());
        if (m_data)
            return m_data->size;
        return 0;
    }
    bool bold() const
    {
        ASSERT(!isHashTableDeletedValue());
        if (m_data)
            return m_data->bold;
        return false;
    }
    bool oblique() const
    {
        ASSERT(!isHashTableDeletedValue());
        if (m_data)
            return m_data->oblique;
        return false;
    }

    unsigned hash() const;
    bool isHashTableDeletedValue() const;

    String description() const;

private:
    FontPlatformDataPrivate* m_data;
};

} // namespace WebCore

#endif // FontPlatformData_h

