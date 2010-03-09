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
#include "NetworkCookie.h"

#include <Message.h>
#include <stdio.h>

BNetworkCookie::BNetworkCookie(const BString& rawValue)
    : fExpirationDate()
    , fRawValue()
{
	_ParseRawValue(rawValue);
}

BNetworkCookie::BNetworkCookie(const BMessage& archive)
    : fExpirationDate()
    , fRawValue()
{
	BString rawValue;
	if (archive.FindString("raw value", &rawValue) == B_OK)
		_ParseRawValue(rawValue);
}

BNetworkCookie::~BNetworkCookie()
{
}

status_t BNetworkCookie::Archive(BMessage* into, bool deep) const
{
	if (!into)
		return B_BAD_VALUE;

	return into->AddString("raw value", AsRawValue().String());
}

BString BNetworkCookie::AsRawValue() const
{
	// TODO: Recompose the raw value, once parsing into individual fields
	// is implemented.
	return fRawValue;
}

const BDateTime& BNetworkCookie::ExpirationDate() const
{
	return fExpirationDate;
}

// #pragma mark - private

void BNetworkCookie::_ParseRawValue(const BString& rawValue)
{
	// TODO: Parse into individual fields. Most important is the
	// expiration date, but also stuff like Domain, for security reasons.
	fRawValue = rawValue;
}
