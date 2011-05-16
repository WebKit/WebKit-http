/*
 * Copyright (C) 2004, 2005 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005 Rob Buis <buis@kde.org>
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
#include "SVGTransform.h"

#include "FloatConversion.h"
#include "FloatPoint.h"
#include "FloatSize.h"
#include "SVGAngle.h"
#include "SVGSVGElement.h"
#include <wtf/MathExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

SVGTransform::SVGTransform()
    : m_type(SVG_TRANSFORM_UNKNOWN)
    , m_angle(0)
{
}

SVGTransform::SVGTransform(SVGTransformType type)
    : m_type(type)
    , m_angle(0)
{
}

SVGTransform::SVGTransform(const AffineTransform& matrix)
    : m_type(SVG_TRANSFORM_MATRIX)
    , m_angle(0)
    , m_matrix(matrix)
{
}

void SVGTransform::setMatrix(const AffineTransform& matrix)
{
    m_type = SVG_TRANSFORM_MATRIX;
    m_angle = 0;
    m_matrix = matrix;
}

void SVGTransform::updateMatrix()
{
    // The underlying matrix has been changed, alter the transformation type.
    // Spec: In case the matrix object is changed directly (i.e., without using the methods on the SVGTransform interface itself)
    // then the type of the SVGTransform changes to SVG_TRANSFORM_MATRIX.
    m_type = SVG_TRANSFORM_MATRIX;
    m_angle = 0;
}

void SVGTransform::setTranslate(float tx, float ty)
{
    m_type = SVG_TRANSFORM_TRANSLATE;
    m_angle = 0;

    m_matrix.makeIdentity();
    m_matrix.translate(tx, ty);
}

FloatPoint SVGTransform::translate() const
{
    return FloatPoint::narrowPrecision(m_matrix.e(), m_matrix.f());
}

void SVGTransform::setScale(float sx, float sy)
{
    m_type = SVG_TRANSFORM_SCALE;
    m_angle = 0;
    m_center = FloatPoint();

    m_matrix.makeIdentity();
    m_matrix.scaleNonUniform(sx, sy);
}

FloatSize SVGTransform::scale() const
{
    return FloatSize::narrowPrecision(m_matrix.a(), m_matrix.d());
}

void SVGTransform::setRotate(float angle, float cx, float cy)
{
    m_type = SVG_TRANSFORM_ROTATE;
    m_angle = angle;
    m_center = FloatPoint(cx, cy);

    // TODO: toString() implementation, which can show cx, cy (need to be stored?)
    m_matrix.makeIdentity();
    m_matrix.translate(cx, cy);
    m_matrix.rotate(angle);
    m_matrix.translate(-cx, -cy);
}

void SVGTransform::setSkewX(float angle)
{
    m_type = SVG_TRANSFORM_SKEWX;
    m_angle = angle;

    m_matrix.makeIdentity();
    m_matrix.skewX(angle);
}

void SVGTransform::setSkewY(float angle)
{
    m_type = SVG_TRANSFORM_SKEWY;
    m_angle = angle;

    m_matrix.makeIdentity();
    m_matrix.skewY(angle);
}

String SVGTransform::valueAsString() const
{
    switch (m_type) {
    case SVG_TRANSFORM_UNKNOWN:
        return String();
    case SVG_TRANSFORM_MATRIX: {
        DEFINE_STATIC_LOCAL(String, matrixString, ("matrix("));
        StringBuilder builder;
        builder.append(matrixString + String::number(m_matrix.a()) + ' ' + String::number(m_matrix.b()) + ' ' + String::number(m_matrix.c()) + ' ' +
                       String::number(m_matrix.d()) + ' ' + String::number(m_matrix.e()) + ' ' + String::number(m_matrix.f()) + ')');
        return builder.toString();
    }
    case SVG_TRANSFORM_TRANSLATE: {
        DEFINE_STATIC_LOCAL(String, translateString, ("translate("));
        return translateString + String::number(m_matrix.e()) + ' ' + String::number(m_matrix.f()) + ')';
    }
    case SVG_TRANSFORM_SCALE: {
        DEFINE_STATIC_LOCAL(String, scaleString, ("scale("));
        return scaleString + String::number(m_matrix.xScale()) + ' ' + String::number(m_matrix.yScale()) + ')';
    }
    case SVG_TRANSFORM_ROTATE: {
        DEFINE_STATIC_LOCAL(String, rotateString, ("rotate("));
        double angleInRad = deg2rad(m_angle);
        double cosAngle = cos(angleInRad);
        double sinAngle = sin(angleInRad);
        float cx = narrowPrecisionToFloat(cosAngle != 1 ? (m_matrix.e() * (1 - cosAngle) - m_matrix.f() * sinAngle) / (1 - cosAngle) / 2 : 0);
        float cy = narrowPrecisionToFloat(cosAngle != 1 ? (m_matrix.e() * sinAngle / (1 - cosAngle) + m_matrix.f()) / 2 : 0);
        if (cx || cy)
            return rotateString + String::number(m_angle) + ' ' + String::number(cx) + ' ' + String::number(cy) + ')';
        return rotateString + String::number(m_angle) + ')';
    }    
    case SVG_TRANSFORM_SKEWX: {
        DEFINE_STATIC_LOCAL(String, skewXString, ("skewX("));
        return skewXString + String::number(m_angle) + ')';
    }
    case SVG_TRANSFORM_SKEWY: {
        DEFINE_STATIC_LOCAL(String, skewYString, ("skewY("));
        return skewYString + String::number(m_angle) + ')';
    }
    }

    ASSERT_NOT_REACHED();
    return String();
}

} // namespace WebCore

#endif // ENABLE(SVG)
