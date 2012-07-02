/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
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
 *
 */

#ifndef ViewportArguments_h
#define ViewportArguments_h

#include "FloatSize.h"
#include <wtf/Forward.h>

namespace WebCore {

class Document;

enum ViewportErrorCode {
    UnrecognizedViewportArgumentKeyError,
    UnrecognizedViewportArgumentValueError,
    TruncatedViewportArgumentValueError,
    MaximumScaleTooLargeError,
    TargetDensityDpiUnsupported
};

struct ViewportAttributes {
    FloatSize layoutSize;

    float devicePixelRatio;

    float initialScale;
    float minimumScale;
    float maximumScale;

    float userScalable;
};

struct ViewportArguments {

    enum Type {
        // These are ordered in increasing importance.
        Implicit,
#if ENABLE(LEGACY_VIEWPORT_ADAPTION)
        XHTMLMobileProfile,
        HandheldFriendlyMeta,
        MobileOptimizedMeta,
#endif
        ViewportMeta
    } type;

    enum {
        ValueAuto = -1,
        ValueDesktopWidth = -2,
        ValueDeviceWidth = -3,
        ValueDeviceHeight = -4,
    };

    ViewportArguments(Type type = Implicit)
        : type(type)
        , initialScale(ValueAuto)
        , minimumScale(ValueAuto)
        , maximumScale(ValueAuto)
        , width(ValueAuto)
        , height(ValueAuto)
        , userScalable(ValueAuto)
    {
    }

    float initialScale;
    float minimumScale;
    float maximumScale;
    float width;
    float height;
    float userScalable;

    bool operator==(const ViewportArguments& other) const
    {
        // Used for figuring out whether to reset the viewport or not,
        // thus we are not taking type into account.
        return initialScale == other.initialScale
            && minimumScale == other.minimumScale
            && maximumScale == other.maximumScale
            && width == other.width
            && height == other.height
            && userScalable == other.userScalable;
    }

    bool operator!=(const ViewportArguments& other) const
    {
        return !(*this == other);
    }

    // FIXME: We're going to keep this constant around until all embedders
    // refactor their code to no longer need it.
    static const float deprecatedTargetDPI;
};

ViewportAttributes computeViewportAttributes(ViewportArguments args, int desktopWidth, int deviceWidth, int deviceHeight, float devicePixelRatio, IntSize visibleViewport);

void restrictMinimumScaleFactorToViewportSize(ViewportAttributes& result, IntSize visibleViewport);
void restrictScaleFactorToInitialScaleIfNotUserScalable(ViewportAttributes& result);
float computeMinimumScaleFactorForContentContained(const ViewportAttributes& result, const IntSize& viewportSize, const IntSize& contentSize);

void setViewportFeature(const String& keyString, const String& valueString, Document*, void* data);
void reportViewportWarning(Document*, ViewportErrorCode, const String& replacement1, const String& replacement2);

} // namespace WebCore

#endif // ViewportArguments_h
