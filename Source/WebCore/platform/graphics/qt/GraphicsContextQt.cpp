/*
 * Copyright (C) 2006 Dirk Mueller <mueller@kde.org>
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2006 George Staikos <staikos@kde.org>
 * Copyright (C) 2006 Simon Hausmann <hausmann@kde.org>
 * Copyright (C) 2006 Allan Sandfeld Jensen <sandfeld@kde.org>
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nokia Corporation and/or its subsidiary(-ies).
 * Copyright (C) 2008 Dirk Schulze <vbs85@gmx.de>
 * Copyright (C) 2010, 2011 Sencha, Inc.
 * Copyright (C) 2011 Andreas Kling <kling@webkit.org>
 * Copyright (C) 2015 The Qt Company Ltd.
 *
 * All rights reserved.
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
#include "GraphicsContext.h"

#if OS(WINDOWS)
#include <windows.h>
#endif

#include "AffineTransform.h"
#include "Color.h"
#include "DisplayListRecorder.h"
#include "FloatConversion.h"
#include "ImageBuffer.h"
#include "ImageBufferDataQt.h"
#include "Path.h"
#include "Pattern.h"
#include "ShadowBlur.h"
#include "TransformationMatrix.h"
#include "TransparencyLayer.h"
#include "URL.h"

#include <QBrush>
#include <QGradient>
#include <QPaintDevice>
#include <QPaintEngine>
#include <QPainter>
#include <QPainterPath>
#include <QPainterPathStroker>
#include <QPixmap>
#include <QPolygonF>
#include <QStack>
#include <QUrl>
#include <QVector>
#include <private/qpdf_p.h>
#include <wtf/MathExtras.h>

#if OS(WINDOWS)
QT_BEGIN_NAMESPACE
Q_GUI_EXPORT QPixmap qt_pixmapFromWinHBITMAP(HBITMAP, int hbitmapFormat = 0);
QT_END_NAMESPACE

enum HBitmapFormat {
    HBitmapNoAlpha,
    HBitmapPremultipliedAlpha,
    HBitmapAlpha
};
#endif

namespace WebCore {

static inline QPainter::CompositionMode toQtCompositionMode(CompositeOperator op)
{
    switch (op) {
    case CompositeClear:
        return QPainter::CompositionMode_Clear;
    case CompositeCopy:
        return QPainter::CompositionMode_Source;
    case CompositeSourceOver:
        return QPainter::CompositionMode_SourceOver;
    case CompositeSourceIn:
        return QPainter::CompositionMode_SourceIn;
    case CompositeSourceOut:
        return QPainter::CompositionMode_SourceOut;
    case CompositeSourceAtop:
        return QPainter::CompositionMode_SourceAtop;
    case CompositeDestinationOver:
        return QPainter::CompositionMode_DestinationOver;
    case CompositeDestinationIn:
        return QPainter::CompositionMode_DestinationIn;
    case CompositeDestinationOut:
        return QPainter::CompositionMode_DestinationOut;
    case CompositeDestinationAtop:
        return QPainter::CompositionMode_DestinationAtop;
    case CompositeXOR:
        return QPainter::CompositionMode_Xor;
    case CompositePlusDarker:
        // there is no exact match, but this is the closest
        return QPainter::CompositionMode_Darken;
    case CompositePlusLighter:
        return QPainter::CompositionMode_Plus;
    case CompositeDifference:
        return QPainter::CompositionMode_Difference;
    default:
        ASSERT_NOT_REACHED();
    }

    return QPainter::CompositionMode_SourceOver;
}

static inline QPainter::CompositionMode toQtCompositionMode(BlendMode op)
{
    switch (op) {
    case BlendModeNormal:
        return QPainter::CompositionMode_SourceOver;
    case BlendModeMultiply:
        return QPainter::CompositionMode_Multiply;
    case BlendModeScreen:
        return QPainter::CompositionMode_Screen;
    case BlendModeOverlay:
        return QPainter::CompositionMode_Overlay;
    case BlendModeDarken:
        return QPainter::CompositionMode_Darken;
    case BlendModeLighten:
        return QPainter::CompositionMode_Lighten;
    case BlendModeColorDodge:
        return QPainter::CompositionMode_ColorDodge;
    case BlendModeColorBurn:
        return QPainter::CompositionMode_ColorBurn;
    case BlendModeHardLight:
        return QPainter::CompositionMode_HardLight;
    case BlendModeSoftLight:
        return QPainter::CompositionMode_SoftLight;
    case BlendModeDifference:
        return QPainter::CompositionMode_Difference;
    case BlendModeExclusion:
        return QPainter::CompositionMode_Exclusion;
    case BlendModeHue:
    case BlendModeSaturation:
    case BlendModeColor:
    case BlendModeLuminosity:
        // Not supported.
        break;
    default:
        ASSERT_NOT_REACHED();
    }

    return QPainter::CompositionMode_SourceOver;
}

static inline Qt::PenCapStyle toQtLineCap(LineCap lc)
{
    switch (lc) {
    case ButtCap:
        return Qt::FlatCap;
    case RoundCap:
        return Qt::RoundCap;
    case SquareCap:
        return Qt::SquareCap;
    default:
        ASSERT_NOT_REACHED();
    }

    return Qt::FlatCap;
}

static inline Qt::PenJoinStyle toQtLineJoin(LineJoin lj)
{
    switch (lj) {
    case MiterJoin:
        return Qt::SvgMiterJoin;
    case RoundJoin:
        return Qt::RoundJoin;
    case BevelJoin:
        return Qt::BevelJoin;
    default:
        ASSERT_NOT_REACHED();
    }

    return Qt::SvgMiterJoin;
}

static Qt::PenStyle toQPenStyle(StrokeStyle style)
{
    switch (style) {
    case NoStroke:
        return Qt::NoPen;
        break;
    case SolidStroke:
    case DoubleStroke:
    case WavyStroke:
        return Qt::SolidLine;
        break;
    case DottedStroke:
        return Qt::DotLine;
        break;
    case DashedStroke:
        return Qt::DashLine;
        break;
    default:
        ASSERT_NOT_REACHED();
    }
    return Qt::NoPen;
}

static inline Qt::FillRule toQtFillRule(WindRule rule)
{
    switch (rule) {
    case RULE_EVENODD:
        return Qt::OddEvenFill;
    case RULE_NONZERO:
        return Qt::WindingFill;
    default:
        ASSERT_NOT_REACHED();
    }
    return Qt::OddEvenFill;
}

class GraphicsContextPlatformPrivate {
    WTF_MAKE_NONCOPYABLE(GraphicsContextPlatformPrivate); WTF_MAKE_FAST_ALLOCATED;
public:
    GraphicsContextPlatformPrivate(QPainter*, const QColor& initialSolidColor);
    ~GraphicsContextPlatformPrivate();

    inline QPainter* p() const
    {
        if (layers.isEmpty())
            return painter;
        return &layers.top()->painter;
    }

    bool antiAliasingForRectsAndLines;

    QStack<TransparencyLayer*> layers;
    // Counting real layers. Required by isInTransparencyLayer() calls
    // For example, layers with valid alphaMask are not real layers
    int layerCount;

    // reuse this brush for solid color (to prevent expensive QBrush construction)
    QBrush solidColor;

    InterpolationQuality imageInterpolationQuality;
    bool initialSmoothPixmapTransformHint;

    QRectF clipBoundingRect() const
    {
        return p()->clipBoundingRect();
    }

    void takeOwnershipOfPlatformContext() { platformContextIsOwned = true; }

private:
    QPainter* painter;
    bool platformContextIsOwned;
};

GraphicsContextPlatformPrivate::GraphicsContextPlatformPrivate(QPainter* p, const QColor& initialSolidColor)
    : antiAliasingForRectsAndLines(false)
    , layerCount(0)
    , solidColor(initialSolidColor)
    , imageInterpolationQuality(InterpolationDefault)
    , initialSmoothPixmapTransformHint(false)
    , painter(p)
    , platformContextIsOwned(false)
{
    if (!painter)
        return;

    // Use the default the QPainter was constructed with.
    antiAliasingForRectsAndLines = painter->testRenderHint(QPainter::Antialiasing);

    // Used for default image interpolation quality.
    initialSmoothPixmapTransformHint = painter->testRenderHint(QPainter::SmoothPixmapTransform);

    painter->setRenderHint(QPainter::Antialiasing, true);

}

GraphicsContextPlatformPrivate::~GraphicsContextPlatformPrivate()
{
    if (!platformContextIsOwned)
        return;

    QPaintDevice* device = painter->device();
    painter->end();
    delete painter;
    delete device;
}

void GraphicsContext::platformInit(QPainter* painter)
{
    if (!painter)
        return;

    m_data = new GraphicsContextPlatformPrivate(painter, fillColor());

    // solidColor is initialized with the fillColor().
    painter->setBrush(m_data->solidColor);

    QPen pen(painter->pen());
    pen.setColor(strokeColor());
    pen.setJoinStyle(toQtLineJoin(MiterJoin));
    pen.setCapStyle(Qt::FlatCap);
    painter->setPen(pen);
}

void GraphicsContext::platformDestroy()
{
    if (m_data) {
        while (!m_data->layers.isEmpty())
            endTransparencyLayer();
    }

    delete m_data;
}

PlatformGraphicsContext* GraphicsContext::platformContext() const
{
    return m_data->p();
}

AffineTransform GraphicsContext::getCTM(IncludeDeviceScale includeScale) const
{
    if (paintingDisabled())
        return AffineTransform();

    const QTransform& matrix = (includeScale == DefinitelyIncludeDeviceScale)
        ? platformContext()->combinedTransform()
        : platformContext()->worldTransform();
    return AffineTransform(matrix.m11(), matrix.m12(), matrix.m21(),
                           matrix.m22(), matrix.dx(), matrix.dy());
}

void GraphicsContext::savePlatformState()
{
    if (!m_data->layers.isEmpty() && !m_data->layers.top()->alphaMask.isNull())
        ++m_data->layers.top()->saveCounter;
    m_data->p()->save();
}

void GraphicsContext::restorePlatformState()
{
    if (!m_data->layers.isEmpty() && !m_data->layers.top()->alphaMask.isNull())
        if (!--m_data->layers.top()->saveCounter)
            endPlatformTransparencyLayer();

    m_data->p()->restore();
}

// Draws a filled rectangle with a stroked border.
// This is only used to draw borders (real fill is done via fillRect), and
// thus it must not cast any shadow.
void GraphicsContext::drawRect(const FloatRect& rect, float borderThickness)
{
    if (paintingDisabled())
        return;

    ASSERT(!rect.isEmpty());

    QPainter* p = m_data->p();
    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, m_data->antiAliasingForRectsAndLines);

    // strokeThickness() is disregarded
    QPen oldPen(p->pen());
    QPen newPen(oldPen);
    newPen.setWidthF(borderThickness);
    p->setPen(newPen);

    p->drawRect(rect);

    p->setPen(oldPen);
    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

// This is only used to draw borders.
// Must not cast any shadow.
void GraphicsContext::drawLine(const FloatPoint& point1, const FloatPoint& point2)
{
    if (paintingDisabled())
        return;

    if (strokeStyle() == NoStroke)
        return;

    if (isRecording()) {
        m_displayListRecorder->drawLine(point1, point2);
        return;
    }

    const Color& strokeColor = this->strokeColor();
    float thickness = strokeThickness();
    bool isVerticalLine = (point1.x() + thickness == point2.x());
    float strokeWidth = isVerticalLine ? point2.y() - point1.y() : point2.x() - point1.x();
    if (!thickness || !strokeWidth)
        return;

    QPainter* p = platformContext();
    const bool savedAntiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, m_data->antiAliasingForRectsAndLines);

    StrokeStyle strokeStyle = this->strokeStyle();
    float cornerWidth = 0;
    bool drawsDashedLine = strokeStyle == DottedStroke || strokeStyle == DashedStroke;

    if (drawsDashedLine) {
        p->save();
        // Figure out end points to ensure we always paint corners.
        cornerWidth = strokeStyle == DottedStroke ? thickness : std::min(2 * thickness, std::max(thickness, strokeWidth / 3));

        if (isVerticalLine) {
            p->fillRect(FloatRect(point1.x(), point1.y(), thickness, cornerWidth), strokeColor);
            p->fillRect(FloatRect(point1.x(), point2.y() - cornerWidth, thickness, cornerWidth),  strokeColor);
        } else {
            p->fillRect(FloatRect(point1.x(), point1.y(), cornerWidth, thickness), strokeColor);
            p->fillRect(FloatRect(point2.x() - cornerWidth, point1.y(), cornerWidth, thickness), strokeColor);
        }

        strokeWidth -= 2 * cornerWidth;
        float patternWidth = strokeStyle == DottedStroke ? thickness : std::min(3 * thickness, std::max(thickness, strokeWidth / 3));
        // Check if corner drawing sufficiently covers the line.
        if (strokeWidth <= patternWidth + 1) {
            p->restore();
            return;
        }

        // Pattern starts with full fill and ends with the empty fill.
        // 1. Let's start with the empty phase after the corner.
        // 2. Check if we've got odd or even number of patterns and whether they fully cover the line.
        // 3. In case of even number of patterns and/or remainder, move the pattern start position
        // so that the pattern is balanced between the corners.
        float patternOffset = patternWidth;
        int numberOfSegments = std::floor(strokeWidth / patternWidth);
        bool oddNumberOfSegments = numberOfSegments % 2;
        float remainder = strokeWidth - (numberOfSegments * patternWidth);
        if (oddNumberOfSegments && remainder)
            patternOffset -= remainder / 2.f;
        else if (!oddNumberOfSegments) {
            if (remainder)
                patternOffset += patternOffset - (patternWidth + remainder) / 2.f;
            else
                patternOffset += patternWidth / 2.f;
        }

        Qt::PenCapStyle capStyle = Qt::FlatCap;
        QVector<qreal> dashes { patternWidth / thickness, patternWidth / thickness };

        QPen pen = p->pen();
        pen.setCapStyle(capStyle);
        pen.setDashPattern(dashes);
        pen.setDashOffset(patternOffset / thickness);
        p->setPen(pen);
    }

    FloatPoint p1 = point1;
    FloatPoint p2 = point2;
    // Center line and cut off corners for pattern patining.
    if (isVerticalLine) {
        float centerOffset = (p2.x() - p1.x()) / 2;
        p1.move(centerOffset, cornerWidth);
        p2.move(-centerOffset, -cornerWidth);
    } else {
        float centerOffset = (p2.y() - p1.y()) / 2;
        p1.move(cornerWidth, centerOffset);
        p2.move(-cornerWidth, -centerOffset);
    }

    p->drawLine(p1, p2);

    if (drawsDashedLine)
        p->restore();

    p->setRenderHint(QPainter::Antialiasing, savedAntiAlias);
}

// This method is only used to draw the little circles used in lists.
void GraphicsContext::drawEllipse(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->p()->drawEllipse(rect);
}

void GraphicsContext::drawPattern(Image& image, const FloatRect& tileRect, const AffineTransform& patternTransform,
    const FloatPoint& phase, const FloatSize& spacing, CompositeOperator op, const FloatRect &destRect, BlendMode blendMode)
{
    if (paintingDisabled() || !patternTransform.isInvertible())
        return;

    QPixmap* framePixmap = image.nativeImageForCurrentFrame();
    if (!framePixmap) // If it's too early we won't have an image yet.
        return;

    if (isRecording()) {
        m_displayListRecorder->drawPattern(image, tileRect, patternTransform, phase, spacing, op, destRect, blendMode);
        return;
    }

#if ENABLE(IMAGE_DECODER_DOWN_SAMPLING)
    FloatRect tileRectAdjusted = adjustSourceRectForDownSampling(tileRect, framePixmap->size());
#else
    FloatRect tileRectAdjusted = tileRect;
#endif

    // Qt interprets 0 width/height as full width/height so just short circuit.
    QRectF dr = QRectF(destRect).normalized();
    QRect tr = QRectF(tileRectAdjusted).toRect().normalized();
    if (!dr.width() || !dr.height() || !tr.width() || !tr.height())
        return;

    QPixmap pixmap = *framePixmap;
    if (tr.x() || tr.y() || tr.width() != pixmap.width() || tr.height() != pixmap.height())
        pixmap = pixmap.copy(tr);

    QPoint trTopLeft = tr.topLeft();

    CompositeOperator previousOperator = compositeOperation();

    setCompositeOperation(!pixmap.hasAlpha() && op == CompositeSourceOver ? CompositeCopy : op);

    QPainter* p = platformContext();
    QTransform transform(patternTransform);

    QTransform combinedTransform = p->combinedTransform();
    QTransform targetScaleTransform = QTransform::fromScale(combinedTransform.m11(), combinedTransform.m22());
    QTransform transformWithTargetScale = transform * targetScaleTransform;

    // If this would draw more than one scaled tile, we scale the pixmap first and then use the result to draw.
    if (transformWithTargetScale.type() == QTransform::TxScale) {
        QRectF tileRectInTargetCoords = (transformWithTargetScale * QTransform().translate(phase.x(), phase.y())).mapRect(tr);

        bool tileWillBePaintedOnlyOnce = tileRectInTargetCoords.contains(dr);
        if (!tileWillBePaintedOnlyOnce) {
            QSizeF scaledSize(qreal(pixmap.width()) * transformWithTargetScale.m11(), qreal(pixmap.height()) * transformWithTargetScale.m22());
            QPixmap scaledPixmap(scaledSize.toSize());
            if (pixmap.hasAlpha())
                scaledPixmap.fill(Qt::transparent);
            {
                QPainter painter(&scaledPixmap);
                painter.setCompositionMode(QPainter::CompositionMode_Source);
                painter.setRenderHints(p->renderHints());
                painter.drawPixmap(QRect(0, 0, scaledPixmap.width(), scaledPixmap.height()), pixmap);
            }
            pixmap = scaledPixmap;
            trTopLeft = transformWithTargetScale.map(trTopLeft);
            transform = targetScaleTransform.inverted().translate(transform.dx(), transform.dy());
        }
    }

    /* Translate the coordinates as phase is not in world matrix coordinate space but the tile rect origin is. */
    transform *= QTransform().translate(phase.x(), phase.y());
    transform.translate(trTopLeft.x(), trTopLeft.y());

    QBrush b(pixmap);
    b.setTransform(transform);
    p->fillRect(dr, b);

    setCompositeOperation(previousOperator);
}

/*
 FIXME: Removed in https://bugs.webkit.org/show_bug.cgi?id=153174
 Find out if we need to adjust anything

void GraphicsContext::drawConvexPolygon(size_t npoints, const FloatPoint* points, bool shouldAntialias)
{
    if (paintingDisabled())
        return;

    if (npoints <= 1)
        return;

    QPolygonF polygon(npoints);

    for (size_t i = 0; i < npoints; i++)
        polygon[i] = points[i];

    QPainter* p = m_data->p();

    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, shouldAntialias);

    p->drawConvexPolygon(polygon);

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

void GraphicsContext::clipConvexPolygon(size_t numPoints, const FloatPoint* points, bool antialiased)
{
    if (paintingDisabled())
        return;

    if (numPoints <= 1)
        return;

    QPainterPath path(points[0]);
    for (size_t i = 1; i < numPoints; ++i)
        path.lineTo(points[i]);
    path.setFillRule(Qt::WindingFill);

    QPainter* p = m_data->p();

    bool painterWasAntialiased = p->testRenderHint(QPainter::Antialiasing);

    if (painterWasAntialiased != antialiased)
        p->setRenderHint(QPainter::Antialiasing, antialiased);

    p->setClipPath(path, Qt::IntersectClip);

    if (painterWasAntialiased != antialiased)
        p->setRenderHint(QPainter::Antialiasing, painterWasAntialiased);
}
*/

void GraphicsContext::fillPath(const Path& path)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(toQtFillRule(fillRule()));

    if (hasShadow()) {
        if (mustUseShadowBlur() || m_state.fillPattern || m_state.fillGradient)
        {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(*this, platformPath.controlPointRect());
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                if (m_state.fillPattern) {
                    shadowPainter->fillPath(platformPath, QBrush(m_state.fillPattern->createPlatformPattern()));
                } else if (m_state.fillGradient) {
                    QBrush brush(*m_state.fillGradient->platformGradient());
                    brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
                    shadowPainter->fillPath(platformPath, brush);
                } else {
                    shadowPainter->fillPath(platformPath, p->brush());
                }
                shadow.endShadowLayer(*this);
            }
        } else {
            QPointF offset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(offset);
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
            p->fillPath(platformPath, shadowColor);
            p->translate(-offset);
        }
    }
    if (m_state.fillPattern) {
        p->fillPath(platformPath, QBrush(m_state.fillPattern->createPlatformPattern()));
    } else if (m_state.fillGradient) {
        QBrush brush(*m_state.fillGradient->platformGradient());
        brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
        p->fillPath(platformPath, brush);
    } else
        p->fillPath(platformPath, p->brush());
}

inline static void fillPathStroke(QPainter* painter, const QPainterPath& platformPath, const QPen& pen)
{
    if (pen.color().alphaF() < 1.0) {
        QPainterPathStroker pathStroker;
        pathStroker.setJoinStyle(pen.joinStyle());
        pathStroker.setDashOffset(pen.dashOffset());
        pathStroker.setDashPattern(pen.dashPattern());
        pathStroker.setMiterLimit(pen.miterLimit());
        pathStroker.setCapStyle(pen.capStyle());
        pathStroker.setWidth(pen.widthF());

        QPainterPath stroke = pathStroker.createStroke(platformPath);
        painter->fillPath(stroke, pen.brush());
    } else {
        painter->strokePath(platformPath, pen);
    }
}

void GraphicsContext::strokePath(const Path& path)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen pen(p->pen());
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(toQtFillRule(fillRule()));

    if (hasShadow()) {
        if (mustUseShadowBlur() || m_state.strokePattern || m_state.strokeGradient)
        {
            ShadowBlur shadow(m_state);
            FloatRect boundingRect = platformPath.controlPointRect();
            boundingRect.inflate(pen.miterLimit() + pen.widthF());
            GraphicsContext* shadowContext = shadow.beginShadowLayer(*this, boundingRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                if (m_state.strokeGradient) {
                    QBrush brush(*m_state.strokeGradient->platformGradient());
                    brush.setTransform(m_state.strokeGradient->gradientSpaceTransform());
                    QPen shadowPen(pen);
                    shadowPen.setBrush(brush);
                    fillPathStroke(shadowPainter, platformPath, shadowPen);
                } else {
                    fillPathStroke(shadowPainter, platformPath, pen);
                }
                shadow.endShadowLayer(*this);
            }
        } else {
            QPointF offset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(offset);
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * pen.color().alphaF());
            QPen shadowPen(pen);
            shadowPen.setColor(shadowColor);
            fillPathStroke(p, platformPath, shadowPen);
            p->translate(-offset);
        }
    }

    if (m_state.strokePattern) {
        QBrush brush = m_state.strokePattern->createPlatformPattern();
        pen.setBrush(brush);
        fillPathStroke(p, platformPath, pen);
    } else if (m_state.strokeGradient) {
        QBrush brush(*m_state.strokeGradient->platformGradient());
        brush.setTransform(m_state.strokeGradient->gradientSpaceTransform());
        pen.setBrush(brush);
        fillPathStroke(p, platformPath, pen);
    } else
        fillPathStroke(p, platformPath, pen);
}

static inline void drawRepeatPattern(QPainter* p, Pattern& pattern, const FloatRect& rect)
{
    const QBrush brush = pattern.createPlatformPattern();
    if (brush.style() != Qt::TexturePattern)
        return;

    const bool repeatX = pattern.repeatX();
    const bool repeatY = pattern.repeatY();
    // Patterns must be painted so that the top left of the first image is anchored at
    // the origin of the coordinate space

    QRectF targetRect(rect);
    const int w = brush.texture().width();
    const int h = brush.texture().height();

    ASSERT(p);
    QRegion oldClip;
    if (p->hasClipping())
        oldClip = p->clipRegion();

    QRectF clip = targetRect;
    QRectF patternRect = brush.transform().mapRect(QRectF(0, 0, w, h));
    if (!repeatX) {
        clip.setLeft(patternRect.left());
        clip.setWidth(patternRect.width());
    }
    if (!repeatY) {
        clip.setTop(patternRect.top());
        clip.setHeight(patternRect.height());
    }
    if (!repeatX || !repeatY)
        p->setClipRect(clip);

    p->fillRect(targetRect, brush);

    if (!oldClip.isEmpty())
        p->setClipRegion(oldClip);
    else if (!repeatX || !repeatY)
        p->setClipping(false);
}

void GraphicsContext::fillRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QRectF normalizedRect = rect.normalized();

    if (m_state.fillPattern) {
        if (hasShadow()) {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(*this, normalizedRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                drawRepeatPattern(shadowPainter, *m_state.fillPattern, normalizedRect);
                shadow.endShadowLayer(*this);
            }
        }
        drawRepeatPattern(p, *m_state.fillPattern, normalizedRect);
    } else if (m_state.fillGradient) {
        QBrush brush(*m_state.fillGradient->platformGradient());
        brush.setTransform(m_state.fillGradient->gradientSpaceTransform());
        if (hasShadow()) {
            ShadowBlur shadow(m_state);
            GraphicsContext* shadowContext = shadow.beginShadowLayer(*this, normalizedRect);
            if (shadowContext) {
                QPainter* shadowPainter = shadowContext->platformContext();
                shadowPainter->fillRect(normalizedRect, brush);
                shadow.endShadowLayer(*this);
            }
        }
        p->fillRect(normalizedRect, brush);
    } else {
        if (hasShadow()) {
            if (mustUseShadowBlur()) {
                ShadowBlur shadow(m_state);
                // drawRectShadowWithTiling does not work with rotations, and the fallback of
                // drawing though clipToImageBuffer() produces scaling artifacts for us.
                if (!getCTM().preservesAxisAlignment()) {
                    GraphicsContext* shadowContext = shadow.beginShadowLayer(*this, normalizedRect);
                    if (shadowContext) {
                        QPainter* shadowPainter = shadowContext->platformContext();
                        shadowPainter->fillRect(normalizedRect, p->brush());
                        shadow.endShadowLayer(*this);
                    }
                } else
                    shadow.drawRectShadow(*this, FloatRoundedRect(rect));
            } else {
                // Solid rectangle fill with no blur shadow or transformations applied can be done
                // faster without using the shadow layer at all.
                QColor shadowColor = m_state.shadowColor;
                shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
                p->fillRect(normalizedRect.translated(QPointF(m_state.shadowOffset.width(), m_state.shadowOffset.height())), shadowColor);
            }
        }

        p->fillRect(normalizedRect, p->brush());
    }
}


void GraphicsContext::fillRect(const FloatRect& rect, const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    QRectF platformRect(rect);
    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawRectShadow(*this, FloatRoundedRect(platformRect));
        } else {
            QColor shadowColor = m_state.shadowColor;
            shadowColor.setAlphaF(shadowColor.alphaF() * p->brush().color().alphaF());
            p->fillRect(platformRect.translated(QPointF(m_state.shadowOffset.width(), m_state.shadowOffset.height())), shadowColor);
        }
    }
    p->fillRect(platformRect, QColor(color));
}

void GraphicsContext::platformFillRoundedRect(const FloatRoundedRect& rect, const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    Path path;
    path.addRoundedRect(rect);
    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawRectShadow(*this, rect);
        } else {
            const QPointF shadowOffset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(shadowOffset);
            p->fillPath(path.platformPath(), QColor(m_state.shadowColor));
            p->translate(-shadowOffset);
        }
    }
    p->fillPath(path.platformPath(), QColor(color));
}

void GraphicsContext::fillRectWithRoundedHole(const FloatRect& rect, const FloatRoundedRect& roundedHoleRect, const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    Path path;
    path.addRect(rect);
    if (!roundedHoleRect.radii().isZero())
        path.addRoundedRect(roundedHoleRect);
    else
        path.addRect(roundedHoleRect.rect());

    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(Qt::OddEvenFill);

    QPainter* p = m_data->p();
    if (hasShadow()) {
        if (mustUseShadowBlur()) {
            ShadowBlur shadow(m_state);
            shadow.drawInsetShadow(*this, rect, roundedHoleRect);
        } else {
            const QPointF shadowOffset(m_state.shadowOffset.width(), m_state.shadowOffset.height());
            p->translate(shadowOffset);
            p->fillPath(platformPath, QColor(m_state.shadowColor));
            p->translate(-shadowOffset);
        }
    }

    p->fillPath(platformPath, QColor(color));
}

void GraphicsContext::clip(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    m_data->p()->setClipRect(rect, Qt::IntersectClip);
}

IntRect GraphicsContext::clipBounds() const
{
    QPainter* p = m_data->p();
    QRectF clipRect;

    clipRect = p->transform().inverted().mapRect(p->window());

    if (p->hasClipping())
        clipRect = clipRect.intersected(m_data->clipBoundingRect());

    return enclosingIntRect(clipRect);
}

void GraphicsContext::clipPath(const Path& path, WindRule clipRule)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath platformPath = path.platformPath();
    platformPath.setFillRule(clipRule == RULE_EVENODD ? Qt::OddEvenFill : Qt::WindingFill);
    p->setClipPath(platformPath, Qt::IntersectClip);
}

void GraphicsContext::clipToImageBuffer(ImageBuffer& buffer, const FloatRect& destRect)
{
    if (paintingDisabled())
        return;

    IntRect rect = enclosingIntRect(destRect);
    buffer.m_data.m_impl->clip(*this, rect);
}

void drawFocusRingForPath(QPainter* p, const QPainterPath& path, const Color& color, bool antiAliasing)
{
    const bool antiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, antiAliasing);

    const QPen oldPen = p->pen();
    const QBrush oldBrush = p->brush();

    QPen nPen = p->pen();
    nPen.setColor(color);
    p->setBrush(Qt::NoBrush);
    nPen.setStyle(Qt::DotLine);

    p->strokePath(path, nPen);
    p->setBrush(oldBrush);
    p->setPen(oldPen);

    p->setRenderHint(QPainter::Antialiasing, antiAlias);
}

void GraphicsContext::drawFocusRing(const Path& path, float /* width */, float /* offset */, const Color& color)
{
    // FIXME: Use 'offset' for something? http://webkit.org/b/49909

    if (paintingDisabled() || !color.isValid())
        return;

    drawFocusRingForPath(m_data->p(), path.platformPath(), color, m_data->antiAliasingForRectsAndLines);
}

/**
 * Focus ring handling for form controls is not handled here. Qt style in
 * RenderTheme handles drawing focus on widgets which 
 * need it. It is still handled here for links.
 */
void GraphicsContext::drawFocusRing(const Vector<FloatRect>& rects, float width, float offset, const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    unsigned rectCount = rects.size();

    if (!rects.size())
        return;

    float radius = (width - 1) / 2;
    QPainterPath path;
    for (unsigned i = 0; i < rectCount; ++i) {
        QRectF rect = QRectF(rects[i]).adjusted(-offset - radius, -offset - radius, offset + radius, offset + radius);
        // This is not the most efficient way to add a rect to a path, but if we don't create the tmpPath,
        // we will end up with ugly lines in between rows of text on anchors with multiple lines.
        QPainterPath tmpPath;
        tmpPath.addRoundedRect(rect, radius, radius);
        path = path.united(tmpPath);
    }
    drawFocusRingForPath(m_data->p(), path, color, m_data->antiAliasingForRectsAndLines);
}

void GraphicsContext::drawLineForText(const FloatPoint& origin, float width, bool printing, bool doubleLines)
{
    if (paintingDisabled())
        return;

    if (isRecording()) {
        DashArray widths;
        widths.append(width);
        widths.append(0);
        m_displayListRecorder->drawLinesForText(origin, widths, printing, doubleLines, strokeThickness());
        return;
    }

    Color localStrokeColor(strokeColor());

    FloatRect bounds = computeLineBoundsAndAntialiasingModeForText(origin, width, printing, localStrokeColor);
    bool strokeColorChanged = strokeColor() != localStrokeColor;
    bool strokeThicknessChanged = strokeThickness() != bounds.height();
    bool needSavePen = strokeColorChanged || strokeThicknessChanged;

    QPainter* p = platformContext();
    const bool savedAntiAlias = p->testRenderHint(QPainter::Antialiasing);
    p->setRenderHint(QPainter::Antialiasing, m_data->antiAliasingForRectsAndLines);

    QPen oldPen(p->pen());
    if (needSavePen) {
        QPen newPen(oldPen);
        if (strokeThicknessChanged)
            newPen.setWidthF(bounds.height());
        if (strokeColorChanged)
            newPen.setColor(localStrokeColor);
        p->setPen(newPen);
    }

    QPointF startPoint = bounds.location();
    startPoint.setY(startPoint.y() + bounds.height() / 2);
    QPointF endPoint = startPoint;
    endPoint.setX(endPoint.x() + bounds.width());

    p->drawLine(startPoint, endPoint);

    if (doubleLines) {
        // The space between double underlines is equal to the height of the underline
        // so distance between line centers is 2x height
        startPoint.setY(startPoint.y() + 2 * bounds.height());
        endPoint.setY(endPoint.y() + 2 * bounds.height());
        p->drawLine(startPoint, endPoint);
    }

    if (needSavePen)
        p->setPen(oldPen);

    p->setRenderHint(QPainter::Antialiasing, savedAntiAlias);
}

// NOTE: this code is based on GraphicsContextCG implementation
void GraphicsContext::drawLinesForText(const FloatPoint& origin, const DashArray& widths, bool printing, bool doubleLines)
{
    if (paintingDisabled())
        return;

    if (widths.size() <= 0)
        return;

    if (isRecording()) {
        m_displayListRecorder->drawLinesForText(origin, widths, printing, doubleLines, strokeThickness());
        return;
    }

    Color localStrokeColor(strokeColor());

    FloatRect bounds = computeLineBoundsAndAntialiasingModeForText(origin, widths.last(), printing, localStrokeColor);
    bool fillColorIsNotEqualToStrokeColor = fillColor() != localStrokeColor;

    // FIXME: drawRects() is significantly slower than drawLine() for thin lines (<= 1px)
    Vector<QRectF, 4> dashBounds;
    ASSERT(!(widths.size() % 2));
    dashBounds.reserveInitialCapacity(dashBounds.size() / 2);
    for (size_t i = 0; i < widths.size(); i += 2)
        dashBounds.append(QRectF(bounds.x() + widths[i], bounds.y(), widths[i+1] - widths[i], bounds.height()));

    if (doubleLines) {
        // The space between double underlines is equal to the height of the underline
        for (size_t i = 0; i < widths.size(); i += 2)
            dashBounds.append(QRectF(bounds.x() + widths[i], bounds.y() + 2 * bounds.height(), widths[i+1] - widths[i], bounds.height()));
    }

    QPainter* p = m_data->p();
    QPen oldPen = p->pen();
    p->setPen(Qt::NoPen);

    if (fillColorIsNotEqualToStrokeColor) {
        const QBrush oldBrush = p->brush();
        p->setBrush(QBrush(localStrokeColor));
        p->drawRects(dashBounds.data(), dashBounds.size());
        p->setBrush(oldBrush);
    } else {
        p->drawRects(dashBounds.data(), dashBounds.size());
    }

    p->setPen(oldPen);
}


/*
 *   NOTE: This code is completely based upon the one from
 *   Source/WebCore/platform/graphics/cairo/DrawErrorUnderline.{h|cpp}
 *
 *   Draws an error underline that looks like one of:
 *
 *               H       E                H
 *      /\      /\      /\        /\      /\               -
 *    A/  \    /  \    /  \     A/  \    /  \              |
 *     \   \  /    \  /   /D     \   \  /    \             |
 *      \   \/  C   \/   /        \   \/   C  \            | height = heightSquares * square
 *       \      /\  F   /          \  F   /\   \           |
 *        \    /  \    /            \    /  \   \G         |
 *         \  /    \  /              \  /    \  /          |
 *          \/      \/                \/      \/           -
 *          B                         B
 *          |---|
 *        unitWidth = (heightSquares - 1) * square
 *
 *  The x, y, width, height passed in give the desired bounding box;
 *  x/width are adjusted to make the underline a integer number of units wide.
*/
static void drawErrorUnderline(QPainter *painter, qreal x, qreal y, qreal width, qreal height)
{
    const qreal heightSquares = 2.5;

    qreal square = height / heightSquares;
    qreal halfSquare = 0.5 * square;

    qreal unitWidth = (heightSquares - 1.0) * square;
    int widthUnits = static_cast<int>((width + 0.5 * unitWidth) / unitWidth);

    x += 0.5 * (width - widthUnits * unitWidth);
    width = widthUnits * unitWidth;

    qreal bottom = y + height;
    qreal top = y;

    QPainterPath path;

    // Bottom of squiggle.
    path.moveTo(x - halfSquare, top + halfSquare); // A

    int i = 0;
    for (i = 0; i < widthUnits; i += 2) {
        qreal middle = x + (i + 1) * unitWidth;
        qreal right = x + (i + 2) * unitWidth;

        path.lineTo(middle, bottom); // B

        if (i + 2 == widthUnits)
            path.lineTo(right + halfSquare, top + halfSquare); // D
        else if (i + 1 != widthUnits)
            path.lineTo(right, top + square); // C
    }

    // Top of squiggle.
    for (i -= 2; i >= 0; i -= 2) {
        qreal left = x + i * unitWidth;
        qreal middle = x + (i + 1) * unitWidth;
        qreal right = x + (i + 2) * unitWidth;

        if (i + 1 == widthUnits)
            path.lineTo(middle + halfSquare, bottom - halfSquare); // G
        else {
            if (i + 2 == widthUnits)
                path.lineTo(right, top); // E

            path.lineTo(middle, bottom - halfSquare); // F
        }

        path.lineTo(left, top); // H
    }

    painter->drawPath(path);
}

void GraphicsContext::updateDocumentMarkerResources()
{
    // Unnecessary, since our document markers don't use resources.
}

void GraphicsContext::drawLineForDocumentMarker(const FloatPoint& origin, float width, DocumentMarkerLineStyle style)
{
    if (paintingDisabled())
        return;

    QPainter* painter = platformContext();
    const QPen originalPen = painter->pen();

    switch (style) {
    case DocumentMarkerSpellingLineStyle:
        painter->setPen(Qt::red);
        break;
    case DocumentMarkerGrammarLineStyle:
        painter->setPen(Qt::green);
        break;
    default:
        return;
    }

    drawErrorUnderline(painter, origin.x(), origin.y(), width, cMisspellingLineThickness);
    painter->setPen(originalPen);
}

FloatRect GraphicsContext::roundToDevicePixels(const FloatRect& frect, RoundingMode)
{
    // It is not enough just to round to pixels in device space. The rotation part of the
    // affine transform matrix to device space can mess with this conversion if we have a
    // rotating image like the hands of the world clock widget. We just need the scale, so
    // we get the affine transform matrix and extract the scale.
    QPainter* painter = platformContext();
    QTransform deviceTransform = painter->deviceTransform();
    if (deviceTransform.isIdentity())
        return frect;

    qreal deviceScaleX = sqrtf(deviceTransform.m11() * deviceTransform.m11() + deviceTransform.m12() * deviceTransform.m12());
    qreal deviceScaleY = sqrtf(deviceTransform.m21() * deviceTransform.m21() + deviceTransform.m22() * deviceTransform.m22());

    QPoint deviceOrigin(frect.x() * deviceScaleX, frect.y() * deviceScaleY);
    QPoint deviceLowerRight(frect.maxX() * deviceScaleX, frect.maxY() * deviceScaleY);

    // Don't let the height or width round to 0 unless either was originally 0
    if (deviceOrigin.y() == deviceLowerRight.y() && frect.height())
        deviceLowerRight.setY(deviceLowerRight.y() + 1);
    if (deviceOrigin.x() == deviceLowerRight.x() && frect.width())
        deviceLowerRight.setX(deviceLowerRight.x() + 1);

    FloatPoint roundedOrigin = FloatPoint(deviceOrigin.x() / deviceScaleX, deviceOrigin.y() / deviceScaleY);
    FloatPoint roundedLowerRight = FloatPoint(deviceLowerRight.x() / deviceScaleX, deviceLowerRight.y() / deviceScaleY);
    return FloatRect(roundedOrigin, roundedLowerRight - roundedOrigin);
}

void GraphicsContext::setPlatformShadow(const FloatSize& size, float, const Color&)
{
    // Qt doesn't support shadows natively, they are drawn manually in the draw*
    // functions
    if (m_state.shadowsIgnoreTransforms) {
        // Meaning that this graphics context is associated with a CanvasRenderingContext
        // We flip the height since CG and HTML5 Canvas have opposite Y axis
        m_state.shadowOffset = FloatSize(size.width(), -size.height());
    }
}

void GraphicsContext::clearPlatformShadow()
{
}

void GraphicsContext::pushTransparencyLayerInternal(const QRect &rect, qreal opacity, QPixmap& alphaMask)
{
    QPainter* p = m_data->p();

    QTransform deviceTransform = p->transform();
    QRect deviceClip = deviceTransform.mapRect(rect);

    alphaMask = alphaMask.transformed(deviceTransform);
    if (alphaMask.width() != deviceClip.width() || alphaMask.height() != deviceClip.height())
        alphaMask = alphaMask.scaled(deviceClip.width(), deviceClip.height());

    m_data->layers.push(new TransparencyLayer(p, deviceClip, 1.0, alphaMask));
}

void GraphicsContext::beginPlatformTransparencyLayer(float opacity)
{
    if (paintingDisabled())
        return;

    int x, y, w, h;
    x = y = 0;
    QPainter* p = m_data->p();
    const QPaintDevice* device = p->device();
    w = device->width();
    h = device->height();

    if (p->hasClipping()) {
        QRectF clip = m_data->clipBoundingRect();
        QRectF deviceClip = p->transform().mapRect(clip);
        x = int(qBound(qreal(0), deviceClip.x(), (qreal)w));
        y = int(qBound(qreal(0), deviceClip.y(), (qreal)h));
        w = int(qBound(qreal(0), deviceClip.width(), (qreal)w) + 2);
        h = int(qBound(qreal(0), deviceClip.height(), (qreal)h) + 2);
    }

    QPixmap emptyAlphaMask;
    m_data->layers.push(new TransparencyLayer(p, QRect(x, y, w, h), opacity, emptyAlphaMask));
    ++m_data->layerCount;
}

void GraphicsContext::endPlatformTransparencyLayer()
{
    if (paintingDisabled())
        return;

    TransparencyLayer* layer = m_data->layers.pop();
    if (!layer->alphaMask.isNull()) {
        layer->painter.resetTransform();
        layer->painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        layer->painter.drawPixmap(QPoint(), layer->alphaMask);
    } else
        --m_data->layerCount; // see the comment for layerCount
    layer->painter.end();

    QPainter* p = m_data->p();
    p->save();
    p->resetTransform();
    p->setOpacity(layer->opacity);
    p->drawPixmap(layer->offset, layer->pixmap);
    p->restore();

    delete layer;
}

bool GraphicsContext::supportsTransparencyLayers()
{
    return true;
}

void GraphicsContext::clearRect(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainter::CompositionMode currentCompositionMode = p->compositionMode();
    p->setCompositionMode(QPainter::CompositionMode_Source);
    p->fillRect(rect, Qt::transparent);
    p->setCompositionMode(currentCompositionMode);
}

void GraphicsContext::strokeRect(const FloatRect& rect, float lineWidth)
{
    if (paintingDisabled())
        return;

    Path path;
    path.addRect(rect);

    float previousStrokeThickness = strokeThickness();

    if (lineWidth != previousStrokeThickness)
        setStrokeThickness(lineWidth);

    strokePath(path);

    if (lineWidth != previousStrokeThickness)
        setStrokeThickness(previousStrokeThickness);
}

void GraphicsContext::setLineCap(LineCap lc)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setCapStyle(toQtLineCap(lc));
    p->setPen(nPen);
}

void GraphicsContext::setLineDash(const DashArray& dashes, float dashOffset)
{
    QPainter* p = m_data->p();
    QPen pen = p->pen();
    unsigned dashLength = dashes.size();
    if (dashLength) {
        QVector<qreal> pattern;
        unsigned count = dashLength;
        if (dashLength % 2)
            count *= 2;

        float penWidth = narrowPrecisionToFloat(double(pen.widthF()));
        if (penWidth <= 0.f)
            penWidth = 1.f;

        for (unsigned i = 0; i < count; i++)
            pattern.append(dashes[i % dashLength] / penWidth);

        pen.setDashPattern(pattern);
        pen.setDashOffset(dashOffset / penWidth);
    } else
        pen.setStyle(Qt::SolidLine);
    p->setPen(pen);
}

void GraphicsContext::setLineJoin(LineJoin lj)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setJoinStyle(toQtLineJoin(lj));
    p->setPen(nPen);
}

void GraphicsContext::setMiterLimit(float limit)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPen nPen = p->pen();
    nPen.setMiterLimit(limit);
    p->setPen(nPen);
}

void GraphicsContext::setPlatformAlpha(float opacity)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    p->setOpacity(opacity);
}

void GraphicsContext::setPlatformCompositeOperation(CompositeOperator op, BlendMode blendMode)
{
    if (paintingDisabled())
        return;

    ASSERT(op == WebCore::CompositeSourceOver || blendMode == WebCore::BlendModeNormal);

    if (op == WebCore::CompositeSourceOver)
        m_data->p()->setCompositionMode(toQtCompositionMode(blendMode));
    else
        m_data->p()->setCompositionMode(toQtCompositionMode(op));
}

void GraphicsContext::canvasClip(const Path& path, WindRule windRule)
{
    if (paintingDisabled())
        return;

    QPainterPath clipPath = path.platformPath();
    clipPath.setFillRule(toQtFillRule(windRule));
    m_data->p()->setClipPath(clipPath, Qt::IntersectClip);
}

void GraphicsContext::clipOut(const Path& path)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath clippedOut = path.platformPath();
    QPainterPath newClip;
    newClip.setFillRule(Qt::OddEvenFill);
    if (p->hasClipping()) {
        newClip.addRect(m_data->clipBoundingRect());
        newClip.addPath(clippedOut);
        p->setClipPath(newClip, Qt::IntersectClip);
    } else {
        QRect windowRect = p->transform().inverted().mapRect(p->window());
        newClip.addRect(windowRect);
        newClip.addPath(clippedOut.intersected(newClip));
        p->setClipPath(newClip);
    }
}

void GraphicsContext::translate(float x, float y)
{
    if (paintingDisabled())
        return;

    m_data->p()->translate(x, y);
}

void GraphicsContext::rotate(float radians)
{
    if (paintingDisabled())
        return;

    QTransform rotation = QTransform().rotateRadians(radians);
    m_data->p()->setTransform(rotation, true);
}

void GraphicsContext::scale(const FloatSize& s)
{
    if (paintingDisabled())
        return;

    m_data->p()->scale(s.width(), s.height());
}

void GraphicsContext::clipOut(const FloatRect& rect)
{
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    QPainterPath newClip;
    newClip.setFillRule(Qt::OddEvenFill);
    if (p->hasClipping()) {
        newClip.addRect(m_data->clipBoundingRect());
        newClip.addRect(QRectF(rect));
        p->setClipPath(newClip, Qt::IntersectClip);
    } else {
        QRectF clipOutRect(rect);
        QRect window = p->transform().inverted().mapRect(p->window());
        clipOutRect &= window;
        newClip.addRect(window);
        newClip.addRect(clipOutRect);
        p->setClipPath(newClip);
    }
}

void GraphicsContext::concatCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, true);
}

void GraphicsContext::setCTM(const AffineTransform& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform);
}

#if ENABLE(3D_TRANSFORMS)
TransformationMatrix GraphicsContext::get3DTransform() const
{
    if (paintingDisabled())
        return TransformationMatrix();

    return platformContext()->worldTransform();
}

void GraphicsContext::concat3DTransform(const TransformationMatrix& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, true);
}

void GraphicsContext::set3DTransform(const TransformationMatrix& transform)
{
    if (paintingDisabled())
        return;

    m_data->p()->setWorldTransform(transform, false);
}
#endif

void GraphicsContext::setURLForRect(const URL& url, const IntRect& rect)
{
#if QT_VERSION >= QT_VERSION_CHECK(5, 6, 0)
    if (paintingDisabled())
        return;

    QPainter* p = m_data->p();
    if (p->paintEngine()->type() == QPaintEngine::Pdf)
        static_cast<QPdfEngine *>(p->paintEngine())->drawHyperlink(p->worldTransform().mapRect(rect), url);
#else
    UNUSED_PARAM(url);
    UNUSED_PARAM(rect);
#endif
}

void GraphicsContext::setPlatformStrokeColor(const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    m_data->solidColor.setColor(color);
    newPen.setBrush(m_data->solidColor);
    p->setPen(newPen);
}

void GraphicsContext::setPlatformStrokeStyle(StrokeStyle strokeStyle)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    newPen.setStyle(toQPenStyle(strokeStyle));
    p->setPen(newPen);
}

void GraphicsContext::setPlatformStrokeThickness(float thickness)
{
    if (paintingDisabled())
        return;
    QPainter* p = m_data->p();
    QPen newPen(p->pen());
    newPen.setWidthF(thickness);
    p->setPen(newPen);
}

void GraphicsContext::setPlatformFillColor(const Color& color)
{
    if (paintingDisabled() || !color.isValid())
        return;

    m_data->solidColor.setColor(color);
    m_data->p()->setBrush(m_data->solidColor);
}

void GraphicsContext::setPlatformShouldAntialias(bool enable)
{
    if (paintingDisabled())
        return;
    m_data->p()->setRenderHint(QPainter::Antialiasing, enable);
}

#if OS(WINDOWS)

HDC GraphicsContext::getWindowsContext(const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    Q_ASSERT(mayCreateBitmap);

    if (dstRect.isEmpty())
        return 0;

    // Create a bitmap DC in which to draw.
    BITMAPINFO bitmapInfo;
    bitmapInfo.bmiHeader.biSize          = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth         = dstRect.width();
    bitmapInfo.bmiHeader.biHeight        = dstRect.height();
    bitmapInfo.bmiHeader.biPlanes        = 1;
    bitmapInfo.bmiHeader.biBitCount      = 32;
    bitmapInfo.bmiHeader.biCompression   = BI_RGB;
    bitmapInfo.bmiHeader.biSizeImage     = 0;
    bitmapInfo.bmiHeader.biXPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biYPelsPerMeter = 0;
    bitmapInfo.bmiHeader.biClrUsed       = 0;
    bitmapInfo.bmiHeader.biClrImportant  = 0;

    void* pixels = 0;
    HBITMAP bitmap = ::CreateDIBSection(0, &bitmapInfo, DIB_RGB_COLORS, &pixels, 0, 0);
    if (!bitmap)
        return 0;

    HDC displayDC = ::GetDC(0);
    HDC bitmapDC = ::CreateCompatibleDC(displayDC);
    ::ReleaseDC(0, displayDC);

    ::SelectObject(bitmapDC, bitmap);

    // Fill our buffer with clear if we're going to alpha blend.
    if (supportAlphaBlend) {
        BITMAP bmpInfo;
        GetObject(bitmap, sizeof(bmpInfo), &bmpInfo);
        int bufferSize = bmpInfo.bmWidthBytes * bmpInfo.bmHeight;
        memset(bmpInfo.bmBits, 0, bufferSize);
    }

#if !OS(WINCE)
    // Make sure we can do world transforms.
    SetGraphicsMode(bitmapDC, GM_ADVANCED);

    // Apply a translation to our context so that the drawing done will be at (0,0) of the bitmap.
    XFORM xform;
    xform.eM11 = 1.0f;
    xform.eM12 = 0.0f;
    xform.eM21 = 0.0f;
    xform.eM22 = 1.0f;
    xform.eDx = -dstRect.x();
    xform.eDy = -dstRect.y();
    ::SetWorldTransform(bitmapDC, &xform);
#endif

    return bitmapDC;
}

void GraphicsContext::releaseWindowsContext(HDC hdc, const IntRect& dstRect, bool supportAlphaBlend, bool mayCreateBitmap)
{
    // painting through native HDC is only supported for plugin, where mayCreateBitmap is always true
    Q_ASSERT(mayCreateBitmap);

    if (hdc) {

        if (!dstRect.isEmpty()) {

            HBITMAP bitmap = static_cast<HBITMAP>(GetCurrentObject(hdc, OBJ_BITMAP));
            BITMAP info;
            GetObject(bitmap, sizeof(info), &info);
            ASSERT(info.bmBitsPixel == 32);

            QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap, supportAlphaBlend ? HBitmapPremultipliedAlpha : HBitmapNoAlpha);
            m_data->p()->drawPixmap(dstRect, pixmap);

            ::DeleteObject(bitmap);
        }

        ::DeleteDC(hdc);
    }
}
#endif

void GraphicsContext::setPlatformImageInterpolationQuality(InterpolationQuality quality)
{
    // FIXME
    m_data->imageInterpolationQuality = quality;

    switch (quality) {
    case InterpolationNone:
    case InterpolationLow:
        // use nearest-neigbor
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, false);
        break;

    case InterpolationMedium:
    case InterpolationHigh:
        // use the filter
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, true);
        break;

    case InterpolationDefault:
    default:
        m_data->p()->setRenderHint(QPainter::SmoothPixmapTransform, m_data->initialSmoothPixmapTransformHint);
        break;
    };
}

void GraphicsContext::takeOwnershipOfPlatformContext()
{
    m_data->takeOwnershipOfPlatformContext();
}

bool GraphicsContext::isAcceleratedContext() const
{
    return (platformContext()->paintEngine()->type() == QPaintEngine::OpenGL2);
}

}
// vim: ts=4 sw=4 et
