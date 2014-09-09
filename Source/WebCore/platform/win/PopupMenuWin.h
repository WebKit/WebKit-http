/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef PopupMenuWin_h
#define PopupMenuWin_h

#include "IntRect.h"
#include "PopupMenu.h"
#include "PopupMenuClient.h"
#include "ScrollableArea.h"
#include "Scrollbar.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/win/GDIObject.h>

namespace WebCore {

class FrameView;
class Scrollbar;

class PopupMenuWin : public PopupMenu, private ScrollableArea {
public:
    PopupMenuWin(PopupMenuClient*);
    ~PopupMenuWin();

    virtual void show(const IntRect&, FrameView*, int index);
    virtual void hide();
    virtual void updateFromElement();
    virtual void disconnectClient();

    static LPCWSTR popupClassName();

private:
    PopupMenuClient* client() const { return m_popupClient; }

    Scrollbar* scrollbar() const { return m_scrollbar.get(); }

    bool up(unsigned lines = 1);
    bool down(unsigned lines = 1);

    int itemHeight() const { return m_itemHeight; }
    const IntRect& windowRect() const { return m_windowRect; }
    IntRect clientRect() const;

    int visibleItems() const;

    int listIndexAtPoint(const IntPoint&) const;

    bool setFocusedIndex(int index, bool hotTracking = false);
    int focusedIndex() const;
    void focusFirst();
    void focusLast();

    void paint(const IntRect& damageRect, HDC = 0);

    HWND popupHandle() const { return m_popup; }

    void setWasClicked(bool b = true) { m_wasClicked = b; }
    bool wasClicked() const { return m_wasClicked; }

    int scrollOffset() const { return m_scrollOffset; }

    bool scrollToRevealSelection();

    void incrementWheelDelta(int delta);
    void reduceWheelDelta(int delta);
    int wheelDelta() const { return m_wheelDelta; }

    bool scrollbarCapturingMouse() const { return m_scrollbarCapturingMouse; }
    void setScrollbarCapturingMouse(bool b) { m_scrollbarCapturingMouse = b; }

    // ScrollableArea
    virtual int scrollSize(ScrollbarOrientation) const override;
    virtual int scrollPosition(Scrollbar*) const override;
    virtual void setScrollOffset(const IntPoint&) override;
    virtual void invalidateScrollbarRect(Scrollbar*, const IntRect&) override;
    virtual void invalidateScrollCornerRect(const IntRect&) override { }
    virtual bool isActive() const override { return true; }
    ScrollableArea* enclosingScrollableArea() const override { return 0; }
    virtual bool isScrollCornerVisible() const override { return false; }
    virtual IntRect scrollCornerRect() const override { return IntRect(); }
    virtual Scrollbar* verticalScrollbar() const override { return m_scrollbar.get(); }
    virtual IntSize visibleSize() const override;
    virtual IntSize contentsSize() const override;
    virtual IntRect scrollableAreaBoundingBox() const override;
    virtual bool updatesScrollLayerPositionOnMainThread() const override { return true; }
    virtual bool forceUpdateScrollbarsOnMainThreadForPerformanceTesting() const override { return false; }

    // NOTE: This should only be called by the overriden setScrollOffset from ScrollableArea.
    void scrollTo(int offset);

    void calculatePositionAndSize(const IntRect&, FrameView*);
    void invalidateItem(int index);

    static LRESULT CALLBACK PopupMenuWndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT wndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);
    static void registerClass();

    PopupMenuClient* m_popupClient;
    RefPtr<Scrollbar> m_scrollbar;
    HWND m_popup;
    GDIObject<HDC> m_DC;
    GDIObject<HBITMAP> m_bmp;
    bool m_wasClicked;
    IntRect m_windowRect;
    int m_itemHeight;
    int m_scrollOffset;
    int m_wheelDelta;
    int m_focusedIndex;
    int m_hoveredIndex;
    bool m_scrollbarCapturingMouse;
    bool m_showPopup;
};

} // namespace WebCore

#endif // PopupMenuWin_h
