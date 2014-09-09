/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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

#ifndef PageOverlay_h
#define PageOverlay_h

#include "APIObject.h"
#include "WKBase.h"
#include <WebCore/Color.h>
#include <WebCore/IntRect.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RunLoop.h>

namespace WebCore {
class Frame;
class GraphicsContext;
class GraphicsLayer;
}

namespace WebKit {

class WebFrame;
class WebMouseEvent;
class WebPage;

class PageOverlay : public API::ObjectImpl<API::Object::Type::BundlePageOverlay> {
public:
    class Client {
    protected:
        virtual ~Client() { }
    
    public:
        virtual void pageOverlayDestroyed(PageOverlay*) = 0;
        virtual void willMoveToWebPage(PageOverlay*, WebPage*) = 0;
        virtual void didMoveToWebPage(PageOverlay*, WebPage*) = 0;
        virtual void drawRect(PageOverlay*, WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect) = 0;
        virtual bool mouseEvent(PageOverlay*, const WebMouseEvent&) = 0;
        virtual void didScrollFrame(PageOverlay*, WebCore::Frame*) { }

        virtual WKTypeRef copyAccessibilityAttributeValue(PageOverlay*, WKStringRef /* attribute */, WKTypeRef /* parameter */) { return 0; }
        virtual WKArrayRef copyAccessibilityAttributeNames(PageOverlay*, bool /* parameterizedNames */) { return 0; }
    };

    enum class OverlayType {
        View, // Fixed to the view size; does not scale or scroll with the document, repaints on scroll.
        Document, // Scales and scrolls with the document.
    };

    static PassRefPtr<PageOverlay> create(Client*, OverlayType = OverlayType::View);
    virtual ~PageOverlay();

    void setPage(WebPage*);
    void setNeedsDisplay(const WebCore::IntRect& dirtyRect);
    void setNeedsDisplay();

    void drawRect(WebCore::GraphicsContext&, const WebCore::IntRect& dirtyRect);
    bool mouseEvent(const WebMouseEvent&);
    void didScrollFrame(WebCore::Frame*);

    WKTypeRef copyAccessibilityAttributeValue(WKStringRef attribute, WKTypeRef parameter);
    WKArrayRef copyAccessibilityAttributeNames(bool parameterizedNames);
    
    void startFadeInAnimation();
    void startFadeOutAnimation();
    void stopFadeOutAnimation();

    void clear();

    Client* client() const { return m_client; }

    enum class FadeMode { DoNotFade, Fade };

    OverlayType overlayType() { return m_overlayType; }

    WebCore::IntRect bounds() const;
    WebCore::IntRect frame() const;
    void setFrame(WebCore::IntRect);

    WebCore::RGBA32 backgroundColor() const { return m_backgroundColor; }
    void setBackgroundColor(WebCore::RGBA32);

    WebCore::GraphicsLayer* layer();
    
protected:
    explicit PageOverlay(Client*, OverlayType);

private:
    void startFadeAnimation();
    void fadeAnimationTimerFired();

    Client* m_client;
    WebPage* m_webPage;

    RunLoop::Timer<PageOverlay> m_fadeAnimationTimer;
    double m_fadeAnimationStartTime;
    double m_fadeAnimationDuration;

    enum FadeAnimationType {
        NoAnimation,
        FadeInAnimation,
        FadeOutAnimation,
    };

    FadeAnimationType m_fadeAnimationType;
    float m_fractionFadedIn;

    OverlayType m_overlayType;
    WebCore::IntRect m_overrideFrame;

    WebCore::RGBA32 m_backgroundColor;
};

} // namespace WebKit

#endif // PageOverlay_h
