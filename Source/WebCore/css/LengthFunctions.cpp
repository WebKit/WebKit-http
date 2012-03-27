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

#include "config.h"
#include "LengthFunctions.h"

#include "Length.h"
#include "RenderView.h"

namespace WebCore {

int minimumValueForLength(Length length, int maximumValue, RenderView* renderView, bool roundPercentages)
{
    switch (length.type()) {
    case Fixed:
        return length.value();
    case Percent:
        if (roundPercentages)
            return static_cast<int>(round(maximumValue * length.percent() / 100.0f));
        // Don't remove the extra cast to float. It is needed for rounding on 32-bit Intel machines that use the FPU stack.
        return static_cast<int>(static_cast<float>(maximumValue * length.percent() / 100.0f));
    case Calculated:
        return length.nonNanCalculatedValue(maximumValue);
    case ViewportRelativeWidth:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().width() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeHeight:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().height() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeMin:
        if (renderView) {
            IntSize viewportSize = renderView->viewportSize();
            return static_cast<int>(std::min(viewportSize.width(), viewportSize.height()) * length.viewportRelativeLength() / 100.0f);
        }
        return 0;
    case Auto:
        return 0;
    case Relative:
    case Intrinsic:
    case MinIntrinsic:
    case Undefined:
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

int valueForLength(Length length, int maximumValue, RenderView* renderView, bool roundPercentages)
{
    switch (length.type()) {
    case Fixed:
    case Percent:
    case Calculated:
    case ViewportRelativeWidth:
    case ViewportRelativeHeight:
    case ViewportRelativeMin:
        return minimumValueForLength(length, maximumValue, renderView, roundPercentages);
    case Auto:
        return maximumValue;
    case Relative:
    case Intrinsic:
    case MinIntrinsic:
    case Undefined:
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

// FIXME: when subpixel layout is supported this copy of floatValueForLength() can be removed. See bug 71143.
float floatValueForLength(Length length, int maximumValue, RenderView* renderView)
{
    switch (length.type()) {
    case Fixed:
        return length.getFloatValue();
    case Percent:
        return static_cast<float>(maximumValue * length.percent() / 100.0f);
    case Auto:
        return static_cast<float>(maximumValue);
    case Calculated:
        return length.nonNanCalculatedValue(maximumValue);                
    case ViewportRelativeWidth:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().width() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeHeight:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().height() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeMin:
        if (renderView) {
            IntSize viewportSize = renderView->viewportSize();
            return static_cast<int>(std::min(viewportSize.width(), viewportSize.height()) * length.viewportRelativeLength() / 100.0f);
        }
        return 0;
    case Relative:
    case Intrinsic:
    case MinIntrinsic:
    case Undefined:
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

float floatValueForLength(Length length, float maximumValue, RenderView* renderView)
{
    switch (length.type()) {
    case Fixed:
        return length.getFloatValue();
    case Percent:
        return static_cast<float>(maximumValue * length.percent() / 100.0f);
    case Auto:
        return static_cast<float>(maximumValue);
    case Calculated:
        return length.nonNanCalculatedValue(maximumValue);
    case ViewportRelativeWidth:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().width() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeHeight:
        if (renderView)
            return static_cast<int>(renderView->viewportSize().height() * length.viewportRelativeLength() / 100.0f);
        return 0;
    case ViewportRelativeMin:
        if (renderView) {
            IntSize viewportSize = renderView->viewportSize();
            return static_cast<int>(std::min(viewportSize.width(), viewportSize.height()) * length.viewportRelativeLength() / 100.0f);
        }
        return 0;
    case Relative:
    case Intrinsic:
    case MinIntrinsic:
    case Undefined:
        ASSERT_NOT_REACHED();
        return 0;
    }
    ASSERT_NOT_REACHED();
    return 0;
}

} // namespace WebCore
