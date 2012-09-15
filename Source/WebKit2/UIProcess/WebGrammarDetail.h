/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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

#ifndef WebGrammarDetail_h
#define WebGrammarDetail_h

#include "APIObject.h"
#include <WebCore/TextCheckerClient.h>
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>

namespace WebKit {

class ImmutableArray;

class WebGrammarDetail : public APIObject {
public:
    static const Type APIType = TypeGrammarDetail;
    static PassRefPtr<WebGrammarDetail> create(int location, int length, ImmutableArray* guesses, const String& userDescription);
    static PassRefPtr<WebGrammarDetail> create(const WebCore::GrammarDetail&);

    int location() const { return m_grammarDetail.location; }
    int length() const { return m_grammarDetail.length; }
    PassRefPtr<ImmutableArray> guesses() const;
    const String& userDescription() const { return m_grammarDetail.userDescription; }

    const WebCore::GrammarDetail& grammarDetail() { return m_grammarDetail; }

private:
    WebGrammarDetail(int location, int length, ImmutableArray* guesses, const String& userDescription);
    explicit WebGrammarDetail(const WebCore::GrammarDetail&);

    virtual Type type() const { return APIType; }

    WebCore::GrammarDetail m_grammarDetail;
};

} // namespace WebKit

#endif // WebGrammarDetail_h
