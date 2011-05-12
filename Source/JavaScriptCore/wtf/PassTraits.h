/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WTF_PassTraits_h
#define WTF_PassTraits_h

#include "OwnPtr.h"
#include "RefPtr.h"

// The PassTraits template exists to help optimize (or make possible) use
// of WTF data structures with WTF smart pointers that have a Pass
// variant for transfer of ownership

namespace WTF {

template<typename T> struct PassTraits {
    typedef T Type;
    typedef T PassType;
    static PassType transfer(Type& value) { return value; }
};

template<typename T> struct PassTraits<OwnPtr<T> > {
    typedef OwnPtr<T> Type;
    typedef PassOwnPtr<T> PassType;
    static PassType transfer(Type& value) { return value.release(); }
};

template<typename T> struct PassTraits<RefPtr<T> > {
    typedef RefPtr<T> Type;
    typedef PassRefPtr<T> PassType;
    static PassType transfer(Type& value) { return value.release(); }
};

} // namespace WTF

using WTF::PassTraits;

#endif // WTF_PassTraits_h
