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

#include <WebCore/ChromeClient.h>
#include <WebCore/FloatRect.h>
#include <WebCore/MediaProducer.h>
#include "QtPlatformPlugin.h"
#include <wtf/RefCounted.h>
#include <wtf/URL.h>
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
class FrameLoadRequest;
class QtAbstractWebPopup;
struct ViewportArguments;
#if ENABLE(VIDEO)
class FullScreenVideoQt;
#endif
class TextureMapperLayerClientQt;

class ChromeClientQt final : public ChromeClient {
public:
    ChromeClientQt(QWebPageAdapter*);
    ~ChromeClientQt();
    void chromeDestroyed() final;

    void setWindowRect(const FloatRect&) final;
    FloatRect windowRect() final;

    FloatRect pageRect() final;

    void focus() final;
    void unfocus() final;

    bool canTakeFocus(FocusDirection) final;
    void takeFocus(FocusDirection) final;

    void focusedElementChanged(Element*) final;
    void focusedFrameChanged(Frame*) final;

    Page* createWindow(Frame&, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&) final;
    void show() final;

    bool canRunModal() final;
    void runModal() final;

    void setToolbarsVisible(bool) final;
    bool toolbarsVisible() final;

    void setStatusbarVisible(bool) final;
    bool statusbarVisible() final;

    void setScrollbarsVisible(bool) final;
    bool scrollbarsVisible() final;

    void setMenubarVisible(bool) final;
    bool menubarVisible() final;

    void setResizable(bool) final;

    void addMessageToConsole(MessageSource, MessageLevel, const String& message, unsigned lineNumber, unsigned columnNumber, const String& sourceID) final;

    bool canRunBeforeUnloadConfirmPanel() final;
    bool runBeforeUnloadConfirmPanel(const String& message, Frame&) final;

    void closeWindowSoon() final;

    void runJavaScriptAlert(Frame&, const String&) final;
    bool runJavaScriptConfirm(Frame&, const String&) final;
    bool runJavaScriptPrompt(Frame&, const String& message, const String& defaultValue, String& result) final;

    void setStatusbarText(const String&) final;

    KeyboardUIMode keyboardUIMode() final;

    void invalidateRootView(const IntRect&) final;
    void invalidateContentsAndRootView(const IntRect&) final;
    void invalidateContentsForSlowScroll(const IntRect&) final;
    void scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect) final;
#if USE(COORDINATED_GRAPHICS)
    void delegatedScrollRequested(const IntPoint& scrollPoint) final;
#endif

    IntPoint screenToRootView(const IntPoint&) const final;
    IntRect rootViewToScreen(const IntRect&) const final;
    PlatformPageClient platformPageClient() const final;
    void contentsSizeChanged(Frame&, const IntSize&) const final;

    void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags) final;

    void setToolTip(const String&, TextDirection) final;

    void print(Frame&) final;
    void exceededDatabaseQuota(Frame&, const String&, DatabaseDetails) final;
    void reachedMaxAppCacheSize(int64_t spaceNeeded) final;
    void reachedApplicationCacheOriginQuota(SecurityOrigin&, int64_t totalSpaceNeeded) final;

    // This is a hook for WebCore to tell us what we need to do with the GraphicsLayers.
    void attachRootGraphicsLayer(Frame&, GraphicsLayer*) final;
    void setNeedsOneShotDrawingSynchronization() final;
    void scheduleCompositingLayerFlush() final;
    CompositingTriggerFlags allowedCompositingTriggers() const final;
    bool allowsAcceleratedCompositing() const final;

#if USE(TILED_BACKING_STORE)
    virtual IntRect visibleRectForTiledBackingStore() const;
#endif

#if ENABLE(TOUCH_EVENTS)
    void needTouchEvents(bool) final { }
#endif

    void isPlayingMediaDidChange(MediaProducer::MediaStateFlags, uint64_t) final;

#if ENABLE(VIDEO) && ((USE(GSTREAMER) && USE(NATIVE_FULLSCREEN_VIDEO)) || USE(QT_MULTIMEDIA))
    bool supportsVideoFullscreen(MediaPlayerEnums::VideoFullscreenMode) final;
    void enterVideoFullscreenForVideoElement(HTMLVideoElement&, MediaPlayerEnums::VideoFullscreenMode) final;
    void exitVideoFullscreenForVideoElement(WebCore::HTMLVideoElement&) final;
    bool requiresFullscreenForVideoPlayback() final;
    FullScreenVideoQt* fullScreenVideo();
#endif

#if ENABLE(FULLSCREEN_API)
    bool supportsFullScreenForElement(const Element&, bool) final;
    void enterFullScreenForElement(Element&) final;
    void exitFullScreenForElement(Element*) final;
#endif

#if ENABLE(INPUT_TYPE_COLOR)
    std::unique_ptr<ColorChooser> createColorChooser(ColorChooserClient&, const Color&) final;
#endif

#if ENABLE(DATALIST_ELEMENT)
    std::unique_ptr<DataListSuggestionPicker> createDataListSuggestionPicker(DataListSuggestionsClient&) final;
#endif

    void runOpenPanel(Frame&, FileChooser&) final;
    void loadIconForFiles(const Vector<String>&, FileIconLoader&) final;

    void setCursor(const Cursor&) final;
    void setCursorHiddenUntilMouseMoves(bool) final { }

    void scrollRectIntoView(const IntRect&) const final { }

    bool selectItemWritingDirectionIsNatural() final;
    bool selectItemAlignmentFollowsMenuWritingDirection() final;
    RefPtr<PopupMenu> createPopupMenu(PopupMenuClient&) const final;
    RefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient&) const final;

    std::unique_ptr<QWebSelectMethod> createSelectPopup() const;

    void dispatchViewportPropertiesDidChange(const ViewportArguments&) const final;

    void wheelEventHandlersChanged(bool) final { }

    void attachViewOverlayGraphicsLayer(GraphicsLayer*) final;

    IntPoint accessibilityScreenToRootView(const IntPoint&) const;
    IntRect rootViewToAccessibilityScreen(const IntRect&) const;
    void didFinishLoadingImageForElement(HTMLImageElement&);
    void intrinsicContentsSizeChanged(const IntSize&) const;
    RefPtr<Icon> createIconForFiles(const Vector<WTF::String>&);

    QWebFullScreenVideoHandler* createFullScreenVideoHandler();

    QWebPageAdapter* m_webPage;
    URL lastHoverURL;
    String lastHoverTitle;
    String lastHoverContent;

    bool toolBarsVisible;
    bool statusBarVisible;
    bool menuBarVisible;
    QEventLoop* m_eventLoop;
    MediaProducer::MediaStateFlags m_mediaState { WebCore::MediaProducer::IsNotPlaying };

#if ENABLE(VIDEO) && (USE(GSTREAMER) || USE(QT_MULTIMEDIA))
    FullScreenVideoQt* m_fullScreenVideo;
#endif

    static bool dumpVisitedLinksCallbacks;

    mutable QtPlatformPlugin m_platformPlugin;

#if USE(TEXTURE_MAPPER)
    std::unique_ptr<TextureMapperLayerClientQt> m_textureMapperLayerClient;
#endif
};
}

#endif
