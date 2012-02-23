/*
 * Copyright (C) 2007, 2008 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#ifndef FontSelector_h
#define FontSelector_h

#include <wtf/Forward.h>
#include <wtf/RefCounted.h>

namespace WebCore {

class FontData;
class FontDescription;
class FontSelectorClient;

class FontSelector : public RefCounted<FontSelector> {
public:
    virtual ~FontSelector() { }
    virtual FontData* getFontData(const FontDescription&, const AtomicString& familyName) = 0;

    virtual void fontCacheInvalidated() { }

    virtual void registerForInvalidationCallbacks(FontSelectorClient*) = 0;
    virtual void unregisterForInvalidationCallbacks(FontSelectorClient*) = 0;
    
    virtual unsigned version() const = 0;
};

class FontSelectorClient {
public:
    virtual ~FontSelectorClient() { }

    virtual void fontsNeedUpdate(FontSelector*) = 0;
};

} // namespace WebCore

#endif // FontSelector_h
