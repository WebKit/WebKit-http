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
#ifndef CREDENTIAL_STORAGE_H
#define CREDENTIAL_STORAGE_H

#include "HashKeys.h"
#include "HashMap.h"
#include <Locker.h>
#include <String.h>


class BFile;
class BMessage;
class BString;


class Credentials {
public:
								Credentials();
								Credentials(const BString& username,
									const BString& password);
								Credentials(
									const Credentials& other);
								Credentials(const BMessage* archive);
								~Credentials();

			status_t			Archive(BMessage* archive) const;

			Credentials&		operator=(const Credentials& other);

			bool				operator==(const Credentials& other) const;
			bool				operator!=(const Credentials& other) const;

			const BString&		Username() const;
			const BString&		Password() const;

private:
			BString				fUsername;
			BString				fPassword;
};


class CredentialsStorage : public BLocker {
public:
	static	CredentialsStorage*	SessionInstance();
	static	CredentialsStorage*	PersistentInstance();

			bool				Contains(const HashKeyString& key);
			status_t			PutCredentials(const HashKeyString& key,
									const Credentials& credentials);
			Credentials			GetCredentials(const HashKeyString& key);

private:
								CredentialsStorage(bool persistent);
	virtual						~CredentialsStorage();

			void				_LoadSettings();
			void				_SaveSettings() const;
			bool				_OpenSettingsFile(BFile& file,
									uint32 mode) const;

private:
			typedef HashMap<HashKeyString, Credentials> CredentialMap;
			CredentialMap		fCredentialMap;

	static	CredentialsStorage	sPersistentInstance;
	static	CredentialsStorage	sSessionInstance;
			bool				fSettingsLoaded;
			bool				fPersistent;
};


#endif // CREDENTIAL_STORAGE_H

