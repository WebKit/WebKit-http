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

BNetworkCookie::BNetworkCookie(const BNetworkCookie& other)
    : fExpirationDate()
    , fRawValue()
{
	*this = other;
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

BNetworkCookie& BNetworkCookie::operator=(const BNetworkCookie& other)
{
	if (this == &other)
		return *this;

	fExpirationDate = other.fExpirationDate;
	fDomain = other.fDomain;
	fPath = other.fPath;
	fComment = other.fComment;
	fName = other.fName;
	fValue = other.fValue;
	fIsSecure = other.fIsSecure;
	fIsHTTPOnly = other.fIsHTTPOnly;
	fRawValue = other.fRawValue;

	return *this;
}

bool BNetworkCookie::operator==(const BNetworkCookie& other) const
{
	if (this == &other)
		return true;

	return fExpirationDate == other.fExpirationDate
		&& fDomain == other.fDomain
		&& fPath == other.fPath
		&& fComment == other.fComment
		&& fName == other.fName
		&& fValue == other.fValue
		&& fIsSecure == other.fIsSecure
		&& fIsHTTPOnly == other.fIsHTTPOnly;
}

bool BNetworkCookie::operator!=(const BNetworkCookie& other) const
{
	return !(*this == other);
}

void BNetworkCookie::SetExpirationDate(const BDateTime& dateTime)
{
	fExpirationDate = dateTime;
}

const BDateTime& BNetworkCookie::ExpirationDate() const
{
	return fExpirationDate;
}

void BNetworkCookie::SetDomain(const BString& domain)
{
	fDomain = domain;
}

const BString& BNetworkCookie::Domain() const
{
	return fDomain;
}

void BNetworkCookie::SetPath(const BString& path)
{
	fPath = path;
}

const BString& BNetworkCookie::Path() const
{
	return fPath;
}

void BNetworkCookie::SetComment(const BString& comment)
{
	fComment = comment;
}

const BString& BNetworkCookie::Comment() const
{
	return fComment;
}

void BNetworkCookie::SetName(const BString& name)
{
	fName = name;
}

const BString& BNetworkCookie::Name() const
{
	return fName;
}

void BNetworkCookie::SetValue(const BString& value)
{
	fValue = value;
}

const BString& BNetworkCookie::Value() const
{
	return fValue;
}

void BNetworkCookie::SetIsSecure(bool isSecure)
{
	fIsSecure = isSecure;
}

bool BNetworkCookie::IsSecure() const
{
	return fIsSecure;
}

void BNetworkCookie::SetIsHTTPOnly(bool isHTTPOnly)
{
	fIsHTTPOnly = isHTTPOnly;
}

bool BNetworkCookie::IsHTTPOnly() const
{
	return fIsHTTPOnly;
}

BString BNetworkCookie::AsRawValue() const
{
	// TODO: Recompose the raw value, once parsing into individual fields
	// is implemented.
	return fRawValue;
}

// #pragma mark - private

void BNetworkCookie::_ParseRawValue(const BString& rawValue)
{
	// TODO: Parse into individual fields. Most important is the
	// expiration date, but also stuff like Domain, for security reasons.
	fRawValue = rawValue;
}
