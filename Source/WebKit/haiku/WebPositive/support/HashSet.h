// HashSet.h
//
// Copyright (c) 2004, Ingo Weinhold (bonefish@cs.tu-berlin.de)
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

#ifndef HASH_SET_H
#define HASH_SET_H

#include <Locker.h>

#include "AutoLocker.h"
#include "OpenHashTable.h"


namespace BPrivate {

// HashSetElement
template<typename Key>
class HashSetElement : public OpenHashElement {
private:
	typedef HashSetElement<Key> Element;
public:

	HashSetElement() : OpenHashElement(), fKey()
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
	}

	Key		fKey;
};

// HashSet
template<typename Key>
class HashSet {
public:
	class Iterator {
	private:
		typedef HashSetElement<Key>	Element;
	public:
		Iterator(const Iterator& other)
			: fSet(other.fSet),
			  fIndex(other.fIndex),
			  fElement(other.fElement),
			  fLastElement(other.fElement)
		{
		}

		bool HasNext() const
		{
			return fElement;
		}

		Key Next()
		{
			if (!fElement)
				return Key();
			Key result(fElement->fKey);
			_FindNext();
			return result;
		}

		bool Remove()
		{
			if (!fLastElement)
				return false;
			fSet->fTable.Remove(fLastElement);
			fLastElement = NULL;
			return true;
		}

		Iterator& operator=(const Iterator& other)
		{
			fSet = other.fSet;
			fIndex = other.fIndex;
			fElement = other.fElement;
			fLastElement = other.fLastElement;
			return *this;
		}

	private:
		Iterator(HashSet<Key>* map)
			: fSet(map),
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
				fElement = fSet->fTable.ElementAt(fElement->fNext);
				return;
			}
			fElement = NULL;
			int32 arraySize = fSet->fTable.ArraySize();
			for (; !fElement && fIndex < arraySize; fIndex++)
				fElement = fSet->fTable.FindFirst(fIndex);
		}

	private:
		friend class HashSet<Key>;

		HashSet<Key>*	fSet;
		int32			fIndex;
		Element*		fElement;
		Element*		fLastElement;
	};

	HashSet();
	~HashSet();

	status_t InitCheck() const;

	status_t Add(const Key& key);
	bool Remove(const Key& key);
	void Clear();
	bool Contains(const Key& key) const;

	int32 Size() const;
	bool IsEmpty() const	{ return Size() == 0; }

	Iterator GetIterator();

protected:
	typedef HashSetElement<Key>	Element;
	friend class Iterator;

private:
	Element *_FindElement(const Key& key) const;

protected:
	OpenHashElementArray<Element>							fElementArray;
	OpenHashTable<Element, OpenHashElementArray<Element> >	fTable;
};

// SynchronizedHashSet
template<typename Key>
class SynchronizedHashSet : public BLocker {
public:
	typedef struct HashSet<Key>::Iterator Iterator;

	SynchronizedHashSet() : BLocker("synchronized hash set")	{}
	~SynchronizedHashSet()	{ Lock(); }

	status_t InitCheck() const
	{
		return fSet.InitCheck();
	}

	status_t Add(const Key& key)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return B_ERROR;
		return fSet.Add(key);
	}

	bool Remove(const Key& key)
	{
		MapLocker locker(this);
		if (!locker.IsLocked())
			return false;
		return fSet.Remove(key);
	}

	bool Contains(const Key& key) const
	{
		const BLocker* lock = this;
		MapLocker locker(const_cast<BLocker*>(lock));
		if (!locker.IsLocked())
			return false;
		return fSet.Contains(key);
	}

	int32 Size() const
	{
		const BLocker* lock = this;
		MapLocker locker(const_cast<BLocker*>(lock));
		return fSet.Size();
	}

	Iterator GetIterator()
	{
		return fSet.GetIterator();
	}

	// for debugging only
	const HashSet<Key>& GetUnsynchronizedSet() const	{ return fSet; }
	HashSet<Key>& GetUnsynchronizedSet()				{ return fSet; }

protected:
	typedef AutoLocker<BLocker> MapLocker;

	HashSet<Key>	fSet;
};

// HashSet

// constructor
template<typename Key>
HashSet<Key>::HashSet()
	: fElementArray(1000),
	  fTable(1000, &fElementArray)
{
}

// destructor
template<typename Key>
HashSet<Key>::~HashSet()
{
}

// InitCheck
template<typename Key>
status_t
HashSet<Key>::InitCheck() const
{
	return (fTable.InitCheck() && fElementArray.InitCheck()
			? B_OK : B_NO_MEMORY);
}

// Add
template<typename Key>
status_t
HashSet<Key>::Add(const Key& key)
{
	if (Contains(key))
		return B_OK;
	Element* element = fTable.Add(key.GetHashCode());
	if (!element)
		return B_NO_MEMORY;
	element->fKey = key;
	return B_OK;
}

// Remove
template<typename Key>
bool
HashSet<Key>::Remove(const Key& key)
{
	if (Element* element = _FindElement(key)) {
		fTable.Remove(element);
		return true;
	}
	return false;
}

// Clear
template<typename Key>
void
HashSet<Key>::Clear()
{
	fTable.RemoveAll();
}

// Contains
template<typename Key>
bool
HashSet<Key>::Contains(const Key& key) const
{
	return _FindElement(key);
}

// Size
template<typename Key>
int32
HashSet<Key>::Size() const
{
	return fTable.CountElements();
}

// GetIterator
template<typename Key>
struct HashSet<Key>::Iterator
HashSet<Key>::GetIterator()
{
	return Iterator(this);
}

// _FindElement
template<typename Key>
struct HashSet<Key>::Element *
HashSet<Key>::_FindElement(const Key& key) const
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

using BPrivate::HashSet;
using BPrivate::SynchronizedHashSet;

#endif	// HASH_SET_H
