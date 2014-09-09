/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef FilterOperation_h
#define FilterOperation_h

#if ENABLE(CSS_FILTERS)

#include "Color.h"
#include "FilterEffect.h"
#include "LayoutSize.h"
#include "Length.h"
#include <wtf/OwnPtr.h>
#include <wtf/PassOwnPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

// Annoyingly, wingdi.h #defines this.
#ifdef PASSTHROUGH
#undef PASSTHROUGH
#endif

namespace WebCore {

// CSS Filters

class CachedSVGDocumentReference;

class FilterOperation : public RefCounted<FilterOperation> {
public:
    enum OperationType {
        REFERENCE, // url(#somefilter)
        GRAYSCALE,
        SEPIA,
        SATURATE,
        HUE_ROTATE,
        INVERT,
        OPACITY,
        BRIGHTNESS,
        CONTRAST,
        BLUR,
        DROP_SHADOW,
        PASSTHROUGH,
        DEFAULT,
        NONE
    };

    virtual ~FilterOperation() { }

    virtual PassRefPtr<FilterOperation> clone() const = 0;

    virtual bool operator==(const FilterOperation&) const = 0;
    bool operator!=(const FilterOperation& o) const { return !(*this == o); }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* /*from*/, double /*progress*/, bool /*blendToPassthrough*/ = false)
    {
        ASSERT(!blendingNeedsRendererSize());
        return 0;
    }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* /*from*/, double /*progress*/, const LayoutSize&, bool /*blendToPassthrough*/ = false)
    {
        ASSERT(blendingNeedsRendererSize());
        return 0;
    }

    OperationType type() const { return m_type; }

    bool isBasicColorMatrixFilterOperation() const
    {
        return m_type == GRAYSCALE || m_type == SEPIA || m_type == SATURATE || m_type == HUE_ROTATE;
    }

    bool isBasicComponentTransferFilterOperation() const
    {
        return m_type == INVERT || m_type == BRIGHTNESS || m_type == CONTRAST || m_type == OPACITY;
    }

    bool isSameType(const FilterOperation& o) const { return o.type() == m_type; }

    // True if the alpha channel of any pixel can change under this operation.
    virtual bool affectsOpacity() const { return false; }
    // True if the the value of one pixel can affect the value of another pixel under this operation, such as blur.
    virtual bool movesPixels() const { return false; }
    // True if the filter needs the size of the box in order to calculate the animations.
    virtual bool blendingNeedsRendererSize() const { return false; }

protected:
    FilterOperation(OperationType type)
        : m_type(type)
    {
    }

    OperationType m_type;
};

#define FILTEROPERATION_TYPE_CASTS(ToValueTypeName, predicate) \
    TYPE_CASTS_BASE(ToValueTypeName, WebCore::FilterOperation, value, value->predicate, value.predicate)

class DefaultFilterOperation : public FilterOperation {
public:
    static PassRefPtr<DefaultFilterOperation> create(OperationType representedType)
    {
        return adoptRef(new DefaultFilterOperation(representedType));
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new DefaultFilterOperation(representedType()));
    }

    OperationType representedType() const { return m_representedType; }

private:
    virtual bool operator==(const FilterOperation&) const override;

    DefaultFilterOperation(OperationType representedType)
        : FilterOperation(DEFAULT)
        , m_representedType(representedType)
    {
    }

    OperationType m_representedType;
};

FILTEROPERATION_TYPE_CASTS(DefaultFilterOperation, type() == FilterOperation::DEFAULT);


class PassthroughFilterOperation : public FilterOperation {
public:
    static PassRefPtr<PassthroughFilterOperation> create()
    {
        return adoptRef(new PassthroughFilterOperation());
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new PassthroughFilterOperation());
    }

private:
    virtual bool operator==(const FilterOperation& o) const override
    {
        return isSameType(o);
    }

    PassthroughFilterOperation()
        : FilterOperation(PASSTHROUGH)
    {
    }
};

FILTEROPERATION_TYPE_CASTS(PassthroughFilterOperation, type() == FilterOperation::PASSTHROUGH);

class ReferenceFilterOperation : public FilterOperation {
public:
    static PassRefPtr<ReferenceFilterOperation> create(const String& url, const String& fragment)
    {
        return adoptRef(new ReferenceFilterOperation(url, fragment));
    }
    virtual ~ReferenceFilterOperation();

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        // Reference filters cannot be cloned.
        ASSERT_NOT_REACHED();
        return nullptr;
    }

    virtual bool affectsOpacity() const override { return true; }
    virtual bool movesPixels() const override { return true; }

    const String& url() const { return m_url; }
    const String& fragment() const { return m_fragment; }

    CachedSVGDocumentReference* cachedSVGDocumentReference() const { return m_cachedSVGDocumentReference.get(); }
    CachedSVGDocumentReference* getOrCreateCachedSVGDocumentReference();

    FilterEffect* filterEffect() const { return m_filterEffect.get(); }
    void setFilterEffect(PassRefPtr<FilterEffect> filterEffect) { m_filterEffect = filterEffect; }

private:
    ReferenceFilterOperation(const String& url, const String& fragment);

    virtual bool operator==(const FilterOperation&) const override;

    String m_url;
    String m_fragment;
    std::unique_ptr<CachedSVGDocumentReference> m_cachedSVGDocumentReference;
    RefPtr<FilterEffect> m_filterEffect;
};

FILTEROPERATION_TYPE_CASTS(ReferenceFilterOperation, type() == FilterOperation::REFERENCE);

// GRAYSCALE, SEPIA, SATURATE and HUE_ROTATE are variations on a basic color matrix effect.
// For HUE_ROTATE, the angle of rotation is stored in m_amount.
class BasicColorMatrixFilterOperation : public FilterOperation {
public:
    static PassRefPtr<BasicColorMatrixFilterOperation> create(double amount, OperationType type)
    {
        return adoptRef(new BasicColorMatrixFilterOperation(amount, type));
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new BasicColorMatrixFilterOperation(amount(), type()));
    }

    double amount() const { return m_amount; }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, bool blendToPassthrough = false) override;

private:
    virtual bool operator==(const FilterOperation&) const override;

    double passthroughAmount() const;

    BasicColorMatrixFilterOperation(double amount, OperationType type)
        : FilterOperation(type)
        , m_amount(amount)
    {
    }

    double m_amount;
};

FILTEROPERATION_TYPE_CASTS(BasicColorMatrixFilterOperation, isBasicColorMatrixFilterOperation());

// INVERT, BRIGHTNESS, CONTRAST and OPACITY are variations on a basic component transfer effect.
class BasicComponentTransferFilterOperation : public FilterOperation {
public:
    static PassRefPtr<BasicComponentTransferFilterOperation> create(double amount, OperationType type)
    {
        return adoptRef(new BasicComponentTransferFilterOperation(amount, type));
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new BasicComponentTransferFilterOperation(amount(), type()));
    }

    double amount() const { return m_amount; }

    virtual bool affectsOpacity() const override { return m_type == OPACITY; }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, bool blendToPassthrough = false) override;

private:
    virtual bool operator==(const FilterOperation&) const override;

    double passthroughAmount() const;

    BasicComponentTransferFilterOperation(double amount, OperationType type)
        : FilterOperation(type)
        , m_amount(amount)
    {
    }

    double m_amount;
};

FILTEROPERATION_TYPE_CASTS(BasicComponentTransferFilterOperation, isBasicComponentTransferFilterOperation());

class BlurFilterOperation : public FilterOperation {
public:
    static PassRefPtr<BlurFilterOperation> create(Length stdDeviation)
    {
        return adoptRef(new BlurFilterOperation(WTF::move(stdDeviation)));
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new BlurFilterOperation(stdDeviation()));
    }

    const Length& stdDeviation() const { return m_stdDeviation; }

    virtual bool affectsOpacity() const override { return true; }
    virtual bool movesPixels() const override { return true; }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, bool blendToPassthrough = false) override;

private:
    virtual bool operator==(const FilterOperation&) const override;

    BlurFilterOperation(Length stdDeviation)
        : FilterOperation(BLUR)
        , m_stdDeviation(WTF::move(stdDeviation))
    {
    }

    Length m_stdDeviation;
};

FILTEROPERATION_TYPE_CASTS(BlurFilterOperation, type() == FilterOperation::BLUR);

class DropShadowFilterOperation : public FilterOperation {
public:
    static PassRefPtr<DropShadowFilterOperation> create(const IntPoint& location, int stdDeviation, Color color)
    {
        return adoptRef(new DropShadowFilterOperation(location, stdDeviation, color));
    }

    virtual PassRefPtr<FilterOperation> clone() const override
    {
        return adoptRef(new DropShadowFilterOperation(location(), stdDeviation(), color()));
    }

    int x() const { return m_location.x(); }
    int y() const { return m_location.y(); }
    IntPoint location() const { return m_location; }
    int stdDeviation() const { return m_stdDeviation; }
    Color color() const { return m_color; }

    virtual bool affectsOpacity() const override { return true; }
    virtual bool movesPixels() const override { return true; }

    virtual PassRefPtr<FilterOperation> blend(const FilterOperation* from, double progress, bool blendToPassthrough = false) override;

private:
    virtual bool operator==(const FilterOperation&) const override;

    DropShadowFilterOperation(const IntPoint& location, int stdDeviation, Color color)
        : FilterOperation(DROP_SHADOW)
        , m_location(location)
        , m_stdDeviation(stdDeviation)
        , m_color(color)
    {
    }

    IntPoint m_location; // FIXME: should location be in Lengths?
    int m_stdDeviation;
    Color m_color;
};

FILTEROPERATION_TYPE_CASTS(DropShadowFilterOperation, type() == FilterOperation::DROP_SHADOW);

} // namespace WebCore

#endif // ENABLE(CSS_FILTERS)

#endif // FilterOperation_h
