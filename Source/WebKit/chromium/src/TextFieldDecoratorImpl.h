/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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

#ifndef TextFieldDecoratorImpl_h
#define TextFieldDecoratorImpl_h

#include "CachedResourceHandle.h"
#include "TextFieldDecorationElement.h"

namespace WebKit {

class WebTextFieldDecoratorClient;

class TextFieldDecoratorImpl : public WebCore::TextFieldDecorator {
public:
    static PassOwnPtr<TextFieldDecoratorImpl> create(WebTextFieldDecoratorClient*);
    virtual ~TextFieldDecoratorImpl();

    WebTextFieldDecoratorClient* decoratorClient();

private:
    virtual bool willAddDecorationTo(WebCore::HTMLInputElement*) OVERRIDE;
    virtual bool visibleByDefault() OVERRIDE;
    virtual WebCore::CachedImage* imageForNormalState() OVERRIDE;
    virtual WebCore::CachedImage* imageForDisabledState() OVERRIDE;
    virtual WebCore::CachedImage* imageForReadonlyState() OVERRIDE;
    virtual WebCore::CachedImage* imageForHoverState() OVERRIDE;
    virtual void handleClick(WebCore::HTMLInputElement*) OVERRIDE;
    virtual void willDetach(WebCore::HTMLInputElement*) OVERRIDE;

    TextFieldDecoratorImpl(WebTextFieldDecoratorClient*);

    WebTextFieldDecoratorClient* m_client;
    WebCore::CachedResourceHandle<WebCore::CachedImage> m_cachedImageForNormalState;
    WebCore::CachedResourceHandle<WebCore::CachedImage> m_cachedImageForDisabledState;
    WebCore::CachedResourceHandle<WebCore::CachedImage> m_cachedImageForReadonlyState;
    WebCore::CachedResourceHandle<WebCore::CachedImage> m_cachedImageForHoverState;
};

}

#endif // TextFieldDecoratorImpl_h
