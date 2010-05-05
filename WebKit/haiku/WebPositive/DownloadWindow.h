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
#ifndef DOWNLOAD_WINDOW_H
#define DOWNLOAD_WINDOW_H


#include <String.h>
#include <Window.h>

class BButton;
class BFile;
class BGroupLayout;
class BScrollView;
class BWebDownload;
class SettingsMessage;


class DownloadWindow : public BWindow {
public:
								DownloadWindow(BRect frame, bool visible,
									SettingsMessage* settings);
	virtual						~DownloadWindow();

	virtual	void				DispatchMessage(BMessage* message,
									BHandler* target);
	virtual	void				MessageReceived(BMessage* message);
	virtual	bool				QuitRequested();

private:
			void				_DownloadStarted(BWebDownload* download);
			void				_DownloadFinished(BWebDownload* download);
			void				_RemoveFinishedDownloads();
			void				_RemoveMissingDownloads();
			void				_ValidateButtonStatus();
			void				_SaveSettings();
			void				_LoadSettings();
			bool				_OpenSettingsFile(BFile& file, uint32 mode);

private:
			BScrollView*		fDownloadsScrollView;
			BGroupLayout*		fDownloadViewsLayout;
			BButton*			fRemoveFinishedButton;
			BButton*			fRemoveMissingButton;
			BString				fDownloadPath;
};

#endif // DOWNLOAD_WINDOW_H
