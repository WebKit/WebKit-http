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

#ifndef WebKitCSSKeyframeRule_h
#define WebKitCSSKeyframeRule_h

#include "CSSRule.h"
#include "StylePropertySet.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>

namespace WebCore {

typedef int ExceptionCode;

class WebKitCSSKeyframeRule : public CSSRule {
public:
    static PassRefPtr<WebKitCSSKeyframeRule> create()
    {
        return adoptRef(new WebKitCSSKeyframeRule(0));
    }
    static PassRefPtr<WebKitCSSKeyframeRule> create(CSSStyleSheet* parent)
    {
        return adoptRef(new WebKitCSSKeyframeRule(parent));
    }

    ~WebKitCSSKeyframeRule();

    String keyText() const              { return m_key; }
    void setKeyText(const String& s)    { m_key = s; }

    void getKeys(Vector<float>& keys) const   { parseKeyString(m_key, keys); }

    CSSStyleDeclaration* style() const { return m_style ? m_style->ensureRuleCSSStyleDeclaration(this) : 0; }

    String cssText() const;

    StylePropertySet* declaration() const { return m_style.get(); }
    void setDeclaration(PassRefPtr<StylePropertySet>);

private:
    static void parseKeyString(const String& s, Vector<float>& keys);

    WebKitCSSKeyframeRule(CSSStyleSheet* parent);

    RefPtr<StylePropertySet> m_style;
    String m_key;        // comma separated list of keys
};

} // namespace WebCore

#endif // WebKitCSSKeyframeRule_h
