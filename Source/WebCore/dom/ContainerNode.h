/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2009, 2010, 2011 Apple Inc. All rights reserved.
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
 *
 */

#ifndef ContainerNode_h
#define ContainerNode_h

#include "ExceptionCodePlaceholder.h"
#include "Node.h"

#include <wtf/OwnPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class FloatPoint;
class QualifiedName;
class RenderElement;

typedef void (*NodeCallback)(Node&, unsigned);

namespace Private { 
    template<class GenericNode, class GenericNodeContainer>
    void addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer&);
};

class NoEventDispatchAssertion {
public:
    NoEventDispatchAssertion()
    {
#ifndef NDEBUG
        if (!isMainThread())
            return;
        s_count++;
#endif
    }

    ~NoEventDispatchAssertion()
    {
#ifndef NDEBUG
        if (!isMainThread())
            return;
        ASSERT(s_count);
        s_count--;
#endif
    }

#ifndef NDEBUG
    static bool isEventDispatchForbidden()
    {
        if (!isMainThread())
            return false;
        return s_count;
    }
#endif

private:
#ifndef NDEBUG
    static unsigned s_count;
#endif
};

class ContainerNode : public Node {
public:
    virtual ~ContainerNode();

    Node* firstChild() const { return m_firstChild; }
    Node* lastChild() const { return m_lastChild; }
    bool hasChildNodes() const { return m_firstChild; }

    unsigned childNodeCount() const;
    Node* childNode(unsigned index) const;

    bool insertBefore(PassRefPtr<Node> newChild, Node* refChild, ExceptionCode& = ASSERT_NO_EXCEPTION);
    bool replaceChild(PassRefPtr<Node> newChild, Node* oldChild, ExceptionCode& = ASSERT_NO_EXCEPTION);
    bool removeChild(Node* child, ExceptionCode& = ASSERT_NO_EXCEPTION);
    bool appendChild(PassRefPtr<Node> newChild, ExceptionCode& = ASSERT_NO_EXCEPTION);

    // These methods are only used during parsing.
    // They don't send DOM mutation events or handle reparenting.
    // However, arbitrary code may be run by beforeload handlers.
    void parserAppendChild(PassRefPtr<Node>);
    void parserRemoveChild(Node&);
    void parserInsertBefore(PassRefPtr<Node> newChild, Node* refChild);

    void removeChildren();
    void takeAllChildrenFrom(ContainerNode*);

    void cloneChildNodes(ContainerNode* clone);

    virtual LayoutRect boundingBox() const override;

    enum ChildChangeType { ElementInserted, ElementRemoved, TextInserted, TextRemoved, TextChanged, AllChildrenRemoved, NonContentsChildChanged };
    enum ChildChangeSource { ChildChangeSourceParser, ChildChangeSourceAPI };
    struct ChildChange {
        ChildChangeType type;
        Element* previousSiblingElement;
        Element* nextSiblingElement;
        ChildChangeSource source;
    };
    virtual void childrenChanged(const ChildChange&);

    void disconnectDescendantFrames();

    virtual bool childShouldCreateRenderer(const Node&) const { return true; }

    using Node::setAttributeEventListener;
    void setAttributeEventListener(const AtomicString& eventType, const QualifiedName& attributeName, const AtomicString& value);

    RenderElement* renderer() const;

    Element* querySelector(const String& selectors, ExceptionCode&);
    RefPtr<NodeList> querySelectorAll(const String& selectors, ExceptionCode&);

    PassRefPtr<NodeList> getElementsByTagName(const AtomicString&);
    PassRefPtr<NodeList> getElementsByTagNameNS(const AtomicString& namespaceURI, const AtomicString& localName);
    PassRefPtr<NodeList> getElementsByName(const String& elementName);
    PassRefPtr<NodeList> getElementsByClassName(const String& classNames);
    PassRefPtr<RadioNodeList> radioNodeList(const AtomicString&);

protected:
    explicit ContainerNode(Document&, ConstructionType = CreateContainer);

    template<class GenericNode, class GenericNodeContainer>
    friend void appendChildToContainer(GenericNode* child, GenericNodeContainer&);

    template<class GenericNode, class GenericNodeContainer>
    friend void Private::addChildNodesToDeletionQueue(GenericNode*& head, GenericNode*& tail, GenericNodeContainer&);

    void removeDetachedChildren();
    void setFirstChild(Node* child) { m_firstChild = child; }
    void setLastChild(Node* child) { m_lastChild = child; }

private:
    void removeBetween(Node* previousChild, Node* nextChild, Node& oldChild);
    void insertBeforeCommon(Node& nextChild, Node& oldChild);

    bool getUpperLeftCorner(FloatPoint&) const;
    bool getLowerRightCorner(FloatPoint&) const;

    void notifyChildInserted(Node& child, ChildChangeSource);
    void notifyChildRemoved(Node& child, Node* previousSibling, Node* nextSibling, ChildChangeSource);

    void updateTreeAfterInsertion(Node& child);

    bool isContainerNode() const = delete;

    void willRemoveChild(Node& child);

    Node* m_firstChild;
    Node* m_lastChild;
};

inline bool isContainerNode(const Node& node) { return node.isContainerNode(); }
void isContainerNode(const ContainerNode&); // Catch unnecessary runtime check of type known at compile time.

NODE_TYPE_CASTS(ContainerNode)

inline ContainerNode::ContainerNode(Document& document, ConstructionType type)
    : Node(document, type)
    , m_firstChild(0)
    , m_lastChild(0)
{
}

inline unsigned Node::childNodeCount() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->childNodeCount();
}

inline Node* Node::childNode(unsigned index) const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->childNode(index);
}

inline Node* Node::firstChild() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->firstChild();
}

inline Node* Node::lastChild() const
{
    if (!isContainerNode())
        return 0;
    return toContainerNode(this)->lastChild();
}

inline Node* Node::highestAncestor() const
{
    Node* node = const_cast<Node*>(this);
    Node* highest = node;
    for (; node; node = node->parentNode())
        highest = node;
    return highest;
}

inline bool Node::needsNodeRenderingTraversalSlowPath() const
{
    if (getFlag(NeedsNodeRenderingTraversalSlowPathFlag))
        return true;
    ContainerNode* parent = parentOrShadowHostNode();
    return parent && parent->getFlag(NeedsNodeRenderingTraversalSlowPathFlag);
}

inline bool Node::isTreeScope() const
{
    return &treeScope().rootNode() == this;
}

// This constant controls how much buffer is initially allocated
// for a Node Vector that is used to store child Nodes of a given Node.
// FIXME: Optimize the value.
const int initialNodeVectorSize = 11;
typedef Vector<Ref<Node>, initialNodeVectorSize> NodeVector;

inline void getChildNodes(Node& node, NodeVector& nodes)
{
    ASSERT(nodes.isEmpty());
    for (Node* child = node.firstChild(); child; child = child->nextSibling())
        nodes.append(*child);
}

class ChildNodesLazySnapshot {
    WTF_MAKE_NONCOPYABLE(ChildNodesLazySnapshot);
    WTF_MAKE_FAST_ALLOCATED;
public:
    explicit ChildNodesLazySnapshot(Node& parentNode)
        : m_currentNode(parentNode.firstChild())
        , m_currentIndex(0)
        , m_hasSnapshot(false)
    {
        m_nextSnapshot = latestSnapshot;
        latestSnapshot = this;
    }

    ALWAYS_INLINE ~ChildNodesLazySnapshot()
    {
        latestSnapshot = m_nextSnapshot;
    }

    // Returns 0 if there is no next Node.
    PassRefPtr<Node> nextNode()
    {
        if (LIKELY(!hasSnapshot())) {
            RefPtr<Node> node = m_currentNode.release();
            if (node)
                m_currentNode = node->nextSibling();
            return node.release();
        }
        if (m_currentIndex >= m_snapshot.size())
            return 0;
        return m_snapshot[m_currentIndex++];
    }

    void takeSnapshot()
    {
        if (hasSnapshot())
            return;
        m_hasSnapshot = true;
        Node* node = m_currentNode.get();
        while (node) {
            m_snapshot.append(node);
            node = node->nextSibling();
        }
    }

    ChildNodesLazySnapshot* nextSnapshot() { return m_nextSnapshot; }
    bool hasSnapshot() { return m_hasSnapshot; }

    static void takeChildNodesLazySnapshot()
    {
        ChildNodesLazySnapshot* snapshot = latestSnapshot;
        while (snapshot && !snapshot->hasSnapshot()) {
            snapshot->takeSnapshot();
            snapshot = snapshot->nextSnapshot();
        }
    }

private:
    static ChildNodesLazySnapshot* latestSnapshot;

    RefPtr<Node> m_currentNode;
    unsigned m_currentIndex;
    bool m_hasSnapshot;
    Vector<RefPtr<Node>> m_snapshot; // Lazily instantiated.
    ChildNodesLazySnapshot* m_nextSnapshot;
};

} // namespace WebCore

#endif // ContainerNode_h
