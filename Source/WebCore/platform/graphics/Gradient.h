/*
 * Copyright (C) 2006, 2007, 2008, 2011 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alp Toker <alp@atoker.com>
 * Copyright (C) 2008 Torch Mobile, Inc.
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

#ifndef Gradient_h
#define Gradient_h

#include "AffineTransform.h"
#include "FloatPoint.h"
#include "Generator.h"
#include "GraphicsTypes.h"
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

#if USE(CG)

typedef struct CGContext* CGContextRef;

#define USE_CG_SHADING defined(BUILDING_ON_LEOPARD)

#if USE_CG_SHADING
typedef struct CGShading* CGShadingRef;
typedef CGShadingRef PlatformGradient;
#else
typedef struct CGGradient* CGGradientRef;
typedef CGGradientRef PlatformGradient;
#endif

#elif PLATFORM(QT)
QT_BEGIN_NAMESPACE
class QGradient;
QT_END_NAMESPACE
typedef QGradient* PlatformGradient;
#elif USE(CAIRO)
typedef struct _cairo_pattern cairo_pattern_t;
typedef cairo_pattern_t* PlatformGradient;
#elif USE(SKIA)
class SkShader;
typedef class SkShader* PlatformGradient;
typedef class SkShader* PlatformPattern;
#else
typedef void* PlatformGradient;
#endif

namespace WebCore {

    class Color;

    class Gradient : public Generator {
    public:
        static PassRefPtr<Gradient> create(const FloatPoint& p0, const FloatPoint& p1)
        {
            return adoptRef(new Gradient(p0, p1));
        }
        static PassRefPtr<Gradient> create(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1, float aspectRatio = 1)
        {
            return adoptRef(new Gradient(p0, r0, p1, r1, aspectRatio));
        }
        virtual ~Gradient();

        struct ColorStop;
        void addColorStop(const ColorStop&);
        void addColorStop(float, const Color&);

        void getColor(float value, float* r, float* g, float* b, float* a) const;
        bool hasAlpha() const;

        bool isRadial() const { return m_radial; }
        bool isZeroSize() const { return m_p0.x() == m_p1.x() && m_p0.y() == m_p1.y() && (!m_radial || m_r0 == m_r1); }

        const FloatPoint& p0() const { return m_p0; }
        const FloatPoint& p1() const { return m_p1; }

        void setP0(const FloatPoint& p) { m_p0 = p; }
        void setP1(const FloatPoint& p) { m_p1 = p; }

        float startRadius() const { return m_r0; }
        float endRadius() const { return m_r1; }

        void setStartRadius(float r) { m_r0 = r; }
        void setEndRadius(float r) { m_r1 = r; }
        
        float aspectRatio() const { return m_aspectRatio; }

#if OS(WINCE) && !PLATFORM(QT)
        const Vector<ColorStop, 2>& getStops() const;
#else
        PlatformGradient platformGradient();
#endif

        struct ColorStop {
            float stop;
            float red;
            float green;
            float blue;
            float alpha;

            ColorStop() : stop(0), red(0), green(0), blue(0), alpha(0) { }
            ColorStop(float s, float r, float g, float b, float a) : stop(s), red(r), green(g), blue(b), alpha(a) { }
        };

        void setStopsSorted(bool s) { m_stopsSorted = s; }
        
        void setSpreadMethod(GradientSpreadMethod);
        GradientSpreadMethod spreadMethod() { return m_spreadMethod; }
        void setGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation);
        // Qt and CG transform the gradient at draw time
        AffineTransform gradientSpaceTransform() { return m_gradientSpaceTransformation; }

        virtual void fill(GraphicsContext*, const FloatRect&);
        virtual void adjustParametersForTiledDrawing(IntSize& size, FloatRect& srcRect);

        void setPlatformGradientSpaceTransform(const AffineTransform& gradientSpaceTransformation);

#if USE(CG)
        void paint(CGContextRef);
        void paint(GraphicsContext*);
#endif

    private:
        Gradient(const FloatPoint& p0, const FloatPoint& p1);
        Gradient(const FloatPoint& p0, float r0, const FloatPoint& p1, float r1, float aspectRatio);

        void platformInit() { m_gradient = 0; }
        void platformDestroy();

        int findStop(float value) const;
        void sortStopsIfNecessary();

        bool m_radial;
        FloatPoint m_p0;
        FloatPoint m_p1;
        float m_r0;
        float m_r1;
        float m_aspectRatio; // For elliptical gradient, width / height.
        mutable Vector<ColorStop, 2> m_stops;
        mutable bool m_stopsSorted;
        mutable int m_lastStop;
        GradientSpreadMethod m_spreadMethod;
        AffineTransform m_gradientSpaceTransformation;

        PlatformGradient m_gradient;
    };

} //namespace

#endif
