/*
 * Copyright 2008 Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998 Eric Shepherd.
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */
#ifndef SETTINGS_MESSAGE_H
#define SETTINGS_MESSAGE_H

#include <FindDirectory.h>
#include <Font.h>
#include <Message.h>
#include <Path.h>

class BString;

class SettingsMessage : public BMessage {
public:
								SettingsMessage(directory_which directory,
									const char* filename);
	virtual  					~SettingsMessage();
		

			status_t			InitCheck() const;
			status_t			Load();
			status_t			Save() const;

			status_t			SetValue(const char* name, bool value);
			status_t			SetValue(const char* name, int8 value);
			status_t			SetValue(const char* name, int16 value);
			status_t			SetValue(const char* name, int32 value);
			status_t			SetValue(const char* name, uint32 value);
			status_t			SetValue(const char* name, int64 value);
			status_t			SetValue(const char* name, float value);
			status_t			SetValue(const char* name, double value);
			status_t			SetValue(const char* name,
									const char* value);
			status_t			SetValue(const char* name,
									const BString& value);
			status_t			SetValue(const char *name, const BPoint& value);
			status_t			SetValue(const char* name, const BRect& value);
			status_t			SetValue(const char* name, const entry_ref& value);
			status_t			SetValue(const char* name,
									const BMessage* value);
			status_t			SetValue(const char* name,
									const BFlattenable* value);
			status_t			SetValue(const char* name,
									const BFont& value);

			bool				GetValue(const char* name,
									bool defaultValue) const;
			int8				GetValue(const char* name,
									int8 defaultValue) const;
			int16				GetValue(const char* name,
									int16 defaultValue) const;
			int32				GetValue(const char* name,
									int32 defaultValue) const;
			uint32				GetValue(const char* name,
									uint32 defaultValue) const;
			int64				GetValue(const char* name,
									int64 defaultValue) const;
			float				GetValue(const char* name,
									float defaultValue) const;
			double				GetValue(const char* name,
									double defaultValue) const;
			BString				GetValue(const char* name,
									const BString& defaultValue) const;
			BPoint				GetValue(const char *name,
									BPoint defaultValue) const;
			BRect				GetValue(const char* name,
									BRect defaultValue) const;
			entry_ref			GetValue(const char* name,
									const entry_ref& defaultValue) const;
			BMessage			GetValue(const char* name,
									const BMessage& defaultValue) const;
			BFont				GetValue(const char* name,
									const BFont& defaultValue) const;

private:
			BPath				fPath;
			status_t			fStatus;
};

#endif  // SETTINGS_MESSAGE_H
