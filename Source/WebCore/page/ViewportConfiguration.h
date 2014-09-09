/*
 * Copyright (C) 2005-2014 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ViewportConfiguration_h
#define ViewportConfiguration_h

#include "FloatSize.h"
#include "IntSize.h"
#include "ViewportArguments.h"
#include <wtf/Noncopyable.h>

namespace WebCore {

class ViewportConfiguration {
    WTF_MAKE_NONCOPYABLE(ViewportConfiguration); WTF_MAKE_FAST_ALLOCATED;
public:
    // FIXME: unify with ViewportArguments.
    struct Parameters {
        Parameters()
            : width(0)
            , height(0)
            , initialScale(0)
            , minimumScale(0)
            , maximumScale(0)
            , allowsUserScaling(false)
            , widthIsSet(false)
            , heightIsSet(false)
            , initialScaleIsSet(false)
        {
        }

        double width;
        double height;
        double initialScale;
        double minimumScale;
        double maximumScale;
        bool allowsUserScaling;

        bool widthIsSet;
        bool heightIsSet;
        bool initialScaleIsSet;
    };

    ViewportConfiguration();

    const Parameters& defaultConfiguration() const { return m_defaultConfiguration; }
    void setDefaultConfiguration(const Parameters&);

    const IntSize& contentsSize() const { return m_contentSize; }
    void setContentsSize(const IntSize&);

    const FloatSize& minimumLayoutSize() const { return m_minimumLayoutSize; }
    void setMinimumLayoutSize(const FloatSize&);

    const FloatSize& minimumLayoutSizeForMinimalUI() const { return m_minimumLayoutSizeForMinimalUI.isEmpty() ? m_minimumLayoutSize : m_minimumLayoutSizeForMinimalUI; }
    void setMinimumLayoutSizeForMinimalUI(const FloatSize&);

    const FloatSize& activeMinimumLayoutSizeInScrollViewCoordinates() const;

    const ViewportArguments& viewportArguments() const { return m_viewportArguments; }
    void setViewportArguments(const ViewportArguments&);

    void resetMinimalUI();
    void didFinishDocumentLoad();

    IntSize layoutSize() const;
    double initialScale() const;
    double minimumScale() const;
    double maximumScale() const { return m_configuration.maximumScale; }
    bool allowsUserScaling() const { return m_configuration.allowsUserScaling; }
    bool usesMinimalUI() const { return m_usesMinimalUI; }

    static Parameters webpageParameters();
    static Parameters textDocumentParameters();
    static Parameters imageDocumentParameters();
    static Parameters xhtmlMobileParameters();
    static Parameters testingParameters();
    
#ifndef NDEBUG
    WTF::CString description() const;
    void dump() const;
#endif
    
private:
    void updateConfiguration();
    int layoutWidth() const;
    int layoutHeight() const;

    Parameters m_configuration;
    Parameters m_defaultConfiguration;
    IntSize m_contentSize;
    FloatSize m_minimumLayoutSize;
    FloatSize m_minimumLayoutSizeForMinimalUI;
    ViewportArguments m_viewportArguments;

    bool m_usesMinimalUI;
    bool m_pageDidFinishDocumentLoad;
};

} // namespace WebCore

#endif // ViewportConfiguration_h
