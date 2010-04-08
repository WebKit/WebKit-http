/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "URLInputGroup.h"

#include <ControlLook.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <TextView.h>
#include <Window.h>

#include <stdio.h>
#include <stdlib.h>

#include "BaseURL.h"
#include "BrowsingHistory.h"
#include "IconButton.h"
#include "TextViewCompleter.h"


class URLChoice : public BAutoCompleter::Choice {
public:
	URLChoice(const BString& choiceText, const BString& displayText,
			int32 matchPos, int32 matchLen, int32 priority)
		:
		BAutoCompleter::Choice(choiceText, displayText, matchPos, matchLen),
		fPriority(priority)
	{
	}

	bool operator<(const URLChoice& other) const
	{
		if (fPriority > other.fPriority)
			return true;
		return DisplayText() < other.DisplayText();
	}

	bool operator==(const URLChoice& other) const
	{
		return fPriority == other.fPriority
			&& DisplayText() < other.DisplayText();
	}

private:
	int32 fPriority;
};


class BrowsingHistoryChoiceModel : public BAutoCompleter::ChoiceModel {
	virtual void FetchChoicesFor(const BString& pattern)
	{
		int32 count = CountChoices();
		for (int32 i = 0; i < count; i++) {
			delete reinterpret_cast<BAutoCompleter::Choice*>(
				fChoices.ItemAtFast(i));
		}
		fChoices.MakeEmpty();

		// Search through BrowsingHistory for any matches.
		BrowsingHistory* history = BrowsingHistory::DefaultInstance();
		if (!history->Lock())
			return;

		BString lastBaseURL;
		int32 priority = INT_MAX;

		count = history->CountItems();
		for (int32 i = 0; i < count; i++) {
			BrowsingHistoryItem item = history->HistoryItemAt(i);
			const BString& choiceText = item.URL();
			int32 matchPos = choiceText.IFindFirst(pattern);
			if (matchPos < 0)
				continue;
			if (lastBaseURL.Length() > 0
				&& choiceText.FindFirst(lastBaseURL) >= 0) {
				priority--;
			} else
				priority = INT_MAX;
			lastBaseURL = baseURL(choiceText);
			fChoices.AddItem(new URLChoice(choiceText,
				choiceText, matchPos, pattern.Length(), priority));
		}

		history->Unlock();

		fChoices.SortItems(_CompareChoices);
	}

	virtual int32 CountChoices() const
	{
		return fChoices.CountItems();
	}

	virtual const BAutoCompleter::Choice* ChoiceAt(int32 index) const
	{
		return reinterpret_cast<BAutoCompleter::Choice*>(
			fChoices.ItemAt(index));
	}

	static int _CompareChoices(const void* a, const void* b)
	{
		const URLChoice* aChoice
			= *reinterpret_cast<const URLChoice* const *>(a);
		const URLChoice* bChoice
			= *reinterpret_cast<const URLChoice* const *>(b);
		if (*aChoice < *bChoice)
			return -1;
		else if (*aChoice == *bChoice)
			return 0;
		return 1;
	}

private:
	BList fChoices;
};


// #pragma mark - URLTextView


static const float kHorizontalTextRectInset = 3.0;


class URLInputGroup::URLTextView : public BTextView {
public:
								URLTextView(URLInputGroup* parent);
	virtual						~URLTextView();

	virtual	void				FrameResized(float width, float height);
	virtual	void				MouseDown(BPoint where);
	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				MakeFocus(bool focused = true);

	virtual	BSize				MinSize();
	virtual	BSize				MaxSize();

protected:
	virtual	void				InsertText(const char* inText, int32 inLength,
									int32 inOffset,
									const text_run_array* inRuns);
private:
			void				_AlignTextRect();

private:
			URLInputGroup*		fURLInputGroup;
			TextViewCompleter*	fURLAutoCompleter;
			BString				fPreviousText;
};


URLInputGroup::URLTextView::URLTextView(URLInputGroup* parent)
	:
	BTextView("url"),
	fURLInputGroup(parent),
	fURLAutoCompleter(new TextViewCompleter(this,
		new BrowsingHistoryChoiceModel())),
	fPreviousText("")
{
	MakeResizable(true);
}


URLInputGroup::URLTextView::~URLTextView()
{
	delete fURLAutoCompleter;
}


void
URLInputGroup::URLTextView::FrameResized(float width, float height)
{
	BTextView::FrameResized(width, height);
	_AlignTextRect();
}


void
URLInputGroup::URLTextView::MouseDown(BPoint where)
{
	if (!IsFocus()) {
		MakeFocus(true);
		return;
	}

	// Only pass through to base class if we already have focus.
	BTextView::MouseDown(where);
}


void
URLInputGroup::URLTextView::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_TAB:
			BView::KeyDown(bytes, numBytes);
			break;

		case B_ESCAPE:
			// Revert to text as it was when we received keyboard focus.
			SetText(fPreviousText.String());
			SelectAll();
			break;

		default:
			BTextView::KeyDown(bytes, numBytes);
			break;
	}
}

void
URLInputGroup::URLTextView::MakeFocus(bool focus)
{
	if (focus == IsFocus())
		return;

	BTextView::MakeFocus(focus);

	if (focus) {
		fPreviousText = Text();
		SelectAll();
	}

	fURLInputGroup->Invalidate();
}


BSize
URLInputGroup::URLTextView::MinSize()
{
	BSize min;
	min.height = ceilf(LineHeight(0) + kHorizontalTextRectInset);
		// we always add at least one pixel vertical inset top/bottom for
		// the text rect.
	min.width = min.height * 3;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), min);
}


BSize
URLInputGroup::URLTextView::MaxSize()
{
	BSize max(MinSize());
	max.width = B_SIZE_UNLIMITED;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), max);
}


void
URLInputGroup::URLTextView::InsertText(const char* inText, int32 inLength,
	int32 inOffset, const text_run_array* inRuns)
{
	char* buffer = NULL;

	if (strpbrk(inText, "\r\n") && inLength <= 1024) {
		buffer = (char*)malloc(inLength);

		if (buffer) {
			strcpy(buffer, inText);

			for (int32 i = 0; i < inLength; i++) {
				if (buffer[i] == '\r' || buffer[i] == '\n')
					buffer[i] = ' ';
			}
		}
	}

	BTextView::InsertText(buffer ? buffer : inText, inLength, inOffset,
		inRuns);

	free(buffer);
}


void
URLInputGroup::URLTextView::_AlignTextRect()
{
	// Layout the text rect to be in the middle, normally this means there
	// is one pixel spacing on each side.
	BRect textRect(Bounds());
	textRect.left = 0.0;
	float vInset = max_c(1,
		floorf((textRect.Height() - LineHeight(0)) / 2.0 + 0.5));
	float hInset = kHorizontalTextRectInset;

	if (be_control_look)
		hInset = be_control_look->DefaultLabelSpacing();

	textRect.InsetBy(hInset, vInset);
	SetTextRect(textRect);
}


// #pragma mark - URLInputGroup


URLInputGroup::URLInputGroup()
	:
	BGroupView(B_HORIZONTAL),
	fWindowActive(false)
{
	GroupLayout()->SetInsets(2, 2, 2, 2);

	fTextView = new URLTextView(this);
	AddChild(fTextView);

	SetFlags(Flags() | B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE);
	SetLowColor(ViewColor());
	SetViewColor(B_TRANSPARENT_COLOR);

	SetExplicitAlignment(BAlignment(B_ALIGN_USE_FULL_WIDTH,
		B_ALIGN_VERTICAL_CENTER));
}


URLInputGroup::~URLInputGroup()
{
}


void
URLInputGroup::AttachedToWindow()
{
	BGroupView::AttachedToWindow();
	fWindowActive = Window()->IsActive();
}


void
URLInputGroup::WindowActivated(bool active)
{
	BGroupView::WindowActivated(active);
	if (fWindowActive != active) {
		fWindowActive = active;
		Invalidate();
	}
}


void
URLInputGroup::Draw(BRect updateRect)
{
	BRect bounds(Bounds());
	rgb_color base(LowColor());
	uint32 flags = 0;
	if (fWindowActive && fTextView->IsFocus())
		flags |= BControlLook::B_FOCUSED;
	be_control_look->DrawTextControlBorder(this, bounds, updateRect, base,
		flags);
}


void
URLInputGroup::MakeFocus(bool focus)
{
	// Forward this to the text view, we never accept focus ourselves.
	fTextView->MakeFocus(focus);
}


BTextView*
URLInputGroup::TextView() const
{
	return fTextView;
}


void
URLInputGroup::SetText(const char* text)
{
	fTextView->SetText(text);
}


const char*
URLInputGroup::Text() const
{
	return fTextView->Text();
}

