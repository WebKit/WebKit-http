/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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

#import "config.h"

#if defined(__LP64__) && defined(__clang__)

#import "WKDOMDocument.h"

#import "WKDOMInternals.h"
#import <WebCore/Document.h>
#import <WebCore/HTMLElement.h>
#import <WebCore/Text.h>

static inline WebCore::Document* toDocument(WebCore::Node* node)
{
    ASSERT(!node || node->isDocumentNode());
    return static_cast<WebCore::Document*>(node);
}

@implementation WKDOMDocument

- (WKDOMElement *)createElement:(NSString *)tagName
{
    // FIXME: Do something about the exception.
    WebCore::ExceptionCode ec = 0;
    return WebKit::toWKDOMElement(toDocument(_impl.get())->createElement(tagName, ec).get());
}

- (WKDOMText *)createTextNode:(NSString *)data
{
    return WebKit::toWKDOMText(toDocument(_impl.get())->createTextNode(data).get());
}

- (WKDOMElement *)body
{
    return WebKit::toWKDOMElement(toDocument(_impl.get())->body());
}

@end

#endif // defined(__LP64__) && defined(__clang__)
