/*
 * Copyright (C) 2012 Google, Inc. All Rights Reserved.
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

#ifndef RefCountedSupplement_h
#define RefCountedSupplement_h

#include "Supplementable.h"
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>

namespace WebCore {

template<class T, class S>
class RefCountedSupplement : public RefCounted<S> {
public:
    typedef RefCountedSupplement<T, S> ThisType;

    virtual ~RefCountedSupplement() { }
    virtual void hostDestroyed() { }

    class Wrapper : public Supplement<T> {
    public:
        explicit Wrapper(PassRefPtr<ThisType> wrapped) : m_wrapped(wrapped) { }
        virtual ~Wrapper() { m_wrapped->hostDestroyed();  }
#if !ASSERT_DISABLED
        virtual bool isRefCountedWrapper() const OVERRIDE { return true; }
#endif
        ThisType* wrapped() const { return m_wrapped.get(); }
    private:

        RefPtr<ThisType> m_wrapped;
    };

    static void provideTo(Supplementable<T>* host, const AtomicString& key, PassRefPtr<ThisType> supplement)
    {
        host->provideSupplement(key, adoptPtr(new Wrapper(supplement)));
    }

    static ThisType* from(Supplementable<T>* host, const AtomicString& key)
    {
        Supplement<T>* found = host->requireSupplement(key);
        if (!found)
            return 0;
        ASSERT(found->isRefCountedWrapper());
        return static_cast<Wrapper*>(found)->wrapped();
    }
};

} // namespace WebCore

#endif // RefCountedSupplement_h
