/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebDocumentType_h
#define WebDocumentType_h

#include "WebNode.h"

#if WEBKIT_IMPLEMENTATION
namespace WebCore { class DocumentType; }
namespace WTF { template <typename T> class PassRefPtr; }
#endif

namespace WebKit {
class WebString;

// Provides readonly access to some properties of a DOM document type.
class WebDocumentType : public WebNode {
public:
    WebDocumentType() { }
    WebDocumentType(const WebDocumentType& e) : WebNode(e) { }

    WebDocumentType& operator=(const WebDocumentType& e)
    {
        WebNode::assign(e);
        return *this;
    }
    void assign(const WebDocumentType& e) { WebNode::assign(e); }

    WEBKIT_EXPORT WebString name() const;

#if WEBKIT_IMPLEMENTATION
    WebDocumentType(const WTF::PassRefPtr<WebCore::DocumentType>&);
    WebDocumentType& operator=(const WTF::PassRefPtr<WebCore::DocumentType>&);
    operator WTF::PassRefPtr<WebCore::DocumentType>() const;
#endif
};

} // namespace WebKit

#endif
