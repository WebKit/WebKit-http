/*
 * Copyright (C) 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2011 Google Inc. All rights reserved.
 * Copyright (C) 2009 Joseph Pecoraro
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
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
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

#include "config.h"
#include "InspectorDOMAgent.h"

#if ENABLE(INSPECTOR)

#include "Attr.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "CharacterData.h"
#include "ContainerNode.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "DOMNodeHighlighter.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Event.h"
#include "EventContext.h"
#include "EventListener.h"
#include "EventNames.h"
#include "EventTarget.h"
#include "Frame.h"
#include "FrameTree.h"
#include "HitTestResult.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "IdentifiersFactory.h"
#include "InjectedScriptManager.h"
#include "InspectorClient.h"
#include "InspectorFrontend.h"
#include "InspectorPageAgent.h"
#include "InspectorState.h"
#include "InstrumentingAgents.h"
#include "IntRect.h"
#include "MutationEvent.h"
#include "Node.h"
#include "NodeList.h"
#include "Page.h"
#include "Pasteboard.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ScriptEventListener.h"
#include "StyleSheetList.h"
#include "Text.h"
#include "XPathResult.h"

#include "markup.h"

#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>
#include <wtf/HashSet.h>
#include <wtf/ListHashSet.h>
#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

namespace DOMAgentState {
static const char documentRequested[] = "documentRequested";
};

static const size_t maxTextSize = 10000;
static const UChar ellipsisUChar[] = { 0x2026, 0 };

static Color parseColor(const RefPtr<InspectorObject>* colorObject)
{
    if (!colorObject || !(*colorObject))
        return Color::transparent;

    int r;
    int g;
    int b;
    bool success = (*colorObject)->getNumber("r", &r);
    success |= (*colorObject)->getNumber("g", &g);
    success |= (*colorObject)->getNumber("b", &b);
    if (!success)
        return Color::transparent;

    double a;
    success = (*colorObject)->getNumber("a", &a);
    if (!success)
        return Color(r, g, b);

    // Clamp alpha to the [0..1] range.
    if (a < 0)
        a = 0;
    else if (a > 1)
        a = 1;

    return Color(r, g, b, static_cast<int>(a * 255));
}

static Color parseConfigColor(const String& fieldName, InspectorObject* configObject)
{
    const RefPtr<InspectorObject> colorObject = configObject->getObject(fieldName);
    return parseColor(&colorObject);
}

class RevalidateStyleAttributeTask {
public:
    RevalidateStyleAttributeTask(InspectorDOMAgent*);
    void scheduleFor(Element*);
    void reset() { m_timer.stop(); }
    void onTimer(Timer<RevalidateStyleAttributeTask>*);

private:
    InspectorDOMAgent* m_domAgent;
    Timer<RevalidateStyleAttributeTask> m_timer;
    HashSet<RefPtr<Element> > m_elements;
};

RevalidateStyleAttributeTask::RevalidateStyleAttributeTask(InspectorDOMAgent* domAgent)
    : m_domAgent(domAgent)
    , m_timer(this, &RevalidateStyleAttributeTask::onTimer)
{
}

void RevalidateStyleAttributeTask::scheduleFor(Element* element)
{
    m_elements.add(element);
    if (!m_timer.isActive())
        m_timer.startOneShot(0);
}

void RevalidateStyleAttributeTask::onTimer(Timer<RevalidateStyleAttributeTask>*)
{
    // The timer is stopped on m_domAgent destruction, so this method will never be called after m_domAgent has been destroyed.
    Vector<Element*> elements;
    for (HashSet<RefPtr<Element> >::iterator it = m_elements.begin(), end = m_elements.end(); it != end; ++it)
        elements.append(it->get());
    m_domAgent->styleAttributeInvalidated(elements);

    m_elements.clear();
}

InspectorDOMAgent::InspectorDOMAgent(InstrumentingAgents* instrumentingAgents, InspectorPageAgent* pageAgent, InspectorClient* client, InspectorState* inspectorState, InjectedScriptManager* injectedScriptManager)
    : InspectorBaseAgent<InspectorDOMAgent>("DOM", instrumentingAgents, inspectorState)
    , m_pageAgent(pageAgent)
    , m_client(client)
    , m_injectedScriptManager(injectedScriptManager)
    , m_frontend(0)
    , m_domListener(0)
    , m_lastNodeId(1)
    , m_searchingForNode(false)
{
}

InspectorDOMAgent::~InspectorDOMAgent()
{
    reset();
    ASSERT(!m_highlightData || (!m_highlightData->node && !m_highlightData->rect));
    ASSERT(!m_searchingForNode);
}

void InspectorDOMAgent::setFrontend(InspectorFrontend* frontend)
{
    ASSERT(!m_frontend);
    m_frontend = frontend->dom();
    m_instrumentingAgents->setInspectorDOMAgent(this);
    m_document = m_pageAgent->mainFrame()->document();

    if (m_nodeToFocus)
        focusNode();
}

void InspectorDOMAgent::clearFrontend()
{
    ASSERT(m_frontend);
    setSearchingForNode(false, 0);

    ErrorString error;
    hideHighlight(&error);

    m_frontend = 0;
    m_instrumentingAgents->setInspectorDOMAgent(0);
    m_state->setBoolean(DOMAgentState::documentRequested, false);
    reset();
}

void InspectorDOMAgent::restore()
{
    // Reset document to avoid early return from setDocument.
    m_document = 0;
    setDocument(m_pageAgent->mainFrame()->document());
}

Vector<Document*> InspectorDOMAgent::documents()
{
    Vector<Document*> result;
    for (Frame* frame = m_document->frame(); frame; frame = frame->tree()->traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;
        result.append(document);
    }
    return result;
}

Node* InspectorDOMAgent::highlightedNode() const
{
    return m_highlightData ? m_highlightData->node.get() : 0;
}

void InspectorDOMAgent::reset()
{
    ErrorString error;
    m_searchResults.clear();
    discardBindings();
    if (m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask->reset();
    m_document = 0;
}

void InspectorDOMAgent::setDOMListener(DOMListener* listener)
{
    m_domListener = listener;
}

void InspectorDOMAgent::setDocument(Document* doc)
{
    if (doc == m_document.get())
        return;

    reset();

    m_document = doc;

    if (!m_state->getBoolean(DOMAgentState::documentRequested))
        return;

    // Immediately communicate 0 document or document that has finished loading.
    if (!doc || !doc->parsing())
        m_frontend->documentUpdated();
}

void InspectorDOMAgent::releaseDanglingNodes()
{
    deleteAllValues(m_danglingNodeToIdMaps);
    m_danglingNodeToIdMaps.clear();
}

int InspectorDOMAgent::bind(Node* node, NodeToIdMap* nodesMap)
{
    int id = nodesMap->get(node);
    if (id)
        return id;
    id = m_lastNodeId++;
    nodesMap->set(node, id);
    m_idToNode.set(id, node);
    m_idToNodesMap.set(id, nodesMap);
    return id;
}

void InspectorDOMAgent::unbind(Node* node, NodeToIdMap* nodesMap)
{
    if (node->isFrameOwnerElement()) {
        const HTMLFrameOwnerElement* frameOwner = static_cast<const HTMLFrameOwnerElement*>(node);
        if (m_domListener)
            m_domListener->didRemoveDocument(frameOwner->contentDocument());
    }

    int id = nodesMap->get(node);
    if (!id)
        return;
    m_idToNode.remove(id);
    nodesMap->remove(node);
    bool childrenRequested = m_childrenRequested.contains(id);
    if (childrenRequested) {
        // Unbind subtree known to client recursively.
        m_childrenRequested.remove(id);
        Node* child = innerFirstChild(node);
        while (child) {
            unbind(child, nodesMap);
            child = innerNextSibling(child);
        }
    }
}

Node* InspectorDOMAgent::assertNode(ErrorString* errorString, int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node) {
        *errorString = "Could not find node with given id";
        return 0;
    }
    return node;
}

Element* InspectorDOMAgent::assertElement(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return 0;

    if (node->nodeType() != Node::ELEMENT_NODE) {
        *errorString = "Node is not an Element";
        return 0;
    }
    return toElement(node);
}


HTMLElement* InspectorDOMAgent::assertHTMLElement(ErrorString* errorString, int nodeId)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return 0;

    if (!element->isHTMLElement()) {
        *errorString = "Node is not an HTML Element";
        return 0;
    }
    return toHTMLElement(element);
}

void InspectorDOMAgent::getDocument(ErrorString*, RefPtr<InspectorObject>* root)
{
    m_state->setBoolean(DOMAgentState::documentRequested, true);

    if (!m_document)
        return;

    // Reset backend state.
    RefPtr<Document> doc = m_document;
    reset();
    m_document = doc;

    *root = buildObjectForNode(m_document.get(), 2, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::pushChildNodesToFrontend(int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node || (node->nodeType() != Node::ELEMENT_NODE && node->nodeType() != Node::DOCUMENT_NODE && node->nodeType() != Node::DOCUMENT_FRAGMENT_NODE))
        return;
    if (m_childrenRequested.contains(nodeId))
        return;

    NodeToIdMap* nodeMap = m_idToNodesMap.get(nodeId);
    RefPtr<InspectorArray> children = buildArrayForContainerChildren(node, 1, nodeMap);
    m_frontend->setChildNodes(nodeId, children.release());
}

void InspectorDOMAgent::discardBindings()
{
    m_documentNodeToIdMap.clear();
    m_idToNode.clear();
    releaseDanglingNodes();
    m_childrenRequested.clear();
}

Node* InspectorDOMAgent::nodeForId(int id)
{
    if (!id)
        return 0;

    HashMap<int, Node*>::iterator it = m_idToNode.find(id);
    if (it != m_idToNode.end())
        return it->second;
    return 0;
}

void InspectorDOMAgent::requestChildNodes(ErrorString*, int nodeId)
{
    pushChildNodesToFrontend(nodeId);
}

void InspectorDOMAgent::querySelector(ErrorString* errorString, int nodeId, const String& selectors, int* elementId)
{
    *elementId = 0;
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ExceptionCode ec = 0;
    RefPtr<Element> element = node->querySelector(selectors, ec);
    if (ec) {
        *errorString = "DOM Error while querying";
        return;
    }

    if (element)
        *elementId = pushNodePathToFrontend(element.get());
}

void InspectorDOMAgent::querySelectorAll(ErrorString* errorString, int nodeId, const String& selectors, RefPtr<InspectorArray>* result)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ExceptionCode ec = 0;
    RefPtr<NodeList> nodes = node->querySelectorAll(selectors, ec);
    if (ec) {
        *errorString = "DOM Error while querying";
        return;
    }

    for (unsigned i = 0; i < nodes->length(); ++i)
        (*result)->pushNumber(pushNodePathToFrontend(nodes->item(i)));
}

int InspectorDOMAgent::pushNodePathToFrontend(Node* nodeToPush)
{
    ASSERT(nodeToPush);  // Invalid input

    if (!m_document)
        return 0;
    if (!m_documentNodeToIdMap.contains(m_document))
        return 0;

    // Return id in case the node is known.
    int result = m_documentNodeToIdMap.get(nodeToPush);
    if (result)
        return result;

    Node* node = nodeToPush;
    Vector<Node*> path;
    NodeToIdMap* danglingMap = 0;

    while (true) {
        Node* parent = innerParentNode(node);
        if (!parent) {
            // Node being pushed is detached -> push subtree root.
            danglingMap = new NodeToIdMap();
            m_danglingNodeToIdMaps.append(danglingMap);
            RefPtr<InspectorArray> children = InspectorArray::create();
            children->pushObject(buildObjectForNode(node, 0, danglingMap));
            m_frontend->setChildNodes(0, children);
            break;
        } else {
            path.append(parent);
            if (m_documentNodeToIdMap.get(parent))
                break;
            else
                node = parent;
        }
    }

    NodeToIdMap* map = danglingMap ? danglingMap : &m_documentNodeToIdMap;
    for (int i = path.size() - 1; i >= 0; --i) {
        int nodeId = map->get(path.at(i));
        ASSERT(nodeId);
        pushChildNodesToFrontend(nodeId);
    }
    return map->get(nodeToPush);
}

int InspectorDOMAgent::boundNodeId(Node* node)
{
    return m_documentNodeToIdMap.get(node);
}

void InspectorDOMAgent::setAttributeValue(ErrorString* errorString, int elementId, const String& name, const String& value)
{
    Element* element = assertElement(errorString, elementId);
    if (!element)
        return;

    ExceptionCode ec = 0;
    element->setAttribute(name, value, ec);
    if (ec)
        *errorString = "Internal error: could not set attribute value";
}

void InspectorDOMAgent::setAttributesAsText(ErrorString* errorString, int elementId, const String& text, const String* const name)
{
    Element* element = assertElement(errorString, elementId);
    if (!element)
        return;

    ExceptionCode ec = 0;
    RefPtr<Element> parsedElement = element->document()->createElement("span", ec);
    if (ec) {
        *errorString = "Internal error: could not set attribute value";
        return;
    }

    toHTMLElement(parsedElement.get())->setInnerHTML("<span " + text + "></span>", ec);
    if (ec) {
        *errorString = "Could not parse value as attributes";
        return;
    }

    Node* child = parsedElement->firstChild();
    if (!child) {
        *errorString = "Could not parse value as attributes";
        return;
    }

    const NamedNodeMap* attrMap = toHTMLElement(child)->attributes(true);
    if (!attrMap && name) {
        element->removeAttribute(*name, ec);
        if (ec)
            *errorString = "Could not remove attribute";
        return;
    }

    bool foundOriginalAttribute = false;
    unsigned numAttrs = attrMap->length();
    for (unsigned i = 0; i < numAttrs; ++i) {
        // Add attribute pair
        const Attribute* attribute = attrMap->attributeItem(i);
        foundOriginalAttribute = foundOriginalAttribute || (name && attribute->name().toString() == *name);
        element->setAttribute(attribute->name(), attribute->value(), ec);
        if (ec) {
            *errorString = "Internal error: could not set attribute value";
            return;
        }
    }

    if (!foundOriginalAttribute && name) {
        element->removeAttribute(*name, ec);
        if (ec)
            *errorString = "Could not remove attribute";
        return;
    }
}

void InspectorDOMAgent::removeAttribute(ErrorString* errorString, int elementId, const String& name)
{
    Element* element = assertElement(errorString, elementId);
    if (element) {
        ExceptionCode ec = 0;
        element->removeAttribute(name, ec);
        if (ec)
            *errorString = "Exception while removing attribute";
    }
}

void InspectorDOMAgent::removeNode(ErrorString* errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    ContainerNode* parentNode = node->parentNode();
    if (!parentNode) {
        *errorString = "Can not remove detached node";
        return;
    }

    ExceptionCode ec = 0;
    parentNode->removeChild(node, ec);
    if (ec)
        *errorString = "Could not remove node due to DOM exception";
}

void InspectorDOMAgent::setNodeName(ErrorString*, int nodeId, const String& tagName, int* newId)
{
    *newId = 0;

    Node* oldNode = nodeForId(nodeId);
    if (!oldNode || !oldNode->isElementNode())
        return;

    ExceptionCode ec = 0;
    RefPtr<Element> newElem = oldNode->document()->createElement(tagName, ec);
    if (ec)
        return;

    // Copy over the original node's attributes.
    Element* oldElem = static_cast<Element*>(oldNode);
    if (oldElem->attributes())
        newElem->attributes()->setAttributes(*(oldElem->attributes(true)));

    // Copy over the original node's children.
    Node* child;
    while ((child = oldNode->firstChild()))
        newElem->appendChild(child, ec);

    // Replace the old node with the new node
    ContainerNode* parent = oldNode->parentNode();
    parent->insertBefore(newElem, oldNode->nextSibling(), ec);
    parent->removeChild(oldNode, ec);

    if (ec)
        return;

    *newId = pushNodePathToFrontend(newElem.get());
    if (m_childrenRequested.contains(nodeId))
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::getOuterHTML(ErrorString* errorString, int nodeId, WTF::String* outerHTML)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    if (node->isHTMLElement()) {
        *outerHTML = static_cast<HTMLElement*>(node)->outerHTML();
        return;
    }

    if (node->isCommentNode()) {
        *outerHTML = "<!--" + node->nodeValue() + "-->";
        return;
    }

    if (node->isTextNode()) {
        *outerHTML = node->nodeValue();
        return;
    }

    *errorString = "Only HTMLElements, Comments, and Text nodes are supported";
}

void InspectorDOMAgent::setOuterHTML(ErrorString* errorString, int nodeId, const String& outerHTML, int* newId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    Element* parentElement = node->parentElement();
    if (!parentElement)
        return;

    Document* document = node->ownerDocument();
    if (!document->isHTMLDocument()) {
        *errorString = "Not an HTML document";
        return;
    }

    Node* previousSibling = node->previousSibling(); // Remember previous sibling before replacing node.

    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document);
    fragment->parseHTML(outerHTML, parentElement);

    ExceptionCode ec = 0;
    parentElement->replaceChild(fragment.release(), node, ec);
    if (ec) {
        *errorString = "Failed to replace Node with new contents";
        return;
    }

    bool requiresTotalUpdate = false;
    if (node->isHTMLElement())
        requiresTotalUpdate = node->nodeName() == "HTML" || node->nodeName() == "BODY" || node->nodeName() == "HEAD";

    if (requiresTotalUpdate) {
        RefPtr<Document> document = m_document;
        reset();
        setDocument(document.get());
        *newId = 0;
        return;
    }

    Node* newNode = previousSibling ? previousSibling->nextSibling() : parentElement->firstChild();
    if (!newNode) {
        // The only child node has been deleted.
        *newId = 0;
        return;
    }

    *newId = pushNodePathToFrontend(newNode);

    bool childrenRequested = m_childrenRequested.contains(nodeId);
    if (childrenRequested)
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::setNodeValue(ErrorString* errorString, int nodeId, const String& value)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    if (node->nodeType() != Node::TEXT_NODE) {
        *errorString = "Can only set value of text nodes";
        return;
    }

    Text* textNode = static_cast<Text*>(node);
    ExceptionCode ec = 0;
    textNode->replaceWholeText(value, ec);
    if (ec)
        *errorString = "DOM Error while setting the node value";
}

void InspectorDOMAgent::getEventListenersForNode(ErrorString*, int nodeId, RefPtr<InspectorArray>* listenersArray)
{
    Node* node = nodeForId(nodeId);
    EventTargetData* d;

    // Quick break if a null node or no listeners at all
    if (!node || !(d = node->eventTargetData()))
        return;

    // Get the list of event types this Node is concerned with
    Vector<AtomicString> eventTypes = d->eventListenerMap.eventTypes();

    // Quick break if no useful listeners
    size_t eventTypesLength = eventTypes.size();
    if (!eventTypesLength)
        return;

    // The Node's Ancestors (not including self)
    Vector<ContainerNode*> ancestors;
    for (ContainerNode* ancestor = node->parentOrHostNode(); ancestor; ancestor = ancestor->parentOrHostNode())
        ancestors.append(ancestor);

    // Nodes and their Listeners for the concerned event types (order is top to bottom)
    Vector<EventListenerInfo> eventInformation;
    for (size_t i = ancestors.size(); i; --i) {
        ContainerNode* ancestor = ancestors[i - 1];
        for (size_t j = 0; j < eventTypesLength; ++j) {
            AtomicString& type = eventTypes[j];
            if (ancestor->hasEventListeners(type))
                eventInformation.append(EventListenerInfo(ancestor, type, ancestor->getEventListeners(type)));
        }
    }

    // Insert the Current Node at the end of that list (last in capturing, first in bubbling)
    for (size_t i = 0; i < eventTypesLength; ++i) {
        const AtomicString& type = eventTypes[i];
        eventInformation.append(EventListenerInfo(node, type, node->getEventListeners(type)));
    }

    // Get Capturing Listeners (in this order)
    size_t eventInformationLength = eventInformation.size();
    for (size_t i = 0; i < eventInformationLength; ++i) {
        const EventListenerInfo& info = eventInformation[i];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (listener.useCapture)
                (*listenersArray)->pushObject(buildObjectForEventListener(listener, info.eventType, info.node));
        }
    }

    // Get Bubbling Listeners (reverse order)
    for (size_t i = eventInformationLength; i; --i) {
        const EventListenerInfo& info = eventInformation[i - 1];
        const EventListenerVector& vector = info.eventListenerVector;
        for (size_t j = 0; j < vector.size(); ++j) {
            const RegisteredEventListener& listener = vector[j];
            if (!listener.useCapture)
                (*listenersArray)->pushObject(buildObjectForEventListener(listener, info.eventType, info.node));
        }
    }
}

void InspectorDOMAgent::performSearch(ErrorString*, const String& whitespaceTrimmedQuery, String* searchId, int* resultCount)
{
    // FIXME: Few things are missing here:
    // 1) Search works with node granularity - number of matches within node is not calculated.
    // 2) There is no need to push all search results to the front-end at a time, pushing next / previous result
    //    is sufficient.

    unsigned queryLength = whitespaceTrimmedQuery.length();
    bool startTagFound = !whitespaceTrimmedQuery.find('<');
    bool endTagFound = whitespaceTrimmedQuery.reverseFind('>') + 1 == queryLength;

    String tagNameQuery = whitespaceTrimmedQuery;
    if (startTagFound)
        tagNameQuery = tagNameQuery.right(tagNameQuery.length() - 1);
    if (endTagFound)
        tagNameQuery = tagNameQuery.left(tagNameQuery.length() - 1);

    Vector<Document*> docs = documents();
    ListHashSet<Node*> resultCollector;

    for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
        Document* document = *it;
        Node* node = document->documentElement();

        // Manual plain text search.
        while ((node = node->traverseNextNode(document->documentElement()))) {
            switch (node->nodeType()) {
            case Node::TEXT_NODE:
            case Node::COMMENT_NODE:
            case Node::CDATA_SECTION_NODE: {
                String text = node->nodeValue();
                if (text.findIgnoringCase(whitespaceTrimmedQuery) != notFound)
                    resultCollector.add(node);
                break;
            }
            case Node::ELEMENT_NODE: {
                if (node->nodeName().findIgnoringCase(tagNameQuery) != notFound) {
                    resultCollector.add(node);
                    break;
                }
                // Go through all attributes and serialize them.
                const NamedNodeMap* attrMap = static_cast<Element*>(node)->attributes(true);
                if (!attrMap)
                    break;

                unsigned numAttrs = attrMap->length();
                for (unsigned i = 0; i < numAttrs; ++i) {
                    // Add attribute pair
                    const Attribute* attribute = attrMap->attributeItem(i);
                    if (attribute->localName().find(whitespaceTrimmedQuery) != notFound) {
                        resultCollector.add(node);
                        break;
                    }
                    if (attribute->value().find(whitespaceTrimmedQuery) != notFound) {
                        resultCollector.add(node);
                        break;
                    }
                }
                break;
            }
            default:
                break;
            }
        }

        // XPath evaluation
        for (Vector<Document*>::iterator it = docs.begin(); it != docs.end(); ++it) {
            Document* document = *it;
            ExceptionCode ec = 0;
            RefPtr<XPathResult> result = document->evaluate(whitespaceTrimmedQuery, document, 0, XPathResult::ORDERED_NODE_SNAPSHOT_TYPE, 0, ec);
            if (ec || !result)
                continue;

            unsigned long size = result->snapshotLength(ec);
            for (unsigned long i = 0; !ec && i < size; ++i) {
                Node* node = result->snapshotItem(i, ec);
                if (ec)
                    break;

                if (node->nodeType() == Node::ATTRIBUTE_NODE)
                    node = static_cast<Attr*>(node)->ownerElement();
                resultCollector.add(node);
            }
        }
    }

    *searchId = IdentifiersFactory::createIdentifier();
    SearchResults::iterator resultsIt = m_searchResults.add(*searchId, Vector<RefPtr<Node> >()).first;

    for (ListHashSet<Node*>::iterator it = resultCollector.begin(); it != resultCollector.end(); ++it)
        resultsIt->second.append(*it);

    *resultCount = resultsIt->second.size();
}

void InspectorDOMAgent::getSearchResults(ErrorString* errorString, const String& searchId, int fromIndex, int toIndex, RefPtr<InspectorArray>* nodeIds)
{
    SearchResults::iterator it = m_searchResults.find(searchId);
    if (it == m_searchResults.end()) {
        *errorString = "No search session with given id found";
        return;
    }

    int size = it->second.size();
    if (fromIndex < 0 || toIndex > size || fromIndex >= toIndex) {
        *errorString = "Invalid search result range";
        return;
    }

    for (int i = fromIndex; i < toIndex; ++i)
        (*nodeIds)->pushNumber(pushNodePathToFrontend((it->second)[i].get()));
}

void InspectorDOMAgent::discardSearchResults(ErrorString*, const String& searchId)
{
    m_searchResults.remove(searchId);
}

bool InspectorDOMAgent::handleMousePress()
{
    if (!m_searchingForNode)
        return false;

    if (m_highlightData && m_highlightData->node) {
        RefPtr<Node> node = m_highlightData->node;
        setSearchingForNode(false, 0);
        inspect(node.get());
    }
    return true;
}

void InspectorDOMAgent::inspect(Node* node)
{
    if (node->nodeType() != Node::ELEMENT_NODE && node->nodeType() != Node::DOCUMENT_NODE)
        node = node->parentNode();
    m_nodeToFocus = node;

    focusNode();
}

void InspectorDOMAgent::focusNode()
{
    if (!m_frontend)
        return;

    ASSERT(m_nodeToFocus);

    RefPtr<Node> node = m_nodeToFocus.get();
    m_nodeToFocus = 0;

    Document* document = node->ownerDocument();
    if (!document)
        return;
    Frame* frame = document->frame();
    if (!frame)
        return;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));
    if (injectedScript.hasNoValue())
        return;

    injectedScript.inspectNode(node.get());
}

void InspectorDOMAgent::mouseDidMoveOverElement(const HitTestResult& result, unsigned)
{
    if (!m_searchingForNode || !m_highlightData)
        return;

    Node* node = result.innerNode();
    while (node && node->nodeType() == Node::TEXT_NODE)
        node = node->parentNode();
    if (node) {
        m_highlightData->node = node;
        highlight();
    }
}

void InspectorDOMAgent::setSearchingForNode(bool enabled, InspectorObject* highlightConfig)
{
    if (m_searchingForNode == enabled)
        return;
    m_searchingForNode = enabled;
    if (enabled)
        setHighlightDataFromConfig(highlightConfig);
    else {
        ErrorString error;
        hideHighlight(&error);
        m_highlightData.clear();
    }
}

void InspectorDOMAgent::setInspectModeEnabled(ErrorString*, bool enabled, const RefPtr<InspectorObject>* highlightConfig)
{
    setSearchingForNode(enabled, highlightConfig ? highlightConfig->get() : 0);
}

bool InspectorDOMAgent::setHighlightDataFromConfig(InspectorObject* highlightConfig)
{
    if (!highlightConfig) {
        m_highlightData.clear();
        return false;
    }

    m_highlightData = adoptPtr(new HighlightData());
    bool showInfo = false; // Default: false (do not show a tooltip).
    highlightConfig->getBoolean("showInfo", &showInfo);
    m_highlightData->showInfo = showInfo;
    m_highlightData->content = parseConfigColor("contentColor", highlightConfig);
    m_highlightData->contentOutline = parseConfigColor("contentOutlineColor", highlightConfig);
    m_highlightData->padding = parseConfigColor("paddingColor", highlightConfig);
    m_highlightData->border = parseConfigColor("borderColor", highlightConfig);
    m_highlightData->margin = parseConfigColor("marginColor", highlightConfig);
    return true;
}

void InspectorDOMAgent::highlight()
{
    // This method requires m_highlightData to have been filled in by its client.
    ASSERT(m_highlightData);
    m_client->highlight();
}

void InspectorDOMAgent::highlightRect(ErrorString*, int x, int y, int width, int height, const RefPtr<InspectorObject>* color, const RefPtr<InspectorObject>* outlineColor)
{
    m_highlightData = adoptPtr(new HighlightData());
    m_highlightData->rect = adoptPtr(new IntRect(x, y, width, height));
    m_highlightData->content = parseColor(color);
    m_highlightData->contentOutline = parseColor(outlineColor);
    m_client->highlight();
}

void InspectorDOMAgent::highlightNode(
    ErrorString*,
    int nodeId,
    const RefPtr<InspectorObject> highlightConfig)
{
    if (Node* node = nodeForId(nodeId)) {
        if (setHighlightDataFromConfig(highlightConfig.get())) {
            m_highlightData->node = node;
            highlight();
        }
    }
}

void InspectorDOMAgent::highlightFrame(
    ErrorString*,
    const String& frameId,
    const RefPtr<InspectorObject>* color,
    const RefPtr<InspectorObject>* outlineColor)
{
    Frame* frame = m_pageAgent->frameForId(frameId);
    if (frame && frame->ownerElement()) {
        m_highlightData = adoptPtr(new HighlightData());
        m_highlightData->node = frame->ownerElement();
        m_highlightData->showInfo = true; // Always show tooltips for frames.
        m_highlightData->content = parseColor(color);
        m_highlightData->contentOutline = parseColor(outlineColor);
        highlight();
    }
}

void InspectorDOMAgent::hideHighlight(ErrorString*)
{
    if (m_highlightData) {
        m_highlightData->node.clear();
        m_highlightData->rect.clear();
    }
    m_client->hideHighlight();
}

void InspectorDOMAgent::moveTo(ErrorString* error, int nodeId, int targetElementId, const int* const anchorNodeId, int* newNodeId)
{
    Node* node = assertNode(error, nodeId);
    if (!node)
        return;

    Element* targetElement = assertElement(error, targetElementId);
    if (!targetElement)
        return;

    Node* anchorNode = 0;
    if (anchorNodeId && *anchorNodeId) {
        anchorNode = assertNode(error, *anchorNodeId);
        if (!anchorNode)
            return;
        if (anchorNode->parentNode() != targetElement) {
            *error = "Anchor node must be child of the target element";
            return;
        }
    }

    ExceptionCode ec = 0;
    bool success = targetElement->insertBefore(node, anchorNode, ec);
    if (ec || !success) {
        *error = "Could not drop node";
        return;
    }
    *newNodeId = pushNodePathToFrontend(node);
}

void InspectorDOMAgent::resolveNode(ErrorString* error, int nodeId, const String* const objectGroup, RefPtr<InspectorObject>* result)
{
    String objectGroupName = objectGroup ? *objectGroup : "";
    Node* node = nodeForId(nodeId);
    if (!node) {
        *error = "No node with given id found";
        return;
    }
    RefPtr<InspectorObject> object = resolveNode(node, objectGroupName);
    if (!object) {
        *error = "Node with given id does not belong to the document";
        return;
    }
    *result = object;
}

void InspectorDOMAgent::getAttributes(ErrorString* errorString, int nodeId, RefPtr<InspectorArray>* result)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return;

    *result = buildArrayForElementAttributes(element);
}

void InspectorDOMAgent::requestNode(ErrorString*, const String& objectId, int* nodeId)
{
    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptForObjectId(objectId);
    Node* node = injectedScript.nodeForObjectId(objectId);
    if (node)
        *nodeId = pushNodePathToFrontend(node);
    else
        *nodeId = 0;
}

// static
String InspectorDOMAgent::documentURLString(Document* document)
{
    if (!document || document->url().isNull())
        return "";
    return document->url().string();
}

PassRefPtr<InspectorObject> InspectorDOMAgent::buildObjectForNode(Node* node, int depth, NodeToIdMap* nodesMap)
{
    RefPtr<InspectorObject> value = InspectorObject::create();

    int id = bind(node, nodesMap);
    String nodeName;
    String localName;
    String nodeValue;

    switch (node->nodeType()) {
    case Node::TEXT_NODE:
    case Node::COMMENT_NODE:
    case Node::CDATA_SECTION_NODE:
        nodeValue = node->nodeValue();
        if (nodeValue.length() > maxTextSize) {
            nodeValue = nodeValue.left(maxTextSize);
            nodeValue.append(ellipsisUChar);
        }
        break;
    case Node::ATTRIBUTE_NODE:
        localName = node->localName();
        break;
    case Node::DOCUMENT_FRAGMENT_NODE:
        break;
    case Node::DOCUMENT_NODE:
    case Node::ELEMENT_NODE:
    default:
        nodeName = node->nodeName();
        localName = node->localName();
        break;
    }

    value->setNumber("nodeId", id);
    value->setNumber("nodeType", node->nodeType());
    value->setString("nodeName", nodeName);
    value->setString("localName", localName);
    value->setString("nodeValue", nodeValue);

    if (node->nodeType() == Node::ELEMENT_NODE || node->nodeType() == Node::DOCUMENT_NODE || node->nodeType() == Node::DOCUMENT_FRAGMENT_NODE) {
        int nodeCount = innerChildNodeCount(node);
        value->setNumber("childNodeCount", nodeCount);
        RefPtr<InspectorArray> children = buildArrayForContainerChildren(node, depth, nodesMap);
        if (children->length() > 0)
            value->setArray("children", children.release());

        if (node->nodeType() == Node::ELEMENT_NODE) {
            Element* element = static_cast<Element*>(node);
            value->setArray("attributes", buildArrayForElementAttributes(element));
            if (node->isFrameOwnerElement()) {
                HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(node);
                value->setString("documentURL", documentURLString(frameOwner->contentDocument()));
            }
        } else if (node->nodeType() == Node::DOCUMENT_NODE) {
            Document* document = static_cast<Document*>(node);
            value->setString("documentURL", documentURLString(document));
            value->setString("xmlVersion", document->xmlVersion());
        }
    } else if (node->nodeType() == Node::DOCUMENT_TYPE_NODE) {
        DocumentType* docType = static_cast<DocumentType*>(node);
        value->setString("publicId", docType->publicId());
        value->setString("systemId", docType->systemId());
        value->setString("internalSubset", docType->internalSubset());
    } else if (node->nodeType() == Node::ATTRIBUTE_NODE) {
        Attr* attribute = static_cast<Attr*>(node);
        value->setString("name", attribute->name());
        value->setString("value", attribute->value());
    }
    return value.release();
}

PassRefPtr<InspectorArray> InspectorDOMAgent::buildArrayForElementAttributes(Element* element)
{
    RefPtr<InspectorArray> attributesValue = InspectorArray::create();
    // Go through all attributes and serialize them.
    const NamedNodeMap* attrMap = element->attributes(true);
    if (!attrMap)
        return attributesValue.release();
    unsigned numAttrs = attrMap->length();
    for (unsigned i = 0; i < numAttrs; ++i) {
        // Add attribute pair
        const Attribute* attribute = attrMap->attributeItem(i);
        attributesValue->pushString(attribute->name().toString());
        attributesValue->pushString(attribute->value());
    }
    return attributesValue.release();
}

PassRefPtr<InspectorArray> InspectorDOMAgent::buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap)
{
    RefPtr<InspectorArray> children = InspectorArray::create();
    Node* child = innerFirstChild(container);

    if (depth == 0) {
        // Special-case the only text child - pretend that container's children have been requested.
        if (child && child->nodeType() == Node::TEXT_NODE && !innerNextSibling(child))
            return buildArrayForContainerChildren(container, 1, nodesMap);
        return children.release();
    }

    depth--;
    m_childrenRequested.add(bind(container, nodesMap));

    while (child) {
        children->pushObject(buildObjectForNode(child, depth, nodesMap));
        child = innerNextSibling(child);
    }
    return children.release();
}

PassRefPtr<InspectorObject> InspectorDOMAgent::buildObjectForEventListener(const RegisteredEventListener& registeredEventListener, const AtomicString& eventType, Node* node)
{
    RefPtr<EventListener> eventListener = registeredEventListener.listener;
    RefPtr<InspectorObject> value = InspectorObject::create();
    value->setString("type", eventType);
    value->setBoolean("useCapture", registeredEventListener.useCapture);
    value->setBoolean("isAttribute", eventListener->isAttribute());
    value->setNumber("nodeId", pushNodePathToFrontend(node));
    value->setString("handlerBody", eventListenerHandlerBody(node->document(), eventListener.get()));
    String sourceName;
    int lineNumber;
    if (eventListenerHandlerLocation(node->document(), eventListener.get(), sourceName, lineNumber)) {
        RefPtr<InspectorObject> location = InspectorObject::create();
        location->setString("scriptId", sourceName);
        location->setNumber("lineNumber", lineNumber);
        value->setObject("location", location);
    }
    return value.release();
}

Node* InspectorDOMAgent::innerFirstChild(Node* node)
{
    if (node->isFrameOwnerElement()) {
        HTMLFrameOwnerElement* frameOwner = static_cast<HTMLFrameOwnerElement*>(node);
        Document* doc = frameOwner->contentDocument();
        if (doc)
            return doc->firstChild();
    }
    node = node->firstChild();
    while (isWhitespace(node))
        node = node->nextSibling();
    return node;
}

Node* InspectorDOMAgent::innerNextSibling(Node* node)
{
    do {
        node = node->nextSibling();
    } while (isWhitespace(node));
    return node;
}

Node* InspectorDOMAgent::innerPreviousSibling(Node* node)
{
    do {
        node = node->previousSibling();
    } while (isWhitespace(node));
    return node;
}

unsigned InspectorDOMAgent::innerChildNodeCount(Node* node)
{
    unsigned count = 0;
    Node* child = innerFirstChild(node);
    while (child) {
        count++;
        child = innerNextSibling(child);
    }
    return count;
}

Node* InspectorDOMAgent::innerParentNode(Node* node)
{
    ContainerNode* parent = node->parentNode();
    if (parent && parent->isDocumentNode())
        return static_cast<Document*>(parent)->ownerElement();
    return parent;
}

bool InspectorDOMAgent::isWhitespace(Node* node)
{
    //TODO: pull ignoreWhitespace setting from the frontend and use here.
    return node && node->nodeType() == Node::TEXT_NODE && node->nodeValue().stripWhiteSpace().length() == 0;
}

void InspectorDOMAgent::mainFrameDOMContentLoaded()
{
    // Re-push document once it is loaded.
    discardBindings();
    if (m_state->getBoolean(DOMAgentState::documentRequested))
        m_frontend->documentUpdated();
}

void InspectorDOMAgent::loadEventFired(Document* document)
{
    Element* frameOwner = document->ownerElement();
    if (!frameOwner)
        return;

    int frameOwnerId = m_documentNodeToIdMap.get(frameOwner);
    if (!frameOwnerId)
        return;

    if (!m_childrenRequested.contains(frameOwnerId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontend->childNodeCountUpdated(frameOwnerId, innerChildNodeCount(frameOwner));
    } else {
        // Re-add frame owner element together with its new children.
        int parentId = m_documentNodeToIdMap.get(innerParentNode(frameOwner));
        m_frontend->childNodeRemoved(parentId, frameOwnerId);
        RefPtr<InspectorObject> value = buildObjectForNode(frameOwner, 0, &m_documentNodeToIdMap);
        Node* previousSibling = innerPreviousSibling(frameOwner);
        int prevId = previousSibling ? m_documentNodeToIdMap.get(previousSibling) : 0;
        m_frontend->childNodeInserted(parentId, prevId, value.release());
        // Invalidate children requested flag for the element.
        m_childrenRequested.remove(m_childrenRequested.find(frameOwnerId));
    }
}

void InspectorDOMAgent::didInsertDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    // We could be attaching existing subtree. Forget the bindings.
    unbind(node, &m_documentNodeToIdMap);

    ContainerNode* parent = node->parentNode();
    int parentId = m_documentNodeToIdMap.get(parent);
    // Return if parent is not mapped yet.
    if (!parentId)
        return;

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontend->childNodeCountUpdated(parentId, innerChildNodeCount(parent));
    } else {
        // Children have been requested -> return value of a new child.
        Node* prevSibling = innerPreviousSibling(node);
        int prevId = prevSibling ? m_documentNodeToIdMap.get(prevSibling) : 0;
        RefPtr<InspectorObject> value = buildObjectForNode(node, 0, &m_documentNodeToIdMap);
        m_frontend->childNodeInserted(parentId, prevId, value.release());
    }
}

void InspectorDOMAgent::didRemoveDOMNode(Node* node)
{
    if (isWhitespace(node))
        return;

    ContainerNode* parent = node->parentNode();
    int parentId = m_documentNodeToIdMap.get(parent);
    // If parent is not mapped yet -> ignore the event.
    if (!parentId)
        return;

    if (m_domListener)
        m_domListener->didRemoveDOMNode(node);

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        if (innerChildNodeCount(parent) == 1)
            m_frontend->childNodeCountUpdated(parentId, 0);
    } else
        m_frontend->childNodeRemoved(parentId, m_documentNodeToIdMap.get(node));
    unbind(node, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::didModifyDOMAttr(Element* element, const AtomicString& name, const AtomicString& value)
{
    int id = boundNodeId(element);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (m_domListener)
        m_domListener->didModifyDOMAttr(element);

    m_frontend->attributeModified(id, name, value);
}

void InspectorDOMAgent::didRemoveDOMAttr(Element* element, const AtomicString& name)
{
    int id = boundNodeId(element);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (m_domListener)
        m_domListener->didModifyDOMAttr(element);

    m_frontend->attributeRemoved(id, name);
}

void InspectorDOMAgent::styleAttributeInvalidated(const Vector<Element*>& elements)
{
    RefPtr<InspectorArray> nodeIds = InspectorArray::create();
    for (unsigned i = 0, size = elements.size(); i < size; ++i) {
        Element* element = elements.at(i);
        int id = boundNodeId(element);
        // If node is not mapped yet -> ignore the event.
        if (!id)
            continue;

        if (m_domListener)
            m_domListener->didModifyDOMAttr(element);
        nodeIds->pushNumber(id);
    }
    m_frontend->inlineStyleInvalidated(nodeIds.release());
}

void InspectorDOMAgent::characterDataModified(CharacterData* characterData)
{
    int id = m_documentNodeToIdMap.get(characterData);
    if (!id)
        return;
    m_frontend->characterDataModified(id, characterData->data());
}

void InspectorDOMAgent::didInvalidateStyleAttr(Node* node)
{
    int id = m_documentNodeToIdMap.get(node);
    // If node is not mapped yet -> ignore the event.
    if (!id)
        return;

    if (!m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask = adoptPtr(new RevalidateStyleAttributeTask(this));
    m_revalidateStyleAttrTask->scheduleFor(static_cast<Element*>(node));
}

Node* InspectorDOMAgent::nodeForPath(const String& path)
{
    // The path is of form "1,HTML,2,BODY,1,DIV"
    if (!m_document)
        return 0;

    Node* node = m_document.get();
    Vector<String> pathTokens;
    path.split(",", false, pathTokens);
    if (!pathTokens.size())
        return 0;
    for (size_t i = 0; i < pathTokens.size() - 1; i += 2) {
        bool success = true;
        unsigned childNumber = pathTokens[i].toUInt(&success);
        if (!success)
            return 0;
        if (childNumber >= innerChildNodeCount(node))
            return 0;

        Node* child = innerFirstChild(node);
        String childName = pathTokens[i + 1];
        for (size_t j = 0; child && j < childNumber; ++j)
            child = innerNextSibling(child);

        if (!child || child->nodeName() != childName)
            return 0;
        node = child;
    }
    return node;
}

void InspectorDOMAgent::copyNode(ErrorString*, int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node)
        return;
    String markup = createMarkup(node);
    Pasteboard::generalPasteboard()->writePlainText(markup);
}

void InspectorDOMAgent::pushNodeByPathToFrontend(ErrorString*, const String& path, int* nodeId)
{
    if (Node* node = nodeForPath(path))
        *nodeId = pushNodePathToFrontend(node);
}

PassRefPtr<InspectorObject> InspectorDOMAgent::resolveNode(Node* node, const String& objectGroup)
{
    Document* document = node->isDocumentNode() ? node->document() : node->ownerDocument();
    Frame* frame = document ? document->frame() : 0;
    if (!frame)
        return 0;

    InjectedScript injectedScript = m_injectedScriptManager->injectedScriptFor(mainWorldScriptState(frame));
    if (injectedScript.hasNoValue())
        return 0;

    return injectedScript.wrapNode(node, objectGroup);
}

void InspectorDOMAgent::drawHighlight(GraphicsContext& context) const
{
    if (!m_highlightData)
        return;

    DOMNodeHighlighter::drawHighlight(context, m_highlightData->node ? m_highlightData->node->document() : m_document.get(), m_highlightData.get());
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
