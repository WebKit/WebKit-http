/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2008, 2010 Apple Inc. All rights reserved.
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
#include "StyledElement.h"

#include "Attribute.h"
#include "CSSImageValue.h"
#include "CSSStyleSelector.h"
#include "CSSStyleSheet.h"
#include "CSSValueKeywords.h"
#include "CSSValuePool.h"
#include "Color.h"
#include "ClassList.h"
#include "ContentSecurityPolicy.h"
#include "DOMTokenList.h"
#include "Document.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "StylePropertySet.h"
#include <wtf/HashFunctions.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

struct PresentationAttributeCacheKey {
    PresentationAttributeCacheKey() : tagName(0) { }
    AtomicStringImpl* tagName;
    // Only the values need refcounting.
    Vector<pair<AtomicStringImpl*, AtomicString>, 3> attributesAndValues;
};

struct PresentationAttributeCacheEntry {
    PresentationAttributeCacheKey key;
    RefPtr<StylePropertySet> value;
};

typedef HashMap<unsigned, OwnPtr<PresentationAttributeCacheEntry>, AlreadyHashed> PresentationAttributeCache;
    
static bool operator!=(const PresentationAttributeCacheKey& a, const PresentationAttributeCacheKey& b)
{
    if (a.tagName != b.tagName)
        return true;
    return a.attributesAndValues != b.attributesAndValues;
}

static PresentationAttributeCache& presentationAttributeCache()
{
    DEFINE_STATIC_LOCAL(PresentationAttributeCache, cache, ());
    return cache;
}

void StyledElement::updateStyleAttribute() const
{
    ASSERT(!isStyleAttributeValid());
    setIsStyleAttributeValid();
    if (const StylePropertySet* inlineStyle = this->inlineStyle())
        const_cast<StyledElement*>(this)->setAttribute(styleAttr, inlineStyle->asText(), InUpdateStyleAttribute);
}

StyledElement::~StyledElement()
{
    destroyInlineStyle();
}

CSSStyleDeclaration* StyledElement::style() 
{ 
    return ensureAttributeData()->ensureMutableInlineStyle(this)->ensureInlineCSSStyleDeclaration(this); 
}

void StyledElement::attributeChanged(Attribute* attr)
{
    parseAttribute(attr);

    if (isPresentationAttribute(attr->name())) {
        setAttributeStyleDirty();
        setNeedsStyleRecalc(InlineStyleChange);
    }

    Element::attributeChanged(attr);
}

void StyledElement::classAttributeChanged(const AtomicString& newClassString)
{
    const UChar* characters = newClassString.characters();
    unsigned length = newClassString.length();
    unsigned i;
    for (i = 0; i < length; ++i) {
        if (isNotHTMLSpace(characters[i]))
            break;
    }
    bool hasClass = i < length;
    if (hasClass) {
        const bool shouldFoldCase = document()->inQuirksMode();
        ensureAttributeData()->setClass(newClassString, shouldFoldCase);
        if (DOMTokenList* classList = optionalClassList())
            static_cast<ClassList*>(classList)->reset(newClassString);
    } else if (attributeData())
        attributeData()->clearClass();
    setNeedsStyleRecalc();
}

void StyledElement::parseAttribute(Attribute* attr)
{
    if (attr->name() == classAttr)
        classAttributeChanged(attr->value());
    else if (attr->name() == styleAttr) {
        if (attr->isNull())
            destroyInlineStyle();
        else if (document()->contentSecurityPolicy()->allowInlineStyle())
            ensureAttributeData()->updateInlineStyleAvoidingMutation(this, attr->value());
        setIsStyleAttributeValid();
        setNeedsStyleRecalc();
        InspectorInstrumentation::didInvalidateStyleAttr(document(), this);
    }
}

void StyledElement::inlineStyleChanged()
{
    setNeedsStyleRecalc(InlineStyleChange);
    setIsStyleAttributeValid(false);
    InspectorInstrumentation::didInvalidateStyleAttr(document(), this);
}
    
bool StyledElement::setInlineStyleProperty(int propertyID, int identifier, bool important)
{
    ensureAttributeData()->ensureMutableInlineStyle(this)->setProperty(propertyID, document()->cssValuePool()->createIdentifierValue(identifier), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(int propertyID, double value, CSSPrimitiveValue::UnitTypes unit, bool important)
{
    ensureAttributeData()->ensureMutableInlineStyle(this)->setProperty(propertyID, document()->cssValuePool()->createValue(value, unit), important);
    inlineStyleChanged();
    return true;
}

bool StyledElement::setInlineStyleProperty(int propertyID, const String& value, bool important)
{
    bool changes = ensureAttributeData()->ensureMutableInlineStyle(this)->setProperty(propertyID, value, important, document()->elementSheet());
    if (changes)
        inlineStyleChanged();
    return changes;
}

bool StyledElement::removeInlineStyleProperty(int propertyID)
{
    StylePropertySet* inlineStyle = attributeData() ? attributeData()->inlineStyle() : 0;
    if (!inlineStyle)
        return false;
    bool changes = inlineStyle->removeProperty(propertyID);
    if (changes)
        inlineStyleChanged();
    return changes;
}

void StyledElement::addSubresourceAttributeURLs(ListHashSet<KURL>& urls) const
{
    if (StylePropertySet* inlineStyle = attributeData() ? attributeData()->inlineStyle() : 0)
        inlineStyle->addSubresourceStyleURLs(urls, document()->elementSheet());
}

static inline bool attributeNameSort(const pair<AtomicStringImpl*, AtomicString>& p1, const pair<AtomicStringImpl*, AtomicString>& p2)
{
    // Sort based on the attribute name pointers. It doesn't matter what the order is as long as it is always the same. 
    return p1.first < p2.first;
}

void StyledElement::makePresentationAttributeCacheKey(PresentationAttributeCacheKey& result) const
{    
    // FIXME: Enable for SVG.
    if (namespaceURI() != xhtmlNamespaceURI)
        return;
    // Interpretation of the size attributes on <input> depends on the type attribute.
    if (hasTagName(inputTag))
        return;
    unsigned size = attributeCount();
    for (unsigned i = 0; i < size; ++i) {
        Attribute* attribute = attributeItem(i);
        if (!isPresentationAttribute(attribute->name()))
            continue;
        if (!attribute->namespaceURI().isNull())
            return;
        // FIXME: Background URL may depend on the base URL and can't be shared. Disallow caching.
        if (attribute->name() == backgroundAttr)
            return;
        result.attributesAndValues.append(make_pair(attribute->localName().impl(), attribute->value()));
    }
    if (result.attributesAndValues.isEmpty())
        return;
    // Attribute order doesn't matter. Sort for easy equality comparison.
    std::sort(result.attributesAndValues.begin(), result.attributesAndValues.end(), attributeNameSort);
    // The cache key is non-null when the tagName is set.
    result.tagName = localName().impl();
}

static unsigned computePresentationAttributeCacheHash(const PresentationAttributeCacheKey& key)
{
    if (!key.tagName)
        return 0;
    ASSERT(key.attributesAndValues.size());
    unsigned attributeHash = StringHasher::hashMemory(key.attributesAndValues.data(), key.attributesAndValues.size() * sizeof(key.attributesAndValues[0]));
    return WTF::intHash((static_cast<uint64_t>(key.tagName->existingHash()) << 32 | attributeHash));
}

void StyledElement::updateAttributeStyle()
{
    PresentationAttributeCacheKey cacheKey;
    makePresentationAttributeCacheKey(cacheKey);

    unsigned cacheHash = computePresentationAttributeCacheHash(cacheKey);

    PresentationAttributeCache::iterator cacheIterator;
    if (cacheHash) {
        cacheIterator = presentationAttributeCache().add(cacheHash, nullptr).first;
        if (cacheIterator->second && cacheIterator->second->key != cacheKey)
            cacheHash = 0;
    } else
        cacheIterator = presentationAttributeCache().end();

    RefPtr<StylePropertySet> style;
    if (cacheHash && cacheIterator->second) 
        style = cacheIterator->second->value;
    else {
        style = StylePropertySet::create();
        unsigned size = attributeCount();
        for (unsigned i = 0; i < size; ++i) {
            Attribute* attribute = attributeItem(i);
            collectStyleForAttribute(attribute, style.get());
        }
    }
    clearAttributeStyleDirty();

    if (style->isEmpty())
        attributeData()->setAttributeStyle(0);
    else {
        style->shrinkToFit();
        attributeData()->setAttributeStyle(style);
    }

    if (!cacheHash || cacheIterator->second)
        return;

    OwnPtr<PresentationAttributeCacheEntry> newEntry = adoptPtr(new PresentationAttributeCacheEntry);
    newEntry->key = cacheKey;
    newEntry->value = style.release();

    static const int presentationAttributeCacheMaximumSize = 128;
    if (presentationAttributeCache().size() > presentationAttributeCacheMaximumSize) {
        // Start building from scratch if the cache ever gets big.
        presentationAttributeCache().clear();
        presentationAttributeCache().set(cacheHash, newEntry.release());
    } else
        cacheIterator->second = newEntry.release();
}

void StyledElement::addPropertyToAttributeStyle(StylePropertySet* style, int propertyID, int identifier)
{
    style->setProperty(propertyID, document()->cssValuePool()->createIdentifierValue(identifier));
}

void StyledElement::addPropertyToAttributeStyle(StylePropertySet* style, int propertyID, double value, CSSPrimitiveValue::UnitTypes unit)
{
    style->setProperty(propertyID, document()->cssValuePool()->createValue(value, unit));
}

}
