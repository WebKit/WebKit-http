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
#ifndef _NETWORK_COOKIE_H_
#define _NETWORK_COOKIE_H_

#include <DateTime.h>
#include <String.h>

class BMessage;


class BNetworkCookie {
public:
								BNetworkCookie(const BString& rawValue);
								BNetworkCookie(const BMessage& archive);
								BNetworkCookie(const BNetworkCookie& other);
	virtual						~BNetworkCookie();

	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;

			BNetworkCookie&		operator=(const BNetworkCookie& other);
			bool				operator==(const BNetworkCookie& other) const;
			bool				operator!=(const BNetworkCookie& other) const;

			void				SetExpirationDate(const BDateTime& dateTime);
			const BDateTime&	ExpirationDate() const;

			void				SetDomain(const BString& domain);
			const BString&		Domain() const;

			void				SetPath(const BString& path);
			const BString&		Path() const;

			void				SetComment(const BString& comment);
			const BString&		Comment() const;

			void				SetName(const BString& name);
			const BString&		Name() const;

			void				SetValue(const BString& value);
			const BString&		Value() const;

			void				SetIsSecure(bool isSecure);
			bool				IsSecure() const;

			void				SetIsHTTPOnly(bool isHTTPOnly);
			bool				IsHTTPOnly() const;

			BString				AsRawValue() const;

private:
			void				_ParseRawValue(const BString& rawValue);

private:
			BDateTime			fExpirationDate;
			BString				fDomain;
			BString				fPath;
			BString				fComment;
			BString				fName;
			BString				fValue;
			bool				fIsSecure;
			bool				fIsHTTPOnly;

			BString				fRawValue;
};


#endif // _NETWORK_COOKIE_H_
