/*
 * Copyright (C) 2004, 2005, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2007 Rob Buis <buis@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#if ENABLE(SVG)
#include "SVGScriptElement.h"

#include "Attribute.h"
#include "Document.h"
#include "Event.h"
#include "EventNames.h"
#include "HTMLNames.h"
#include "SVGAnimatedStaticPropertyTearOff.h"
#include "SVGElementInstance.h"
#include "SVGNames.h"
#include "ScriptEventListener.h"

namespace WebCore {

// Animated property definitions
DEFINE_ANIMATED_STRING(SVGScriptElement, XLinkNames::hrefAttr, Href, href)
DEFINE_ANIMATED_BOOLEAN(SVGScriptElement, SVGNames::externalResourcesRequiredAttr, ExternalResourcesRequired, externalResourcesRequired)

BEGIN_REGISTER_ANIMATED_PROPERTIES(SVGScriptElement)
    REGISTER_LOCAL_ANIMATED_PROPERTY(href)
    REGISTER_LOCAL_ANIMATED_PROPERTY(externalResourcesRequired)
END_REGISTER_ANIMATED_PROPERTIES

inline SVGScriptElement::SVGScriptElement(const QualifiedName& tagName, Document* document, bool wasInsertedByParser, bool alreadyStarted)
    : SVGElement(tagName, document)
    , ScriptElement(this, wasInsertedByParser, alreadyStarted)
{
    ASSERT(hasTagName(SVGNames::scriptTag));
    registerAnimatedPropertiesForSVGScriptElement();
}

PassRefPtr<SVGScriptElement> SVGScriptElement::create(const QualifiedName& tagName, Document* document, bool insertedByParser)
{
    return adoptRef(new SVGScriptElement(tagName, document, insertedByParser, false));
}

bool SVGScriptElement::isSupportedAttribute(const QualifiedName& attrName)
{
    DEFINE_STATIC_LOCAL(HashSet<QualifiedName>, supportedAttributes, ());
    if (supportedAttributes.isEmpty()) {
        SVGURIReference::addSupportedAttributes(supportedAttributes);
        SVGExternalResourcesRequired::addSupportedAttributes(supportedAttributes);
        supportedAttributes.add(SVGNames::typeAttr);
        supportedAttributes.add(HTMLNames::onerrorAttr);
    }
    return supportedAttributes.contains(attrName);
}

void SVGScriptElement::parseMappedAttribute(Attribute* attr)
{
    if (!isSupportedAttribute(attr->name())) {
        SVGElement::parseMappedAttribute(attr);
        return;
    }

    if (attr->name() == SVGNames::typeAttr) {
        setType(attr->value());
        return;
    }

    if (attr->name() == HTMLNames::onerrorAttr) {
        setAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(this, attr));
        return;
    }

    if (SVGURIReference::parseMappedAttribute(attr))
        return;
    if (SVGExternalResourcesRequired::parseMappedAttribute(attr))
        return;

    ASSERT_NOT_REACHED();
}

void SVGScriptElement::svgAttributeChanged(const QualifiedName& attrName)
{
    if (!isSupportedAttribute(attrName)) {
        SVGElement::svgAttributeChanged(attrName);
        return;
    }

    SVGElementInstance::InvalidationGuard invalidationGuard(this);

    if (attrName == SVGNames::typeAttr || attrName == HTMLNames::onerrorAttr)
        return;

    if (SVGURIReference::isKnownAttribute(attrName)) {
        handleSourceAttribute(href());
        return;
    }

    if (SVGExternalResourcesRequired::isKnownAttribute(attrName)) {
        // Handle dynamic updates of the 'externalResourcesRequired' attribute. Only possible case: changing from 'true' to 'false'
        // causes an immediate dispatch of the SVGLoad event. If the attribute value was 'false' before inserting the script element
        // in the document, the SVGLoad event has already been dispatched.
        if (!externalResourcesRequiredBaseValue() && !haveFiredLoadEvent() && !isParserInserted()) {
            setHaveFiredLoadEvent(true);
            ASSERT(haveLoadedRequiredResources());

            sendSVGLoadEventIfPossible();
        }
        return;
    }

    ASSERT_NOT_REACHED();
}

void SVGScriptElement::insertedIntoDocument()
{
    SVGElement::insertedIntoDocument();
    ScriptElement::insertedIntoDocument();

    if (isParserInserted())
        return;

    // Eventually send SVGLoad event now for the dynamically inserted script element
    if (!externalResourcesRequiredBaseValue()) {
        setHaveFiredLoadEvent(true);
        sendSVGLoadEventIfPossible();
    }
}

void SVGScriptElement::removedFromDocument()
{
    SVGElement::removedFromDocument();
    ScriptElement::removedFromDocument();
}

void SVGScriptElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    ScriptElement::childrenChanged();
    SVGElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
}

bool SVGScriptElement::isURLAttribute(Attribute* attr) const
{
    return attr->name() == sourceAttributeValue();
}

void SVGScriptElement::finishParsingChildren()
{
    SVGElement::finishParsingChildren();

    // A SVGLoad event has been fired by SVGElement::finishParsingChildren.
    if (!externalResourcesRequiredBaseValue())
        setHaveFiredLoadEvent(true);
}

String SVGScriptElement::type() const
{
    return m_type;
}

void SVGScriptElement::setType(const String& type)
{
    m_type = type;
}

void SVGScriptElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    SVGElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, document()->completeURL(href()));
}

bool SVGScriptElement::haveLoadedRequiredResources()
{
    return !externalResourcesRequiredBaseValue() || haveFiredLoadEvent();
}

String SVGScriptElement::sourceAttributeValue() const
{
    return href();
}

String SVGScriptElement::charsetAttributeValue() const
{
    return String();
}

String SVGScriptElement::typeAttributeValue() const
{
    return type();
}

String SVGScriptElement::languageAttributeValue() const
{
    return String();
}

String SVGScriptElement::forAttributeValue() const
{
    return String();
}

String SVGScriptElement::eventAttributeValue() const
{
    return String();
}

bool SVGScriptElement::asyncAttributeValue() const
{
    return false;
}

bool SVGScriptElement::deferAttributeValue() const
{
    return false;
}

bool SVGScriptElement::hasSourceAttribute() const
{
    return hasAttribute(XLinkNames::hrefAttr);
}

void SVGScriptElement::dispatchLoadEvent()
{
    bool externalResourcesRequired = externalResourcesRequiredBaseValue();

    if (isParserInserted())
        ASSERT(externalResourcesRequired != haveFiredLoadEvent());
    else if (haveFiredLoadEvent()) {
        // If we've already fired an load event and externalResourcesRequired is set to 'true'
        // externalResourcesRequired has been modified while loading the <script>. Don't dispatch twice.
        if (externalResourcesRequired)
            return;
    }

    // HTML and SVG differ completly in the 'onload' event handling of <script> elements.
    // HTML fires the 'load' event after it sucessfully loaded a remote resource, otherwhise an error event.
    // SVG fires the SVGLoad event immediately after parsing the <script> element, if externalResourcesRequired
    // is set to 'false', otherwhise it dispatches the 'SVGLoad' event just after loading the remote resource.
    if (externalResourcesRequired) {
        ASSERT(!haveFiredLoadEvent());

        // Dispatch SVGLoad event
        setHaveFiredLoadEvent(true);
        ASSERT(haveLoadedRequiredResources());

        sendSVGLoadEventIfPossible();
    }
}

PassRefPtr<Element> SVGScriptElement::cloneElementWithoutAttributesAndChildren()
{
    return adoptRef(new SVGScriptElement(tagQName(), document(), false, alreadyStarted()));
}

}

#endif // ENABLE(SVG)
