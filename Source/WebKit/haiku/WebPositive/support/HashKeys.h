/*
 * Copyright 2010, Stephan Aßmus <superstippi@gmx.de>. All rights reserved.
 * Copyright 2004-2007, Ingo Weinhold <ingo_weinhold@gmx.de>. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef HASH_KEYS_H
#define HASH_KEYS_H

#include <String.h>


namespace BPrivate {

#if 0
// TODO: Move here from HashMap.h and adapt all clients.
// HashKey32
template<typename Value>
struct HashKey32 {
	HashKey32() {}
	HashKey32(const Value& value) : value(value) {}

	uint32 GetHashCode() const
	{
		return (uint32)value;
	}

	HashKey32<Value> operator=(const HashKey32<Value>& other)
	{
		value = other.value;
		return *this;
	}

	bool operator==(const HashKey32<Value>& other) const
	{
		return (value == other.value);
	}

	bool operator!=(const HashKey32<Value>& other) const
	{
		return (value != other.value);
	}

	Value	value;
};


// HashKey64
template<typename Value>
struct HashKey64 {
	HashKey64() {}
	HashKey64(const Value& value) : value(value) {}

	uint32 GetHashCode() const
	{
		uint64 v = (uint64)value;
		return (uint32)(v >> 32) ^ (uint32)v;
	}

	HashKey64<Value> operator=(const HashKey64<Value>& other)
	{
		value = other.value;
		return *this;
	}

	bool operator==(const HashKey64<Value>& other) const
	{
		return (value == other.value);
	}

	bool operator!=(const HashKey64<Value>& other) const
	{
		return (value != other.value);
	}

	Value	value;
};
#endif


struct HashKeyString {
	HashKeyString() {}
	HashKeyString(const BString& value) : value(value) {}
	HashKeyString(const char* string) : value(string) {}

	uint32 GetHashCode() const
	{
		// from the Dragon Book: a slightly modified hashpjw()
		uint32 hash = 0;
		const char* string = value.String();
		if (string != NULL) {
			for (; *string; string++) {
				uint32 g = hash & 0xf0000000;
				if (g != 0)
					hash ^= g >> 24;
				hash = (hash << 4) + *string;
			}
		}
		return hash;
	}

	HashKeyString operator=(const HashKeyString& other)
	{
		value = other.value;
		return *this;
	}

	bool operator==(const HashKeyString& other) const
	{
		return (value == other.value);
	}

	bool operator!=(const HashKeyString& other) const
	{
		return (value != other.value);
	}

	BString	value;
};

}	// namespace BPrivate

using BPrivate::HashKeyString;

#endif	// HASH_KEYS_H
