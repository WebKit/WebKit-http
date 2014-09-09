/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 2004-2005 Allan Sandfeld Jensen (kde@carewolf.com)
 * Copyright (C) 2006, 2007 Nicholas Shanks (webkit@nickshanks.com)
 * Copyright (C) 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012, 2013, 2014 Apple Inc. All rights reserved.
 * Copyright (C) 2007 Alexey Proskuryakov <ap@webkit.org>
 * Copyright (C) 2007, 2008 Eric Seidel <eric@webkit.org>
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 * Copyright (C) Research In Motion Limited 2011. All rights reserved.
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
#include "SelectorChecker.h"

#include "CSSSelector.h"
#include "CSSSelectorList.h"
#include "Document.h"
#include "ElementTraversal.h"
#include "Frame.h"
#include "FrameSelection.h"
#include "HTMLAnchorElement.h"
#include "HTMLDocument.h"
#include "HTMLFrameElementBase.h"
#include "HTMLInputElement.h"
#include "HTMLNames.h"
#include "HTMLOptGroupElement.h"
#include "HTMLOptionElement.h"
#include "HTMLProgressElement.h"
#include "HTMLStyleElement.h"
#include "InsertionPoint.h"
#include "InspectorInstrumentation.h"
#include "NodeRenderStyle.h"
#include "Page.h"
#include "RenderElement.h"
#include "RenderScrollbar.h"
#include "RenderStyle.h"
#include "ScrollableArea.h"
#include "ScrollbarTheme.h"
#include "SelectorCheckerTestFunctions.h"
#include "ShadowRoot.h"
#include "StyledElement.h"
#include "Text.h"

namespace WebCore {

using namespace HTMLNames;
    
static inline bool isFirstChildElement(const Element* element)
{
    return !ElementTraversal::previousSibling(element);
}

static inline bool isLastChildElement(const Element* element)
{
    return !ElementTraversal::nextSibling(element);
}

static inline bool isFirstOfType(const Element* element, const QualifiedName& type)
{
    for (const Element* sibling = ElementTraversal::previousSibling(element); sibling; sibling = ElementTraversal::previousSibling(sibling)) {
        if (sibling->hasTagName(type))
            return false;
    }
    return true;
}

static inline bool isLastOfType(const Element* element, const QualifiedName& type)
{
    for (const Element* sibling = ElementTraversal::nextSibling(element); sibling; sibling = ElementTraversal::nextSibling(sibling)) {
        if (sibling->hasTagName(type))
            return false;
    }
    return true;
}

static inline int countElementsBefore(const Element* element)
{
    int count = 0;
    for (const Element* sibling = ElementTraversal::previousSibling(element); sibling; sibling = ElementTraversal::previousSibling(sibling)) {
        unsigned index = sibling->childIndex();
        if (index) {
            count += index;
            break;
        }
        count++;
    }
    return count;
}

static inline int countElementsOfTypeBefore(const Element* element, const QualifiedName& type)
{
    int count = 0;
    for (const Element* sibling = ElementTraversal::previousSibling(element); sibling; sibling = ElementTraversal::previousSibling(sibling)) {
        if (sibling->hasTagName(type))
            ++count;
    }
    return count;
}

static inline int countElementsAfter(const Element* element)
{
    int count = 0;
    for (const Element* sibling = ElementTraversal::nextSibling(element); sibling; sibling = ElementTraversal::nextSibling(sibling))
        ++count;
    return count;
}

static inline int countElementsOfTypeAfter(const Element* element, const QualifiedName& type)
{
    int count = 0;
    for (const Element* sibling = ElementTraversal::nextSibling(element); sibling; sibling = ElementTraversal::nextSibling(sibling)) {
        if (sibling->hasTagName(type))
            ++count;
    }
    return count;
}

SelectorChecker::SelectorChecker(Document& document, Mode mode)
    : m_strictParsing(!document.inQuirksMode())
    , m_documentIsHTML(document.isHTMLDocument())
    , m_mode(mode)
{
}

bool SelectorChecker::match(const SelectorCheckingContext& context) const
{
    PseudoId pseudoId = NOPSEUDO;
    if (matchRecursively(context, pseudoId) != SelectorMatches)
        return false;
    if (context.pseudoId != NOPSEUDO && context.pseudoId != pseudoId)
        return false;
    if (context.pseudoId == NOPSEUDO && pseudoId != NOPSEUDO) {
        if (m_mode == Mode::ResolvingStyle && pseudoId < FIRST_INTERNAL_PSEUDOID)
            context.elementStyle->setHasPseudoStyle(pseudoId);

        // For SharingRules testing, any match is good enough, we don't care what is matched.
        return m_mode == Mode::SharingRules || m_mode == Mode::StyleInvalidation;
    }
    return true;
}

// Recursive check of selectors and combinators
// It can return 4 different values:
// * SelectorMatches          - the selector matches the element e
// * SelectorFailsLocally     - the selector fails for the element e
// * SelectorFailsAllSiblings - the selector fails for e and any sibling of e
// * SelectorFailsCompletely  - the selector fails for e and any sibling or ancestor of e
SelectorChecker::Match SelectorChecker::matchRecursively(const SelectorCheckingContext& context, PseudoId& dynamicPseudo) const
{
    // The first selector has to match.
    if (!checkOne(context))
        return SelectorFailsLocally;

    if (context.selector->m_match == CSSSelector::PseudoElement) {
        // In Selectors Level 4, a pseudo element inside a functional pseudo class is undefined (issue 7).
        // Make it as matching failure until the spec clarifies this case.
        if (context.inFunctionalPseudoClass)
            return SelectorFailsCompletely;

        if (context.selector->isCustomPseudoElement()) {
            if (ShadowRoot* root = context.element->containingShadowRoot()) {
                if (context.element->shadowPseudoId() != context.selector->value())
                    return SelectorFailsLocally;

                if (context.selector->pseudoElementType() == CSSSelector::PseudoElementWebKitCustom && root->type() != ShadowRoot::UserAgentShadowRoot)
                    return SelectorFailsLocally;
            } else
                return SelectorFailsLocally;
        } else {
            if ((!context.elementStyle && m_mode == Mode::ResolvingStyle) || m_mode == Mode::QueryingRules)
                return SelectorFailsLocally;

            PseudoId pseudoId = CSSSelector::pseudoId(context.selector->pseudoElementType());
            if (pseudoId != NOPSEUDO)
                dynamicPseudo = pseudoId;
        }
    }

    // The rest of the selectors has to match
    CSSSelector::Relation relation = context.selector->relation();

    // Prepare next selector
    const CSSSelector* historySelector = context.selector->tagHistory();
    if (!historySelector)
        return SelectorMatches;

    SelectorCheckingContext nextContext(context);
    nextContext.selector = historySelector;

    PseudoId ignoreDynamicPseudo = NOPSEUDO;
    if (relation != CSSSelector::SubSelector) {
        // Bail-out if this selector is irrelevant for the pseudoId
        if (context.pseudoId != NOPSEUDO && context.pseudoId != dynamicPseudo)
            return SelectorFailsCompletely;

        // Disable :visited matching when we see the first link or try to match anything else than an ancestors.
        if (context.element->isLink() || (relation != CSSSelector::Descendant && relation != CSSSelector::Child))
            nextContext.visitedMatchType = VisitedMatchDisabled;

        nextContext.pseudoId = NOPSEUDO;
    }

    switch (relation) {
    case CSSSelector::Descendant:
        nextContext.element = context.element->parentElement();
        nextContext.firstSelectorOfTheFragment = nextContext.selector;
        nextContext.elementStyle = 0;
        for (; nextContext.element; nextContext.element = nextContext.element->parentElement()) {
            Match match = this->matchRecursively(nextContext, ignoreDynamicPseudo);
            if (match == SelectorMatches || match == SelectorFailsCompletely)
                return match;
        }
        return SelectorFailsCompletely;

    case CSSSelector::Child:
        {
            nextContext.element = context.element->parentElement();
            if (!nextContext.element)
                return SelectorFailsCompletely;
            nextContext.firstSelectorOfTheFragment = nextContext.selector;
            nextContext.elementStyle = nullptr;
            Match match = matchRecursively(nextContext, ignoreDynamicPseudo);
            if (match == SelectorMatches || match == SelectorFailsCompletely)
                return match;
            return SelectorFailsAllSiblings;
        }

    case CSSSelector::DirectAdjacent:
        if (m_mode == Mode::ResolvingStyle) {
            if (Element* parentElement = context.element->parentElement())
                parentElement->setChildrenAffectedByDirectAdjacentRules();
        }
        nextContext.element = context.element->previousElementSibling();
        if (!nextContext.element)
            return SelectorFailsAllSiblings;
        nextContext.firstSelectorOfTheFragment = nextContext.selector;
        nextContext.elementStyle = 0;
        return matchRecursively(nextContext, ignoreDynamicPseudo);

    case CSSSelector::IndirectAdjacent:
        if (m_mode == Mode::ResolvingStyle) {
            if (Element* parentElement = context.element->parentElement())
                parentElement->setChildrenAffectedByForwardPositionalRules();
        }
        nextContext.element = context.element->previousElementSibling();
        nextContext.firstSelectorOfTheFragment = nextContext.selector;
        nextContext.elementStyle = 0;
        for (; nextContext.element; nextContext.element = nextContext.element->previousElementSibling()) {
            Match match = this->matchRecursively(nextContext, ignoreDynamicPseudo);
            if (match == SelectorMatches || match == SelectorFailsAllSiblings || match == SelectorFailsCompletely)
                return match;
        };
        return SelectorFailsAllSiblings;

    case CSSSelector::SubSelector:
        // a selector is invalid if something follows a pseudo-element
        // We make an exception for scrollbar pseudo elements and allow a set of pseudo classes (but nothing else)
        // to follow the pseudo elements.
        nextContext.hasScrollbarPseudo = dynamicPseudo != NOPSEUDO && (context.scrollbar || dynamicPseudo == SCROLLBAR_CORNER || dynamicPseudo == RESIZER);
        nextContext.hasSelectionPseudo = dynamicPseudo == SELECTION;
        if ((context.elementStyle || m_mode == Mode::CollectingRules) && dynamicPseudo != NOPSEUDO
            && !nextContext.hasSelectionPseudo
            && !(nextContext.hasScrollbarPseudo && nextContext.selector->m_match == CSSSelector::PseudoClass))
            return SelectorFailsCompletely;
        return matchRecursively(nextContext, dynamicPseudo);

    case CSSSelector::ShadowDescendant:
        {
            Element* shadowHostNode = context.element->shadowHost();
            if (!shadowHostNode)
                return SelectorFailsCompletely;
            nextContext.element = shadowHostNode;
            nextContext.firstSelectorOfTheFragment = nextContext.selector;
            nextContext.elementStyle = 0;
            return matchRecursively(nextContext, ignoreDynamicPseudo);
        }
    }

    ASSERT_NOT_REACHED();
    return SelectorFailsCompletely;
}

static bool attributeValueMatches(const Attribute& attribute, CSSSelector::Match match, const AtomicString& selectorValue, bool caseSensitive)
{
    const AtomicString& value = attribute.value();
    ASSERT(!value.isNull());

    switch (match) {
    case CSSSelector::Set:
        break;
    case CSSSelector::Exact:
        if (caseSensitive ? selectorValue != value : !equalIgnoringCase(selectorValue, value))
            return false;
        break;
    case CSSSelector::List:
        {
            // Ignore empty selectors or selectors containing spaces
            if (selectorValue.contains(' ') || selectorValue.isEmpty())
                return false;

            unsigned startSearchAt = 0;
            while (true) {
                size_t foundPos = value.find(selectorValue, startSearchAt, caseSensitive);
                if (foundPos == notFound)
                    return false;
                if (!foundPos || value[foundPos - 1] == ' ') {
                    unsigned endStr = foundPos + selectorValue.length();
                    if (endStr == value.length() || value[endStr] == ' ')
                        break; // We found a match.
                }

                // No match. Keep looking.
                startSearchAt = foundPos + 1;
            }
            break;
        }
    case CSSSelector::Contain:
        if (!value.contains(selectorValue, caseSensitive) || selectorValue.isEmpty())
            return false;
        break;
    case CSSSelector::Begin:
        if (!value.startsWith(selectorValue, caseSensitive) || selectorValue.isEmpty())
            return false;
        break;
    case CSSSelector::End:
        if (!value.endsWith(selectorValue, caseSensitive) || selectorValue.isEmpty())
            return false;
        break;
    case CSSSelector::Hyphen:
        if (value.length() < selectorValue.length())
            return false;
        if (!value.startsWith(selectorValue, caseSensitive))
            return false;
        // It they start the same, check for exact match or following '-':
        if (value.length() != selectorValue.length() && value[selectorValue.length()] != '-')
            return false;
        break;
    default:
        ASSERT_NOT_REACHED();
        return false;
    }

    return true;
}

static bool anyAttributeMatches(Element* element, const CSSSelector* selector, const QualifiedName& selectorAttr, bool caseSensitive)
{
    ASSERT(element->hasAttributesWithoutUpdate());
    for (const Attribute& attribute : element->attributesIterator()) {
        if (!attribute.matches(selectorAttr.prefix(), element->isHTMLElement() ? selector->attributeCanonicalLocalName() : selectorAttr.localName(), selectorAttr.namespaceURI()))
            continue;

        if (attributeValueMatches(attribute, static_cast<CSSSelector::Match>(selector->m_match), selector->value(), caseSensitive))
            return true;
    }

    return false;
}

static bool canMatchHoverOrActiveInQuirksMode(const SelectorChecker::SelectorCheckingContext& context)
{
    // For quirks mode, follow this: http://quirks.spec.whatwg.org/#the-:active-and-:hover-quirk
    // In quirks mode, a compound selector 'selector' that matches the following conditions must not match elements that would not also match the ':any-link' selector.
    //
    //    selector uses the ':active' or ':hover' pseudo-classes.
    //    selector does not use a type selector.
    //    selector does not use an attribute selector.
    //    selector does not use an ID selector.
    //    selector does not use a class selector.
    //    selector does not use a pseudo-class selector other than ':active' and ':hover'.
    //    selector does not use a pseudo-element selector.
    //    selector is not part of an argument to a functional pseudo-class or pseudo-element.
    if (context.inFunctionalPseudoClass)
        return true;

    for (const CSSSelector* selector = context.firstSelectorOfTheFragment; selector; selector = selector->tagHistory()) {
        switch (selector->m_match) {
        case CSSSelector::Tag:
            if (selector->tagQName() != anyQName())
                return true;
            break;
        case CSSSelector::PseudoClass: {
            CSSSelector::PseudoClassType pseudoClassType = selector->pseudoClassType();
            if (pseudoClassType != CSSSelector::PseudoClassHover && pseudoClassType != CSSSelector::PseudoClassActive)
                return true;
            break;
        }
        case CSSSelector::Id:
        case CSSSelector::Class:
        case CSSSelector::Exact:
        case CSSSelector::Set:
        case CSSSelector::List:
        case CSSSelector::Hyphen:
        case CSSSelector::Contain:
        case CSSSelector::Begin:
        case CSSSelector::End:
        case CSSSelector::PagePseudoClass:
        case CSSSelector::PseudoElement:
            return true;
        case CSSSelector::Unknown:
            ASSERT_NOT_REACHED();
            break;
        }

        CSSSelector::Relation relation = selector->relation();
        if (relation == CSSSelector::ShadowDescendant)
            return true;

        if (relation != CSSSelector::SubSelector)
            return false;
    }
    return false;
}

bool SelectorChecker::checkOne(const SelectorCheckingContext& context) const
{
    Element* const & element = context.element;
    const CSSSelector* const & selector = context.selector;
    ASSERT(element);
    ASSERT(selector);

    if (selector->m_match == CSSSelector::Tag)
        return SelectorChecker::tagMatches(element, selector->tagQName());

    if (selector->m_match == CSSSelector::Class)
        return element->hasClass() && element->classNames().contains(selector->value());

    if (selector->m_match == CSSSelector::Id)
        return element->hasID() && element->idForStyleResolution() == selector->value();

    if (selector->isAttributeSelector()) {
        if (!element->hasAttributes())
            return false;

        const QualifiedName& attr = selector->attribute();
        bool caseSensitive = !(m_documentIsHTML && element->isHTMLElement()) || HTMLDocument::isCaseSensitiveAttribute(attr);

        if (!anyAttributeMatches(element, selector, attr, caseSensitive))
            return false;
    }

    if (selector->m_match == CSSSelector::PseudoClass) {
        // Handle :not up front.
        if (selector->pseudoClassType() == CSSSelector::PseudoClassNot) {
            const CSSSelectorList* selectorList = selector->selectorList();

            // FIXME: We probably should fix the parser and make it never produce :not rules with missing selector list.
            if (!selectorList)
                return false;

            SelectorCheckingContext subContext(context);
            subContext.inFunctionalPseudoClass = true;
            subContext.firstSelectorOfTheFragment = selectorList->first();
            for (subContext.selector = selectorList->first(); subContext.selector; subContext.selector = subContext.selector->tagHistory()) {
                if (subContext.selector->m_match == CSSSelector::PseudoClass) {
                    // :not cannot nest. I don't really know why this is a
                    // restriction in CSS3, but it is, so let's honor it.
                    // the parser enforces that this never occurs
                    ASSERT(subContext.selector->pseudoClassType() != CSSSelector::PseudoClassNot);
                    // We select between :visited and :link when applying. We don't know which one applied (or not) yet.
                    if (subContext.selector->pseudoClassType() == CSSSelector::PseudoClassVisited || (subContext.selector->pseudoClassType() == CSSSelector::PseudoClassLink && subContext.visitedMatchType == VisitedMatchEnabled))
                        return true;
                }
                if (!checkOne(subContext))
                    return true;
            }
        } else if (context.hasScrollbarPseudo) {
            // CSS scrollbars match a specific subset of pseudo classes, and they have specialized rules for each
            // (since there are no elements involved).
            return checkScrollbarPseudoClass(context, &element->document(), selector);
        }

        // Normal element pseudo class checking.
        switch (selector->pseudoClassType()) {
            // Pseudo classes:
        case CSSSelector::PseudoClassNot:
            break; // Already handled up above.
        case CSSSelector::PseudoClassEmpty:
            {
                bool result = true;
                for (Node* n = element->firstChild(); n; n = n->nextSibling()) {
                    if (n->isElementNode()) {
                        result = false;
                        break;
                    }
                    if (n->isTextNode()) {
                        Text* textNode = toText(n);
                        if (!textNode->data().isEmpty()) {
                            result = false;
                            break;
                        }
                    }
                }
                if (m_mode == Mode::ResolvingStyle) {
                    element->setStyleAffectedByEmpty();
                    if (context.elementStyle)
                        context.elementStyle->setEmptyState(result);
                    else if (element->renderStyle() && (element->document().styleSheetCollection().usesSiblingRules() || element->renderStyle()->unique()))
                        element->renderStyle()->setEmptyState(result);
                }
                return result;
            }
        case CSSSelector::PseudoClassFirstChild:
            // first-child matches the first child that is an element
            if (Element* parentElement = element->parentElement()) {
                bool result = isFirstChildElement(element);
                if (m_mode == Mode::ResolvingStyle) {
                    RenderStyle* childStyle = context.elementStyle ? context.elementStyle : element->renderStyle();
                    parentElement->setChildrenAffectedByFirstChildRules();
                    if (result && childStyle)
                        childStyle->setFirstChildState();
                }
                return result;
            }
            break;
        case CSSSelector::PseudoClassFirstOfType:
            // first-of-type matches the first element of its type
            if (Element* parentElement = element->parentElement()) {
                bool result = isFirstOfType(element, element->tagQName());
                if (m_mode == Mode::ResolvingStyle)
                    parentElement->setChildrenAffectedByForwardPositionalRules();
                return result;
            }
            break;
        case CSSSelector::PseudoClassLastChild:
            // last-child matches the last child that is an element
            if (Element* parentElement = element->parentElement()) {
                bool result = parentElement->isFinishedParsingChildren() && isLastChildElement(element);
                if (m_mode == Mode::ResolvingStyle) {
                    RenderStyle* childStyle = context.elementStyle ? context.elementStyle : element->renderStyle();
                    parentElement->setChildrenAffectedByLastChildRules();
                    if (result && childStyle)
                        childStyle->setLastChildState();
                }
                return result;
            }
            break;
        case CSSSelector::PseudoClassLastOfType:
            // last-of-type matches the last element of its type
            if (Element* parentElement = element->parentElement()) {
                if (m_mode == Mode::ResolvingStyle)
                    parentElement->setChildrenAffectedByBackwardPositionalRules();
                if (!parentElement->isFinishedParsingChildren())
                    return false;
                return isLastOfType(element, element->tagQName());
            }
            break;
        case CSSSelector::PseudoClassOnlyChild:
            if (Element* parentElement = element->parentElement()) {
                bool firstChild = isFirstChildElement(element);
                bool onlyChild = firstChild && parentElement->isFinishedParsingChildren() && isLastChildElement(element);
                if (m_mode == Mode::ResolvingStyle) {
                    RenderStyle* childStyle = context.elementStyle ? context.elementStyle : element->renderStyle();
                    parentElement->setChildrenAffectedByFirstChildRules();
                    parentElement->setChildrenAffectedByLastChildRules();
                    if (firstChild && childStyle)
                        childStyle->setFirstChildState();
                    if (onlyChild && childStyle)
                        childStyle->setLastChildState();
                }
                return onlyChild;
            }
            break;
        case CSSSelector::PseudoClassOnlyOfType:
            // FIXME: This selector is very slow.
            if (Element* parentElement = element->parentElement()) {
                if (m_mode == Mode::ResolvingStyle) {
                    parentElement->setChildrenAffectedByForwardPositionalRules();
                    parentElement->setChildrenAffectedByBackwardPositionalRules();
                }
                if (!parentElement->isFinishedParsingChildren())
                    return false;
                return isFirstOfType(element, element->tagQName()) && isLastOfType(element, element->tagQName());
            }
            break;
        case CSSSelector::PseudoClassNthChild:
            if (!selector->parseNth())
                break;
            if (Element* parentElement = element->parentElement()) {
                int count = 1 + countElementsBefore(element);
                if (m_mode == Mode::ResolvingStyle) {
                    RenderStyle* childStyle = context.elementStyle ? context.elementStyle : element->renderStyle();
                    element->setChildIndex(count);
                    if (childStyle)
                        childStyle->setUnique();
                    parentElement->setChildrenAffectedByForwardPositionalRules();
                }

                if (selector->matchNth(count))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassNthOfType:
            if (!selector->parseNth())
                break;
            if (Element* parentElement = element->parentElement()) {
                int count = 1 + countElementsOfTypeBefore(element, element->tagQName());
                if (m_mode == Mode::ResolvingStyle)
                    parentElement->setChildrenAffectedByForwardPositionalRules();

                if (selector->matchNth(count))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassNthLastChild:
            if (!selector->parseNth())
                break;
            if (Element* parentElement = element->parentElement()) {
                if (m_mode == Mode::ResolvingStyle)
                    parentElement->setChildrenAffectedByBackwardPositionalRules();
                if (!parentElement->isFinishedParsingChildren())
                    return false;
                int count = 1 + countElementsAfter(element);
                if (selector->matchNth(count))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassNthLastOfType:
            if (!selector->parseNth())
                break;
            if (Element* parentElement = element->parentElement()) {
                if (m_mode == Mode::ResolvingStyle)
                    parentElement->setChildrenAffectedByBackwardPositionalRules();
                if (!parentElement->isFinishedParsingChildren())
                    return false;

                int count = 1 + countElementsOfTypeAfter(element, element->tagQName());
                if (selector->matchNth(count))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassTarget:
            if (element == element->document().cssTarget())
                return true;
            break;
        case CSSSelector::PseudoClassAny:
            {
                SelectorCheckingContext subContext(context);
                subContext.inFunctionalPseudoClass = true;
                PseudoId ignoreDynamicPseudo = NOPSEUDO;
                for (subContext.selector = selector->selectorList()->first(); subContext.selector; subContext.selector = CSSSelectorList::next(subContext.selector)) {
                    subContext.firstSelectorOfTheFragment = subContext.selector;
                    if (matchRecursively(subContext, ignoreDynamicPseudo) == SelectorMatches)
                        return true;
                }
            }
            break;
        case CSSSelector::PseudoClassAutofill:
            return isAutofilled(element);
        case CSSSelector::PseudoClassAnyLink:
        case CSSSelector::PseudoClassLink:
            // :visited and :link matches are separated later when applying the style. Here both classes match all links...
            return element->isLink();
        case CSSSelector::PseudoClassVisited:
            // ...except if :visited matching is disabled for ancestor/sibling matching.
            return element->isLink() && context.visitedMatchType == VisitedMatchEnabled;
        case CSSSelector::PseudoClassDrag:
            if (m_mode == Mode::ResolvingStyle) {
                if (context.elementStyle)
                    context.elementStyle->setAffectedByDrag();
                else
                    element->setChildrenAffectedByDrag();
            }
            if (element->renderer() && element->renderer()->isDragging())
                return true;
            break;
        case CSSSelector::PseudoClassFocus:
            return matchesFocusPseudoClass(element);
        case CSSSelector::PseudoClassHover:
            if (m_strictParsing || element->isLink() || canMatchHoverOrActiveInQuirksMode(context)) {
                if (m_mode == Mode::ResolvingStyle) {
                    if (context.elementStyle)
                        context.elementStyle->setAffectedByHover();
                    else
                        element->setChildrenAffectedByHover();
                }
                if (element->hovered() || InspectorInstrumentation::forcePseudoState(element, CSSSelector::PseudoClassHover))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassActive:
            if (m_strictParsing || element->isLink() || canMatchHoverOrActiveInQuirksMode(context)) {
                if (m_mode == Mode::ResolvingStyle) {
                    if (context.elementStyle)
                        context.elementStyle->setAffectedByActive();
                    else
                        element->setChildrenAffectedByActive();
                }
                if (element->active() || InspectorInstrumentation::forcePseudoState(element, CSSSelector::PseudoClassActive))
                    return true;
            }
            break;
        case CSSSelector::PseudoClassEnabled:
            return isEnabled(element);
        case CSSSelector::PseudoClassFullPageMedia:
            return element->document().isMediaDocument();
        case CSSSelector::PseudoClassDefault:
            return isDefaultButtonForForm(element);
        case CSSSelector::PseudoClassDisabled:
            return isDisabled(element);
        case CSSSelector::PseudoClassReadOnly:
            return matchesReadOnlyPseudoClass(element);
        case CSSSelector::PseudoClassReadWrite:
            return matchesReadWritePseudoClass(element);
        case CSSSelector::PseudoClassOptional:
            return isOptionalFormControl(element);
        case CSSSelector::PseudoClassRequired:
            return isRequiredFormControl(element);
        case CSSSelector::PseudoClassValid:
            return isValid(element);
        case CSSSelector::PseudoClassInvalid:
            return isInvalid(element);
        case CSSSelector::PseudoClassChecked:
            return isChecked(element);
        case CSSSelector::PseudoClassIndeterminate:
            return shouldAppearIndeterminate(element);
        case CSSSelector::PseudoClassRoot:
            if (element == element->document().documentElement())
                return true;
            break;
        case CSSSelector::PseudoClassLang:
            {
                const AtomicString& argument = selector->argument();
                if (argument.isNull())
                    return false;
                return matchesLangPseudoClass(element, argument.impl());
            }
#if ENABLE(FULLSCREEN_API)
        case CSSSelector::PseudoClassFullScreen:
            return matchesFullScreenPseudoClass(element);
        case CSSSelector::PseudoClassAnimatingFullScreenTransition:
            return matchesFullScreenAnimatingFullScreenTransitionPseudoClass(element);
        case CSSSelector::PseudoClassFullScreenAncestor:
            return matchesFullScreenAncestorPseudoClass(element);
        case CSSSelector::PseudoClassFullScreenDocument:
            return matchesFullScreenDocumentPseudoClass(element);
#endif
        case CSSSelector::PseudoClassInRange:
            return isInRange(element);
        case CSSSelector::PseudoClassOutOfRange:
            return isOutOfRange(element);
#if ENABLE(VIDEO_TRACK)
        case CSSSelector::PseudoClassFuture:
            return matchesFutureCuePseudoClass(element);
        case CSSSelector::PseudoClassPast:
            return matchesPastCuePseudoClass(element);
#endif

        case CSSSelector::PseudoClassScope:
            {
                const Node* contextualReferenceNode = !context.scope ? element->document().documentElement() : context.scope;
                if (element == contextualReferenceNode)
                    return true;
                break;
            }

        case CSSSelector::PseudoClassWindowInactive:
            return isWindowInactive(element);

        case CSSSelector::PseudoClassHorizontal:
        case CSSSelector::PseudoClassVertical:
        case CSSSelector::PseudoClassDecrement:
        case CSSSelector::PseudoClassIncrement:
        case CSSSelector::PseudoClassStart:
        case CSSSelector::PseudoClassEnd:
        case CSSSelector::PseudoClassDoubleButton:
        case CSSSelector::PseudoClassSingleButton:
        case CSSSelector::PseudoClassNoButton:
        case CSSSelector::PseudoClassCornerPresent:
            return false;

        case CSSSelector::PseudoClassUnknown:
            ASSERT_NOT_REACHED();
            break;
        }
        return false;
    }
#if ENABLE(VIDEO_TRACK)
    else if (selector->m_match == CSSSelector::PseudoElement && selector->pseudoElementType() == CSSSelector::PseudoElementCue) {
        SelectorCheckingContext subContext(context);

        PseudoId ignoreDynamicPseudo = NOPSEUDO;
        const CSSSelector* const & selector = context.selector;
        for (subContext.selector = selector->selectorList()->first(); subContext.selector; subContext.selector = CSSSelectorList::next(subContext.selector)) {
            subContext.firstSelectorOfTheFragment = subContext.selector;
            subContext.inFunctionalPseudoClass = true;
            if (matchRecursively(subContext, ignoreDynamicPseudo) == SelectorMatches)
                return true;
        }
        return false;
    }
#endif
    // ### add the rest of the checks...
    return true;
}

bool SelectorChecker::checkScrollbarPseudoClass(const SelectorCheckingContext& context, Document* document, const CSSSelector* selector) const
{
    ASSERT(selector->m_match == CSSSelector::PseudoClass);

    RenderScrollbar* scrollbar = context.scrollbar;
    ScrollbarPart part = context.scrollbarPart;

    // FIXME: This is a temporary hack for resizers and scrollbar corners. Eventually :window-inactive should become a real
    // pseudo class and just apply to everything.
    if (selector->pseudoClassType() == CSSSelector::PseudoClassWindowInactive)
        return !document->page()->focusController().isActive();

    if (!scrollbar)
        return false;

    switch (selector->pseudoClassType()) {
    case CSSSelector::PseudoClassEnabled:
        return scrollbar->enabled();
    case CSSSelector::PseudoClassDisabled:
        return !scrollbar->enabled();
    case CSSSelector::PseudoClassHover:
        {
            ScrollbarPart hoveredPart = scrollbar->hoveredPart();
            if (part == ScrollbarBGPart)
                return hoveredPart != NoPart;
            if (part == TrackBGPart)
                return hoveredPart == BackTrackPart || hoveredPart == ForwardTrackPart || hoveredPart == ThumbPart;
            return part == hoveredPart;
        }
    case CSSSelector::PseudoClassActive:
        {
            ScrollbarPart pressedPart = scrollbar->pressedPart();
            if (part == ScrollbarBGPart)
                return pressedPart != NoPart;
            if (part == TrackBGPart)
                return pressedPart == BackTrackPart || pressedPart == ForwardTrackPart || pressedPart == ThumbPart;
            return part == pressedPart;
        }
    case CSSSelector::PseudoClassHorizontal:
        return scrollbar->orientation() == HorizontalScrollbar;
    case CSSSelector::PseudoClassVertical:
        return scrollbar->orientation() == VerticalScrollbar;
    case CSSSelector::PseudoClassDecrement:
        return part == BackButtonStartPart || part == BackButtonEndPart || part == BackTrackPart;
    case CSSSelector::PseudoClassIncrement:
        return part == ForwardButtonStartPart || part == ForwardButtonEndPart || part == ForwardTrackPart;
    case CSSSelector::PseudoClassStart:
        return part == BackButtonStartPart || part == ForwardButtonStartPart || part == BackTrackPart;
    case CSSSelector::PseudoClassEnd:
        return part == BackButtonEndPart || part == ForwardButtonEndPart || part == ForwardTrackPart;
    case CSSSelector::PseudoClassDoubleButton:
        {
            ScrollbarButtonsPlacement buttonsPlacement = scrollbar->theme()->buttonsPlacement();
            if (part == BackButtonStartPart || part == ForwardButtonStartPart || part == BackTrackPart)
                return buttonsPlacement == ScrollbarButtonsDoubleStart || buttonsPlacement == ScrollbarButtonsDoubleBoth;
            if (part == BackButtonEndPart || part == ForwardButtonEndPart || part == ForwardTrackPart)
                return buttonsPlacement == ScrollbarButtonsDoubleEnd || buttonsPlacement == ScrollbarButtonsDoubleBoth;
            return false;
        }
    case CSSSelector::PseudoClassSingleButton:
        {
            ScrollbarButtonsPlacement buttonsPlacement = scrollbar->theme()->buttonsPlacement();
            if (part == BackButtonStartPart || part == ForwardButtonEndPart || part == BackTrackPart || part == ForwardTrackPart)
                return buttonsPlacement == ScrollbarButtonsSingle;
            return false;
        }
    case CSSSelector::PseudoClassNoButton:
        {
            ScrollbarButtonsPlacement buttonsPlacement = scrollbar->theme()->buttonsPlacement();
            if (part == BackTrackPart)
                return buttonsPlacement == ScrollbarButtonsNone || buttonsPlacement == ScrollbarButtonsDoubleEnd;
            if (part == ForwardTrackPart)
                return buttonsPlacement == ScrollbarButtonsNone || buttonsPlacement == ScrollbarButtonsDoubleStart;
            return false;
        }
    case CSSSelector::PseudoClassCornerPresent:
        return scrollbar->scrollableArea()->isScrollCornerVisible();
    default:
        return false;
    }
}

unsigned SelectorChecker::determineLinkMatchType(const CSSSelector* selector)
{
    unsigned linkMatchType = MatchAll;

    // Statically determine if this selector will match a link in visited, unvisited or any state, or never.
    // :visited never matches other elements than the innermost link element.
    for (; selector; selector = selector->tagHistory()) {
        if (selector->m_match == CSSSelector::PseudoClass) {
            switch (selector->pseudoClassType()) {
            case CSSSelector::PseudoClassNot:
                {
                    // :not(:visited) is equivalent to :link. Parser enforces that :not can't nest.
                    const CSSSelectorList* selectorList = selector->selectorList();
                    if (!selectorList)
                        break;

                    for (const CSSSelector* subSelector = selectorList->first(); subSelector; subSelector = subSelector->tagHistory()) {
                        if (subSelector->m_match == CSSSelector::PseudoClass) {
                            CSSSelector::PseudoClassType subType = subSelector->pseudoClassType();
                            if (subType == CSSSelector::PseudoClassVisited)
                                linkMatchType &= ~SelectorChecker::MatchVisited;
                            else if (subType == CSSSelector::PseudoClassLink)
                                linkMatchType &= ~SelectorChecker::MatchLink;
                        }
                    }
                }
                break;
            case CSSSelector::PseudoClassLink:
                linkMatchType &= ~SelectorChecker::MatchVisited;
                break;
            case CSSSelector::PseudoClassVisited:
                linkMatchType &= ~SelectorChecker::MatchLink;
                break;
            default:
                // We don't support :link and :visited inside :-webkit-any.
                break;
            }
        }
        CSSSelector::Relation relation = selector->relation();
        if (relation == CSSSelector::SubSelector)
            continue;
        if (relation != CSSSelector::Descendant && relation != CSSSelector::Child)
            return linkMatchType;
        if (linkMatchType != MatchAll)
            return linkMatchType;
    }
    return linkMatchType;
}

static bool isFrameFocused(const Element* element)
{
    return element->document().frame() && element->document().frame()->selection().isFocusedAndActive();
}

bool SelectorChecker::matchesFocusPseudoClass(const Element* element)
{
    if (InspectorInstrumentation::forcePseudoState(const_cast<Element*>(element), CSSSelector::PseudoClassFocus))
        return true;
    return element->focused() && isFrameFocused(element);
}

}
