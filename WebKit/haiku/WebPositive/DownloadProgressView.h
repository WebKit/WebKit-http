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
#ifndef DOWNLOAD_PROGRESS_VIEW_H
#define DOWNLOAD_PROGRESS_VIEW_H


#include <GroupView.h>
#include <Path.h>
#include <String.h>

class BEntry;
class BStatusBar;
class BStringView;
class BWebDownload;
class IconView;
class SmallButton;


enum {
	SAVE_SETTINGS = 'svst'
};


class DownloadProgressView : public BGroupView {
public:
								DownloadProgressView(BWebDownload* download);
								DownloadProgressView(const BMessage* archive);

			bool				Init(BMessage* archive = NULL);

			status_t			SaveSettings(BMessage* archive);
	virtual	void				AttachedToWindow();
	virtual	void				DetachedFromWindow();
	virtual	void				AllAttached();

	virtual	void				Draw(BRect updateRect);

	virtual	void				MessageReceived(BMessage* message);

			BWebDownload*		Download() const;
			const BString&		URL() const;
			bool				IsMissing() const;
			bool				IsFinished() const;

			void				DownloadFinished();
			void				DownloadCanceled();

	static	void				SpeedVersusEstimatedFinishTogglePulse();

private:
			void				_UpdateStatus(off_t currentSize,
									off_t expectedSize);
			void				_UpdateStatusText();
			void				_StartNodeMonitor(const BEntry& entry);
			void				_StopNodeMonitor();

private:
			IconView*			fIconView;
			BStatusBar*			fStatusBar;
			BStringView*		fInfoView;
			SmallButton*		fTopButton;
			SmallButton*		fBottomButton;
			BWebDownload*		fDownload;
			BString				fURL;
			BPath				fPath;

			off_t				fCurrentSize;
			off_t				fExpectedSize;
			off_t				fLastSpeedReferenceSize;
			off_t				fEstimatedFinishReferenceSize;
			bigtime_t			fLastUpdateTime;
			bigtime_t			fLastSpeedReferenceTime;
			bigtime_t			fProcessStartTime;
			bigtime_t			fLastSpeedUpdateTime;
			bigtime_t			fEstimatedFinishReferenceTime;
	static	const size_t		kBytesPerSecondSlots = 10;
			size_t				fCurrentBytesPerSecondSlot;
			double				fBytesPerSecondSlot[kBytesPerSecondSlots];
			double				fBytesPerSecond;

	static	bigtime_t			sLastEstimatedFinishSpeedToggleTime;
	static	bool				sShowSpeed;
};

#endif // DOWNLOAD_PROGRESS_VIEW_H
