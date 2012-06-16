// HashMap.h
//
// Copyright (c) 2004-2007, Ingo Weinhold (bonefish@cs.tu-berlin.de)
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of a copyright holder shall
// not be used in advertising or otherwise to promote the sale, use or other
// dealings in this Software without prior written authorization of the
// copyright holder.

#ifndef HASH_MAP_H
#define HASH_MAP_H


#include <Locker.h>

#include "AutoLocker.h"
#include "OpenHashTable.h"


namespace BPrivate {

// HashMapElement
template<typename Key, typename Value>
class HashMapElement : public OpenHashElement {
private:
	typedef HashMapElement<Key, Value> Element;
public:

	HashMapElement() : OpenHashElement(), fKey(), fValue()
	{
		fNext = -1;
	}

	inline uint32 Hash() const
	{
		return fKey.GetHashCode();
	}

	inline bool operator==(const OpenHashElement &_element) const
	{
		const Element &element = static_cast<const Element&>(_element);
		return (fKey == element.fKey);
	}

	inline void Adopt(Element &element)
	{
		fKey = element.fKey;
		fValue = element.fValue;
	}

	Key		fKey;
	Value	fValue;
};

// HashMap
template<typename Key, typename Value>
class HashMap {
public:
	class Entry {
	public:
		Entry() {}
		Entry(const Key& key, Value value) : key(key), value(value) {}

		Key		key;
		Value	value;
	};

	class Iterator {
	private:
		typedef HashMapElement<Key, Value>	Element;
	public:
		Iterator(const Iterator& other)
			:
			fMap(other.fMap),
			fIndex(other.fIndex),
			fElement(other.fElement),
			fLastElement(other.fElement)
		{
		}

		bool HasNext() const
		{
			return fElement;
		}

		Entry Next()
		{
			if (!fElement)
				return Entry();
			Entry result(fElement->fKey, fElement->fValue);
			_FindNext();
			return result;
		}

		Value* NextValue()
		{
			if (fElement == NULL)
				return NULL;

			Value* value = &fElement->fValue;
			_FindNext();
			return value;
		}

		Entry Remove()
		{
			if (!fLastElement)
				return Entry();
			Entry result(fLastElement->fKey, fLastElement->fValue);
			fMap->fTable.Remove(fLastElement, true);
			fLastElement = NULL;
			return result;
		}

		Iterator& operator=(const Iterator& other)
		{
			fMap = other.fMap;
			fIndex = other.fIndex;
			fElement = other.fElement;
			fLastElement = other.fLastElement;
			return *this;
		}

	private:
		Iterator(const HashMap<Key, Value>* map)
			:
			fMap(const_cast<HashMap<Key, Value>*>(map)),
			fIndex(0),
			fElement(NULL),
			fLastElement(NULL)
		{
			// find first
			_FindNext();
		}

		void _FindNext()
		{
			fLastElement = fElement;
			if (fElement && fElement->fNext >= 0) {
				fElement = fMap->fTable.ElementAt(fElement->fNext);
				return;
			}
			fElement = NULL;
			int32 arraySize = fMap->fTable.ArraySize();
			for (; !fElement && fIndex < arraySize; fIndex++)
				fElement = fMap->fTable.FindFirst(fIndex);
		}

	private:
		friend class HashMap<Key, Value>;

		HashMap<Key, Value>*	fMap;
		int32					fIndex;
		Element*				fElement;
		Element*				fLastElement;
	};

	HashMap();
	~HashMap();

	status_t InitCheck() const;

	status_t Put(const Key& key, Value value);
	Value Remove(const Key& key);
	void Clear();
	Value Get(const Key& key) const;
	bool Get(const Key& key, Value*& _value) const;

	bool ContainsKey(const Key& key) const;

	int32 Size() const;

	Iterator GetIterator() const;

protected:
	typedef HashMapElement<Key, Value>	Element;
	friend class Iterator;

private:
	Element *_FindElement(const Key& key) const;

protected:
	OpenHashElementArray<Element>							fElementArray;
	OpenHashTable<Element, OpenHashElementArray<Element> >	fTable;
};

// SynchronizedHashMap
template<typename Key, typename Value>
class SynchronizedHashMap : public BLocker {
public:
	typedef struct HashMap<Key, Value>::Entry Entry;
	typedef struct HashMap<Key, Value>::Iterator Iterator;

	SynchronizedHashMap() : BLocker("synchronized hash map")	{}
	~SynchronizedHashMap()	{ Lock(); }

	status_t InitCheck() const
	{
		return fMap.InitCheck();
	}

	status_t Put(const Key& key, Value value)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return B_ERROR;
		return fMap.Put(key, value);
	}

	Value Remove(const Key& key)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return Value();
		return fMap.Remove(key);
	}

	void Clear()
	{
		MapLocker locker(this);
		return fMap.Clear();
	}

	Value Get(const Key& key) const
	{
		const BLocker* lock = this;
		MapLocker locker(const_cast<BLocker*>(lock));
		if (!locker.IsLocked())
			return Value();
		return fMap.Get(key);
	}

	bool ContainsKey(const Key& key) const
	{
		const BLocker* lock = this;
		MapLocker locker(const_cast<BLocker*>(lock));
		if (!locker.IsLocked())
			return false;
		return fMap.ContainsKey(key);
	}

	int32 Size() const
	{
		const BLocker* lock = this;
		MapLocker locker(const_cast<BLocker*>(lock));
		return fMap.Size();
	}

	Iterator GetIterator()
	{
		return fMap.GetIterator();
	}

	// for debugging only
	const HashMap<Key, Value>& GetUnsynchronizedMap() const	{ return fMap; }
	HashMap<Key, Value>& GetUnsynchronizedMap()				{ return fMap; }

protected:
	typedef AutoLocker<BLocker> MapLocker;

	HashMap<Key, Value>	fMap;
};

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


// HashMap

// constructor
template<typename Key, typename Value>
HashMap<Key, Value>::HashMap()
	:
	fElementArray(1000),
	fTable(1000, &fElementArray)
{
}

// destructor
template<typename Key, typename Value>
HashMap<Key, Value>::~HashMap()
{
}

// InitCheck
template<typename Key, typename Value>
status_t
HashMap<Key, Value>::InitCheck() const
{
	return (fTable.InitCheck() && fElementArray.InitCheck()
			? B_OK : B_NO_MEMORY);
}

// Put
template<typename Key, typename Value>
status_t
HashMap<Key, Value>::Put(const Key& key, Value value)
{
	Element* element = _FindElement(key);
	if (element) {
		// already contains the key: just set the new value
		element->fValue = value;
		return B_OK;
	}
	// does not contain the key yet: add an element
	element = fTable.Add(key.GetHashCode());
	if (!element)
		return B_NO_MEMORY;
	element->fKey = key;
	element->fValue = value;
	return B_OK;
}

// Remove
template<typename Key, typename Value>
Value
HashMap<Key, Value>::Remove(const Key& key)
{
	Value value = Value();
	if (Element* element = _FindElement(key)) {
		value = element->fValue;
		fTable.Remove(element);
	}
	return value;
}

// Clear
template<typename Key, typename Value>
void
HashMap<Key, Value>::Clear()
{
	fTable.RemoveAll();
}

// Get
template<typename Key, typename Value>
Value
HashMap<Key, Value>::Get(const Key& key) const
{
	if (Element* element = _FindElement(key))
		return element->fValue;
	return Value();
}

// Get
template<typename Key, typename Value>
bool
HashMap<Key, Value>::Get(const Key& key, Value*& _value) const
{
	if (Element* element = _FindElement(key)) {
		_value = &element->fValue;
		return true;
	}

	return false;
}

// ContainsKey
template<typename Key, typename Value>
bool
HashMap<Key, Value>::ContainsKey(const Key& key) const
{
	return _FindElement(key);
}

// Size
template<typename Key, typename Value>
int32
HashMap<Key, Value>::Size() const
{
	return fTable.CountElements();
}

// GetIterator
template<typename Key, typename Value>
struct HashMap<Key, Value>::Iterator
HashMap<Key, Value>::GetIterator() const
{
	return Iterator(this);
}

// _FindElement
template<typename Key, typename Value>
struct HashMap<Key, Value>::Element *
HashMap<Key, Value>::_FindElement(const Key& key) const
{
	Element* element = fTable.FindFirst(key.GetHashCode());
	while (element && element->fKey != key) {
		if (element->fNext >= 0)
			element = fTable.ElementAt(element->fNext);
		else
			element = NULL;
	}
	return element;
}

}	// namespace BPrivate

using BPrivate::HashMap;
using BPrivate::HashKey32;
using BPrivate::HashKey64;
using BPrivate::SynchronizedHashMap;

#endif	// HASH_MAP_H
