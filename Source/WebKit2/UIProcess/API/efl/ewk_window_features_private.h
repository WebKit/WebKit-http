/*
 * Copyright (C) 2012 Samsung Electronics. All rights reserved.
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

#ifndef ewk_window_features_private_h
#define ewk_window_features_private_h

#include "EwkViewImpl.h"
#include "ImmutableDictionary.h"
#include "WindowFeatures.h"
#include "ewk_object_private.h"
#include <WebCore/FloatRect.h>
#include <wtf/RefCounted.h>

class EwkViewImpl;

class EwkWindowFeatures : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkWindowFeatures)

    static PassRefPtr<EwkWindowFeatures> create(WebKit::ImmutableDictionary* windowFeatures, EwkViewImpl* viewImpl)
    {
        return adoptRef(new EwkWindowFeatures(windowFeatures, viewImpl));
    }

    WebCore::FloatRect geometry() const { return m_geometry; }
    void setGeometry(WebCore::FloatRect& geometry) { m_geometry = geometry; }

    bool toolbarVisible() const { return m_toolbarVisible; }
    void setToolbarVisible(bool toolbarVisible);

    bool statusBarVisible() const { return m_statusBarVisible; }
    void setStatusBarVisible(bool statusBarVisible);

    bool scrollbarsVisible() const { return m_scrollbarsVisible; }
    void setScrollbarsVisible(bool scrollbarsVisible) { m_scrollbarsVisible = scrollbarsVisible; }

    bool menuBarVisible() const { return m_menuBarVisible; }
    void setMenuBarVisible(bool menuBarVisible);

    bool locationBarVisible() const { return m_locationBarVisible; }
    void setLocationBarVisible(bool locationBarVisible) { m_locationBarVisible = locationBarVisible; }

    bool resizable() const { return m_resizable; }
    void setResizable(bool resizable);

    bool fullScreen() const { return m_fullScreen; }
    void setFullScreen(bool fullScreen) { m_fullScreen = fullScreen; }

private:
    EwkWindowFeatures(WebKit::ImmutableDictionary* windowFeatures, EwkViewImpl* viewImpl);
    template <typename T1, typename T2>
    static T1 getWindowFeatureValue(WebKit::ImmutableDictionary* windowFeatures, const String& featureName);

    EwkViewImpl* m_viewImpl;

    WebCore::FloatRect m_geometry;
    bool m_toolbarVisible;
    bool m_statusBarVisible;
    bool m_scrollbarsVisible;
    bool m_menuBarVisible;
    bool m_locationBarVisible;
    bool m_resizable;
    bool m_fullScreen;
};

#endif // ewk_window_features_private_h
