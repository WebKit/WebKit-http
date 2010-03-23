/*
 * Copyright 2008-2010, Stephan Aßmus <superstippi@gmx.de>.
 * Copyright 1998, Eric Shepherd.
 * All rights reserved. Distributed under the terms of the Be Sample Code
 * license.
 */

//! Be Newsletter Volume II, Issue 35; September 2, 1998 (Eric Shepherd)

#include "SettingsMessage.h"

#include <new>

#include <Autolock.h>
#include <Entry.h>
#include <File.h>
#include <Messenger.h>
#include <String.h>


SettingsMessage::SettingsMessage(directory_which directory,
		const char* filename)
	:
	BMessage('pref'),
	fListeners(4)
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

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--)
		delete reinterpret_cast<BMessenger*>(fListeners.ItemAtFast(i));
}


status_t
SettingsMessage::InitCheck() const
{
	return fStatus;
}


status_t
SettingsMessage::Load()
{
	BAutolock _(this);

	BFile file(fPath.Path(), B_READ_ONLY);
	status_t status = file.InitCheck();

	if (status == B_OK)
		status = Unflatten(&file);

	return status;
}


status_t
SettingsMessage::Save() const
{
	BAutolock _(const_cast<SettingsMessage*>(this));

	BFile file(fPath.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	status_t status = file.InitCheck();

	if (status == B_OK)
		status = Flatten(&file);

	return status;
}


bool
SettingsMessage::AddListener(const BMessenger& listener)
{
	BAutolock _(this);

	BMessenger* listenerCopy = new(std::nothrow) BMessenger(listener);
	if (listenerCopy && fListeners.AddItem(listenerCopy))
		return true;
	delete listenerCopy;
	return false;
}


void
SettingsMessage::RemoveListener(const BMessenger& listener)
{
	BAutolock _(this);

	for (int32 i = fListeners.CountItems() - 1; i >= 0; i--) {
		BMessenger* listenerItem = reinterpret_cast<BMessenger*>(
			fListeners.ItemAtFast(i));
		if (*listenerItem == listener) {
			fListeners.RemoveItem(i);
			delete listenerItem;
			return;
		}
	}
}


// #pragma mark -


status_t
SettingsMessage::SetValue(const char* name, bool value)
{
	status_t ret = ReplaceBool(name, value);
	if (ret != B_OK)
		ret = AddBool(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, int8 value)
{
	status_t ret = ReplaceInt8(name, value);
	if (ret != B_OK)
		ret = AddInt8(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, int16 value)
{
	status_t ret = ReplaceInt16(name, value);
	if (ret != B_OK)
		ret = AddInt16(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, int32 value)
{
	status_t ret = ReplaceInt32(name, value);
	if (ret != B_OK)
		ret = AddInt32(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, uint32 value)
{
	status_t ret = ReplaceUInt32(name, value);
	if (ret != B_OK)
		ret = AddUInt32(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, int64 value)
{
	status_t ret = ReplaceInt64(name, value);
	if (ret != B_OK)
		ret = AddInt64(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, float value)
{
	status_t ret = ReplaceFloat(name, value);
	if (ret != B_OK)
		ret = AddFloat(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, double value)
{
	status_t ret = ReplaceDouble(name, value);
	if (ret != B_OK)
		ret = AddDouble(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const char* value)
{
	status_t ret = ReplaceString(name, value);
	if (ret != B_OK)
		ret = AddString(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const BString& value)
{
	status_t ret = ReplaceString(name, value);
	if (ret != B_OK)
		ret = AddString(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const BPoint& value)
{
	status_t ret = ReplacePoint(name, value);
	if (ret != B_OK)
		ret = AddPoint(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const BRect& value)
{
	status_t ret = ReplaceRect(name, value);
	if (ret != B_OK)
		ret = AddRect(name, value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const entry_ref& value)
{
	status_t ret = ReplaceRef(name, &value);
	if (ret != B_OK)
		ret = AddRef(name, &value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const BMessage& value)
{
	status_t ret = ReplaceMessage(name, &value);
	if (ret != B_OK)
		ret = AddMessage(name, &value);
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
}


status_t
SettingsMessage::SetValue(const char* name, const BFlattenable* value)
{
	status_t ret = ReplaceFlat(name, const_cast<BFlattenable*>(value));
	if (ret != B_OK)
		ret = AddFlat(name, const_cast<BFlattenable*>(value));
	if (ret == B_OK)
		_NotifyValueChanged(name);
	return ret;
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
	if (ret == B_OK)
		_NotifyValueChanged(name);
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


const char*
SettingsMessage::GetValue(const char* name, const char* defaultValue) const
{
	const char* value;
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


// #pragma mark - private


void
SettingsMessage::_NotifyValueChanged(const char* name) const
{
	BMessage message(SETTINGS_VALUE_CHANGED);
	message.AddString("name", name);

	// Add the value of that name to the notification.
	type_code type;
	if (GetInfo(name, &type) == B_OK) {
		const void* data;
		ssize_t numBytes;
		if (FindData(name, type, &data, &numBytes) == B_OK)
			message.AddData("value", type, data, numBytes);
	}

	int32 count = fListeners.CountItems();
	for (int32 i = 0; i < count; i++) {
		BMessenger* listener = reinterpret_cast<BMessenger*>(
			fListeners.ItemAtFast(i));
		listener->SendMessage(&message);
	}
}

