/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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


#ifndef WebVideoFullscreenInterfaceAVKit_h
#define WebVideoFullscreenInterfaceAVKit_h

#if PLATFORM(IOS) && __IPHONE_OS_VERSION_MIN_REQUIRED >= 80000

#include <WebCore/EventListener.h>
#include <WebCore/PlatformLayer.h>
#include <WebCore/WebVideoFullscreenInterface.h>
#include <wtf/RefPtr.h>
#include <wtf/RetainPtr.h>

OBJC_CLASS WebAVPlayerController;
OBJC_CLASS AVPlayerViewController;
OBJC_CLASS UIViewController;
OBJC_CLASS UIWindow;
OBJC_CLASS UIView;
OBJC_CLASS CALayer;
OBJC_CLASS WebAVVideoLayer;

namespace WTF {
class String;
}

namespace WebCore {
class IntRect;
class WebVideoFullscreenModel;
    
class WebVideoFullscreenChangeObserver {
public:
    virtual ~WebVideoFullscreenChangeObserver() { };
    virtual void didSetupFullscreen() = 0;
    virtual void didEnterFullscreen() = 0;
    virtual void didExitFullscreen() = 0;
    virtual void didCleanupFullscreen() = 0;
};

class WebVideoFullscreenInterfaceAVKit
    : public WebVideoFullscreenInterface
    , public RefCounted<WebVideoFullscreenInterfaceAVKit> {
        
    RetainPtr<WebAVPlayerController> m_playerController;
    RetainPtr<AVPlayerViewController> m_playerViewController;
    RetainPtr<CALayer> m_videoLayer;
    RetainPtr<WebAVVideoLayer> m_videoLayerContainer;
    WebVideoFullscreenModel* m_videoFullscreenModel;
    WebVideoFullscreenChangeObserver* m_fullscreenChangeObserver;

    // These are only used when fullscreen is presented in a separate window.
    RetainPtr<UIWindow> m_window;
    RetainPtr<UIViewController> m_viewController;
    RetainPtr<UIView> m_parentView;

    WebAVPlayerController *playerController();
    
    void doEnterFullscreen();
        
public:
    WEBCORE_EXPORT WebVideoFullscreenInterfaceAVKit();
    virtual ~WebVideoFullscreenInterfaceAVKit() { }
    WEBCORE_EXPORT void setWebVideoFullscreenModel(WebVideoFullscreenModel*);
    WEBCORE_EXPORT void setWebVideoFullscreenChangeObserver(WebVideoFullscreenChangeObserver*);
    
    WEBCORE_EXPORT virtual void setDuration(double) override;
    WEBCORE_EXPORT virtual void setCurrentTime(double currentTime, double anchorTime) override;
    WEBCORE_EXPORT virtual void setRate(bool isPlaying, float playbackRate) override;
    WEBCORE_EXPORT virtual void setVideoDimensions(bool hasVideo, float width, float height) override;
    WEBCORE_EXPORT virtual void setSeekableRanges(const TimeRanges&) override;
    WEBCORE_EXPORT virtual void setCanPlayFastReverse(bool) override;
    WEBCORE_EXPORT virtual void setAudioMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT virtual void setLegibleMediaSelectionOptions(const Vector<WTF::String>& options, uint64_t selectedIndex) override;
    WEBCORE_EXPORT virtual void setExternalPlayback(bool enabled, ExternalPlaybackTargetType, WTF::String localizedDeviceName) override;

    WEBCORE_EXPORT virtual void setupFullscreen(PlatformLayer&, IntRect initialRect, UIView *);
    WEBCORE_EXPORT virtual void enterFullscreen();
    WEBCORE_EXPORT virtual void exitFullscreen(IntRect finalRect);
    WEBCORE_EXPORT virtual void cleanupFullscreen();
    WEBCORE_EXPORT virtual void invalidate();
    WEBCORE_EXPORT virtual void requestHideAndExitFullscreen();
};

}

#endif

#endif
