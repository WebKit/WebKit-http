/*
 * Copyright (c) 2008, 2009, Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "PopupMenuChromium.h"

#include "Chrome.h"
#include "ChromeClientChromium.h"
#include "Font.h"
#include "FontSelector.h"
#include "FrameView.h"
#include "Frame.h"
#include "FramelessScrollView.h"
#include "FramelessScrollViewClient.h"
#include "GraphicsContext.h"
#include "IntRect.h"
#include "KeyboardCodes.h"
#include "Page.h"
#include "PlatformKeyboardEvent.h"
#include "PlatformMouseEvent.h"
#include "PlatformScreen.h"
#include "PlatformWheelEvent.h"
#include "PopupMenuClient.h"
#include "RenderTheme.h"
#include "ScrollbarTheme.h"
#include "StringTruncator.h"
#include "SystemTime.h"
#include "TextRun.h"
#include "UserGestureIndicator.h"
#include <wtf/CurrentTime.h>
#include <wtf/unicode/CharacterNames.h>

using namespace WTF;
using namespace Unicode;

using std::min;
using std::max;

namespace WebCore {

typedef unsigned long long TimeStamp;

static const int kMaxVisibleRows = 20;
static const int kMaxHeight = 500;
static const int kBorderSize = 1;
static const int kTextToLabelPadding = 10;
static const int kLabelToIconPadding = 5;
static const int kMinEndOfLinePadding = 2;
static const TimeStamp kTypeAheadTimeoutMs = 1000;
static const int kLinePaddingHeight = 3; // Padding height put at the top and bottom of each line.

// The settings used for the drop down menu.
// This is the delegate used if none is provided.
static const PopupContainerSettings dropDownSettings = {
    true, // setTextOnIndexChange
    true, // acceptOnAbandon
    false, // loopSelectionNavigation
    false // restrictWidthOfListBox
};

// This class uses WebCore code to paint and handle events for a drop-down list
// box ("combobox" on Windows).
class PopupListBox : public FramelessScrollView {
public:
    static PassRefPtr<PopupListBox> create(PopupMenuClient* client, const PopupContainerSettings& settings)
    {
        return adoptRef(new PopupListBox(client, settings));
    }

    // FramelessScrollView
    virtual void paint(GraphicsContext*, const IntRect&);
    virtual bool handleMouseDownEvent(const PlatformMouseEvent&);
    virtual bool handleMouseMoveEvent(const PlatformMouseEvent&);
    virtual bool handleMouseReleaseEvent(const PlatformMouseEvent&);
    virtual bool handleWheelEvent(const PlatformWheelEvent&);
    virtual bool handleKeyEvent(const PlatformKeyboardEvent&);

    // ScrollView
    virtual HostWindow* hostWindow() const;

    // PopupListBox methods

    // Hides the popup.
    void hidePopup();

    // Updates our internal list to match the client.
    void updateFromElement();

    // Frees any allocated resources used in a particular popup session.
    void clear();

    // Sets the index of the option that is displayed in the <select> widget in the page
    void setOriginalIndex(int index);

    // Gets the index of the item that the user is currently moused over or has selected with
    // the keyboard. This is not the same as the original index, since the user has not yet
    // accepted this input.
    int selectedIndex() const { return m_selectedIndex; }

    // Moves selection down/up the given number of items, scrolling if necessary.
    // Positive is down.  The resulting index will be clamped to the range
    // [0, numItems), and non-option items will be skipped.
    void adjustSelectedIndex(int delta);

    // Returns the number of items in the list.
    int numItems() const { return static_cast<int>(m_items.size()); }

    void setBaseWidth(int width) { m_baseWidth = min(m_maxWindowWidth, width); }

    // Computes the size of widget and children.
    void layout();

    // Returns whether the popup wants to process events for the passed key.
    bool isInterestedInEventForKey(int keyCode);

    // Gets the height of a row.
    int getRowHeight(int index);

    void setMaxHeight(int maxHeight) { m_maxHeight = maxHeight; }

    void setMaxWidthAndLayout(int);

    void disconnectClient() { m_popupClient = 0; }

    const Vector<PopupItem*>& items() const { return m_items; }

private:
    friend class PopupContainer;
    friend class RefCounted<PopupListBox>;

    PopupListBox(PopupMenuClient* client, const PopupContainerSettings& settings)
        : m_settings(settings)
        , m_originalIndex(0)
        , m_selectedIndex(0)
        , m_acceptedIndexOnAbandon(-1)
        , m_visibleRows(0)
        , m_baseWidth(0)
        , m_maxHeight(kMaxHeight)
        , m_popupClient(client)
        , m_repeatingChar(0)
        , m_lastCharTime(0)
        , m_maxWindowWidth(std::numeric_limits<int>::max())
    {
        setScrollbarModes(ScrollbarAlwaysOff, ScrollbarAlwaysOff);
    }

    ~PopupListBox()
    {
        clear();
    }

    // Closes the popup
    void abandon();

    // Returns true if the selection can be changed to index.
    // Disabled items, or labels cannot be selected.
    bool isSelectableItem(int index);

    // Select an index in the list, scrolling if necessary.
    void selectIndex(int index);

    // Accepts the selected index as the value to be displayed in the <select> widget on
    // the web page, and closes the popup. Returns true if index is accepted.
    bool acceptIndex(int index);

    // Clears the selection (so no row appears selected).
    void clearSelection();

    // Scrolls to reveal the given index.
    void scrollToRevealRow(int index);
    void scrollToRevealSelection() { scrollToRevealRow(m_selectedIndex); }

    // Invalidates the row at the given index.
    void invalidateRow(int index);

    // Get the bounds of a row.
    IntRect getRowBounds(int index);

    // Converts a point to an index of the row the point is over
    int pointToRowIndex(const IntPoint&);

    // Paint an individual row
    void paintRow(GraphicsContext*, const IntRect&, int rowIndex);

    // Test if the given point is within the bounds of the popup window.
    bool isPointInBounds(const IntPoint&);

    // Called when the user presses a text key.  Does a prefix-search of the items.
    void typeAheadFind(const PlatformKeyboardEvent&);

    // Returns the font to use for the given row
    Font getRowFont(int index);

    // Moves the selection down/up one item, taking care of looping back to the
    // first/last element if m_loopSelectionNavigation is true.
    void selectPreviousRow();
    void selectNextRow();

    // The settings that specify the behavior for this Popup window.
    PopupContainerSettings m_settings;

    // This is the index of the item marked as "selected" - i.e. displayed in the widget on the
    // page.
    int m_originalIndex;

    // This is the index of the item that the user is hovered over or has selected using the
    // keyboard in the list. They have not confirmed this selection by clicking or pressing
    // enter yet however.
    int m_selectedIndex;

    // If >= 0, this is the index we should accept if the popup is "abandoned".
    // This is used for keyboard navigation, where we want the
    // selection to change immediately, and is only used if the settings
    // acceptOnAbandon field is true.
    int m_acceptedIndexOnAbandon;

    // This is the number of rows visible in the popup. The maximum number visible at a time is
    // defined as being kMaxVisibleRows. For a scrolled popup, this can be thought of as the
    // page size in data units.
    int m_visibleRows;

    // Our suggested width, not including scrollbar.
    int m_baseWidth;

    // The maximum height we can be without being off-screen.
    int m_maxHeight;

    // A list of the options contained within the <select>
    Vector<PopupItem*> m_items;

    // The <select> PopupMenuClient that opened us.
    PopupMenuClient* m_popupClient;

    // The scrollbar which has mouse capture.  Mouse events go straight to this
    // if non-NULL.
    RefPtr<Scrollbar> m_capturingScrollbar;

    // The last scrollbar that the mouse was over.  Used for mouseover highlights.
    RefPtr<Scrollbar> m_lastScrollbarUnderMouse;

    // The string the user has typed so far into the popup. Used for typeAheadFind.
    String m_typedString;

    // The char the user has hit repeatedly.  Used for typeAheadFind.
    UChar m_repeatingChar;

    // The last time the user hit a key.  Used for typeAheadFind.
    TimeStamp m_lastCharTime;

    // If width exeeds screen width, we have to clip it.
    int m_maxWindowWidth;

    // To forward last mouse release event.
    RefPtr<Node> m_focusedNode;
};

static PlatformMouseEvent constructRelativeMouseEvent(const PlatformMouseEvent& e,
                                                      FramelessScrollView* parent,
                                                      FramelessScrollView* child)
{
    IntPoint pos = parent->convertSelfToChild(child, e.pos());

    // FIXME: This is a horrible hack since PlatformMouseEvent has no setters for x/y.
    PlatformMouseEvent relativeEvent = e;
    IntPoint& relativePos = const_cast<IntPoint&>(relativeEvent.pos());
    relativePos.setX(pos.x());
    relativePos.setY(pos.y());
    return relativeEvent;
}

static PlatformWheelEvent constructRelativeWheelEvent(const PlatformWheelEvent& e,
                                                      FramelessScrollView* parent,
                                                      FramelessScrollView* child)
{
    IntPoint pos = parent->convertSelfToChild(child, e.pos());

    // FIXME: This is a horrible hack since PlatformWheelEvent has no setters for x/y.
    PlatformWheelEvent relativeEvent = e;
    IntPoint& relativePos = const_cast<IntPoint&>(relativeEvent.pos());
    relativePos.setX(pos.x());
    relativePos.setY(pos.y());
    return relativeEvent;
}

///////////////////////////////////////////////////////////////////////////////
// PopupContainer implementation

// static
PassRefPtr<PopupContainer> PopupContainer::create(PopupMenuClient* client,
                                                  PopupType popupType,
                                                  const PopupContainerSettings& settings)
{
    return adoptRef(new PopupContainer(client, popupType, settings));
}

PopupContainer::PopupContainer(PopupMenuClient* client,
                               PopupType popupType,
                               const PopupContainerSettings& settings)
    : m_listBox(PopupListBox::create(client, settings))
    , m_settings(settings)
    , m_popupType(popupType)
    , m_popupOpen(false)
{
    setScrollbarModes(ScrollbarAlwaysOff, ScrollbarAlwaysOff);
}

PopupContainer::~PopupContainer()
{
    if (m_listBox && m_listBox->parent())
        removeChild(m_listBox.get());
}

IntRect PopupContainer::layoutAndCalculateWidgetRect(int targetControlHeight, const IntPoint& popupInitialCoordinate)
{
    // Reset the max height to its default value, it will be recomputed below
    // if necessary.
    m_listBox->setMaxHeight(kMaxHeight);

    // Lay everything out to figure out our preferred size, then tell the view's
    // WidgetClient about it.  It should assign us a client.
    int rightOffset = layoutAndGetRightOffset();

    // Assume m_listBox size is already calculated.
    IntSize targetSize(m_listBox->width() + kBorderSize * 2,
                       m_listBox->height() + kBorderSize * 2);

    IntRect widgetRect;
    ChromeClientChromium* chromeClient = chromeClientChromium();
    if (chromeClient) {
        // If the popup would extend past the bottom of the screen, open upwards
        // instead.
        FloatRect screen = screenAvailableRect(m_frameView.get());
        // Use popupInitialCoordinate.x() + rightOffset because RTL position
        // needs to be considered.
        widgetRect = chromeClient->windowToScreen(IntRect(popupInitialCoordinate.x() + rightOffset, popupInitialCoordinate.y(), targetSize.width(), targetSize.height()));

        // If we have multiple screens and the browser rect is in one screen, we have
        // to clip the window width to the screen width.
        // When clipping, we also need to set a maximum width for the list box.
        FloatRect windowRect = chromeClient->windowRect();
        if (windowRect.x() >= screen.x() && windowRect.maxX() <= screen.maxX()) {
            if (m_listBox->m_popupClient->menuStyle().textDirection() == RTL && widgetRect.x() < screen.x()) {
                widgetRect.setWidth(widgetRect.maxX() - screen.x());
                widgetRect.setX(screen.x());
                listBox()->setMaxWidthAndLayout(max(widgetRect.width() - kBorderSize * 2, 0));
            } else if (widgetRect.maxX() > screen.maxX()) {
                widgetRect.setWidth(screen.maxX() - widgetRect.x());
                listBox()->setMaxWidthAndLayout(max(widgetRect.width() - kBorderSize * 2, 0));
            }
        }

        // Calculate Y axis size.
        if (widgetRect.maxY() > static_cast<int>(screen.maxY())) {
            if (widgetRect.y() - widgetRect.height() - targetControlHeight > 0) {
                // There is enough room to open upwards.
                widgetRect.move(0, -(widgetRect.height() + targetControlHeight));
            } else {
                // Figure whether upwards or downwards has more room and set the
                // maximum number of items.
                int spaceAbove = widgetRect.y() - targetControlHeight;
                int spaceBelow = screen.maxY() - widgetRect.y();
                if (spaceAbove > spaceBelow)
                    m_listBox->setMaxHeight(spaceAbove);
                else
                    m_listBox->setMaxHeight(spaceBelow);
                layoutAndGetRightOffset();
                // Our height has changed, so recompute only Y axis of widgetRect.
                // We don't have to recompute X axis, so we only replace Y axis
                // in widgetRect.
                IntRect frameInScreen = chromeClient->windowToScreen(frameRect());
                widgetRect.setY(frameInScreen.y());
                widgetRect.setHeight(frameInScreen.height());
                // And move upwards if necessary.
                if (spaceAbove > spaceBelow)
                    widgetRect.move(0, -(widgetRect.height() + targetControlHeight));
            }
        }
    }
    return widgetRect;
}

void PopupContainer::showPopup(FrameView* view)
{
    m_frameView = view;
    listBox()->m_focusedNode = m_frameView->frame()->document()->focusedNode();

    ChromeClientChromium* chromeClient = chromeClientChromium();
    if (chromeClient) {
        IntRect popupRect = frameRect();
        chromeClient->popupOpened(this, layoutAndCalculateWidgetRect(popupRect.height(), popupRect.location()), false);
        m_popupOpen = true;
    }

    if (!m_listBox->parent())
        addChild(m_listBox.get());

    // Enable scrollbars after the listbox is inserted into the hierarchy,
    // so it has a proper WidgetClient.
    m_listBox->setVerticalScrollbarMode(ScrollbarAuto);

    m_listBox->scrollToRevealSelection();

    invalidate();
}

void PopupContainer::hidePopup()
{
    listBox()->hidePopup();
}

void PopupContainer::notifyPopupHidden()
{
    if (!m_popupOpen)
        return;
    m_popupOpen = false;
    chromeClientChromium()->popupClosed(this);
}

int PopupContainer::layoutAndGetRightOffset()
{
    m_listBox->layout();

    // Place the listbox within our border.
    m_listBox->move(kBorderSize, kBorderSize);

    // popupWidth is the width of <select> element. Record it before resize frame.
    int popupWidth = frameRect().width();
    // Size ourselves to contain listbox + border.
    int listBoxWidth = m_listBox->width() + kBorderSize * 2;
    resize(listBoxWidth, m_listBox->height() + kBorderSize * 2);

    // Adjust the starting x-axis for RTL dropdown. For RTL dropdown, the right edge
    // of dropdown box should be aligned with the right edge of <select> element box,
    // and the dropdown box should be expanded to left if more space needed.
    PopupMenuClient* popupClient = m_listBox->m_popupClient;
    int rightOffset = 0;
    if (popupClient) {
        bool rightAligned = m_listBox->m_popupClient->menuStyle().textDirection() == RTL;
        if (rightAligned)
            rightOffset = popupWidth - listBoxWidth;
    }
    invalidate();

    return rightOffset;
}

bool PopupContainer::handleMouseDownEvent(const PlatformMouseEvent& event)
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    return m_listBox->handleMouseDownEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleMouseMoveEvent(const PlatformMouseEvent& event)
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    return m_listBox->handleMouseMoveEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleMouseReleaseEvent(const PlatformMouseEvent& event)
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    return m_listBox->handleMouseReleaseEvent(
        constructRelativeMouseEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleWheelEvent(const PlatformWheelEvent& event)
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    return m_listBox->handleWheelEvent(
        constructRelativeWheelEvent(event, this, m_listBox.get()));
}

bool PopupContainer::handleKeyEvent(const PlatformKeyboardEvent& event)
{
    UserGestureIndicator gestureIndicator(DefinitelyProcessingUserGesture);
    return m_listBox->handleKeyEvent(event);
}

void PopupContainer::hide()
{
    m_listBox->abandon();
}

void PopupContainer::paint(GraphicsContext* gc, const IntRect& rect)
{
    // adjust coords for scrolled frame
    IntRect r = intersection(rect, frameRect());
    int tx = x();
    int ty = y();

    r.move(-tx, -ty);

    gc->translate(static_cast<float>(tx), static_cast<float>(ty));
    m_listBox->paint(gc, r);
    gc->translate(-static_cast<float>(tx), -static_cast<float>(ty));

    paintBorder(gc, rect);
}

void PopupContainer::paintBorder(GraphicsContext* gc, const IntRect& rect)
{
    // FIXME: Where do we get the border color from?
    Color borderColor(127, 157, 185);

    gc->setStrokeStyle(NoStroke);
    gc->setFillColor(borderColor, ColorSpaceDeviceRGB);

    int tx = x();
    int ty = y();

    // top, left, bottom, right
    gc->drawRect(IntRect(tx, ty, width(), kBorderSize));
    gc->drawRect(IntRect(tx, ty, kBorderSize, height()));
    gc->drawRect(IntRect(tx, ty + height() - kBorderSize, width(), kBorderSize));
    gc->drawRect(IntRect(tx + width() - kBorderSize, ty, kBorderSize, height()));
}

bool PopupContainer::isInterestedInEventForKey(int keyCode)
{
    return m_listBox->isInterestedInEventForKey(keyCode);
}

ChromeClientChromium* PopupContainer::chromeClientChromium()
{
    return static_cast<ChromeClientChromium*>(m_frameView->frame()->page()->chrome()->client());
}

void PopupContainer::showInRect(const IntRect& r, FrameView* v, int index)
{
    // The rect is the size of the select box. It's usually larger than we need.
    // subtract border size so that usually the container will be displayed
    // exactly the same width as the select box.
    listBox()->setBaseWidth(max(r.width() - kBorderSize * 2, 0));

    listBox()->updateFromElement();

    // We set the selected item in updateFromElement(), and disregard the
    // index passed into this function (same as Webkit's PopupMenuWin.cpp)
    // FIXME: make sure this is correct, and add an assertion.
    // ASSERT(popupWindow(popup)->listBox()->selectedIndex() == index);

    // Convert point to main window coords.
    IntPoint location = v->contentsToWindow(r.location());

    // Move it below the select widget.
    location.move(0, r.height());

    setFrameRect(IntRect(location, r.size()));
    showPopup(v);
}

void PopupContainer::refresh(const IntRect& targetControlRect)
{
    IntPoint location = m_frameView->contentsToWindow(targetControlRect.location());
    // Move it below the select widget.
    location.move(0, targetControlRect.height());

    listBox()->updateFromElement();
    // Store the original size to check if we need to request the location.
    IntSize originalSize = size();
    IntRect widgetRect = layoutAndCalculateWidgetRect(targetControlRect.height(), location);
    if (originalSize != widgetRect.size())
        setFrameRect(widgetRect);

    invalidate();
}

int PopupContainer::selectedIndex() const
{
    return m_listBox->selectedIndex();
}

int PopupContainer::menuItemHeight() const
{
    return m_listBox->getRowHeight(0);
}

int PopupContainer::menuItemFontSize() const
{
    return m_listBox->getRowFont(0).size();
}

PopupMenuStyle PopupContainer::menuStyle() const
{
    return m_listBox->m_popupClient->menuStyle();
}

const WTF::Vector<PopupItem*>& PopupContainer:: popupData() const
{
    return m_listBox->items();
}

String PopupContainer::getSelectedItemToolTip()
{
    // We cannot use m_popupClient->selectedIndex() to choose tooltip message,
    // because the selectedIndex() might return final selected index, not hovering selection.
    return listBox()->m_popupClient->itemToolTip(listBox()->m_selectedIndex);
}

///////////////////////////////////////////////////////////////////////////////
// PopupListBox implementation

bool PopupListBox::handleMouseDownEvent(const PlatformMouseEvent& event)
{
    Scrollbar* scrollbar = scrollbarAtPoint(event.pos());
    if (scrollbar) {
        m_capturingScrollbar = scrollbar;
        m_capturingScrollbar->mouseDown(event);
        return true;
    }

    if (!isPointInBounds(event.pos()))
        abandon();

    return true;
}

bool PopupListBox::handleMouseMoveEvent(const PlatformMouseEvent& event)
{
    if (m_capturingScrollbar) {
        m_capturingScrollbar->mouseMoved(event);
        return true;
    }

    Scrollbar* scrollbar = scrollbarAtPoint(event.pos());
    if (m_lastScrollbarUnderMouse != scrollbar) {
        // Send mouse exited to the old scrollbar.
        if (m_lastScrollbarUnderMouse)
            m_lastScrollbarUnderMouse->mouseExited();
        m_lastScrollbarUnderMouse = scrollbar;
    }

    if (scrollbar) {
        scrollbar->mouseMoved(event);
        return true;
    }

    if (!isPointInBounds(event.pos()))
        return false;

    selectIndex(pointToRowIndex(event.pos()));
    return true;
}

bool PopupListBox::handleMouseReleaseEvent(const PlatformMouseEvent& event)
{
    if (m_capturingScrollbar) {
        m_capturingScrollbar->mouseUp();
        m_capturingScrollbar = 0;
        return true;
    }

    if (!isPointInBounds(event.pos()))
        return true;

    // Need to check before calling acceptIndex(), because m_popupClient might be removed in acceptIndex() calling because of event handler.
    bool isSelectPopup = m_popupClient->menuStyle().menuType() == PopupMenuStyle::SelectPopup;
    if (acceptIndex(pointToRowIndex(event.pos())) && m_focusedNode && isSelectPopup) {
        m_focusedNode->dispatchMouseEvent(event, eventNames().mouseupEvent);
        m_focusedNode->dispatchMouseEvent(event, eventNames().clickEvent);

        // Clear m_focusedNode here, because we cannot clear in hidePopup() which is called before dispatchMouseEvent() is called.
        m_focusedNode = 0;
    }

    return true;
}

bool PopupListBox::handleWheelEvent(const PlatformWheelEvent& event)
{
    if (!isPointInBounds(event.pos())) {
        abandon();
        return true;
    }

    // Pass it off to the scroll view.
    // Sadly, WebCore devs don't understand the whole "const" thing.
    wheelEvent(const_cast<PlatformWheelEvent&>(event));
    return true;
}

// Should be kept in sync with handleKeyEvent().
bool PopupListBox::isInterestedInEventForKey(int keyCode)
{
    switch (keyCode) {
    case VKEY_ESCAPE:
    case VKEY_RETURN:
    case VKEY_UP:
    case VKEY_DOWN:
    case VKEY_PRIOR:
    case VKEY_NEXT:
    case VKEY_HOME:
    case VKEY_END:
    case VKEY_TAB:
        return true;
    default:
        return false;
    }
}

static bool isCharacterTypeEvent(const PlatformKeyboardEvent& event)
{
    // Check whether the event is a character-typed event or not.
    // We use RawKeyDown/Char/KeyUp event scheme on all platforms,
    // so PlatformKeyboardEvent::Char (not RawKeyDown) type event
    // is considered as character type event.
    return event.type() == PlatformKeyboardEvent::Char;
}

bool PopupListBox::handleKeyEvent(const PlatformKeyboardEvent& event)
{
    if (event.type() == PlatformKeyboardEvent::KeyUp)
        return true;

    if (numItems() == 0 && event.windowsVirtualKeyCode() != VKEY_ESCAPE)
        return true;

    switch (event.windowsVirtualKeyCode()) {
    case VKEY_ESCAPE:
        abandon();  // may delete this
        return true;
    case VKEY_RETURN:
        if (m_selectedIndex == -1)  {
            hidePopup();
            // Don't eat the enter if nothing is selected.
            return false;
        }
        acceptIndex(m_selectedIndex);  // may delete this
        return true;
    case VKEY_UP:
    case VKEY_DOWN:
        // We have to forward only shift + up combination to focused node when autofill popup.
        // Because all characters from the cursor to the start of the text area should selected when you press shift + up arrow.
        // shift + down should be the similar way to shift + up.
        if (event.modifiers() && m_popupClient->menuStyle().menuType() == PopupMenuStyle::AutofillPopup)
            m_focusedNode->dispatchKeyEvent(event);
        else if (event.windowsVirtualKeyCode() == VKEY_UP)
            selectPreviousRow();
        else
            selectNextRow();
        break;
    case VKEY_PRIOR:
        adjustSelectedIndex(-m_visibleRows);
        break;
    case VKEY_NEXT:
        adjustSelectedIndex(m_visibleRows);
        break;
    case VKEY_HOME:
        adjustSelectedIndex(-m_selectedIndex);
        break;
    case VKEY_END:
        adjustSelectedIndex(m_items.size());
        break;
    default:
        if (!event.ctrlKey() && !event.altKey() && !event.metaKey()
            && isPrintableChar(event.windowsVirtualKeyCode())
            && isCharacterTypeEvent(event))
            typeAheadFind(event);
        break;
    }

    if (m_originalIndex != m_selectedIndex) {
        // Keyboard events should update the selection immediately (but we don't
        // want to fire the onchange event until the popup is closed, to match
        // IE).  We change the original index so we revert to that when the
        // popup is closed.
        if (m_settings.acceptOnAbandon)
            m_acceptedIndexOnAbandon = m_selectedIndex;

        setOriginalIndex(m_selectedIndex);
        if (m_settings.setTextOnIndexChange)
            m_popupClient->setTextFromItem(m_selectedIndex);
    }
    if (event.windowsVirtualKeyCode() == VKEY_TAB) {
        // TAB is a special case as it should select the current item if any and
        // advance focus.
        if (m_selectedIndex >= 0) {
            acceptIndex(m_selectedIndex); // May delete us.
            // Return false so the TAB key event is propagated to the page.
            return false;
        }
        // Call abandon() so we honor m_acceptedIndexOnAbandon if set.
        abandon();
        // Return false so the TAB key event is propagated to the page.
        return false;
    }

    return true;
}

HostWindow* PopupListBox::hostWindow() const
{
    // Our parent is the root ScrollView, so it is the one that has a
    // HostWindow.  FrameView::hostWindow() works similarly.
    return parent() ? parent()->hostWindow() : 0;
}

// From HTMLSelectElement.cpp
static String stripLeadingWhiteSpace(const String& string)
{
    int length = string.length();
    int i;
    for (i = 0; i < length; ++i)
        if (string[i] != noBreakSpace
            && (string[i] <= 0x7F ? !isspace(string[i]) : (direction(string[i]) != WhiteSpaceNeutral)))
            break;

    return string.substring(i, length - i);
}

// From HTMLSelectElement.cpp, with modifications
void PopupListBox::typeAheadFind(const PlatformKeyboardEvent& event)
{
    TimeStamp now = static_cast<TimeStamp>(currentTime() * 1000.0f);
    TimeStamp delta = now - m_lastCharTime;

    // Reset the time when user types in a character. The time gap between
    // last character and the current character is used to indicate whether
    // user typed in a string or just a character as the search prefix.
    m_lastCharTime = now;

    UChar c = event.windowsVirtualKeyCode();

    String prefix;
    int searchStartOffset = 1;
    if (delta > kTypeAheadTimeoutMs) {
        m_typedString = prefix = String(&c, 1);
        m_repeatingChar = c;
    } else {
        m_typedString.append(c);

        if (c == m_repeatingChar)
            // The user is likely trying to cycle through all the items starting with this character, so just search on the character
            prefix = String(&c, 1);
        else {
            m_repeatingChar = 0;
            prefix = m_typedString;
            searchStartOffset = 0;
        }
    }

    // Compute a case-folded copy of the prefix string before beginning the search for
    // a matching element. This code uses foldCase to work around the fact that
    // String::startWith does not fold non-ASCII characters. This code can be changed
    // to use startWith once that is fixed.
    String prefixWithCaseFolded(prefix.foldCase());
    int itemCount = numItems();
    int index = (max(0, m_selectedIndex) + searchStartOffset) % itemCount;
    for (int i = 0; i < itemCount; i++, index = (index + 1) % itemCount) {
        if (!isSelectableItem(index))
            continue;

        if (stripLeadingWhiteSpace(m_items[index]->label).foldCase().startsWith(prefixWithCaseFolded)) {
            selectIndex(index);
            return;
        }
    }
}

void PopupListBox::paint(GraphicsContext* gc, const IntRect& rect)
{
    // adjust coords for scrolled frame
    IntRect r = intersection(rect, frameRect());
    int tx = x() - scrollX();
    int ty = y() - scrollY();

    r.move(-tx, -ty);

    // set clip rect to match revised damage rect
    gc->save();
    gc->translate(static_cast<float>(tx), static_cast<float>(ty));
    gc->clip(r);

    // FIXME: Can we optimize scrolling to not require repainting the entire
    // window?  Should we?
    for (int i = 0; i < numItems(); ++i)
        paintRow(gc, r, i);

    // Special case for an empty popup.
    if (numItems() == 0)
        gc->fillRect(r, Color::white, ColorSpaceDeviceRGB);

    gc->restore();

    ScrollView::paint(gc, rect);
}

static const int separatorPadding = 4;
static const int separatorHeight = 1;

void PopupListBox::paintRow(GraphicsContext* gc, const IntRect& rect, int rowIndex)
{
    // This code is based largely on RenderListBox::paint* methods.

    IntRect rowRect = getRowBounds(rowIndex);
    if (!rowRect.intersects(rect))
        return;

    PopupMenuStyle style = m_popupClient->itemStyle(rowIndex);

    // Paint background
    Color backColor, textColor, labelColor;
    if (rowIndex == m_selectedIndex) {
        backColor = RenderTheme::defaultTheme()->activeListBoxSelectionBackgroundColor();
        textColor = RenderTheme::defaultTheme()->activeListBoxSelectionForegroundColor();
        labelColor = textColor;
    } else {
        backColor = style.backgroundColor();
        textColor = style.foregroundColor();
        // FIXME: for now the label color is hard-coded. It should be added to
        // the PopupMenuStyle.
        labelColor = Color(115, 115, 115);
    }

    // If we have a transparent background, make sure it has a color to blend
    // against.
    if (backColor.hasAlpha())
        gc->fillRect(rowRect, Color::white, ColorSpaceDeviceRGB);

    gc->fillRect(rowRect, backColor, ColorSpaceDeviceRGB);

    // It doesn't look good but Autofill requires special style for separator.
    // Autofill doesn't have padding and #dcdcdc color.
    if (m_popupClient->itemIsSeparator(rowIndex)) {
        int padding = style.menuType() == PopupMenuStyle::AutofillPopup ? 0 : separatorPadding;
        IntRect separatorRect(
            rowRect.x() + padding,
            rowRect.y() + (rowRect.height() - separatorHeight) / 2,
            rowRect.width() - 2 * padding, separatorHeight);
        gc->fillRect(separatorRect, style.menuType() == PopupMenuStyle::AutofillPopup ? Color(0xdc, 0xdc, 0xdc) : textColor, ColorSpaceDeviceRGB);
        return;
    }

    if (!style.isVisible())
        return;

    gc->setFillColor(textColor, ColorSpaceDeviceRGB);

    Font itemFont = getRowFont(rowIndex);
    // FIXME: http://crbug.com/19872 We should get the padding of individual option
    // elements.  This probably implies changes to PopupMenuClient.
    bool rightAligned = m_popupClient->menuStyle().textDirection() == RTL;
    int textX = 0;
    int maxWidth = 0;
    if (rightAligned)
        maxWidth = rowRect.width() - max(0, m_popupClient->clientPaddingRight() - m_popupClient->clientInsetRight());
    else {
        textX = max(0, m_popupClient->clientPaddingLeft() - m_popupClient->clientInsetLeft());
        maxWidth = rowRect.width() - textX;
    }
    // Prepare text to be drawn.
    String itemText = m_popupClient->itemText(rowIndex);
    String itemLabel = m_popupClient->itemLabel(rowIndex);
    String itemIcon = m_popupClient->itemIcon(rowIndex);
    if (m_settings.restrictWidthOfListBox) { // Truncate strings to fit in.
        // FIXME: We should leftTruncate for the rtl case.
        // StringTruncator::leftTruncate would have to be implemented.
        String str = StringTruncator::rightTruncate(itemText, maxWidth, itemFont);
        if (str != itemText) {
            itemText = str;
            // Don't display the label or icon, we already don't have enough room for the item text.
            itemLabel = "";
            itemIcon = "";
        } else if (!itemLabel.isEmpty()) {
            int availableWidth = maxWidth - kTextToLabelPadding -
                StringTruncator::width(itemText, itemFont);
            itemLabel = StringTruncator::rightTruncate(itemLabel, availableWidth, itemFont);
        }
    }

    // Prepare the directionality to draw text.
    TextRun textRun(itemText.characters(), itemText.length(), false, 0, 0, TextRun::AllowTrailingExpansion, style.textDirection(), style.hasTextDirectionOverride());
    // If the text is right-to-left, make it right-aligned by adjusting its
    // beginning position.
    if (rightAligned)
        textX += maxWidth - itemFont.width(textRun);

    // Draw the item text.
    int textY = rowRect.y() + itemFont.fontMetrics().ascent() + (rowRect.height() - itemFont.fontMetrics().height()) / 2;
    gc->drawBidiText(itemFont, textRun, IntPoint(textX, textY));

    // We are using the left padding as the right padding includes room for the scroll-bar which
    // does not show in this case.
    int rightPadding = max(0, m_popupClient->clientPaddingLeft() - m_popupClient->clientInsetLeft());
    int remainingWidth = rowRect.width() - rightPadding;

    // Draw the icon if applicable.
    RefPtr<Image> image(Image::loadPlatformResource(itemIcon.utf8().data()));
    if (image && !image->isNull()) {
        IntRect imageRect = image->rect();
        remainingWidth -= (imageRect.width() + kLabelToIconPadding);
        imageRect.setX(rowRect.width() - rightPadding - imageRect.width());
        imageRect.setY(rowRect.y() + (rowRect.height() - imageRect.height()) / 2);
        gc->drawImage(image.get(), ColorSpaceDeviceRGB, imageRect);
    }

    // Draw the the label if applicable.
    if (itemLabel.isEmpty())
        return;

    // Autofill label is 0.9 smaller than regular font size.
    if (style.menuType() == PopupMenuStyle::AutofillPopup) {
        itemFont = m_popupClient->itemStyle(rowIndex).font();
        FontDescription d = itemFont.fontDescription();
        d.setComputedSize(d.computedSize() * 0.9);
        itemFont = Font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
        itemFont.update(0);
    }

    TextRun labelTextRun(itemLabel.characters(), itemLabel.length(), false, 0, 0, TextRun::AllowTrailingExpansion, style.textDirection(), style.hasTextDirectionOverride());
    if (rightAligned)
        textX = max(0, m_popupClient->clientPaddingLeft() - m_popupClient->clientInsetLeft());
    else
        textX = remainingWidth - itemFont.width(labelTextRun);

    gc->setFillColor(labelColor, ColorSpaceDeviceRGB);
    gc->drawBidiText(itemFont, labelTextRun, IntPoint(textX, textY));
}

Font PopupListBox::getRowFont(int rowIndex)
{
    Font itemFont = m_popupClient->itemStyle(rowIndex).font();
    if (m_popupClient->itemIsLabel(rowIndex)) {
        // Bold-ify labels (ie, an <optgroup> heading).
        FontDescription d = itemFont.fontDescription();
        d.setWeight(FontWeightBold);
        Font font(d, itemFont.letterSpacing(), itemFont.wordSpacing());
        font.update(0);
        return font;
    }

    return itemFont;
}

void PopupListBox::abandon()
{
    RefPtr<PopupListBox> keepAlive(this);

    m_selectedIndex = m_originalIndex;

    hidePopup();

    if (m_acceptedIndexOnAbandon >= 0) {
        if (m_popupClient)
            m_popupClient->valueChanged(m_acceptedIndexOnAbandon);
        m_acceptedIndexOnAbandon = -1;
    }
}

int PopupListBox::pointToRowIndex(const IntPoint& point)
{
    int y = scrollY() + point.y();

    // FIXME: binary search if perf matters.
    for (int i = 0; i < numItems(); ++i) {
        if (y < m_items[i]->yOffset)
            return i-1;
    }

    // Last item?
    if (y < contentsHeight())
        return m_items.size()-1;

    return -1;
}

bool PopupListBox::acceptIndex(int index)
{
    // Clear m_acceptedIndexOnAbandon once user accepts the selected index.
    if (m_acceptedIndexOnAbandon >= 0)
        m_acceptedIndexOnAbandon = -1;

    if (index >= numItems())
        return false;

    if (index < 0) {
        if (m_popupClient) {
            // Enter pressed with no selection, just close the popup.
            hidePopup();
        }
        return false;
    }

    if (isSelectableItem(index)) {
        RefPtr<PopupListBox> keepAlive(this);

        // Hide ourselves first since valueChanged may have numerous side-effects.
        hidePopup();

        // Tell the <select> PopupMenuClient what index was selected.
        m_popupClient->valueChanged(index);

        return true;
    }

    return false;
}

void PopupListBox::selectIndex(int index)
{
    if (index < 0 || index >= numItems())
        return;

    bool isSelectable = isSelectableItem(index);
    if (index != m_selectedIndex && isSelectable) {
        invalidateRow(m_selectedIndex);
        m_selectedIndex = index;
        invalidateRow(m_selectedIndex);

        scrollToRevealSelection();
        m_popupClient->selectionChanged(m_selectedIndex);
    } else if (!isSelectable) {
        clearSelection();
    }
}

void PopupListBox::setOriginalIndex(int index)
{
    m_originalIndex = m_selectedIndex = index;
}

int PopupListBox::getRowHeight(int index)
{
    if (index < 0)
        return 0;

    if (m_popupClient->itemStyle(index).isDisplayNone())
        return 0;

    // Separator row height is the same size as itself.
    if (m_popupClient->itemIsSeparator(index))
        return separatorHeight;

    String icon = m_popupClient->itemIcon(index);
    RefPtr<Image> image(Image::loadPlatformResource(icon.utf8().data()));

    int fontHeight = getRowFont(index).fontMetrics().height();
    int iconHeight = (image && !image->isNull()) ? image->rect().height() : 0;

    int linePaddingHeight = m_popupClient->menuStyle().menuType() == PopupMenuStyle::AutofillPopup ? kLinePaddingHeight : 0;
    return max(fontHeight, iconHeight) + linePaddingHeight * 2;
}

IntRect PopupListBox::getRowBounds(int index)
{
    if (index < 0)
        return IntRect(0, 0, visibleWidth(), getRowHeight(index));

    return IntRect(0, m_items[index]->yOffset, visibleWidth(), getRowHeight(index));
}

void PopupListBox::invalidateRow(int index)
{
    if (index < 0)
        return;

    // Invalidate in the window contents, as FramelessScrollView::invalidateRect
    // paints in the window coordinates.
    invalidateRect(contentsToWindow(getRowBounds(index)));
}

void PopupListBox::scrollToRevealRow(int index)
{
    if (index < 0)
        return;

    IntRect rowRect = getRowBounds(index);

    if (rowRect.y() < scrollY()) {
        // Row is above current scroll position, scroll up.
        ScrollView::setScrollPosition(IntPoint(0, rowRect.y()));
    } else if (rowRect.maxY() > scrollY() + visibleHeight()) {
        // Row is below current scroll position, scroll down.
        ScrollView::setScrollPosition(IntPoint(0, rowRect.maxY() - visibleHeight()));
    }
}

bool PopupListBox::isSelectableItem(int index)
{
    ASSERT(index >= 0 && index < numItems());
    return m_items[index]->type == PopupItem::TypeOption && m_popupClient->itemIsEnabled(index);
}

void PopupListBox::clearSelection()
{
    if (m_selectedIndex != -1) {
        invalidateRow(m_selectedIndex);
        m_selectedIndex = -1;
        m_popupClient->selectionCleared();
    }
}

void PopupListBox::selectNextRow()
{
    if (!m_settings.loopSelectionNavigation || m_selectedIndex != numItems() - 1) {
        adjustSelectedIndex(1);
        return;
    }

    // We are moving past the last item, no row should be selected.
    clearSelection();
}

void PopupListBox::selectPreviousRow()
{
    if (!m_settings.loopSelectionNavigation || m_selectedIndex > 0) {
        adjustSelectedIndex(-1);
        return;
    }

    if (m_selectedIndex == 0) {
        // We are moving past the first item, clear the selection.
        clearSelection();
        return;
    }

    // No row is selected, jump to the last item.
    selectIndex(numItems() - 1);
    scrollToRevealSelection();
}

void PopupListBox::adjustSelectedIndex(int delta)
{
    int targetIndex = m_selectedIndex + delta;
    targetIndex = min(max(targetIndex, 0), numItems() - 1);
    if (!isSelectableItem(targetIndex)) {
        // We didn't land on an option.  Try to find one.
        // We try to select the closest index to target, prioritizing any in
        // the range [current, target].

        int dir = delta > 0 ? 1 : -1;
        int testIndex = m_selectedIndex;
        int bestIndex = m_selectedIndex;
        bool passedTarget = false;
        while (testIndex >= 0 && testIndex < numItems()) {
            if (isSelectableItem(testIndex))
                bestIndex = testIndex;
            if (testIndex == targetIndex)
                passedTarget = true;
            if (passedTarget && bestIndex != m_selectedIndex)
                break;

            testIndex += dir;
        }

        // Pick the best index, which may mean we don't change.
        targetIndex = bestIndex;
    }

    // Select the new index, and ensure its visible.  We do this regardless of
    // whether the selection changed to ensure keyboard events always bring the
    // selection into view.
    selectIndex(targetIndex);
    scrollToRevealSelection();
}

void PopupListBox::hidePopup()
{
    if (parent()) {
        PopupContainer* container = static_cast<PopupContainer*>(parent());
        if (container->client())
            container->client()->popupClosed(container);
        container->notifyPopupHidden();
    }

    if (m_popupClient)
        m_popupClient->popupDidHide();
}

void PopupListBox::updateFromElement()
{
    clear();

    int size = m_popupClient->listSize();
    for (int i = 0; i < size; ++i) {
        PopupItem::Type type;
        if (m_popupClient->itemIsSeparator(i))
            type = PopupItem::TypeSeparator;
        else if (m_popupClient->itemIsLabel(i))
            type = PopupItem::TypeGroup;
        else
            type = PopupItem::TypeOption;
        m_items.append(new PopupItem(m_popupClient->itemText(i), type));
        m_items[i]->enabled = isSelectableItem(i);
        PopupMenuStyle style = m_popupClient->itemStyle(i);
        m_items[i]->textDirection = style.textDirection();
        m_items[i]->hasTextDirectionOverride = style.hasTextDirectionOverride();
    }

    m_selectedIndex = m_popupClient->selectedIndex();
    setOriginalIndex(m_selectedIndex);

    layout();
}

void PopupListBox::setMaxWidthAndLayout(int maxWidth)
{
    m_maxWindowWidth = maxWidth;
    layout();
}

void PopupListBox::layout()
{
    bool isRightAligned = m_popupClient->menuStyle().textDirection() == RTL;
    
    // Size our child items.
    int baseWidth = 0;
    int paddingWidth = 0;
    int lineEndPaddingWidth = 0;
    int y = 0;
    for (int i = 0; i < numItems(); ++i) {
        // Place the item vertically.
        m_items[i]->yOffset = y;
        if (m_popupClient->itemStyle(i).isDisplayNone())
            continue;
        y += getRowHeight(i);

        // Ensure the popup is wide enough to fit this item.
        Font itemFont = getRowFont(i);
        String text = m_popupClient->itemText(i);
        String label = m_popupClient->itemLabel(i);
        String icon = m_popupClient->itemIcon(i);
        RefPtr<Image> iconImage(Image::loadPlatformResource(icon.utf8().data()));
        int width = 0;
        if (!text.isEmpty())
            width = itemFont.width(TextRun(text));
        if (!label.isEmpty()) {
            if (width > 0)
                width += kTextToLabelPadding;
            width += itemFont.width(TextRun(label));
        }
        if (iconImage && !iconImage->isNull()) {
            if (width > 0)
                width += kLabelToIconPadding;
            width += iconImage->rect().width();
        }

        baseWidth = max(baseWidth, width);
        // FIXME: http://b/1210481 We should get the padding of individual option elements.
        paddingWidth = max(paddingWidth,
            m_popupClient->clientPaddingLeft() + m_popupClient->clientPaddingRight());
        lineEndPaddingWidth = max(lineEndPaddingWidth,
            isRightAligned ? m_popupClient->clientPaddingLeft() : m_popupClient->clientPaddingRight());
    }

    // Calculate scroll bar width.
    int windowHeight = 0;
    m_visibleRows = min(numItems(), kMaxVisibleRows);

    for (int i = 0; i < m_visibleRows; ++i) {
        int rowHeight = getRowHeight(i);

        // Only clip the window height for non-Mac platforms.
        if (windowHeight + rowHeight > m_maxHeight) {
            m_visibleRows = i;
            break;
        }

        windowHeight += rowHeight;
    }

    // Set our widget and scrollable contents sizes.
    int scrollbarWidth = 0;
    if (m_visibleRows < numItems()) {
        scrollbarWidth = ScrollbarTheme::nativeTheme()->scrollbarThickness();

        // Use kMinEndOfLinePadding when there is a scrollbar so that we use
        // as much as (lineEndPaddingWidth - kMinEndOfLinePadding) padding
        // space for scrollbar and allow user to use CSS padding to make the
        // popup listbox align with the select element.
        paddingWidth = paddingWidth - lineEndPaddingWidth + kMinEndOfLinePadding;
    }

    int windowWidth;
    int contentWidth;
    if (m_settings.restrictWidthOfListBox) {
        windowWidth = m_baseWidth;
        contentWidth = m_baseWidth - scrollbarWidth;
    } else {
        windowWidth = baseWidth + scrollbarWidth + paddingWidth;
        if (windowWidth > m_maxWindowWidth) {
            // windowWidth exceeds m_maxWindowWidth, so we have to clip.
            windowWidth = m_maxWindowWidth;
            baseWidth = windowWidth - scrollbarWidth - paddingWidth;
            m_baseWidth = baseWidth;
        }
        contentWidth = windowWidth - scrollbarWidth;

        if (windowWidth < m_baseWidth) {
            windowWidth = m_baseWidth;
            contentWidth = m_baseWidth - scrollbarWidth;
        } else
            m_baseWidth = baseWidth;
    }

    resize(windowWidth, windowHeight);
    setContentsSize(IntSize(contentWidth, getRowBounds(numItems() - 1).maxY()));

    if (hostWindow())
        scrollToRevealSelection();

    invalidate();
}

void PopupListBox::clear()
{
    for (Vector<PopupItem*>::iterator it = m_items.begin(); it != m_items.end(); ++it)
        delete *it;
    m_items.clear();
}

bool PopupListBox::isPointInBounds(const IntPoint& point)
{
    return numItems() != 0 && IntRect(0, 0, width(), height()).contains(point);
}

///////////////////////////////////////////////////////////////////////////////
// PopupMenuChromium implementation
//
// Note: you cannot add methods to this class, since it is defined above the
//       portability layer. To access methods and properties on the
//       popup widgets, use |popupWindow| above.

PopupMenuChromium::PopupMenuChromium(PopupMenuClient* client)
    : m_popupClient(client)
{
}

PopupMenuChromium::~PopupMenuChromium()
{
    // When the PopupMenuChromium is destroyed, the client could already have been
    // deleted.
    if (p.popup)
        p.popup->listBox()->disconnectClient();
    hide();
}

void PopupMenuChromium::show(const IntRect& r, FrameView* v, int index)
{
    if (!p.popup)
        p.popup = PopupContainer::create(client(), PopupContainer::Select, dropDownSettings);
    p.popup->showInRect(r, v, index);
}

void PopupMenuChromium::hide()
{
    if (p.popup)
        p.popup->hide();
}

void PopupMenuChromium::updateFromElement()
{
    p.popup->listBox()->updateFromElement();
}


void PopupMenuChromium::disconnectClient()
{
    m_popupClient = 0;
}

} // namespace WebCore
