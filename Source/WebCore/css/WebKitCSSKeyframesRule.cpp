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

#include "config.h"
#include "WebKitCSSKeyframesRule.h"

#include "CSSParser.h"
#include "CSSRuleList.h"
#include "StylePropertySet.h"
#include "StyleSheet.h"
#include "WebKitCSSKeyframeRule.h"

namespace WebCore {

WebKitCSSKeyframesRule::WebKitCSSKeyframesRule(CSSStyleSheet* parent)
    : CSSRule(parent, CSSRule::WEBKIT_KEYFRAMES_RULE)
    , m_lstCSSRules(CSSRuleList::create())
{
}

WebKitCSSKeyframesRule::~WebKitCSSKeyframesRule()
{
    for (unsigned i = 0; i < length(); ++i) {
        WebKitCSSKeyframeRule* rule = item(i);
        rule->setParentRule(0);
    }
}

void WebKitCSSKeyframesRule::setName(const String& name)
{
    m_name = name;

    // Since the name is used in the keyframe map list in CSSStyleSelector, we need
    // to recompute the style sheet to get the updated name.
    if (CSSStyleSheet* styleSheet = parentStyleSheet())
        styleSheet->styleSheetChanged();
}

WebKitCSSKeyframeRule* WebKitCSSKeyframesRule::item(unsigned index)
{
    CSSRule* rule = m_lstCSSRules->item(index);
    ASSERT(rule->isKeyframeRule());
    return static_cast<WebKitCSSKeyframeRule*>(rule);
}

const WebKitCSSKeyframeRule* WebKitCSSKeyframesRule::item(unsigned index) const
{
    const CSSRule* rule = m_lstCSSRules->item(index);
    ASSERT(rule->isKeyframeRule());
    return static_cast<const WebKitCSSKeyframeRule*>(rule);
}

void WebKitCSSKeyframesRule::append(WebKitCSSKeyframeRule* rule)
{
    if (!rule)
        return;

    m_lstCSSRules->append(rule);
    rule->setParentRule(this);
}

void WebKitCSSKeyframesRule::insertRule(const String& rule)
{
    CSSParser p(useStrictParsing());
    RefPtr<WebKitCSSKeyframeRule> newRule = p.parseKeyframeRule(parentStyleSheet(), rule);
    if (newRule)
        append(newRule.get());
}

void WebKitCSSKeyframesRule::deleteRule(const String& s)
{
    int i = findRuleIndex(s);
    if (i < 0)
        return;

    WebKitCSSKeyframeRule* rule = item(i);
    rule->setParentRule(0);
    m_lstCSSRules->deleteRule(i);
}

WebKitCSSKeyframeRule* WebKitCSSKeyframesRule::findRule(const String& s)
{
    int i = findRuleIndex(s);
    return (i >= 0) ? item(i) : 0;
}

int WebKitCSSKeyframesRule::findRuleIndex(const String& key) const
{
    String percentageString;
    if (equalIgnoringCase(key, "from"))
        percentageString = "0%";
    else if (equalIgnoringCase(key, "to"))
        percentageString = "100%";
    else
        percentageString = key;

    for (unsigned i = 0; i < length(); ++i) {
        if (item(i)->keyText() == percentageString)
            return i;
    }

    return -1;
}

String WebKitCSSKeyframesRule::cssText() const
{
    String result = "@-webkit-keyframes ";
    result += m_name;
    result += " { \n";

    if (m_lstCSSRules)
        result += m_lstCSSRules->rulesText();

    result += "}";
    return result;
}

} // namespace WebCore
