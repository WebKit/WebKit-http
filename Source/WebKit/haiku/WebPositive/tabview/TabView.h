/*
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
#ifndef TAB_VIEW_H
#define TAB_VIEW_H

#include <AbstractLayoutItem.h>
#include <Rect.h>
#include <String.h>


class BMessage;
class BView;
class TabContainerView;
class TabLayoutItem;


class TabView {
public:
								TabView();
	virtual						~TabView();

	virtual	BSize				MinSize();
	virtual	BSize				PreferredSize();
	virtual	BSize				MaxSize();

			void 				Draw(BRect updateRect);
	virtual	void				DrawBackground(BView* owner, BRect frame,
									const BRect& updateRect, bool isFirst,
									bool isLast, bool isFront);
	virtual	void				DrawContents(BView* owner, BRect frame,
									const BRect& updateRect, bool isFirst,
									bool isLast, bool isFront);

	virtual	void				MouseDown(BPoint where, uint32 buttons);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

			void				SetIsFront(bool isFront);
			bool				IsFront() const;
			void				SetIsLast(bool isLast);
	virtual	void				Update(bool isFirst, bool isLast,
									bool isFront);

			BLayoutItem*		LayoutItem() const;
			void				SetContainerView(
									TabContainerView* containerView);
			TabContainerView*	ContainerView() const;

			void				SetLabel(const char* label);
			const BString&		Label() const;

			BRect				Frame() const;

private:
			float				_LabelHeight() const;

private:
			TabContainerView*	fContainerView;
			TabLayoutItem*		fLayoutItem;

			BString				fLabel;

			bool				fIsFirst;
			bool				fIsLast;
			bool				fIsFront;
};


class TabLayoutItem : public BAbstractLayoutItem {
public:
								TabLayoutItem(TabView* parent);

	virtual	bool				IsVisible();
	virtual	void				SetVisible(bool visible);

	virtual	BRect				Frame();
	virtual	void				SetFrame(BRect frame);

	virtual	BView*				View();

	virtual	BSize				BaseMinSize();
	virtual	BSize				BaseMaxSize();
	virtual	BSize				BasePreferredSize();
	virtual	BAlignment			BaseAlignment();

			TabView*			Parent() const;
			void				InvalidateContainer();
			void				InvalidateContainer(BRect frame);
private:
			TabView*			fParent;
			BRect				fFrame;
			bool				fVisible;
};



#endif // TAB_VIEW_H
