/*
 *  Copyright (C) 2006, 2007, 2008, 2009, 2010 Apple Inc. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#ifndef WTF_OwnPtr_h
#define WTF_OwnPtr_h

#include <wtf/Assertions.h>
#include <wtf/Noncopyable.h>
#include <wtf/NullPtr.h>
#include <wtf/OwnPtrCommon.h>
#include <wtf/TypeTraits.h>
#include <algorithm>
#include <memory>

namespace WTF {

    // Unlike most of our smart pointers, OwnPtr can take either the pointer type or the pointed-to type.

    template<typename T> class PassOwnPtr;
    template<typename T> PassOwnPtr<T> adoptPtr(T*);

    template<typename T> class OwnPtr {
#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        // If rvalue references are not supported, the copy constructor is
        // public so OwnPtr cannot be marked noncopyable. See note below.
        WTF_MAKE_NONCOPYABLE(OwnPtr);
#endif
    public:
        typedef typename RemovePointer<T>::Type ValueType;
        typedef ValueType* PtrType;

        OwnPtr() : m_ptr(0) { }
        OwnPtr(std::nullptr_t) : m_ptr(0) { }

        // See comment in PassOwnPtr.h for why this takes a const reference.
        template<typename U> OwnPtr(const PassOwnPtr<U>& o);

#if !COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        // This copy constructor is used implicitly by gcc when it generates
        // transients for assigning a PassOwnPtr<T> object to a stack-allocated
        // OwnPtr<T> object. It should never be called explicitly and gcc
        // should optimize away the constructor when generating code.
        OwnPtr(const OwnPtr<ValueType>&);
#endif

        ~OwnPtr() { deleteOwnedPtr(m_ptr); }

        PtrType get() const { return m_ptr; }

        void clear();
        PassOwnPtr<T> release();
        PtrType leakPtr() WARN_UNUSED_RETURN;

        ValueType& operator*() const { ASSERT(m_ptr); return *m_ptr; }
        PtrType operator->() const { ASSERT(m_ptr); return m_ptr; }

        bool operator!() const { return !m_ptr; }

        // This conversion operator allows implicit conversion to bool but not to other integer types.
        typedef PtrType OwnPtr::*UnspecifiedBoolType;
        operator UnspecifiedBoolType() const { return m_ptr ? &OwnPtr::m_ptr : 0; }

        OwnPtr& operator=(const PassOwnPtr<T>&);
        OwnPtr& operator=(std::nullptr_t) { clear(); return *this; }
        template<typename U> OwnPtr& operator=(const PassOwnPtr<U>&);

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        OwnPtr(OwnPtr&&);
        template<typename U> OwnPtr(OwnPtr<U>&&);

        OwnPtr& operator=(OwnPtr&&);
        template<typename U> OwnPtr& operator=(OwnPtr<U>&&);
#endif

        void swap(OwnPtr& o) { std::swap(m_ptr, o.m_ptr); }

    private:
#if !COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
        // If rvalue references are supported, noncopyable takes care of this.
        OwnPtr& operator=(const OwnPtr&);
#endif

        // We should never have two OwnPtrs for the same underlying object (otherwise we'll get
        // double-destruction), so these equality operators should never be needed.
        template<typename U> bool operator==(const OwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator!=(const OwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator==(const PassOwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator!=(const PassOwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }

        PtrType m_ptr;
    };

    template<typename T> template<typename U> inline OwnPtr<T>::OwnPtr(const PassOwnPtr<U>& o)
        : m_ptr(o.leakPtr())
    {
    }

    template<typename T> inline void OwnPtr<T>::clear()
    {
        PtrType ptr = m_ptr;
        m_ptr = 0;
        deleteOwnedPtr(ptr);
    }

    template<typename T> inline PassOwnPtr<T> OwnPtr<T>::release()
    {
        PtrType ptr = m_ptr;
        m_ptr = 0;
        return adoptPtr(ptr);
    }

    template<typename T> inline typename OwnPtr<T>::PtrType OwnPtr<T>::leakPtr()
    {
        PtrType ptr = m_ptr;
        m_ptr = 0;
        return ptr;
    }

    template<typename T> inline OwnPtr<T>& OwnPtr<T>::operator=(const PassOwnPtr<T>& o)
    {
        PtrType ptr = m_ptr;
        m_ptr = o.leakPtr();
        ASSERT(!ptr || m_ptr != ptr);
        deleteOwnedPtr(ptr);
        return *this;
    }

    template<typename T> template<typename U> inline OwnPtr<T>& OwnPtr<T>::operator=(const PassOwnPtr<U>& o)
    {
        PtrType ptr = m_ptr;
        m_ptr = o.leakPtr();
        ASSERT(!ptr || m_ptr != ptr);
        deleteOwnedPtr(ptr);
        return *this;
    }

#if COMPILER_SUPPORTS(CXX_RVALUE_REFERENCES)
    template<typename T> inline OwnPtr<T>::OwnPtr(OwnPtr<T>&& o)
        : m_ptr(o.leakPtr())
    {
    }

    template<typename T> template<typename U> inline OwnPtr<T>::OwnPtr(OwnPtr<U>&& o)
        : m_ptr(o.leakPtr())
    {
    }

    template<typename T> inline OwnPtr<T>& OwnPtr<T>::operator=(OwnPtr<T>&& o)
    {
        PtrType ptr = m_ptr;
        m_ptr = o.leakPtr();
        ASSERT(!ptr || m_ptr != ptr);
        deleteOwnedPtr(ptr);

        return *this;
    }

    template<typename T> template<typename U> inline OwnPtr<T>& OwnPtr<T>::operator=(OwnPtr<U>&& o)
    {
        PtrType ptr = m_ptr;
        m_ptr = o.leakPtr();
        ASSERT(!ptr || m_ptr != ptr);
        deleteOwnedPtr(ptr);

        return *this;
    }
#endif

    template<typename T> inline void swap(OwnPtr<T>& a, OwnPtr<T>& b)
    {
        a.swap(b);
    }

    template<typename T, typename U> inline bool operator==(const OwnPtr<T>& a, U* b)
    {
        return a.get() == b; 
    }

    template<typename T, typename U> inline bool operator==(T* a, const OwnPtr<U>& b) 
    {
        return a == b.get(); 
    }

    template<typename T, typename U> inline bool operator!=(const OwnPtr<T>& a, U* b)
    {
        return a.get() != b; 
    }

    template<typename T, typename U> inline bool operator!=(T* a, const OwnPtr<U>& b)
    {
        return a != b.get(); 
    }

    template<typename T> inline typename OwnPtr<T>::PtrType getPtr(const OwnPtr<T>& p)
    {
        return p.get();
    }

} // namespace WTF

using WTF::OwnPtr;

#endif // WTF_OwnPtr_h
