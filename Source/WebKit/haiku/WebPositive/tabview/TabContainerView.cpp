/*
 * Copyright (C) 2010 Rene Gollent <rene@gollent.com>
 * Copyright (C) 2010 Stephan Aßmus <superstippi@gmx.de>
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

#include "TabContainerView.h"

#include <stdio.h>

#include <Application.h>
#include <AbstractLayoutItem.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <MenuBar.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "TabView.h"


static const float kLeftTabInset = 4;


TabContainerView::TabContainerView(Controller* controller)
	:
	BGroupView(B_HORIZONTAL, 0.0),
	fLastMouseEventTab(NULL),
	fMouseDown(false),
	fClickCount(0),
	fSelectedTab(NULL),
	fController(controller),
	fFirstVisibleTabIndex(0)
{
	SetFlags(Flags() | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	SetViewColor(B_TRANSPARENT_COLOR);
	GroupLayout()->SetInsets(kLeftTabInset, 0, 0, 1);
	GroupLayout()->AddItem(BSpaceLayoutItem::CreateGlue(), 0.0f);
}


TabContainerView::~TabContainerView()
{
}


BSize
TabContainerView::MinSize()
{
	// Eventually, we want to be scrolling if the tabs don't fit.
	BSize size(BGroupView::MinSize());
	size.width = 300;
	return size;
}


void
TabContainerView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		default:
			BGroupView::MessageReceived(message);
	}
}


void
TabContainerView::Draw(BRect updateRect)
{
	// Stroke separator line at bottom.
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	BRect frame(Bounds());
	SetHighColor(tint_color(base, B_DARKEN_2_TINT));
	StrokeLine(frame.LeftBottom(), frame.RightBottom());
	frame.bottom--;

	// Draw empty area before first tab.
	uint32 borders = BControlLook::B_TOP_BORDER | BControlLook::B_BOTTOM_BORDER;
	BRect leftFrame(frame.left, frame.top, kLeftTabInset, frame.bottom);
	be_control_look->DrawInactiveTab(this, leftFrame, updateRect, base, 0,
		borders);

	// Draw all tabs, keeping track of where they end.
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (!item || !item->IsVisible())
			continue;
		item->Parent()->Draw(updateRect);
		frame.left = item->Frame().right + 1;
	}

	// Draw empty area after last tab.
	be_control_look->DrawInactiveTab(this, frame, updateRect, base, 0, borders);
}


void
TabContainerView::MouseDown(BPoint where)
{
	uint32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", (int32*)&buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;
	uint32 clicks;
	if (Window()->CurrentMessage()->FindInt32("clicks", (int32*)&clicks) != B_OK)
		clicks = 1;
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	if (fLastMouseEventTab)
		fLastMouseEventTab->MouseDown(where, buttons);
	else {
		if ((buttons & B_TERTIARY_MOUSE_BUTTON) != 0) {
			// Middle click outside tabs should always open a new tab.
			fClickCount = 2;
		} else if (clicks > 1)
			fClickCount = fClickCount++;
		else
			fClickCount = 1;
	}
}


void
TabContainerView::MouseUp(BPoint where)
{
	fMouseDown = false;
	if (fLastMouseEventTab) {
		fLastMouseEventTab->MouseUp(where);
		fClickCount = 0;
	} else if (fClickCount > 1) {
		// NOTE: fClickCount is >= 1 only if the first click was outside
		// any tab. So even if fLastMouseEventTab has been reset to NULL
		// because this tab was removed during mouse down, we wouldn't
		// run the "outside tabs" code below.
		fController->DoubleClickOutsideTabs();
		fClickCount = 0;
	}
	// Always check the tab under the mouse again, since we don't update
	// it with fMouseDown == true.
	_SendFakeMouseMoved();
}


void
TabContainerView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	_MouseMoved(where, transit, dragMessage);
}


void
TabContainerView::DoLayout()
{
	BGroupView::DoLayout();

	_ValidateTabVisibility();
	_SendFakeMouseMoved();
}

void
TabContainerView::AddTab(const char* label, int32 index)
{
	TabView* tab;
	if (fController)
		tab = fController->CreateTabView();
	else
		tab = new TabView();
	tab->SetLabel(label);
	AddTab(tab, index);
}


void
TabContainerView::AddTab(TabView* tab, int32 index)
{
	tab->SetContainerView(this);

	if (index == -1)
		index = GroupLayout()->CountItems() - 1;

	bool hasFrames = fController != NULL && fController->HasFrames();
	bool isFirst = index == 0 && hasFrames;
	bool isLast = index == GroupLayout()->CountItems() - 1 && hasFrames;
	bool isFront = fSelectedTab == NULL;
	tab->Update(isFirst, isLast, isFront);

	GroupLayout()->AddItem(index, tab->LayoutItem());

	if (isFront)
		SelectTab(tab);
	if (isLast) {
		TabLayoutItem* item
			= dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index - 1));
		if (item)
			item->Parent()->SetIsLast(false);
	}

	SetFirstVisibleTabIndex(MaxFirstVisibleTabIndex());
	_ValidateTabVisibility();
}

TabView*
TabContainerView::RemoveTab(int32 index)
{
	TabLayoutItem* item
		= dynamic_cast<TabLayoutItem*>(GroupLayout()->RemoveItem(index));

	if (!item)
		return NULL;

	BRect dirty(Bounds());
	dirty.left = item->Frame().left;
	TabView* removedTab = item->Parent();
	removedTab->SetContainerView(NULL);

	if (removedTab == fLastMouseEventTab)
		fLastMouseEventTab = NULL;

	// Update tabs after or before the removed tab.
	bool hasFrames = fController != NULL && fController->HasFrames();
	item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index));
	if (item) {
		// This tab is behind the removed tab.
		TabView* tab = item->Parent();
		tab->Update(index == 0 && hasFrames,
			index == GroupLayout()->CountItems() - 2 && hasFrames,
			tab == fSelectedTab);
		if (removedTab == fSelectedTab) {
			fSelectedTab = NULL;
			SelectTab(tab);
		} else if (fController && tab == fSelectedTab)
			fController->TabSelected(index);
	} else {
		// The removed tab was the last tab.
		item = dynamic_cast<TabLayoutItem*>(GroupLayout()->ItemAt(index - 1));
		if (item) {
			TabView* tab = item->Parent();
			tab->Update(index == 0 && hasFrames,
				index == GroupLayout()->CountItems() - 2 && hasFrames,
				tab == fSelectedTab);
			if (removedTab == fSelectedTab) {
				fSelectedTab = NULL;
				SelectTab(tab);
			}
		}
	}

	Invalidate(dirty);
	_ValidateTabVisibility();

	return removedTab;
}


TabView*
TabContainerView::TabAt(int32 index) const
{
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(index));
	if (item)
		return item->Parent();
	return NULL;
}


int32
TabContainerView::IndexOf(TabView* tab) const
{
	return GroupLayout()->IndexOfItem(tab->LayoutItem());
}


void
TabContainerView::SelectTab(int32 index)
{
	TabView* tab = NULL;
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(index));
	if (item)
		tab = item->Parent();

	SelectTab(tab);
}


void
TabContainerView::SelectTab(TabView* tab)
{
	if (tab == fSelectedTab)
		return;

	if (fSelectedTab)
		fSelectedTab->SetIsFront(false);

	fSelectedTab = tab;

	if (fSelectedTab)
		fSelectedTab->SetIsFront(true);

	if (fController != NULL) {
		int32 index = -1;
		if (fSelectedTab != NULL)
			index = GroupLayout()->IndexOfItem(tab->LayoutItem());

		if (!tab->LayoutItem()->IsVisible()) {
			SetFirstVisibleTabIndex(index);
		}

		fController->TabSelected(index);
	}
}


void
TabContainerView::SetTabLabel(int32 tabIndex, const char* label)
{
	TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
		GroupLayout()->ItemAt(tabIndex));
	if (item == NULL)
		return;

	item->Parent()->SetLabel(label);
}


void
TabContainerView::SetFirstVisibleTabIndex(int32 index)
{
	if (index < 0)
		index = 0;
	if (index > MaxFirstVisibleTabIndex())
		index = MaxFirstVisibleTabIndex();
	if (fFirstVisibleTabIndex == index)
		return;

	fFirstVisibleTabIndex = index;

	_UpdateTabVisibility();
}


int32
TabContainerView::FirstVisibleTabIndex() const
{
	return fFirstVisibleTabIndex;
}


int32
TabContainerView::MaxFirstVisibleTabIndex() const
{
	float availableWidth = _AvailableWidthForTabs();
	if (availableWidth < 0)
		return 0;
	float visibleTabsWidth = 0;

	BGroupLayout* layout = GroupLayout();
	int32 i = layout->CountItems() - 2;
	for (; i >= 0; i--) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (item == NULL)
			continue;

		float itemWidth = item->MinSize().width;
		if (availableWidth >= visibleTabsWidth + itemWidth)
			visibleTabsWidth += itemWidth;
		else {
			// The tab before this tab is the last one that can be visible.
			return i + 1;
		}
	}

	return 0;
}


bool
TabContainerView::CanScrollLeft() const
{
	return fFirstVisibleTabIndex < MaxFirstVisibleTabIndex();
}


bool
TabContainerView::CanScrollRight() const
{
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	if (count > 0) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(count - 1));
		return !item->IsVisible();
	}
	return false;
}


// #pragma mark -


TabView*
TabContainerView::_TabAt(const BPoint& where) const
{
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(layout->ItemAt(i));
		if (item == NULL || !item->IsVisible())
			continue;
		// Account for the fact that the tab frame does not contain the
		// visible bottom border.
		BRect frame = item->Frame();
		frame.bottom++;
		if (frame.Contains(where))
			return item->Parent();
	}
	return NULL;
}


void
TabContainerView::_MouseMoved(BPoint where, uint32 _transit,
	const BMessage* dragMessage)
{
	TabView* tab = _TabAt(where);
	if (fMouseDown) {
		uint32 transit = tab == fLastMouseEventTab
			? B_INSIDE_VIEW : B_OUTSIDE_VIEW;
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, transit, dragMessage);
		return;
	}

	if (fLastMouseEventTab != NULL && fLastMouseEventTab == tab)
		fLastMouseEventTab->MouseMoved(where, B_INSIDE_VIEW, dragMessage);
	else {
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_EXITED_VIEW, dragMessage);
		fLastMouseEventTab = tab;
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_ENTERED_VIEW, dragMessage);
		else
			fController->SetToolTip("Double-click or middle-click to open new tab.");
	}
}


void
TabContainerView::_ValidateTabVisibility()
{
	if (fFirstVisibleTabIndex > MaxFirstVisibleTabIndex())
		SetFirstVisibleTabIndex(MaxFirstVisibleTabIndex());
	else
		_UpdateTabVisibility();
}


void
TabContainerView::_UpdateTabVisibility()
{
	float availableWidth = _AvailableWidthForTabs();
	if (availableWidth < 0)
		return;
	float visibleTabsWidth = 0;

	bool canScrollTabsLeft = fFirstVisibleTabIndex > 0;
	bool canScrollTabsRight = false;

	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (i < fFirstVisibleTabIndex)
			item->SetVisible(false);
		else {
			float itemWidth = item->MinSize().width;
			bool visible = availableWidth >= visibleTabsWidth + itemWidth;
			item->SetVisible(visible && !canScrollTabsRight);
			visibleTabsWidth += itemWidth;
			if (!visible)
				canScrollTabsRight = true;
		}
	}
	fController->UpdateTabScrollability(canScrollTabsLeft, canScrollTabsRight);
}


float
TabContainerView::_AvailableWidthForTabs() const
{
	float width = Bounds().Width() - 10;
		// TODO: Don't really know why -10 is needed above.

	float left;
	float right;
	GroupLayout()->GetInsets(&left, NULL, &right, NULL);
	width -= left + right;

	return width;
}


void
TabContainerView::_SendFakeMouseMoved()
{
	BPoint where;
	uint32 buttons;
	GetMouse(&where, &buttons, false);
	if (Bounds().Contains(where))
		_MouseMoved(where, B_INSIDE_VIEW, NULL);
}

