/*
    Copyright (C) 1999 Lars Knoll (knoll@kde.org)
    Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
    Copyright (C) 2011 Rik Cabanier (cabanier@adobe.com)
    Copyright (C) 2011 Adobe Systems Incorporated. All rights reserved.
    Copyright (C) 2012 Motorola Mobility, Inc. All rights reserved.

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

#ifndef LengthFunctions_h
#define LengthFunctions_h

namespace WebCore {

class FloatSize;
class LayoutUnit;
class RenderView;
struct Length;
struct LengthSize;

int minimumIntValueForLength(const Length&, LayoutUnit maximumValue, bool roundPercentages = false);
int intValueForLength(const Length&, LayoutUnit maximumValue);
LayoutUnit minimumValueForLength(const Length&, LayoutUnit maximumValue, bool roundPercentages = false);
LayoutUnit valueForLength(const Length&, LayoutUnit maximumValue);
float floatValueForLength(const Length&, LayoutUnit maximumValue);
float floatValueForLength(const Length&, float maximumValue);
FloatSize floatSizeForLengthSize(const LengthSize&, const FloatSize&);

} // namespace WebCore

#endif // LengthFunctions_h
