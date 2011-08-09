/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#ifndef Internals_h
#define Internals_h

#include "ExceptionCode.h"
#include "PlatformString.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class ClientRect;
class Document;
class Element;
class Node;

class Internals : public RefCounted<Internals> {
public:
    static PassRefPtr<Internals> create();
    virtual ~Internals();
    
    String elementRenderTreeAsText(Element*, ExceptionCode&);

    bool isPreloaded(Document*, const String& url);

    Node* ensureShadowRoot(Element* host, ExceptionCode&);
    Node* shadowRoot(Element* host, ExceptionCode&);
    void removeShadowRoot(Element* host, ExceptionCode&);
    Element* includerFor(Node*, ExceptionCode&);
    String shadowPseudoId(Element*, ExceptionCode&);
    PassRefPtr<Element> createShadowContentElement(Document*, ExceptionCode&);
    Element* getElementByIdInShadowRoot(Node* shadowRoot, const String& id, ExceptionCode&);
    void disableMemoryCache(bool disabled);

#if ENABLE(INSPECTOR)
    void setInspectorResourcesDataSizeLimits(Document*, int maximumResourcesContentSize, int maximumSingleResourceContentSize, ExceptionCode&);
#else
    void setInspectorResourcesDataSizeLimits(Document*, int maximumResourcesContentSize, int maximumSingleResourceContentSize, ExceptionCode&) { }
#endif

    PassRefPtr<ClientRect> boundingBox(Element*, ExceptionCode&);

    void setForceCompositingMode(Document*, bool enabled, ExceptionCode&);

private:
    Internals();
};

} // namespace WebCore

#endif
