/*
 * Copyright (C) 2013 Adobe Systems Incorporated. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDER "AS IS" AND ANY
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

#include "config.h"
#include "WebKitCSSFilterRule.h"

#if ENABLE(CSS_SHADERS)

#include "PropertySetCSSStyleDeclaration.h"
#include "StylePropertySet.h"
#include "StyleRule.h"
#include <wtf/text/StringBuilder.h>

namespace WebCore {

WebKitCSSFilterRule::WebKitCSSFilterRule(StyleRuleFilter* filterRule, CSSStyleSheet* parent)
    : CSSRule(parent)
    , m_filterRule(filterRule)
{
}

WebKitCSSFilterRule::~WebKitCSSFilterRule()
{
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->clearParentRule();
}

CSSStyleDeclaration* WebKitCSSFilterRule::style()
{
    if (!m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper = StyleRuleCSSStyleDeclaration::create(m_filterRule->mutableProperties(), *this);
    return m_propertiesCSSOMWrapper.get();
}

String WebKitCSSFilterRule::cssText() const
{
    StringBuilder result;
    result.appendLiteral("@-webkit-filter ");

    String filterName = m_filterRule->filterName();
    result.append(filterName);
    result.appendLiteral(" { ");

    String descs = m_filterRule->properties().asText();
    result.append(descs);
    if (!descs.isEmpty())
        result.append(' ');
    result.append('}');

    return result.toString();
}

void WebKitCSSFilterRule::reattach(StyleRuleBase* rule)
{
    ASSERT(rule);
    ASSERT_WITH_SECURITY_IMPLICATION(rule->isFilterRule());
    m_filterRule = static_cast<StyleRuleFilter*>(rule);
    if (m_propertiesCSSOMWrapper)
        m_propertiesCSSOMWrapper->reattach(m_filterRule->mutableProperties());
}

} // namespace WebCore

#endif // ENABLE(CSS_SHADERS)
