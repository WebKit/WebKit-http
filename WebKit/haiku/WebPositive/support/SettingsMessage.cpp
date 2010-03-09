/*
 * Copyright 2008-2010, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998, Eric Shepherd.
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

//! Be Newsletter Volume II, Issue 35; September 2, 1998 (Eric Shepherd)

#include "SettingsMessage.h"

#include <Entry.h>
#include <File.h>
#include <String.h>


SettingsMessage::SettingsMessage(directory_which directory,
		const char* filename)
	: BMessage('pref')
{
	fStatus = find_directory(directory, &fPath);

	if (fStatus == B_OK)
		fStatus = fPath.Append(filename);

	if (fStatus == B_OK)
		fStatus = Load();
}


SettingsMessage::~SettingsMessage()
{
	Save();
}


status_t
SettingsMessage::InitCheck() const
{
	return fStatus;
}


status_t
SettingsMessage::Load()
{
	BFile file(fPath.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();

	if (status == B_OK)
		status = Unflatten(&file);

	return status;
}


status_t
SettingsMessage::Save() const
{
	BFile file(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();

	if (status == B_OK)
		status = Flatten(&file);

	return status;
}


// #pragma mark -


status_t
SettingsMessage::SetValue(const char* name, bool value)
{
	if (ReplaceBool(name, value) == B_OK)
		return B_OK;
	return AddBool(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, int8 value)
{
	if (ReplaceInt8(name, value) == B_OK)
		return B_OK;
	return AddInt8(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, int16 value)
{
	if (ReplaceInt16(name, value) == B_OK)
		return B_OK;
	return AddInt16(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, int32 value)
{
	if (ReplaceInt32(name, value) == B_OK)
		return B_OK;
	return AddInt32(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, uint32 value)
{
	if (ReplaceInt32(name, (int32)value) == B_OK)
		return B_OK;
	return AddInt32(name, (int32)value);
}


status_t
SettingsMessage::SetValue(const char* name, int64 value)
{
	if (ReplaceInt64(name, value) == B_OK)
		return B_OK;
	return AddInt64(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, float value)
{
	if (ReplaceFloat(name, value) == B_OK)
		return B_OK;
	return AddFloat(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, double value)
{
	if (ReplaceDouble(name, value) == B_OK)
		return B_OK;
	return AddDouble(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, const char* value)
{
	if (ReplaceString(name, value) == B_OK)
		return B_OK;
	return AddString(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, const BString& value)
{
	return SetValue(name, value.String());
}


status_t
SettingsMessage::SetValue(const char* name, const BPoint& value)
{
	if (ReplacePoint(name, value) == B_OK)
		return B_OK;
	return AddPoint(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, const BRect& value)
{
	if (ReplaceRect(name, value) == B_OK)
		return B_OK;
	return AddRect(name, value);
}


status_t
SettingsMessage::SetValue(const char* name, const entry_ref& value)
{
	if (ReplaceRef(name, &value) == B_OK)
		return B_OK;
	return AddRef(name, &value);
}


status_t
SettingsMessage::SetValue(const char* name, const BMessage& value)
{
	if (ReplaceMessage(name, &value) == B_OK)
		return B_OK;
	return AddMessage(name, &value);
}


status_t
SettingsMessage::SetValue(const char* name, const BFlattenable* value)
{
	if (ReplaceFlat(name, const_cast<BFlattenable*>(value)) == B_OK)
		return B_OK;
	return AddFlat(name, const_cast<BFlattenable*>(value));
}


status_t
SettingsMessage::SetValue(const char* name, const BFont& value)
{
	font_family family;
	font_style style;
	value.GetFamilyAndStyle(&family, &style);

	BMessage fontMessage;
	status_t ret = fontMessage.AddString("family", family);
	if (ret == B_OK)
		ret = fontMessage.AddString("style", style);
	if (ret == B_OK)
		ret = fontMessage.AddFloat("size", value.Size());

	if (ret == B_OK) {
		if (ReplaceMessage(name, &fontMessage) != B_OK)
			ret = AddMessage(name, &fontMessage);
	}
	return ret;
}


// #pragma mark -


bool
SettingsMessage::GetValue(const char* name, bool defaultValue) const
{
	bool value;
	if (FindBool(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int8
SettingsMessage::GetValue(const char* name, int8 defaultValue) const
{
	int8 value;
	if (FindInt8(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int16
SettingsMessage::GetValue(const char* name, int16 defaultValue) const
{
	int16 value;
	if (FindInt16(name, &value) != B_OK)
		return defaultValue;
	return value;
}


int32
SettingsMessage::GetValue(const char* name, int32 defaultValue) const
{
	int32 value;
	if (FindInt32(name, &value) != B_OK)
		return defaultValue;
	return value;
}


uint32
SettingsMessage::GetValue(const char* name, uint32 defaultValue) const
{
	int32 value;
	if (FindInt32(name, &value) != B_OK)
		return defaultValue;
	return (uint32)value;
}


int64
SettingsMessage::GetValue(const char* name, int64 defaultValue) const
{
	int64 value;
	if (FindInt64(name, &value) != B_OK)
		return defaultValue;
	return value;
}


float
SettingsMessage::GetValue(const char* name, float defaultValue) const
{
	float value;
	if (FindFloat(name, &value) != B_OK)
		return defaultValue;
	return value;
}


double
SettingsMessage::GetValue(const char* name, double defaultValue) const
{
	double value;
	if (FindDouble(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BString
SettingsMessage::GetValue(const char* name, const BString& defaultValue) const
{
	BString value;
	if (FindString(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BPoint
SettingsMessage::GetValue(const char *name, BPoint defaultValue) const
{
	BPoint value;
	if (FindPoint(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BRect
SettingsMessage::GetValue(const char* name, BRect defaultValue) const
{
	BRect value;
	if (FindRect(name, &value) != B_OK)
		return defaultValue;
	return value;
}


entry_ref
SettingsMessage::GetValue(const char* name, const entry_ref& defaultValue) const
{
	entry_ref value;
	if (FindRef(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BMessage
SettingsMessage::GetValue(const char* name, const BMessage& defaultValue) const
{
	BMessage value;
	if (FindMessage(name, &value) != B_OK)
		return defaultValue;
	return value;
}


BFont
SettingsMessage::GetValue(const char* name, const BFont& defaultValue) const
{
	BMessage fontMessage;
	if (FindMessage(name, &fontMessage) != B_OK)
		return defaultValue;

	const char* family;
	const char* style;
	float size;
	if (fontMessage.FindString("family", &family) != B_OK
		|| fontMessage.FindString("style", &style) != B_OK
		|| fontMessage.FindFloat("size", &size) != B_OK) {
		return defaultValue;
	}

	BFont value;
	if (value.SetFamilyAndStyle(family, style) != B_OK)
		return defaultValue;

	value.SetSize(size);

	return value;
}

