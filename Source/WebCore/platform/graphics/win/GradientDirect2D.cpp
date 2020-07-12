/*
 * Copyright (C) 2016-2019 Apple Inc.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "Gradient.h"

#include "FloatPoint.h"
#include "GraphicsContext.h"
#include "PlatformContextDirect2D.h"
#include <d2d1.h>
#include <wtf/MathExtras.h>

#define GRADIENT_DRAWING 3

namespace WebCore {

void Gradient::stopsChanged()
{
    m_brush = nullptr;
}

static bool circleIsEntirelyContained(const FloatPoint& centerA, float radiusA, const FloatPoint& centerB, float radiusB)
{
    double deltaX = centerB.x() - centerA.x();
    double deltaY = centerB.y() - centerA.y();
    double deltaRadius = radiusB - radiusA;

    return deltaX * deltaX + deltaY * deltaY < deltaRadius * deltaRadius && radiusA < radiusB;
}

ID2D1Brush* Gradient::createBrush(ID2D1RenderTarget* renderTarget)
{
    sortStops();

    Vector<D2D1_GRADIENT_STOP> gradientStops;
    gradientStops.reserveInitialCapacity(m_stops.size());
    for (auto& stop : m_stops) {
        // FIXME: Add support for non-sRGBA color spaces.
        auto [r, g, b, a] stop.color.toSRGBALossy<float>();
        gradientStops.gradientStops.uncheckedAppend(D2D1::GradientStop(stop.offset, D2D1::ColorF(r, g, b, a)));
    }

    WTF::switchOn(m_data,
        [&] (const LinearData&) {
            // No action needed.
        },
        [&] (RadialData& data) {
            RELEASE_ASSERT(data.startRadius >= 0);
            RELEASE_ASSERT(data.endRadius >= 0);

            if (data.startRadius > data.endRadius) {
                gradientStops.reverse();
                for (auto& stop : gradientStops)
                    stop.position = 1.0 - stop.position;

                std::swap(data.startRadius, data.endRadius);
                std::swap(data.point0, data.point1);
            }

            if (data.startRadius) {
                RELEASE_ASSERT(data.endRadius > 0);
                float startPosition = data.startRadius / data.endRadius;
                RELEASE_ASSERT(startPosition <= 1.0);
                float availableRange = 1.0 - startPosition;
                RELEASE_ASSERT(availableRange <= 1.0 && availableRange >= 0);

                for (auto& stop : gradientStops) {
                    stop.position *= availableRange;
                    stop.position += startPosition;
                }

                // Restore the 'start' position
                auto firstStop = gradientStops.first();
                if (!WTF::areEssentiallyEqual(firstStop.position, 0.0f)) {
                    firstStop.position = 0;
                    gradientStops.insert(0, firstStop);
                }
            }
        },
        [&] (const ConicData&) {
            // FIXME: Assess whether needed for Conic Gradients.
        }
    );

    COMPtr<ID2D1GradientStopCollection> gradientStopCollection;
    HRESULT hr = renderTarget->CreateGradientStopCollection(gradientStops.data(), gradientStops.size(), &gradientStopCollection);
    RELEASE_ASSERT(SUCCEEDED(hr));

    WTF::switchOn(m_data,
        [&] (const LinearData& data) {
            ID2D1LinearGradientBrush* linearGradient = nullptr;
            hr = renderTarget->CreateLinearGradientBrush(
                D2D1::LinearGradientBrushProperties(data.point0, data.point1),
                D2D1::BrushProperties(), gradientStopCollection.get(),
                &linearGradient);
            RELEASE_ASSERT(SUCCEEDED(hr));
            m_brush = adoptCOM(linearGradient);
        },
        [&] (const RadialData& data) {
            FloatSize offset;
            if (!circleIsEntirelyContained(data.point0, data.startRadius, data.point1, data.endRadius))
                offset = (data.point1.scaled(data.startRadius) - data.point0.scaled(data.endRadius)) / (data.endRadius - data.startRadius);
            else
                offset = data.point0 - data.point1;

            FloatPoint center = data.point1;
            float radiusX = data.endRadius;
            float radiusY = radiusX / data.aspectRatio;

            ID2D1RadialGradientBrush* radialGradient = nullptr;
            hr = renderTarget->CreateRadialGradientBrush(
                D2D1::RadialGradientBrushProperties(center, D2D1::Point2F(offset.width(), offset.height()), radiusX, radiusY),
                D2D1::BrushProperties(), gradientStopCollection.get(),
                &radialGradient);
            RELEASE_ASSERT(SUCCEEDED(hr));
            m_brush = adoptCOM(radialGradient);
        },
        [&] (const ConicData&) {
            // FIXME: implement conic gradient rendering.
            m_brush = nullptr;
        }
    );

    return m_brush.get();
}

void Gradient::fill(GraphicsContext& context, const FloatRect& rect)
{
    auto d2dContext = context.platformContext()->renderTarget();

    WTF::switchOn(m_data,
        [&] (const LinearData& data) {
            // FIXME: If we have a brush, why can we re-use it? What guarantees it's right for this context?
            if (!m_brush)
                createBrush(d2dContext);

#if ASSERT_ENABLED
            d2dContext->SetTags(GRADIENT_DRAWING, __LINE__);
#endif

            const D2D1_RECT_F d2dRect = rect;
            d2dContext->FillRectangle(&d2dRect, m_brush.get());
        },
        [&] (const RadialData& data) {
            bool needScaling = data.aspectRatio != 1;
            if (needScaling) {
                context.save();
                // Scale from the center of the gradient. We only ever scale non-deprecated gradients,
                // for which m_p0 == m_p1.
                ASSERT(data.point0 == data.point1);

                D2D1_MATRIX_3X2_F ctm = { };
                d2dContext->GetTransform(&ctm);

                AffineTransform transform(ctm);
                transform.translate(data.point0);
                transform.scaleNonUniform(1.0, 1.0 / data.aspectRatio);
                transform.translate(-data.point0);

                d2dContext->SetTransform(ctm);
            }

            // FIXME: If we have a brush, why can we re-use it? What guarantees it's right for this context?
            if (!m_brush)
                createBrush(d2dContext);

#if ASSERT_ENABLED
            d2dContext->SetTags(GRADIENT_DRAWING, __LINE__);
#endif

            const D2D1_RECT_F d2dRect = rect;
            d2dContext->FillRectangle(&d2dRect, m_brush.get());

            if (needScaling)
                context.restore();
        },
        [&] (const ConicData&) {
            // FIXME: implement conic gradient rendering.
        }
    );
}

}
