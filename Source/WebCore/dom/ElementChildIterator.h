/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ElementChildIterator_h
#define ElementChildIterator_h

#include "ElementIterator.h"

namespace WebCore {

template <typename ElementType>
class ElementChildIterator : public ElementIterator<ElementType> {
public:
    ElementChildIterator(const ContainerNode& parent);
    ElementChildIterator(const ContainerNode& parent, ElementType* current);
    ElementChildIterator& operator++();
};

template <typename ElementType>
class ElementChildConstIterator : public ElementConstIterator<ElementType> {
public:
    ElementChildConstIterator(const ContainerNode& parent);
    ElementChildConstIterator(const ContainerNode& parent, const ElementType* current);
    ElementChildConstIterator& operator++();
};

template <typename ElementType>
class ElementChildIteratorAdapter {
public:
    ElementChildIteratorAdapter(ContainerNode& parent);
    ElementChildIterator<ElementType> begin();
    ElementChildIterator<ElementType> end();
    ElementType* first();
    ElementType* last();

private:
    ContainerNode& m_parent;
};

template <typename ElementType>
class ElementChildConstIteratorAdapter {
public:
    ElementChildConstIteratorAdapter(const ContainerNode& parent);
    ElementChildConstIterator<ElementType> begin() const;
    ElementChildConstIterator<ElementType> end() const;
    const ElementType* first() const;
    const ElementType* last() const;

private:
    const ContainerNode& m_parent;
};

ElementChildIteratorAdapter<Element> elementChildren(ContainerNode&);
ElementChildConstIteratorAdapter<Element> elementChildren(const ContainerNode&);
template <typename ElementType> ElementChildIteratorAdapter<ElementType> childrenOfType(ContainerNode&);
template <typename ElementType> ElementChildConstIteratorAdapter<ElementType> childrenOfType(const ContainerNode&);

// ElementChildIterator

template <typename ElementType>
inline ElementChildIterator<ElementType>::ElementChildIterator(const ContainerNode& parent)
    : ElementIterator<ElementType>(&parent)
{
}

template <typename ElementType>
inline ElementChildIterator<ElementType>::ElementChildIterator(const ContainerNode& parent, ElementType* current)
    : ElementIterator<ElementType>(&parent, current)
{
}

template <typename ElementType>
inline ElementChildIterator<ElementType>& ElementChildIterator<ElementType>::operator++()
{
    return static_cast<ElementChildIterator<ElementType>&>(ElementIterator<ElementType>::traverseNextSibling());
}

// ElementChildConstIterator

template <typename ElementType>
inline ElementChildConstIterator<ElementType>::ElementChildConstIterator(const ContainerNode& parent)
    : ElementConstIterator<ElementType>(&parent)
{
}

template <typename ElementType>
inline ElementChildConstIterator<ElementType>::ElementChildConstIterator(const ContainerNode& parent, const ElementType* current)
    : ElementConstIterator<ElementType>(&parent, current)
{
}

template <typename ElementType>
inline ElementChildConstIterator<ElementType>& ElementChildConstIterator<ElementType>::operator++()
{
    return static_cast<ElementChildConstIterator<ElementType>&>(ElementConstIterator<ElementType>::traverseNextSibling());
}

// ElementChildIteratorAdapter

template <typename ElementType>
inline ElementChildIteratorAdapter<ElementType>::ElementChildIteratorAdapter(ContainerNode& parent)
    : m_parent(parent)
{
}

template <typename ElementType>
inline ElementChildIterator<ElementType> ElementChildIteratorAdapter<ElementType>::begin()
{
    return ElementChildIterator<ElementType>(m_parent, Traversal<ElementType>::firstChild(&m_parent));
}

template <typename ElementType>
inline ElementChildIterator<ElementType> ElementChildIteratorAdapter<ElementType>::end()
{
    return ElementChildIterator<ElementType>(m_parent);
}

template <typename ElementType>
inline ElementType* ElementChildIteratorAdapter<ElementType>::first()
{
    return Traversal<ElementType>::firstChild(&m_parent);
}

template <typename ElementType>
inline ElementType* ElementChildIteratorAdapter<ElementType>::last()
{
    return Traversal<ElementType>::lastChild(&m_parent);
}

// ElementChildConstIteratorAdapter

template <typename ElementType>
inline ElementChildConstIteratorAdapter<ElementType>::ElementChildConstIteratorAdapter(const ContainerNode& parent)
    : m_parent(parent)
{
}

template <typename ElementType>
inline ElementChildConstIterator<ElementType> ElementChildConstIteratorAdapter<ElementType>::begin() const
{
    return ElementChildConstIterator<ElementType>(m_parent, Traversal<ElementType>::firstChild(&m_parent));
}

template <typename ElementType>
inline ElementChildConstIterator<ElementType> ElementChildConstIteratorAdapter<ElementType>::end() const
{
    return ElementChildConstIterator<ElementType>(m_parent);
}

template <typename ElementType>
inline const ElementType* ElementChildConstIteratorAdapter<ElementType>::first() const
{
    return Traversal<ElementType>::firstChild(&m_parent);
}

template <typename ElementType>
inline const ElementType* ElementChildConstIteratorAdapter<ElementType>::last() const
{
    return Traversal<ElementType>::lastChild(&m_parent);
}

// Standalone functions

inline ElementChildIteratorAdapter<Element> elementChildren(ContainerNode& parent)
{
    return ElementChildIteratorAdapter<Element>(parent);
}

template <typename ElementType>
inline ElementChildIteratorAdapter<ElementType> childrenOfType(ContainerNode& parent)
{
    return ElementChildIteratorAdapter<ElementType>(parent);
}

inline ElementChildConstIteratorAdapter<Element> elementChildren(const ContainerNode& parent)
{
    return ElementChildConstIteratorAdapter<Element>(parent);
}

template <typename ElementType>
inline ElementChildConstIteratorAdapter<ElementType> childrenOfType(const ContainerNode& parent)
{
    return ElementChildConstIteratorAdapter<ElementType>(parent);
}

}

#endif
