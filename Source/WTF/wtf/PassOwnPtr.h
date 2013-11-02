/*
 * Copyright (C) 2009, 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef WTF_PassOwnPtr_h
#define WTF_PassOwnPtr_h

#include <cstddef>
#include <wtf/Assertions.h>
#include <wtf/OwnPtrCommon.h>
#include <type_traits>

namespace WTF {

    template<typename T> class OwnPtr;
    template<typename T> class PassOwnPtr;
    template<typename T> PassOwnPtr<T> adoptPtr(T*);

    class RefCountedBase;
    class ThreadSafeRefCountedBase;

    template<typename T> class PassOwnPtr {
    public:
        typedef T ValueType;
        typedef ValueType* PtrType;

        PassOwnPtr() : m_ptr(0) { }
        PassOwnPtr(std::nullptr_t) : m_ptr(0) { }

        // It somewhat breaks the type system to allow transfer of ownership out of
        // a const PassOwnPtr. However, it makes it much easier to work with PassOwnPtr
        // temporaries, and we don't have a need to use real const PassOwnPtrs anyway.
        PassOwnPtr(const PassOwnPtr& o) : m_ptr(o.leakPtr()) { }
        template<typename U> PassOwnPtr(const PassOwnPtr<U>& o) : m_ptr(o.leakPtr()) { }

        ~PassOwnPtr() { deleteOwnedPtr(m_ptr); }

        PtrType get() const { return m_ptr; }

        PtrType leakPtr() const WARN_UNUSED_RETURN;

        ValueType& operator*() const { ASSERT(m_ptr); return *m_ptr; }
        PtrType operator->() const { ASSERT(m_ptr); return m_ptr; }

        bool operator!() const { return !m_ptr; }

        // This conversion operator allows implicit conversion to bool but not to other integer types.
        typedef PtrType PassOwnPtr::*UnspecifiedBoolType;
        operator UnspecifiedBoolType() const { return m_ptr ? &PassOwnPtr::m_ptr : 0; }

        PassOwnPtr& operator=(const PassOwnPtr&) { COMPILE_ASSERT(!sizeof(T*), PassOwnPtr_should_never_be_assigned_to); return *this; }

        template<typename U> friend PassOwnPtr<U> adoptPtr(U*);

    private:
        explicit PassOwnPtr(PtrType ptr) : m_ptr(ptr) { }

        // We should never have two OwnPtrs for the same underlying object (otherwise we'll get
        // double-destruction), so these equality operators should never be needed.
        template<typename U> bool operator==(const PassOwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator!=(const PassOwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator==(const OwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }
        template<typename U> bool operator!=(const OwnPtr<U>&) { COMPILE_ASSERT(!sizeof(U*), OwnPtrs_should_never_be_equal); return false; }

        mutable PtrType m_ptr;
    };

    template<typename T> inline typename PassOwnPtr<T>::PtrType PassOwnPtr<T>::leakPtr() const
    {
        PtrType ptr = m_ptr;
        m_ptr = 0;
        return ptr;
    }

    template<typename T, typename U> inline bool operator==(const PassOwnPtr<T>& a, const PassOwnPtr<U>& b) 
    {
        return a.get() == b.get(); 
    }

    template<typename T, typename U> inline bool operator==(const PassOwnPtr<T>& a, const OwnPtr<U>& b) 
    {
        return a.get() == b.get(); 
    }
    
    template<typename T, typename U> inline bool operator==(const OwnPtr<T>& a, const PassOwnPtr<U>& b) 
    {
        return a.get() == b.get(); 
    }
    
    template<typename T, typename U> inline bool operator==(const PassOwnPtr<T>& a, U* b) 
    {
        return a.get() == b; 
    }
    
    template<typename T, typename U> inline bool operator==(T* a, const PassOwnPtr<U>& b) 
    {
        return a == b.get(); 
    }
    
    template<typename T, typename U> inline bool operator!=(const PassOwnPtr<T>& a, const PassOwnPtr<U>& b) 
    {
        return a.get() != b.get(); 
    }
    
    template<typename T, typename U> inline bool operator!=(const PassOwnPtr<T>& a, const OwnPtr<U>& b) 
    {
        return a.get() != b.get(); 
    }
    
    template<typename T, typename U> inline bool operator!=(const OwnPtr<T>& a, const PassOwnPtr<U>& b) 
    {
        return a.get() != b.get(); 
    }
    
    template<typename T, typename U> inline bool operator!=(const PassOwnPtr<T>& a, U* b)
    {
        return a.get() != b; 
    }
    
    template<typename T, typename U> inline bool operator!=(T* a, const PassOwnPtr<U>& b) 
    {
        return a != b.get(); 
    }

    template<typename T> inline PassOwnPtr<T> adoptPtr(T* ptr)
    {
        static_assert(!std::is_convertible<T*, RefCountedBase*>::value, "Do not use adoptPtr with RefCounted, use adoptRef!");
        static_assert(!std::is_convertible<T*, ThreadSafeRefCountedBase*>::value, "Do not use adoptPtr with ThreadSafeRefCounted, use adoptRef!");

        return PassOwnPtr<T>(ptr);
    }

    template<typename T, typename U> inline PassOwnPtr<T> static_pointer_cast(const PassOwnPtr<U>& p) 
    {
        return adoptPtr(static_cast<T*>(p.leakPtr()));
    }

    template<typename T> inline T* getPtr(const PassOwnPtr<T>& p)
    {
        return p.get();
    }

} // namespace WTF

using WTF::PassOwnPtr;
using WTF::adoptPtr;
using WTF::static_pointer_cast;

#endif // WTF_PassOwnPtr_h
