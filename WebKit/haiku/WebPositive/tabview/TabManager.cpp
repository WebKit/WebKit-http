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

#include "TabManager.h"

#include <stdio.h>

#include <Application.h>
#include <AbstractLayoutItem.h>
#include <Bitmap.h>
#include <Button.h>
#include <CardLayout.h>
#include <ControlLook.h>
#include <GroupView.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Rect.h>
#include <SpaceLayoutItem.h>
#include <Window.h>

#include "TabContainerView.h"
#include "TabView.h"


const static BString kEmptyString;


// #pragma mark - Helper classes


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
			0, borders);
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
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_LEFT_ARROW, 0, tint);
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
		frame.OffsetBy(1, 0);
		float tint = IsEnabled() ? B_DARKEN_4_TINT : B_DARKEN_1_TINT;
		be_control_look->DrawArrowShape(this, frame, updateRect,
			base, BControlLook::B_RIGHT_ARROW, 0, tint);
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

	virtual void MouseDown(BPoint point)
	{
		if (!IsEnabled())
			return;

		// Invoke must be called before setting B_CONTROL_ON
		// for the button to stay "down"
		Invoke();
		SetValue(B_CONTROL_ON);
	}

	virtual void MouseUp(BPoint point)
	{
		// Do nothing
	}

	void MenuClosed()
	{
		SetValue(B_CONTROL_OFF);
	}
};


enum {
	MSG_SCROLL_TABS_LEFT	= 'stlt',
	MSG_SCROLL_TABS_RIGHT	= 'strt',
	MSG_OPEN_TAB_MENU		= 'otmn'
};


class TabContainerGroup : public BGroupView {
public:
	TabContainerGroup(TabContainerView* tabContainerView)
		:
		BGroupView(B_HORIZONTAL, 0.0),
		fTabContainerView(tabContainerView),
		fScrollLeftTabButton(NULL),
		fScrollRightTabButton(NULL),
		fTabMenuButton(NULL)
	{
	}

	virtual void AttachedToWindow()
	{
		if (fScrollLeftTabButton != NULL)
			fScrollLeftTabButton->SetTarget(this);
		if (fScrollRightTabButton != NULL)
			fScrollRightTabButton->SetTarget(this);
		if (fTabMenuButton != NULL)
			fTabMenuButton->SetTarget(this);
	}

	virtual void MessageReceived(BMessage* message)
	{
		switch (message->what) {
			case MSG_SCROLL_TABS_LEFT:
				fTabContainerView->SetFirstVisibleTabIndex(
					fTabContainerView->FirstVisibleTabIndex() - 1);
				break;
			case MSG_SCROLL_TABS_RIGHT:
				fTabContainerView->SetFirstVisibleTabIndex(
					fTabContainerView->FirstVisibleTabIndex() + 1);
				break;
			case MSG_OPEN_TAB_MENU:
			{
				BPopUpMenu* tabMenu = new BPopUpMenu("tab menu", true, false);
				int tabCount = fTabContainerView->GetLayout()->CountItems();
				for (int i = 0; i < tabCount; i++) {
					TabView* tab = fTabContainerView->TabAt(i);
					if (tab) {
						BMenuItem* item = new BMenuItem(tab->Label(), NULL);
						tabMenu->AddItem(item);
						if (tab->IsFront())
							item->SetMarked(true);
					}
				}

				// Force layout to get the final menu size. InvalidateLayout()
				// did not seem to work here.
				tabMenu->AttachedToWindow();
				BRect buttonFrame = fTabMenuButton->Frame();
				BRect menuFrame = tabMenu->Frame();
				BPoint openPoint = ConvertToScreen(buttonFrame.LeftBottom());
				// Open with the right side of the menu aligned with the right
				// side of the button and a little below.
				openPoint.x -= menuFrame.Width() - buttonFrame.Width();
				openPoint.y += 2;

				BMenuItem *selected = tabMenu->Go(openPoint, false, false,
					ConvertToScreen(buttonFrame));
				if (selected) {
					selected->SetMarked(true);
					int32 index = tabMenu->IndexOf(selected);
					if (index != B_ERROR)
						fTabContainerView->SelectTab(index);
				}
				fTabMenuButton->MenuClosed();
				delete tabMenu;
				
				break;
			}
			default:
				BGroupView::MessageReceived(message);
				break;
		}
	}

	void AddScrollLeftButton(TabButton* button)
	{
		fScrollLeftTabButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void AddScrollRightButton(TabButton* button)
	{
		fScrollRightTabButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void AddTabMenuButton(TabMenuTabButton* button)
	{
		fTabMenuButton = button;
		GroupLayout()->AddView(button, 0.0f);
	}

	void EnableScrollButtons(bool canScrollLeft, bool canScrollRight)
	{
		fScrollLeftTabButton->SetEnabled(canScrollLeft);
		fScrollRightTabButton->SetEnabled(canScrollRight);
		if (!canScrollLeft && !canScrollRight) {
			// hide scroll buttons
		} else {
			// show scroll buttons
		}
	}

private:
	TabContainerView*	fTabContainerView;
	TabButton*			fScrollLeftTabButton;
	TabButton*			fScrollRightTabButton;
	TabMenuTabButton*	fTabMenuButton;
};


class TabButtonContainer : public BGroupView {
public:
	TabButtonContainer()
		:
		BGroupView(B_HORIZONTAL, 0.0)
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


class TabManagerController : public TabContainerView::Controller {
public:
	TabManagerController(TabManager* manager);

	virtual ~TabManagerController();

	virtual void TabSelected(int32 index)
	{
		fManager->SelectTab(index);
	}

	virtual bool HasFrames()
	{
		return false;
	}

	virtual TabView* CreateTabView();

	virtual void DoubleClickOutsideTabs();

	virtual void UpdateTabScrollability(bool canScrollLeft,
		bool canScrollRight)
	{
		fTabContainerGroup->EnableScrollButtons(canScrollLeft, canScrollRight);
	}

	virtual	void SetToolTip(const BString& text)
	{
		if (fCurrentToolTip == text)
			return;
		fCurrentToolTip = text;
		fManager->GetTabContainerView()->HideToolTip();
		fManager->GetTabContainerView()->SetToolTip(
			reinterpret_cast<BToolTip*>(NULL));
		fManager->GetTabContainerView()->SetToolTip(fCurrentToolTip.String());
	}

	void CloseTab(int32 index);

	void SetCloseButtonsAvailable(bool available)
	{
		fCloseButtonsAvailable = available;
	}

	bool CloseButtonsAvailable() const
	{
		return fCloseButtonsAvailable;
	}

	void SetDoubleClickOutsideTabsMessage(const BMessage& message,
		const BMessenger& target);

	void SetTabContainerGroup(TabContainerGroup* tabContainerGroup)
	{
		fTabContainerGroup = tabContainerGroup;
	}

private:
	TabManager*			fManager;
	TabContainerGroup*	fTabContainerGroup;
	bool				fCloseButtonsAvailable;
	BMessage*			fDoubleClickOutsideTabsMessage;
	BMessenger			fTarget;
	BString				fCurrentToolTip;
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
	bool fCloseOnMouseUp;
};


WebTabView::WebTabView(TabManagerController* controller)
	:
	TabView(),
	fIcon(NULL),
	fController(controller),
	fOverCloseRect(false),
	fClicked(false),
	fCloseOnMouseUp(false)
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
		else {
			// Try to scale down the icon by an even factor so the
			// final size is between 14 and 18 pixel size. If this fails,
			// the icon will simply be displayed at 18x18.
			float scale = 2;
			while ((fIcon->Bounds().Width() + 1) / scale > kIconSize)
				scale *= 2;
			if ((fIcon->Bounds().Width() + 1) / scale >= kIconSize - 4
				&& (fIcon->Bounds().Height() + 1) / scale >= kIconSize - 4
				&& (fIcon->Bounds().Height() + 1) / scale <= kIconSize) {
				iconBounds.right = (fIcon->Bounds().Width() + 1) / scale - 1;
				iconBounds.bottom = (fIcon->Bounds().Height() + 1) / scale - 1;
			}
		}
		BPoint iconPos(frame.left + kIconInset - 1,
			frame.top + floorf((frame.Height() - iconBounds.Height()) / 2));
		iconBounds.OffsetTo(iconPos);
		owner->SetDrawingMode(B_OP_ALPHA);
		owner->SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);
		owner->DrawBitmap(fIcon, fIcon->Bounds(), iconBounds,
			B_FILTER_BITMAP_BILINEAR);
		owner->SetDrawingMode(B_OP_COPY);
		frame.left = frame.left + kIconSize + kIconInset * 2;
	}

	TabView::DrawContents(owner, frame, updateRect, isFirst, isLast, isFront);
}


void
WebTabView::MouseDown(BPoint where, uint32 buttons)
{
	if (buttons & B_TERTIARY_MOUSE_BUTTON) {
		fCloseOnMouseUp = true;
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
	if (!fClicked && !fCloseOnMouseUp) {
		TabView::MouseUp(where);
		return;
	}

	if (fCloseOnMouseUp && Frame().Contains(where)) {
		fCloseOnMouseUp = false;
		fController->CloseTab(ContainerView()->IndexOf(this));
		// Probably this object is toast now, better return here.
		return;
	}

	fClicked = false;
	fCloseOnMouseUp = false;

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

	fController->SetToolTip(Label());

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
	fTabContainerGroup(NULL),
	fCloseButtonsAvailable(false),
	fDoubleClickOutsideTabsMessage(NULL)
{
}


TabManagerController::~TabManagerController()
{
	delete fDoubleClickOutsideTabsMessage;
}


TabView*
TabManagerController::CreateTabView()
{
	return new WebTabView(this);
}


void
TabManagerController::DoubleClickOutsideTabs()
{
	fTarget.SendMessage(fDoubleClickOutsideTabsMessage);
}


void
TabManagerController::CloseTab(int32 index)
{
	fManager->CloseTab(index);
}


void
TabManagerController::SetDoubleClickOutsideTabsMessage(const BMessage& message,
	const BMessenger& target)
{
	delete fDoubleClickOutsideTabsMessage;
	fDoubleClickOutsideTabsMessage = new BMessage(message);
	fTarget = target;
}


// #pragma mark - TabManager


TabManager::TabManager(const BMessenger& target, BMessage* newTabMessage)
    :
    fController(new TabManagerController(this)),
    fTarget(target)
{
	fController->SetDoubleClickOutsideTabsMessage(*newTabMessage,
		be_app_messenger);

	fContainerView = new BView("web view container", 0);
	fCardLayout = new BCardLayout();
	fContainerView->SetLayout(fCardLayout);

	fTabContainerView = new TabContainerView(fController);
	fTabContainerGroup = new TabContainerGroup(fTabContainerView);
	fTabContainerGroup->GroupLayout()->SetInsets(0, 3, 0, 0);

	fController->SetTabContainerGroup(fTabContainerGroup);

#if INTEGRATE_MENU_INTO_TAB_BAR
	fMenu = new BMenu("Menu");
	BMenuBar* menuBar = new BMenuBar("Menu bar");
	menuBar->AddItem(fMenu);
	TabButtonContainer* menuBarContainer = new TabButtonContainer();
	menuBarContainer->GroupLayout()->AddView(menuBar);
	fTabContainerGroup->GroupLayout()->AddView(menuBarContainer, 0.0f);
#endif

	fTabContainerGroup->GroupLayout()->AddView(fTabContainerView);
	fTabContainerGroup->AddScrollLeftButton(new ScrollLeftTabButton(
		new BMessage(MSG_SCROLL_TABS_LEFT)));
	fTabContainerGroup->AddScrollRightButton(new ScrollRightTabButton(
		new BMessage(MSG_SCROLL_TABS_RIGHT)));
	NewTabButton* newTabButton = new NewTabButton(newTabMessage);
	newTabButton->SetTarget(be_app);
	fTabContainerGroup->GroupLayout()->AddView(newTabButton, 0.0f);
	fTabContainerGroup->AddTabMenuButton(new TabMenuTabButton(
		new BMessage(MSG_OPEN_TAB_MENU)));
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
TabManager::GetTabContainerView() const
{
	return fTabContainerView;
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


int32
TabManager::TabForView(const BView* containedView) const
{
	int32 count = fCardLayout->CountItems();
	for (int32 i = 0; i < count; i++) {
		BLayoutItem* item = fCardLayout->ItemAt(i);
		if (item->View() == containedView)
			return i;
	}
	return -1;
}


bool
TabManager::HasView(const BView* containedView) const
{
	return TabForView(containedView) >= 0;
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
	int32 tabIndex = TabForView(containedView);
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

const BString&
TabManager::TabLabel(int32 tabIndex)
{
	TabView* tab = fTabContainerView->TabAt(tabIndex);
	if (tab)
		return tab->Label();
	else
		return kEmptyString;
}

void
TabManager::SetTabIcon(const BView* containedView, const BBitmap* icon)
{
	WebTabView* tab = dynamic_cast<WebTabView*>(fTabContainerView->TabAt(
		TabForView(containedView)));
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


