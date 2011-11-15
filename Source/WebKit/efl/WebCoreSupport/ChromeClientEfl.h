/*
 * Copyright (C) 2007 Holger Hans Peter Freyther
 * Copyright (C) 2008 INdT - Instituto Nokia de Tecnologia
 * Copyright (C) 2009-2010 ProFUSION embedded systems
 * Copyright (C) 2009-2010 Samsung Electronics
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
 */

#ifndef ChromeClientEfl_h
#define ChromeClientEfl_h

#include "ChromeClient.h"
#include "KURL.h"
#include "PopupMenu.h"

typedef struct _Evas_Object Evas_Object;

namespace WebCore {

class ChromeClientEfl : public ChromeClient {
public:
    explicit ChromeClientEfl(Evas_Object* view);
    virtual ~ChromeClientEfl();

    virtual void chromeDestroyed();

    virtual void* webView() const { return 0; }
    virtual void setWindowRect(const FloatRect&);
    virtual FloatRect windowRect();

    virtual FloatRect pageRect();

    virtual void focus();
    virtual void unfocus();

    virtual bool canTakeFocus(FocusDirection);
    virtual void takeFocus(FocusDirection);

    virtual void focusedNodeChanged(Node*);
    virtual void focusedFrameChanged(Frame*);

    virtual Page* createWindow(Frame*, const FrameLoadRequest&, const WindowFeatures&, const NavigationAction&);
    virtual void show();

    virtual bool canRunModal();
    virtual void runModal();

    virtual void setToolbarsVisible(bool);
    virtual bool toolbarsVisible();

    virtual void setStatusbarVisible(bool);
    virtual bool statusbarVisible();

    virtual void setScrollbarsVisible(bool);
    virtual bool scrollbarsVisible();

    virtual void setMenubarVisible(bool);
    virtual bool menubarVisible();

    virtual void createSelectPopup(PopupMenuClient*, int selected, const IntRect&);
    virtual bool destroySelectPopup();

    virtual void setResizable(bool);

    virtual void addMessageToConsole(MessageSource, MessageType, MessageLevel, const String& message,
                                     unsigned int lineNumber, const String& sourceID);

    virtual bool canRunBeforeUnloadConfirmPanel();
    virtual bool runBeforeUnloadConfirmPanel(const String& message, Frame*);

    virtual void closeWindowSoon();

    virtual void runJavaScriptAlert(Frame*, const String&);
    virtual bool runJavaScriptConfirm(Frame*, const String&);
    virtual bool runJavaScriptPrompt(Frame*, const String& message, const String& defaultValue, String& result);
    virtual void setStatusbarText(const String&);
    virtual bool shouldInterruptJavaScript();
    virtual WebCore::KeyboardUIMode keyboardUIMode();

    virtual IntRect windowResizerRect() const;

    virtual void contentsSizeChanged(Frame*, const IntSize&) const;
    virtual IntPoint screenToRootView(const IntPoint&) const;
    virtual IntRect rootViewToScreen(const IntRect&) const;
    virtual PlatformPageClient platformPageClient() const;

    virtual void scrollbarsModeDidChange() const;
    virtual void mouseDidMoveOverElement(const HitTestResult&, unsigned modifierFlags);

    virtual void setToolTip(const String&, TextDirection);

    virtual void print(Frame*);

#if ENABLE(SQL_DATABASE)
    virtual void exceededDatabaseQuota(Frame*, const String&);
#endif

#if ENABLE(NOTIFICATIONS)
    virtual NotificationPresenter* notificationPresenter() const;
#endif

    virtual void reachedMaxAppCacheSize(int64_t spaceNeeded);
    virtual void reachedApplicationCacheOriginQuota(SecurityOrigin*, int64_t totalSpaceNeeded);

#if ENABLE(CONTEXT_MENUS)
    virtual void showContextMenu() { }
#endif

#if ENABLE(TOUCH_EVENTS)
    virtual void needTouchEvents(bool);
#endif

#if USE(ACCELERATED_COMPOSITING)
    virtual void attachRootGraphicsLayer(Frame*, GraphicsLayer*);
    virtual void setNeedsOneShotDrawingSynchronization();
    virtual void scheduleCompositingLayerSync();
    virtual CompositingTriggerFlags allowedCompositingTriggers() const;
#endif

#if ENABLE(FULLSCREEN_API)
    virtual bool supportsFullScreenForElement(const WebCore::Element*, bool withKeyboard);
    virtual void enterFullScreenForElement(WebCore::Element*);
    virtual void exitFullScreenForElement(WebCore::Element*);
#endif

    virtual void runOpenPanel(Frame*, PassRefPtr<FileChooser>);
    virtual void loadIconForFiles(const Vector<String>&, FileIconLoader*);
    virtual void formStateDidChange(const Node*);

    virtual void setCursor(const Cursor&);
    virtual void setCursorHiddenUntilMouseMoves(bool);

    virtual void scrollRectIntoView(const IntRect&) const { }

    virtual void requestGeolocationPermissionForFrame(Frame*, Geolocation*);
    virtual void cancelGeolocationPermissionRequestForFrame(Frame*, Geolocation*);
    virtual void cancelGeolocationPermissionForFrame(Frame*, Geolocation*);

    virtual void invalidateContents(const IntRect&, bool);
    virtual void invalidateRootView(const IntRect&, bool);
    virtual void invalidateContentsAndRootView(const IntRect&, bool);
    virtual void invalidateContentsForSlowScroll(const IntRect&, bool);
    virtual void scroll(const IntSize&, const IntRect&, const IntRect&);
    virtual void cancelGeolocationPermissionRequestForFrame(Frame*);
    virtual void iconForFiles(const Vector<String, 0u>&, PassRefPtr<FileChooser>);

    virtual void dispatchViewportPropertiesDidChange(const ViewportArguments&) const;

    virtual bool selectItemWritingDirectionIsNatural();
    virtual bool selectItemAlignmentFollowsMenuWritingDirection();
    virtual PassRefPtr<PopupMenu> createPopupMenu(PopupMenuClient*) const;
    virtual PassRefPtr<SearchPopupMenu> createSearchPopupMenu(PopupMenuClient*) const;

    virtual bool shouldRubberBandInDirection(WebCore::ScrollDirection) const { return true; }
    virtual void numWheelEventHandlersChanged(unsigned) { }

    Evas_Object* m_view;
    KURL m_hoveredLinkURL;
};
}

#endif // ChromeClientEfl_h
