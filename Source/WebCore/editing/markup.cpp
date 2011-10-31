/*
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 Apple Inc. All rights reserved.
 * Copyright (C) 2008, 2009, 2010, 2011 Google Inc. All rights reserved.
 * Copyright (C) 2011 Igalia S.L.
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE COMPUTER, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 */

#include "config.h"
#include "markup.h"

#include "CDATASection.h"
#include "CSSComputedStyleDeclaration.h"
#include "CSSMutableStyleDeclaration.h"
#include "CSSPrimitiveValue.h"
#include "CSSProperty.h"
#include "CSSPropertyNames.h"
#include "CSSRule.h"
#include "CSSRuleList.h"
#include "CSSStyleRule.h"
#include "CSSStyleSelector.h"
#include "CSSValue.h"
#include "CSSValueKeywords.h"
#include "DeleteButtonController.h"
#include "DocumentFragment.h"
#include "DocumentType.h"
#include "Editor.h"
#include "Frame.h"
#include "HTMLBodyElement.h"
#include "HTMLElement.h"
#include "HTMLNames.h"
#include "KURL.h"
#include "MarkupAccumulator.h"
#include "Range.h"
#include "RenderObject.h"
#include "TextIterator.h"
#include "VisibleSelection.h"
#include "XMLNSNames.h"
#include "htmlediting.h"
#include "visible_units.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/StringBuilder.h>
#include <wtf/unicode/CharacterNames.h>

using namespace std;

namespace WebCore {

using namespace HTMLNames;

static bool propertyMissingOrEqualToNone(CSSStyleDeclaration*, int propertyID);

class AttributeChange {
public:
    AttributeChange()
        : m_name(nullAtom, nullAtom, nullAtom)
    {
    }

    AttributeChange(PassRefPtr<Element> element, const QualifiedName& name, const String& value)
        : m_element(element), m_name(name), m_value(value)
    {
    }

    void apply()
    {
        m_element->setAttribute(m_name, m_value);
    }

private:
    RefPtr<Element> m_element;
    QualifiedName m_name;
    String m_value;
};

static void completeURLs(Node* node, const String& baseURL)
{
    Vector<AttributeChange> changes;

    KURL parsedBaseURL(ParsedURLString, baseURL);

    Node* end = node->traverseNextSibling();
    for (Node* n = node; n != end; n = n->traverseNextNode()) {
        if (n->isElementNode()) {
            Element* e = static_cast<Element*>(n);
            NamedNodeMap* attributes = e->attributes();
            unsigned length = attributes->length();
            for (unsigned i = 0; i < length; i++) {
                Attribute* attribute = attributes->attributeItem(i);
                if (e->isURLAttribute(attribute))
                    changes.append(AttributeChange(e, attribute->name(), KURL(parsedBaseURL, attribute->value()).string()));
            }
        }
    }

    size_t numChanges = changes.size();
    for (size_t i = 0; i < numChanges; ++i)
        changes[i].apply();
}
    
class StyledMarkupAccumulator : public MarkupAccumulator {
public:
    enum RangeFullySelectsNode { DoesFullySelectNode, DoesNotFullySelectNode };

    StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs, EAnnotateForInterchange, const Range*, Node* highestNodeToBeSerialized = 0);
    Node* serializeNodes(Node* startNode, Node* pastEnd);
    virtual void appendString(const String& s) { return MarkupAccumulator::appendString(s); }
    void wrapWithNode(Node*, bool convertBlocksToInlines = false, RangeFullySelectsNode = DoesFullySelectNode);
    void wrapWithStyleNode(CSSStyleDeclaration*, Document*, bool isBlock = false);
    String takeResults();

private:
    void appendStyleNodeOpenTag(StringBuilder&, CSSStyleDeclaration*, Document*, bool isBlock = false);
    const String styleNodeCloseTag(bool isBlock = false);
    virtual void appendText(StringBuilder& out, Text*);
    String renderedText(const Node*, const Range*);
    String stringValueForRange(const Node*, const Range*);
    void appendElement(StringBuilder& out, Element*, bool addDisplayInline, RangeFullySelectsNode);
    void appendElement(StringBuilder& out, Element* element, Namespaces*) { appendElement(out, element, false, DoesFullySelectNode); }

    enum NodeTraversalMode { EmitString, DoNotEmitString };
    Node* traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode);

    bool shouldAnnotate() { return m_shouldAnnotate == AnnotateForInterchange; }
    bool shouldApplyWrappingStyle(Node* node) const
    {
        return m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode() == node->parentNode()
            && m_wrappingStyle && m_wrappingStyle->style();
    }

    Vector<String> m_reversedPrecedingMarkup;
    const EAnnotateForInterchange m_shouldAnnotate;
    Node* m_highestNodeToBeSerialized;
    RefPtr<EditingStyle> m_wrappingStyle;
};

inline StyledMarkupAccumulator::StyledMarkupAccumulator(Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs, EAnnotateForInterchange shouldAnnotate,
    const Range* range, Node* highestNodeToBeSerialized)
    : MarkupAccumulator(nodes, shouldResolveURLs, range)
    , m_shouldAnnotate(shouldAnnotate)
    , m_highestNodeToBeSerialized(highestNodeToBeSerialized)
{
}

void StyledMarkupAccumulator::wrapWithNode(Node* node, bool convertBlocksToInlines, RangeFullySelectsNode rangeFullySelectsNode)
{
    StringBuilder markup;
    if (node->isElementNode())
        appendElement(markup, static_cast<Element*>(node), convertBlocksToInlines && isBlock(const_cast<Node*>(node)), rangeFullySelectsNode);
    else
        appendStartMarkup(markup, node, 0);
    m_reversedPrecedingMarkup.append(markup.toString());
    appendEndTag(node);
    if (m_nodes)
        m_nodes->append(node);
}

void StyledMarkupAccumulator::wrapWithStyleNode(CSSStyleDeclaration* style, Document* document, bool isBlock)
{
    StringBuilder openTag;
    appendStyleNodeOpenTag(openTag, style, document, isBlock);
    m_reversedPrecedingMarkup.append(openTag.toString());
    appendString(styleNodeCloseTag(isBlock));
}

void StyledMarkupAccumulator::appendStyleNodeOpenTag(StringBuilder& out, CSSStyleDeclaration* style, Document* document, bool isBlock)
{
    // wrappingStyleForSerialization should have removed -webkit-text-decorations-in-effect
    ASSERT(propertyMissingOrEqualToNone(style, CSSPropertyWebkitTextDecorationsInEffect));
    DEFINE_STATIC_LOCAL(const String, divStyle, ("<div style=\""));
    DEFINE_STATIC_LOCAL(const String, styleSpanOpen, ("<span style=\""));
    out.append(isBlock ? divStyle : styleSpanOpen);
    appendAttributeValue(out, style->cssText(), document->isHTMLDocument());
    out.append('\"');
    out.append('>');
}

const String StyledMarkupAccumulator::styleNodeCloseTag(bool isBlock)
{
    DEFINE_STATIC_LOCAL(const String, divClose, ("</div>"));
    DEFINE_STATIC_LOCAL(const String, styleSpanClose, ("</span>"));
    return isBlock ? divClose : styleSpanClose;
}

String StyledMarkupAccumulator::takeResults()
{
    StringBuilder result;
    result.reserveCapacity(totalLength(m_reversedPrecedingMarkup) + length());

    for (size_t i = m_reversedPrecedingMarkup.size(); i > 0; --i)
        result.append(m_reversedPrecedingMarkup[i - 1]);

    concatenateMarkup(result);

    // We remove '\0' characters because they are not visibly rendered to the user.
    return result.toString().replace(0, "");
}

void StyledMarkupAccumulator::appendText(StringBuilder& out, Text* text)
{    
    const bool parentIsTextarea = text->parentElement() && text->parentElement()->tagQName() == textareaTag;
    const bool wrappingSpan = shouldApplyWrappingStyle(text) && !parentIsTextarea;
    if (wrappingSpan) {
        RefPtr<EditingStyle> wrappingStyle = m_wrappingStyle->copy();
        // FIXME: <rdar://problem/5371536> Style rules that match pasted content can change it's appearance
        // Make sure spans are inline style in paste side e.g. span { display: block }.
        wrappingStyle->forceInline();
        // FIXME: Should this be included in forceInline?
        wrappingStyle->style()->setProperty(CSSPropertyFloat, CSSValueNone);

        StringBuilder openTag;
        appendStyleNodeOpenTag(openTag, wrappingStyle->style(), text->document());
        out.append(openTag.characters(), openTag.length());
    }

    if (!shouldAnnotate() || parentIsTextarea)
        MarkupAccumulator::appendText(out, text);
    else {
        const bool useRenderedText = !enclosingNodeWithTag(firstPositionInNode(text), selectTag);
        String content = useRenderedText ? renderedText(text, m_range) : stringValueForRange(text, m_range);
        StringBuilder buffer;
        appendCharactersReplacingEntities(buffer, content.characters(), content.length(), EntityMaskInPCDATA);
        out.append(convertHTMLTextToInterchangeFormat(buffer.toString(), text));
    }

    if (wrappingSpan)
        out.append(styleNodeCloseTag());
}
    
String StyledMarkupAccumulator::renderedText(const Node* node, const Range* range)
{
    if (!node->isTextNode())
        return String();

    ExceptionCode ec;
    const Text* textNode = static_cast<const Text*>(node);
    unsigned startOffset = 0;
    unsigned endOffset = textNode->length();

    if (range && node == range->startContainer(ec))
        startOffset = range->startOffset(ec);
    if (range && node == range->endContainer(ec))
        endOffset = range->endOffset(ec);

    Position start = createLegacyEditingPosition(const_cast<Node*>(node), startOffset);
    Position end = createLegacyEditingPosition(const_cast<Node*>(node), endOffset);
    return plainText(Range::create(node->document(), start, end).get());
}

String StyledMarkupAccumulator::stringValueForRange(const Node* node, const Range* range)
{
    if (!range)
        return node->nodeValue();

    String str = node->nodeValue();
    ExceptionCode ec;
    if (node == range->endContainer(ec))
        str.truncate(range->endOffset(ec));
    if (node == range->startContainer(ec))
        str.remove(0, range->startOffset(ec));
    return str;
}

void StyledMarkupAccumulator::appendElement(StringBuilder& out, Element* element, bool addDisplayInline, RangeFullySelectsNode rangeFullySelectsNode)
{
    const bool documentIsHTML = element->document()->isHTMLDocument();
    appendOpenTag(out, element, 0);

    NamedNodeMap* attributes = element->attributes();
    const unsigned length = attributes->length();
    const bool shouldAnnotateOrForceInline = element->isHTMLElement() && (shouldAnnotate() || addDisplayInline);
    const bool shouldOverrideStyleAttr = shouldAnnotateOrForceInline || shouldApplyWrappingStyle(element);
    for (unsigned int i = 0; i < length; i++) {
        Attribute* attribute = attributes->attributeItem(i);
        // We'll handle the style attribute separately, below.
        if (attribute->name() == styleAttr && shouldOverrideStyleAttr)
            continue;
        appendAttribute(out, element, *attribute, 0);
    }

    if (shouldOverrideStyleAttr) {
        RefPtr<EditingStyle> newInlineStyle;

        if (shouldApplyWrappingStyle(element)) {
            newInlineStyle = m_wrappingStyle->copy();
            newInlineStyle->removePropertiesInElementDefaultStyle(element);
            newInlineStyle->removeStyleConflictingWithStyleOfNode(element);
        } else
            newInlineStyle = EditingStyle::create();

        if (element->isStyledElement() && static_cast<StyledElement*>(element)->inlineStyleDecl())
            newInlineStyle->overrideWithStyle(static_cast<StyledElement*>(element)->inlineStyleDecl());

        if (shouldAnnotateOrForceInline) {
            if (shouldAnnotate())
                newInlineStyle->mergeStyleFromRulesForSerialization(toHTMLElement(element));

            if (addDisplayInline)
                newInlineStyle->forceInline();

            // If the node is not fully selected by the range, then we don't want to keep styles that affect its relationship to the nodes around it
            // only the ones that affect it and the nodes within it.
            if (rangeFullySelectsNode == DoesNotFullySelectNode && newInlineStyle->style())
                newInlineStyle->style()->removeProperty(CSSPropertyFloat);
        }

        if (!newInlineStyle->isEmpty()) {
            DEFINE_STATIC_LOCAL(const String, stylePrefix, (" style=\""));
            out.append(stylePrefix);
            appendAttributeValue(out, newInlineStyle->style()->cssText(), documentIsHTML);
            out.append('\"');
        }
    }

    appendCloseTag(out, element);
}

Node* StyledMarkupAccumulator::serializeNodes(Node* startNode, Node* pastEnd)
{
    if (!m_highestNodeToBeSerialized) {
        Node* lastClosed = traverseNodesForSerialization(startNode, pastEnd, DoNotEmitString);
        m_highestNodeToBeSerialized = lastClosed;
    }

    if (m_highestNodeToBeSerialized && m_highestNodeToBeSerialized->parentNode())
        m_wrappingStyle = EditingStyle::wrappingStyleForSerialization(m_highestNodeToBeSerialized->parentNode(), shouldAnnotate());

    return traverseNodesForSerialization(startNode, pastEnd, EmitString);
}

Node* StyledMarkupAccumulator::traverseNodesForSerialization(Node* startNode, Node* pastEnd, NodeTraversalMode traversalMode)
{
    const bool shouldEmit = traversalMode == EmitString;
    Vector<Node*> ancestorsToClose;
    Node* next;
    Node* lastClosed = 0;
    for (Node* n = startNode; n != pastEnd; n = next) {
        // According to <rdar://problem/5730668>, it is possible for n to blow
        // past pastEnd and become null here. This shouldn't be possible.
        // This null check will prevent crashes (but create too much markup)
        // and the ASSERT will hopefully lead us to understanding the problem.
        ASSERT(n);
        if (!n)
            break;
        
        next = n->traverseNextNode();
        bool openedTag = false;

        if (isBlock(n) && canHaveChildrenForEditing(n) && next == pastEnd)
            // Don't write out empty block containers that aren't fully selected.
            continue;

        if (!n->renderer() && !enclosingNodeWithTag(firstPositionInOrBeforeNode(n), selectTag)) {
            next = n->traverseNextSibling();
            // Don't skip over pastEnd.
            if (pastEnd && pastEnd->isDescendantOf(n))
                next = pastEnd;
        } else {
            // Add the node to the markup if we're not skipping the descendants
            if (shouldEmit)
                appendStartTag(n);

            // If node has no children, close the tag now.
            if (!n->childNodeCount()) {
                if (shouldEmit)
                    appendEndTag(n);
                lastClosed = n;
            } else {
                openedTag = true;
                ancestorsToClose.append(n);
            }
        }

        // If we didn't insert open tag and there's no more siblings or we're at the end of the traversal, take care of ancestors.
        // FIXME: What happens if we just inserted open tag and reached the end?
        if (!openedTag && (!n->nextSibling() || next == pastEnd)) {
            // Close up the ancestors.
            while (!ancestorsToClose.isEmpty()) {
                Node* ancestor = ancestorsToClose.last();
                if (next != pastEnd && next->isDescendantOf(ancestor))
                    break;
                // Not at the end of the range, close ancestors up to sibling of next node.
                if (shouldEmit)
                    appendEndTag(ancestor);
                lastClosed = ancestor;
                ancestorsToClose.removeLast();
            }

            // Surround the currently accumulated markup with markup for ancestors we never opened as we leave the subtree(s) rooted at those ancestors.
            ContainerNode* nextParent = next ? next->parentNode() : 0;
            if (next != pastEnd && n != nextParent) {
                Node* lastAncestorClosedOrSelf = n->isDescendantOf(lastClosed) ? lastClosed : n;
                for (ContainerNode* parent = lastAncestorClosedOrSelf->parentNode(); parent && parent != nextParent; parent = parent->parentNode()) {
                    // All ancestors that aren't in the ancestorsToClose list should either be a) unrendered:
                    if (!parent->renderer())
                        continue;
                    // or b) ancestors that we never encountered during a pre-order traversal starting at startNode:
                    ASSERT(startNode->isDescendantOf(parent));
                    if (shouldEmit)
                        wrapWithNode(parent);
                    lastClosed = parent;
                }
            }
        }
    }

    return lastClosed;
}

static bool isHTMLBlockElement(const Node* node)
{
    return node->hasTagName(tdTag)
        || node->hasTagName(thTag)
        || isNonTableCellHTMLBlockElement(node);
}

static Node* ancestorToRetainStructureAndAppearanceForBlock(Node* commonAncestorBlock)
{
    if (!commonAncestorBlock)
        return 0;

    if (commonAncestorBlock->hasTagName(tbodyTag) || commonAncestorBlock->hasTagName(trTag)) {
        ContainerNode* table = commonAncestorBlock->parentNode();
        while (table && !table->hasTagName(tableTag))
            table = table->parentNode();

        return table;
    }

    if (isNonTableCellHTMLBlockElement(commonAncestorBlock))
        return commonAncestorBlock;

    return 0;
}

static inline Node* ancestorToRetainStructureAndAppearance(Node* commonAncestor)
{
    return ancestorToRetainStructureAndAppearanceForBlock(enclosingBlock(commonAncestor));
}

static inline Node* ancestorToRetainStructureAndAppearanceWithNoRenderer(Node* commonAncestor)
{
    Node* commonAncestorBlock = enclosingNodeOfType(firstPositionInOrBeforeNode(commonAncestor), isHTMLBlockElement);
    return ancestorToRetainStructureAndAppearanceForBlock(commonAncestorBlock);
}

static bool propertyMissingOrEqualToNone(CSSStyleDeclaration* style, int propertyID)
{
    if (!style)
        return false;
    RefPtr<CSSValue> value = style->getPropertyCSSValue(propertyID);
    if (!value)
        return true;
    if (!value->isPrimitiveValue())
        return false;
    return static_cast<CSSPrimitiveValue*>(value.get())->getIdent() == CSSValueNone;
}

static bool needInterchangeNewlineAfter(const VisiblePosition& v)
{
    VisiblePosition next = v.next();
    Node* upstreamNode = next.deepEquivalent().upstream().deprecatedNode();
    Node* downstreamNode = v.deepEquivalent().downstream().deprecatedNode();
    // Add an interchange newline if a paragraph break is selected and a br won't already be added to the markup to represent it.
    return isEndOfParagraph(v) && isStartOfParagraph(next) && !(upstreamNode->hasTagName(brTag) && upstreamNode == downstreamNode);
}

static PassRefPtr<EditingStyle> styleFromMatchedRulesAndInlineDecl(const Node* node)
{
    if (!node->isHTMLElement())
        return 0;

    // FIXME: Having to const_cast here is ugly, but it is quite a bit of work to untangle
    // the non-const-ness of styleFromMatchedRulesForElement.
    HTMLElement* element = const_cast<HTMLElement*>(static_cast<const HTMLElement*>(node));
    RefPtr<EditingStyle> style = EditingStyle::create(element->inlineStyleDecl());
    style->mergeStyleFromRules(element);
    return style.release();
}

static bool isElementPresentational(const Node* node)
{
    return node->hasTagName(uTag) || node->hasTagName(sTag) || node->hasTagName(strikeTag)
        || node->hasTagName(iTag) || node->hasTagName(emTag) || node->hasTagName(bTag) || node->hasTagName(strongTag);
}

static Node* highestAncestorToWrapMarkup(const Range* range, EAnnotateForInterchange shouldAnnotate)
{
    ExceptionCode ec;
    Node* commonAncestor = range->commonAncestorContainer(ec);
    ASSERT(commonAncestor);
    Node* specialCommonAncestor = 0;
    if (shouldAnnotate == AnnotateForInterchange) {
        // Include ancestors that aren't completely inside the range but are required to retain 
        // the structure and appearance of the copied markup.
        specialCommonAncestor = ancestorToRetainStructureAndAppearance(commonAncestor);

        // Retain the Mail quote level by including all ancestor mail block quotes.
        if (Node* highestMailBlockquote = highestEnclosingNodeOfType(firstPositionInOrBeforeNode(range->firstNode()), isMailBlockquote, CanCrossEditingBoundary))
            specialCommonAncestor = highestMailBlockquote;
    }

    Node* checkAncestor = specialCommonAncestor ? specialCommonAncestor : commonAncestor;
    if (checkAncestor->renderer()) {
        Node* newSpecialCommonAncestor = highestEnclosingNodeOfType(firstPositionInNode(checkAncestor), &isElementPresentational);
        if (newSpecialCommonAncestor)
            specialCommonAncestor = newSpecialCommonAncestor;
    }

    // If a single tab is selected, commonAncestor will be a text node inside a tab span.
    // If two or more tabs are selected, commonAncestor will be the tab span.
    // In either case, if there is a specialCommonAncestor already, it will necessarily be above 
    // any tab span that needs to be included.
    if (!specialCommonAncestor && isTabSpanTextNode(commonAncestor))
        specialCommonAncestor = commonAncestor->parentNode();
    if (!specialCommonAncestor && isTabSpanNode(commonAncestor))
        specialCommonAncestor = commonAncestor;

    if (Node *enclosingAnchor = enclosingNodeWithTag(firstPositionInNode(specialCommonAncestor ? specialCommonAncestor : commonAncestor), aTag))
        specialCommonAncestor = enclosingAnchor;

    return specialCommonAncestor;
}

// FIXME: Shouldn't we omit style info when annotate == DoNotAnnotateForInterchange? 
// FIXME: At least, annotation and style info should probably not be included in range.markupString()
String createMarkup(const Range* range, Vector<Node*>* nodes, EAnnotateForInterchange shouldAnnotate, bool convertBlocksToInlines, EAbsoluteURLs shouldResolveURLs)
{
    DEFINE_STATIC_LOCAL(const String, interchangeNewlineString, ("<br class=\"" AppleInterchangeNewline "\">"));

    if (!range)
        return "";

    Document* document = range->ownerDocument();
    if (!document)
        return "";

    // Disable the delete button so it's elements are not serialized into the markup,
    // but make sure neither endpoint is inside the delete user interface.
    Frame* frame = document->frame();
    DeleteButtonController* deleteButton = frame ? frame->editor()->deleteButtonController() : 0;
    RefPtr<Range> updatedRange = avoidIntersectionWithNode(range, deleteButton ? deleteButton->containerElement() : 0);
    if (!updatedRange)
        return "";

    if (deleteButton)
        deleteButton->disable();

    ExceptionCode ec = 0;
    bool collapsed = updatedRange->collapsed(ec);
    ASSERT(!ec);
    if (collapsed)
        return "";
    Node* commonAncestor = updatedRange->commonAncestorContainer(ec);
    ASSERT(!ec);
    if (!commonAncestor)
        return "";

    document->updateLayoutIgnorePendingStylesheets();

    Node* body = enclosingNodeWithTag(firstPositionInNode(commonAncestor), bodyTag);
    Node* fullySelectedRoot = 0;
    // FIXME: Do this for all fully selected blocks, not just the body.
    if (body && areRangesEqual(VisibleSelection::selectionFromContentsOfNode(body).toNormalizedRange().get(), range))
        fullySelectedRoot = body;
    Node* specialCommonAncestor = highestAncestorToWrapMarkup(updatedRange.get(), shouldAnnotate);
    StyledMarkupAccumulator accumulator(nodes, shouldResolveURLs, shouldAnnotate, updatedRange.get(), specialCommonAncestor);
    Node* pastEnd = updatedRange->pastLastNode();

    Node* startNode = updatedRange->firstNode();
    VisiblePosition visibleStart(updatedRange->startPosition(), VP_DEFAULT_AFFINITY);
    VisiblePosition visibleEnd(updatedRange->endPosition(), VP_DEFAULT_AFFINITY);
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleStart)) {
        if (visibleStart == visibleEnd.previous()) {
            if (deleteButton)
                deleteButton->enable();
            return interchangeNewlineString;
        }

        accumulator.appendString(interchangeNewlineString);
        startNode = visibleStart.next().deepEquivalent().deprecatedNode();

        ExceptionCode ec = 0;
        if (pastEnd && Range::compareBoundaryPoints(startNode, 0, pastEnd, 0, ec) >= 0) {
            ASSERT(!ec);
            if (deleteButton)
                deleteButton->enable();
            return interchangeNewlineString;
        }
        ASSERT(!ec);
    }

    Node* lastClosed = accumulator.serializeNodes(startNode, pastEnd);

    if (specialCommonAncestor && lastClosed) {
        // Also include all of the ancestors of lastClosed up to this special ancestor.
        for (ContainerNode* ancestor = lastClosed->parentNode(); ancestor; ancestor = ancestor->parentNode()) {
            if (ancestor == fullySelectedRoot && !convertBlocksToInlines) {
                RefPtr<EditingStyle> fullySelectedRootStyle = styleFromMatchedRulesAndInlineDecl(fullySelectedRoot);

                // Bring the background attribute over, but not as an attribute because a background attribute on a div
                // appears to have no effect.
                if ((!fullySelectedRootStyle || !fullySelectedRootStyle->style() || !fullySelectedRootStyle->style()->getPropertyCSSValue(CSSPropertyBackgroundImage))
                    && static_cast<Element*>(fullySelectedRoot)->hasAttribute(backgroundAttr))
                    fullySelectedRootStyle->style()->setProperty(CSSPropertyBackgroundImage, "url('" + static_cast<Element*>(fullySelectedRoot)->getAttribute(backgroundAttr) + "')");

                if (fullySelectedRootStyle->style()) {
                    // Reset the CSS properties to avoid an assertion error in addStyleMarkup().
                    // This assertion is caused at least when we select all text of a <body> element whose
                    // 'text-decoration' property is "inherit", and copy it.
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyTextDecoration))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyTextDecoration, CSSValueNone);
                    if (!propertyMissingOrEqualToNone(fullySelectedRootStyle->style(), CSSPropertyWebkitTextDecorationsInEffect))
                        fullySelectedRootStyle->style()->setProperty(CSSPropertyWebkitTextDecorationsInEffect, CSSValueNone);
                    accumulator.wrapWithStyleNode(fullySelectedRootStyle->style(), document, true);
                }
            } else {
                // Since this node and all the other ancestors are not in the selection we want to set RangeFullySelectsNode to DoesNotFullySelectNode
                // so that styles that affect the exterior of the node are not included.
                accumulator.wrapWithNode(ancestor, convertBlocksToInlines, StyledMarkupAccumulator::DoesNotFullySelectNode);
            }
            if (nodes)
                nodes->append(ancestor);
            
            lastClosed = ancestor;
            
            if (ancestor == specialCommonAncestor)
                break;
        }
    }

    // FIXME: The interchange newline should be placed in the block that it's in, not after all of the content, unconditionally.
    if (shouldAnnotate == AnnotateForInterchange && needInterchangeNewlineAfter(visibleEnd.previous()))
        accumulator.appendString(interchangeNewlineString);

    if (deleteButton)
        deleteButton->enable();

    return accumulator.takeResults();
}

PassRefPtr<DocumentFragment> createFragmentFromMarkup(Document* document, const String& markup, const String& baseURL, FragmentScriptingPermission scriptingPermission)
{
    // We use a fake body element here to trick the HTML parser to using the InBody insertion mode.
    RefPtr<HTMLBodyElement> fakeBody = HTMLBodyElement::create(document);
    RefPtr<DocumentFragment> fragment =  Range::createDocumentFragmentForElement(markup, fakeBody.get(), scriptingPermission);

    if (fragment && !baseURL.isEmpty() && baseURL != blankURL() && baseURL != document->baseURL())
        completeURLs(fragment.get(), baseURL);

    return fragment.release();
}

static const char fragmentMarkerTag[] = "webkit-fragment-marker";

static bool findNodesSurroundingContext(Document* document, RefPtr<Node>& nodeBeforeContext, RefPtr<Node>& nodeAfterContext)
{
    for (Node* node = document->firstChild(); node; node = node->traverseNextNode()) {
        if (node->nodeType() == Node::COMMENT_NODE && static_cast<CharacterData*>(node)->data() == fragmentMarkerTag) {
            if (!nodeBeforeContext)
                nodeBeforeContext = node;
            else {
                nodeAfterContext = node;
                return true;
            }
        }
    }
    return false;
}

static void trimFragment(DocumentFragment* fragment, Node* nodeBeforeContext, Node* nodeAfterContext)
{
    ExceptionCode ec = 0;
    Node* next;
    for (RefPtr<Node> node = fragment->firstChild(); node; node = next) {
        if (nodeBeforeContext->isDescendantOf(node.get())) {
            next = node->traverseNextNode();
            continue;
        }
        next = node->traverseNextSibling();
        ASSERT(!node->contains(nodeAfterContext));
        node->parentNode()->removeChild(node.get(), ec);
        if (nodeBeforeContext == node)
            break;
    }

    ASSERT(nodeAfterContext->parentNode());
    for (Node* node = nodeAfterContext; node; node = next) {
        next = node->traverseNextSibling();
        node->parentNode()->removeChild(node, ec);
        ASSERT(!ec);
    }
}

PassRefPtr<DocumentFragment> createFragmentFromMarkupWithContext(Document* document, const String& markup, unsigned fragmentStart, unsigned fragmentEnd,
    const String& baseURL, FragmentScriptingPermission scriptingPermission)
{
    // FIXME: Need to handle the case where the markup already contains these markers.

    StringBuilder taggedMarkup;
    taggedMarkup.append(markup.left(fragmentStart));
    MarkupAccumulator::appendComment(taggedMarkup, fragmentMarkerTag);
    taggedMarkup.append(markup.substring(fragmentStart, fragmentEnd - fragmentStart));
    MarkupAccumulator::appendComment(taggedMarkup, fragmentMarkerTag);
    taggedMarkup.append(markup.substring(fragmentEnd));

    RefPtr<DocumentFragment> taggedFragment = createFragmentFromMarkup(document, taggedMarkup.toString(), baseURL, scriptingPermission);
    RefPtr<Document> taggedDocument = Document::create(0, KURL());
    taggedDocument->takeAllChildrenFrom(taggedFragment.get());

    RefPtr<Node> nodeBeforeContext;
    RefPtr<Node> nodeAfterContext;
    if (!findNodesSurroundingContext(taggedDocument.get(), nodeBeforeContext, nodeAfterContext))
        return 0;

    RefPtr<Range> range = Range::create(taggedDocument.get(),
        positionAfterNode(nodeBeforeContext.get()).parentAnchoredEquivalent(),
        positionBeforeNode(nodeAfterContext.get()).parentAnchoredEquivalent());

    ExceptionCode ec = 0;
    Node* commonAncestor = range->commonAncestorContainer(ec);
    ASSERT(!ec);
    Node* specialCommonAncestor = ancestorToRetainStructureAndAppearanceWithNoRenderer(commonAncestor);

    // When there's a special common ancestor outside of the fragment, we must include it as well to
    // preserve the structure and appearance of the fragment. For example, if the fragment contains
    // TD, we need to include the enclosing TABLE tag as well.
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document);
    if (specialCommonAncestor) {
        fragment->appendChild(specialCommonAncestor, ec);
        ASSERT(!ec);
    } else
        fragment->takeAllChildrenFrom(static_cast<ContainerNode*>(commonAncestor));

    trimFragment(fragment.get(), nodeBeforeContext.get(), nodeAfterContext.get());

    return fragment;
}

String createMarkup(const Node* node, EChildrenOnly childrenOnly, Vector<Node*>* nodes, EAbsoluteURLs shouldResolveURLs)
{
    if (!node)
        return "";

    HTMLElement* deleteButtonContainerElement = 0;
    if (Frame* frame = node->document()->frame()) {
        deleteButtonContainerElement = frame->editor()->deleteButtonController()->containerElement();
        if (node->isDescendantOf(deleteButtonContainerElement))
            return "";
    }

    MarkupAccumulator accumulator(nodes, shouldResolveURLs);
    return accumulator.serializeNodes(const_cast<Node*>(node), deleteButtonContainerElement, childrenOnly);
}

static void fillContainerFromString(ContainerNode* paragraph, const String& string)
{
    Document* document = paragraph->document();

    ExceptionCode ec = 0;
    if (string.isEmpty()) {
        paragraph->appendChild(createBlockPlaceholderElement(document), ec);
        ASSERT(!ec);
        return;
    }

    ASSERT(string.find('\n') == notFound);

    Vector<String> tabList;
    string.split('\t', true, tabList);
    String tabText = "";
    bool first = true;
    size_t numEntries = tabList.size();
    for (size_t i = 0; i < numEntries; ++i) {
        const String& s = tabList[i];

        // append the non-tab textual part
        if (!s.isEmpty()) {
            if (!tabText.isEmpty()) {
                paragraph->appendChild(createTabSpanElement(document, tabText), ec);
                ASSERT(!ec);
                tabText = "";
            }
            RefPtr<Node> textNode = document->createTextNode(stringWithRebalancedWhitespace(s, first, i + 1 == numEntries));
            paragraph->appendChild(textNode.release(), ec);
            ASSERT(!ec);
        }

        // there is a tab after every entry, except the last entry
        // (if the last character is a tab, the list gets an extra empty entry)
        if (i + 1 != numEntries)
            tabText.append('\t');
        else if (!tabText.isEmpty()) {
            paragraph->appendChild(createTabSpanElement(document, tabText), ec);
            ASSERT(!ec);
        }
        
        first = false;
    }
}

bool isPlainTextMarkup(Node *node)
{
    if (!node->isElementNode() || !node->hasTagName(divTag) || static_cast<Element*>(node)->attributes()->length())
        return false;
    
    if (node->childNodeCount() == 1 && (node->firstChild()->isTextNode() || (node->firstChild()->firstChild())))
        return true;
    
    return (node->childNodeCount() == 2 && isTabSpanTextNode(node->firstChild()->firstChild()) && node->firstChild()->nextSibling()->isTextNode());
}

PassRefPtr<DocumentFragment> createFragmentFromText(Range* context, const String& text)
{
    if (!context)
        return 0;

    Node* styleNode = context->firstNode();
    if (!styleNode) {
        styleNode = context->startPosition().deprecatedNode();
        if (!styleNode)
            return 0;
    }

    Document* document = styleNode->document();
    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();
    
    if (text.isEmpty())
        return fragment.release();

    String string = text;
    string.replace("\r\n", "\n");
    string.replace('\r', '\n');

    ExceptionCode ec = 0;
    RenderObject* renderer = styleNode->renderer();
    if (renderer && renderer->style()->preserveNewline()) {
        fragment->appendChild(document->createTextNode(string), ec);
        ASSERT(!ec);
        if (string.endsWith("\n")) {
            RefPtr<Element> element = createBreakElement(document);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
            fragment->appendChild(element.release(), ec);
            ASSERT(!ec);
        }
        return fragment.release();
    }

    // A string with no newlines gets added inline, rather than being put into a paragraph.
    if (string.find('\n') == notFound) {
        fillContainerFromString(fragment.get(), string);
        return fragment.release();
    }

    // Break string into paragraphs. Extra line breaks turn into empty paragraphs.
    Node* blockNode = enclosingBlock(context->firstNode());
    Element* block = static_cast<Element*>(blockNode);
    bool useClonesOfEnclosingBlock = blockNode
        && blockNode->isElementNode()
        && !block->hasTagName(bodyTag)
        && !block->hasTagName(htmlTag)
        && block != editableRootForPosition(context->startPosition());
    
    Vector<String> list;
    string.split('\n', true, list); // true gets us empty strings in the list
    size_t numLines = list.size();
    for (size_t i = 0; i < numLines; ++i) {
        const String& s = list[i];

        RefPtr<Element> element;
        if (s.isEmpty() && i + 1 == numLines) {
            // For last line, use the "magic BR" rather than a P.
            element = createBreakElement(document);
            element->setAttribute(classAttr, AppleInterchangeNewline);            
        } else {
            if (useClonesOfEnclosingBlock)
                element = block->cloneElementWithoutChildren();
            else
                element = createDefaultParagraphElement(document);
            fillContainerFromString(element.get(), s);
        }
        fragment->appendChild(element.release(), ec);
        ASSERT(!ec);
    }
    return fragment.release();
}

PassRefPtr<DocumentFragment> createFragmentFromNodes(Document *document, const Vector<Node*>& nodes)
{
    if (!document)
        return 0;

    // disable the delete button so it's elements are not serialized into the markup
    if (document->frame())
        document->frame()->editor()->deleteButtonController()->disable();

    RefPtr<DocumentFragment> fragment = document->createDocumentFragment();

    ExceptionCode ec = 0;
    size_t size = nodes.size();
    for (size_t i = 0; i < size; ++i) {
        RefPtr<Element> element = createDefaultParagraphElement(document);
        element->appendChild(nodes[i], ec);
        ASSERT(!ec);
        fragment->appendChild(element.release(), ec);
        ASSERT(!ec);
    }

    if (document->frame())
        document->frame()->editor()->deleteButtonController()->enable();

    return fragment.release();
}

String createFullMarkup(const Node* node)
{
    if (!node)
        return String();
        
    Document* document = node->document();
    if (!document)
        return String();
        
    Frame* frame = document->frame();
    if (!frame)
        return String();

    // FIXME: This is never "for interchange". Is that right?    
    String markupString = createMarkup(node, IncludeNode, 0);
    Node::NodeType nodeType = node->nodeType();
    if (nodeType != Node::DOCUMENT_NODE && nodeType != Node::DOCUMENT_TYPE_NODE)
        markupString = frame->documentTypeString() + markupString;

    return markupString;
}

String createFullMarkup(const Range* range)
{
    if (!range)
        return String();

    Node* node = range->startContainer();
    if (!node)
        return String();
        
    Document* document = node->document();
    if (!document)
        return String();
        
    Frame* frame = document->frame();
    if (!frame)
        return String();

    // FIXME: This is always "for interchange". Is that right? See the previous method.
    return frame->documentTypeString() + createMarkup(range, 0, AnnotateForInterchange);        
}

String urlToMarkup(const KURL& url, const String& title)
{
    StringBuilder markup;
    markup.append("<a href=\"");
    markup.append(url.string());
    markup.append("\">");
    appendCharactersReplacingEntities(markup, title.characters(), title.length(), EntityMaskInPCDATA);
    markup.append("</a>");
    return markup.toString();
}

}
