/*
 * Copyright (C) 2006, 2007, 2008 Apple Computer, Inc.  All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
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

#include "GraphicsContext.h"

#include <QGradient>
#include <QPainter>

namespace WebCore {

void Gradient::platformDestroy()
{
    delete m_gradient;
    m_gradient = 0;
}

QGradient* Gradient::platformGradient()
{
    if (m_gradient)
        return m_gradient;

    bool reversed;
    qreal innerRadius;
    qreal outerRadius;

    WTF::switchOn(m_data,
        [&] (const LinearData& data) {
            m_gradient = new QLinearGradient(data.point0.x(), data.point0.y(), data.point1.x(), data.point1.y());
        },
        [&] (const RadialData& data) {
            reversed = data.startRadius > data.endRadius;
            innerRadius = reversed ? data.endRadius : data.startRadius;
            outerRadius = reversed ? data.startRadius : data.endRadius;
            QPointF center = reversed ? data.point0 : data.point1;
            QPointF focalPoint = reversed ? data.point1 : data.point0;

            m_gradient = new QRadialGradient(center, outerRadius, focalPoint);
        },
        [&] (const ConicData&) {}
    );

    m_gradient->setInterpolationMode(QGradient::ComponentInterpolation);

    sortStopsIfNecessary();

    QColor stopColor;
    qreal lastStop(0.0);
    const qreal lastStopDiff = 0.0000001;

    for (auto stop : m_stops) {
        // Drop gradient stops after 1.0 to avoid overwriting color at 1.0
        if (lastStop >= 1)
            break;

        stopColor.setRgbF(stop.color.red(), stop.color.green(), stop.color.blue(), stop.color.alpha());
        if (qFuzzyCompare(lastStop, qreal(stop.offset)))
            lastStop = stop.offset + lastStopDiff;
        else
            lastStop = stop.offset;

        WTF::switchOn(m_data,
            [&] (const LinearData&) {},
            [&] (const RadialData&) {
                if (!qFuzzyCompare(1 + outerRadius, qreal(1))) {
                    lastStop = lastStop * (1.0f - innerRadius / outerRadius);
                    if (!reversed)
                        lastStop += innerRadius / outerRadius;
                }
            },
            [&] (const ConicData&) {});

        // Clamp stop position to 1.0, otherwise QGradient will ignore it
        // https://bugs.webkit.org/show_bug.cgi?id=41484
        qreal stopPosition = qMin(lastStop, qreal(1.0f));

        WTF::switchOn(m_data,
            [&] (const LinearData&) {},
            [&] (const RadialData&) {
                if (reversed)
                    stopPosition = 1 - stopPosition;
            },
            [&] (const ConicData&) {});

        m_gradient->setColorAt(stopPosition, stopColor);
        // Keep the lastStop as orginal value, since the following stopColor depend it
        lastStop = stop.offset;
    }

    if (m_stops.isEmpty()) {
        // The behavior of QGradient with no stops is defined differently from HTML5 spec,
        // where the latter requires the gradient to be transparent black.
        m_gradient->setColorAt(0.0, QColor(0, 0, 0, 0));
    }

    switch (m_spreadMethod) {
    case SpreadMethodPad:
        m_gradient->setSpread(QGradient::PadSpread);
        break;
    case SpreadMethodReflect:
        m_gradient->setSpread(QGradient::ReflectSpread);
        break;
    case SpreadMethodRepeat:
        m_gradient->setSpread(QGradient::RepeatSpread);
        break;
    }

    return m_gradient;
}

void Gradient::fill(GraphicsContext& context, const FloatRect& rect)
{
    context.platformContext()->fillRect(rect, *platformGradient());
}

} //namespace
