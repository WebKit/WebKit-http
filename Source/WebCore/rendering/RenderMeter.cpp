/*
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#include "RenderMeter.h"

#if ENABLE(METER_ELEMENT)
#include "HTMLMeterElement.h"
#include "HTMLNames.h"
#include "RenderTheme.h"

using namespace std;

namespace WebCore {

using namespace HTMLNames;

RenderMeter::RenderMeter(HTMLElement* element)
    : RenderBlock(element)
{
}

RenderMeter::~RenderMeter()
{
}

HTMLMeterElement* RenderMeter::meterElement() const
{
    ASSERT(node());

    if (isHTMLMeterElement(node()))
        return toHTMLMeterElement(node());

    ASSERT(node()->shadowHost());
    return toHTMLMeterElement(node()->shadowHost());
}

void RenderMeter::updateLogicalWidth()
{
    RenderBox::updateLogicalWidth();
    setWidth(theme()->meterSizeForBounds(this, pixelSnappedIntRect(frameRect())).width());
}

void RenderMeter::updateLogicalHeight()
{
    RenderBox::updateLogicalHeight();
    setHeight(theme()->meterSizeForBounds(this, pixelSnappedIntRect(frameRect())).height());
}

double RenderMeter::valueRatio() const
{
    return meterElement()->valueRatio();
}

void RenderMeter::updateFromElement()
{
    repaint();
}

} // namespace WebCore

#endif
