/*
 * Copyright (c) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "HTMLOutputElement.h"

#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include <wtf/IsoMallocInlines.h>
#include <wtf/NeverDestroyed.h>

namespace WebCore {

WTF_MAKE_ISO_ALLOCATED_IMPL(HTMLOutputElement);

using namespace HTMLNames;

inline HTMLOutputElement::HTMLOutputElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
    : HTMLFormControlElement(tagName, document, form)
    , m_isDefaultValueMode(true)
    , m_isSetTextContentInProgress(false)
    , m_defaultValue(emptyString())
{
}

Ref<HTMLOutputElement> HTMLOutputElement::create(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
{
    return adoptRef(*new HTMLOutputElement(tagName, document, form));
}

const AtomString& HTMLOutputElement::formControlType() const
{
    static MainThreadNeverDestroyed<const AtomString> output("output", AtomString::ConstructFromLiteral);
    return output;
}

bool HTMLOutputElement::supportsFocus() const
{
    return HTMLElement::supportsFocus();
}

void HTMLOutputElement::parseAttribute(const QualifiedName& name, const AtomString& value)
{
    if (name == forAttr) {
        if (m_tokens)
            m_tokens->associatedAttributeValueChanged(value);
    } else
        HTMLFormControlElement::parseAttribute(name, value);
}

void HTMLOutputElement::childrenChanged(const ChildChange& change)
{
    HTMLFormControlElement::childrenChanged(change);

    if (change.source == ChildChangeSource::Parser || m_isSetTextContentInProgress) {
        m_isSetTextContentInProgress = false;
        return;
    }

    if (m_isDefaultValueMode)
        m_defaultValue = textContent();
}

void HTMLOutputElement::reset()
{
    // The reset algorithm for output elements is to set the element's
    // value mode flag to "default" and then to set the element's textContent
    // attribute to the default value.
    m_isDefaultValueMode = true;
    if (m_defaultValue == value())
        return;
    setTextContentInternal(m_defaultValue);
}

String HTMLOutputElement::value() const
{
    return textContent();
}

void HTMLOutputElement::setValue(const String& value)
{
    // The value mode flag set to "value" when the value attribute is set.
    m_isDefaultValueMode = false;
    if (value == this->value())
        return;
    setTextContentInternal(value);
}

String HTMLOutputElement::defaultValue() const
{
    return m_defaultValue;
}

void HTMLOutputElement::setDefaultValue(const String& value)
{
    if (m_defaultValue == value)
        return;
    m_defaultValue = value;
    // The spec requires the value attribute set to the default value
    // when the element's value mode flag to "default".
    if (m_isDefaultValueMode)
        setTextContentInternal(value);
}

DOMTokenList& HTMLOutputElement::htmlFor()
{
    if (!m_tokens)
        m_tokens = makeUnique<DOMTokenList>(*this, forAttr);
    return *m_tokens;
}

void HTMLOutputElement::setTextContentInternal(const String& value)
{
    ASSERT(!m_isSetTextContentInProgress);
    m_isSetTextContentInProgress = true;
    setTextContent(value);
}

} // namespace
