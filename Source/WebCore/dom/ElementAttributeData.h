/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2010, 2012 Apple Inc. All rights reserved.
 * Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef ElementAttributeData_h
#define ElementAttributeData_h

#include "Attribute.h"
#include "SpaceSplitString.h"
#include "StylePropertySet.h"
#include <wtf/NotFound.h>

namespace WebCore {

class Element;
class ImmutableElementAttributeData;
class MutableElementAttributeData;

class ElementAttributeData : public RefCounted<ElementAttributeData> {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassRefPtr<ElementAttributeData> create();
    static PassRefPtr<ElementAttributeData> createImmutable(const Vector<Attribute>&);

    // Override RefCounted's deref() to ensure operator delete is called on
    // the appropriate subclass type.
    void deref();

    void clearClass() const { m_classNames.clear(); }
    void setClass(const AtomicString& className, bool shouldFoldCase) const { m_classNames.set(className, shouldFoldCase); }
    const SpaceSplitString& classNames() const { return m_classNames; }

    const AtomicString& idForStyleResolution() const { return m_idForStyleResolution; }
    void setIdForStyleResolution(const AtomicString& newId) const { m_idForStyleResolution = newId; }

    const StylePropertySet* inlineStyle() const { return m_inlineStyle.get(); }

    const StylePropertySet* presentationAttributeStyle() const;
    void setPresentationAttributeStyle(PassRefPtr<StylePropertySet>) const;

    size_t length() const;
    bool isEmpty() const { return !length(); }

    // Internal interface.
    const Attribute* attributeItem(unsigned index) const;
    const Attribute* getAttributeItem(const QualifiedName&) const;
    Attribute* attributeItem(unsigned index);
    Attribute* getAttributeItem(const QualifiedName&);
    size_t getAttributeItemIndex(const QualifiedName&) const;
    size_t getAttributeItemIndex(const AtomicString& name, bool shouldIgnoreAttributeCase) const;

    // These functions do no error checking.
    void addAttribute(const Attribute&);
    void removeAttribute(size_t index);

    bool hasID() const { return !m_idForStyleResolution.isNull(); }
    bool hasClass() const { return !m_classNames.isNull(); }

    bool isEquivalent(const ElementAttributeData* other) const;

    void reportMemoryUsage(MemoryObjectInfo*) const;

    bool isMutable() const { return m_isMutable; }
    const Attribute* immutableAttributeArray() const;

protected:
    ElementAttributeData()
        : m_isMutable(true)
        , m_arraySize(0)
        , m_presentationAttributeStyleIsDirty(false)
        , m_styleAttributeIsDirty(false)
#if ENABLE(SVG)
        , m_animatedSVGAttributesAreDirty(false)
#endif
    { }

    ElementAttributeData(unsigned arraySize)
        : m_isMutable(false)
        , m_arraySize(arraySize)
        , m_presentationAttributeStyleIsDirty(false)
        , m_styleAttributeIsDirty(false)
#if ENABLE(SVG)
        , m_animatedSVGAttributesAreDirty(false)
#endif
    { }

    ElementAttributeData(const ElementAttributeData&, bool isMutable);

    unsigned m_isMutable : 1;
    unsigned m_arraySize : 28;
    mutable unsigned m_presentationAttributeStyleIsDirty : 1;
    mutable unsigned m_styleAttributeIsDirty : 1;
#if ENABLE(SVG)
    mutable unsigned m_animatedSVGAttributesAreDirty : 1;
#endif

    mutable RefPtr<StylePropertySet> m_inlineStyle;
    mutable SpaceSplitString m_classNames;
    mutable AtomicString m_idForStyleResolution;

private:
    friend class Element;
    friend class StyledElement;
    friend class ImmutableElementAttributeData;
    friend class MutableElementAttributeData;
#if ENABLE(SVG)
    friend class SVGElement;
#endif

    Attribute* getAttributeItem(const AtomicString& name, bool shouldIgnoreAttributeCase);
    const Attribute* getAttributeItem(const AtomicString& name, bool shouldIgnoreAttributeCase) const;
    size_t getAttributeItemIndexSlowCase(const AtomicString& name, bool shouldIgnoreAttributeCase) const;

    PassRefPtr<ElementAttributeData> makeMutableCopy() const;
    PassRefPtr<ElementAttributeData> makeImmutableCopy() const;

    Vector<Attribute, 4>& mutableAttributeVector();
    const Vector<Attribute, 4>& mutableAttributeVector() const;
};

class ImmutableElementAttributeData : public ElementAttributeData {
public:
    ImmutableElementAttributeData(const Vector<Attribute>&);
    ImmutableElementAttributeData(const MutableElementAttributeData&);
    ~ImmutableElementAttributeData();

    void* m_attributeArray;
};

class MutableElementAttributeData : public ElementAttributeData {
public:
    MutableElementAttributeData() { }
    MutableElementAttributeData(const ImmutableElementAttributeData&);
    MutableElementAttributeData(const MutableElementAttributeData&);

    mutable RefPtr<StylePropertySet> m_presentationAttributeStyle;
    Vector<Attribute, 4> m_attributeVector;
};

inline Vector<Attribute, 4>& ElementAttributeData::mutableAttributeVector()
{
    ASSERT(m_isMutable);
    return static_cast<MutableElementAttributeData*>(this)->m_attributeVector;
}

inline const Vector<Attribute, 4>& ElementAttributeData::mutableAttributeVector() const
{
    ASSERT(m_isMutable);
    return static_cast<const MutableElementAttributeData*>(this)->m_attributeVector;
}

inline const Attribute* ElementAttributeData::immutableAttributeArray() const
{
    ASSERT(!m_isMutable);
    return reinterpret_cast<const Attribute*>(&static_cast<const ImmutableElementAttributeData*>(this)->m_attributeArray);
}

inline size_t ElementAttributeData::length() const
{
    if (isMutable())
        return mutableAttributeVector().size();
    return m_arraySize;
}

inline const StylePropertySet* ElementAttributeData::presentationAttributeStyle() const
{
    if (!m_isMutable)
        return 0;
    return static_cast<const MutableElementAttributeData*>(this)->m_presentationAttributeStyle.get();
}

inline void ElementAttributeData::setPresentationAttributeStyle(PassRefPtr<StylePropertySet> style) const
{
    ASSERT(m_isMutable);
    static_cast<const MutableElementAttributeData*>(this)->m_presentationAttributeStyle = style;
}

inline Attribute* ElementAttributeData::getAttributeItem(const AtomicString& name, bool shouldIgnoreAttributeCase)
{
    size_t index = getAttributeItemIndex(name, shouldIgnoreAttributeCase);
    if (index != notFound)
        return attributeItem(index);
    return 0;
}

inline const Attribute* ElementAttributeData::getAttributeItem(const AtomicString& name, bool shouldIgnoreAttributeCase) const
{
    size_t index = getAttributeItemIndex(name, shouldIgnoreAttributeCase);
    if (index != notFound)
        return attributeItem(index);
    return 0;
}

inline size_t ElementAttributeData::getAttributeItemIndex(const QualifiedName& name) const
{
    for (unsigned i = 0; i < length(); ++i) {
        if (attributeItem(i)->name().matches(name))
            return i;
    }
    return notFound;
}

// We use a boolean parameter instead of calling shouldIgnoreAttributeCase so that the caller
// can tune the behavior (hasAttribute is case sensitive whereas getAttribute is not).
inline size_t ElementAttributeData::getAttributeItemIndex(const AtomicString& name, bool shouldIgnoreAttributeCase) const
{
    unsigned len = length();
    bool doSlowCheck = shouldIgnoreAttributeCase;

    // Optimize for the case where the attribute exists and its name exactly matches.
    for (unsigned i = 0; i < len; ++i) {
        const Attribute* attribute = attributeItem(i);
        if (!attribute->name().hasPrefix()) {
            if (name == attribute->localName())
                return i;
        } else
            doSlowCheck = true;
    }

    if (doSlowCheck)
        return getAttributeItemIndexSlowCase(name, shouldIgnoreAttributeCase);
    return notFound;
}

inline const Attribute* ElementAttributeData::getAttributeItem(const QualifiedName& name) const
{
    for (unsigned i = 0; i < length(); ++i) {
        if (attributeItem(i)->name().matches(name))
            return attributeItem(i);
    }
    return 0;
}

inline Attribute* ElementAttributeData::getAttributeItem(const QualifiedName& name)
{
    for (unsigned i = 0; i < length(); ++i) {
        if (attributeItem(i)->name().matches(name))
            return attributeItem(i);
    }
    return 0;
}

inline const Attribute* ElementAttributeData::attributeItem(unsigned index) const
{
    ASSERT(index < length());
    if (m_isMutable)
        return &mutableAttributeVector().at(index);
    return &immutableAttributeArray()[index];
}

inline Attribute* ElementAttributeData::attributeItem(unsigned index)
{
    ASSERT(index < length());
    return &mutableAttributeVector().at(index);
}

inline void ElementAttributeData::deref()
{
    if (!derefBase())
        return;

    if (m_isMutable)
        delete static_cast<MutableElementAttributeData*>(this);
    else
        delete static_cast<ImmutableElementAttributeData*>(this);
}

}

#endif // ElementAttributeData_h
