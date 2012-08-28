/*
 * Copyright (C) 2006 Apple Inc. All rights reserved.
 * Copyright (C) 2006 Nikolas Zimmermann <zimmermann@kde.org>
 * Copyright (C) 2007 Rob Buis <buis@kde.org>
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
#include "SVGDocumentExtensions.h"

#include "Console.h"
#include "DOMWindow.h"
#include "Document.h"
#include "EventListener.h"
#include "Frame.h"
#include "FrameLoader.h"
#include "Page.h"
#include "SMILTimeContainer.h"
#include "SVGElement.h"
#include "SVGResourcesCache.h"
#include "SVGSMILElement.h"
#include "SVGSVGElement.h"
#include "ScriptableDocumentParser.h"
#include "XLinkNames.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

SVGDocumentExtensions::SVGDocumentExtensions(Document* document)
    : m_document(document)
    , m_resourcesCache(adoptPtr(new SVGResourcesCache))
{
}

SVGDocumentExtensions::~SVGDocumentExtensions()
{
    deleteAllValues(m_animatedElements);
    deleteAllValues(m_pendingResources);
    deleteAllValues(m_pendingResourcesForRemoval);
}

void SVGDocumentExtensions::addTimeContainer(SVGSVGElement* element)
{
    m_timeContainers.add(element);
}

void SVGDocumentExtensions::removeTimeContainer(SVGSVGElement* element)
{
    m_timeContainers.remove(element);
}

void SVGDocumentExtensions::addResource(const AtomicString& id, RenderSVGResourceContainer* resource)
{
    ASSERT(resource);

    if (id.isEmpty())
        return;

    // Replaces resource if already present, to handle potential id changes
    m_resources.set(id, resource);
}

void SVGDocumentExtensions::removeResource(const AtomicString& id)
{
    if (id.isEmpty() || !m_resources.contains(id))
        return;

    m_resources.remove(id);
}

RenderSVGResourceContainer* SVGDocumentExtensions::resourceById(const AtomicString& id) const
{
    if (id.isEmpty())
        return 0;

    return m_resources.get(id);
}

void SVGDocumentExtensions::startAnimations()
{
    // FIXME: Eventually every "Time Container" will need a way to latch on to some global timer
    // starting animations for a document will do this "latching"
    // FIXME: We hold a ref pointers to prevent a shadow tree from getting removed out from underneath us.
    // In the future we should refactor the use-element to avoid this. See https://webkit.org/b/53704
    Vector<RefPtr<SVGSVGElement> > timeContainers;
    timeContainers.appendRange(m_timeContainers.begin(), m_timeContainers.end());
    Vector<RefPtr<SVGSVGElement> >::iterator end = timeContainers.end();
    for (Vector<RefPtr<SVGSVGElement> >::iterator itr = timeContainers.begin(); itr != end; ++itr)
        (*itr)->timeContainer()->begin();
}
    
void SVGDocumentExtensions::pauseAnimations()
{
    HashSet<SVGSVGElement*>::iterator end = m_timeContainers.end();
    for (HashSet<SVGSVGElement*>::iterator itr = m_timeContainers.begin(); itr != end; ++itr)
        (*itr)->pauseAnimations();
}

void SVGDocumentExtensions::unpauseAnimations()
{
    HashSet<SVGSVGElement*>::iterator end = m_timeContainers.end();
    for (HashSet<SVGSVGElement*>::iterator itr = m_timeContainers.begin(); itr != end; ++itr)
        (*itr)->unpauseAnimations();
}

void SVGDocumentExtensions::dispatchSVGLoadEventToOutermostSVGElements()
{
    Vector<RefPtr<SVGSVGElement> > timeContainers;
    timeContainers.appendRange(m_timeContainers.begin(), m_timeContainers.end());

    Vector<RefPtr<SVGSVGElement> >::iterator end = timeContainers.end();
    for (Vector<RefPtr<SVGSVGElement> >::iterator it = timeContainers.begin(); it != end; ++it) {
        SVGSVGElement* outerSVG = (*it).get();
        if (!outerSVG->isOutermostSVGSVGElement())
            continue;
        outerSVG->sendSVGLoadEventIfPossible();
    }
}

void SVGDocumentExtensions::addAnimationElementToTarget(SVGSMILElement* animationElement, SVGElement* targetElement)
{
    ASSERT(targetElement);
    ASSERT(animationElement);

    if (HashSet<SVGSMILElement*>* animationElementsForTarget = m_animatedElements.get(targetElement)) {
        animationElementsForTarget->add(animationElement);
        return;
    }

    HashSet<SVGSMILElement*>* animationElementsForTarget = new HashSet<SVGSMILElement*>;
    animationElementsForTarget->add(animationElement);
    m_animatedElements.set(targetElement, animationElementsForTarget);
}

void SVGDocumentExtensions::removeAnimationElementFromTarget(SVGSMILElement* animationElement, SVGElement* targetElement)
{
    ASSERT(targetElement);
    ASSERT(animationElement);

    HashMap<SVGElement*, HashSet<SVGSMILElement*>* >::iterator it = m_animatedElements.find(targetElement);
    ASSERT(it != m_animatedElements.end());
    
    HashSet<SVGSMILElement*>* animationElementsForTarget = it->second;
    ASSERT(!animationElementsForTarget->isEmpty());

    animationElementsForTarget->remove(animationElement);
    if (animationElementsForTarget->isEmpty()) {
        m_animatedElements.remove(it);
        delete animationElementsForTarget;
    }
}

void SVGDocumentExtensions::removeAllAnimationElementsFromTarget(SVGElement* targetElement)
{
    ASSERT(targetElement);
    HashMap<SVGElement*, HashSet<SVGSMILElement*>* >::iterator it = m_animatedElements.find(targetElement);
    if (it == m_animatedElements.end())
        return;

    HashSet<SVGSMILElement*>* animationElementsForTarget = it->second;
    Vector<SVGSMILElement*> toBeReset;

    HashSet<SVGSMILElement*>::iterator end = animationElementsForTarget->end();
    for (HashSet<SVGSMILElement*>::iterator it = animationElementsForTarget->begin(); it != end; ++it)
        toBeReset.append(*it);

    Vector<SVGSMILElement*>::iterator vectorEnd = toBeReset.end();
    for (Vector<SVGSMILElement*>::iterator vectorIt = toBeReset.begin(); vectorIt != vectorEnd; ++vectorIt)
        (*vectorIt)->resetTargetElement();
}

// FIXME: Callers should probably use ScriptController::eventHandlerLineNumber()
static int parserLineNumber(Document* document)
{
    ScriptableDocumentParser* parser = document->scriptableDocumentParser();
    if (!parser)
        return 1;
    return parser->lineNumber().oneBasedInt();
}

static void reportMessage(Document* document, MessageLevel level, const String& message)
{
    if (document->frame())
        document->domWindow()->console()->addMessage(JSMessageSource, LogMessageType, level, message, document->documentURI(), parserLineNumber(document));
}

void SVGDocumentExtensions::reportWarning(const String& message)
{
    reportMessage(m_document, WarningMessageLevel, "Warning: " + message);
}

void SVGDocumentExtensions::reportError(const String& message)
{
    reportMessage(m_document, ErrorMessageLevel, "Error: " + message);
}

void SVGDocumentExtensions::addPendingResource(const AtomicString& id, SVGStyledElement* element)
{
    ASSERT(element);

    if (id.isEmpty())
        return;

    // The HashMap add function leaves the map alone and returns a pointer to the element in the
    // map if the element already exists. So we add with a value of 0, and it either finds the
    // existing element or adds a new one in a single operation. The ".iterator->second" idiom gets
    // us to the iterator from add's result, and then to the value inside the hash table.
    SVGPendingElements*& set = m_pendingResources.add(id, 0).iterator->second;
    if (!set)
        set = new SVGPendingElements;
    set->add(element);

    element->setHasPendingResources();
}

bool SVGDocumentExtensions::hasPendingResource(const AtomicString& id) const
{
    if (id.isEmpty())
        return false;

    return m_pendingResources.contains(id);
}

bool SVGDocumentExtensions::isElementPendingResources(SVGStyledElement* element) const
{
    // This algorithm takes time proportional to the number of pending resources and need not.
    // If performance becomes an issue we can keep a counted set of elements and answer the question efficiently.

    ASSERT(element);

    HashMap<AtomicString, SVGPendingElements*>::const_iterator end = m_pendingResources.end();
    for (HashMap<AtomicString, SVGPendingElements*>::const_iterator it = m_pendingResources.begin(); it != end; ++it) {
        SVGPendingElements* elements = it->second;
        ASSERT(elements);

        if (elements->contains(element))
            return true;
    }
    return false;
}

bool SVGDocumentExtensions::isElementPendingResource(SVGStyledElement* element, const AtomicString& id) const
{
    ASSERT(element);

    if (!hasPendingResource(id))
        return false;

    return m_pendingResources.get(id)->contains(element);
}

void SVGDocumentExtensions::removeElementFromPendingResources(SVGStyledElement* element)
{
    ASSERT(element);

    // Remove the element from pending resources.
    if (!m_pendingResources.isEmpty() && element->hasPendingResources()) {
        Vector<AtomicString> toBeRemoved;
        HashMap<AtomicString, SVGPendingElements*>::iterator end = m_pendingResources.end();
        for (HashMap<AtomicString, SVGPendingElements*>::iterator it = m_pendingResources.begin(); it != end; ++it) {
            SVGPendingElements* elements = it->second;
            ASSERT(elements);
            ASSERT(!elements->isEmpty());

            elements->remove(element);
            if (elements->isEmpty())
                toBeRemoved.append(it->first);
        }

        element->clearHasPendingResourcesIfPossible();

        // We use the removePendingResource function here because it deals with set lifetime correctly.
        Vector<AtomicString>::iterator vectorEnd = toBeRemoved.end();
        for (Vector<AtomicString>::iterator it = toBeRemoved.begin(); it != vectorEnd; ++it)
            removePendingResource(*it);
    }

    // Remove the element from pending resources that were scheduled for removal.
    if (!m_pendingResourcesForRemoval.isEmpty()) {
        Vector<AtomicString> toBeRemoved;
        HashMap<AtomicString, SVGPendingElements*>::iterator end = m_pendingResourcesForRemoval.end();
        for (HashMap<AtomicString, SVGPendingElements*>::iterator it = m_pendingResourcesForRemoval.begin(); it != end; ++it) {
            SVGPendingElements* elements = it->second;
            ASSERT(elements);
            ASSERT(!elements->isEmpty());

            elements->remove(element);
            if (elements->isEmpty())
                toBeRemoved.append(it->first);
        }

        // We use the removePendingResourceForRemoval function here because it deals with set lifetime correctly.
        Vector<AtomicString>::iterator vectorEnd = toBeRemoved.end();
        for (Vector<AtomicString>::iterator it = toBeRemoved.begin(); it != vectorEnd; ++it)
            removePendingResourceForRemoval(*it);
    }
}

PassOwnPtr<SVGDocumentExtensions::SVGPendingElements> SVGDocumentExtensions::removePendingResource(const AtomicString& id)
{
    ASSERT(m_pendingResources.contains(id));
    return adoptPtr(m_pendingResources.take(id));
}

PassOwnPtr<SVGDocumentExtensions::SVGPendingElements> SVGDocumentExtensions::removePendingResourceForRemoval(const AtomicString& id)
{
    ASSERT(m_pendingResourcesForRemoval.contains(id));
    return adoptPtr(m_pendingResourcesForRemoval.take(id));
}

void SVGDocumentExtensions::markPendingResourcesForRemoval(const AtomicString& id)
{
    if (id.isEmpty())
        return;

    ASSERT(!m_pendingResourcesForRemoval.contains(id));

    SVGPendingElements* existing = m_pendingResources.take(id);
    if (existing && !existing->isEmpty())
        m_pendingResourcesForRemoval.add(id, existing);
}

SVGStyledElement* SVGDocumentExtensions::removeElementFromPendingResourcesForRemoval(const AtomicString& id)
{
    if (id.isEmpty())
        return 0;

    SVGPendingElements* resourceSet = m_pendingResourcesForRemoval.get(id);
    if (!resourceSet || resourceSet->isEmpty())
        return 0;

    SVGPendingElements::iterator firstElement = resourceSet->begin();
    SVGStyledElement* element = *firstElement;

    resourceSet->remove(firstElement);

    if (resourceSet->isEmpty())
        removePendingResourceForRemoval(id);

    return element;
}

HashSet<SVGElement*>* SVGDocumentExtensions::setOfElementsReferencingTarget(SVGElement* referencedElement) const
{
    ASSERT(referencedElement);
    const HashMap<SVGElement*, OwnPtr<HashSet<SVGElement*> > >::const_iterator it = m_elementDependencies.find(referencedElement);
    if (it == m_elementDependencies.end())
        return 0;
    return it->second.get();
}

void SVGDocumentExtensions::addElementReferencingTarget(SVGElement* referencingElement, SVGElement* referencedElement)
{
    ASSERT(referencingElement);
    ASSERT(referencedElement);

    if (HashSet<SVGElement*>* elements = m_elementDependencies.get(referencedElement)) {
        elements->add(referencingElement);
        return;
    }

    OwnPtr<HashSet<SVGElement*> > elements = adoptPtr(new HashSet<SVGElement*>);
    elements->add(referencingElement);
    m_elementDependencies.set(referencedElement, elements.release());
}

void SVGDocumentExtensions::removeAllTargetReferencesForElement(SVGElement* referencingElement)
{
    Vector<SVGElement*> toBeRemoved;

    HashMap<SVGElement*, OwnPtr<HashSet<SVGElement*> > >::iterator end = m_elementDependencies.end();
    for (HashMap<SVGElement*, OwnPtr<HashSet<SVGElement*> > >::iterator it = m_elementDependencies.begin(); it != end; ++it) {
        SVGElement* referencedElement = it->first;
        HashSet<SVGElement*>* referencingElements = it->second.get();
        HashSet<SVGElement*>::iterator setIt = referencingElements->find(referencingElement);
        if (setIt == referencingElements->end())
            continue;

        referencingElements->remove(setIt);
        if (referencingElements->isEmpty())
            toBeRemoved.append(referencedElement);
    }

    Vector<SVGElement*>::iterator vectorEnd = toBeRemoved.end();
    for (Vector<SVGElement*>::iterator it = toBeRemoved.begin(); it != vectorEnd; ++it)
        m_elementDependencies.remove(*it);
}

void SVGDocumentExtensions::removeAllElementReferencesForTarget(SVGElement* referencedElement)
{
    ASSERT(referencedElement);
    HashMap<SVGElement*, OwnPtr<HashSet<SVGElement*> > >::iterator it = m_elementDependencies.find(referencedElement);
    if (it == m_elementDependencies.end())
        return;
    ASSERT(it->first == referencedElement);
    Vector<SVGElement*> toBeNotified;

    HashSet<SVGElement*>* referencingElements = it->second.get();
    HashSet<SVGElement*>::iterator setEnd = referencingElements->end();
    for (HashSet<SVGElement*>::iterator setIt = referencingElements->begin(); setIt != setEnd; ++setIt)
        toBeNotified.append(*setIt);

    // Force rebuilding the referencingElement so it knows about this change.
    Vector<SVGElement*>::iterator vectorEnd = toBeNotified.end();
    for (Vector<SVGElement*>::iterator vectorIt = toBeNotified.begin(); vectorIt != vectorEnd; ++vectorIt) {
        // Before rebuilding referencingElement ensure it was not removed from under us.
        if (HashSet<SVGElement*>* referencingElements = setOfElementsReferencingTarget(referencedElement)) {
            if (referencingElements->contains(*vectorIt))
                (*vectorIt)->svgAttributeChanged(XLinkNames::hrefAttr);
        }
    }

    m_elementDependencies.remove(referencedElement);
}

#if ENABLE(SVG_FONTS)
void SVGDocumentExtensions::registerSVGFontFaceElement(SVGFontFaceElement* element)
{
    m_svgFontFaceElements.add(element);
}

void SVGDocumentExtensions::unregisterSVGFontFaceElement(SVGFontFaceElement* element)
{
    ASSERT(m_svgFontFaceElements.contains(element));
    m_svgFontFaceElements.remove(element);
}
#endif

}

#endif
