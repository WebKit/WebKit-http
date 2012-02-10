/*
 * Copyright (C) 2007, 2008, 2009 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer. 
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution. 
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission. 
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "AnimationBase.h"

#include "AnimationControllerPrivate.h"
#include "CSSCrossfadeValue.h"
#include "CSSImageGeneratorValue.h"
#include "CSSImageValue.h"
#include "CSSPrimitiveValue.h"
#include "CSSPropertyLonghand.h"
#include "CSSPropertyNames.h"
#include "CompositeAnimation.h"
#include "Document.h"
#include "EventNames.h"
#include "FloatConversion.h"
#include "IdentityTransformOperation.h"
#include "MatrixTransformOperation.h"
#include "Matrix3DTransformOperation.h"
#include "RenderBox.h"
#include "RenderLayer.h"
#include "RenderLayerBacking.h"
#include "RenderStyle.h"
#include "StyleCachedImage.h"
#include "StyleGeneratedImage.h"
#include "UnitBezier.h"
#include <algorithm>
#include <wtf/CurrentTime.h>

using namespace std;

namespace WebCore {

// The epsilon value we pass to UnitBezier::solve given that the animation is going to run over |dur| seconds. The longer the
// animation, the more precision we need in the timing function result to avoid ugly discontinuities.
static inline double solveEpsilon(double duration)
{
    return 1.0 / (200.0 * duration);
}

static inline double solveCubicBezierFunction(double p1x, double p1y, double p2x, double p2y, double t, double duration)
{
    // Convert from input time to parametric value in curve, then from
    // that to output time.
    UnitBezier bezier(p1x, p1y, p2x, p2y);
    return bezier.solve(t, solveEpsilon(duration));
}

static inline double solveStepsFunction(int numSteps, bool stepAtStart, double t)
{
    if (stepAtStart)
        return min(1.0, (floor(numSteps * t) + 1) / numSteps);
    return floor(numSteps * t) / numSteps;
}

static inline int blendFunc(const AnimationBase*, int from, int to, double progress)
{  
    return blend(from, to, progress);
}

static inline unsigned blendFunc(const AnimationBase*, unsigned from, unsigned to, double progress)
{
    return blend(from, to, progress);
}

static inline double blendFunc(const AnimationBase*, double from, double to, double progress)
{  
    return blend(from, to, progress);
}

static inline float blendFunc(const AnimationBase*, float from, float to, double progress)
{  
    return narrowPrecisionToFloat(from + (to - from) * progress);
}

static inline Color blendFunc(const AnimationBase*, const Color& from, const Color& to, double progress)
{
    return blend(from, to, progress);
}

static inline Length blendFunc(const AnimationBase*, const Length& from, const Length& to, double progress)
{  
    return to.blend(from, narrowPrecisionToFloat(progress));
}

static inline LengthSize blendFunc(const AnimationBase* anim, const LengthSize& from, const LengthSize& to, double progress)
{  
    return LengthSize(blendFunc(anim, from.width(), to.width(), progress),
                      blendFunc(anim, from.height(), to.height(), progress));
}

static inline IntSize blendFunc(const AnimationBase* anim, const IntSize& from, const IntSize& to, double progress)
{  
    return IntSize(blendFunc(anim, from.width(), to.width(), progress),
                   blendFunc(anim, from.height(), to.height(), progress));
}

static inline ShadowStyle blendFunc(const AnimationBase* anim, ShadowStyle from, ShadowStyle to, double progress)
{
    if (from == to)
        return to;

    double fromVal = from == Normal ? 1 : 0;
    double toVal = to == Normal ? 1 : 0;
    double result = blendFunc(anim, fromVal, toVal, progress);
    return result > 0 ? Normal : Inset;
}

static inline PassOwnPtr<ShadowData> blendFunc(const AnimationBase* anim, const ShadowData* from, const ShadowData* to, double progress)
{  
    ASSERT(from && to);
    if (from->style() != to->style())
        return adoptPtr(new ShadowData(*to));

    return adoptPtr(new ShadowData(blend(from->x(), to->x(), progress),
                                   blend(from->y(), to->y(), progress), 
                                   blend(from->blur(), to->blur(), progress),
                                   blend(from->spread(), to->spread(), progress),
                                   blendFunc(anim, from->style(), to->style(), progress),
                                   from->isWebkitBoxShadow(),
                                   blend(from->color(), to->color(), progress)));
}

static inline TransformOperations blendFunc(const AnimationBase* anim, const TransformOperations& from, const TransformOperations& to, double progress)
{    
    TransformOperations result;

    // If we have a transform function list, use that to do a per-function animation. Otherwise do a Matrix animation
    if (anim->isTransformFunctionListValid()) {
        unsigned fromSize = from.operations().size();
        unsigned toSize = to.operations().size();
        unsigned size = max(fromSize, toSize);
        for (unsigned i = 0; i < size; i++) {
            RefPtr<TransformOperation> fromOp = (i < fromSize) ? from.operations()[i].get() : 0;
            RefPtr<TransformOperation> toOp = (i < toSize) ? to.operations()[i].get() : 0;
            RefPtr<TransformOperation> blendedOp = toOp ? toOp->blend(fromOp.get(), progress) : (fromOp ? fromOp->blend(0, progress, true) : PassRefPtr<TransformOperation>(0));
            if (blendedOp)
                result.operations().append(blendedOp);
            else {
                RefPtr<TransformOperation> identityOp = IdentityTransformOperation::create();
                if (progress > 0.5)
                    result.operations().append(toOp ? toOp : identityOp);
                else
                    result.operations().append(fromOp ? fromOp : identityOp);
            }
        }
    } else {
        // Convert the TransformOperations into matrices
        LayoutSize size = anim->renderer()->isBox() ? toRenderBox(anim->renderer())->borderBoxRect().size() : LayoutSize();
        TransformationMatrix fromT;
        TransformationMatrix toT;
        from.apply(size, fromT);
        to.apply(size, toT);
        
        toT.blend(fromT, progress);
        
        // Append the result
        result.operations().append(Matrix3DTransformOperation::create(toT));
    }
    return result;
}

#if ENABLE(CSS_FILTERS)
static inline FilterOperations blendFunc(const AnimationBase* anim, const FilterOperations& from, const FilterOperations& to, double progress)
{    
    FilterOperations result;

    // If we have a filter function list, use that to do a per-function animation.
    if (anim->filterFunctionListsMatch()) {
        size_t fromSize = from.operations().size();
        size_t toSize = to.operations().size();
        size_t size = max(fromSize, toSize);
        for (size_t i = 0; i < size; i++) {
            RefPtr<FilterOperation> fromOp = (i < fromSize) ? from.operations()[i].get() : 0;
            RefPtr<FilterOperation> toOp = (i < toSize) ? to.operations()[i].get() : 0;
            RefPtr<FilterOperation> blendedOp = toOp ? toOp->blend(fromOp.get(), progress) : (fromOp ? fromOp->blend(0, progress, true) : PassRefPtr<FilterOperation>(0));
            if (blendedOp)
                result.operations().append(blendedOp);
            else {
                RefPtr<FilterOperation> identityOp = PassthroughFilterOperation::create();
                if (progress > 0.5)
                    result.operations().append(toOp ? toOp : identityOp);
                else
                    result.operations().append(fromOp ? fromOp : identityOp);
            }
        }
    } else {
        // If the filter function lists don't match, we could try to cross-fade, but don't yet have a way to represent that in CSS.
        // For now we'll just fail to animate.
        result = to;
    }

    return result;
}
#endif // ENABLE(CSS_FILTERS)

static inline EVisibility blendFunc(const AnimationBase* anim, EVisibility from, EVisibility to, double progress)
{
    // Any non-zero result means we consider the object to be visible.  Only at 0 do we consider the object to be
    // invisible.   The invisible value we use (HIDDEN vs. COLLAPSE) depends on the specified from/to values.
    double fromVal = from == VISIBLE ? 1. : 0.;
    double toVal = to == VISIBLE ? 1. : 0.;
    if (fromVal == toVal)
        return to;
    double result = blendFunc(anim, fromVal, toVal, progress);
    return result > 0. ? VISIBLE : (to != VISIBLE ? to : from);
}

static inline LengthBox blendFunc(const AnimationBase* anim, const LengthBox& from, const LengthBox& to, double progress)
{
    // Length types have to match to animate
    if (from.top().type() != to.top().type()
        || from.right().type() != to.right().type()
        || from.bottom().type() != to.bottom().type()
        || from.left().type() != to.left().type())
        return to;
    
    LengthBox result(blendFunc(anim, from.top(), to.top(), progress),
                     blendFunc(anim, from.right(), to.right(), progress),
                     blendFunc(anim, from.bottom(), to.bottom(), progress),
                     blendFunc(anim, from.left(), to.left(), progress));
    return result;
}

#if ENABLE(SVG)
static inline SVGLength blendFunc(const AnimationBase*, const SVGLength& from, const SVGLength& to, double progress)
{  
    return to.blend(from, narrowPrecisionToFloat(progress));
}
#endif

static inline PassRefPtr<StyleImage> crossfadeBlend(const AnimationBase*, StyleCachedImage* fromStyleImage, StyleCachedImage* toStyleImage, double progress)
{
    // If progress is at one of the extremes, we want getComputedStyle to show the image,
    // not a completed cross-fade, so we hand back one of the existing images.
    if (!progress)
        return fromStyleImage;
    if (progress == 1)
        return toStyleImage;

    CachedImage* fromCachedImage = static_cast<CachedImage*>(fromStyleImage->data());
    CachedImage* toCachedImage = static_cast<CachedImage*>(toStyleImage->data());

    RefPtr<CSSImageValue> fromImageValue = CSSImageValue::create(fromCachedImage->url(), fromStyleImage);
    RefPtr<CSSImageValue> toImageValue = CSSImageValue::create(toCachedImage->url(), toStyleImage);
    RefPtr<CSSCrossfadeValue> crossfadeValue = CSSCrossfadeValue::create(fromImageValue, toImageValue);

    crossfadeValue->setPercentage(CSSPrimitiveValue::create(progress, CSSPrimitiveValue::CSS_NUMBER));

    return StyleGeneratedImage::create(crossfadeValue.get());
}

static inline PassRefPtr<StyleImage> blendFunc(const AnimationBase* anim, StyleImage* from, StyleImage* to, double progress)
{
    if (!from || !to)
        return to;

    if (from->isCachedImage() && to->isCachedImage())
        return crossfadeBlend(anim, static_cast<StyleCachedImage*>(from), static_cast<StyleCachedImage*>(to), progress);

    // FIXME: Support transitioning generated images as well. (gradients, etc.)

    return to;
}

static inline NinePieceImage blendFunc(const AnimationBase* anim, const NinePieceImage& from, const NinePieceImage& to, double progress)
{
    if (!from.hasImage() || !to.hasImage())
        return to;

    // FIXME (74112): Support transitioning between NinePieceImages that differ by more than image content.

    if (from.imageSlices() != to.imageSlices() || from.borderSlices() != to.borderSlices() || from.outset() != to.outset() || from.fill() != to.fill() || from.horizontalRule() != to.horizontalRule() || from.verticalRule() != to.verticalRule())
        return to;

    if (from.image()->imageSize(anim->renderer(), 1.0) != to.image()->imageSize(anim->renderer(), 1.0))
        return to;

    RefPtr<StyleImage> newContentImage = blendFunc(anim, from.image(), to.image(), progress);

    return NinePieceImage(newContentImage, from.imageSlices(), from.fill(), from.borderSlices(), from.outset(), from.horizontalRule(), from.verticalRule());
}

class PropertyWrapperBase;

static void addShorthandProperties();
static PropertyWrapperBase* wrapperForProperty(int propertyID);

class PropertyWrapperBase {
    WTF_MAKE_NONCOPYABLE(PropertyWrapperBase); WTF_MAKE_FAST_ALLOCATED;
public:
    PropertyWrapperBase(int prop)
        : m_prop(prop)
    {
    }

    virtual ~PropertyWrapperBase() { }
    
    virtual bool isShorthandWrapper() const { return false; }
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const = 0;
    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const = 0;

    int property() const { return m_prop; }

#if USE(ACCELERATED_COMPOSITING)
    virtual bool animationIsAccelerated() const { return false; }
#endif

private:
    int m_prop;
};

template <typename T>
class PropertyWrapperGetter : public PropertyWrapperBase {
public:
    PropertyWrapperGetter(int prop, T (RenderStyle::*getter)() const)
        : PropertyWrapperBase(prop)
        , m_getter(getter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if ((!a && !b) || a == b)
           return true;
       if (!a || !b)
            return false;
        return (a->*m_getter)() == (b->*m_getter)();
    }

protected:
    T (RenderStyle::*m_getter)() const;
};

template <typename T>
class PropertyWrapper : public PropertyWrapperGetter<T> {
public:
    PropertyWrapper(int prop, T (RenderStyle::*getter)() const, void (RenderStyle::*setter)(T))
        : PropertyWrapperGetter<T>(prop, getter)
        , m_setter(setter)
    {
    }
    
    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*PropertyWrapperGetter<T>::m_getter)(), (b->*PropertyWrapperGetter<T>::m_getter)(), progress));
    }

protected:
    void (RenderStyle::*m_setter)(T);
};

template <typename T>
class RefCountedPropertyWrapper : public PropertyWrapperGetter<T*> {
public:
    RefCountedPropertyWrapper(int prop, T* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<T>))
        : PropertyWrapperGetter<T*>(prop, getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*PropertyWrapperGetter<T*>::m_getter)(), (b->*PropertyWrapperGetter<T*>::m_getter)(), progress));
    }

protected:
    void (RenderStyle::*m_setter)(PassRefPtr<T>);
};

class StyleImagePropertyWrapper : public RefCountedPropertyWrapper<StyleImage> {
public:
    StyleImagePropertyWrapper(int prop, StyleImage* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassRefPtr<StyleImage>))
        : RefCountedPropertyWrapper<StyleImage>(prop, getter, setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if (a == b)
           return true;
       if (!a || !b)
            return false;
            
        StyleImage* imageA = (a->*m_getter)();
        StyleImage* imageB = (b->*m_getter)();
        return StyleImage::imagesEquivalent(imageA, imageB);
    }
};

#if USE(ACCELERATED_COMPOSITING)
class PropertyWrapperAcceleratedOpacity : public PropertyWrapper<float> {
public:
    PropertyWrapperAcceleratedOpacity()
        : PropertyWrapper<float>(CSSPropertyOpacity, &RenderStyle::opacity, &RenderStyle::setOpacity)
    {
    }

    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        float fromOpacity = a->opacity();
        
        // This makes sure we put the object being animated into a RenderLayer during the animation
        dst->setOpacity(blendFunc(anim, (fromOpacity == 1) ? 0.999999f : fromOpacity, b->opacity(), progress));
    }
};

class PropertyWrapperAcceleratedTransform : public PropertyWrapper<const TransformOperations&> {
public:
    PropertyWrapperAcceleratedTransform()
        : PropertyWrapper<const TransformOperations&>(CSSPropertyWebkitTransform, &RenderStyle::transform, &RenderStyle::setTransform)
    {
    }
    
    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        dst->setTransform(blendFunc(anim, a->transform(), b->transform(), progress));
    }
};

#if ENABLE(CSS_FILTERS)
class PropertyWrapperAcceleratedFilter : public PropertyWrapper<const FilterOperations&> {
public:
    PropertyWrapperAcceleratedFilter()
        : PropertyWrapper<const FilterOperations&>(CSSPropertyWebkitFilter, &RenderStyle::filter, &RenderStyle::setFilter)
    {
    }
    
    virtual bool animationIsAccelerated() const { return true; }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        dst->setFilter(blendFunc(anim, a->filter(), b->filter(), progress));
    }
};
#endif
#endif // USE(ACCELERATED_COMPOSITING)

static inline size_t shadowListLength(const ShadowData* shadow)
{
    size_t count;
    for (count = 0; shadow; shadow = shadow->next())
        ++count;
    return count;
}

static inline const ShadowData* shadowForBlending(const ShadowData* srcShadow, const ShadowData* otherShadow)
{
    DEFINE_STATIC_LOCAL(ShadowData, defaultShadowData, (0, 0, 0, 0, Normal, false, Color::transparent));
    DEFINE_STATIC_LOCAL(ShadowData, defaultInsetShadowData, (0, 0, 0, 0, Inset, false, Color::transparent));

    DEFINE_STATIC_LOCAL(ShadowData, defaultWebKitBoxShadowData, (0, 0, 0, 0, Normal, true, Color::transparent));
    DEFINE_STATIC_LOCAL(ShadowData, defaultInsetWebKitBoxShadowData, (0, 0, 0, 0, Inset, true, Color::transparent));

    if (srcShadow)
        return srcShadow;

    if (otherShadow->style() == Inset)
        return otherShadow->isWebkitBoxShadow() ? &defaultInsetWebKitBoxShadowData : &defaultInsetShadowData;
    
    return otherShadow->isWebkitBoxShadow() ? &defaultWebKitBoxShadowData : &defaultShadowData;
}

class PropertyWrapperShadow : public PropertyWrapperBase {
public:
    PropertyWrapperShadow(int prop, const ShadowData* (RenderStyle::*getter)() const, void (RenderStyle::*setter)(PassOwnPtr<ShadowData>, bool))
        : PropertyWrapperBase(prop)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        const ShadowData* shadowA = (a->*m_getter)();
        const ShadowData* shadowB = (b->*m_getter)();
        
        while (true) {
            if (!shadowA && !shadowB)   // end of both lists
                return true;

            if (!shadowA || !shadowB)   // end of just one of the lists
                return false;

            if (*shadowA != *shadowB)
                return false;
        
            shadowA = shadowA->next();
            shadowB = shadowB->next();
        }

        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        const ShadowData* shadowA = (a->*m_getter)();
        const ShadowData* shadowB = (b->*m_getter)();

        int fromLength = shadowListLength(shadowA);
        int toLength = shadowListLength(shadowB);

        if (fromLength == toLength || (fromLength <= 1 && toLength <= 1)) {
            (dst->*m_setter)(blendSimpleOrMatchedShadowLists(anim, progress, shadowA, shadowB), false);
            return;
        }

        (dst->*m_setter)(blendMismatchedShadowLists(anim, progress, shadowA, shadowB, fromLength, toLength), false);
    }

private:
    PassOwnPtr<ShadowData*> blendSimpleOrMatchedShadowLists(const AnimationBase* anim, double progress, const ShadowData* shadowA, const ShadowData* shadowB) const
    {
        OwnPtr<ShadowData> newShadowData;
        ShadowData* lastShadow = 0;
        
        while (shadowA || shadowB) {
            const ShadowData* srcShadow = shadowForBlending(shadowA, shadowB);
            const ShadowData* dstShadow = shadowForBlending(shadowB, shadowA);

            OwnPtr<ShadowData> blendedShadow = blendFunc(anim, srcShadow, dstShadow, progress);
            ShadowData* blendedShadowPtr = blendedShadow.get();

            if (!lastShadow)
                newShadowData = blendedShadow.release();
            else
                lastShadow->setNext(blendedShadow.release());
            
            lastShadow = blendedShadowPtr;

            shadowA = shadowA ? shadowA->next() : 0;
            shadowB = shadowB ? shadowB->next() : 0;
        }
        
        return newShadowData.release();
    }

    PassOwnPtr<ShadowData*> blendMismatchedShadowLists(const AnimationBase* anim, double progress, const ShadowData* shadowA, const ShadowData* shadowB, int fromLength, int toLength) const
    {
        // The shadows in ShadowData are stored in reverse order, so when animating mismatched lists,
        // reverse them and match from the end.
        Vector<const ShadowData*, 4> fromShadows(fromLength);
        for (int i = fromLength - 1; i >= 0; --i) {
            fromShadows[i] = shadowA;
            shadowA = shadowA->next();
        }

        Vector<const ShadowData*, 4> toShadows(toLength);
        for (int i = toLength - 1; i >= 0; --i) {
            toShadows[i] = shadowB;
            shadowB = shadowB->next();
        }

        OwnPtr<ShadowData> newShadowData;
        
        int maxLength = max(fromLength, toLength);
        for (int i = 0; i < maxLength; ++i) {
            const ShadowData* fromShadow = i < fromLength ? fromShadows[i] : 0;
            const ShadowData* toShadow = i < toLength ? toShadows[i] : 0;
            
            const ShadowData* srcShadow = shadowForBlending(fromShadow, toShadow);
            const ShadowData* dstShadow = shadowForBlending(toShadow, fromShadow);

            OwnPtr<ShadowData> blendedShadow = blendFunc(anim, srcShadow, dstShadow, progress);
            // Insert at the start of the list to preserve the order.
            blendedShadow->setNext(newShadowData.release());
            newShadowData = blendedShadow.release();
        }

        return newShadowData.release();
    }

    const ShadowData* (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(PassOwnPtr<ShadowData>, bool);
};

class PropertyWrapperMaybeInvalidColor : public PropertyWrapperBase {
public:
    PropertyWrapperMaybeInvalidColor(int prop, const Color& (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
        : PropertyWrapperBase(prop)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return true;

        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();

        return fromColor == toColor;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return;

        if (!fromColor.isValid())
            fromColor = a->color();
        if (!toColor.isValid())
            toColor = b->color();
        (dst->*m_setter)(blendFunc(anim, fromColor, toColor, progress));
    }

private:
    const Color& (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(const Color&);
};

enum MaybeInvalidColorTag { MaybeInvalidColor };
class PropertyWrapperVisitedAffectedColor : public PropertyWrapperBase {
public:
    PropertyWrapperVisitedAffectedColor(int prop, const Color& (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&), 
                                        const Color& (RenderStyle::*visitedGetter)() const, void (RenderStyle::*visitedSetter)(const Color&))
        : PropertyWrapperBase(prop)
        , m_wrapper(adoptPtr(new PropertyWrapper<const Color&>(prop, getter, setter)))
        , m_visitedWrapper(adoptPtr(new PropertyWrapper<const Color&>(prop, visitedGetter, visitedSetter)))
    {
    }
    PropertyWrapperVisitedAffectedColor(int prop, MaybeInvalidColorTag, const Color& (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&), 
                                        const Color& (RenderStyle::*visitedGetter)() const, void (RenderStyle::*visitedSetter)(const Color&))
        : PropertyWrapperBase(prop)
        , m_wrapper(adoptPtr(new PropertyWrapperMaybeInvalidColor(prop, getter, setter)))
        , m_visitedWrapper(adoptPtr(new PropertyWrapperMaybeInvalidColor(prop, visitedGetter, visitedSetter)))
    {
    }
    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        return m_wrapper->equals(a, b) && m_visitedWrapper->equals(a, b);
    }
    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        m_wrapper->blend(anim, dst, a, b, progress);
        m_visitedWrapper->blend(anim, dst, a, b, progress);
    }
    
private:
    OwnPtr<PropertyWrapperBase> m_wrapper;
    OwnPtr<PropertyWrapperBase> m_visitedWrapper;
};

// Wrapper base class for an animatable property in a FillLayer
class FillLayerPropertyWrapperBase {
public:
    FillLayerPropertyWrapperBase()
    {
    }

    virtual ~FillLayerPropertyWrapperBase() { }
    
    virtual bool equals(const FillLayer* a, const FillLayer* b) const = 0;
    virtual void blend(const AnimationBase* anim, FillLayer* dst, const FillLayer* a, const FillLayer* b, double progress) const = 0;
};

template <typename T>
class FillLayerPropertyWrapperGetter : public FillLayerPropertyWrapperBase {
    WTF_MAKE_NONCOPYABLE(FillLayerPropertyWrapperGetter);
public:
    FillLayerPropertyWrapperGetter(T (FillLayer::*getter)() const)
        : m_getter(getter)
    {
    }

    virtual bool equals(const FillLayer* a, const FillLayer* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if ((!a && !b) || a == b)
           return true;
       if (!a || !b)
            return false;
        return (a->*m_getter)() == (b->*m_getter)();
    }

protected:
    T (FillLayer::*m_getter)() const;
};

template <typename T>
class FillLayerPropertyWrapper : public FillLayerPropertyWrapperGetter<T> {
public:
    FillLayerPropertyWrapper(T (FillLayer::*getter)() const, void (FillLayer::*setter)(T))
        : FillLayerPropertyWrapperGetter<T>(getter)
        , m_setter(setter)
    {
    }
    
    virtual void blend(const AnimationBase* anim, FillLayer* dst, const FillLayer* a, const FillLayer* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*FillLayerPropertyWrapperGetter<T>::m_getter)(), (b->*FillLayerPropertyWrapperGetter<T>::m_getter)(), progress));
    }

protected:
    void (FillLayer::*m_setter)(T);
};

template <typename T>
class FillLayerRefCountedPropertyWrapper : public FillLayerPropertyWrapperGetter<T*> {
public:
    FillLayerRefCountedPropertyWrapper(T* (FillLayer::*getter)() const, void (FillLayer::*setter)(PassRefPtr<T>))
        : FillLayerPropertyWrapperGetter<T*>(getter)
        , m_setter(setter)
    {
    }

    virtual void blend(const AnimationBase* anim, FillLayer* dst, const FillLayer* a, const FillLayer* b, double progress) const
    {
        (dst->*m_setter)(blendFunc(anim, (a->*FillLayerPropertyWrapperGetter<T*>::m_getter)(), (b->*FillLayerPropertyWrapperGetter<T*>::m_getter)(), progress));
    }

protected:
    void (FillLayer::*m_setter)(PassRefPtr<T>);
};

class FillLayerStyleImagePropertyWrapper : public FillLayerRefCountedPropertyWrapper<StyleImage> {
public:
    FillLayerStyleImagePropertyWrapper(StyleImage* (FillLayer::*getter)() const, void (FillLayer::*setter)(PassRefPtr<StyleImage>))
        : FillLayerRefCountedPropertyWrapper<StyleImage>(getter, setter)
    {
    }

    virtual bool equals(const FillLayer* a, const FillLayer* b) const
    {
       // If the style pointers are the same, don't bother doing the test.
       // If either is null, return false. If both are null, return true.
       if (a == b)
           return true;
       if (!a || !b)
            return false;

        StyleImage* imageA = (a->*m_getter)();
        StyleImage* imageB = (b->*m_getter)();
        return StyleImage::imagesEquivalent(imageA, imageB);
    }
};

class FillLayersPropertyWrapper : public PropertyWrapperBase {
public:
    typedef const FillLayer* (RenderStyle::*LayersGetter)() const;
    typedef FillLayer* (RenderStyle::*LayersAccessor)();
    
    FillLayersPropertyWrapper(int prop, LayersGetter getter, LayersAccessor accessor)
        : PropertyWrapperBase(prop)
        , m_layersGetter(getter)
        , m_layersAccessor(accessor)
    {
        switch (prop) {
            case CSSPropertyBackgroundPositionX:
            case CSSPropertyWebkitMaskPositionX:
                m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<Length>(&FillLayer::xPosition, &FillLayer::setXPosition);
                break;
            case CSSPropertyBackgroundPositionY:
            case CSSPropertyWebkitMaskPositionY:
                m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<Length>(&FillLayer::yPosition, &FillLayer::setYPosition);
                break;
            case CSSPropertyBackgroundSize:
            case CSSPropertyWebkitBackgroundSize:
            case CSSPropertyWebkitMaskSize:
                m_fillLayerPropertyWrapper = new FillLayerPropertyWrapper<LengthSize>(&FillLayer::sizeLength, &FillLayer::setSizeLength);
                break;
            case CSSPropertyBackgroundImage:
                m_fillLayerPropertyWrapper = new FillLayerStyleImagePropertyWrapper(&FillLayer::image, &FillLayer::setImage);
                break;
        }
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        const FillLayer* fromLayer = (a->*m_layersGetter)();
        const FillLayer* toLayer = (b->*m_layersGetter)();

        while (fromLayer && toLayer) {
            if (!m_fillLayerPropertyWrapper->equals(fromLayer, toLayer))
                return false;
            
            fromLayer = fromLayer->next();
            toLayer = toLayer->next();
        }

        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        const FillLayer* aLayer = (a->*m_layersGetter)();
        const FillLayer* bLayer = (b->*m_layersGetter)();
        FillLayer* dstLayer = (dst->*m_layersAccessor)();

        while (aLayer && bLayer && dstLayer) {
            m_fillLayerPropertyWrapper->blend(anim, dstLayer, aLayer, bLayer, progress);
            aLayer = aLayer->next();
            bLayer = bLayer->next();
            dstLayer = dstLayer->next();
        }
    }

private:
    FillLayerPropertyWrapperBase* m_fillLayerPropertyWrapper;
    
    LayersGetter m_layersGetter;
    LayersAccessor m_layersAccessor;
};

class ShorthandPropertyWrapper : public PropertyWrapperBase {
public:
    ShorthandPropertyWrapper(int property, const CSSPropertyLonghand& longhand)
        : PropertyWrapperBase(property)
    {
        for (unsigned i = 0; i < longhand.length(); ++i) {
            PropertyWrapperBase* wrapper = wrapperForProperty(longhand.properties()[i]);
            if (wrapper)
                m_propertyWrappers.append(wrapper);
        }
    }

    virtual bool isShorthandWrapper() const { return true; }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        Vector<PropertyWrapperBase*>::const_iterator end = m_propertyWrappers.end();
        for (Vector<PropertyWrapperBase*>::const_iterator it = m_propertyWrappers.begin(); it != end; ++it) {
            if (!(*it)->equals(a, b))
                return false;
        }
        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        Vector<PropertyWrapperBase*>::const_iterator end = m_propertyWrappers.end();
        for (Vector<PropertyWrapperBase*>::const_iterator it = m_propertyWrappers.begin(); it != end; ++it)
            (*it)->blend(anim, dst, a, b, progress);
    }

    const Vector<PropertyWrapperBase*> propertyWrappers() const { return m_propertyWrappers; }

private:
    Vector<PropertyWrapperBase*> m_propertyWrappers;
};

#if ENABLE(SVG)
class PropertyWrapperSVGPaint : public PropertyWrapperBase {
public:
    PropertyWrapperSVGPaint(int prop, const SVGPaint::SVGPaintType& (RenderStyle::*paintTypeGetter)() const, const Color& (RenderStyle::*getter)() const, void (RenderStyle::*setter)(const Color&))
        : PropertyWrapperBase(prop)
        , m_paintTypeGetter(paintTypeGetter)
        , m_getter(getter)
        , m_setter(setter)
    {
    }

    virtual bool equals(const RenderStyle* a, const RenderStyle* b) const
    {
        if ((a->*m_paintTypeGetter)() != (b->*m_paintTypeGetter)())
            return false;
        
        // We only support animations between SVGPaints that are pure Color values.
        // For everything else we must return true for this method, otherwise
        // we will try to animate between values forever.
        if ((a->*m_paintTypeGetter)() == SVGPaint::SVG_PAINTTYPE_RGBCOLOR) {
            Color fromColor = (a->*m_getter)();
            Color toColor = (b->*m_getter)();

            if (!fromColor.isValid() && !toColor.isValid())
                return true;

            if (!fromColor.isValid())
                fromColor = Color();
            if (!toColor.isValid())
                toColor = Color();

            return fromColor == toColor;
        }
        return true;
    }

    virtual void blend(const AnimationBase* anim, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress) const
    {
        if ((a->*m_paintTypeGetter)() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR
            || (b->*m_paintTypeGetter)() != SVGPaint::SVG_PAINTTYPE_RGBCOLOR)
            return;

        Color fromColor = (a->*m_getter)();
        Color toColor = (b->*m_getter)();

        if (!fromColor.isValid() && !toColor.isValid())
            return;

        if (!fromColor.isValid())
            fromColor = Color();
        if (!toColor.isValid())
            toColor = Color();
        (dst->*m_setter)(blendFunc(anim, fromColor, toColor, progress));
    }

private:
    const SVGPaint::SVGPaintType& (RenderStyle::*m_paintTypeGetter)() const;
    const Color& (RenderStyle::*m_getter)() const;
    void (RenderStyle::*m_setter)(const Color&);
};
#endif

static Vector<PropertyWrapperBase*>* gPropertyWrappers = 0;
static int gPropertyWrapperMap[numCSSProperties];

static const int cInvalidPropertyWrapperIndex = -1;


void AnimationBase::ensurePropertyMap()
{
    // FIXME: This data is never destroyed. Maybe we should ref count it and toss it when the last AnimationController is destroyed?
    if (gPropertyWrappers == 0) {
        gPropertyWrappers = new Vector<PropertyWrapperBase*>();

        // build the list of property wrappers to do the comparisons and blends
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyLeft, &RenderStyle::left, &RenderStyle::setLeft));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyRight, &RenderStyle::right, &RenderStyle::setRight));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyTop, &RenderStyle::top, &RenderStyle::setTop));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyBottom, &RenderStyle::bottom, &RenderStyle::setBottom));

        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWidth, &RenderStyle::width, &RenderStyle::setWidth));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMinWidth, &RenderStyle::minWidth, &RenderStyle::setMinWidth));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMaxWidth, &RenderStyle::maxWidth, &RenderStyle::setMaxWidth));

        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyHeight, &RenderStyle::height, &RenderStyle::setHeight));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMinHeight, &RenderStyle::minHeight, &RenderStyle::setMinHeight));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMaxHeight, &RenderStyle::maxHeight, &RenderStyle::setMaxHeight));

        gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderLeftWidth, &RenderStyle::borderLeftWidth, &RenderStyle::setBorderLeftWidth));
        gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderRightWidth, &RenderStyle::borderRightWidth, &RenderStyle::setBorderRightWidth));
        gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderTopWidth, &RenderStyle::borderTopWidth, &RenderStyle::setBorderTopWidth));
        gPropertyWrappers->append(new PropertyWrapper<unsigned>(CSSPropertyBorderBottomWidth, &RenderStyle::borderBottomWidth, &RenderStyle::setBorderBottomWidth));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginLeft, &RenderStyle::marginLeft, &RenderStyle::setMarginLeft));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginRight, &RenderStyle::marginRight, &RenderStyle::setMarginRight));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginTop, &RenderStyle::marginTop, &RenderStyle::setMarginTop));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyMarginBottom, &RenderStyle::marginBottom, &RenderStyle::setMarginBottom));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingLeft, &RenderStyle::paddingLeft, &RenderStyle::setPaddingLeft));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingRight, &RenderStyle::paddingRight, &RenderStyle::setPaddingRight));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingTop, &RenderStyle::paddingTop, &RenderStyle::setPaddingTop));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyPaddingBottom, &RenderStyle::paddingBottom, &RenderStyle::setPaddingBottom));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyColor, &RenderStyle::color, &RenderStyle::setColor, &RenderStyle::visitedLinkColor, &RenderStyle::setVisitedLinkColor));

        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBackgroundColor, &RenderStyle::backgroundColor, &RenderStyle::setBackgroundColor, &RenderStyle::visitedLinkBackgroundColor, &RenderStyle::setVisitedLinkBackgroundColor));

        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundImage, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
        gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyListStyleImage, &RenderStyle::listStyleImage, &RenderStyle::setListStyleImage));
        gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyWebkitMaskImage, &RenderStyle::maskImage, &RenderStyle::setMaskImage));

        gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyBorderImageSource, &RenderStyle::borderImageSource, &RenderStyle::setBorderImageSource));
        gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageSlice, &RenderStyle::borderImageSlices, &RenderStyle::setBorderImageSlices));
        gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageWidth, &RenderStyle::borderImageWidth, &RenderStyle::setBorderImageWidth));
        gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyBorderImageOutset, &RenderStyle::borderImageOutset, &RenderStyle::setBorderImageOutset));

        gPropertyWrappers->append(new StyleImagePropertyWrapper(CSSPropertyWebkitMaskBoxImageSource, &RenderStyle::maskBoxImageSource, &RenderStyle::setMaskBoxImageSource));
        gPropertyWrappers->append(new PropertyWrapper<const NinePieceImage&>(CSSPropertyWebkitMaskBoxImage, &RenderStyle::maskBoxImage, &RenderStyle::setMaskBoxImage));

        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundPositionX, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundPositionY, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyBackgroundSize, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));
        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitBackgroundSize, &RenderStyle::backgroundLayers, &RenderStyle::accessBackgroundLayers));

        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskPositionX, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));
        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskPositionY, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));
        gPropertyWrappers->append(new FillLayersPropertyWrapper(CSSPropertyWebkitMaskSize, &RenderStyle::maskLayers, &RenderStyle::accessMaskLayers));

        gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyFontSize, &RenderStyle::fontSize, &RenderStyle::setBlendedFontSize));
        gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyWebkitColumnRuleWidth, &RenderStyle::columnRuleWidth, &RenderStyle::setColumnRuleWidth));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitColumnGap, &RenderStyle::columnGap, &RenderStyle::setColumnGap));
        gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyWebkitColumnCount, &RenderStyle::columnCount, &RenderStyle::setColumnCount));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitColumnWidth, &RenderStyle::columnWidth, &RenderStyle::setColumnWidth));
        gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyWebkitBorderHorizontalSpacing, &RenderStyle::horizontalBorderSpacing, &RenderStyle::setHorizontalBorderSpacing));
        gPropertyWrappers->append(new PropertyWrapper<short>(CSSPropertyWebkitBorderVerticalSpacing, &RenderStyle::verticalBorderSpacing, &RenderStyle::setVerticalBorderSpacing));
        gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyZIndex, &RenderStyle::zIndex, &RenderStyle::setZIndex));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyLineHeight, &RenderStyle::lineHeight, &RenderStyle::setLineHeight));
        gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyOutlineOffset, &RenderStyle::outlineOffset, &RenderStyle::setOutlineOffset));
        gPropertyWrappers->append(new PropertyWrapper<unsigned short>(CSSPropertyOutlineWidth, &RenderStyle::outlineWidth, &RenderStyle::setOutlineWidth));
        gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyLetterSpacing, &RenderStyle::letterSpacing, &RenderStyle::setLetterSpacing));
        gPropertyWrappers->append(new PropertyWrapper<int>(CSSPropertyWordSpacing, &RenderStyle::wordSpacing, &RenderStyle::setWordSpacing));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyTextIndent, &RenderStyle::textIndent, &RenderStyle::setTextIndent));

        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitPerspective, &RenderStyle::perspective, &RenderStyle::setPerspective));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitPerspectiveOriginX, &RenderStyle::perspectiveOriginX, &RenderStyle::setPerspectiveOriginX));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitPerspectiveOriginY, &RenderStyle::perspectiveOriginY, &RenderStyle::setPerspectiveOriginY));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitTransformOriginX, &RenderStyle::transformOriginX, &RenderStyle::setTransformOriginX));
        gPropertyWrappers->append(new PropertyWrapper<Length>(CSSPropertyWebkitTransformOriginY, &RenderStyle::transformOriginY, &RenderStyle::setTransformOriginY));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyWebkitTransformOriginZ, &RenderStyle::transformOriginZ, &RenderStyle::setTransformOriginZ));
        gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderTopLeftRadius, &RenderStyle::borderTopLeftRadius, &RenderStyle::setBorderTopLeftRadius));
        gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderTopRightRadius, &RenderStyle::borderTopRightRadius, &RenderStyle::setBorderTopRightRadius));
        gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderBottomLeftRadius, &RenderStyle::borderBottomLeftRadius, &RenderStyle::setBorderBottomLeftRadius));
        gPropertyWrappers->append(new PropertyWrapper<LengthSize>(CSSPropertyBorderBottomRightRadius, &RenderStyle::borderBottomRightRadius, &RenderStyle::setBorderBottomRightRadius));
        gPropertyWrappers->append(new PropertyWrapper<EVisibility>(CSSPropertyVisibility, &RenderStyle::visibility, &RenderStyle::setVisibility));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyZoom, &RenderStyle::zoom, &RenderStyle::setZoomWithoutReturnValue));

        gPropertyWrappers->append(new PropertyWrapper<LengthBox>(CSSPropertyClip, &RenderStyle::clip, &RenderStyle::setClip));
        
#if USE(ACCELERATED_COMPOSITING)
        gPropertyWrappers->append(new PropertyWrapperAcceleratedOpacity());
        gPropertyWrappers->append(new PropertyWrapperAcceleratedTransform());
#if ENABLE(CSS_FILTERS)
        gPropertyWrappers->append(new PropertyWrapperAcceleratedFilter());
#endif
#else
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyOpacity, &RenderStyle::opacity, &RenderStyle::setOpacity));
        gPropertyWrappers->append(new PropertyWrapper<const TransformOperations&>(CSSPropertyWebkitTransform, &RenderStyle::transform, &RenderStyle::setTransform));
#if ENABLE(CSS_FILTERS)
        gPropertyWrappers->append(new PropertyWrapper<const FilterOperations&>(CSSPropertyWebkitFilter, &RenderStyle::filter, &RenderStyle::setFilter));
#endif
#endif

        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitColumnRuleColor, MaybeInvalidColor, &RenderStyle::columnRuleColor, &RenderStyle::setColumnRuleColor, &RenderStyle::visitedLinkColumnRuleColor, &RenderStyle::setVisitedLinkColumnRuleColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitTextStrokeColor, MaybeInvalidColor, &RenderStyle::textStrokeColor, &RenderStyle::setTextStrokeColor, &RenderStyle::visitedLinkTextStrokeColor, &RenderStyle::setVisitedLinkTextStrokeColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyWebkitTextFillColor, MaybeInvalidColor, &RenderStyle::textFillColor, &RenderStyle::setTextFillColor, &RenderStyle::visitedLinkTextFillColor, &RenderStyle::setVisitedLinkTextFillColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderLeftColor, MaybeInvalidColor, &RenderStyle::borderLeftColor, &RenderStyle::setBorderLeftColor, &RenderStyle::visitedLinkBorderLeftColor, &RenderStyle::setVisitedLinkBorderLeftColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderRightColor, MaybeInvalidColor, &RenderStyle::borderRightColor, &RenderStyle::setBorderRightColor, &RenderStyle::visitedLinkBorderRightColor, &RenderStyle::setVisitedLinkBorderRightColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderTopColor, MaybeInvalidColor, &RenderStyle::borderTopColor, &RenderStyle::setBorderTopColor, &RenderStyle::visitedLinkBorderTopColor, &RenderStyle::setVisitedLinkBorderTopColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyBorderBottomColor, MaybeInvalidColor, &RenderStyle::borderBottomColor, &RenderStyle::setBorderBottomColor, &RenderStyle::visitedLinkBorderBottomColor, &RenderStyle::setVisitedLinkBorderBottomColor));
        gPropertyWrappers->append(new PropertyWrapperVisitedAffectedColor(CSSPropertyOutlineColor, MaybeInvalidColor, &RenderStyle::outlineColor, &RenderStyle::setOutlineColor, &RenderStyle::visitedLinkOutlineColor, &RenderStyle::setVisitedLinkOutlineColor));

        gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyBoxShadow, &RenderStyle::boxShadow, &RenderStyle::setBoxShadow));
        gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyWebkitBoxShadow, &RenderStyle::boxShadow, &RenderStyle::setBoxShadow));
        gPropertyWrappers->append(new PropertyWrapperShadow(CSSPropertyTextShadow, &RenderStyle::textShadow, &RenderStyle::setTextShadow));

#if ENABLE(SVG)
        gPropertyWrappers->append(new PropertyWrapperSVGPaint(CSSPropertyFill, &RenderStyle::fillPaintType, &RenderStyle::fillPaintColor, &RenderStyle::setFillPaintColor));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyFillOpacity, &RenderStyle::fillOpacity, &RenderStyle::setFillOpacity));

        gPropertyWrappers->append(new PropertyWrapperSVGPaint(CSSPropertyStroke, &RenderStyle::strokePaintType, &RenderStyle::strokePaintColor, &RenderStyle::setStrokePaintColor));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStrokeOpacity, &RenderStyle::strokeOpacity, &RenderStyle::setStrokeOpacity));
        gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyStrokeWidth, &RenderStyle::strokeWidth, &RenderStyle::setStrokeWidth));
        gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyStrokeDashoffset, &RenderStyle::strokeDashOffset, &RenderStyle::setStrokeDashOffset));
        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStrokeMiterlimit, &RenderStyle::strokeMiterLimit, &RenderStyle::setStrokeMiterLimit));

        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyFloodOpacity, &RenderStyle::floodOpacity, &RenderStyle::setFloodOpacity));
        gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyFloodColor, &RenderStyle::floodColor, &RenderStyle::setFloodColor));

        gPropertyWrappers->append(new PropertyWrapper<float>(CSSPropertyStopOpacity, &RenderStyle::stopOpacity, &RenderStyle::setStopOpacity));
        gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyStopColor, &RenderStyle::stopColor, &RenderStyle::setStopColor));

        gPropertyWrappers->append(new PropertyWrapperMaybeInvalidColor(CSSPropertyLightingColor, &RenderStyle::lightingColor, &RenderStyle::setLightingColor));

        gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyBaselineShift, &RenderStyle::baselineShiftValue, &RenderStyle::setBaselineShiftValue));
        gPropertyWrappers->append(new PropertyWrapper<SVGLength>(CSSPropertyKerning, &RenderStyle::kerning, &RenderStyle::setKerning));
#endif

        // TODO:
        // 
        //  CSSPropertyVerticalAlign
        // 
        // Compound properties that have components that should be animatable:
        // 
        //  CSSPropertyWebkitColumns
        //  CSSPropertyWebkitBoxReflect

        // Make sure unused slots have a value
        for (unsigned int i = 0; i < static_cast<unsigned int>(numCSSProperties); ++i)
            gPropertyWrapperMap[i] = cInvalidPropertyWrapperIndex;

        // First we put the non-shorthand property wrappers into the map, so the shorthand-building
        // code can find them.
        size_t n = gPropertyWrappers->size();
        for (unsigned int i = 0; i < n; ++i) {
            ASSERT((*gPropertyWrappers)[i]->property() - firstCSSProperty < numCSSProperties);
            gPropertyWrapperMap[(*gPropertyWrappers)[i]->property() - firstCSSProperty] = i;
        }
        
        // Now add the shorthand wrappers.
        addShorthandProperties();
    }
}

static void addPropertyWrapper(int propertyID, PropertyWrapperBase* wrapper)
{
    int propIndex = propertyID - firstCSSProperty;

    ASSERT(gPropertyWrapperMap[propIndex] == cInvalidPropertyWrapperIndex);

    unsigned wrapperIndex = gPropertyWrappers->size();
    gPropertyWrappers->append(wrapper);
    gPropertyWrapperMap[propIndex] = wrapperIndex;
}

static void addShorthandProperties()
{
    static const int animatableShorthandProperties[] = {
        CSSPropertyBackground, // for background-color, background-position, background-image
        CSSPropertyBackgroundPosition,
        CSSPropertyFont, // for font-size, font-weight
        CSSPropertyWebkitMask, // for mask-position
        CSSPropertyWebkitMaskPosition,
        CSSPropertyBorderTop, CSSPropertyBorderRight, CSSPropertyBorderBottom, CSSPropertyBorderLeft,
        CSSPropertyBorderColor,
        CSSPropertyBorderRadius,
        CSSPropertyBorderWidth,
        CSSPropertyBorder,
        CSSPropertyBorderImage,
        CSSPropertyBorderSpacing,
        CSSPropertyListStyle, // for list-style-image
        CSSPropertyMargin,
        CSSPropertyOutline,
        CSSPropertyPadding,
        CSSPropertyWebkitTextStroke,
        CSSPropertyWebkitColumnRule,
        CSSPropertyWebkitBorderRadius,
        CSSPropertyWebkitTransformOrigin
    };

    for (size_t i = 0; i < WTF_ARRAY_LENGTH(animatableShorthandProperties); ++i) {
        int propertyID = animatableShorthandProperties[i];
        CSSPropertyLonghand longhand = longhandForProperty(propertyID);
        if (longhand.length() > 0)
            addPropertyWrapper(propertyID, new ShorthandPropertyWrapper(propertyID, longhand));
    }
}

static PropertyWrapperBase* wrapperForProperty(int propertyID)
{
    int propIndex = propertyID - firstCSSProperty;
    if (propIndex >= 0 && propIndex < numCSSProperties) {
        int wrapperIndex = gPropertyWrapperMap[propIndex];
        if (wrapperIndex >= 0)
            return (*gPropertyWrappers)[wrapperIndex];
    }
    return 0;
}

AnimationBase::AnimationBase(const Animation* transition, RenderObject* renderer, CompositeAnimation* compAnim)
    : m_animState(AnimationStateNew)
    , m_isAnimating(false)
    , m_isAccelerated(false)
    , m_transformFunctionListValid(false)
#if ENABLE(CSS_FILTERS)
    , m_filterFunctionListsMatch(false)
#endif
    , m_startTime(0)
    , m_pauseTime(-1)
    , m_requestedStartTime(0)
    , m_totalDuration(-1)
    , m_nextIterationDuration(-1)
    , m_object(renderer)
    , m_animation(const_cast<Animation*>(transition))
    , m_compAnim(compAnim)
{
    // Compute the total duration
    if (m_animation->iterationCount() > 0)
        m_totalDuration = m_animation->duration() * m_animation->iterationCount();
}

bool AnimationBase::propertiesEqual(int prop, const RenderStyle* a, const RenderStyle* b)
{
    ensurePropertyMap();
    if (prop == cAnimateAll) {
        size_t n = gPropertyWrappers->size();
        for (unsigned int i = 0; i < n; ++i) {
            PropertyWrapperBase* wrapper = (*gPropertyWrappers)[i];
            // No point comparing shorthand wrappers for 'all'.
            if (!wrapper->isShorthandWrapper() && !wrapper->equals(a, b))
                return false;
        }
    } else {
        PropertyWrapperBase* wrapper = wrapperForProperty(prop);
        if (wrapper)
            return wrapper->equals(a, b);
    }
    return true;
}

int AnimationBase::getPropertyAtIndex(int i, bool& isShorthand)
{
    ensurePropertyMap();
    if (i < 0 || i >= static_cast<int>(gPropertyWrappers->size()))
        return CSSPropertyInvalid;

    PropertyWrapperBase* wrapper = (*gPropertyWrappers)[i];
    isShorthand = wrapper->isShorthandWrapper();
    return wrapper->property();
}

int AnimationBase::getNumProperties()
{
    ensurePropertyMap();
    return gPropertyWrappers->size();
}

// Returns true if we need to start animation timers
bool AnimationBase::blendProperties(const AnimationBase* anim, int prop, RenderStyle* dst, const RenderStyle* a, const RenderStyle* b, double progress)
{
    ASSERT(prop != cAnimateAll);

    ensurePropertyMap();
    PropertyWrapperBase* wrapper = wrapperForProperty(prop);
    if (wrapper) {
        wrapper->blend(anim, dst, a, b, progress);
#if USE(ACCELERATED_COMPOSITING)
        return !wrapper->animationIsAccelerated() || !anim->isAccelerated();
#else
        return true;
#endif
    }

    return false;
}

#if USE(ACCELERATED_COMPOSITING)
bool AnimationBase::animationOfPropertyIsAccelerated(int prop)
{
    ensurePropertyMap();
    PropertyWrapperBase* wrapper = wrapperForProperty(prop);
    return wrapper ? wrapper->animationIsAccelerated() : false;
}
#endif

static bool gatherEnclosingShorthandProperties(int property, PropertyWrapperBase* wrapper, HashSet<int>& propertySet)
{
    if (!wrapper->isShorthandWrapper())
        return false;

    ShorthandPropertyWrapper* shorthandWrapper = static_cast<ShorthandPropertyWrapper*>(wrapper);
    
    bool contained = false;
    for (size_t i = 0; i < shorthandWrapper->propertyWrappers().size(); ++i) {
        PropertyWrapperBase* currWrapper = shorthandWrapper->propertyWrappers()[i];

        if (gatherEnclosingShorthandProperties(property, currWrapper, propertySet) || currWrapper->property() == property)
            contained = true;
    }
    
    if (contained)
        propertySet.add(wrapper->property());

    return contained;
}

// Note: this is inefficient. It's only called from pauseTransitionAtTime().
HashSet<int> AnimationBase::animatableShorthandsAffectingProperty(int property)
{
    ensurePropertyMap();

    HashSet<int> foundProperties;
    for (int i = 0; i < getNumProperties(); ++i)
        gatherEnclosingShorthandProperties(property, (*gPropertyWrappers)[i], foundProperties);

    return foundProperties;
}

void AnimationBase::setNeedsStyleRecalc(Node* node)
{
    ASSERT(!node || (node->document() && !node->document()->inPageCache()));
    if (node)
        node->setNeedsStyleRecalc(SyntheticStyleChange);
}

double AnimationBase::duration() const
{
    return m_animation->duration();
}

bool AnimationBase::playStatePlaying() const
{
    return m_animation->playState() == AnimPlayStatePlaying;
}

bool AnimationBase::animationsMatch(const Animation* anim) const
{
    return m_animation->animationsMatch(anim);
}

void AnimationBase::updateStateMachine(AnimStateInput input, double param)
{
    if (!m_compAnim)
        return;

    // If we get AnimationStateInputRestartAnimation then we force a new animation, regardless of state.
    if (input == AnimationStateInputMakeNew) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        m_animState = AnimationStateNew;
        m_startTime = 0;
        m_pauseTime = -1;
        m_requestedStartTime = 0;
        m_nextIterationDuration = -1;
        endAnimation();
        return;
    }

    if (input == AnimationStateInputRestartAnimation) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        m_animState = AnimationStateNew;
        m_startTime = 0;
        m_pauseTime = -1;
        m_requestedStartTime = 0;
        m_nextIterationDuration = -1;
        endAnimation();

        if (!paused())
            updateStateMachine(AnimationStateInputStartAnimation, -1);
        return;
    }

    if (input == AnimationStateInputEndAnimation) {
        if (m_animState == AnimationStateStartWaitStyleAvailable)
            m_compAnim->animationController()->removeFromAnimationsWaitingForStyle(this);
        m_animState = AnimationStateDone;
        endAnimation();
        return;
    }

    if (input == AnimationStateInputPauseOverride) {
        if (m_animState == AnimationStateStartWaitResponse) {
            // If we are in AnimationStateStartWaitResponse, the animation will get canceled before 
            // we get a response, so move to the next state.
            endAnimation();
            updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
        }
        return;
    }

    if (input == AnimationStateInputResumeOverride) {
        if (m_animState == AnimationStateLooping || m_animState == AnimationStateEnding) {
            // Start the animation
            startAnimation(beginAnimationUpdateTime() - m_startTime);
        }
        return;
    }

    // Execute state machine
    switch (m_animState) {
        case AnimationStateNew:
            ASSERT(input == AnimationStateInputStartAnimation || input == AnimationStateInputPlayStateRunning || input == AnimationStateInputPlayStatePaused);
            if (input == AnimationStateInputStartAnimation || input == AnimationStateInputPlayStateRunning) {
                m_requestedStartTime = beginAnimationUpdateTime();
                m_animState = AnimationStateStartWaitTimer;
            }
            break;
        case AnimationStateStartWaitTimer:
            ASSERT(input == AnimationStateInputStartTimerFired || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStartTimerFired) {
                ASSERT(param >= 0);
                // Start timer has fired, tell the animation to start and wait for it to respond with start time
                m_animState = AnimationStateStartWaitStyleAvailable;
                m_compAnim->animationController()->addToAnimationsWaitingForStyle(this);

                // Trigger a render so we can start the animation
                if (m_object)
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
            } else {
                ASSERT(!paused());
                // We're waiting for the start timer to fire and we got a pause. Cancel the timer, pause and wait
                m_pauseTime = beginAnimationUpdateTime();
                m_animState = AnimationStatePausedWaitTimer;
            }
            break;
        case AnimationStateStartWaitStyleAvailable:
            ASSERT(input == AnimationStateInputStyleAvailable || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStyleAvailable) {
                // Start timer has fired, tell the animation to start and wait for it to respond with start time
                m_animState = AnimationStateStartWaitResponse;

                overrideAnimations();

                // Start the animation
                if (overridden()) {
                    // We won't try to start accelerated animations if we are overridden and
                    // just move on to the next state.
                    m_animState = AnimationStateStartWaitResponse;
                    m_isAccelerated = false;
                    updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
                } else {
                    double timeOffset = 0;
                    // If the value for 'animation-delay' is negative then the animation appears to have started in the past.
                    if (m_animation->delay() < 0)
                        timeOffset = -m_animation->delay();
                    bool started = startAnimation(timeOffset);

                    m_compAnim->animationController()->addToAnimationsWaitingForStartTimeResponse(this, started);
                    m_isAccelerated = started;
                }
            } else {
                // We're waiting for the style to be available and we got a pause. Pause and wait
                m_pauseTime = beginAnimationUpdateTime();
                m_animState = AnimationStatePausedWaitStyleAvailable;
            }
            break;
        case AnimationStateStartWaitResponse:
            ASSERT(input == AnimationStateInputStartTimeSet || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputStartTimeSet) {
                ASSERT(param >= 0);
                // We have a start time, set it, unless the startTime is already set
                if (m_startTime <= 0) {
                    m_startTime = param;
                    // If the value for 'animation-delay' is negative then the animation appears to have started in the past.
                    if (m_animation->delay() < 0)
                        m_startTime += m_animation->delay();
                }

                // Now that we know the start time, fire the start event.
                onAnimationStart(0); // The elapsedTime is 0.

                // Decide whether to go into looping or ending state
                goIntoEndingOrLoopingState();

                // Dispatch updateStyleIfNeeded so we can start the animation
                if (m_object)
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
            } else {
                // We are pausing while waiting for a start response. Cancel the animation and wait. When 
                // we unpause, we will act as though the start timer just fired
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                m_animState = AnimationStatePausedWaitResponse;
            }
            break;
        case AnimationStateLooping:
            ASSERT(input == AnimationStateInputLoopTimerFired || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputLoopTimerFired) {
                ASSERT(param >= 0);
                // Loop timer fired, loop again or end.
                onAnimationIteration(param);

                // Decide whether to go into looping or ending state
                goIntoEndingOrLoopingState();
            } else {
                // We are pausing while running. Cancel the animation and wait
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                m_animState = AnimationStatePausedRun;
            }
            break;
        case AnimationStateEnding:
            ASSERT(input == AnimationStateInputEndTimerFired || input == AnimationStateInputPlayStatePaused);

            if (input == AnimationStateInputEndTimerFired) {

                ASSERT(param >= 0);
                // End timer fired, finish up
                onAnimationEnd(param);

                m_animState = AnimationStateDone;
                
                if (m_object) {
                    if (m_animation->fillsForwards())
                        m_animState = AnimationStateFillingForwards;
                    else
                        resumeOverriddenAnimations();

                    // Fire off another style change so we can set the final value
                    m_compAnim->animationController()->addNodeChangeToDispatch(m_object->node());
                }
            } else {
                // We are pausing while running. Cancel the animation and wait
                m_pauseTime = beginAnimationUpdateTime();
                pauseAnimation(beginAnimationUpdateTime() - m_startTime);
                m_animState = AnimationStatePausedRun;
            }
            // |this| may be deleted here
            break;
        case AnimationStatePausedWaitTimer:
            ASSERT(input == AnimationStateInputPlayStateRunning);
            ASSERT(paused());
            // Update the times
            m_startTime += beginAnimationUpdateTime() - m_pauseTime;
            m_pauseTime = -1;

            // we were waiting for the start timer to fire, go back and wait again
            m_animState = AnimationStateNew;
            updateStateMachine(AnimationStateInputStartAnimation, 0);
            break;
        case AnimationStatePausedWaitResponse:
        case AnimationStatePausedWaitStyleAvailable:
        case AnimationStatePausedRun:
            // We treat these two cases the same. The only difference is that, when we are in
            // AnimationStatePausedWaitResponse, we don't yet have a valid startTime, so we send 0 to startAnimation.
            // When the AnimationStateInputStartTimeSet comes in and we were in AnimationStatePausedRun, we will notice
            // that we have already set the startTime and will ignore it.
            ASSERT(input == AnimationStateInputPlayStateRunning || input == AnimationStateInputStartTimeSet || input == AnimationStateInputStyleAvailable);
            ASSERT(paused());
            
            if (input == AnimationStateInputPlayStateRunning) {
                // Update the times
                if (m_animState == AnimationStatePausedRun)
                    m_startTime += beginAnimationUpdateTime() - m_pauseTime;
                else
                    m_startTime = 0;
                m_pauseTime = -1;

                if (m_animState == AnimationStatePausedWaitStyleAvailable)
                    m_animState = AnimationStateStartWaitStyleAvailable;
                else {
                    // We were either running or waiting for a begin time response from the animation.
                    // Either way we need to restart the animation (possibly with an offset if we
                    // had already been running) and wait for it to start.
                    m_animState = AnimationStateStartWaitResponse;

                    // Start the animation
                    if (overridden()) {
                        // We won't try to start accelerated animations if we are overridden and
                        // just move on to the next state.
                        updateStateMachine(AnimationStateInputStartTimeSet, beginAnimationUpdateTime());
                        m_isAccelerated = true;
                    } else {
                        bool started = startAnimation(beginAnimationUpdateTime() - m_startTime);
                        m_compAnim->animationController()->addToAnimationsWaitingForStartTimeResponse(this, started);
                        m_isAccelerated = started;
                    }
                }
                break;
            }
            
            if (input == AnimationStateInputStartTimeSet) {
                ASSERT(m_animState == AnimationStatePausedWaitResponse);
                
                // We are paused but we got the callback that notifies us that an accelerated animation started.
                // We ignore the start time and just move into the paused-run state.
                m_animState = AnimationStatePausedRun;
                ASSERT(m_startTime == 0);
                m_startTime = param;
                m_pauseTime += m_startTime;
                break;
            }
            
            ASSERT(m_animState == AnimationStatePausedWaitStyleAvailable);
            // We are paused but we got the callback that notifies us that style has been updated.
            // We move to the AnimationStatePausedWaitResponse state
            m_animState = AnimationStatePausedWaitResponse;
            overrideAnimations();
            break;
        case AnimationStateFillingForwards:
        case AnimationStateDone:
            // We're done. Stay in this state until we are deleted
            break;
    }
}
    
void AnimationBase::fireAnimationEventsIfNeeded()
{
    if (!m_compAnim)
        return;

    // If we are waiting for the delay time to expire and it has, go to the next state
    if (m_animState != AnimationStateStartWaitTimer && m_animState != AnimationStateLooping && m_animState != AnimationStateEnding)
        return;

    // We have to make sure to keep a ref to the this pointer, because it could get destroyed
    // during an animation callback that might get called. Since the owner is a CompositeAnimation
    // and it ref counts this object, we will keep a ref to that instead. That way the AnimationBase
    // can still access the resources of its CompositeAnimation as needed.
    RefPtr<AnimationBase> protector(this);
    RefPtr<CompositeAnimation> compProtector(m_compAnim);
    
    // Check for start timeout
    if (m_animState == AnimationStateStartWaitTimer) {
        if (beginAnimationUpdateTime() - m_requestedStartTime >= m_animation->delay())
            updateStateMachine(AnimationStateInputStartTimerFired, 0);
        return;
    }
    
    double elapsedDuration = beginAnimationUpdateTime() - m_startTime;
    // FIXME: we need to ensure that elapsedDuration is never < 0. If it is, this suggests that
    // we had a recalcStyle() outside of beginAnimationUpdate()/endAnimationUpdate().
    // Also check in getTimeToNextEvent().
    elapsedDuration = max(elapsedDuration, 0.0);
    
    // Check for end timeout
    if (m_totalDuration >= 0 && elapsedDuration >= m_totalDuration) {
        // We may still be in AnimationStateLooping if we've managed to skip a
        // whole iteration, in which case we should jump to the end state.
        m_animState = AnimationStateEnding;

        // Fire an end event
        updateStateMachine(AnimationStateInputEndTimerFired, m_totalDuration);
    } else {
        // Check for iteration timeout
        if (m_nextIterationDuration < 0) {
            // Hasn't been set yet, set it
            double durationLeft = m_animation->duration() - fmod(elapsedDuration, m_animation->duration());
            m_nextIterationDuration = elapsedDuration + durationLeft;
        }
        
        if (elapsedDuration >= m_nextIterationDuration) {
            // Set to the next iteration
            double previous = m_nextIterationDuration;
            double durationLeft = m_animation->duration() - fmod(elapsedDuration, m_animation->duration());
            m_nextIterationDuration = elapsedDuration + durationLeft;
            
            // Send the event
            updateStateMachine(AnimationStateInputLoopTimerFired, previous);
        }
    }
}

void AnimationBase::updatePlayState(EAnimPlayState playState)
{
    if (!m_compAnim)
        return;

    // When we get here, we can have one of 4 desired states: running, paused, suspended, paused & suspended.
    // The state machine can be in one of two states: running, paused.
    // Set the state machine to the desired state.
    bool pause = playState == AnimPlayStatePaused || m_compAnim->suspended();
    
    if (pause == paused() && !isNew())
        return;
    
    updateStateMachine(pause ?  AnimationStateInputPlayStatePaused : AnimationStateInputPlayStateRunning, -1);
}

double AnimationBase::timeToNextService()
{
    // Returns the time at which next service is required. -1 means no service is required. 0 means 
    // service is required now, and > 0 means service is required that many seconds in the future.
    if (paused() || isNew() || m_animState == AnimationStateFillingForwards)
        return -1;
    
    if (m_animState == AnimationStateStartWaitTimer) {
        double timeFromNow = m_animation->delay() - (beginAnimationUpdateTime() - m_requestedStartTime);
        return max(timeFromNow, 0.0);
    }
    
    fireAnimationEventsIfNeeded();
        
    // In all other cases, we need service right away.
    return 0;
}

// Compute the fractional time, taking into account direction.
// There is no need to worry about iterations, we assume that we would have
// short circuited above if we were done.

double AnimationBase::fractionalTime(double scale, double elapsedTime, double offset) const
{
    double fractionalTime = m_animation->duration() ? (elapsedTime / m_animation->duration()) : 1;
    // FIXME: startTime can be before the current animation "frame" time. This is to sync with the frame time
    // concept in AnimationTimeController. So we need to somehow sync the two. Until then, the possible
    // error is small and will probably not be noticeable. Until we fix this, remove the assert.
    // https://bugs.webkit.org/show_bug.cgi?id=52037
    // ASSERT(fractionalTime >= 0);
    if (fractionalTime < 0)
        fractionalTime = 0;

    int integralTime = static_cast<int>(fractionalTime);
    if (m_animation->iterationCount() != Animation::IterationCountInfinite)
        integralTime = min(integralTime, m_animation->iterationCount() - 1);
    fractionalTime -= integralTime;

    if (((m_animation->direction() == Animation::AnimationDirectionAlternate) && (integralTime & 1))
        || ((m_animation->direction() == Animation::AnimationDirectionAlternateReverse) && !(integralTime & 1))
        || m_animation->direction() == Animation::AnimationDirectionReverse)
        fractionalTime = 1 - fractionalTime;

    if (scale != 1 || offset)
        fractionalTime = (fractionalTime - offset) * scale;

    return fractionalTime;
}

double AnimationBase::progress(double scale, double offset, const TimingFunction* tf) const
{
    if (preActive())
        return 0;

    double elapsedTime = getElapsedTime();

    double dur = m_animation->duration();
    if (m_animation->iterationCount() > 0)
        dur *= m_animation->iterationCount();

    if (postActive() || !m_animation->duration())
        return 1.0;
    if (m_animation->iterationCount() > 0 && elapsedTime >= dur)
        return (m_animation->iterationCount() % 2) ? 1.0 : 0.0;

    const double fractionalTime = this->fractionalTime(scale, elapsedTime, offset);

    if (!tf)
        tf = m_animation->timingFunction().get();

    if (tf->isCubicBezierTimingFunction()) {
        const CubicBezierTimingFunction* ctf = static_cast<const CubicBezierTimingFunction*>(tf);
        return solveCubicBezierFunction(ctf->x1(),
                                        ctf->y1(),
                                        ctf->x2(),
                                        ctf->y2(),
                                        fractionalTime, m_animation->duration());
    } else if (tf->isStepsTimingFunction()) {
        const StepsTimingFunction* stf = static_cast<const StepsTimingFunction*>(tf);
        return solveStepsFunction(stf->numberOfSteps(), stf->stepAtStart(), fractionalTime);
    } else
        return fractionalTime;
}

void AnimationBase::getTimeToNextEvent(double& time, bool& isLooping) const
{
    // Decide when the end or loop event needs to fire
    const double elapsedDuration = max(beginAnimationUpdateTime() - m_startTime, 0.0);
    double durationLeft = 0;
    double nextIterationTime = m_totalDuration;

    if (m_totalDuration < 0 || elapsedDuration < m_totalDuration) {
        durationLeft = m_animation->duration() > 0 ? (m_animation->duration() - fmod(elapsedDuration, m_animation->duration())) : 0;
        nextIterationTime = elapsedDuration + durationLeft;
    }
    
    if (m_totalDuration < 0 || nextIterationTime < m_totalDuration) {
        // We are not at the end yet
        ASSERT(nextIterationTime > 0);
        isLooping = true;
    } else {
        // We are at the end
        isLooping = false;
    }
    
    time = durationLeft;
}

void AnimationBase::goIntoEndingOrLoopingState()
{
    double t;
    bool isLooping;
    getTimeToNextEvent(t, isLooping);
    m_animState = isLooping ? AnimationStateLooping : AnimationStateEnding;
}
  
void AnimationBase::freezeAtTime(double t)
{
    if (!m_compAnim)
        return;

    if (!m_startTime) {
        // If we haven't started yet, make it as if we started.
        m_animState = AnimationStateStartWaitResponse;
        onAnimationStartResponse(currentTime());
    }

    ASSERT(m_startTime);        // if m_startTime is zero, we haven't started yet, so we'll get a bad pause time.
    if (t <= m_animation->delay())
        m_pauseTime = m_startTime;
    else
        m_pauseTime = m_startTime + t - m_animation->delay();

#if USE(ACCELERATED_COMPOSITING)
    if (m_object && m_object->hasLayer()) {
        RenderLayer* layer = toRenderBoxModelObject(m_object)->layer();
        if (layer->isComposited())
            layer->backing()->suspendAnimations(m_pauseTime);
    }
#endif
}

double AnimationBase::beginAnimationUpdateTime() const
{
    if (!m_compAnim)
        return 0;

    return m_compAnim->animationController()->beginAnimationUpdateTime();
}

double AnimationBase::getElapsedTime() const
{
    if (paused())    
        return m_pauseTime - m_startTime;
    if (m_startTime <= 0)
        return 0;
    if (postActive())
        return 1;

    return beginAnimationUpdateTime() - m_startTime;
}

void AnimationBase::setElapsedTime(double time)
{
    // FIXME: implement this method
    UNUSED_PARAM(time);
}

void AnimationBase::play()
{
    // FIXME: implement this method
}

void AnimationBase::pause()
{
    // FIXME: implement this method
}

} // namespace WebCore
