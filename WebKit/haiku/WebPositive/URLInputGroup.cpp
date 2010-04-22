/*
 * Copyright 2010 Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "URLInputGroup.h"

#include <Bitmap.h>
#include <Button.h>
#include <ControlLook.h>
#include <Clipboard.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <SeparatorView.h>
#include <TextView.h>
#include <TranslationUtils.h>
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
private:
	static const uint32 MSG_CLEAR = 'cler';

public:
								URLTextView(URLInputGroup* parent);
	virtual						~URLTextView();

	virtual	void				MessageReceived(BMessage* message);
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
	SetStylable(true);
}


URLInputGroup::URLTextView::~URLTextView()
{
	delete fURLAutoCompleter;
}


void
URLInputGroup::URLTextView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case MSG_CLEAR:
			SetText("");
			break;
		default:
			BTextView::MessageReceived(message);
			break;
	}
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
	bool wasFocus = IsFocus();
	if (!wasFocus)
		MakeFocus(true);

	int32 buttons;
	if (Window()->CurrentMessage()->FindInt32("buttons", &buttons) != B_OK)
		buttons = B_PRIMARY_MOUSE_BUTTON;

	if ((buttons & B_SECONDARY_MOUSE_BUTTON) != 0) {
		// Display context menu
		int32 selectionStart;
		int32 selectionEnd;
		GetSelection(&selectionStart, &selectionEnd);
		bool canCutOrCopy = selectionEnd > selectionStart;

		bool canPaste = false;
		if (be_clipboard->Lock()) {
			if (BMessage* data = be_clipboard->Data())
				canPaste = data->HasData("text/plain", B_MIME_TYPE);
			be_clipboard->Unlock();
		}

		BMenuItem* cutItem = new BMenuItem("Cut", new BMessage(B_CUT));
		BMenuItem* copyItem = new BMenuItem("Copy", new BMessage(B_COPY));
		BMenuItem* pasteItem = new BMenuItem("Paste", new BMessage(B_PASTE));
		BMenuItem* clearItem = new BMenuItem("Clear", new BMessage(MSG_CLEAR));
		cutItem->SetEnabled(canCutOrCopy);
		copyItem->SetEnabled(canCutOrCopy);
		pasteItem->SetEnabled(canPaste);
		clearItem->SetEnabled(strlen(Text()) > 0);

		BPopUpMenu* menu = new BPopUpMenu("url context");
		menu->AddItem(cutItem);
		menu->AddItem(copyItem);
		menu->AddItem(pasteItem);
		menu->AddItem(clearItem);

		menu->SetTargetForItems(this);
		menu->Go(ConvertToScreen(where), true, true, true);
		return;
	}

	// Only pass through to base class if we already have focus.
	if (!wasFocus)
		return;

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

		case B_RETURN:
			// Don't let this through to the text view.
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
	// Filter all line breaks, note that inText is not terminated.
	if (inLength == 1) {
		if (*inText == '\n' || *inText == '\r')
			BTextView::InsertText(" ", 1, inOffset, inRuns);
		else
			BTextView::InsertText(inText, 1, inOffset, inRuns);
	} else {
		BString filteredText(inText, inLength);
		filteredText.ReplaceAll('\n', ' ');
		filteredText.ReplaceAll('\r', ' ');
		BTextView::InsertText(filteredText.String(), inLength, inOffset,
			inRuns);
	}

	// Make the base URL part bold.
	BString text(Text(), TextLength());
	int32 baseUrlStart = text.FindFirst("://");
	if (baseUrlStart >= 0)
		baseUrlStart += 3;
	else
		baseUrlStart = 0;
	int32 baseUrlEnd = text.FindFirst("/", baseUrlStart);
	if (baseUrlEnd < 0)
		baseUrlEnd = TextLength();
	BFont font;
	GetFont(&font);
	const rgb_color black = (rgb_color){ 0, 0, 0, 255 };
	const rgb_color gray = (rgb_color){ 60, 60, 60, 255 };
	if (baseUrlStart > 0)
		SetFontAndColor(0, baseUrlStart - 1, &font, B_FONT_ALL, &gray);
	if (baseUrlEnd > baseUrlStart) {
		font.SetFace(B_BOLD_FACE);
		SetFontAndColor(baseUrlStart, baseUrlEnd, &font, B_FONT_ALL, &black);
	}
	if (baseUrlEnd < TextLength()) {
		font.SetFace(B_REGULAR_FACE);
		SetFontAndColor(baseUrlEnd, TextLength(), &font, B_FONT_ALL, &gray);
	}
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


// #pragma mark - GoButton


const uint32 kGoBitmapWidth = 14;
const uint32 kGoBitmapHeight = 14;
const color_space kGoBitmapFormat = B_RGBA32;

const unsigned char kGoBitmapBits[] = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe0, 0x50, 0x50, 0x50, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe0,
	0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x2f, 0x2f, 0x2f, 0x56, 0x50, 0x50, 0x50, 0xff, 0x4d, 0x4d, 0x4d, 0xed,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21,
	0x49, 0x49, 0x49, 0xe0, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x50, 0x50, 0x50, 0xff, 0x37, 0x37, 0x37, 0x78, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe0, 0x50, 0x50, 0x50, 0xff,
	0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x50, 0x50, 0x50, 0xff, 0x4b, 0x4b, 0x4b, 0xec, 0x37, 0x37, 0x37, 0x77, 0x00, 0x00, 0x00, 0x02,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe1, 0x50, 0x50, 0x50, 0xff, 0x50, 0x50, 0x50, 0xff,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21,
	0x49, 0x49, 0x49, 0xe1, 0x50, 0x50, 0x50, 0xff, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21, 0x49, 0x49, 0x49, 0xe1,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x17, 0x17, 0x17, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};


class BitmapButton : public BButton {
	static const float kFrameInset = 2;

public:
	BitmapButton(const char* resourceName, BMessage* message)
		:
		BButton("", message),
		fBitmap(BTranslationUtils::GetBitmap(resourceName))
	{
	}

	BitmapButton(const uint8* bits, uint32 width, uint32 height,
			color_space format, BMessage* message)
		:
		BButton("", message),
		fBitmap(new BBitmap(BRect(0, 0, width - 1, height - 1), 0, format))
	{
		memcpy(fBitmap->Bits(), bits, fBitmap->BitsLength());
	}

	~BitmapButton()
	{
		delete fBitmap;
	}

	virtual BSize MinSize()
	{
		BSize min(0, 0);
		if (fBitmap) {
			min.width = fBitmap->Bounds().Width();
			min.height = fBitmap->Bounds().Height();
		}
		min.width += kFrameInset * 2;
		min.height += kFrameInset * 2;
		return min;
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
		uint32 flags = be_control_look->Flags(this);

		be_control_look->DrawButtonBackground(this, bounds, updateRect, base,
			flags);

		SetDrawingMode(B_OP_ALPHA);

		if (!IsEnabled()) {
			SetBlendingMode(B_CONSTANT_ALPHA, B_ALPHA_OVERLAY);
			SetHighColor(0, 0, 0, 120);
		}

		BPoint bitmapLocation(kFrameInset, kFrameInset);
		if (Value() == B_CONTROL_ON)
			bitmapLocation += BPoint(1, 1);

		DrawBitmap(fBitmap, bitmapLocation);
	}

private:
	BBitmap* fBitmap;
};


// #pragma mark - URLInputGroup


URLInputGroup::URLInputGroup(BMessage* goMessage)
	:
	BGroupView(B_HORIZONTAL),
	fWindowActive(false)
{
	GroupLayout()->SetInsets(2, 2, 2, 2);

	fTextView = new URLTextView(this);
	AddChild(fTextView);

	AddChild(new BSeparatorView(B_VERTICAL, B_PLAIN_BORDER));

// TODO: Fix in Haiku, no in-built support for archived BBitmaps from
// resources?
//	fGoButton = new BitmapButton("kActionGo", NULL);
	fGoButton = new BitmapButton(kGoBitmapBits, kGoBitmapWidth,
		kGoBitmapHeight, kGoBitmapFormat, goMessage);
	GroupLayout()->AddView(fGoButton, 0.0);

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
	if (!text || !Text() || strcmp(Text(), text) != 0)
		fTextView->SetText(text);
}


const char*
URLInputGroup::Text() const
{
	return fTextView->Text();
}


BButton*
URLInputGroup::GoButton() const
{
	return fGoButton;
}


