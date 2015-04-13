/*
 * Copyright (C) 2006 Zack Rusin <zack@kde.org>
 * Copyright (C) 2007 Ryan Leavengood <leavengood@gmail.com> All rights reserved.
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com> All rights reserved.
 *
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

#ifndef ChromeClientHaiku_h
#define ChromeClientHaiku_h

#include "ChromeClient.h"
#include "FloatRect.h"
#include "NotImplemented.h"
#include "URL.h"

#include <Entry.h>

#include <wtf/RefCounted.h>
#include "WebPage.h"

namespace WebCore {

    class Page;
    class WindowFeatures;
    struct FrameLoadRequest;

    class ChromeClientHaiku : public ChromeClient {
    public:
        ChromeClientHaiku(BWebPage*, BWebView*);
        virtual ~ChromeClientHaiku();
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

        void addMessageToConsole(MessageSource, MessageLevel,
                                         const String& message, unsigned int lineNumber, unsigned columnNumber, const String& sourceID) override;

        bool canRunBeforeUnloadConfirmPanel() override;
        bool runBeforeUnloadConfirmPanel(const String& message, Frame* frame) override;

        void closeWindowSoon() override;

        void runJavaScriptAlert(Frame*, const String&) override;
        bool runJavaScriptConfirm(Frame*, const String&) override;
        bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result) override;
        std::unique_ptr<ColorChooser> createColorChooser(ColorChooserClient*, const Color&) override;

        bool shouldInterruptJavaScript() override;
        KeyboardUIMode keyboardUIMode() override;

        void setStatusbarText(const String&) override;

        IntRect windowResizerRect() const override;

        void invalidateRootView(const IntRect&) override;
        void invalidateContentsAndRootView(const IntRect&) override;

        void invalidateContentsForSlowScroll(const IntRect&) override;
        void scroll(const IntSize& scrollDelta, const IntRect& rectToScroll, const IntRect& clipRect) override;

        IntPoint screenToRootView(const IntPoint&) const override;
        IntRect rootViewToScreen(const IntRect&) const override;

        PlatformPageClient platformPageClient() const override;
        void contentsSizeChanged(Frame*, const IntSize&) const override;
        void scrollRectIntoView(const IntRect&) const override;
        void attachViewOverlayGraphicsLayer(WebCore::Frame* frame, WebCore::GraphicsLayer* layer) override;

        void scrollbarsModeDidChange() const override { }
        void setCursor(const Cursor&) override ;
        void setCursorHiddenUntilMouseMoves(bool) override { }

#if ENABLE(REQUEST_ANIMATION_FRAME) && !USE(REQUEST_ANIMATION_FRAME_TIMER)
        void scheduleAnimation() override;
#endif

        void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags) override;

        void setToolTip(const String&, TextDirection) override;

        void print(Frame*) override;

        void exceededDatabaseQuota(Frame*, const String& databaseName, DatabaseDetails) override;
        void reachedMaxAppCacheSize(int64_t spaceNeeded) override;
        void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t totalSpaceNeeded) override;

        void attachRootGraphicsLayer(Frame*, GraphicsLayer*) override;
        void setNeedsOneShotDrawingSynchronization() override;
        void scheduleCompositingLayerFlush() override;

        void runOpenPanel(Frame*, PassRefPtr<FileChooser>) override;
        void setPanelDirectory(entry_ref dir) { m_filePanelDirectory = dir; }

        // Asynchronous request to load an icon for specified filenames.
        void loadIconForFiles(const Vector<String>&, FileIconLoader*) override;

        bool selectItemWritingDirectionIsNatural() override;
        bool selectItemAlignmentFollowsMenuWritingDirection() override;
        bool hasOpenedPopup() const override;
        PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const override;
        PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const override;

        void wheelEventHandlersChanged(bool) override { }

#if ENABLE(POINTER_LOCK)
        bool requestPointerLock() override;
        void requestPointerUnlock() override;
        bool isPointerLocked() override;
#endif

#if USE(TILED_BACKING_STORE)
        void delegatedScrollRequested(const WebCore::IntPoint& pos) override;
#endif
    private:
        BWebPage* m_webPage;
        BWebView* m_webView;

        URL lastHoverURL;
        String lastHoverTitle;
        String lastHoverContent;

        entry_ref m_filePanelDirectory;
    };

} // namespace WebCore

#endif

