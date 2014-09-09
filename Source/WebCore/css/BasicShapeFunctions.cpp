/*
 * Copyright (C) 2012 Adobe Systems Incorporated. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "config.h"
#include "BasicShapeFunctions.h"

#include "BasicShapes.h"
#include "CSSBasicShapes.h"
#include "CSSPrimitiveValueMappings.h"
#include "CSSValuePool.h"
#include "Pair.h"
#include "RenderStyle.h"

namespace WebCore {

static PassRefPtr<CSSPrimitiveValue> valueForCenterCoordinate(CSSValuePool& pool, const RenderStyle* style, const BasicShapeCenterCoordinate& center, EBoxOrient orientation)
{
    if (center.direction() == BasicShapeCenterCoordinate::TopLeft)
        return pool.createValue(center.length(), style);

    CSSValueID keyword = orientation == HORIZONTAL ? CSSValueRight : CSSValueBottom;

    return pool.createValue(Pair::create(pool.createIdentifierValue(keyword), pool.createValue(center.length(), style)));
}

static PassRefPtr<CSSPrimitiveValue> basicShapeRadiusToCSSValue(const RenderStyle* style, CSSValuePool& pool, const BasicShapeRadius& radius)
{
    switch (radius.type()) {
    case BasicShapeRadius::Value:
        return pool.createValue(radius.value(), style);
    case BasicShapeRadius::ClosestSide:
        return pool.createIdentifierValue(CSSValueClosestSide);
    case BasicShapeRadius::FarthestSide:
        return pool.createIdentifierValue(CSSValueFarthestSide);
    }

    ASSERT_NOT_REACHED();
    return 0;
}

PassRefPtr<CSSValue> valueForBasicShape(const RenderStyle* style, const BasicShape* basicShape)
{
    CSSValuePool& pool = cssValuePool();

    RefPtr<CSSBasicShape> basicShapeValue;
    switch (basicShape->type()) {
    case BasicShape::BasicShapeCircleType: {
        const BasicShapeCircle* circle = static_cast<const BasicShapeCircle*>(basicShape);
        RefPtr<CSSBasicShapeCircle> circleValue = CSSBasicShapeCircle::create();

        circleValue->setCenterX(valueForCenterCoordinate(pool, style, circle->centerX(), HORIZONTAL));
        circleValue->setCenterY(valueForCenterCoordinate(pool, style, circle->centerY(), VERTICAL));
        circleValue->setRadius(basicShapeRadiusToCSSValue(style, pool, circle->radius()));
        basicShapeValue = circleValue.release();
        break;
    }
    case BasicShape::BasicShapeEllipseType: {
        const BasicShapeEllipse* ellipse = static_cast<const BasicShapeEllipse*>(basicShape);
        RefPtr<CSSBasicShapeEllipse> ellipseValue = CSSBasicShapeEllipse::create();

        ellipseValue->setCenterX(valueForCenterCoordinate(pool, style, ellipse->centerX(), HORIZONTAL));
        ellipseValue->setCenterY(valueForCenterCoordinate(pool, style, ellipse->centerY(), VERTICAL));
        ellipseValue->setRadiusX(basicShapeRadiusToCSSValue(style, pool, ellipse->radiusX()));
        ellipseValue->setRadiusY(basicShapeRadiusToCSSValue(style, pool, ellipse->radiusY()));
        basicShapeValue = ellipseValue.release();
        break;
    }
    case BasicShape::BasicShapePolygonType: {
        const BasicShapePolygon* polygon = static_cast<const BasicShapePolygon*>(basicShape);
        RefPtr<CSSBasicShapePolygon> polygonValue = CSSBasicShapePolygon::create();

        polygonValue->setWindRule(polygon->windRule());
        const Vector<Length>& values = polygon->values();
        for (unsigned i = 0; i < values.size(); i += 2)
            polygonValue->appendPoint(pool.createValue(values.at(i), style), pool.createValue(values.at(i + 1), style));

        basicShapeValue = polygonValue.release();
        break;
    }
    case BasicShape::BasicShapeInsetType: {
        const BasicShapeInset* inset = static_cast<const BasicShapeInset*>(basicShape);
        RefPtr<CSSBasicShapeInset> insetValue = CSSBasicShapeInset::create();

        insetValue->setTop(pool.createValue(inset->top(), style));
        insetValue->setRight(pool.createValue(inset->right(), style));
        insetValue->setBottom(pool.createValue(inset->bottom(), style));
        insetValue->setLeft(pool.createValue(inset->left(), style));

        insetValue->setTopLeftRadius(pool.createValue(inset->topLeftRadius(), style));
        insetValue->setTopRightRadius(pool.createValue(inset->topRightRadius(), style));
        insetValue->setBottomRightRadius(pool.createValue(inset->bottomRightRadius(), style));
        insetValue->setBottomLeftRadius(pool.createValue(inset->bottomLeftRadius(), style));

        basicShapeValue = insetValue.release();
        break;
    }
    default:
        break;
    }

    return pool.createValue(basicShapeValue.release());
}

static Length convertToLength(const CSSToLengthConversionData& conversionData, CSSPrimitiveValue* value)
{
    return value->convertToLength<FixedIntegerConversion | FixedFloatConversion | PercentConversion | CalculatedConversion>(conversionData);
}

static LengthSize convertToLengthSize(const CSSToLengthConversionData& conversionData, CSSPrimitiveValue* value)
{
    if (!value)
        return LengthSize(Length(0, Fixed), Length(0, Fixed));

    Pair* pair = value->getPairValue();
    return LengthSize(convertToLength(conversionData, pair->first()), convertToLength(conversionData, pair->second()));
}

static BasicShapeCenterCoordinate convertToCenterCoordinate(const CSSToLengthConversionData& conversionData, CSSPrimitiveValue* value)
{
    BasicShapeCenterCoordinate::Direction direction;
    Length offset = Length(0, Fixed);

    CSSValueID keyword = CSSValueTop;
    if (!value)
        keyword = CSSValueCenter;
    else if (value->isValueID())
        keyword = value->getValueID();
    else if (Pair* pair = value->getPairValue()) {
        keyword = pair->first()->getValueID();
        offset = convertToLength(conversionData, pair->second());
    } else
        offset = convertToLength(conversionData, value);

    switch (keyword) {
    case CSSValueTop:
    case CSSValueLeft:
        direction = BasicShapeCenterCoordinate::TopLeft;
        break;
    case CSSValueRight:
    case CSSValueBottom:
        direction = BasicShapeCenterCoordinate::BottomRight;
        break;
    case CSSValueCenter:
        direction = BasicShapeCenterCoordinate::TopLeft;
        offset = Length(50, Percent);
        break;
    default:
        ASSERT_NOT_REACHED();
        direction = BasicShapeCenterCoordinate::TopLeft;
        break;
    }

    return BasicShapeCenterCoordinate(direction, offset);
}

static BasicShapeRadius cssValueToBasicShapeRadius(const CSSToLengthConversionData& conversionData, PassRefPtr<CSSPrimitiveValue> radius)
{
    if (!radius)
        return BasicShapeRadius(BasicShapeRadius::ClosestSide);

    if (radius->isValueID()) {
        switch (radius->getValueID()) {
        case CSSValueClosestSide:
            return BasicShapeRadius(BasicShapeRadius::ClosestSide);
        case CSSValueFarthestSide:
            return BasicShapeRadius(BasicShapeRadius::FarthestSide);
        default:
            ASSERT_NOT_REACHED();
            break;
        }
    }

    return BasicShapeRadius(convertToLength(conversionData, radius.get()));
}

PassRefPtr<BasicShape> basicShapeForValue(const CSSToLengthConversionData& conversionData, const CSSBasicShape* basicShapeValue)
{
    RefPtr<BasicShape> basicShape;

    switch (basicShapeValue->type()) {
    case CSSBasicShape::CSSBasicShapeCircleType: {
        const CSSBasicShapeCircle* circleValue = static_cast<const CSSBasicShapeCircle *>(basicShapeValue);
        RefPtr<BasicShapeCircle> circle = BasicShapeCircle::create();

        circle->setCenterX(convertToCenterCoordinate(conversionData, circleValue->centerX()));
        circle->setCenterY(convertToCenterCoordinate(conversionData, circleValue->centerY()));
        circle->setRadius(cssValueToBasicShapeRadius(conversionData, circleValue->radius()));

        basicShape = circle.release();
        break;
    }
    case CSSBasicShape::CSSBasicShapeEllipseType: {
        const CSSBasicShapeEllipse* ellipseValue = static_cast<const CSSBasicShapeEllipse *>(basicShapeValue);
        RefPtr<BasicShapeEllipse> ellipse = BasicShapeEllipse::create();

        ellipse->setCenterX(convertToCenterCoordinate(conversionData, ellipseValue->centerX()));
        ellipse->setCenterY(convertToCenterCoordinate(conversionData, ellipseValue->centerY()));

        ellipse->setRadiusX(cssValueToBasicShapeRadius(conversionData, ellipseValue->radiusX()));
        ellipse->setRadiusY(cssValueToBasicShapeRadius(conversionData, ellipseValue->radiusY()));

        basicShape = ellipse.release();
        break;
    }
    case CSSBasicShape::CSSBasicShapePolygonType: {
        const CSSBasicShapePolygon* polygonValue = static_cast<const CSSBasicShapePolygon *>(basicShapeValue);
        RefPtr<BasicShapePolygon> polygon = BasicShapePolygon::create();

        polygon->setWindRule(polygonValue->windRule());
        const Vector<RefPtr<CSSPrimitiveValue>>& values = polygonValue->values();
        for (unsigned i = 0; i < values.size(); i += 2)
            polygon->appendPoint(convertToLength(conversionData, values.at(i).get()), convertToLength(conversionData, values.at(i + 1).get()));

        basicShape = polygon.release();
        break;
    }
    case CSSBasicShape::CSSBasicShapeInsetType: {
        const CSSBasicShapeInset* rectValue = static_cast<const CSSBasicShapeInset* >(basicShapeValue);
        RefPtr<BasicShapeInset> rect = BasicShapeInset::create();

        rect->setTop(convertToLength(conversionData, rectValue->top()));
        rect->setRight(convertToLength(conversionData, rectValue->right()));
        rect->setBottom(convertToLength(conversionData, rectValue->bottom()));
        rect->setLeft(convertToLength(conversionData, rectValue->left()));

        rect->setTopLeftRadius(convertToLengthSize(conversionData, rectValue->topLeftRadius()));
        rect->setTopRightRadius(convertToLengthSize(conversionData, rectValue->topRightRadius()));
        rect->setBottomRightRadius(convertToLengthSize(conversionData, rectValue->bottomRightRadius()));
        rect->setBottomLeftRadius(convertToLengthSize(conversionData, rectValue->bottomLeftRadius()));

        basicShape = rect.release();
        break;
    }
    default:
        break;
    }

    return basicShape.release();
}

float floatValueForCenterCoordinate(const BasicShapeCenterCoordinate& center, float boxDimension)
{
    float offset = floatValueForLength(center.length(), boxDimension);
    if (center.direction() == BasicShapeCenterCoordinate::TopLeft)
        return offset;
    return boxDimension - offset;
}

}
