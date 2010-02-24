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

#include "config.h"
#include "WebDownload.h"

#include "WebDownloadPrivate.h"
#include "WebPage.h"

BWebDownload::BWebDownload(BPrivate::WebDownloadPrivate* data)
    : fData(data)
{
	ASSERT(fData);

	fData->setDownload(this);
}

BWebDownload::~BWebDownload()
{
	delete fData;
}

void BWebDownload::Start()
{
    fData->start();
}

void BWebDownload::Cancel()
{
	// This is invoked from the client, within any thread, so we need to
	// dispatch this asynchronously so that it is actually handled in the
	// main thread.
    fData->webPage()->cancelDownload(this);
}

void BWebDownload::SetProgressListener(const BMessenger& listener)
{
	fData->setProgressListener(listener);
}

const BString& BWebDownload::URL() const
{
	return fData->url();
}

const BPath& BWebDownload::Path() const
{
	return fData->path();
}

const BString& BWebDownload::Filename() const
{
	return fData->filename();
}

off_t BWebDownload::CurrentSize() const
{
	return fData->currentSize();
}

off_t BWebDownload::ExpectedSize() const
{
	return fData->expectedSize();
}

