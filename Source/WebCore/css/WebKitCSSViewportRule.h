/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer.
 * 2. Redistributions in binary form must reproduce the above
 *    copyright notice, this list of conditions and the following
 *    disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER “AS IS” AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
 * OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
 * TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
 * THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#ifndef WebKitCSSViewportRule_h
#define WebKitCSSViewportRule_h

#if ENABLE(CSS_DEVICE_ADAPTATION)

#include "CSSRule.h"

namespace WebCore {

class CSSStyleDeclaration;
class StyleRuleViewport;
class StyleRuleCSSStyleDeclaration;

class WebKitCSSViewportRule: public CSSRule {
public:
    static PassRefPtr<WebKitCSSViewportRule> create(StyleRuleViewport* viewportRule, CSSStyleSheet* sheet)
    {
        return adoptRef(new WebKitCSSViewportRule(viewportRule, sheet));
    }
    ~WebKitCSSViewportRule();

    virtual CSSRule::Type type() const OVERRIDE { return WEBKIT_VIEWPORT_RULE; }
    virtual String cssText() const OVERRIDE;
    virtual void reattach(StyleRuleBase*) OVERRIDE;
    virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

    CSSStyleDeclaration* style() const;

private:
    WebKitCSSViewportRule(StyleRuleViewport*, CSSStyleSheet*);

    RefPtr<StyleRuleViewport> m_viewportRule;
    mutable RefPtr<StyleRuleCSSStyleDeclaration> m_propertiesCSSOMWrapper;
};

} // namespace WebCore

#endif // WebKitCSSViewportRule_h

#endif // ENABLE(CSS_DEVICE_ADAPTATION)
