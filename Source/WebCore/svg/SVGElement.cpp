/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2004, 2005, 2006, 2008 Rob Buis <buis@kde.org>
 * Copyright (C) 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Alp Toker <alp@atoker.com>
 * Copyright (C) 2009 Cameron McCormack <cam@mcc.id.au>
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
#include "SVGElement.h"

#include "Attribute.h"
#include "CSSCursorImageValue.h"
#include "CSSStyleSelector.h"
#include "DOMImplementation.h"
#include "Document.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "FrameView.h"
#include "HTMLNames.h"
#include "RegisteredEventListener.h"
#include "RenderObject.h"
#include "SVGCursorElement.h"
#include "SVGDocumentExtensions.h"
#include "SVGElementInstance.h"
#include "SVGElementRareData.h"
#include "SVGNames.h"
#include "SVGSVGElement.h"
#include "SVGStyledLocatableElement.h"
#include "SVGTextElement.h"
#include "SVGURIReference.h"
#include "SVGUseElement.h"
#include "ScriptEventListener.h"
#include "XMLNames.h"

namespace WebCore {

using namespace HTMLNames;

SVGElement::SVGElement(const QualifiedName& tagName, Document* document, ConstructionType constructionType)
    : StyledElement(tagName, document, constructionType)
{
    setHasCustomStyleForRenderer();
}

PassRefPtr<SVGElement> SVGElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new SVGElement(tagName, document));
}

SVGElement::~SVGElement()
{
    if (!hasRareSVGData())
        ASSERT(!SVGElementRareData::rareDataMap().contains(this));
    else {
        SVGElementRareData::SVGElementRareDataMap& rareDataMap = SVGElementRareData::rareDataMap();
        SVGElementRareData::SVGElementRareDataMap::iterator it = rareDataMap.find(this);
        ASSERT(it != rareDataMap.end());

        SVGElementRareData* rareData = it->second;
        if (SVGCursorElement* cursorElement = rareData->cursorElement())
            cursorElement->removeClient(this);
        if (CSSCursorImageValue* cursorImageValue = rareData->cursorImageValue())
            cursorImageValue->removeReferencedElement(this);

        delete rareData;
        rareDataMap.remove(it);
    }
    document()->accessSVGExtensions()->removeAllAnimationElementsFromTarget(this);
}

SVGElementRareData* SVGElement::rareSVGData() const
{
    ASSERT(hasRareSVGData());
    return SVGElementRareData::rareDataFromMap(this);
}

SVGElementRareData* SVGElement::ensureRareSVGData()
{
    if (hasRareSVGData())
        return rareSVGData();

    ASSERT(!SVGElementRareData::rareDataMap().contains(this));
    SVGElementRareData* data = new SVGElementRareData;
    SVGElementRareData::rareDataMap().set(this, data);
    setHasRareSVGData();
    return data;
}

void SVGElement::reportAttributeParsingError(SVGParsingError error, Attribute* attribute)
{
    if (error == NoError)
        return;

    String errorString = "<" + tagName() + "> attribute " + attribute->name().toString() + "=\"" + attribute->value() + "\"";
    SVGDocumentExtensions* extensions = document()->accessSVGExtensions();

    if (error == NegativeValueForbiddenError) {
        extensions->reportError("Invalid negative value for " + errorString);
        return;
    }

    if (error == ParsingAttributeFailedError) {
        extensions->reportError("Invalid value for " + errorString);
        return;
    }

    ASSERT_NOT_REACHED();
}


bool SVGElement::isSupported(StringImpl* feature, StringImpl* version) const
{
    return DOMImplementation::hasFeature(feature, version);
}

String SVGElement::xmlbase() const
{
    return fastGetAttribute(XMLNames::baseAttr);
}

void SVGElement::setXmlbase(const String& value, ExceptionCode&)
{
    setAttribute(XMLNames::baseAttr, value);
}

void SVGElement::removedFromDocument()
{
    document()->accessSVGExtensions()->removeAllAnimationElementsFromTarget(this);
    StyledElement::removedFromDocument();
}

SVGSVGElement* SVGElement::ownerSVGElement() const
{
    ContainerNode* n = parentOrHostNode();
    while (n) {
        if (n->hasTagName(SVGNames::svgTag))
            return static_cast<SVGSVGElement*>(n);

        n = n->parentOrHostNode();
    }

    return 0;
}

SVGElement* SVGElement::viewportElement() const
{
    // This function needs shadow tree support - as RenderSVGContainer uses this function
    // to determine the "overflow" property. <use> on <symbol> wouldn't work otherwhise.
    ContainerNode* n = parentOrHostNode();
    while (n) {
        if (n->hasTagName(SVGNames::svgTag) || n->hasTagName(SVGNames::imageTag) || n->hasTagName(SVGNames::symbolTag))
            return static_cast<SVGElement*>(n);

        n = n->parentOrHostNode();
    }

    return 0;
}

SVGDocumentExtensions* SVGElement::accessDocumentSVGExtensions()
{
    // This function is provided for use by SVGAnimatedProperty to avoid
    // global inclusion of Document.h in SVG code.
    return document() ? document()->accessSVGExtensions() : 0;
}
 
void SVGElement::mapInstanceToElement(SVGElementInstance* instance)
{
    ASSERT(instance);

    HashSet<SVGElementInstance*>& instances = ensureRareSVGData()->elementInstances();
    ASSERT(!instances.contains(instance));

    instances.add(instance);
}
 
void SVGElement::removeInstanceMapping(SVGElementInstance* instance)
{
    ASSERT(instance);
    ASSERT(hasRareSVGData());

    HashSet<SVGElementInstance*>& instances = rareSVGData()->elementInstances();
    ASSERT(instances.contains(instance));

    instances.remove(instance);
}

const HashSet<SVGElementInstance*>& SVGElement::instancesForElement() const
{
    if (!hasRareSVGData()) {
        DEFINE_STATIC_LOCAL(HashSet<SVGElementInstance*>, emptyInstances, ());
        return emptyInstances;
    }
    return rareSVGData()->elementInstances();
}

bool SVGElement::boundingBox(FloatRect& rect, SVGLocatable::StyleUpdateStrategy styleUpdateStrategy)
{
    if (isStyledLocatable()) {
        rect = static_cast<SVGStyledLocatableElement*>(this)->getBBox(styleUpdateStrategy);
        return true;
    }
    if (hasTagName(SVGNames::textTag)) {
        rect = static_cast<SVGTextElement*>(this)->getBBox(styleUpdateStrategy);
        return true;
    }
    return false;
}

void SVGElement::setCursorElement(SVGCursorElement* cursorElement)
{
    SVGElementRareData* rareData = ensureRareSVGData();
    if (SVGCursorElement* oldCursorElement = rareData->cursorElement()) {
        if (cursorElement == oldCursorElement)
            return;
        oldCursorElement->removeReferencedElement(this);
    }
    rareData->setCursorElement(cursorElement);
}

void SVGElement::cursorElementRemoved() 
{
    ASSERT(hasRareSVGData());
    rareSVGData()->setCursorElement(0);
}

void SVGElement::setCursorImageValue(CSSCursorImageValue* cursorImageValue)
{
    SVGElementRareData* rareData = ensureRareSVGData();
    if (CSSCursorImageValue* oldCursorImageValue = rareData->cursorImageValue()) {
        if (cursorImageValue == oldCursorImageValue)
            return;
        oldCursorImageValue->removeReferencedElement(this);
    }
    rareData->setCursorImageValue(cursorImageValue);
}

void SVGElement::cursorImageValueRemoved()
{
    ASSERT(hasRareSVGData());
    rareSVGData()->setCursorImageValue(0);
}

SVGElement* SVGElement::correspondingElement()
{
    ASSERT(!hasRareSVGData() || !rareSVGData()->correspondingElement() || shadowTreeRootNode());
    return hasRareSVGData() ? rareSVGData()->correspondingElement() : 0;
}

void SVGElement::setCorrespondingElement(SVGElement* correspondingElement)
{
    ensureRareSVGData()->setCorrespondingElement(correspondingElement);
}

void SVGElement::parseMappedAttribute(Attribute* attr)
{
    // standard events
    if (attr->name() == onloadAttr)
        setAttributeEventListener(eventNames().loadEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onclickAttr)
        setAttributeEventListener(eventNames().clickEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onmousedownAttr)
        setAttributeEventListener(eventNames().mousedownEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onmousemoveAttr)
        setAttributeEventListener(eventNames().mousemoveEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onmouseoutAttr)
        setAttributeEventListener(eventNames().mouseoutEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onmouseoverAttr)
        setAttributeEventListener(eventNames().mouseoverEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == onmouseupAttr)
        setAttributeEventListener(eventNames().mouseupEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == SVGNames::onfocusinAttr)
        setAttributeEventListener(eventNames().focusinEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == SVGNames::onfocusoutAttr)
        setAttributeEventListener(eventNames().focusoutEvent, createAttributeEventListener(this, attr));
    else if (attr->name() == SVGNames::onactivateAttr)
        setAttributeEventListener(eventNames().DOMActivateEvent, createAttributeEventListener(this, attr));
    else
        StyledElement::parseMappedAttribute(attr);
}

void SVGElement::animatedPropertyTypeForAttribute(const QualifiedName& attributeName, Vector<AnimatedPropertyType>& propertyTypes)
{
    localAttributeToPropertyMap().animatedPropertyTypeForAttribute(attributeName, propertyTypes);
}

bool SVGElement::haveLoadedRequiredResources()
{
    Node* child = firstChild();
    while (child) {
        if (child->isSVGElement() && !static_cast<SVGElement*>(child)->haveLoadedRequiredResources())
            return false;
        child = child->nextSibling();
    }
    return true;
}

static bool hasLoadListener(Node* node)
{
    if (node->hasEventListeners(eventNames().loadEvent))
        return true;

    for (node = node->parentNode(); node && node->isElementNode(); node = node->parentNode()) {
        const EventListenerVector& entry = node->getEventListeners(eventNames().loadEvent);
        for (size_t i = 0; i < entry.size(); ++i) {
            if (entry[i].useCapture)
                return true;
        }
    }

    return false;
}

void SVGElement::sendSVGLoadEventIfPossible(bool sendParentLoadEvents)
{
    RefPtr<SVGElement> currentTarget = this;
    while (currentTarget && currentTarget->haveLoadedRequiredResources()) {
        RefPtr<Node> parent;
        if (sendParentLoadEvents)
            parent = currentTarget->parentNode(); // save the next parent to dispatch too incase dispatching the event changes the tree
        if (hasLoadListener(currentTarget.get()))
            currentTarget->dispatchEvent(Event::create(eventNames().loadEvent, false, false));
        currentTarget = (parent && parent->isSVGElement()) ? static_pointer_cast<SVGElement>(parent) : RefPtr<SVGElement>();
    }
}

void SVGElement::finishParsingChildren()
{
    StyledElement::finishParsingChildren();

    // finishParsingChildren() is called when the close tag is reached for an element (e.g. </svg>)
    // we send SVGLoad events here if we can, otherwise they'll be sent when any required loads finish
    sendSVGLoadEventIfPossible();
}

bool SVGElement::childShouldCreateRenderer(Node* child) const
{
    if (child->isSVGElement())
        return static_cast<SVGElement*>(child)->isValid();
    return false;
}

void SVGElement::attributeChanged(Attribute* attr, bool preserveDecls)
{
    ASSERT(attr);
    if (!attr)
        return;

    StyledElement::attributeChanged(attr, preserveDecls);

    // When an animated SVG property changes through SVG DOM, svgAttributeChanged() is called, not attributeChanged().
    // Next time someone tries to access the XML attributes, the synchronization code starts. During that synchronization
    // SVGAnimatedPropertySynchronizer may call NamedNodeMap::removeAttribute(), which in turn calls attributeChanged().
    // At this point we're not allowed to call svgAttributeChanged() again - it may lead to extra work being done, or crashes
    // see bug https://bugs.webkit.org/show_bug.cgi?id=40994.
    if (isSynchronizingSVGAttributes())
        return;

    if (isIdAttributeName(attr->name()))
        document()->accessSVGExtensions()->removeAllAnimationElementsFromTarget(this);

    // Changes to the style attribute are processed lazily (see Element::getAttribute() and related methods),
    // so we don't want changes to the style attribute to result in extra work here.
    if (attr->name() != HTMLNames::styleAttr)
        svgAttributeChanged(attr->name());
}

void SVGElement::updateAnimatedSVGAttribute(const QualifiedName& name) const
{
    if (isSynchronizingSVGAttributes() || areSVGAttributesValid())
        return;

    setIsSynchronizingSVGAttributes();

    SVGElement* nonConstThis = const_cast<SVGElement*>(this);
    if (name == anyQName()) {
        nonConstThis->localAttributeToPropertyMap().synchronizeProperties(nonConstThis);
        setAreSVGAttributesValid();
    } else
        nonConstThis->localAttributeToPropertyMap().synchronizeProperty(nonConstThis, name);

    clearIsSynchronizingSVGAttributes();
}

SVGAttributeToPropertyMap& SVGElement::localAttributeToPropertyMap()
{
    ASSERT_NOT_REACHED();

    DEFINE_STATIC_LOCAL(SVGAttributeToPropertyMap, dummyMap, ());
    return dummyMap;
}

void SVGElement::synchronizeRequiredFeatures(void* contextElement)
{
    ASSERT(contextElement);
    static_cast<SVGElement*>(contextElement)->synchronizeRequiredFeatures();
}

void SVGElement::synchronizeRequiredExtensions(void* contextElement)
{
    ASSERT(contextElement);
    static_cast<SVGElement*>(contextElement)->synchronizeRequiredExtensions();
}

void SVGElement::synchronizeSystemLanguage(void* contextElement)
{
    ASSERT(contextElement);
    static_cast<SVGElement*>(contextElement)->synchronizeSystemLanguage();
}

PassRefPtr<RenderStyle> SVGElement::customStyleForRenderer()
{
    if (correspondingElement())
        return document()->styleSelector()->styleForElement(correspondingElement(), parentNode() ? parentNode()->renderer()->style() : 0, false/*allowSharing*/);

    return document()->styleSelector()->styleForElement(static_cast<Element*>(this), 0, true);
}

}

#endif // ENABLE(SVG)
