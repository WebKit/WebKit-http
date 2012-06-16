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
#ifndef TAB_CONTAINER_VIEW_H
#define TAB_CONTAINER_VIEW_H

#include <GroupView.h>


class TabView;


class TabContainerView : public BGroupView {
public:
	class Controller {
	public:
		virtual	void			TabSelected(int32 tabIndex) = 0;
		virtual	bool			HasFrames() = 0;
		virtual	TabView*		CreateTabView() = 0;
		virtual	void			DoubleClickOutsideTabs() = 0;
		virtual	void			UpdateTabScrollability(bool canScrollLeft,
									bool canScrollRight) = 0;
		virtual	void			SetToolTip(const BString& text) = 0;
	};

public:
								TabContainerView(Controller* controller);
	virtual						~TabContainerView();

	virtual	BSize				MinSize();

	virtual	void				MessageReceived(BMessage*);

	virtual	void				Draw(BRect updateRect);

	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);
	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);

	virtual	void				DoLayout();

			void				AddTab(const char* label, int32 index = -1);
			void				AddTab(TabView* tab, int32 index = -1);
			TabView*			RemoveTab(int32 index);
			TabView*			TabAt(int32 index) const;

			int32				IndexOf(TabView* tab) const;

			void				SelectTab(int32 tabIndex);
			void				SelectTab(TabView* tab);

			void				SetTabLabel(int32 tabIndex, const char* label);

			void				SetFirstVisibleTabIndex(int32 index);
			int32				FirstVisibleTabIndex() const;
			int32				MaxFirstVisibleTabIndex() const;

			bool				CanScrollLeft() const;
			bool				CanScrollRight() const;

private:
			TabView*			_TabAt(const BPoint& where) const;
			void				_MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
			void				_ValidateTabVisibility();
			void				_UpdateTabVisibility();
			float				_AvailableWidthForTabs() const;
			void				_SendFakeMouseMoved();

private:
			TabView*			fLastMouseEventTab;
			bool				fMouseDown;
			uint32				fClickCount;
			TabView*			fSelectedTab;
			Controller*			fController;
			int32				fFirstVisibleTabIndex;
};

#endif // TAB_CONTAINER_VIEW_H
