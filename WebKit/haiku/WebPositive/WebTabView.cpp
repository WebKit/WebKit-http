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

#include "WebTabView.h"

#include "WebView.h"
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
#include <stdio.h>


class TabView;
class TabContainerView;


class TabLayoutItem : public BAbstractLayoutItem {
public:
	TabLayoutItem(TabView* parent);

	virtual	bool IsVisible();
	virtual	void SetVisible(bool visible);

	virtual	BRect Frame();
	virtual	void SetFrame(BRect frame);

	virtual	BView* View();

	virtual	BSize BaseMinSize();
	virtual	BSize BaseMaxSize();
	virtual	BSize BasePreferredSize();
	virtual	BAlignment BaseAlignment();

	TabView* Parent() const;

private:
	TabView* fParent;
	BRect fFrame;
};


class TabView {
public:
	TabView();
	virtual ~TabView();

	virtual BSize MinSize();
	virtual BSize PreferredSize();
	virtual BSize MaxSize();

	void Draw(BRect updateRect);
	virtual void DrawBackground(BView* owner, BRect frame,
		const BRect& updateRect, bool isFirst, bool isLast, bool isFront);
	virtual void DrawContents(BView* owner, BRect frame,
		const BRect& updateRect, bool isFirst, bool isLast, bool isFront);

	virtual void MouseDown(BPoint where, uint32 buttons);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage);

	void SetIsFront(bool isFront);
	bool IsFront() const;
	void SetIsLast(bool isLast);
	virtual void Update(bool isFirst, bool isLast, bool isFront);

	BLayoutItem* LayoutItem() const;
	void SetContainerView(TabContainerView* containerView);
	TabContainerView* ContainerView() const;

	void SetLabel(const char* label);
	const BString& Label() const;

	BRect Frame() const;

private:
	float _LabelHeight() const;

private:
	TabContainerView* fContainerView;
	TabLayoutItem* fLayoutItem;
	
	BString fLabel;

	bool fIsFirst;
	bool fIsLast;
	bool fIsFront;
};


class TabContainerView : public BGroupView {
public:
	class Controller {
	public:
		virtual void TabSelected(int32 tabIndex) = 0;
		virtual bool HasFrames() = 0;
		virtual	TabView* CreateTabView() = 0;
	};

public:
	TabContainerView(Controller* controller);
	virtual ~TabContainerView();

	virtual BSize MinSize();

	virtual void MessageReceived(BMessage*);

	virtual void Draw(BRect updateRect);

	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage);

	void AddTab(const char* label, int32 index = -1);
	void AddTab(TabView* tab, int32 index = -1);
	TabView* RemoveTab(int32 index);
	TabView* TabAt(int32 index) const;

	int32 IndexOf(TabView* tab) const;

	void SelectTab(int32 tabIndex);
	void SelectTab(TabView* tab);

	void SetTabLabel(int32 tabIndex, const char* label);

private:
	TabView* _TabAt(const BPoint& where) const;

private:
	TabView* fLastMouseEventTab;
	bool fMouseDown;
	TabView* fSelectedTab;
	Controller* fController;
};


// #pragma mark - TabContainerView


static const float kLeftTabInset = 4;


TabContainerView::TabContainerView(Controller* controller)
	:
	BGroupView(B_HORIZONTAL),
	fLastMouseEventTab(NULL),
	fMouseDown(false),
	fSelectedTab(NULL),
	fController(controller)
{
	SetFlags(Flags() | B_WILL_DRAW);
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
		if (!item)
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
	fMouseDown = true;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	if (fLastMouseEventTab)
		fLastMouseEventTab->MouseDown(where, buttons);
}


void
TabContainerView::MouseUp(BPoint where)
{
	fMouseDown = false;
	if (fLastMouseEventTab)
		fLastMouseEventTab->MouseUp(where);
}


void
TabContainerView::MouseMoved(BPoint where, uint32 transit,
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

	if (fLastMouseEventTab && fLastMouseEventTab == tab)
		fLastMouseEventTab->MouseMoved(where, B_INSIDE_VIEW, dragMessage);
	else {
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_EXITED_VIEW, dragMessage);
		fLastMouseEventTab = tab;
		if (fLastMouseEventTab)
			fLastMouseEventTab->MouseMoved(where, B_ENTERED_VIEW, dragMessage);
	}
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


TabView*
TabContainerView::_TabAt(const BPoint& where) const
{
	BGroupLayout* layout = GroupLayout();
	int32 count = layout->CountItems() - 1;
	for (int32 i = 0; i < count; i++) {
		TabLayoutItem* item = dynamic_cast<TabLayoutItem*>(
			layout->ItemAt(i));
		if (item && item->Frame().Contains(where))
			return item->Parent();
	}
	return NULL;
}


// #pragma mark - TabView


TabView::TabView()
	: fContainerView(NULL)
	, fLayoutItem(new TabLayoutItem(this))
	, fLabel()
{
}

TabView::~TabView()
{
	// The layout item is deleted for us by the layout which contains it.
	if (!fContainerView)
		delete fLayoutItem;
}

BSize TabView::MinSize()
{
	BSize size(MaxSize());
	size.width = 100.0f;
	return size;
}

BSize TabView::PreferredSize()
{
	return MaxSize();
}

BSize TabView::MaxSize()
{
	float extra = be_control_look->DefaultLabelSpacing();
	float labelWidth = fContainerView->StringWidth(fLabel.String()) + 2 * extra;
	labelWidth = min_c(300.0f, labelWidth);
	return BSize(labelWidth, _LabelHeight() + extra);
}

void TabView::Draw(BRect updateRect)
{
	BRect frame(fLayoutItem->Frame());
	if (fIsFront) {
	    // Extend the front tab outward left/right in order to merge
	    // the frames of adjacent tabs.
	    if (!fIsFirst)
	    	frame.left--;
	    if (!fIsLast)
	    	frame.right++;

	   	frame.bottom++;
	}
	DrawBackground(fContainerView, frame, updateRect, fIsFirst, fIsLast,
		fIsFront);
	if (fIsFront) {
	    frame.top += 3.0f;
	    if (!fIsFirst)
	    	frame.left++;
	    if (!fIsLast)
	    	frame.right--;
	} else
		frame.top += 6.0f;
	float spacing = be_control_look->DefaultLabelSpacing();
	frame.InsetBy(spacing, spacing / 2);
	DrawContents(fContainerView, frame, updateRect, fIsFirst, fIsLast,
		fIsFront);
}

void TabView::DrawBackground(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	uint32 borders = BControlLook::B_TOP_BORDER
		| BControlLook::B_BOTTOM_BORDER;
	if (isFirst)
		borders |= BControlLook::B_LEFT_BORDER;
	if (isLast)
		borders |= BControlLook::B_RIGHT_BORDER;
	if (isFront) {
		be_control_look->DrawActiveTab(owner, frame, updateRect, base,
			0, borders);
	} else {
		be_control_look->DrawInactiveTab(owner, frame, updateRect, base,
			0, borders);
	}
}

void TabView::DrawContents(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	be_control_look->DrawLabel(owner, fLabel.String(), frame, updateRect,
		base, 0, BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
}

void TabView::MouseDown(BPoint where, uint32 buttons)
{
	fContainerView->SelectTab(this);
}

void TabView::MouseUp(BPoint where)
{
}

void TabView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
}

void TabView::SetIsFront(bool isFront)
{
	Update(fIsFirst, fIsLast, isFront);
}

bool TabView::IsFront() const
{
	return fIsFront;
}

void TabView::SetIsLast(bool isLast)
{
	Update(fIsFirst, isLast, fIsFront);
}

void TabView::Update(bool isFirst, bool isLast, bool isFront)
{
	if (fIsFirst == isFirst && fIsLast == isLast && fIsFront == isFront)
		return;
	fIsFirst = isFirst;
	fIsLast = isLast;
	fIsFront = isFront;
	BRect frame = fLayoutItem->Frame();
	frame.bottom++;
	fContainerView->Invalidate(frame);
}

void TabView::SetContainerView(TabContainerView* containerView)
{
	fContainerView = containerView;
}

TabContainerView* TabView::ContainerView() const
{
	return fContainerView;
}

BLayoutItem* TabView::LayoutItem() const
{
	return fLayoutItem;
}

void TabView::SetLabel(const char* label)
{
	if (fLabel == label)
		return;
	fLabel = label;
	fLayoutItem->InvalidateLayout();
}

const BString& TabView::Label() const
{
	return fLabel;
}


BRect
TabView::Frame() const
{
	return fLayoutItem->Frame();
}


float TabView::_LabelHeight() const
{
	font_height fontHeight;
	fContainerView->GetFontHeight(&fontHeight);
	return ceilf(fontHeight.ascent) + ceilf(fontHeight.descent);
}

// #pragma mark - TabLayoutItem

TabLayoutItem::TabLayoutItem(TabView* parent)
	: fParent(parent)
{
}

bool TabLayoutItem::IsVisible()
{
	return !fParent->ContainerView()->IsHidden(fParent->ContainerView());
}

void TabLayoutItem::SetVisible(bool visible)
{
	// not allowed
}

BRect TabLayoutItem::Frame()
{
	return fFrame;
}

void TabLayoutItem::SetFrame(BRect frame)
{
	BRect dirty = fFrame;
	fFrame = frame;
	dirty = dirty | fFrame;
	// Invalidate more than necessary, to help the TabContainerView
	// redraw the parts outside any tabs...
	dirty.bottom++;
	dirty.right++;
	fParent->ContainerView()->Invalidate(dirty);
}

BView* TabLayoutItem::View()
{
	return NULL;
}

BSize TabLayoutItem::BaseMinSize()
{
	return fParent->MinSize();
}

BSize TabLayoutItem::BaseMaxSize()
{
	return fParent->MaxSize();
}

BSize TabLayoutItem::BasePreferredSize()
{
	return fParent->PreferredSize();
}

BAlignment TabLayoutItem::BaseAlignment()
{
	return BAlignment(B_ALIGN_USE_FULL_WIDTH, B_ALIGN_USE_FULL_HEIGHT);
}

TabView* TabLayoutItem::Parent() const
{
	return fParent;
}


// #pragma mark - TabManagerController


class TabManagerController : public TabContainerView::Controller {
public:
	TabManagerController(TabManager* manager);

	virtual void TabSelected(int32 index)
	{
		fManager->SelectTab(index);
	}

	virtual bool HasFrames()
	{
		return false;
	}

	virtual TabView* CreateTabView();

	void CloseTab(int32 index);

	void SetCloseButtonsAvailable(bool available)
	{
		fCloseButtonsAvailable = available;
	}

	bool CloseButtonsAvailable() const
	{
		return fCloseButtonsAvailable;
	}

private:
	TabManager* fManager;
	bool fCloseButtonsAvailable;
};


// #pragma mark - WebTabView


class WebTabView : public TabView {
public:
	WebTabView(TabManagerController* controller);
	~WebTabView();

	virtual BSize MaxSize();

	virtual void DrawContents(BView* owner, BRect frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront);

	virtual void MouseDown(BPoint where, uint32 buttons);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 transit,
		const BMessage* dragMessage);

	void SetIcon(const BBitmap* icon);

private:
	void _DrawCloseButton(BView* owner, BRect& frame, const BRect& updateRect,
		bool isFirst, bool isLast, bool isFront);
	BRect _CloseRectFrame(BRect frame) const;

private:
	BBitmap* fIcon;
	TabManagerController* fController;
	bool fOverCloseRect;
	bool fClicked;
};


WebTabView::WebTabView(TabManagerController* controller)
	:
	TabView(),
	fIcon(NULL),
	fController(controller),
	fOverCloseRect(false),
	fClicked(false)
{
}


WebTabView::~WebTabView()
{
	delete fIcon;
}


static const int kIconSize = 18;
static const int kIconInset = 3;


BSize
WebTabView::MaxSize()
{
	// Account for icon.
	BSize size(TabView::MaxSize());
	size.height = max_c(size.height, kIconSize + kIconInset * 2);
	if (fIcon)
		size.width += kIconSize + kIconInset * 2;
	// Account for close button.
	size.width += size.height;
	return size;
}


void
WebTabView::DrawContents(BView* owner, BRect frame, const BRect& updateRect,
	bool isFirst, bool isLast, bool isFront)
{
	if (fController->CloseButtonsAvailable())
		_DrawCloseButton(owner, frame, updateRect, isFirst, isLast, isFront);

	if (fIcon) {
		BRect iconBounds(0, 0, kIconSize - 1, kIconSize - 1);
		// clip to icon bounds, if they are smaller
		if (iconBounds.Contains(fIcon->Bounds()))
			iconBounds = fIcon->Bounds();
		BPoint iconPos(frame.left + kIconInset - 1,
			frame.top + floorf((frame.Height() - iconBounds.Height()) / 2));
		iconBounds.OffsetTo(iconPos);
		owner->SetDrawingMode(B_OP_OVER);
		owner->DrawBitmap(fIcon, fIcon->Bounds(), iconBounds);
		frame.left = frame.left + kIconSize + kIconInset * 2;
	}

	TabView::DrawContents(owner, frame, updateRect, isFirst, isLast, isFront);
}


void
WebTabView::MouseDown(BPoint where, uint32 buttons)
{
	if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		fController->CloseTab(ContainerView()->IndexOf(this));
		return;
	}

	BRect closeRect = _CloseRectFrame(Frame());
	if (!fController->CloseButtonsAvailable() || !closeRect.Contains(where)) {
		TabView::MouseDown(where, buttons);
		return;
	}

	fClicked = true;
	ContainerView()->Invalidate(closeRect);
}


void
WebTabView::MouseUp(BPoint where)
{
	if (!fClicked) {
		TabView::MouseUp(where);
		return;
	}

	fClicked = false;

	if (_CloseRectFrame(Frame()).Contains(where))
		fController->CloseTab(ContainerView()->IndexOf(this));
}


void
WebTabView::MouseMoved(BPoint where, uint32 transit,
	const BMessage* dragMessage)
{
	if (fController->CloseButtonsAvailable()) {
		BRect closeRect = _CloseRectFrame(Frame());
		bool overCloseRect = closeRect.Contains(where);
		if (overCloseRect != fOverCloseRect) {
			fOverCloseRect = overCloseRect;
			ContainerView()->Invalidate(closeRect);
		}
	}

	TabView::MouseMoved(where, transit, dragMessage);
}


void
WebTabView::SetIcon(const BBitmap* icon)
{
	delete fIcon;
	if (icon)
		fIcon = new BBitmap(icon);
	else
		fIcon = NULL;
	LayoutItem()->InvalidateLayout();
}


BRect
WebTabView::_CloseRectFrame(BRect frame) const
{
	frame.left = frame.right - frame.Height();
	return frame;
}


void WebTabView::_DrawCloseButton(BView* owner, BRect& frame,
	const BRect& updateRect, bool isFirst, bool isLast, bool isFront)
{
	BRect closeRect = _CloseRectFrame(frame);
	frame.right = closeRect.left - be_control_look->DefaultLabelSpacing();

	closeRect.left = (closeRect.left + closeRect.right) / 2 - 3;
	closeRect.right = closeRect.left + 6;
	closeRect.top = (closeRect.top + closeRect.bottom) / 2 - 3;
	closeRect.bottom = closeRect.top + 6;

	rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
	float tint = B_DARKEN_1_TINT;
	if (!IsFront()) {
		base = tint_color(base, tint);
		tint *= 1.02;
	}

	if (fOverCloseRect)
		tint *= 1.2;

	if (fClicked && fOverCloseRect) {
		BRect buttonRect(closeRect.InsetByCopy(-4, -4));
		be_control_look->DrawButtonFrame(owner, buttonRect, updateRect,
			base, base,
			BControlLook::B_ACTIVATED | BControlLook::B_BLEND_FRAME);
		be_control_look->DrawButtonBackground(owner, buttonRect, updateRect,
			base, BControlLook::B_ACTIVATED);
		tint *= 1.2;
		closeRect.OffsetBy(1, 1);
	}

	base = tint_color(base, tint);
	owner->SetHighColor(base);
	owner->SetPenSize(2);
	owner->StrokeLine(closeRect.LeftTop(), closeRect.RightBottom());
	owner->StrokeLine(closeRect.LeftBottom(), closeRect.RightTop());
	owner->SetPenSize(1);
}


// #pragma mark - TabManagerController


TabManagerController::TabManagerController(TabManager* manager)
	:
	fManager(manager),
	fCloseButtonsAvailable(false)
{
}


TabView*
TabManagerController::CreateTabView()
{
	return new WebTabView(this);
}


void
TabManagerController::CloseTab(int32 index)
{
	fManager->CloseTab(index);
}


// #pragma mark - TabButtonContainer


class TabButtonContainer : public BGroupView {
public:
	TabButtonContainer()
		: BGroupView(B_HORIZONTAL)
	{
		SetFlags(Flags() | B_WILL_DRAW);
		SetViewColor(B_TRANSPARENT_COLOR);
		SetLowColor(ui_color(B_PANEL_BACKGROUND_COLOR));
		GroupLayout()->SetInsets(0, 6, 0, 0);
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		rgb_color base = LowColor();
		be_control_look->DrawInactiveTab(this, bounds, updateRect,
			base, 0, BControlLook::B_TOP_BORDER);
	}
};


// #pragma mark - TabButton


class TabButton : public BButton {
public:
	TabButton(BMessage* message)
		: BButton("", message)
	{
	}

	virtual BSize MinSize()
	{
		return BSize(12, 12);
	}

	virtual BSize MaxSize()
	{
		return BSize(B_SIZE_UNLIMITED, B_SIZE_UNLIMITED);
	}

	virtual BSize PreferredSize()
	{
		return MinSize();
	}

	virtual void Draw(BRect updateRect)
	{
		BRect bounds(Bounds());
		rgb_color base = ui_color(B_PANEL_BACKGROUND_COLOR);
		SetHighColor(tint_color(base, B_DARKEN_2_TINT));
		StrokeLine(bounds.LeftBottom(), bounds.RightBottom());
		bounds.bottom--;
		uint32 flags = be_control_look->Flags(this);
		uint32 borders = BControlLook::B_TOP_BORDER
			| BControlLook::B_BOTTOM_BORDER;
		be_control_look->DrawInactiveTab(this, bounds, updateRect, base,
			flags, borders);
		if (IsEnabled()) {
			rgb_color button = tint_color(base, 1.07);
			be_control_look->DrawButtonBackground(this, bounds, updateRect,
				button, flags, 0);
		}

		bounds.left = (bounds.left + bounds.right) / 2 - 6;
		bounds.top = (bounds.top + bounds.bottom) / 2 - 6;
		bounds.right = bounds.left + 12;
		bounds.bottom = bounds.top + 12;
		DrawSymbol(bounds, updateRect, base);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
	}
};


class ScrollLeftTabButton : public TabButton {
public:
	ScrollLeftTabButton(BMessage* message)
		: TabButton(message)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_LEFT_ARROW, 0, B_DARKEN_4_TINT);
	}
};


class ScrollRightTabButton : public TabButton {
public:
	ScrollRightTabButton(BMessage* message)
		: TabButton(message)
	{
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_RIGHT_ARROW, 0, B_DARKEN_4_TINT);
	}
};


class NewTabButton : public TabButton {
public:
	NewTabButton(BMessage* message)
		: TabButton(message)
	{
		SetToolTip("New tab (Cmd-T)");
	}

	virtual BSize MinSize()
	{
		return BSize(18, 12);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		SetHighColor(tint_color(base, B_DARKEN_4_TINT));
		float inset = 3;
		frame.InsetBy(2, 2);
		frame.top++;
		frame.left++;
		FillRoundRect(BRect(frame.left, frame.top + inset,
			frame.right, frame.bottom - inset), 1, 1);
		FillRoundRect(BRect(frame.left + inset, frame.top,
			frame.right - inset, frame.bottom), 1, 1);
	}
};


class TabMenuTabButton : public TabButton {
public:
	TabMenuTabButton(BMessage* message)
		: TabButton(message)
	{
	}

	virtual BSize MinSize()
	{
		return BSize(18, 12);
	}

	virtual void DrawSymbol(BRect frame, const BRect& updateRect,
		const rgb_color& base)
	{
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_DOWN_ARROW, 0, B_DARKEN_4_TINT);
	}
};


// #pragma mark - TabManager


TabManager::TabManager(const BMessenger& target, BMessage* newTabMessage)
    :
    fController(new TabManagerController(this)),
    fTarget(target)
{
	fContainerView = new BView("web view container", 0);
	fCardLayout = new BCardLayout();
	fContainerView->SetLayout(fCardLayout);

	fTabContainerView = new TabContainerView(fController);
	fTabContainerGroup = new BGroupView(B_HORIZONTAL);
	fTabContainerGroup->GroupLayout()->SetInsets(0, 3, 0, 0);

#if INTEGRATE_MENU_INTO_TAB_BAR
	fMenu = new BMenu("Menu");
	BMenuBar* menuBar = new BMenuBar("Menu bar");
	menuBar->AddItem(fMenu);
	TabButtonContainer* menuBarContainer = new TabButtonContainer();
	menuBarContainer->GroupLayout()->AddView(menuBar);
	fTabContainerGroup->GroupLayout()->AddView(menuBarContainer, 0.0f);
#endif

	fTabContainerGroup->GroupLayout()->AddView(fTabContainerView);
//	fTabContainerGroup->GroupLayout()->AddView(new ScrollLeftTabButton(NULL), 0.0f);
//	fTabContainerGroup->GroupLayout()->AddView(new ScrollRightTabButton(NULL), 0.0f);
	NewTabButton* newTabButton = new NewTabButton(newTabMessage);
	newTabButton->SetTarget(be_app);
	fTabContainerGroup->GroupLayout()->AddView(newTabButton, 0.0f);
//	fTabContainerGroup->GroupLayout()->AddView(new TabMenuTabButton(NULL), 0.0f);
}


TabManager::~TabManager()
{
	delete fController;
}


void
TabManager::SetTarget(const BMessenger& target)
{
    fTarget = target;
}


const BMessenger&
TabManager::Target() const
{
    return fTarget;
}


#if INTEGRATE_MENU_INTO_TAB_BAR
BMenu*
TabManager::Menu() const
{
	return fMenu;
}
#endif


BView*
TabManager::TabGroup() const
{
	return fTabContainerGroup;
}


BView*
TabManager::ContainerView() const
{
	return fContainerView;
}


BView*
TabManager::ViewForTab(int32 tabIndex) const
{
	BLayoutItem* item = fCardLayout->ItemAt(tabIndex);
	if (item != NULL)
		return item->View();
	return NULL;
}


void
TabManager::SelectTab(int32 tabIndex)
{
	fCardLayout->SetVisibleItem(tabIndex);
	fTabContainerView->SelectTab(tabIndex);

    BMessage message(TAB_CHANGED);
    message.AddInt32("tab index", tabIndex);
    fTarget.SendMessage(&message);
}


void
TabManager::SelectTab(const BView* containedView)
{
	int32 tabIndex = _TabIndexForContainedView(containedView);
	if (tabIndex > 0)
		SelectTab(tabIndex);
}


int32
TabManager::SelectedTabIndex() const
{
	return fCardLayout->VisibleIndex();
}


void
TabManager::CloseTab(int32 tabIndex)
{
    BMessage message(CLOSE_TAB);
    message.AddInt32("tab index", tabIndex);
    fTarget.SendMessage(&message);
}


void
TabManager::AddTab(BView* view, const char* label, int32 index)
{
	fTabContainerView->AddTab(label, index);
	fCardLayout->AddView(index, view);
}


BView*
TabManager::RemoveTab(int32 index)
{
	// It's important to remove the view first, since
	// removing the tab will preliminary mess with the selected tab
	// and then item count of card layout and tab container will not
	// match yet.
	BLayoutItem* item = fCardLayout->RemoveItem(index);
	if (item == NULL)
		return NULL;

	TabView* tab = fTabContainerView->RemoveTab(index);
	delete tab;

	BView* view = item->View();
	delete item;
	return view;
}


int32
TabManager::CountTabs() const
{
	return fCardLayout->CountItems();
}


void
TabManager::SetTabLabel(int32 tabIndex, const char* label)
{
	fTabContainerView->SetTabLabel(tabIndex, label);
}


void
TabManager::SetTabIcon(const BView* containedView, const BBitmap* icon)
{
	WebTabView* tab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(
		_TabIndexForContainedView(containedView)));
	if (tab)
		tab->SetIcon(icon);
}


void
TabManager::SetCloseButtonsAvailable(bool available)
{
	if (available == fController->CloseButtonsAvailable())
		return;
	fController->SetCloseButtonsAvailable(available);
	fTabContainerView->Invalidate();
}


int32
TabManager::_TabIndexForContainedView(const BView* containedView) const
{
	int32 count = fCardLayout->CountItems();
	for (int32 i = 0; i < count; i++) {
		BLayoutItem* item = fCardLayout->ItemAt(i);
		if (item->View() == containedView)
			return i;
	}
	return -1;
}

