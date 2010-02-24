/*
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
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
#ifndef _WEB_VIEW_H_
#define _WEB_VIEW_H_


#include <String.h>
#include <View.h>

class BWebPage;


class BWebView : public BView {
public:
								BWebView(const char* name);
	virtual						~BWebView();

	// BView hooks
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();

	virtual	void				Draw(BRect);
	virtual	void				FrameResized(float width, float height);
	virtual	void				GetPreferredSize(float* width, float* height);
	virtual	void				MessageReceived(BMessage* message);

	virtual	void				MakeFocus(bool focused = true);
	virtual	void				WindowActivated(bool activated);

	virtual	void				MouseMoved(BPoint where, uint32 transit,
									const BMessage* dragMessage);
	virtual	void				MouseDown(BPoint where);
	virtual	void				MouseUp(BPoint where);

	virtual	void				KeyDown(const char* bytes, int32 numBytes);
	virtual	void				KeyUp(const char* bytes, int32 numBytes);

	// BWebPage API
			BWebPage*			WebPage() const { return fWebPage; }

			BString				MainFrameTitle() const;
			BString				MainFrameRequestedURL() const;
			BString				MainFrameURL() const;

			void				LoadURL(const char* urlString);
			void				GoBack();
			void				GoForward();

			void				IncreaseTextSize();
			void				DecreaseTextSize();
			void				ResetTextSize();

			void				FindString(const char* string,
									bool forward = true,
									bool caseSensitive = false,
									bool wrapSelection = true,
									bool startInSelection = false);

private:
	friend class BWebPage;
	inline	BView*				OffscreenView() const
									{ return fOffscreenView; }
			void				SetOffscreenViewClean(BRect cleanRect,
									bool immediate);
			void				InvalidateOffscreenView();

private:
			void				_ResizeOffscreenView(int width, int height);
			void				_DispatchMouseEvent(const BPoint& where,
									uint32 sanityWhat);
			void				_DispatchKeyEvent(uint32 sanityWhat);

private:
			uint32				fLastMouseButtons;

			BBitmap*			fOffscreenBitmap;
			BView*				fOffscreenView;
			bool				fOffscreenViewClean;

			BWebPage*			fWebPage;
};

#endif // _WEB_VIEW_H_
