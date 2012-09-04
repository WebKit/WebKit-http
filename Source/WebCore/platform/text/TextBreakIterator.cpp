/*
 * (C) 1999 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2007-2009 Torch Mobile, Inc.
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

#include "config.h"
#include "TextBreakIterator.h"

namespace WebCore {

unsigned numGraphemeClusters(const String& s)
{
    NonSharedCharacterBreakIterator it(s.characters(), s.length());
    if (!it)
        return s.length();

    unsigned num = 0;
    while (textBreakNext(it) != TextBreakDone)
        ++num;
    return num;
}

unsigned numCharactersInGraphemeClusters(const String& s, unsigned numGraphemeClusters)
{
    NonSharedCharacterBreakIterator it(s.characters(), s.length());
    if (!it)
        return std::min(s.length(), numGraphemeClusters);

    for (unsigned i = 0; i < numGraphemeClusters; ++i) {
        if (textBreakNext(it) == TextBreakDone)
            return s.length();
    }
    return textBreakCurrent(it);
}

} // namespace WebCore
