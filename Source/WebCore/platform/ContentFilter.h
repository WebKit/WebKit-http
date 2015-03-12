/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ContentFilter_h
#define ContentFilter_h

#if ENABLE(CONTENT_FILTERING)

#include "ContentFilterUnblockHandler.h"
#include <wtf/Vector.h>

namespace WebCore {

class ResourceResponse;

class ContentFilter {
public:
    template <typename T> static void addType() { types().append(type<T>()); }

    static std::unique_ptr<ContentFilter> createIfNeeded(const ResourceResponse&);

    virtual ~ContentFilter() { }
    virtual void addData(const char* data, int length) = 0;
    virtual void finishedAddingData() = 0;
    virtual bool needsMoreData() const = 0;
    virtual bool didBlockData() const = 0;
    virtual const char* getReplacementData(int& length) const = 0;
    virtual ContentFilterUnblockHandler unblockHandler() const = 0;

private:
    struct Type {
        const std::function<bool(const ResourceResponse&)> canHandleResponse;
        const std::function<std::unique_ptr<ContentFilter>(const ResourceResponse&)> create;
    };
    template <typename T> static Type type() { return { T::canHandleResponse, T::create }; }
    WEBCORE_EXPORT static Vector<Type>& types();
};

} // namespace WebCore

#endif // ENABLE(CONTENT_FILTERING)

#endif // ContentFilter_h
