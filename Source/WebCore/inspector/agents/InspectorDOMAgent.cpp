/*
 * Copyright (C) 2009-2020 Apple Inc. All rights reserved.
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
 * 3.  Neither the name of Apple Inc. ("Apple") nor the names of
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

#include "AXObjectCache.h"
#include "AccessibilityNodeObject.h"
#include "Attr.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSPropertySourceData.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSheet.h"
#include "CharacterData.h"
#include "CommandLineAPIHost.h"
#include "ComposedTreeIterator.h"
#include "ContainerNode.h"
#include "Cookie.h"
#include "CookieJar.h"
#include "DOMEditor.h"
#include "DOMException.h"
#include "DOMPatchSupport.h"
#include "DOMWindow.h"
#include "Document.h"
#include "DocumentType.h"
#include "Editing.h"
#include "Element.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "Frame.h"
#include "FrameTree.h"
#include "FrameView.h"
#include "FullscreenManager.h"
#include "HTMLElement.h"
#include "HTMLFrameOwnerElement.h"
#include "HTMLMediaElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLScriptElement.h"
#include "HTMLStyleElement.h"
#include "HTMLTemplateElement.h"
#include "HTMLVideoElement.h"
#include "HitTestResult.h"
#include "InspectorCSSAgent.h"
#include "InspectorClient.h"
#include "InspectorController.h"
#include "InspectorHistory.h"
#include "InspectorNodeFinder.h"
#include "InspectorOverlay.h"
#include "InspectorPageAgent.h"
#include "InstrumentingAgents.h"
#include "IntRect.h"
#include "JSDOMBindingSecurity.h"
#include "JSEventListener.h"
#include "JSNode.h"
#include "MutationEvent.h"
#include "Node.h"
#include "NodeList.h"
#include "Page.h"
#include "Pasteboard.h"
#include "PseudoElement.h"
#include "RenderStyle.h"
#include "RenderStyleConstants.h"
#include "ScriptState.h"
#include "SelectorChecker.h"
#include "ShadowRoot.h"
#include "StaticNodeList.h"
#include "StyleProperties.h"
#include "StyleResolver.h"
#include "StyleSheetList.h"
#include "Text.h"
#include "TextNodeTraversal.h"
#include "Timer.h"
#include "VideoPlaybackQuality.h"
#include "WebInjectedScriptManager.h"
#include "XPathResult.h"
#include "markup.h"
#include <JavaScriptCore/IdentifiersFactory.h>
#include <JavaScriptCore/InjectedScript.h>
#include <JavaScriptCore/InjectedScriptManager.h>
#include <JavaScriptCore/JSCInlines.h>
#include <pal/crypto/CryptoDigest.h>
#include <wtf/Function.h>
#include <wtf/text/Base64.h>
#include <wtf/text/CString.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

using namespace Inspector;

using namespace HTMLNames;

static const size_t maxTextSize = 10000;
static const UChar ellipsisUChar[] = { 0x2026, 0 };

static Color parseColor(const JSON::Object* colorObject)
{
    if (!colorObject)
        return Color::transparentBlack;

    int r = 0;
    int g = 0;
    int b = 0;
    if (!colorObject->getInteger("r", r) || !colorObject->getInteger("g", g) || !colorObject->getInteger("b", b))
        return Color::transparentBlack;

    double a = 1.0;
    if (!colorObject->getDouble("a", a))
        return clampToComponentBytes<SRGBA>(r, g, b);
    return clampToComponentBytes<SRGBA>(r, g, b, convertToComponentByte(a));
}

static Color parseConfigColor(const String& fieldName, const JSON::Object* configObject)
{
    RefPtr<JSON::Object> colorObject;
    configObject->getObject(fieldName, colorObject);

    return parseColor(colorObject.get());
}

static bool parseQuad(const JSON::Array& quadArray, FloatQuad* quad)
{
    const size_t coordinatesInQuad = 8;
    double coordinates[coordinatesInQuad];
    if (quadArray.length() != coordinatesInQuad)
        return false;
    for (size_t i = 0; i < coordinatesInQuad; ++i) {
        if (!quadArray.get(i)->asDouble(*(coordinates + i)))
            return false;
    }
    quad->setP1(FloatPoint(coordinates[0], coordinates[1]));
    quad->setP2(FloatPoint(coordinates[2], coordinates[3]));
    quad->setP3(FloatPoint(coordinates[4], coordinates[5]));
    quad->setP4(FloatPoint(coordinates[6], coordinates[7]));

    return true;
}

class RevalidateStyleAttributeTask {
    WTF_MAKE_FAST_ALLOCATED;
public:
    RevalidateStyleAttributeTask(InspectorDOMAgent*);
    void scheduleFor(Element*);
    void reset() { m_timer.stop(); }
    void timerFired();

private:
    InspectorDOMAgent* m_domAgent;
    Timer m_timer;
    HashSet<RefPtr<Element>> m_elements;
};

RevalidateStyleAttributeTask::RevalidateStyleAttributeTask(InspectorDOMAgent* domAgent)
    : m_domAgent(domAgent)
    , m_timer(*this, &RevalidateStyleAttributeTask::timerFired)
{
}

void RevalidateStyleAttributeTask::scheduleFor(Element* element)
{
    m_elements.add(element);
    if (!m_timer.isActive())
        m_timer.startOneShot(0_s);
}

void RevalidateStyleAttributeTask::timerFired()
{
    // The timer is stopped on m_domAgent destruction, so this method will never be called after m_domAgent has been destroyed.
    Vector<Element*> elements;
    for (auto& element : m_elements)
        elements.append(element.get());
    m_domAgent->styleAttributeInvalidated(elements);

    m_elements.clear();
}

class InspectableNode final : public CommandLineAPIHost::InspectableObject {
public:
    explicit InspectableNode(Node* node)
        : m_node(node)
    {
    }

    JSC::JSValue get(JSC::JSGlobalObject& state) final
    {
        return InspectorDOMAgent::nodeAsScriptValue(state, m_node.get());
    }
private:
    RefPtr<Node> m_node;
};

class EventFiredCallback final : public EventListener {
public:
    static Ref<EventFiredCallback> create(InspectorDOMAgent& domAgent)
    {
        return adoptRef(*new EventFiredCallback(domAgent));
    }

    bool operator==(const EventListener& other) const final
    {
        return this == &other;
    }

    void handleEvent(ScriptExecutionContext&, Event& event) final
    {
        if (!is<Node>(event.target()) || m_domAgent.m_dispatchedEvents.contains(&event))
            return;

        auto* node = downcast<Node>(event.target());
        int nodeId = m_domAgent.pushNodePathToFrontend(node);
        if (!nodeId)
            return;

        m_domAgent.m_dispatchedEvents.add(&event);

        RefPtr<JSON::Object> data = JSON::Object::create();

#if ENABLE(FULLSCREEN_API)
        if (event.type() == eventNames().webkitfullscreenchangeEvent)
            data->setBoolean("enabled"_s, !!node->document().fullscreenManager().fullscreenElement());
#endif // ENABLE(FULLSCREEN_API)

        auto timestamp = m_domAgent.m_environment.executionStopwatch().elapsedTime().seconds();
        m_domAgent.m_frontendDispatcher->didFireEvent(nodeId, event.type(), timestamp, data->size() ? WTFMove(data) : nullptr);
    }

private:
    EventFiredCallback(InspectorDOMAgent& domAgent)
        : EventListener(EventListener::CPPEventListenerType)
        , m_domAgent(domAgent)
    {
    }

    InspectorDOMAgent& m_domAgent;
};

String InspectorDOMAgent::toErrorString(ExceptionCode ec)
{
    return ec ? String(DOMException::name(ec)) : emptyString();
}

String InspectorDOMAgent::toErrorString(Exception&& exception)
{
    return DOMException::name(exception.code());
}

InspectorDOMAgent::InspectorDOMAgent(PageAgentContext& context, InspectorOverlay* overlay)
    : InspectorAgentBase("DOM"_s, context)
    , m_injectedScriptManager(context.injectedScriptManager)
    , m_frontendDispatcher(makeUnique<Inspector::DOMFrontendDispatcher>(context.frontendRouter))
    , m_backendDispatcher(Inspector::DOMBackendDispatcher::create(context.backendDispatcher, this))
    , m_inspectedPage(context.inspectedPage)
    , m_overlay(overlay)
#if ENABLE(VIDEO)
    , m_mediaMetricsTimer(*this, &InspectorDOMAgent::mediaMetricsTimerFired)
#endif
{
}

InspectorDOMAgent::~InspectorDOMAgent() = default;

void InspectorDOMAgent::didCreateFrontendAndBackend(Inspector::FrontendRouter*, Inspector::BackendDispatcher*)
{
    m_history = makeUnique<InspectorHistory>();
    m_domEditor = makeUnique<DOMEditor>(*m_history);

    m_instrumentingAgents.setPersistentDOMAgent(this);
    m_document = m_inspectedPage.mainFrame().document();

#if ENABLE(VIDEO)
    if (m_document)
        addEventListenersToNode(*m_document);

    for (auto* mediaElement : HTMLMediaElement::allMediaElements())
        addEventListenersToNode(*mediaElement);
#endif
}

void InspectorDOMAgent::willDestroyFrontendAndBackend(Inspector::DisconnectReason)
{
    m_history.reset();
    m_domEditor.reset();
    m_nodeToFocus = nullptr;
    m_mousedOverNode = nullptr;
    m_inspectedNode = nullptr;

    ErrorString ignored;
    setSearchingForNode(ignored, false, nullptr, false);
    hideHighlight(ignored);

    m_instrumentingAgents.setPersistentDOMAgent(nullptr);
    m_documentRequested = false;
    reset();
}

Vector<Document*> InspectorDOMAgent::documents()
{
    Vector<Document*> result;
    for (Frame* frame = m_document->frame(); frame; frame = frame->tree().traverseNext()) {
        Document* document = frame->document();
        if (!document)
            continue;
        result.append(document);
    }
    return result;
}

void InspectorDOMAgent::reset()
{
    if (m_history)
        m_history->reset();
    m_searchResults.clear();
    discardBindings();
    if (m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask->reset();
    m_document = nullptr;
}

void InspectorDOMAgent::setDocument(Document* document)
{
    if (document == m_document.get())
        return;

    reset();

    m_document = document;

    if (!m_documentRequested)
        return;

    // Immediately communicate null document or document that has finished loading.
    if (!document || !document->parsing())
        m_frontendDispatcher->documentUpdated();
}

void InspectorDOMAgent::releaseDanglingNodes()
{
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
    int id = nodesMap->get(node);
    if (!id)
        return;

    m_idToNode.remove(id);

    if (node->isFrameOwnerElement()) {
        const HTMLFrameOwnerElement* frameOwner = static_cast<const HTMLFrameOwnerElement*>(node);
        if (Document* contentDocument = frameOwner->contentDocument())
            unbind(contentDocument, nodesMap);
    }

    if (is<Element>(*node)) {
        Element& element = downcast<Element>(*node);
        if (ShadowRoot* root = element.shadowRoot())
            unbind(root, nodesMap);
        if (PseudoElement* beforeElement = element.beforePseudoElement())
            unbind(beforeElement, nodesMap);
        if (PseudoElement* afterElement = element.afterPseudoElement())
            unbind(afterElement, nodesMap);
    }

    nodesMap->remove(node);

    if (auto* cssAgent = m_instrumentingAgents.enabledCSSAgent())
        cssAgent->didRemoveDOMNode(*node, id);

    if (m_childrenRequested.remove(id)) {
        // FIXME: Would be better to do this iteratively rather than recursively.
        for (Node* child = innerFirstChild(node); child; child = innerNextSibling(child))
            unbind(child, nodesMap);
    }
}

Node* InspectorDOMAgent::assertNode(ErrorString& errorString, int nodeId)
{
    Node* node = nodeForId(nodeId);
    if (!node) {
        errorString = "Missing node for given nodeId"_s;
        return nullptr;
    }
    return node;
}

Document* InspectorDOMAgent::assertDocument(ErrorString& errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return nullptr;
    if (!is<Document>(node)) {
        errorString = "Node for given nodeId is not a document"_s;
        return nullptr;
    }
    return downcast<Document>(node);
}

Element* InspectorDOMAgent::assertElement(ErrorString& errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return nullptr;
    if (!is<Element>(node)) {
        errorString = "Node for given nodeId is not an element"_s;
        return nullptr;
    }
    return downcast<Element>(node);
}

Node* InspectorDOMAgent::assertEditableNode(ErrorString& errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return nullptr;
    if (node->isInUserAgentShadowTree() && !m_allowEditingUserAgentShadowTrees) {
        errorString = "Node for given nodeId is in a shadow tree"_s;
        return nullptr;
    }
    if (node->isPseudoElement()) {
        errorString = "Node for given nodeId is a pseudo-element"_s;
        return nullptr;
    }
    return node;
}

Element* InspectorDOMAgent::assertEditableElement(ErrorString& errorString, int nodeId)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return nullptr;
    if (!is<Element>(node)) {
        errorString = "Node for given nodeId is not an element"_s;
        return nullptr;
    }
    return downcast<Element>(node);
}

void InspectorDOMAgent::getDocument(ErrorString& errorString, RefPtr<Inspector::Protocol::DOM::Node>& root)
{
    m_documentRequested = true;

    if (!m_document) {
        errorString = "Internal error: missing document"_s;
        return;
    }

    // Reset backend state.
    RefPtr<Document> document = m_document;
    reset();
    m_document = document;

    root = buildObjectForNode(m_document.get(), 2, &m_documentNodeToIdMap);

    if (m_nodeToFocus)
        focusNode();
}

void InspectorDOMAgent::pushChildNodesToFrontend(int nodeId, int depth)
{
    Node* node = nodeForId(nodeId);
    if (!node || (node->nodeType() != Node::ELEMENT_NODE && node->nodeType() != Node::DOCUMENT_NODE && node->nodeType() != Node::DOCUMENT_FRAGMENT_NODE))
        return;

    NodeToIdMap* nodeMap = m_idToNodesMap.get(nodeId);

    if (m_childrenRequested.contains(nodeId)) {
        if (depth <= 1)
            return;

        depth--;

        for (node = innerFirstChild(node); node; node = innerNextSibling(node)) {
            int childNodeId = nodeMap->get(node);
            ASSERT(childNodeId);
            pushChildNodesToFrontend(childNodeId, depth);
        }

        return;
    }

    auto children = buildArrayForContainerChildren(node, depth, nodeMap);
    m_frontendDispatcher->setChildNodes(nodeId, WTFMove(children));
}

void InspectorDOMAgent::discardBindings()
{
    m_documentNodeToIdMap.clear();
    m_idToNode.clear();
    m_dispatchedEvents.clear();
    m_eventListenerEntries.clear();
    releaseDanglingNodes();
    m_childrenRequested.clear();
}

int InspectorDOMAgent::pushNodeToFrontend(Node* nodeToPush)
{
    if (!nodeToPush)
        return 0;

    // FIXME: <https://webkit.org/b/213499> Web Inspector: allow DOM nodes to be instrumented at any point, regardless of whether the main document has also been instrumented

    ErrorString ignored;
    return pushNodeToFrontend(ignored, boundNodeId(&nodeToPush->document()), nodeToPush);
}

int InspectorDOMAgent::pushNodeToFrontend(ErrorString& errorString, int documentNodeId, Node* nodeToPush)
{
    Document* document = assertDocument(errorString, documentNodeId);
    if (!document)
        return 0;
    if (&nodeToPush->document() != document) {
        errorString = "nodeToPush is not part of the document with given documentNodeId"_s;
        return 0;
    }

    return pushNodePathToFrontend(errorString, nodeToPush);
}

Node* InspectorDOMAgent::nodeForId(int id)
{
    if (!m_idToNode.isValidKey(id))
        return nullptr;

    return m_idToNode.get(id);
}

void InspectorDOMAgent::requestChildNodes(ErrorString& errorString, int nodeId, const int* depth)
{
    int sanitizedDepth;

    if (!depth)
        sanitizedDepth = 1;
    else if (*depth == -1)
        sanitizedDepth = INT_MAX;
    else if (*depth > 0)
        sanitizedDepth = *depth;
    else {
        errorString = "Unexpected value below -1 for given depth"_s;
        return;
    }

    pushChildNodesToFrontend(nodeId, sanitizedDepth);
}

void InspectorDOMAgent::querySelector(ErrorString& errorString, int nodeId, const String& selectors, int* elementId)
{
    *elementId = 0;
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;
    if (!is<ContainerNode>(*node)) {
        assertElement(errorString, nodeId);
        return;
    }

    auto queryResult = downcast<ContainerNode>(*node).querySelector(selectors);
    if (queryResult.hasException()) {
        errorString = "DOM Error while querying with given selectors"_s;
        return;
    }

    if (auto* element = queryResult.releaseReturnValue())
        *elementId = pushNodePathToFrontend(errorString, element);
}

void InspectorDOMAgent::querySelectorAll(ErrorString& errorString, int nodeId, const String& selectors, RefPtr<JSON::ArrayOf<int>>& result)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;
    if (!is<ContainerNode>(*node)) {
        assertElement(errorString, nodeId);
        return;
    }

    auto queryResult = downcast<ContainerNode>(*node).querySelectorAll(selectors);
    if (queryResult.hasException()) {
        errorString = "DOM Error while querying with given selectors"_s;
        return;
    }

    auto nodes = queryResult.releaseReturnValue();
    result = JSON::ArrayOf<int>::create();
    for (unsigned i = 0; i < nodes->length(); ++i)
        result->addItem(pushNodePathToFrontend(nodes->item(i)));
}

int InspectorDOMAgent::pushNodePathToFrontend(Node* nodeToPush)
{
    ErrorString ignored;
    return pushNodePathToFrontend(ignored, nodeToPush);
}

int InspectorDOMAgent::pushNodePathToFrontend(ErrorString errorString, Node* nodeToPush)
{
    ASSERT(nodeToPush);  // Invalid input

    if (!m_document) {
        errorString = "Missing document"_s;
        return 0;
    }

    // FIXME: <https://webkit.org/b/213499> Web Inspector: allow DOM nodes to be instrumented at any point, regardless of whether the main document has also been instrumented
    if (!m_documentNodeToIdMap.contains(m_document)) {
        errorString = "Document must have been requested"_s;
        return 0;
    }

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
            auto newMap = makeUnique<NodeToIdMap>();
            danglingMap = newMap.get();
            m_danglingNodeToIdMaps.append(newMap.release());
            auto children = JSON::ArrayOf<Inspector::Protocol::DOM::Node>::create();
            children->addItem(buildObjectForNode(node, 0, danglingMap));
            m_frontendDispatcher->setChildNodes(0, WTFMove(children));
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

int InspectorDOMAgent::boundNodeId(const Node* node)
{
    return m_documentNodeToIdMap.get(const_cast<Node*>(node));
}

void InspectorDOMAgent::setAttributeValue(ErrorString& errorString, int elementId, const String& name, const String& value)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    m_domEditor->setAttribute(*element, name, value, errorString);
}

void InspectorDOMAgent::setAttributesAsText(ErrorString& errorString, int elementId, const String& text, const String* name)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    auto parsedElement = createHTMLElement(element->document(), spanTag);
    auto result = parsedElement.get().setInnerHTML("<span " + text + "></span>");
    if (result.hasException()) {
        errorString = toErrorString(result.releaseException());
        return;
    }

    Node* child = parsedElement->firstChild();
    if (!child) {
        errorString = "Could not parse given text"_s;
        return;
    }

    Element* childElement = downcast<Element>(child);
    if (!childElement->hasAttributes() && name) {
        m_domEditor->removeAttribute(*element, *name, errorString);
        return;
    }

    bool foundOriginalAttribute = false;
    for (const Attribute& attribute : childElement->attributesIterator()) {
        // Add attribute pair
        foundOriginalAttribute = foundOriginalAttribute || (name && attribute.name().toString() == *name);
        if (!m_domEditor->setAttribute(*element, attribute.name().toString(), attribute.value(), errorString))
            return;
    }

    if (!foundOriginalAttribute && name && !name->stripWhiteSpace().isEmpty())
        m_domEditor->removeAttribute(*element, *name, errorString);
}

void InspectorDOMAgent::removeAttribute(ErrorString& errorString, int elementId, const String& name)
{
    Element* element = assertEditableElement(errorString, elementId);
    if (!element)
        return;

    m_domEditor->removeAttribute(*element, name, errorString);
}

void InspectorDOMAgent::removeNode(ErrorString& errorString, int nodeId)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    ContainerNode* parentNode = node->parentNode();
    if (!parentNode) {
        errorString = "Cannot remove detached node"_s;
        return;
    }

    m_domEditor->removeChild(*parentNode, *node, errorString);
}

void InspectorDOMAgent::setNodeName(ErrorString& errorString, int nodeId, const String& tagName, int* newId)
{
    *newId = 0;

    RefPtr<Node> oldNode = nodeForId(nodeId);
    if (!is<Element>(oldNode))
        return;

    auto createElementResult = oldNode->document().createElementForBindings(tagName);
    if (createElementResult.hasException())
        return;
    auto newElement = createElementResult.releaseReturnValue();

    // Copy over the original node's attributes.
    newElement->cloneAttributesFromElement(downcast<Element>(*oldNode));

    // Copy over the original node's children.
    RefPtr<Node> child;
    while ((child = oldNode->firstChild())) {
        if (!m_domEditor->insertBefore(newElement, *child, 0, errorString))
            return;
    }

    // Replace the old node with the new node
    RefPtr<ContainerNode> parent = oldNode->parentNode();
    if (!m_domEditor->insertBefore(*parent, newElement.copyRef(), oldNode->nextSibling(), errorString))
        return;
    if (!m_domEditor->removeChild(*parent, *oldNode, errorString))
        return;

    *newId = pushNodePathToFrontend(errorString, newElement.ptr());
    if (m_childrenRequested.contains(nodeId))
        pushChildNodesToFrontend(*newId);
}

void InspectorDOMAgent::getOuterHTML(ErrorString& errorString, int nodeId, WTF::String* outerHTML)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    *outerHTML = serializeFragment(*node, SerializedNodes::SubtreeIncludingNode);
}

void InspectorDOMAgent::setOuterHTML(ErrorString& errorString, int nodeId, const String& outerHTML)
{
    if (!nodeId) {
        DOMPatchSupport { *m_domEditor, *m_document }.patchDocument(outerHTML);
        return;
    }

    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    Document& document = node->document();
    if (!document.isHTMLDocument() && !document.isXMLDocument()) {
        errorString = "Document of node for given nodeId is not HTML/XML"_s;
        return;
    }

    Node* newNode = nullptr;
    if (!m_domEditor->setOuterHTML(*node, outerHTML, newNode, errorString))
        return;

    if (!newNode) {
        // The only child node has been deleted.
        return;
    }

    int newId = pushNodePathToFrontend(errorString, newNode);

    bool childrenRequested = m_childrenRequested.contains(nodeId);
    if (childrenRequested)
        pushChildNodesToFrontend(newId);
}

void InspectorDOMAgent::insertAdjacentHTML(ErrorString& errorString, int nodeId, const String& position, const String& html)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    if (!is<Element>(node)) {
        errorString = "Node for given nodeId is not an element"_s;
        return;
    }

    m_domEditor->insertAdjacentHTML(downcast<Element>(*node), position, html, errorString);
}

void InspectorDOMAgent::setNodeValue(ErrorString& errorString, int nodeId, const String& value)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    if (!is<Text>(*node)) {
        errorString = "Node for given nodeId is not text"_s;
        return;
    }

    m_domEditor->replaceWholeText(downcast<Text>(*node), value, errorString);
}

void InspectorDOMAgent::getSupportedEventNames(ErrorString&, RefPtr<JSON::ArrayOf<String>>& eventNames)
{
    eventNames = JSON::ArrayOf<String>::create();

#define DOM_EVENT_NAMES_ADD(name) eventNames->addItem(#name);
    DOM_EVENT_NAMES_FOR_EACH(DOM_EVENT_NAMES_ADD)
#undef DOM_EVENT_NAMES_ADD
}

#if ENABLE(INSPECTOR_ALTERNATE_DISPATCHERS)
void InspectorDOMAgent::getDataBindingsForNode(ErrorString& errorString, int /* nodeId */, RefPtr<JSON::ArrayOf<Inspector::Protocol::DOM::DataBinding>>& /* dataBindings */)
{
    errorString = "Not supported"_s;
}

void InspectorDOMAgent::getAssociatedDataForNode(ErrorString& errorString, int /* nodeId */, Optional<String>& /* associatedData */)
{
    errorString = "Not supported"_s;
}
#endif

void InspectorDOMAgent::getEventListenersForNode(ErrorString& errorString, int nodeId, RefPtr<JSON::ArrayOf<Inspector::Protocol::DOM::EventListener>>& listenersArray)
{
    listenersArray = JSON::ArrayOf<Inspector::Protocol::DOM::EventListener>::create();

    auto* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    Vector<RefPtr<EventTarget>> ancestors;
    ancestors.append(node);
    for (auto* ancestor = node->parentOrShadowHostNode(); ancestor; ancestor = ancestor->parentOrShadowHostNode())
        ancestors.append(ancestor);
    if (auto* window = node->document().domWindow())
        ancestors.append(window);

    struct EventListenerInfo {
        RefPtr<EventTarget> eventTarget;
        const AtomString eventType;
        const EventListenerVector eventListeners;
    };

    Vector<EventListenerInfo> eventInformation;
    for (size_t i = ancestors.size(); i; --i) {
        auto& ancestor = ancestors[i - 1];
        for (auto& eventType : ancestor->eventTypes()) {
            EventListenerVector filteredListeners;
            for (auto& listener : ancestor->eventListeners(eventType)) {
                if (listener->callback().type() == EventListener::JSEventListenerType)
                    filteredListeners.append(listener);
            }
            if (!filteredListeners.isEmpty())
                eventInformation.append({ ancestor, eventType, WTFMove(filteredListeners) });
        }
    }

    auto addListener = [&] (RegisteredEventListener& listener, const EventListenerInfo& info) {
        int identifier = 0;
        bool disabled = false;
        bool hasBreakpoint = false;

        for (auto& inspectorEventListener : m_eventListenerEntries.values()) {
            if (inspectorEventListener.matches(*info.eventTarget, info.eventType, listener.callback(), listener.useCapture())) {
                identifier = inspectorEventListener.identifier;
                disabled = inspectorEventListener.disabled;
                hasBreakpoint = inspectorEventListener.hasBreakpoint;
                break;
            }
        }

        if (!identifier) {
            InspectorEventListener inspectorEventListener(m_lastEventListenerId++, *info.eventTarget, info.eventType, listener.callback(), listener.useCapture());

            identifier = inspectorEventListener.identifier;
            disabled = inspectorEventListener.disabled;
            hasBreakpoint = inspectorEventListener.hasBreakpoint;

            m_eventListenerEntries.add(identifier, inspectorEventListener);
        }

        listenersArray->addItem(buildObjectForEventListener(listener, identifier, *info.eventTarget, info.eventType, disabled, hasBreakpoint));
    };

    // Get Capturing Listeners (in this order)
    size_t eventInformationLength = eventInformation.size();
    for (auto& info : eventInformation) {
        for (auto& listener : info.eventListeners) {
            if (listener->useCapture())
                addListener(*listener, info);
        }
    }

    // Get Bubbling Listeners (reverse order)
    for (size_t i = eventInformationLength; i; --i) {
        const EventListenerInfo& info = eventInformation[i - 1];
        for (auto& listener : info.eventListeners) {
            if (!listener->useCapture())
                addListener(*listener, info);
        }
    }

    if (m_inspectedNode == node)
        m_suppressEventListenerChangedEvent = false;
}

void InspectorDOMAgent::setEventListenerDisabled(ErrorString& errorString, int eventListenerId, bool disabled)
{
    auto it = m_eventListenerEntries.find(eventListenerId);
    if (it == m_eventListenerEntries.end()) {
        errorString = "Missing event listener for given eventListenerId"_s;
        return;
    }

    it->value.disabled = disabled;
}

void InspectorDOMAgent::setBreakpointForEventListener(ErrorString& errorString, int eventListenerId)
{
    auto it = m_eventListenerEntries.find(eventListenerId);
    if (it == m_eventListenerEntries.end()) {
        errorString = "Missing event listener for given eventListenerId"_s;
        return;
    }

    it->value.hasBreakpoint = true;
}

void InspectorDOMAgent::removeBreakpointForEventListener(ErrorString& errorString, int eventListenerId)
{
    auto it = m_eventListenerEntries.find(eventListenerId);
    if (it == m_eventListenerEntries.end()) {
        errorString = "Missing event listener for given eventListenerId"_s;
        return;
    }

    it->value.hasBreakpoint = false;
}

void InspectorDOMAgent::getAccessibilityPropertiesForNode(ErrorString& errorString, int nodeId, RefPtr<Inspector::Protocol::DOM::AccessibilityProperties>& axProperties)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    axProperties = buildObjectForAccessibilityProperties(node);
}

void InspectorDOMAgent::performSearch(ErrorString& errorString, const String& query, const JSON::Array* nodeIds, const bool* caseSensitive, String* searchId, int* resultCount)
{
    // FIXME: Search works with node granularity - number of matches within node is not calculated.
    InspectorNodeFinder finder(query, caseSensitive && *caseSensitive);

    if (nodeIds) {
        for (auto& nodeValue : *nodeIds) {
            if (!nodeValue) {
                errorString = "Unexpected non-integer item in given nodeIds"_s;
                return;
            }
            int nodeId = 0;
            if (!nodeValue->asInteger(nodeId)) {
                errorString = "Unexpected non-integer item in given nodeIds"_s;
                return;
            }
            Node* node = assertNode(errorString, nodeId);
            if (!node) {
                // assertNode should have filled the errorString for us.
                ASSERT(errorString.length());
                return;
            }
            finder.performSearch(node);
        }
    } else {
        // There's no need to iterate the frames tree because
        // the search helper will go inside the frame owner elements.
        finder.performSearch(m_document.get());
    }

    *searchId = IdentifiersFactory::createIdentifier();

    auto& resultsVector = m_searchResults.add(*searchId, Vector<RefPtr<Node>>()).iterator->value;
    for (auto& result : finder.results())
        resultsVector.append(result);

    *resultCount = resultsVector.size();
}

void InspectorDOMAgent::getSearchResults(ErrorString& errorString, const String& searchId, int fromIndex, int toIndex, RefPtr<JSON::ArrayOf<int>>& nodeIds)
{
    SearchResults::iterator it = m_searchResults.find(searchId);
    if (it == m_searchResults.end()) {
        errorString = "Missing search result for given searchId"_s;
        return;
    }

    int size = it->value.size();
    if (fromIndex < 0 || toIndex > size || fromIndex >= toIndex) {
        errorString = "Invalid search result range for given fromIndex and toIndex"_s;
        return;
    }

    nodeIds = JSON::ArrayOf<int>::create();
    for (int i = fromIndex; i < toIndex; ++i)
        nodeIds->addItem(pushNodePathToFrontend((it->value)[i].get()));
}

void InspectorDOMAgent::discardSearchResults(ErrorString&, const String& searchId)
{
    m_searchResults.remove(searchId);
}

bool InspectorDOMAgent::handleMousePress()
{
    if (!m_searchingForNode)
        return false;

    if (Node* node = m_overlay->highlightedNode()) {
        inspect(node);
        return true;
    }
    return false;
}

bool InspectorDOMAgent::handleTouchEvent(Node& node)
{
    if (!m_searchingForNode)
        return false;
    if (m_inspectModeHighlightConfig) {
        m_overlay->highlightNode(&node, *m_inspectModeHighlightConfig);
        inspect(&node);
        return true;
    }
    return false;
}

void InspectorDOMAgent::inspect(Node* inspectedNode)
{
    ErrorString ignored;
    RefPtr<Node> node = inspectedNode;
    setSearchingForNode(ignored, false, nullptr, false);

    if (!node->isElementNode() && !node->isDocumentNode())
        node = node->parentNode();
    m_nodeToFocus = node;

    if (!m_nodeToFocus)
        return;

    focusNode();
}

void InspectorDOMAgent::focusNode()
{
    // FIXME: <https://webkit.org/b/213499> Web Inspector: allow DOM nodes to be instrumented at any point, regardless of whether the main document has also been instrumented
    if (!m_documentRequested)
        return;

    ASSERT(m_nodeToFocus);

    RefPtr<Node> node = m_nodeToFocus.get();
    m_nodeToFocus = nullptr;

    Frame* frame = node->document().frame();
    if (!frame)
        return;

    JSC::JSGlobalObject* scriptState = mainWorldExecState(frame);
    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptFor(scriptState);
    if (injectedScript.hasNoValue())
        return;

    injectedScript.inspectObject(nodeAsScriptValue(*scriptState, node.get()));
}

void InspectorDOMAgent::mouseDidMoveOverElement(const HitTestResult& result, unsigned)
{
    m_mousedOverNode = result.innerNode();

    if (!m_searchingForNode)
        return;

    highlightMousedOverNode();
}

void InspectorDOMAgent::highlightMousedOverNode()
{
    Node* node = m_mousedOverNode.get();
    if (node && node->isTextNode())
        node = node->parentNode();
    if (node && m_inspectModeHighlightConfig)
        m_overlay->highlightNode(node, *m_inspectModeHighlightConfig);
}

void InspectorDOMAgent::setSearchingForNode(ErrorString& errorString, bool enabled, const JSON::Object* highlightInspectorObject, bool showRulers)
{
    if (m_searchingForNode == enabled)
        return;

    m_searchingForNode = enabled;

    m_overlay->setShowRulersDuringElementSelection(m_searchingForNode && showRulers);

    if (m_searchingForNode) {
        m_inspectModeHighlightConfig = highlightConfigFromInspectorObject(errorString, highlightInspectorObject);
        if (!m_inspectModeHighlightConfig)
            return;
        highlightMousedOverNode();
    } else
        hideHighlight(errorString);

    m_overlay->didSetSearchingForNode(m_searchingForNode);

    if (InspectorClient* client = m_inspectedPage.inspectorController().inspectorClient())
        client->elementSelectionChanged(m_searchingForNode);
}

std::unique_ptr<HighlightConfig> InspectorDOMAgent::highlightConfigFromInspectorObject(ErrorString& errorString, const JSON::Object* highlightInspectorObject)
{
    if (!highlightInspectorObject) {
        errorString = "Internal error: highlight configuration parameter is missing"_s;
        return nullptr;
    }

    auto highlightConfig = makeUnique<HighlightConfig>();
    bool showInfo = false; // Default: false (do not show a tooltip).
    highlightInspectorObject->getBoolean("showInfo", showInfo);
    highlightConfig->showInfo = showInfo;
    highlightConfig->content = parseConfigColor("contentColor", highlightInspectorObject);
    highlightConfig->contentOutline = parseConfigColor("contentOutlineColor", highlightInspectorObject);
    highlightConfig->padding = parseConfigColor("paddingColor", highlightInspectorObject);
    highlightConfig->border = parseConfigColor("borderColor", highlightInspectorObject);
    highlightConfig->margin = parseConfigColor("marginColor", highlightInspectorObject);
    return highlightConfig;
}

#if PLATFORM(IOS_FAMILY)
void InspectorDOMAgent::setInspectModeEnabled(ErrorString& errorString, bool enabled, const JSON::Object* highlightConfig)
{
    setSearchingForNode(errorString, enabled, highlightConfig ? highlightConfig : nullptr, false);
}
#else
void InspectorDOMAgent::setInspectModeEnabled(ErrorString& errorString, bool enabled, const JSON::Object* highlightConfig, const bool* showRulers)
{
    setSearchingForNode(errorString, enabled, highlightConfig ? highlightConfig : nullptr, showRulers && *showRulers);
}
#endif

void InspectorDOMAgent::highlightRect(ErrorString&, int x, int y, int width, int height, const JSON::Object* color, const JSON::Object* outlineColor, const bool* usePageCoordinates)
{
    auto quad = makeUnique<FloatQuad>(FloatRect(x, y, width, height));
    innerHighlightQuad(WTFMove(quad), color, outlineColor, usePageCoordinates);
}

void InspectorDOMAgent::highlightQuad(ErrorString& errorString, const JSON::Array& quadArray, const JSON::Object* color, const JSON::Object* outlineColor, const bool* usePageCoordinates)
{
    auto quad = makeUnique<FloatQuad>();
    if (!parseQuad(quadArray, quad.get())) {
        errorString = "Unexpected invalid quadArray"_s;
        return;
    }
    innerHighlightQuad(WTFMove(quad), color, outlineColor, usePageCoordinates);
}

void InspectorDOMAgent::innerHighlightQuad(std::unique_ptr<FloatQuad> quad, const JSON::Object* color, const JSON::Object* outlineColor, const bool* usePageCoordinates)
{
    auto highlightConfig = makeUnique<HighlightConfig>();
    highlightConfig->content = parseColor(color);
    highlightConfig->contentOutline = parseColor(outlineColor);
    highlightConfig->usePageCoordinates = usePageCoordinates ? *usePageCoordinates : false;
    m_overlay->highlightQuad(WTFMove(quad), *highlightConfig);
}

void InspectorDOMAgent::highlightSelector(ErrorString& errorString, const JSON::Object& highlightInspectorObject, const String& selectorString, const String* frameId)
{
    auto highlightConfig = highlightConfigFromInspectorObject(errorString, &highlightInspectorObject);
    if (!highlightConfig)
        return;

    RefPtr<Document> document;

    if (frameId) {
        auto* pageAgent = m_instrumentingAgents.enabledPageAgent();
        if (!pageAgent) {
            errorString = "Page domain must be enabled"_s;
            return;
        }

        auto* frame = pageAgent->assertFrame(errorString, *frameId);
        if (!frame)
            return;

        document = frame->document();
    } else
        document = m_document;

    if (!document) {
        errorString = "Missing document of frame for given frameId"_s;
        return;
    }

    CSSParser parser(*document);
    CSSSelectorList selectorList;
    parser.parseSelector(selectorString, selectorList);

    SelectorChecker selectorChecker(*document);

    Vector<Ref<Node>> nodeList;
    HashSet<Node*> seenNodes;

    for (auto& descendant : composedTreeDescendants(*document)) {
        if (!is<Element>(descendant))
            continue;

        auto& descendantElement = downcast<Element>(descendant);

        auto isInUserAgentShadowTree = descendantElement.isInUserAgentShadowTree();
        auto pseudoId = descendantElement.pseudoId();
        auto& pseudo = descendantElement.pseudo();

        for (const auto* selector = selectorList.first(); selector; selector = CSSSelectorList::next(selector)) {
            if (isInUserAgentShadowTree && (selector->match() != CSSSelector::PseudoElement || selector->value() != pseudo))
                continue;

            SelectorChecker::CheckingContext context(SelectorChecker::Mode::ResolvingStyle);
            context.pseudoId = pseudoId;

            if (selectorChecker.match(*selector, descendantElement, context)) {
                if (seenNodes.add(&descendantElement))
                    nodeList.append(descendantElement);
            }

            if (context.pseudoIDSet) {
                auto pseudoIDs = PseudoIdSet::fromMask(context.pseudoIDSet.data());

                if (pseudoIDs.has(PseudoId::Before)) {
                    pseudoIDs.remove(PseudoId::Before);
                    if (auto* beforePseudoElement = descendantElement.beforePseudoElement()) {
                        if (seenNodes.add(beforePseudoElement))
                            nodeList.append(*beforePseudoElement);
                    }
                }

                if (pseudoIDs.has(PseudoId::After)) {
                    pseudoIDs.remove(PseudoId::After);
                    if (auto* afterPseudoElement = descendantElement.afterPseudoElement()) {
                        if (seenNodes.add(afterPseudoElement))
                            nodeList.append(*afterPseudoElement);
                    }
                }

                if (pseudoIDs) {
                    if (seenNodes.add(&descendantElement))
                        nodeList.append(descendantElement);
                }
            }
        }
    }

    m_overlay->highlightNodeList(StaticNodeList::create(WTFMove(nodeList)), *highlightConfig);
}

void InspectorDOMAgent::highlightNode(ErrorString& errorString, const JSON::Object& highlightInspectorObject, const int* nodeId, const String* objectId)
{
    Node* node = nullptr;
    if (nodeId)
        node = assertNode(errorString, *nodeId);
    else if (objectId) {
        node = nodeForObjectId(*objectId);
        if (!node)
            errorString = "Missing node for given objectId"_s;
    } else
        errorString = "Either nodeId or objectId must be specified"_s;

    if (!node)
        return;

    std::unique_ptr<HighlightConfig> highlightConfig = highlightConfigFromInspectorObject(errorString, &highlightInspectorObject);
    if (!highlightConfig)
        return;

    m_overlay->highlightNode(node, *highlightConfig);
}

void InspectorDOMAgent::highlightNodeList(ErrorString& errorString, const JSON::Array& nodeIds, const JSON::Object& highlightInspectorObject)
{
    Vector<Ref<Node>> nodes;
    for (auto& nodeValue : nodeIds) {
        if (!nodeValue) {
            errorString = "Unexpected non-integer item in given nodeIds"_s;
            return;
        }

        int nodeId = 0;
        if (!nodeValue->asInteger(nodeId)) {
            errorString = "Unexpected non-integer item in given nodeIds"_s;
            return;
        }

        // In the case that a node is removed in the time between when highlightNodeList is invoked
        // by the frontend and it is executed by the backend, we should still attempt to highlight
        // as many nodes as possible. As such, we should ignore any errors generated when attempting
        // to get a Node from a given nodeId. 
        ErrorString ignored;
        Node* node = assertNode(ignored, nodeId);
        if (!node)
            continue;

        nodes.append(*node);
    }

    std::unique_ptr<HighlightConfig> highlightConfig = highlightConfigFromInspectorObject(errorString, &highlightInspectorObject);
    if (!highlightConfig)
        return;

    m_overlay->highlightNodeList(StaticNodeList::create(WTFMove(nodes)), *highlightConfig);
}

void InspectorDOMAgent::highlightFrame(ErrorString& errorString, const String& frameId, const JSON::Object* color, const JSON::Object* outlineColor)
{
    auto* pageAgent = m_instrumentingAgents.enabledPageAgent();
    if (!pageAgent) {
        errorString = "Page domain must be enabled"_s;
        return;
    }

    auto* frame = pageAgent->assertFrame(errorString, frameId);
    if (!frame)
        return;

    if (frame->ownerElement()) {
        auto highlightConfig = makeUnique<HighlightConfig>();
        highlightConfig->showInfo = true; // Always show tooltips for frames.
        highlightConfig->content = parseColor(color);
        highlightConfig->contentOutline = parseColor(outlineColor);
        m_overlay->highlightNode(frame->ownerElement(), *highlightConfig);
    }
}

void InspectorDOMAgent::hideHighlight(ErrorString&)
{
    m_overlay->hideHighlight();
}

void InspectorDOMAgent::moveTo(ErrorString& errorString, int nodeId, int targetElementId, const int* anchorNodeId, int* newNodeId)
{
    Node* node = assertEditableNode(errorString, nodeId);
    if (!node)
        return;

    Element* targetElement = assertEditableElement(errorString, targetElementId);
    if (!targetElement)
        return;

    Node* anchorNode = 0;
    if (anchorNodeId && *anchorNodeId) {
        anchorNode = assertEditableNode(errorString, *anchorNodeId);
        if (!anchorNode)
            return;
        if (anchorNode->parentNode() != targetElement) {
            errorString = "Given anchorNodeId must be a child of given targetElementId"_s;
            return;
        }
    }

    if (!m_domEditor->insertBefore(*targetElement, *node, anchorNode, errorString))
        return;

    *newNodeId = pushNodePathToFrontend(errorString, node);
}

void InspectorDOMAgent::undo(ErrorString& errorString)
{
    auto result = m_history->undo();
    if (result.hasException())
        errorString = toErrorString(result.releaseException());
}

void InspectorDOMAgent::redo(ErrorString& errorString)
{
    auto result = m_history->redo();
    if (result.hasException())
        errorString = toErrorString(result.releaseException());
}

void InspectorDOMAgent::markUndoableState(ErrorString&)
{
    m_history->markUndoableState();
}

void InspectorDOMAgent::focus(ErrorString& errorString, int nodeId)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return;
    if (!element->isFocusable()) {
        errorString = "Element for given nodeId is not focusable"_s;
        return;
    }
    element->focus();
}

void InspectorDOMAgent::setInspectedNode(ErrorString& errorString, int nodeId)
{
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;

    if (node->isInUserAgentShadowTree() && !m_allowEditingUserAgentShadowTrees) {
        errorString = "Node for given nodeId is in a shadow tree"_s;
        return;
    }

    m_inspectedNode = node;

    if (auto& commandLineAPIHost = static_cast<WebInjectedScriptManager&>(m_injectedScriptManager).commandLineAPIHost())
        commandLineAPIHost->addInspectedObject(makeUnique<InspectableNode>(node));

    m_suppressEventListenerChangedEvent = false;
}

void InspectorDOMAgent::resolveNode(ErrorString& errorString, int nodeId, const String* objectGroup, RefPtr<Inspector::Protocol::Runtime::RemoteObject>& result)
{
    String objectGroupName = objectGroup ? *objectGroup : emptyString();
    Node* node = assertNode(errorString, nodeId);
    if (!node)
        return;
    RefPtr<Inspector::Protocol::Runtime::RemoteObject> object = resolveNode(node, objectGroupName);
    if (!object) {
        errorString = "Missing injected script for given nodeId"_s;
        return;
    }
    result = object;
}

void InspectorDOMAgent::getAttributes(ErrorString& errorString, int nodeId, RefPtr<JSON::ArrayOf<String>>& result)
{
    Element* element = assertElement(errorString, nodeId);
    if (!element)
        return;

    result = buildArrayForElementAttributes(element);
}

void InspectorDOMAgent::requestNode(ErrorString& errorString, const String& objectId, int* nodeId)
{
    Node* node = nodeForObjectId(objectId);
    if (node)
        *nodeId = pushNodePathToFrontend(errorString, node);
    else
        *nodeId = 0;
}

String InspectorDOMAgent::documentURLString(Document* document)
{
    if (!document || document->url().isNull())
        return emptyString();
    return document->url().string();
}

static String documentBaseURLString(Document* document)
{
    return document->completeURL(emptyString()).string();
}

static bool pseudoElementType(PseudoId pseudoId, Inspector::Protocol::DOM::PseudoType* type)
{
    switch (pseudoId) {
    case PseudoId::Before:
        *type = Inspector::Protocol::DOM::PseudoType::Before;
        return true;
    case PseudoId::After:
        *type = Inspector::Protocol::DOM::PseudoType::After;
        return true;
    default:
        return false;
    }
}

static Inspector::Protocol::DOM::ShadowRootType shadowRootType(ShadowRootMode mode)
{
    switch (mode) {
    case ShadowRootMode::UserAgent:
        return Inspector::Protocol::DOM::ShadowRootType::UserAgent;
    case ShadowRootMode::Closed:
        return Inspector::Protocol::DOM::ShadowRootType::Closed;
    case ShadowRootMode::Open:
        return Inspector::Protocol::DOM::ShadowRootType::Open;
    }

    ASSERT_NOT_REACHED();
    return Inspector::Protocol::DOM::ShadowRootType::UserAgent;
}

static Inspector::Protocol::DOM::CustomElementState customElementState(const Element& element)
{
    if (element.isDefinedCustomElement())
        return Inspector::Protocol::DOM::CustomElementState::Custom;
    if (element.isFailedCustomElement())
        return Inspector::Protocol::DOM::CustomElementState::Failed;
    if (element.isUndefinedCustomElement() || element.isCustomElementUpgradeCandidate())
        return Inspector::Protocol::DOM::CustomElementState::Waiting;
    return Inspector::Protocol::DOM::CustomElementState::Builtin;
}

static String computeContentSecurityPolicySHA256Hash(const Element& element)
{
    // FIXME: Compute the digest with respect to the raw bytes received from the page.
    // See <https://bugs.webkit.org/show_bug.cgi?id=155184>.
    TextEncoding documentEncoding = element.document().textEncoding();
    const TextEncoding& encodingToUse = documentEncoding.isValid() ? documentEncoding : UTF8Encoding();
    auto content = encodingToUse.encode(TextNodeTraversal::contentsAsString(element), UnencodableHandling::Entities);
    auto cryptoDigest = PAL::CryptoDigest::create(PAL::CryptoDigest::Algorithm::SHA_256);
    cryptoDigest->addBytes(content.data(), content.size());
    auto digest = cryptoDigest->computeHash();
    return makeString("sha256-", base64Encode(digest.data(), digest.size()));
}

Ref<Inspector::Protocol::DOM::Node> InspectorDOMAgent::buildObjectForNode(Node* node, int depth, NodeToIdMap* nodesMap)
{
    int id = bind(node, nodesMap);
    String nodeName;
    String localName;
    String nodeValue;

    switch (node->nodeType()) {
    case Node::PROCESSING_INSTRUCTION_NODE:
        nodeName = node->nodeName();
        localName = node->localName();
        FALLTHROUGH;
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
    case Node::DOCUMENT_NODE:
    case Node::ELEMENT_NODE:
    default:
        nodeName = node->nodeName();
        localName = node->localName();
        break;
    }

    auto value = Inspector::Protocol::DOM::Node::create()
        .setNodeId(id)
        .setNodeType(static_cast<int>(node->nodeType()))
        .setNodeName(nodeName)
        .setLocalName(localName)
        .setNodeValue(nodeValue)
        .release();

    if (node->isContainerNode()) {
        int nodeCount = innerChildNodeCount(node);
        value->setChildNodeCount(nodeCount);
        Ref<JSON::ArrayOf<Inspector::Protocol::DOM::Node>> children = buildArrayForContainerChildren(node, depth, nodesMap);
        if (children->length() > 0)
            value->setChildren(WTFMove(children));
    }

    auto* pageAgent = m_instrumentingAgents.enabledPageAgent();
    if (pageAgent) {
        if (auto* frameView = node->document().view())
            value->setFrameId(pageAgent->frameId(&frameView->frame()));
    }

    if (is<Element>(*node)) {
        Element& element = downcast<Element>(*node);
        value->setAttributes(buildArrayForElementAttributes(&element));
        if (is<HTMLFrameOwnerElement>(element)) {
            if (auto* document = downcast<HTMLFrameOwnerElement>(element).contentDocument())
                value->setContentDocument(buildObjectForNode(document, 0, nodesMap));
        }

        if (ShadowRoot* root = element.shadowRoot()) {
            auto shadowRoots = JSON::ArrayOf<Inspector::Protocol::DOM::Node>::create();
            shadowRoots->addItem(buildObjectForNode(root, 0, nodesMap));
            value->setShadowRoots(WTFMove(shadowRoots));
        }

        if (is<HTMLTemplateElement>(element))
            value->setTemplateContent(buildObjectForNode(&downcast<HTMLTemplateElement>(element).content(), 0, nodesMap));

        if (is<HTMLStyleElement>(element) || (is<HTMLScriptElement>(element) && !element.hasAttributeWithoutSynchronization(HTMLNames::srcAttr)))
            value->setContentSecurityPolicyHash(computeContentSecurityPolicySHA256Hash(element));

        auto state = customElementState(element);
        if (state != Inspector::Protocol::DOM::CustomElementState::Builtin)
            value->setCustomElementState(state);

        if (element.pseudoId() != PseudoId::None) {
            Inspector::Protocol::DOM::PseudoType pseudoType;
            if (pseudoElementType(element.pseudoId(), &pseudoType))
                value->setPseudoType(pseudoType);
        } else {
            if (auto pseudoElements = buildArrayForPseudoElements(element, nodesMap))
                value->setPseudoElements(WTFMove(pseudoElements));
        }
    } else if (is<Document>(*node)) {
        Document& document = downcast<Document>(*node);
        if (pageAgent)
            value->setFrameId(pageAgent->frameId(document.frame()));
        value->setDocumentURL(documentURLString(&document));
        value->setBaseURL(documentBaseURLString(&document));
        value->setXmlVersion(document.xmlVersion());
    } else if (is<DocumentType>(*node)) {
        DocumentType& docType = downcast<DocumentType>(*node);
        value->setPublicId(docType.publicId());
        value->setSystemId(docType.systemId());
    } else if (is<Attr>(*node)) {
        Attr& attribute = downcast<Attr>(*node);
        value->setName(attribute.name());
        value->setValue(attribute.value());
    } else if (is<ShadowRoot>(*node)) {
        ShadowRoot& shadowRoot = downcast<ShadowRoot>(*node);
        value->setShadowRootType(shadowRootType(shadowRoot.mode()));
    }

    return value;
}

Ref<JSON::ArrayOf<String>> InspectorDOMAgent::buildArrayForElementAttributes(Element* element)
{
    auto attributesValue = JSON::ArrayOf<String>::create();
    // Go through all attributes and serialize them.
    if (!element->hasAttributes())
        return attributesValue;
    for (const Attribute& attribute : element->attributesIterator()) {
        // Add attribute pair
        attributesValue->addItem(attribute.name().toString());
        attributesValue->addItem(attribute.value());
    }
    return attributesValue;
}

Ref<JSON::ArrayOf<Inspector::Protocol::DOM::Node>> InspectorDOMAgent::buildArrayForContainerChildren(Node* container, int depth, NodeToIdMap* nodesMap)
{
    auto children = JSON::ArrayOf<Inspector::Protocol::DOM::Node>::create();
    if (depth == 0) {
        // Special-case the only text child - pretend that container's children have been requested.
        Node* firstChild = container->firstChild();
        if (firstChild && firstChild->nodeType() == Node::TEXT_NODE && !firstChild->nextSibling()) {
            children->addItem(buildObjectForNode(firstChild, 0, nodesMap));
            m_childrenRequested.add(bind(container, nodesMap));
        }
        return children;
    }

    Node* child = innerFirstChild(container);
    depth--;
    m_childrenRequested.add(bind(container, nodesMap));

    while (child) {
        children->addItem(buildObjectForNode(child, depth, nodesMap));
        child = innerNextSibling(child);
    }
    return children;
}

RefPtr<JSON::ArrayOf<Inspector::Protocol::DOM::Node>> InspectorDOMAgent::buildArrayForPseudoElements(const Element& element, NodeToIdMap* nodesMap)
{
    PseudoElement* beforeElement = element.beforePseudoElement();
    PseudoElement* afterElement = element.afterPseudoElement();
    if (!beforeElement && !afterElement)
        return nullptr;

    auto pseudoElements = JSON::ArrayOf<Inspector::Protocol::DOM::Node>::create();
    if (beforeElement)
        pseudoElements->addItem(buildObjectForNode(beforeElement, 0, nodesMap));
    if (afterElement)
        pseudoElements->addItem(buildObjectForNode(afterElement, 0, nodesMap));
    return pseudoElements;
}

Ref<Inspector::Protocol::DOM::EventListener> InspectorDOMAgent::buildObjectForEventListener(const RegisteredEventListener& registeredEventListener, int identifier, EventTarget& eventTarget, const AtomString& eventType, bool disabled, bool hasBreakpoint)
{
    Ref<EventListener> eventListener = registeredEventListener.callback();

    String handlerName;
    int lineNumber = 0;
    int columnNumber = 0;
    String scriptID;
    if (is<JSEventListener>(eventListener.get())) {
        auto& scriptListener = downcast<JSEventListener>(eventListener.get());

        Document* document = nullptr;
        if (auto* scriptExecutionContext = eventTarget.scriptExecutionContext()) {
            if (is<Document>(scriptExecutionContext))
                document = downcast<Document>(scriptExecutionContext);
        } else if (is<Node>(eventTarget))
            document = &downcast<Node>(eventTarget).document();

        JSC::JSObject* handlerObject = nullptr;
        JSC::JSGlobalObject* exec = nullptr;

        JSC::JSLockHolder lock(scriptListener.isolatedWorld().vm());

        if (document) {
            handlerObject = scriptListener.ensureJSFunction(*document);
            exec = execStateFromNode(scriptListener.isolatedWorld(), document);
        }

        if (handlerObject && exec) {
            JSC::VM& vm = exec->vm();
            JSC::JSFunction* handlerFunction = JSC::jsDynamicCast<JSC::JSFunction*>(vm, handlerObject);

            if (!handlerFunction) {
                auto scope = DECLARE_CATCH_SCOPE(vm);

                // If the handler is not actually a function, see if it implements the EventListener interface and use that.
                auto handleEventValue = handlerObject->get(exec, JSC::Identifier::fromString(vm, "handleEvent"));

                if (UNLIKELY(scope.exception()))
                    scope.clearException();

                if (handleEventValue)
                    handlerFunction = JSC::jsDynamicCast<JSC::JSFunction*>(vm, handleEventValue);
            }

            if (handlerFunction && !handlerFunction->isHostOrBuiltinFunction()) {
                // If the listener implements the EventListener interface, use the class name instead of
                // "handleEvent", unless it is a plain object.
                if (handlerFunction != handlerObject)
                    handlerName = JSC::JSObject::calculatedClassName(handlerObject);
                if (handlerName.isEmpty() || handlerName == "Object"_s)
                    handlerName = handlerFunction->calculatedDisplayName(vm);

                if (auto executable = handlerFunction->jsExecutable()) {
                    lineNumber = executable->firstLine() - 1;
                    columnNumber = executable->startColumn() - 1;
                    scriptID = executable->sourceID() == JSC::SourceProvider::nullID ? emptyString() : String::number(executable->sourceID());
                }
            }
        }
    }

    auto value = Inspector::Protocol::DOM::EventListener::create()
        .setEventListenerId(identifier)
        .setType(eventType)
        .setUseCapture(registeredEventListener.useCapture())
        .setIsAttribute(eventListener->isAttribute())
        .release();
    if (is<Node>(eventTarget))
        value->setNodeId(pushNodePathToFrontend(&downcast<Node>(eventTarget)));
    else if (is<DOMWindow>(eventTarget))
        value->setOnWindow(true);
    if (!scriptID.isNull()) {
        auto location = Inspector::Protocol::Debugger::Location::create()
            .setScriptId(scriptID)
            .setLineNumber(lineNumber)
            .release();
        location->setColumnNumber(columnNumber);
        value->setLocation(WTFMove(location));
    }
    if (!handlerName.isEmpty())
        value->setHandlerName(handlerName);
    if (registeredEventListener.isPassive())
        value->setPassive(true);
    if (registeredEventListener.isOnce())
        value->setOnce(true);
    if (disabled)
        value->setDisabled(disabled);
    if (hasBreakpoint)
        value->setHasBreakpoint(hasBreakpoint);
    return value;
}

void InspectorDOMAgent::processAccessibilityChildren(AXCoreObject& axObject, JSON::ArrayOf<int>& childNodeIds)
{
    const auto& children = axObject.children();
    if (!children.size())
        return;

    for (const auto& childObject : children) {
        if (Node* childNode = childObject->node())
            childNodeIds.addItem(pushNodePathToFrontend(childNode));
        else
            processAccessibilityChildren(*childObject, childNodeIds);
    }
}
    
RefPtr<Inspector::Protocol::DOM::AccessibilityProperties> InspectorDOMAgent::buildObjectForAccessibilityProperties(Node* node)
{
    ASSERT(node);
    if (!node)
        return nullptr;

    if (!WebCore::AXObjectCache::accessibilityEnabled())
        WebCore::AXObjectCache::enableAccessibility();

    Node* activeDescendantNode = nullptr;
    bool busy = false;
    auto checked = Inspector::Protocol::DOM::AccessibilityProperties::Checked::False;
    RefPtr<JSON::ArrayOf<int>> childNodeIds;
    RefPtr<JSON::ArrayOf<int>> controlledNodeIds;
    auto currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::False;
    bool exists = false;
    bool expanded = false;
    bool disabled = false;
    RefPtr<JSON::ArrayOf<int>> flowedNodeIds;
    bool focused = false;
    bool ignored = true;
    bool ignoredByDefault = false;
    auto invalid = Inspector::Protocol::DOM::AccessibilityProperties::Invalid::False;
    bool hidden = false;
    String label;
    bool liveRegionAtomic = false;
    RefPtr<JSON::ArrayOf<String>> liveRegionRelevant;
    auto liveRegionStatus = Inspector::Protocol::DOM::AccessibilityProperties::LiveRegionStatus::Off;
    Node* mouseEventNode = nullptr;
    RefPtr<JSON::ArrayOf<int>> ownedNodeIds;
    Node* parentNode = nullptr;
    bool pressed = false;
    bool readonly = false;
    bool required = false;
    String role;
    bool selected = false;
    RefPtr<JSON::ArrayOf<int>> selectedChildNodeIds;
    bool supportsChecked = false;
    bool supportsExpanded = false;
    bool supportsLiveRegion = false;
    bool supportsPressed = false;
    bool supportsRequired = false;
    bool supportsFocused = false;
    bool isPopupButton = false;
    int headingLevel = 0;
    unsigned hierarchicalLevel = 0;
    unsigned level = 0;

    if (AXObjectCache* axObjectCache = node->document().axObjectCache()) {
        if (AXCoreObject* axObject = axObjectCache->getOrCreate(node)) {

            if (AXCoreObject* activeDescendant = axObject->activeDescendant())
                activeDescendantNode = activeDescendant->node();

            // An AX object is "busy" if it or any ancestor has aria-busy="true" set.
            AXCoreObject* current = axObject;
            while (!busy && current) {
                busy = current->isBusy();
                current = current->parentObject();
            }

            supportsChecked = axObject->supportsChecked();
            if (supportsChecked) {
                AccessibilityButtonState checkValue = axObject->checkboxOrRadioValue(); // Element using aria-checked.
                if (checkValue == AccessibilityButtonState::On)
                    checked = Inspector::Protocol::DOM::AccessibilityProperties::Checked::True;
                else if (checkValue == AccessibilityButtonState::Mixed)
                    checked = Inspector::Protocol::DOM::AccessibilityProperties::Checked::Mixed;
                else if (axObject->isChecked()) // Native checkbox.
                    checked = Inspector::Protocol::DOM::AccessibilityProperties::Checked::True;
            }
            
            if (!axObject->children().isEmpty()) {
                childNodeIds = JSON::ArrayOf<int>::create();
                processAccessibilityChildren(*axObject, *childNodeIds);
            }
            
            Vector<Element*> controlledElements;
            axObject->elementsFromAttribute(controlledElements, aria_controlsAttr);
            if (controlledElements.size()) {
                controlledNodeIds = JSON::ArrayOf<int>::create();
                for (Element* controlledElement : controlledElements)
                    controlledNodeIds->addItem(pushNodePathToFrontend(controlledElement));
            }
            
            switch (axObject->currentState()) {
            case AccessibilityCurrentState::False:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::False;
                break;
            case AccessibilityCurrentState::Page:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::Page;
                break;
            case AccessibilityCurrentState::Step:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::Step;
                break;
            case AccessibilityCurrentState::Location:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::Location;
                break;
            case AccessibilityCurrentState::Date:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::Date;
                break;
            case AccessibilityCurrentState::Time:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::Time;
                break;
            case AccessibilityCurrentState::True:
                currentState = Inspector::Protocol::DOM::AccessibilityProperties::Current::True;
                break;
            }

            disabled = !axObject->isEnabled();
            exists = true;
            
            supportsExpanded = axObject->supportsExpanded();
            if (supportsExpanded)
                expanded = axObject->isExpanded();

            Vector<Element*> flowedElements;
            axObject->elementsFromAttribute(flowedElements, aria_flowtoAttr);
            if (flowedElements.size()) {
                flowedNodeIds = JSON::ArrayOf<int>::create();
                for (Element* flowedElement : flowedElements)
                    flowedNodeIds->addItem(pushNodePathToFrontend(flowedElement));
            }
            
            if (is<Element>(*node)) {
                supportsFocused = axObject->canSetFocusAttribute();
                if (supportsFocused)
                    focused = axObject->isFocused();
            }

            ignored = axObject->accessibilityIsIgnored();
            ignoredByDefault = axObject->accessibilityIsIgnoredByDefault();
            
            String invalidValue = axObject->invalidStatus();
            if (invalidValue == "false")
                invalid = Inspector::Protocol::DOM::AccessibilityProperties::Invalid::False;
            else if (invalidValue == "grammar")
                invalid = Inspector::Protocol::DOM::AccessibilityProperties::Invalid::Grammar;
            else if (invalidValue == "spelling")
                invalid = Inspector::Protocol::DOM::AccessibilityProperties::Invalid::Spelling;
            else // Future versions of ARIA may allow additional truthy values. Ex. format, order, or size.
                invalid = Inspector::Protocol::DOM::AccessibilityProperties::Invalid::True;
            
            if (axObject->isAXHidden() || axObject->isDOMHidden())
                hidden = true;
            
            label = axObject->computedLabel();

            if (axObject->supportsLiveRegion()) {
                supportsLiveRegion = true;
                liveRegionAtomic = axObject->liveRegionAtomic();

                String ariaRelevantAttrValue = axObject->liveRegionRelevant();
                if (!ariaRelevantAttrValue.isEmpty()) {
                    // FIXME: Pass enum values rather than strings once unblocked. http://webkit.org/b/133711
                    String ariaRelevantAdditions = Inspector::Protocol::InspectorHelpers::getEnumConstantValue(Inspector::Protocol::DOM::LiveRegionRelevant::Additions);
                    String ariaRelevantRemovals = Inspector::Protocol::InspectorHelpers::getEnumConstantValue(Inspector::Protocol::DOM::LiveRegionRelevant::Removals);
                    String ariaRelevantText = Inspector::Protocol::InspectorHelpers::getEnumConstantValue(Inspector::Protocol::DOM::LiveRegionRelevant::Text);
                    liveRegionRelevant = JSON::ArrayOf<String>::create();
                    const SpaceSplitString& values = SpaceSplitString(ariaRelevantAttrValue, true);
                    // @aria-relevant="all" is exposed as ["additions","removals","text"], in order.
                    // This order is controlled in WebCore and expected in WebInspectorUI.
                    if (values.contains("all")) {
                        liveRegionRelevant->addItem(ariaRelevantAdditions);
                        liveRegionRelevant->addItem(ariaRelevantRemovals);
                        liveRegionRelevant->addItem(ariaRelevantText);
                    } else {
                        if (values.contains(ariaRelevantAdditions))
                            liveRegionRelevant->addItem(ariaRelevantAdditions);
                        if (values.contains(ariaRelevantRemovals))
                            liveRegionRelevant->addItem(ariaRelevantRemovals);
                        if (values.contains(ariaRelevantText))
                            liveRegionRelevant->addItem(ariaRelevantText);
                    }
                }

                String ariaLive = axObject->liveRegionStatus();
                if (ariaLive == "assertive")
                    liveRegionStatus = Inspector::Protocol::DOM::AccessibilityProperties::LiveRegionStatus::Assertive;
                else if (ariaLive == "polite")
                    liveRegionStatus = Inspector::Protocol::DOM::AccessibilityProperties::LiveRegionStatus::Polite;
            }

            if (is<AccessibilityNodeObject>(*axObject))
                mouseEventNode = downcast<AccessibilityNodeObject>(*axObject).mouseButtonListener(MouseButtonListenerResultFilter::IncludeBodyElement);

            if (axObject->supportsARIAOwns()) {
                Vector<Element*> ownedElements;
                axObject->elementsFromAttribute(ownedElements, aria_ownsAttr);
                if (ownedElements.size()) {
                    ownedNodeIds = JSON::ArrayOf<int>::create();
                    for (Element* ownedElement : ownedElements)
                        ownedNodeIds->addItem(pushNodePathToFrontend(ownedElement));
                }
            }

            if (AXCoreObject* parentObject = axObject->parentObjectUnignored())
                parentNode = parentObject->node();

            supportsPressed = axObject->pressedIsPresent();
            if (supportsPressed)
                pressed = axObject->isPressed();
            
            if (axObject->isTextControl())
                readonly = !axObject->canSetValueAttribute();

            supportsRequired = axObject->supportsRequiredAttribute();
            if (supportsRequired)
                required = axObject->isRequired();
            
            role = axObject->computedRoleString();
            selected = axObject->isSelected();

            AXCoreObject::AccessibilityChildrenVector selectedChildren;
            axObject->selectedChildren(selectedChildren);
            if (selectedChildren.size()) {
                selectedChildNodeIds = JSON::ArrayOf<int>::create();
                for (auto& selectedChildObject : selectedChildren) {
                    if (Node* selectedChildNode = selectedChildObject->node())
                        selectedChildNodeIds->addItem(pushNodePathToFrontend(selectedChildNode));
                }
            }
            
            headingLevel = axObject->headingLevel();
            hierarchicalLevel = axObject->hierarchicalLevel();
            
            level = hierarchicalLevel ? hierarchicalLevel : headingLevel;
            isPopupButton = axObject->isPopUpButton() || axObject->hasPopup();
        }
    }
    
    Ref<Inspector::Protocol::DOM::AccessibilityProperties> value = Inspector::Protocol::DOM::AccessibilityProperties::create()
        .setExists(exists)
        .setLabel(label)
        .setRole(role)
        .setNodeId(pushNodePathToFrontend(node))
        .release();

    if (exists) {
        if (activeDescendantNode)
            value->setActiveDescendantNodeId(pushNodePathToFrontend(activeDescendantNode));
        if (busy)
            value->setBusy(busy);
        if (supportsChecked)
            value->setChecked(checked);
        if (childNodeIds)
            value->setChildNodeIds(childNodeIds);
        if (controlledNodeIds)
            value->setControlledNodeIds(controlledNodeIds);
        if (currentState != Inspector::Protocol::DOM::AccessibilityProperties::Current::False)
            value->setCurrent(currentState);
        if (disabled)
            value->setDisabled(disabled);
        if (supportsExpanded)
            value->setExpanded(expanded);
        if (flowedNodeIds)
            value->setFlowedNodeIds(flowedNodeIds);
        if (supportsFocused)
            value->setFocused(focused);
        if (ignored)
            value->setIgnored(ignored);
        if (ignoredByDefault)
            value->setIgnoredByDefault(ignoredByDefault);
        if (invalid != Inspector::Protocol::DOM::AccessibilityProperties::Invalid::False)
            value->setInvalid(invalid);
        if (hidden)
            value->setHidden(hidden);
        if (supportsLiveRegion) {
            value->setLiveRegionAtomic(liveRegionAtomic);
            if (liveRegionRelevant->length())
                value->setLiveRegionRelevant(liveRegionRelevant);
            value->setLiveRegionStatus(liveRegionStatus);
        }
        if (mouseEventNode)
            value->setMouseEventNodeId(pushNodePathToFrontend(mouseEventNode));
        if (ownedNodeIds)
            value->setOwnedNodeIds(ownedNodeIds);
        if (parentNode)
            value->setParentNodeId(pushNodePathToFrontend(parentNode));
        if (supportsPressed)
            value->setPressed(pressed);
        if (readonly)
            value->setReadonly(readonly);
        if (supportsRequired)
            value->setRequired(required);
        if (selected)
            value->setSelected(selected);
        if (selectedChildNodeIds)
            value->setSelectedChildNodeIds(selectedChildNodeIds);
        
        // H1 -- H6 always have a headingLevel property that can be complimented by a hierarchicalLevel
        // property when aria-level is set on the element, in which case we want to remain calling
        // this value the "Heading Level" in the inspector.
        // Also, we do not want it to say Hierarchy Level: 0
        if (headingLevel)
            value->setHeadingLevel(level);
        else if (level)
            value->setHierarchyLevel(level);
        if (isPopupButton)
            value->setIsPopUpButton(isPopupButton);
    }

    return value;
}

static bool containsOnlyHTMLWhitespace(Node* node)
{
    // FIXME: Respect ignoreWhitespace setting from inspector front end?
    return is<Text>(node) && downcast<Text>(*node).data().isAllSpecialCharacters<isHTMLSpace>();
}

Node* InspectorDOMAgent::innerFirstChild(Node* node)
{
    node = node->firstChild();
    while (containsOnlyHTMLWhitespace(node))
        node = node->nextSibling();
    return node;
}

Node* InspectorDOMAgent::innerNextSibling(Node* node)
{
    do {
        node = node->nextSibling();
    } while (containsOnlyHTMLWhitespace(node));
    return node;
}

Node* InspectorDOMAgent::innerPreviousSibling(Node* node)
{
    do {
        node = node->previousSibling();
    } while (containsOnlyHTMLWhitespace(node));
    return node;
}

unsigned InspectorDOMAgent::innerChildNodeCount(Node* node)
{
    unsigned count = 0;
    for (Node* child = innerFirstChild(node); child; child = innerNextSibling(child))
        ++count;
    return count;
}

Node* InspectorDOMAgent::innerParentNode(Node* node)
{
    ASSERT(node);
    if (is<Document>(*node))
        return downcast<Document>(*node).ownerElement();
    if (is<ShadowRoot>(*node))
        return downcast<ShadowRoot>(*node).host();
    return node->parentNode();
}

void InspectorDOMAgent::didCommitLoad(Document* document)
{
    if (m_nodeToFocus && &m_nodeToFocus->document() == document)
        m_nodeToFocus = nullptr;

    if (m_mousedOverNode && &m_mousedOverNode->document() == document)
        m_mousedOverNode = nullptr;

    if (m_inspectedNode && &m_inspectedNode->document() == document)
        m_inspectedNode = nullptr;

    RefPtr<Element> frameOwner = document->ownerElement();
    if (!frameOwner)
        return;

    int frameOwnerId = m_documentNodeToIdMap.get(frameOwner);
    if (!frameOwnerId)
        return;

    // Re-add frame owner element together with its new children.
    int parentId = m_documentNodeToIdMap.get(innerParentNode(frameOwner.get()));
    m_frontendDispatcher->childNodeRemoved(parentId, frameOwnerId);
    unbind(frameOwner.get(), &m_documentNodeToIdMap);

    Ref<Inspector::Protocol::DOM::Node> value = buildObjectForNode(frameOwner.get(), 0, &m_documentNodeToIdMap);
    Node* previousSibling = innerPreviousSibling(frameOwner.get());
    int prevId = previousSibling ? m_documentNodeToIdMap.get(previousSibling) : 0;
    m_frontendDispatcher->childNodeInserted(parentId, prevId, WTFMove(value));
}

int InspectorDOMAgent::identifierForNode(Node& node)
{
    return pushNodePathToFrontend(&node);
}

void InspectorDOMAgent::addEventListenersToNode(Node& node)
{
#if ENABLE(VIDEO)
    auto callback = EventFiredCallback::create(*this);

    auto createEventListener = [&] (const AtomString& eventName) {
        node.addEventListener(eventName, callback.copyRef(), false);
    };

#if ENABLE(FULLSCREEN_API)
    if (is<Document>(node) || is<HTMLMediaElement>(node))
        createEventListener(eventNames().webkitfullscreenchangeEvent);
#endif // ENABLE(FULLSCREEN_API)

    if (is<HTMLMediaElement>(node)) {
        createEventListener(eventNames().abortEvent);
        createEventListener(eventNames().canplayEvent);
        createEventListener(eventNames().canplaythroughEvent);
        createEventListener(eventNames().emptiedEvent);
        createEventListener(eventNames().endedEvent);
        createEventListener(eventNames().loadeddataEvent);
        createEventListener(eventNames().loadedmetadataEvent);
        createEventListener(eventNames().loadstartEvent);
        createEventListener(eventNames().pauseEvent);
        createEventListener(eventNames().playEvent);
        createEventListener(eventNames().playingEvent);
        createEventListener(eventNames().seekedEvent);
        createEventListener(eventNames().seekingEvent);
        createEventListener(eventNames().stalledEvent);
        createEventListener(eventNames().suspendEvent);
        createEventListener(eventNames().waitingEvent);

        if (!m_mediaMetricsTimer.isActive())
            m_mediaMetricsTimer.start(0_s, 1_s / 15.);
    }
#else
    UNUSED_PARAM(node);
#endif // ENABLE(VIDEO)
}

void InspectorDOMAgent::didInsertDOMNode(Node& node)
{
    if (containsOnlyHTMLWhitespace(&node))
        return;

    // We could be attaching existing subtree. Forget the bindings.
    unbind(&node, &m_documentNodeToIdMap);

    ContainerNode* parent = node.parentNode();
    if (!parent)
        return;

    int parentId = m_documentNodeToIdMap.get(parent);
    // Return if parent is not mapped yet.
    if (!parentId)
        return;

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        m_frontendDispatcher->childNodeCountUpdated(parentId, innerChildNodeCount(parent));
    } else {
        // Children have been requested -> return value of a new child.
        Node* prevSibling = innerPreviousSibling(&node);
        int prevId = prevSibling ? m_documentNodeToIdMap.get(prevSibling) : 0;
        Ref<Inspector::Protocol::DOM::Node> value = buildObjectForNode(&node, 0, &m_documentNodeToIdMap);
        m_frontendDispatcher->childNodeInserted(parentId, prevId, WTFMove(value));
    }
}

void InspectorDOMAgent::didRemoveDOMNode(Node& node)
{
    if (containsOnlyHTMLWhitespace(&node))
        return;

    ContainerNode* parent = node.parentNode();

    // If parent is not mapped yet -> ignore the event.
    if (!m_documentNodeToIdMap.contains(parent))
        return;

    int parentId = m_documentNodeToIdMap.get(parent);

    if (!m_childrenRequested.contains(parentId)) {
        // No children are mapped yet -> only notify on changes of hasChildren.
        if (innerChildNodeCount(parent) == 1)
            m_frontendDispatcher->childNodeCountUpdated(parentId, 0);
    } else
        m_frontendDispatcher->childNodeRemoved(parentId, m_documentNodeToIdMap.get(&node));
    unbind(&node, &m_documentNodeToIdMap);
}

void InspectorDOMAgent::willModifyDOMAttr(Element&, const AtomString& oldValue, const AtomString& newValue)
{
    m_suppressAttributeModifiedEvent = (oldValue == newValue);
}

void InspectorDOMAgent::didModifyDOMAttr(Element& element, const AtomString& name, const AtomString& value)
{
    bool shouldSuppressEvent = m_suppressAttributeModifiedEvent;
    m_suppressAttributeModifiedEvent = false;
    if (shouldSuppressEvent)
        return;

    int id = boundNodeId(&element);
    if (!id)
        return;

    if (auto* cssAgent = m_instrumentingAgents.enabledCSSAgent())
        cssAgent->didModifyDOMAttr(element);

    m_frontendDispatcher->attributeModified(id, name, value);
}

void InspectorDOMAgent::didRemoveDOMAttr(Element& element, const AtomString& name)
{
    int id = boundNodeId(&element);
    if (!id)
        return;

    if (auto* cssAgent = m_instrumentingAgents.enabledCSSAgent())
        cssAgent->didModifyDOMAttr(element);

    m_frontendDispatcher->attributeRemoved(id, name);
}

void InspectorDOMAgent::styleAttributeInvalidated(const Vector<Element*>& elements)
{
    auto nodeIds = JSON::ArrayOf<int>::create();
    for (auto& element : elements) {
        int id = boundNodeId(element);
        if (!id)
            continue;

        if (auto* cssAgent = m_instrumentingAgents.enabledCSSAgent())
            cssAgent->didModifyDOMAttr(*element);

        nodeIds->addItem(id);
    }
    m_frontendDispatcher->inlineStyleInvalidated(WTFMove(nodeIds));
}

void InspectorDOMAgent::characterDataModified(CharacterData& characterData)
{
    int id = m_documentNodeToIdMap.get(&characterData);
    if (!id) {
        // Push text node if it is being created.
        didInsertDOMNode(characterData);
        return;
    }
    m_frontendDispatcher->characterDataModified(id, characterData.data());
}

void InspectorDOMAgent::didInvalidateStyleAttr(Element& element)
{
    int id = m_documentNodeToIdMap.get(&element);
    if (!id)
        return;

    if (!m_revalidateStyleAttrTask)
        m_revalidateStyleAttrTask = makeUnique<RevalidateStyleAttributeTask>(this);
    m_revalidateStyleAttrTask->scheduleFor(&element);
}

void InspectorDOMAgent::didPushShadowRoot(Element& host, ShadowRoot& root)
{
    int hostId = m_documentNodeToIdMap.get(&host);
    if (hostId)
        m_frontendDispatcher->shadowRootPushed(hostId, buildObjectForNode(&root, 0, &m_documentNodeToIdMap));
}

void InspectorDOMAgent::willPopShadowRoot(Element& host, ShadowRoot& root)
{
    int hostId = m_documentNodeToIdMap.get(&host);
    int rootId = m_documentNodeToIdMap.get(&root);
    if (hostId && rootId)
        m_frontendDispatcher->shadowRootPopped(hostId, rootId);
}

void InspectorDOMAgent::didChangeCustomElementState(Element& element)
{
    int elementId = m_documentNodeToIdMap.get(&element);
    if (!elementId)
        return;

    m_frontendDispatcher->customElementStateChanged(elementId, customElementState(element));
}

void InspectorDOMAgent::frameDocumentUpdated(Frame& frame)
{
    Document* document = frame.document();
    if (!document)
        return;

    if (!frame.isMainFrame())
        return;

    // Only update the main frame document, nested frame document updates are not required
    // (will be handled by didCommitLoad()).
    setDocument(document);
}

void InspectorDOMAgent::pseudoElementCreated(PseudoElement& pseudoElement)
{
    Element* parent = pseudoElement.hostElement();
    if (!parent)
        return;

    int parentId = m_documentNodeToIdMap.get(parent);
    if (!parentId)
        return;

    pushChildNodesToFrontend(parentId, 1);
    m_frontendDispatcher->pseudoElementAdded(parentId, buildObjectForNode(&pseudoElement, 0, &m_documentNodeToIdMap));
}

void InspectorDOMAgent::pseudoElementDestroyed(PseudoElement& pseudoElement)
{
    int pseudoElementId = m_documentNodeToIdMap.get(&pseudoElement);
    if (!pseudoElementId)
        return;

    // If a PseudoElement is bound, its parent element must have been bound.
    Element* parent = pseudoElement.hostElement();
    ASSERT(parent);
    int parentId = m_documentNodeToIdMap.get(parent);
    ASSERT(parentId);

    unbind(&pseudoElement, &m_documentNodeToIdMap);
    m_frontendDispatcher->pseudoElementRemoved(parentId, pseudoElementId);
}

void InspectorDOMAgent::didAddEventListener(EventTarget& target)
{
    if (!is<Node>(target))
        return;

    auto& node = downcast<Node>(target);
    if (!node.contains(m_inspectedNode.get()))
        return;

    int nodeId = boundNodeId(&node);
    if (!nodeId)
        return;

    if (m_suppressEventListenerChangedEvent)
        return;

    m_suppressEventListenerChangedEvent = true;

    m_frontendDispatcher->didAddEventListener(nodeId);
}

void InspectorDOMAgent::willRemoveEventListener(EventTarget& target, const AtomString& eventType, EventListener& listener, bool capture)
{
    if (!is<Node>(target))
        return;

    auto& node = downcast<Node>(target);
    if (!node.contains(m_inspectedNode.get()))
        return;

    int nodeId = boundNodeId(&node);
    if (!nodeId)
        return;

    bool listenerExists = false;
    for (auto& item : node.eventListeners(eventType)) {
        if (item->callback() == listener && item->useCapture() == capture) {
            listenerExists = true;
            break;
        }
    }

    if (!listenerExists)
        return;

    m_eventListenerEntries.removeIf([&] (auto& entry) {
        return entry.value.matches(target, eventType, listener, capture);
    });

    if (m_suppressEventListenerChangedEvent)
        return;

    m_suppressEventListenerChangedEvent = true;

    m_frontendDispatcher->willRemoveEventListener(nodeId);
}

bool InspectorDOMAgent::isEventListenerDisabled(EventTarget& target, const AtomString& eventType, EventListener& listener, bool capture)
{
    for (auto& inspectorEventListener : m_eventListenerEntries.values()) {
        if (inspectorEventListener.matches(target, eventType, listener, capture))
            return inspectorEventListener.disabled;
    }
    return false;
}

void InspectorDOMAgent::eventDidResetAfterDispatch(const Event& event)
{
    m_dispatchedEvents.remove(&event);
}

bool InspectorDOMAgent::hasBreakpointForEventListener(EventTarget& target, const AtomString& eventType, EventListener& listener, bool capture)
{
    for (auto& inspectorEventListener : m_eventListenerEntries.values()) {
        if (inspectorEventListener.matches(target, eventType, listener, capture))
            return inspectorEventListener.hasBreakpoint;
    }
    return false;
}

int InspectorDOMAgent::idForEventListener(EventTarget& target, const AtomString& eventType, EventListener& listener, bool capture)
{
    for (auto& inspectorEventListener : m_eventListenerEntries.values()) {
        if (inspectorEventListener.matches(target, eventType, listener, capture))
            return inspectorEventListener.identifier;
    }
    return 0;
}

#if ENABLE(VIDEO)
void InspectorDOMAgent::mediaMetricsTimerFired()
{
    // FIXME: remove metrics information for any media element when it's destroyed

    if (HTMLMediaElement::allMediaElements().isEmpty()) {
        if (m_mediaMetricsTimer.isActive())
            m_mediaMetricsTimer.stop();
        m_mediaMetrics.clear();
        return;
    }

    for (auto* mediaElement : HTMLMediaElement::allMediaElements()) {
        if (!is<HTMLVideoElement>(mediaElement) || !mediaElement->isPlaying())
            continue;

        auto videoPlaybackQuality = mediaElement->getVideoPlaybackQuality();
        unsigned displayCompositedVideoFrames = videoPlaybackQuality->displayCompositedVideoFrames();

        auto iterator = m_mediaMetrics.find(mediaElement);
        if (iterator == m_mediaMetrics.end()) {
            m_mediaMetrics.set(mediaElement, MediaMetrics(displayCompositedVideoFrames));
            continue;
        }

        bool isPowerEfficient = (displayCompositedVideoFrames - iterator->value.displayCompositedFrames) > 0;
        if (iterator->value.isPowerEfficient != isPowerEfficient) {
            iterator->value.isPowerEfficient = isPowerEfficient;

            int nodeId = pushNodePathToFrontend(mediaElement);
            if (nodeId) {
                auto timestamp = m_environment.executionStopwatch().elapsedTime().seconds();
                m_frontendDispatcher->powerEfficientPlaybackStateChanged(nodeId, timestamp, iterator->value.isPowerEfficient);
            }
        }

        iterator->value.displayCompositedFrames = displayCompositedVideoFrames;
    }

    m_mediaMetrics.removeIf([&] (auto& entry) {
        return !HTMLMediaElement::allMediaElements().contains(entry.key);
    });
}
#endif

Node* InspectorDOMAgent::nodeForPath(const String& path)
{
    // The path is of form "1,HTML,2,BODY,1,DIV"
    if (!m_document)
        return nullptr;

    Node* node = m_document.get();
    Vector<String> pathTokens = path.split(',');
    if (!pathTokens.size())
        return nullptr;

    for (size_t i = 0; i < pathTokens.size() - 1; i += 2) {
        bool success = true;
        unsigned childNumber = pathTokens[i].toUInt(&success);
        if (!success)
            return nullptr;

        Node* child;
        if (is<HTMLFrameOwnerElement>(*node)) {
            ASSERT(!childNumber);
            auto& frameOwner = downcast<HTMLFrameOwnerElement>(*node);
            child = frameOwner.contentDocument();
        } else {
            if (childNumber >= innerChildNodeCount(node))
                return nullptr;

            child = innerFirstChild(node);
            for (size_t j = 0; child && j < childNumber; ++j)
                child = innerNextSibling(child);
        }

        const auto& childName = pathTokens[i + 1];
        if (!child || child->nodeName() != childName)
            return nullptr;
        node = child;
    }
    return node;
}

Node* InspectorDOMAgent::nodeForObjectId(const String& objectId)
{
    InjectedScript injectedScript = m_injectedScriptManager.injectedScriptForObjectId(objectId);
    if (injectedScript.hasNoValue())
        return nullptr;

    return scriptValueAsNode(injectedScript.findObjectById(objectId));
}

void InspectorDOMAgent::pushNodeByPathToFrontend(ErrorString& errorString, const String& path, int* nodeId)
{
    if (Node* node = nodeForPath(path))
        *nodeId = pushNodePathToFrontend(errorString, node);
    else
        errorString = "Missing node for given path"_s;
}

RefPtr<Inspector::Protocol::Runtime::RemoteObject> InspectorDOMAgent::resolveNode(Node* node, const String& objectGroup)
{
    Document* document = &node->document();
    if (auto* templateHost = document->templateDocumentHost())
        document = templateHost;
    auto* frame =  document->frame();
    if (!frame)
        return nullptr;

    auto& state = *mainWorldExecState(frame);
    auto injectedScript = m_injectedScriptManager.injectedScriptFor(&state);
    if (injectedScript.hasNoValue())
        return nullptr;

    return injectedScript.wrapObject(nodeAsScriptValue(state, node), objectGroup);
}

Node* InspectorDOMAgent::scriptValueAsNode(JSC::JSValue value)
{
    if (!value || !value.isObject())
        return nullptr;
    return JSNode::toWrapped(value.getObject()->vm(), value.getObject());
}

JSC::JSValue InspectorDOMAgent::nodeAsScriptValue(JSC::JSGlobalObject& state, Node* node)
{
    JSC::JSLockHolder lock(&state);
    return toJS(&state, deprecatedGlobalObjectForPrototype(&state), BindingSecurity::checkSecurityForNode(state, node));
}

void InspectorDOMAgent::setAllowEditingUserAgentShadowTrees(ErrorString&, bool allow)
{
    m_allowEditingUserAgentShadowTrees = allow;
}

} // namespace WebCore
