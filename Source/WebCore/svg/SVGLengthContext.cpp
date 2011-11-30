/*
 * Copyright (C) 2004, 2005, 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2007 Rob Buis <buis@kde.org>
 * Copyright (C) 2007 Apple Inc. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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

#if ENABLE(SVG)
#include "SVGLengthContext.h"

#include "CSSHelper.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "RenderPart.h"
#include "RenderView.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"

namespace WebCore {

SVGLengthContext::SVGLengthContext(const SVGElement* context)
    : m_context(context)
{
}

SVGLengthContext::SVGLengthContext(const SVGElement* context, const FloatRect& viewport)
    : m_context(context)
    , m_overridenViewport(viewport)
{
}

FloatRect SVGLengthContext::resolveRectangle(const SVGElement* context, SVGUnitTypes::SVGUnitType type, const FloatRect& viewport, const SVGLength& x, const SVGLength& y, const SVGLength& width, const SVGLength& height)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        SVGLengthContext lengthContext(context);
        return FloatRect(x.value(lengthContext), y.value(lengthContext), width.value(lengthContext), height.value(lengthContext));
    }

    SVGLengthContext lengthContext(context, viewport);
    return FloatRect(x.value(lengthContext) + viewport.x(),
                     y.value(lengthContext) + viewport.y(),
                     width.value(lengthContext),
                     height.value(lengthContext));
}

FloatPoint SVGLengthContext::resolvePoint(const SVGElement* context, SVGUnitTypes::SVGUnitType type, const SVGLength& x, const SVGLength& y)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        SVGLengthContext lengthContext(context);
        return FloatPoint(x.value(lengthContext), y.value(lengthContext));
    }

    // FIXME: valueAsPercentage() won't be correct for eg. cm units. They need to be resolved in user space and then be considered in objectBoundingBox space.
    return FloatPoint(x.valueAsPercentage(), y.valueAsPercentage());
}

float SVGLengthContext::resolveLength(const SVGElement* context, SVGUnitTypes::SVGUnitType type, const SVGLength& x)
{
    ASSERT(type != SVGUnitTypes::SVG_UNIT_TYPE_UNKNOWN);
    if (type == SVGUnitTypes::SVG_UNIT_TYPE_USERSPACEONUSE) {
        SVGLengthContext lengthContext(context);
        return x.value(lengthContext);
    }

    // FIXME: valueAsPercentage() won't be correct for eg. cm units. They need to be resolved in user space and then be considered in objectBoundingBox space.
    return x.valueAsPercentage();
}

float SVGLengthContext::convertValueToUserUnits(float value, SVGLengthMode mode, SVGLengthType fromUnit, ExceptionCode& ec) const
{
    // If the SVGLengthContext carries a custom viewport, force resolving against it.
    if (!m_overridenViewport.isEmpty()) {
        // 100% = 100.0 instead of 1.0 for historical reasons, this could eventually be changed
        if (fromUnit == LengthTypePercentage)
            value /= 100;
        return convertValueFromPercentageToUserUnits(value, mode, ec);
    }

    switch (fromUnit) {
    case LengthTypeUnknown:
        ec = NOT_SUPPORTED_ERR;
        return 0;
    case LengthTypeNumber:
        return value;
    case LengthTypePX:
        return value;
    case LengthTypePercentage:
        return convertValueFromPercentageToUserUnits(value / 100, mode, ec);
    case LengthTypeEMS:
        return convertValueFromEMSToUserUnits(value, ec);
    case LengthTypeEXS:
        return convertValueFromEXSToUserUnits(value, ec);
    case LengthTypeCM:
        return value * cssPixelsPerInch / 2.54f;
    case LengthTypeMM:
        return value * cssPixelsPerInch / 25.4f;
    case LengthTypeIN:
        return value * cssPixelsPerInch;
    case LengthTypePT:
        return value * cssPixelsPerInch / 72;
    case LengthTypePC:
        return value * cssPixelsPerInch / 6;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLengthContext::convertValueFromUserUnits(float value, SVGLengthMode mode, SVGLengthType toUnit, ExceptionCode& ec) const
{
    switch (toUnit) {
    case LengthTypeUnknown:
        ec = NOT_SUPPORTED_ERR;
        return 0;
    case LengthTypeNumber:
        return value;
    case LengthTypePercentage:
        return convertValueFromUserUnitsToPercentage(value * 100, mode, ec);
    case LengthTypeEMS:
        return convertValueFromUserUnitsToEMS(value, ec);
    case LengthTypeEXS:
        return convertValueFromUserUnitsToEXS(value, ec);
    case LengthTypePX:
        return value;
    case LengthTypeCM:
        return value * 2.54f / cssPixelsPerInch;
    case LengthTypeMM:
        return value * 25.4f / cssPixelsPerInch;
    case LengthTypeIN:
        return value / cssPixelsPerInch;
    case LengthTypePT:
        return value * 72 / cssPixelsPerInch;
    case LengthTypePC:
        return value * 6 / cssPixelsPerInch;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLengthContext::convertValueFromUserUnitsToPercentage(float value, SVGLengthMode mode, ExceptionCode& ec) const
{
    float width = 0;
    float height = 0;
    if (!determineViewport(width, height)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (mode) {
    case LengthModeWidth:
        return value / width * 100;
    case LengthModeHeight:
        return value / height * 100;
    case LengthModeOther:
        return value / (sqrtf((width * width + height * height) / 2)) * 100;
    };

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLengthContext::convertValueFromPercentageToUserUnits(float value, SVGLengthMode mode, ExceptionCode& ec) const
{
    float width = 0;
    float height = 0;
    if (!determineViewport(width, height)) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    switch (mode) {
    case LengthModeWidth:
        return value * width;
    case LengthModeHeight:
        return value * height;
    case LengthModeOther:
        return value * sqrtf((width * width + height * height) / 2);
    };

    ASSERT_NOT_REACHED();
    return 0;
}

float SVGLengthContext::convertValueFromUserUnitsToEMS(float value, ExceptionCode& ec) const
{
    if (!m_context || !m_context->renderer() || !m_context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = m_context->renderer()->style();
    float fontSize = style->fontSize();
    if (!fontSize) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    return value / fontSize;
}

float SVGLengthContext::convertValueFromEMSToUserUnits(float value, ExceptionCode& ec) const
{
    if (!m_context || !m_context->renderer() || !m_context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = m_context->renderer()->style();
    return value * style->fontSize();
}

float SVGLengthContext::convertValueFromUserUnitsToEXS(float value, ExceptionCode& ec) const
{
    if (!m_context || !m_context->renderer() || !m_context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = m_context->renderer()->style();

    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    float xHeight = ceilf(style->fontMetrics().xHeight());
    if (!xHeight) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    return value / xHeight;
}

float SVGLengthContext::convertValueFromEXSToUserUnits(float value, ExceptionCode& ec) const
{
    if (!m_context || !m_context->renderer() || !m_context->renderer()->style()) {
        ec = NOT_SUPPORTED_ERR;
        return 0;
    }

    RenderStyle* style = m_context->renderer()->style();
    // Use of ceil allows a pixel match to the W3Cs expected output of coords-units-03-b.svg
    // if this causes problems in real world cases maybe it would be best to remove this
    return value * ceilf(style->fontMetrics().xHeight());
}

bool SVGLengthContext::determineViewport(float& width, float& height) const
{
    if (!m_context)
        return false;

    // If an overriden viewport is given, it has precedence.
    if (!m_overridenViewport.isEmpty()) {
        width = m_overridenViewport.width();
        height = m_overridenViewport.height();
        return true;
    }

    // Take size from outermost <svg> element.
    Document* document = m_context->document();
    if (document->documentElement() == m_context) {
        if (m_context->isSVG()) {
            Frame* frame = m_context->document() ? m_context->document()->frame() : 0;
            if (!frame)
                return false;

            // SVGs embedded through <object> resolve percentage values against the owner renderer in the host document.
            if (RenderPart* ownerRenderer = frame->ownerRenderer()) {
                width = ownerRenderer->width();
                height = ownerRenderer->height();
                return true;
            }
        }

        RenderView* view = toRenderView(document->renderer());
        if (!view)
            return false;

        // Always resolve percentages against the unscaled viewport, as agreed across browsers.
        float zoom = view->style()->effectiveZoom();
        width = view->viewWidth();
        height = view->viewHeight();
        if (zoom != 1) {
            width /= zoom;
            height /= zoom;
        }
        return true;
    }

    // Take size from nearest viewport element (common case: inner <svg> elements)
    SVGElement* viewportElement = m_context->viewportElement();
    if (viewportElement && viewportElement->isSVG()) {
        const SVGSVGElement* svg = static_cast<const SVGSVGElement*>(viewportElement);
        FloatRect viewBox = svg->currentViewBoxRect();
        if (viewBox.isEmpty()) {
            SVGLengthContext viewportContext(svg);
            width = svg->width().value(viewportContext);
            height = svg->height().value(viewportContext);
        } else {
            width = viewBox.width();
            height = viewBox.height();
        }

        return true;
    }
    
    // Take size from enclosing non-SVG RenderBox (common case: inline SVG)
    if (!m_context->parentNode() || m_context->parentNode()->isSVGElement())
        return false;

    RenderObject* renderer = m_context->renderer();
    if (!renderer || !renderer->isBox())
        return false;

    RenderBox* box = toRenderBox(renderer);
    width = box->width();
    height = box->height();
    return true;
}

}

#endif
