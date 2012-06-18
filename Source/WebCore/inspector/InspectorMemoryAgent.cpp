/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
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

#if ENABLE(INSPECTOR)

#include "InspectorMemoryAgent.h"

#include "CharacterData.h"
#include "DOMWrapperVisitor.h"
#include "Document.h"
#include "EventListenerMap.h"
#include "Frame.h"
#include "InspectorFrontend.h"
#include "InspectorState.h"
#include "InspectorValues.h"
#include "InstrumentingAgents.h"
#include "MemoryCache.h"
#include "MemoryUsageSupport.h"
#include "Node.h"
#include "Page.h"
#include "ScriptGCEvent.h"
#include "ScriptProfiler.h"
#include "StyledElement.h"
#include <wtf/HashSet.h>
#include <wtf/text/StringBuilder.h>

using WebCore::TypeBuilder::Memory::DOMGroup;
using WebCore::TypeBuilder::Memory::ListenerCount;
using WebCore::TypeBuilder::Memory::NodeCount;
using WebCore::TypeBuilder::Memory::StringStatistics;

// Use a type alias instead of 'using' here which would cause a conflict on Mac.
typedef WebCore::TypeBuilder::Memory::MemoryBlock InspectorMemoryBlock;

namespace WebCore {

namespace MemoryBlockName {
static const char jsHeapAllocated[] = "JSHeapAllocated";
static const char jsHeapUsed[] = "JSHeapUsed";
static const char inspectorData[] = "InspectorData";
static const char memoryCache[] = "MemoryCache";
static const char processPrivateMemory[] = "ProcessPrivateMemory";

static const char cachedImages[] = "CachedImages";
static const char cachedCssStyleSheets[] = "CachedCssStyleSheets";
static const char cachedScripts[] = "CachedScripts";
static const char cachedXslStyleSheets[] = "CachedXslStyleSheets";
static const char cachedFonts[] = "CachedFonts";
static const char renderTreeUsed[] = "RenderTreeUsed";
static const char renderTreeAllocated[] = "RenderTreeAllocated";
}

namespace {

String nodeName(Node* node)
{
    if (node->document()->isXHTMLDocument())
         return node->nodeName();
    return node->nodeName().lower();
}

int stringSize(StringImpl* string)
{
    int size = string->length();
    if (!string->is8Bit())
        size *= 2;
    return size + sizeof(*string);
}

typedef HashSet<StringImpl*, PtrHash<StringImpl*> > StringImplIdentitySet;

class CharacterDataStatistics {
    WTF_MAKE_NONCOPYABLE(CharacterDataStatistics);
public:
    CharacterDataStatistics() : m_characterDataSize(0) { }

    void collectCharacterData(Node* node)
    {
        if (!node->isCharacterDataNode())
            return;

        CharacterData* characterData = static_cast<CharacterData*>(node);
        StringImpl* dataImpl = characterData->dataImpl();
        if (m_domStringImplSet.contains(dataImpl))
            return;
        m_domStringImplSet.add(dataImpl);

        m_characterDataSize += stringSize(dataImpl);
    }

    bool contains(StringImpl* s) { return m_domStringImplSet.contains(s); }

    int characterDataSize() { return m_characterDataSize; }

private:
    StringImplIdentitySet m_domStringImplSet;
    int m_characterDataSize;
};

class DOMTreeStatistics {
    WTF_MAKE_NONCOPYABLE(DOMTreeStatistics);
public:
    DOMTreeStatistics(Node* rootNode, CharacterDataStatistics& characterDataStatistics)
        : m_totalNodeCount(0)
        , m_characterDataStatistics(characterDataStatistics)
    {
        collectTreeStatistics(rootNode);
    }

    int totalNodeCount() { return m_totalNodeCount; }

    PassRefPtr<TypeBuilder::Array<TypeBuilder::Memory::NodeCount> > nodeCount()
    {
        RefPtr<TypeBuilder::Array<TypeBuilder::Memory::NodeCount> > childrenStats = TypeBuilder::Array<TypeBuilder::Memory::NodeCount>::create();
        for (HashMap<String, int>::iterator it = m_nodeNameToCount.begin(); it != m_nodeNameToCount.end(); ++it) {
            RefPtr<NodeCount> nodeCount = NodeCount::create().setNodeName(it->first)
                                                             .setCount(it->second);
            childrenStats->addItem(nodeCount);
        }
        return childrenStats.release();
    }

    PassRefPtr<TypeBuilder::Array<TypeBuilder::Memory::ListenerCount> > listenerCount()
    {
        RefPtr<TypeBuilder::Array<TypeBuilder::Memory::ListenerCount> > listenerStats = TypeBuilder::Array<TypeBuilder::Memory::ListenerCount>::create();
        for (HashMap<AtomicString, int>::iterator it = m_eventTypeToCount.begin(); it != m_eventTypeToCount.end(); ++it) {
            RefPtr<ListenerCount> listenerCount = ListenerCount::create().setType(it->first)
                                                                         .setCount(it->second);
            listenerStats->addItem(listenerCount);
        }
        return listenerStats.release();
    }

private:
    void collectTreeStatistics(Node* rootNode)
    {
        Node* currentNode = rootNode;
        collectListenersInfo(rootNode);
        while ((currentNode = currentNode->traverseNextNode(rootNode))) {
            ++m_totalNodeCount;
            collectNodeStatistics(currentNode);
        }
    }
    void collectNodeStatistics(Node* node)
    {
        m_characterDataStatistics.collectCharacterData(node);
        collectNodeNameInfo(node);
        collectListenersInfo(node);
    }

    void collectCharacterData(Node*)
    {
    }

    void collectNodeNameInfo(Node* node)
    {
        String name = nodeName(node);
        int currentCount = m_nodeNameToCount.get(name);
        m_nodeNameToCount.set(name, currentCount + 1);
    }

    void collectListenersInfo(Node* node)
    {
        EventTargetData* d = node->eventTargetData();
        if (!d)
            return;
        EventListenerMap& eventListenerMap = d->eventListenerMap;
        if (eventListenerMap.isEmpty())
            return;
        Vector<AtomicString> eventNames = eventListenerMap.eventTypes();
        for (Vector<AtomicString>::iterator it = eventNames.begin(); it != eventNames.end(); ++it) {
            AtomicString name = *it;
            EventListenerVector* listeners = eventListenerMap.find(name);
            int count = 0;
            for (EventListenerVector::iterator j = listeners->begin(); j != listeners->end(); ++j) {
                if (j->listener->type() == EventListener::JSEventListenerType)
                    ++count;
            }
            if (count)
                m_eventTypeToCount.set(name, m_eventTypeToCount.get(name) + count);
        }
    }

    int m_totalNodeCount;
    HashMap<AtomicString, int> m_eventTypeToCount;
    HashMap<String, int> m_nodeNameToCount;
    CharacterDataStatistics& m_characterDataStatistics;
};

class CounterVisitor : public DOMWrapperVisitor {
public:
    CounterVisitor(Page* page)
        : m_page(page)
        , m_domGroups(TypeBuilder::Array<TypeBuilder::Memory::DOMGroup>::create())
        , m_jsExternalStringSize(0)
        , m_sharedStringSize(0) { }

    TypeBuilder::Array<TypeBuilder::Memory::DOMGroup>* domGroups() { return m_domGroups.get(); }

    PassRefPtr<StringStatistics> strings()
    {
        RefPtr<StringStatistics> stringStatistics = StringStatistics::create()
            .setDom(m_characterDataStatistics.characterDataSize())
            .setJs(m_jsExternalStringSize)
            .setShared(m_sharedStringSize);
        return stringStatistics.release();
    }

    virtual void visitNode(Node* node)
    {
        if (node->document()->frame() && m_page != node->document()->frame()->page())
            return;

        Node* rootNode = node;
        while (rootNode->parentNode())
            rootNode = rootNode->parentNode();

        if (m_roots.contains(rootNode))
            return;
        m_roots.add(rootNode);

        DOMTreeStatistics domTreeStats(rootNode, m_characterDataStatistics);

        RefPtr<DOMGroup> domGroup = DOMGroup::create()
            .setSize(domTreeStats.totalNodeCount())
            .setTitle(rootNode->nodeType() == Node::ELEMENT_NODE ? elementTitle(static_cast<Element*>(rootNode)) : rootNode->nodeName())
            .setNodeCount(domTreeStats.nodeCount())
            .setListenerCount(domTreeStats.listenerCount());
        if (rootNode->nodeType() == Node::DOCUMENT_NODE)
            domGroup->setDocumentURI(static_cast<Document*>(rootNode)->documentURI());

        m_domGroups->addItem(domGroup);
    }

    virtual void visitJSExternalString(StringImpl* string)
    {
        int size = stringSize(string);
        m_jsExternalStringSize += size;
        if (m_characterDataStatistics.contains(string))
            m_sharedStringSize += size;
    }

private:
    String elementTitle(Element* element)
    {
        StringBuilder result;
        result.append(nodeName(element));

        const AtomicString& idValue = element->getIdAttribute();
        String idString;
        if (!idValue.isNull() && !idValue.isEmpty()) {
            result.append("#");
            result.append(idValue);
        }

        HashSet<AtomicString> usedClassNames;
        if (element->hasClass() && element->isStyledElement()) {
            const SpaceSplitString& classNamesString = static_cast<StyledElement*>(element)->classNames();
            size_t classNameCount = classNamesString.size();
            for (size_t i = 0; i < classNameCount; ++i) {
                const AtomicString& className = classNamesString[i];
                if (usedClassNames.contains(className))
                    continue;
                usedClassNames.add(className);
                result.append(".");
                result.append(className);
            }
        }
        return result.toString();
    }

    HashSet<Node*> m_roots;
    Page* m_page;
    RefPtr<TypeBuilder::Array<TypeBuilder::Memory::DOMGroup> > m_domGroups;
    CharacterDataStatistics m_characterDataStatistics;
    int m_jsExternalStringSize;
    int m_sharedStringSize;
};

} // namespace

InspectorMemoryAgent::~InspectorMemoryAgent()
{
}

void InspectorMemoryAgent::getDOMNodeCount(ErrorString*, RefPtr<TypeBuilder::Array<TypeBuilder::Memory::DOMGroup> >& domGroups, RefPtr<TypeBuilder::Memory::StringStatistics>& strings)
{
    CounterVisitor counterVisitor(m_page);
    ScriptProfiler::visitJSDOMWrappers(&counterVisitor);

    // Make sure all documents reachable from the main frame are accounted.
    for (Frame* frame = m_page->mainFrame(); frame; frame = frame->tree()->traverseNext()) {
        if (Document* doc = frame->document())
            counterVisitor.visitNode(doc);
    }

    ScriptProfiler::visitExternalJSStrings(&counterVisitor);

    domGroups = counterVisitor.domGroups();
    strings = counterVisitor.strings();
}

static PassRefPtr<InspectorMemoryBlock> jsHeapInfo()
{
    size_t usedJSHeapSize;
    size_t totalJSHeapSize;
    size_t jsHeapSizeLimit;
    ScriptGCEvent::getHeapSize(usedJSHeapSize, totalJSHeapSize, jsHeapSizeLimit);

    RefPtr<InspectorMemoryBlock> jsHeapAllocated = InspectorMemoryBlock::create().setName(MemoryBlockName::jsHeapAllocated);
    jsHeapAllocated->setSize(static_cast<int>(totalJSHeapSize));

    RefPtr<TypeBuilder::Array<InspectorMemoryBlock> > children = TypeBuilder::Array<InspectorMemoryBlock>::create();
    RefPtr<InspectorMemoryBlock> jsHeapUsed = InspectorMemoryBlock::create().setName(MemoryBlockName::jsHeapUsed);
    jsHeapUsed->setSize(static_cast<int>(usedJSHeapSize));
    children->addItem(jsHeapUsed);

    jsHeapAllocated->setChildren(children);
    return jsHeapAllocated.release();
}

static PassRefPtr<InspectorMemoryBlock> inspectorData()
{
    size_t dataSize = ScriptProfiler::profilerSnapshotsSize();
    RefPtr<InspectorMemoryBlock> inspectorData = InspectorMemoryBlock::create().setName(MemoryBlockName::inspectorData);
    inspectorData->setSize(static_cast<int>(dataSize));
    return inspectorData.release();
}

static PassRefPtr<InspectorMemoryBlock> renderTreeInfo(Page* page)
{
    ArenaSize arenaSize = page->renderTreeSize();

    RefPtr<InspectorMemoryBlock> renderTreeAllocated = InspectorMemoryBlock::create().setName(MemoryBlockName::renderTreeAllocated);
    renderTreeAllocated->setSize(static_cast<int>(arenaSize.allocated));

    RefPtr<TypeBuilder::Array<InspectorMemoryBlock> > children = TypeBuilder::Array<InspectorMemoryBlock>::create();
    RefPtr<InspectorMemoryBlock> renderTreeUsed = InspectorMemoryBlock::create().setName(MemoryBlockName::renderTreeUsed);
    renderTreeUsed->setSize(static_cast<int>(arenaSize.treeSize));
    children->addItem(renderTreeUsed);

    renderTreeAllocated->setChildren(children);
    return renderTreeAllocated.release();
}

static void addMemoryBlockFor(TypeBuilder::Array<InspectorMemoryBlock>* array, const MemoryCache::TypeStatistic& statistic, const char* name)
{
    RefPtr<InspectorMemoryBlock> result = InspectorMemoryBlock::create().setName(name);
    result->setSize(statistic.size);
    array->addItem(result);
}

static PassRefPtr<InspectorMemoryBlock> memoryCacheInfo()
{
    MemoryCache::Statistics stats = memoryCache()->getStatistics();
    int totalSize = stats.images.size +
                    stats.cssStyleSheets.size +
                    stats.scripts.size +
                    stats.xslStyleSheets.size +
                    stats.fonts.size;
    RefPtr<InspectorMemoryBlock> memoryCacheStats = InspectorMemoryBlock::create().setName(MemoryBlockName::memoryCache);
    memoryCacheStats->setSize(totalSize);

    RefPtr<TypeBuilder::Array<InspectorMemoryBlock> > children = TypeBuilder::Array<InspectorMemoryBlock>::create();
    addMemoryBlockFor(children.get(), stats.images, MemoryBlockName::cachedImages);
    addMemoryBlockFor(children.get(), stats.cssStyleSheets, MemoryBlockName::cachedCssStyleSheets);
    addMemoryBlockFor(children.get(), stats.scripts, MemoryBlockName::cachedScripts);
    addMemoryBlockFor(children.get(), stats.xslStyleSheets, MemoryBlockName::cachedXslStyleSheets);
    addMemoryBlockFor(children.get(), stats.fonts, MemoryBlockName::cachedFonts);
    memoryCacheStats->setChildren(children);
    return memoryCacheStats.release();
}

void InspectorMemoryAgent::getProcessMemoryDistribution(ErrorString*, RefPtr<InspectorMemoryBlock>& processMemory)
{
    size_t privateBytes = 0;
    size_t sharedBytes = 0;
    MemoryUsageSupport::processMemorySizesInBytes(&privateBytes, &sharedBytes);
    processMemory = InspectorMemoryBlock::create().setName(MemoryBlockName::processPrivateMemory);
    processMemory->setSize(static_cast<int>(privateBytes));

    RefPtr<TypeBuilder::Array<InspectorMemoryBlock> > children = TypeBuilder::Array<InspectorMemoryBlock>::create();
    children->addItem(jsHeapInfo());
    children->addItem(inspectorData());
    children->addItem(memoryCacheInfo());
    children->addItem(renderTreeInfo(m_page)); // TODO: collect for all pages?
    processMemory->setChildren(children);
}

InspectorMemoryAgent::InspectorMemoryAgent(InstrumentingAgents* instrumentingAgents, InspectorState* state, Page* page, InspectorDOMAgent*)
    : InspectorBaseAgent<InspectorMemoryAgent>("Memory", instrumentingAgents, state)
    , m_page(page)
{
}

} // namespace WebCore

#endif // ENABLE(INSPECTOR)
