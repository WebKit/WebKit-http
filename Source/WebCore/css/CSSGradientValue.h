/*
 * Copyright (C) 2008 Apple Inc. All rights reserved.
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

#ifndef CSSGradientValue_h
#define CSSGradientValue_h

#include "CSSImageGeneratorValue.h"
#include "CSSPrimitiveValue.h"
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class Gradient;

enum CSSGradientType { CSSLinearGradient, CSSRadialGradient };
enum CSSGradientRepeat { NonRepeating, Repeating };

struct CSSGradientColorStop {
    CSSGradientColorStop() : m_colorIsDerivedFromElement(false) { };
    RefPtr<CSSPrimitiveValue> m_position; // percentage or length
    RefPtr<CSSPrimitiveValue> m_color;
    Color m_resolvedColor;
    bool m_colorIsDerivedFromElement;
    void reportMemoryUsage(MemoryObjectInfo*) const;
};

class CSSGradientValue : public CSSImageGeneratorValue {
public:
    PassRefPtr<Image> image(RenderObject*, const IntSize&);

    void setFirstX(PassRefPtr<CSSPrimitiveValue> val) { m_firstX = val; }
    void setFirstY(PassRefPtr<CSSPrimitiveValue> val) { m_firstY = val; }
    void setSecondX(PassRefPtr<CSSPrimitiveValue> val) { m_secondX = val; }
    void setSecondY(PassRefPtr<CSSPrimitiveValue> val) { m_secondY = val; }

    void addStop(const CSSGradientColorStop& stop) { m_stops.append(stop); }

    Vector<CSSGradientColorStop>& stops() { return m_stops; }

    void sortStopsIfNeeded();

    bool isLinearGradient() const { return classType() == LinearGradientClass; }
    bool isRadialGradient() const { return classType() == RadialGradientClass; }

    bool isRepeating() const { return m_repeating; }

    bool deprecatedType() const { return m_deprecatedType; } // came from -webkit-gradient

    bool isFixedSize() const { return false; }
    IntSize fixedSize(const RenderObject*) const { return IntSize(); }

    bool isPending() const { return false; }
    void loadSubimages(CachedResourceLoader*) { }
    PassRefPtr<CSSGradientValue> gradientWithStylesResolved(StyleResolver*);

protected:
    CSSGradientValue(ClassType classType, CSSGradientRepeat repeat, bool deprecatedType = false)
        : CSSImageGeneratorValue(classType)
        , m_stopsSorted(false)
        , m_deprecatedType(deprecatedType)
        , m_repeating(repeat == Repeating)
    {
    }

    CSSGradientValue(const CSSGradientValue& other, ClassType classType, bool deprecatedType = false)
        : CSSImageGeneratorValue(classType)
        , m_firstX(other.m_firstX)
        , m_firstY(other.m_firstY)
        , m_secondX(other.m_secondX)
        , m_secondY(other.m_secondY)
        , m_stops(other.m_stops)
        , m_stopsSorted(other.m_stopsSorted)
        , m_deprecatedType(deprecatedType)
        , m_repeating(other.isRepeating() ? Repeating : NonRepeating)
    {
    }

    void addStops(Gradient*, RenderObject*, RenderStyle* rootStyle, float maxLengthForRepeat = 0);

    // Resolve points/radii to front end values.
    FloatPoint computeEndPoint(CSSPrimitiveValue*, CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, const IntSize&);

    bool isCacheable() const;

    void reportBaseClassMemoryUsage(MemoryObjectInfo*) const;

    // Points. Some of these may be null for linear gradients.
    RefPtr<CSSPrimitiveValue> m_firstX;
    RefPtr<CSSPrimitiveValue> m_firstY;

    RefPtr<CSSPrimitiveValue> m_secondX;
    RefPtr<CSSPrimitiveValue> m_secondY;

    // Stops
    Vector<CSSGradientColorStop> m_stops;
    bool m_stopsSorted;
    bool m_deprecatedType; // -webkit-gradient()
    bool m_repeating;
};


class CSSLinearGradientValue : public CSSGradientValue {
public:
    static PassRefPtr<CSSLinearGradientValue> create(CSSGradientRepeat repeat, bool deprecatedType = false)
    {
        return adoptRef(new CSSLinearGradientValue(repeat, deprecatedType));
    }

    void setAngle(PassRefPtr<CSSPrimitiveValue> val) { m_angle = val; }

    String customCssText() const;

    // Create the gradient for a given size.
    PassRefPtr<Gradient> createGradient(RenderObject*, const IntSize&);

    PassRefPtr<CSSLinearGradientValue> clone() const
    {
        return adoptRef(new CSSLinearGradientValue(*this));
    }

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    CSSLinearGradientValue(CSSGradientRepeat repeat, bool deprecatedType = false)
        : CSSGradientValue(LinearGradientClass, repeat, deprecatedType)
    {
    }

    CSSLinearGradientValue(const CSSLinearGradientValue& other)
        : CSSGradientValue(other, LinearGradientClass, other.deprecatedType())
        , m_angle(other.m_angle)
    {
    }

    RefPtr<CSSPrimitiveValue> m_angle; // may be null.
};

class CSSRadialGradientValue : public CSSGradientValue {
public:
    static PassRefPtr<CSSRadialGradientValue> create(CSSGradientRepeat repeat, bool deprecatedType = false)
    {
        return adoptRef(new CSSRadialGradientValue(repeat, deprecatedType));
    }

    PassRefPtr<CSSRadialGradientValue> clone() const
    {
        return adoptRef(new CSSRadialGradientValue(*this));
    }

    String customCssText() const;

    void setFirstRadius(PassRefPtr<CSSPrimitiveValue> val) { m_firstRadius = val; }
    void setSecondRadius(PassRefPtr<CSSPrimitiveValue> val) { m_secondRadius = val; }

    void setShape(PassRefPtr<CSSPrimitiveValue> val) { m_shape = val; }
    void setSizingBehavior(PassRefPtr<CSSPrimitiveValue> val) { m_sizingBehavior = val; }

    void setEndHorizontalSize(PassRefPtr<CSSPrimitiveValue> val) { m_endHorizontalSize = val; }
    void setEndVerticalSize(PassRefPtr<CSSPrimitiveValue> val) { m_endVerticalSize = val; }

    // Create the gradient for a given size.
    PassRefPtr<Gradient> createGradient(RenderObject*, const IntSize&);

    void reportDescendantMemoryUsage(MemoryObjectInfo*) const;

private:
    CSSRadialGradientValue(CSSGradientRepeat repeat, bool deprecatedType = false)
        : CSSGradientValue(RadialGradientClass, repeat, deprecatedType)
    {
    }

    CSSRadialGradientValue(const CSSRadialGradientValue& other)
        : CSSGradientValue(other, RadialGradientClass, other.deprecatedType())
        , m_firstRadius(other.m_firstRadius)
        , m_secondRadius(other.m_secondRadius)
        , m_shape(other.m_shape)
        , m_sizingBehavior(other.m_sizingBehavior)
        , m_endHorizontalSize(other.m_endHorizontalSize)
        , m_endVerticalSize(other.m_endVerticalSize)
    {
    }


    // Resolve points/radii to front end values.
    float resolveRadius(CSSPrimitiveValue*, RenderStyle*, RenderStyle* rootStyle, float* widthOrHeight = 0);

    // These may be null for non-deprecated gradients.
    RefPtr<CSSPrimitiveValue> m_firstRadius;
    RefPtr<CSSPrimitiveValue> m_secondRadius;

    // The below are only used for non-deprecated gradients.
    RefPtr<CSSPrimitiveValue> m_shape;
    RefPtr<CSSPrimitiveValue> m_sizingBehavior;

    RefPtr<CSSPrimitiveValue> m_endHorizontalSize;
    RefPtr<CSSPrimitiveValue> m_endVerticalSize;
};

} // namespace WebCore

#endif // CSSGradientValue_h
