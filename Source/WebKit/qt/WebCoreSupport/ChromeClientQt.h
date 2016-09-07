/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ChromeClientQt_h
#define ChromeClientQt_h

#include "ChromeClient.h"
#include "FloatRect.h"
#include "MediaProducer.h"
#include "QtPlatformPlugin.h"
#include "URL.h"
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

QT_BEGIN_NAMESPACE
class QEventLoop;
QT_END_NAMESPACE

class QWebPage;
class QWebPageAdapter;
class QWebFullScreenVideoHandler;

namespace WebCore {

class FileChooser;
class FileIconLoader;
class FloatRect;
class Page;
class RefreshAnimation;
struct FrameLoadRequest;
class QtAbstractWebPopup;
struct ViewportArguments;
#if ENABLE(VIDEO)
class FullScreenVideoQt;
#endif
class TextureMapperLayerClientQt;

class ChromeClientQt final : public ChromeClient {
public:
    ChromeClientQt(QWebPageAdapter*);
    virtual ~ChromeClientQt();
    void chromeDestroyed() override;

    void setWindowRect(const FloatRect&) override;
    FloatRect windowRect() override;

    FloatRect pageRect() override;

    void focus() override;
    void unfocus() override;

    bool canTakeFocus(FocusDirection) override;
    void takeFocus(FocusDirection) override;

    void focusedElementChanged(Element*) override;
    void focusedFrameChanged(Frame*) override;

    Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&) override;
    void show() override;

    bool canRunModal() override;
    void runModal() override;

    void setToolbarsVisible(bool) override;
    bool toolbarsVisible() override;

    void setStatusbarVisible(bool) override;
    bool statusbarVisible() override;

    void setScrollbarsVisible(bool) override;
    bool scrollbarsVisible() override;

    void setMenubarVisible(bool) override;
    bool menubarVisible() override;

    void setResizable(bool) override;

    void addMessageToConsole(MessageSource, MessageLevel, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID) override;

    bool canRunBeforeUnloadConfirmPanel() override;
    bool runBeforeUnloadConfirmPanel(const String& message, Frame*) override;

    void closeWindowSoon() override;

    void runJavaScriptAlert(Frame*, const String&) override;
    bool runJavaScriptConfirm(Frame*, const String&) override;
    bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result) override;
    virtual bool shouldInterruptJavaScript();

    void setStatusbarText(const String&) override;

    KeyboardUIMode keyboardUIMode() override;

    void invalidateRootView(const IntRect&) override;
    void invalidateContentsAndRootView(const IntRect&) override;
    void invalidateContentsForSlowScroll(const IntRect&) override;
    void scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect) override;
#if USE(TILED_BACKING_STORE)
    virtual void delegatedScrollRequested(const IntPoint& scrollPoint);
#endif

    IntPoint screenToRootView(const IntPoint&) const override;
    IntRect rootViewToScreen(const IntRect&) const override;
    PlatformPageClient platformPageClient() const override;
    void contentsSizeChanged(Frame*, const IntSize&) const override;

    void scrollbarsModeDidChange() const override { }
    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags) override;

    void setToolTip(const String&, TextDirection) override;

    void print(Frame*) override;
    void exceededDatabaseQuota(Frame*, const String&, DatabaseDetails) override;
    void reachedMaxAppCacheSize(int64_t spaceNeeded) override;
    void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t totalSpaceNeeded) override;

    // This is a hook for WebCore to tell us what we need to do with the GraphicsLayers.
    void attachRootGraphicsLayer(Frame*, GraphicsLayer*) override;
    void setNeedsOneShotDrawingSynchronization() override;
    void scheduleCompositingLayerFlush() override;
    CompositingTriggerFlags allowedCompositingTriggers() const override;
    bool allowsAcceleratedCompositing() const override;

#if USE(TILED_BACKING_STORE)
    virtual IntRect visibleRectForTiledBackingStore() const;
#endif

#if ENABLE(TOUCH_EVENTS)
    void needTouchEvents(bool) override { }
#endif

    void isPlayingMediaDidChange(MediaProducer::MediaStateFlags, uint64_t) override;

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    bool supportsVideoFullscreen(MediaPlayerEnums::VideoFullscreenMode) override;
    void enterVideoFullscreenForVideoElement(HTMLVideoElement&, MediaPlayerEnums::VideoFullscreenMode) override;
    void exitVideoFullscreenForVideoElement(WebCore::HTMLVideoElement&) override;
    bool requiresFullscreenForVideoPlayback() override;
    FullScreenVideoQt* fullScreenVideo();
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    std::unique_ptr<ColorChooser> createColorChooser(ColorChooserClient*, const Color&) override;
#endif

    void runOpenPanel(Frame*, PassRefPtr<FileChooser>) override;
    void loadIconForFiles(const Vector<String>&, FileIconLoader*) override;

    void setCursor(const Cursor&) override;
    void setCursorHiddenUntilMouseMoves(bool) override { }

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    void scheduleAnimation() override;
    void serviceScriptedAnimations();
#endif

    void scrollRectIntoView(const IntRect&) const override { }

    bool selectItemWritingDirectionIsNatural() override;
    bool selectItemAlignmentFollowsMenuWritingDirection() override;
    bool hasOpenedPopup() const override;
    RefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const override;
    RefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const override;

    std::unique_ptr<QWebSelectMethod> createSelectPopup() const;

    void dispatchViewportPropertiesDidChange(const ViewportArguments&) const override;

    void wheelEventHandlersChanged(bool) override { }

    void attachViewOverlayGraphicsLayer(Frame *, GraphicsLayer *) override;

    QWebFullScreenVideoHandler* createFullScreenVideoHandler();

    QWebPageAdapter* m_webPage;
    URL lastHoverURL;
    String lastHoverTitle;
    String lastHoverContent;

    bool toolBarsVisible;
    bool statusBarVisible;
    bool menuBarVisible;
    QEventLoop* m_eventLoop;
#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
    std::unique_ptr<RefreshAnimation> m_refreshAnimation;
#endif
    MediaProducer::MediaStateFlags m_mediaState { WebCore::MediaProducer::IsNotPlaying };

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA))
    FullScreenVideoQt* m_fullScreenVideo;
#endif

    static bool dumpVisitedLinksCallbacks;

    mutable QtPlatformPlugin m_platformPlugin;

    std::unique_ptr<TextureMapperLayerClientQt> m_textureMapperLayerClient;
};
}

#endif
