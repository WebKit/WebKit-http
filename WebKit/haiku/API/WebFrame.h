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
#ifndef _WEB_FRAME_H_
#define _WEB_FRAME_H_


#include <String.h>

class BMessenger;
class BWebPage;

namespace WebCore {
class ChromeClientHaiku;
class Frame;
class FrameLoaderClientHaiku;
class KURL;
}

class WebFramePrivate;


class BWebFrame {
public:
			void				SetListener(const BMessenger& listener);

			void				LoadURL(BString url);

			void				StopLoading();
			void				Reload();

			BString				RequestedURL() const;
			BString				URL() const;

			bool				CanCopy() const;
			bool				CanCut() const;
			bool				CanPaste() const;

			void				Copy();
			void				Cut();
			void				Paste();

			bool				CanUndo() const;
			bool				CanRedo() const;

			void				Undo();
			void				Redo();

			bool				AllowsScrolling() const;
			void				SetAllowsScrolling(bool enable);

			BString				FrameSource() const;
			void				SetFrameSource(const BString& source);

			void				SetTransparent(bool transparent);
			bool				IsTransparent() const;

			BString				InnerText() const;
			BString				AsMarkup() const;
			BString				ExternalRepresentation() const;

			bool				FindString(const char* string,
									bool forward = true,
									bool caseSensitive = false,
									bool wrapSelection = true,
									bool startInSelection = true);

			bool				CanIncreaseTextSize() const;
			bool				CanDecreaseTextSize() const;

			void				IncreaseTextSize();
			void				DecreaseTextSize();

			void				ResetTextSize();

			void				SetEditable(bool editable);
			bool				IsEditable() const;

			void				SetTitle(const BString& title);
			const BString&		Title() const;

private:
	friend class BWebPage;

	friend class WebCore::ChromeClientHaiku;
	friend class WebCore::FrameLoaderClientHaiku;

								BWebFrame(BWebPage* webPage,
									BWebFrame* parentFrame,
									WebFramePrivate* data);
								~BWebFrame();

			void				Shutdown();

			void				LoadURL(WebCore::KURL);
			WebCore::Frame*		Frame() const;

private:
			float				fTextMagnifier;
			bool				fIsEditable;
			BString				fTitle;

			WebFramePrivate*	fData;
};

#endif // _WEB_FRAME_H_
