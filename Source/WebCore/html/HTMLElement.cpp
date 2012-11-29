/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Torch Mobile Inc. All rights reserved. (http://www.torchmobile.com/)
 * Copyright (C) 2011 Motorola Mobility. All rights reserved.
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

#include "config.h"
#include "HTMLElement.h"

#include "Attribute.h"
#include "CSSParser.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "CSSValuePool.h"
#include "DOMSettableTokenList.h"
#include "DocumentFragment.h"
#include "Event.h"
#include "EventListener.h"
#include "EventNames.h"
#include "ExceptionCode.h"
#include "Frame.h"
#include "HTMLBRElement.h"
#include "HTMLCollection.h"
#include "HTMLDocument.h"
#include "HTMLElementFactory.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "HTMLTextFormControlElement.h"
#include "RenderWordBreak.h"
#include "ScriptEventListener.h"
#include "Settings.h"
#include "Text.h"
#include "TextIterator.h"
#include "XMLNames.h"
#include "markup.h"
#include <wtf/StdLibExtras.h>
#include <wtf/text/CString.h>

#if ENABLE(MICRODATA)
#include "HTMLPropertiesCollection.h"
#include "MicroDataItemValue.h"
#endif

namespace WebCore {

using namespace HTMLNames;
using namespace WTF;

using std::min;
using std::max;

PassRefPtr<HTMLElement> HTMLElement::create(const QualifiedName& tagName, Document* document)
{
    return adoptRef(new HTMLElement(tagName, document));
}

String HTMLElement::nodeName() const
{
    // FIXME: Would be nice to have an atomicstring lookup based off uppercase
    // chars that does not have to copy the string on a hit in the hash.
    // FIXME: We should have a way to detect XHTML elements and replace the hasPrefix() check with it.
    if (document()->isHTMLDocument() && !tagQName().hasPrefix())
        return tagQName().localNameUpper();
    return Element::nodeName();
}

bool HTMLElement::ieForbidsInsertHTML() const
{
    // FIXME: Supposedly IE disallows settting innerHTML, outerHTML
    // and createContextualFragment on these tags.  We have no tests to
    // verify this however, so this list could be totally wrong.
    // This list was moved from the previous endTagRequirement() implementation.
    // This is also called from editing and assumed to be the list of tags
    // for which no end tag should be serialized. It's unclear if the list for
    // IE compat and the list for serialization sanity are the same.
    if (hasLocalName(areaTag)
        || hasLocalName(baseTag)
        || hasLocalName(basefontTag)
        || hasLocalName(brTag)
        || hasLocalName(colTag)
        || hasLocalName(embedTag)
        || hasLocalName(frameTag)
        || hasLocalName(hrTag)
        || hasLocalName(imageTag)
        || hasLocalName(imgTag)
        || hasLocalName(inputTag)
        || hasLocalName(isindexTag)
        || hasLocalName(linkTag)
        || hasLocalName(metaTag)
        || hasLocalName(paramTag)
        || hasLocalName(sourceTag)
        || hasLocalName(wbrTag))
        return true;
    // FIXME: I'm not sure why dashboard mode would want to change the
    // serialization of <canvas>, that seems like a bad idea.
#if ENABLE(DASHBOARD_SUPPORT)
    if (hasLocalName(canvasTag)) {
        Settings* settings = document()->settings();
        if (settings && settings->usesDashboardBackwardCompatibilityMode())
            return true;
    }
#endif
    return false;
}

static inline int unicodeBidiAttributeForDirAuto(HTMLElement* element)
{
    if (element->hasLocalName(preTag) || element->hasLocalName(textareaTag))
        return CSSValueWebkitPlaintext;
    // FIXME: For bdo element, dir="auto" should result in "bidi-override isolate" but we don't support having multiple values in unicode-bidi yet.
    // See https://bugs.webkit.org/show_bug.cgi?id=73164.
    return CSSValueWebkitIsolate;
}

static unsigned parseBorderWidthAttribute(const Attribute& attribute)
{
    ASSERT(attribute.name() == borderAttr);
    unsigned borderWidth = 0;
    if (!attribute.isEmpty())
        parseHTMLNonNegativeInteger(attribute.value(), borderWidth);
    return borderWidth;
}

void HTMLElement::applyBorderAttributeToStyle(const Attribute& attribute, StylePropertySet* style)
{
    addPropertyToPresentationAttributeStyle(style, CSSPropertyBorderWidth, parseBorderWidthAttribute(attribute), CSSPrimitiveValue::CSS_PX);
    addPropertyToPresentationAttributeStyle(style, CSSPropertyBorderStyle, CSSValueSolid);
}

void HTMLElement::mapLanguageAttributeToLocale(const Attribute& attribute, StylePropertySet* style)
{
    ASSERT((attribute.name() == langAttr || attribute.name().matches(XMLNames::langAttr)));
    if (!attribute.isEmpty()) {
        // Have to quote so the locale id is treated as a string instead of as a CSS keyword.
        addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitLocale, quoteCSSString(attribute.value()));
    } else {
        // The empty string means the language is explicitly unknown.
        addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitLocale, CSSValueAuto);
    }
}

bool HTMLElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == alignAttr || name == contenteditableAttr || name == hiddenAttr || name == langAttr || name.matches(XMLNames::langAttr) || name == draggableAttr || name == dirAttr)
        return true;
    return StyledElement::isPresentationAttribute(name);
}

void HTMLElement::collectStyleForPresentationAttribute(const Attribute& attribute, StylePropertySet* style)
{
    if (attribute.name() == alignAttr) {
        if (equalIgnoringCase(attribute.value(), "middle"))
            addPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign, CSSValueCenter);
        else
            addPropertyToPresentationAttributeStyle(style, CSSPropertyTextAlign, attribute.value());
    } else if (attribute.name() == contenteditableAttr) {
        if (attribute.isEmpty() || equalIgnoringCase(attribute.value(), "true")) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserModify, CSSValueReadWrite);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWordWrap, CSSValueBreakWord);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitNbspMode, CSSValueSpace);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitLineBreak, CSSValueAfterWhiteSpace);
        } else if (equalIgnoringCase(attribute.value(), "plaintext-only")) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserModify, CSSValueReadWritePlaintextOnly);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWordWrap, CSSValueBreakWord);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitNbspMode, CSSValueSpace);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitLineBreak, CSSValueAfterWhiteSpace);
        } else if (equalIgnoringCase(attribute.value(), "false"))
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserModify, CSSValueReadOnly);
    } else if (attribute.name() == hiddenAttr) {
        addPropertyToPresentationAttributeStyle(style, CSSPropertyDisplay, CSSValueNone);
    } else if (attribute.name() == draggableAttr) {
        if (equalIgnoringCase(attribute.value(), "true")) {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserDrag, CSSValueElement);
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserSelect, CSSValueNone);
        } else if (equalIgnoringCase(attribute.value(), "false"))
            addPropertyToPresentationAttributeStyle(style, CSSPropertyWebkitUserDrag, CSSValueNone);
    } else if (attribute.name() == dirAttr) {
        if (equalIgnoringCase(attribute.value(), "auto"))
            addPropertyToPresentationAttributeStyle(style, CSSPropertyUnicodeBidi, unicodeBidiAttributeForDirAuto(this));
        else {
            addPropertyToPresentationAttributeStyle(style, CSSPropertyDirection, attribute.value());
            if (!hasTagName(bdiTag) && !hasTagName(bdoTag) && !hasTagName(outputTag))
                addPropertyToPresentationAttributeStyle(style, CSSPropertyUnicodeBidi, CSSValueEmbed);
        }
    } else if (attribute.name().matches(XMLNames::langAttr)) {
        mapLanguageAttributeToLocale(attribute, style);
    } else if (attribute.name() == langAttr) {
        // xml:lang has a higher priority than lang.
        if (!fastHasAttribute(XMLNames::langAttr))
            mapLanguageAttributeToLocale(attribute, style);
    } else
        StyledElement::collectStyleForPresentationAttribute(attribute, style);
}

void HTMLElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (isIdAttributeName(name) || name == classAttr || name == styleAttr)
        return StyledElement::parseAttribute(name, value);

    if (name == dirAttr)
        dirAttributeChanged(value);
    else if (name == tabindexAttr) {
        int tabindex = 0;
        if (value.isEmpty())
            clearTabIndexExplicitly();
        else if (parseHTMLInteger(value, tabindex)) {
            // Clamp tabindex to the range of 'short' to match Firefox's behavior.
            setTabIndexExplicitly(max(static_cast<int>(std::numeric_limits<short>::min()), min(tabindex, static_cast<int>(std::numeric_limits<short>::max()))));
        }
#if ENABLE(MICRODATA)
    } else if (name == itempropAttr) {
        setItemProp(value);
    } else if (name == itemrefAttr) {
        setItemRef(value);
    } else if (name == itemtypeAttr) {
        setItemType(value);
#endif
    }
// standard events
    else if (name == onclickAttr) {
        setAttributeEventListener(eventNames().clickEvent, createAttributeEventListener(this, name, value));
    } else if (name == oncontextmenuAttr) {
        setAttributeEventListener(eventNames().contextmenuEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondblclickAttr) {
        setAttributeEventListener(eventNames().dblclickEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmousedownAttr) {
        setAttributeEventListener(eventNames().mousedownEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmousemoveAttr) {
        setAttributeEventListener(eventNames().mousemoveEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmouseoutAttr) {
        setAttributeEventListener(eventNames().mouseoutEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmouseoverAttr) {
        setAttributeEventListener(eventNames().mouseoverEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmouseupAttr) {
        setAttributeEventListener(eventNames().mouseupEvent, createAttributeEventListener(this, name, value));
    } else if (name == onmousewheelAttr) {
        setAttributeEventListener(eventNames().mousewheelEvent, createAttributeEventListener(this, name, value));
    } else if (name == onfocusAttr) {
        setAttributeEventListener(eventNames().focusEvent, createAttributeEventListener(this, name, value));
    } else if (name == onfocusinAttr) {
        setAttributeEventListener(eventNames().focusinEvent, createAttributeEventListener(this, name, value));
    } else if (name == onfocusoutAttr) {
        setAttributeEventListener(eventNames().focusoutEvent, createAttributeEventListener(this, name, value));
    } else if (name == onblurAttr) {
        setAttributeEventListener(eventNames().blurEvent, createAttributeEventListener(this, name, value));
    } else if (name == onkeydownAttr) {
        setAttributeEventListener(eventNames().keydownEvent, createAttributeEventListener(this, name, value));
    } else if (name == onkeypressAttr) {
        setAttributeEventListener(eventNames().keypressEvent, createAttributeEventListener(this, name, value));
    } else if (name == onkeyupAttr) {
        setAttributeEventListener(eventNames().keyupEvent, createAttributeEventListener(this, name, value));
    } else if (name == onscrollAttr) {
        setAttributeEventListener(eventNames().scrollEvent, createAttributeEventListener(this, name, value));
    } else if (name == onbeforecutAttr) {
        setAttributeEventListener(eventNames().beforecutEvent, createAttributeEventListener(this, name, value));
    } else if (name == oncutAttr) {
        setAttributeEventListener(eventNames().cutEvent, createAttributeEventListener(this, name, value));
    } else if (name == onbeforecopyAttr) {
        setAttributeEventListener(eventNames().beforecopyEvent, createAttributeEventListener(this, name, value));
    } else if (name == oncopyAttr) {
        setAttributeEventListener(eventNames().copyEvent, createAttributeEventListener(this, name, value));
    } else if (name == onbeforepasteAttr) {
        setAttributeEventListener(eventNames().beforepasteEvent, createAttributeEventListener(this, name, value));
    } else if (name == onpasteAttr) {
        setAttributeEventListener(eventNames().pasteEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragenterAttr) {
        setAttributeEventListener(eventNames().dragenterEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragoverAttr) {
        setAttributeEventListener(eventNames().dragoverEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragleaveAttr) {
        setAttributeEventListener(eventNames().dragleaveEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondropAttr) {
        setAttributeEventListener(eventNames().dropEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragstartAttr) {
        setAttributeEventListener(eventNames().dragstartEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragAttr) {
        setAttributeEventListener(eventNames().dragEvent, createAttributeEventListener(this, name, value));
    } else if (name == ondragendAttr) {
        setAttributeEventListener(eventNames().dragendEvent, createAttributeEventListener(this, name, value));
    } else if (name == onselectstartAttr) {
        setAttributeEventListener(eventNames().selectstartEvent, createAttributeEventListener(this, name, value));
    } else if (name == onsubmitAttr) {
        setAttributeEventListener(eventNames().submitEvent, createAttributeEventListener(this, name, value));
    } else if (name == onerrorAttr) {
        setAttributeEventListener(eventNames().errorEvent, createAttributeEventListener(this, name, value));
    } else if (name == onwebkitanimationstartAttr) {
        setAttributeEventListener(eventNames().webkitAnimationStartEvent, createAttributeEventListener(this, name, value));
    } else if (name == onwebkitanimationiterationAttr) {
        setAttributeEventListener(eventNames().webkitAnimationIterationEvent, createAttributeEventListener(this, name, value));
    } else if (name == onwebkitanimationendAttr) {
        setAttributeEventListener(eventNames().webkitAnimationEndEvent, createAttributeEventListener(this, name, value));
    } else if (name == onwebkittransitionendAttr) {
        setAttributeEventListener(eventNames().webkitTransitionEndEvent, createAttributeEventListener(this, name, value));
    } else if (name == oninputAttr) {
        setAttributeEventListener(eventNames().inputEvent, createAttributeEventListener(this, name, value));
    } else if (name == oninvalidAttr) {
        setAttributeEventListener(eventNames().invalidEvent, createAttributeEventListener(this, name, value));
    } else if (name == ontouchstartAttr) {
        setAttributeEventListener(eventNames().touchstartEvent, createAttributeEventListener(this, name, value));
    } else if (name == ontouchmoveAttr) {
        setAttributeEventListener(eventNames().touchmoveEvent, createAttributeEventListener(this, name, value));
    } else if (name == ontouchendAttr) {
        setAttributeEventListener(eventNames().touchendEvent, createAttributeEventListener(this, name, value));
    } else if (name == ontouchcancelAttr) {
        setAttributeEventListener(eventNames().touchcancelEvent, createAttributeEventListener(this, name, value));
#if ENABLE(FULLSCREEN_API)
    } else if (name == onwebkitfullscreenchangeAttr) {
        setAttributeEventListener(eventNames().webkitfullscreenchangeEvent, createAttributeEventListener(this, name, value));
    } else if (name == onwebkitfullscreenerrorAttr) {
        setAttributeEventListener(eventNames().webkitfullscreenerrorEvent, createAttributeEventListener(this, name, value));
#endif
    }
}

String HTMLElement::innerHTML() const
{
    return createMarkup(this, ChildrenOnly);
}

String HTMLElement::outerHTML() const
{
    return createMarkup(this);
}

void HTMLElement::setInnerHTML(const String& html, ExceptionCode& ec)
{
    if (RefPtr<DocumentFragment> fragment = createFragmentForInnerOuterHTML(html, this, AllowScriptingContent, ec))
        replaceChildrenWithFragment(this, fragment.release(), ec);
}

static void mergeWithNextTextNode(PassRefPtr<Node> node, ExceptionCode& ec)
{
    ASSERT(node && node->isTextNode());
    Node* next = node->nextSibling();
    if (!next || !next->isTextNode())
        return;
    
    RefPtr<Text> textNode = toText(node.get());
    RefPtr<Text> textNext = toText(next);
    textNode->appendData(textNext->data(), ec);
    if (ec)
        return;
    if (textNext->parentNode()) // Might have been removed by mutation event.
        textNext->remove(ec);
}

void HTMLElement::setOuterHTML(const String& html, ExceptionCode& ec)
{
    Node* p = parentNode();
    if (!p || !p->isHTMLElement()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    RefPtr<HTMLElement> parent = toHTMLElement(p);
    RefPtr<Node> prev = previousSibling();
    RefPtr<Node> next = nextSibling();

    RefPtr<DocumentFragment> fragment = createFragmentForInnerOuterHTML(html, parent.get(), AllowScriptingContent, ec);
    if (ec)
        return;
      
    parent->replaceChild(fragment.release(), this, ec);
    RefPtr<Node> node = next ? next->previousSibling() : 0;
    if (!ec && node && node->isTextNode())
        mergeWithNextTextNode(node.release(), ec);

    if (!ec && prev && prev->isTextNode())
        mergeWithNextTextNode(prev.release(), ec);
}

PassRefPtr<DocumentFragment> HTMLElement::textToFragment(const String& text, ExceptionCode& ec)
{
    RefPtr<DocumentFragment> fragment = DocumentFragment::create(document());
    unsigned int i, length = text.length();
    UChar c = 0;
    for (unsigned int start = 0; start < length; ) {

        // Find next line break.
        for (i = start; i < length; i++) {
          c = text[i];
          if (c == '\r' || c == '\n')
              break;
        }

        fragment->appendChild(Text::create(document(), text.substring(start, i - start)), ec);
        if (ec)
            return 0;

        if (c == '\r' || c == '\n') {
            fragment->appendChild(HTMLBRElement::create(document()), ec);
            if (ec)
                return 0;
            // Make sure \r\n doesn't result in two line breaks.
            if (c == '\r' && i + 1 < length && text[i + 1] == '\n')
                i++;
        }

        start = i + 1; // Character after line break.
    }

    return fragment;
}

void HTMLElement::setInnerText(const String& text, ExceptionCode& ec)
{
    if (ieForbidsInsertHTML()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    // FIXME: This doesn't take whitespace collapsing into account at all.

    if (!text.contains('\n') && !text.contains('\r')) {
        if (text.isEmpty()) {
            removeChildren();
            return;
        }
        replaceChildrenWithText(this, text, ec);
        return;
    }

    // FIXME: Do we need to be able to detect preserveNewline style even when there's no renderer?
    // FIXME: Can the renderer be out of date here? Do we need to call updateStyleIfNeeded?
    // For example, for the contents of textarea elements that are display:none?
    RenderObject* r = renderer();
    if (r && r->style()->preserveNewline()) {
        if (!text.contains('\r')) {
            replaceChildrenWithText(this, text, ec);
            return;
        }
        String textWithConsistentLineBreaks = text;
        textWithConsistentLineBreaks.replace("\r\n", "\n");
        textWithConsistentLineBreaks.replace('\r', '\n');
        replaceChildrenWithText(this, textWithConsistentLineBreaks, ec);
        return;
    }

    // Add text nodes and <br> elements.
    ec = 0;
    RefPtr<DocumentFragment> fragment = textToFragment(text, ec);
    if (!ec)
        replaceChildrenWithFragment(this, fragment.release(), ec);
}

void HTMLElement::setOuterText(const String &text, ExceptionCode& ec)
{
    if (ieForbidsInsertHTML()) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }
    if (hasLocalName(colTag) || hasLocalName(colgroupTag) || hasLocalName(framesetTag) ||
        hasLocalName(headTag) || hasLocalName(htmlTag) || hasLocalName(tableTag) || 
        hasLocalName(tbodyTag) || hasLocalName(tfootTag) || hasLocalName(theadTag) ||
        hasLocalName(trTag)) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    ContainerNode* parent = parentNode();
    if (!parent) {
        ec = NO_MODIFICATION_ALLOWED_ERR;
        return;
    }

    RefPtr<Node> prev = previousSibling();
    RefPtr<Node> next = nextSibling();
    RefPtr<Node> newChild;
    ec = 0;
    
    // Convert text to fragment with <br> tags instead of linebreaks if needed.
    if (text.contains('\r') || text.contains('\n'))
        newChild = textToFragment(text, ec);
    else
        newChild = Text::create(document(), text);

    if (!this || !parentNode())
        ec = HIERARCHY_REQUEST_ERR;
    if (ec)
        return;
    parent->replaceChild(newChild.release(), this, ec);

    RefPtr<Node> node = next ? next->previousSibling() : 0;
    if (!ec && node && node->isTextNode())
        mergeWithNextTextNode(node.release(), ec);

    if (!ec && prev && prev->isTextNode())
        mergeWithNextTextNode(prev.release(), ec);
}

Node* HTMLElement::insertAdjacent(const String& where, Node* newChild, ExceptionCode& ec)
{
    // In Internet Explorer if the element has no parent and where is "beforeBegin" or "afterEnd",
    // a document fragment is created and the elements appended in the correct order. This document
    // fragment isn't returned anywhere.
    //
    // This is impossible for us to implement as the DOM tree does not allow for such structures,
    // Opera also appears to disallow such usage.

    if (equalIgnoringCase(where, "beforeBegin")) {
        ContainerNode* parent = this->parentNode();
        return (parent && parent->insertBefore(newChild, this, ec)) ? newChild : 0;
    }

    if (equalIgnoringCase(where, "afterBegin"))
        return insertBefore(newChild, firstChild(), ec) ? newChild : 0;

    if (equalIgnoringCase(where, "beforeEnd"))
        return appendChild(newChild, ec) ? newChild : 0;

    if (equalIgnoringCase(where, "afterEnd")) {
        ContainerNode* parent = this->parentNode();
        return (parent && parent->insertBefore(newChild, nextSibling(), ec)) ? newChild : 0;
    }
    
    // IE throws COM Exception E_INVALIDARG; this is the best DOM exception alternative.
    ec = NOT_SUPPORTED_ERR;
    return 0;
}

Element* HTMLElement::insertAdjacentElement(const String& where, Element* newChild, ExceptionCode& ec)
{
    if (!newChild) {
        // IE throws COM Exception E_INVALIDARG; this is the best DOM exception alternative.
        ec = TYPE_MISMATCH_ERR;
        return 0;
    }

    Node* returnValue = insertAdjacent(where, newChild, ec);
    ASSERT(!returnValue || returnValue->isElementNode());
    return static_cast<Element*>(returnValue); 
}

// Step 3 of http://www.whatwg.org/specs/web-apps/current-work/multipage/apis-in-html-documents.html#insertadjacenthtml()
static Element* contextElementForInsertion(const String& where, Element* element, ExceptionCode& ec)
{
    if (equalIgnoringCase(where, "beforeBegin") || equalIgnoringCase(where, "afterEnd")) {
        ContainerNode* parent = element->parentNode();
        if (parent && !parent->isElementNode()) {
            ec = NO_MODIFICATION_ALLOWED_ERR;
            return 0;
        }
        ASSERT(!parent || parent->isElementNode());
        return static_cast<Element*>(parent);
    }
    if (equalIgnoringCase(where, "afterBegin") || equalIgnoringCase(where, "beforeEnd"))
        return element;
    ec =  SYNTAX_ERR;
    return 0;
}

void HTMLElement::insertAdjacentHTML(const String& where, const String& markup, ExceptionCode& ec)
{
    Element* contextElement = contextElementForInsertion(where, this, ec);
    if (!contextElement)
        return;
    RefPtr<DocumentFragment> fragment = createFragmentForInnerOuterHTML(markup, contextElement, AllowScriptingContent, ec);
    if (!fragment)
        return;
    insertAdjacent(where, fragment.get(), ec);
}

void HTMLElement::insertAdjacentText(const String& where, const String& text, ExceptionCode& ec)
{
    RefPtr<Text> textNode = document()->createTextNode(text);
    insertAdjacent(where, textNode.get(), ec);
}

void HTMLElement::applyAlignmentAttributeToStyle(const Attribute& attribute, StylePropertySet* style)
{
    // Vertical alignment with respect to the current baseline of the text
    // right or left means floating images.
    int floatValue = CSSValueInvalid;
    int verticalAlignValue = CSSValueInvalid;

    const AtomicString& alignment = attribute.value();
    if (equalIgnoringCase(alignment, "absmiddle"))
        verticalAlignValue = CSSValueMiddle;
    else if (equalIgnoringCase(alignment, "absbottom"))
        verticalAlignValue = CSSValueBottom;
    else if (equalIgnoringCase(alignment, "left")) {
        floatValue = CSSValueLeft;
        verticalAlignValue = CSSValueTop;
    } else if (equalIgnoringCase(alignment, "right")) {
        floatValue = CSSValueRight;
        verticalAlignValue = CSSValueTop;
    } else if (equalIgnoringCase(alignment, "top"))
        verticalAlignValue = CSSValueTop;
    else if (equalIgnoringCase(alignment, "middle"))
        verticalAlignValue = CSSValueWebkitBaselineMiddle;
    else if (equalIgnoringCase(alignment, "center"))
        verticalAlignValue = CSSValueMiddle;
    else if (equalIgnoringCase(alignment, "bottom"))
        verticalAlignValue = CSSValueBaseline;
    else if (equalIgnoringCase(alignment, "texttop"))
        verticalAlignValue = CSSValueTextTop;

    if (floatValue != CSSValueInvalid)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyFloat, floatValue);

    if (verticalAlignValue != CSSValueInvalid)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign, verticalAlignValue);
}

bool HTMLElement::hasCustomFocusLogic() const
{
    return false;
}

bool HTMLElement::supportsFocus() const
{
    return Element::supportsFocus() || (rendererIsEditable() && parentNode() && !parentNode()->rendererIsEditable());
}

String HTMLElement::contentEditable() const
{
    const AtomicString& value = fastGetAttribute(contenteditableAttr);

    if (value.isNull())
        return "inherit";
    if (value.isEmpty() || equalIgnoringCase(value, "true"))
        return "true";
    if (equalIgnoringCase(value, "false"))
         return "false";
    if (equalIgnoringCase(value, "plaintext-only"))
        return "plaintext-only";

    return "inherit";
}

void HTMLElement::setContentEditable(const String& enabled, ExceptionCode& ec)
{
    if (equalIgnoringCase(enabled, "true"))
        setAttribute(contenteditableAttr, "true");
    else if (equalIgnoringCase(enabled, "false"))
        setAttribute(contenteditableAttr, "false");
    else if (equalIgnoringCase(enabled, "plaintext-only"))
        setAttribute(contenteditableAttr, "plaintext-only");
    else if (equalIgnoringCase(enabled, "inherit"))
        removeAttribute(contenteditableAttr);
    else
        ec = SYNTAX_ERR;
}

bool HTMLElement::draggable() const
{
    return equalIgnoringCase(getAttribute(draggableAttr), "true");
}

void HTMLElement::setDraggable(bool value)
{
    setAttribute(draggableAttr, value ? "true" : "false");
}

bool HTMLElement::spellcheck() const
{
    return isSpellCheckingEnabled();
}

void HTMLElement::setSpellcheck(bool enable)
{
    setAttribute(spellcheckAttr, enable ? "true" : "false");
}


void HTMLElement::click()
{
    dispatchSimulatedClick(0, SendNoEvents, DoNotShowPressedLook);
}

void HTMLElement::accessKeyAction(bool sendMouseEvents)
{
    dispatchSimulatedClick(0, sendMouseEvents ? SendMouseUpDownEvents : SendNoEvents);
}

String HTMLElement::title() const
{
    return getAttribute(titleAttr);
}

short HTMLElement::tabIndex() const
{
    if (supportsFocus())
        return Element::tabIndex();
    return -1;
}

void HTMLElement::setTabIndex(int value)
{
    setAttribute(tabindexAttr, String::number(value));
}

TranslateAttributeMode HTMLElement::translateAttributeMode() const
{
    const AtomicString& value = getAttribute(translateAttr);

    if (value == nullAtom)
        return TranslateAttributeInherit;
    if (equalIgnoringCase(value, "yes") || equalIgnoringCase(value, ""))
        return TranslateAttributeYes;
    if (equalIgnoringCase(value, "no"))
        return TranslateAttributeNo;

    return TranslateAttributeInherit;
}

bool HTMLElement::translate() const
{
    for (const Node* n = this; n; n = n->parentNode()) {
        if (n->isHTMLElement()) {
            TranslateAttributeMode mode = static_cast<const HTMLElement*>(n)->translateAttributeMode();
            if (mode != TranslateAttributeInherit) {
                ASSERT(mode == TranslateAttributeYes || mode == TranslateAttributeNo);
                return mode == TranslateAttributeYes;
            }
        }
    }

    // Default on the root element is translate=yes.
    return true;
}

void HTMLElement::setTranslate(bool enable)
{
    setAttribute(translateAttr, enable ? "yes" : "no");
}


PassRefPtr<HTMLCollection> HTMLElement::children()
{
    return ensureCachedHTMLCollection(NodeChildren);
}

bool HTMLElement::rendererIsNeeded(const NodeRenderingContext& context)
{
    if (hasLocalName(noscriptTag)) {
        Frame* frame = document()->frame();
        if (frame && frame->script()->canExecuteScripts(NotAboutToExecuteScript))
            return false;
    } else if (hasLocalName(noembedTag)) {
        Frame* frame = document()->frame();
        if (frame && frame->loader()->subframeLoader()->allowPlugins(NotAboutToInstantiatePlugin))
            return false;
    }
    return StyledElement::rendererIsNeeded(context);
}

RenderObject* HTMLElement::createRenderer(RenderArena* arena, RenderStyle* style)
{
    if (hasLocalName(wbrTag))
        return new (arena) RenderWordBreak(this);
    return RenderObject::createObject(this, style);
}

HTMLFormElement* HTMLElement::findFormAncestor() const
{
    for (ContainerNode* ancestor = parentNode(); ancestor; ancestor = ancestor->parentNode()) {
        if (ancestor->hasTagName(formTag))
            return static_cast<HTMLFormElement*>(ancestor);
    }
    return 0;
}

HTMLFormElement* HTMLElement::virtualForm() const
{
    return findFormAncestor();
}

static inline bool elementAffectsDirectionality(const Node* node)
{
    return node->isHTMLElement() && (node->hasTagName(bdiTag) || toHTMLElement(node)->hasAttribute(dirAttr));
}

static void setHasDirAutoFlagRecursively(Node* firstNode, bool flag, Node* lastNode = 0)
{
    firstNode->setSelfOrAncestorHasDirAutoAttribute(flag);

    Node* node = firstNode->firstChild();

    while (node) {
        if (node->selfOrAncestorHasDirAutoAttribute() == flag)
            return;

        if (elementAffectsDirectionality(node)) {
            if (node == lastNode)
                return;
            node = node->traverseNextSibling(firstNode);
            continue;
        }
        node->setSelfOrAncestorHasDirAutoAttribute(flag);
        if (node == lastNode)
            return;
        node = node->traverseNextNode(firstNode);
    }
}

void HTMLElement::childrenChanged(bool changedByParser, Node* beforeChange, Node* afterChange, int childCountDelta)
{
    StyledElement::childrenChanged(changedByParser, beforeChange, afterChange, childCountDelta);
    adjustDirectionalityIfNeededAfterChildrenChanged(beforeChange, childCountDelta);
}

bool HTMLElement::hasDirectionAuto() const
{
    const AtomicString& direction = fastGetAttribute(dirAttr);
    return (hasTagName(bdiTag) && direction == nullAtom) || equalIgnoringCase(direction, "auto");
}

TextDirection HTMLElement::directionalityIfhasDirAutoAttribute(bool& isAuto) const
{
    if (!(selfOrAncestorHasDirAutoAttribute() && hasDirectionAuto())) {
        isAuto = false;
        return LTR;
    }

    isAuto = true;
    return directionality();
}

TextDirection HTMLElement::directionality(Node** strongDirectionalityTextNode) const
{
    if (HTMLTextFormControlElement* textElement = toTextFormControl(const_cast<HTMLElement*>(this))) {
        bool hasStrongDirectionality;
        Unicode::Direction textDirection = textElement->value().defaultWritingDirection(&hasStrongDirectionality);
        if (strongDirectionalityTextNode)
            *strongDirectionalityTextNode = hasStrongDirectionality ? textElement : 0;
        return (textDirection == Unicode::LeftToRight) ? LTR : RTL;
    }

    Node* node = firstChild();
    while (node) {
        // Skip bdi, script, style and text form controls.
        if (equalIgnoringCase(node->nodeName(), "bdi") || node->hasTagName(scriptTag) || node->hasTagName(styleTag) 
            || (node->isElementNode() && toElement(node)->isTextFormControl())) {
            node = node->traverseNextSibling(this);
            continue;
        }

        // Skip elements with valid dir attribute
        if (node->isElementNode()) {
            AtomicString dirAttributeValue = toElement(node)->fastGetAttribute(dirAttr);
            if (equalIgnoringCase(dirAttributeValue, "rtl") || equalIgnoringCase(dirAttributeValue, "ltr") || equalIgnoringCase(dirAttributeValue, "auto")) {
                node = node->traverseNextSibling(this);
                continue;
            }
        }

        if (node->isTextNode()) {
            bool hasStrongDirectionality;
            WTF::Unicode::Direction textDirection = node->textContent(true).defaultWritingDirection(&hasStrongDirectionality);
            if (hasStrongDirectionality) {
                if (strongDirectionalityTextNode)
                    *strongDirectionalityTextNode = node;
                return (textDirection == WTF::Unicode::LeftToRight) ? LTR : RTL;
            }
        }
        node = node->traverseNextNode(this);
    }
    if (strongDirectionalityTextNode)
        *strongDirectionalityTextNode = 0;
    return LTR;
}

void HTMLElement::dirAttributeChanged(const AtomicString& value)
{
    Element* parent = parentElement();

    if (parent && parent->isHTMLElement() && parent->selfOrAncestorHasDirAutoAttribute())
        toHTMLElement(parent)->adjustDirectionalityIfNeededAfterChildAttributeChanged(this);

    if (equalIgnoringCase(value, "auto"))
        calculateAndAdjustDirectionality();
}

void HTMLElement::adjustDirectionalityIfNeededAfterChildAttributeChanged(Element* child)
{
    ASSERT(selfOrAncestorHasDirAutoAttribute());
    Node* strongDirectionalityTextNode;
    TextDirection textDirection = directionality(&strongDirectionalityTextNode);
    setHasDirAutoFlagRecursively(child, false);
    if (renderer() && renderer()->style() && renderer()->style()->direction() != textDirection) {
        Element* elementToAdjust = this;
        for (; elementToAdjust; elementToAdjust = elementToAdjust->parentElement()) {
            if (elementAffectsDirectionality(elementToAdjust)) {
                elementToAdjust->setNeedsStyleRecalc();
                return;
            }
        }
    }
}

void HTMLElement::calculateAndAdjustDirectionality()
{
    Node* strongDirectionalityTextNode;
    TextDirection textDirection = directionality(&strongDirectionalityTextNode);
    setHasDirAutoFlagRecursively(this, true, strongDirectionalityTextNode);
    if (renderer() && renderer()->style() && renderer()->style()->direction() != textDirection)
        setNeedsStyleRecalc();
}

void HTMLElement::adjustDirectionalityIfNeededAfterChildrenChanged(Node* beforeChange, int childCountDelta)
{
    if ((!document() || document()->renderer()) && childCountDelta < 0) {
        Node* node = beforeChange ? beforeChange->traverseNextSibling() : 0;
        for (int counter = 0; node && counter < childCountDelta; counter++, node = node->traverseNextSibling()) {
            if (elementAffectsDirectionality(node))
                continue;

            setHasDirAutoFlagRecursively(node, false);
        }
    }

    if (!selfOrAncestorHasDirAutoAttribute())
        return;

    Node* oldMarkedNode = beforeChange ? beforeChange->traverseNextSibling() : 0;
    while (oldMarkedNode && elementAffectsDirectionality(oldMarkedNode))
        oldMarkedNode = oldMarkedNode->traverseNextSibling(this);
    if (oldMarkedNode)
        setHasDirAutoFlagRecursively(oldMarkedNode, false);

    for (Element* elementToAdjust = this; elementToAdjust; elementToAdjust = elementToAdjust->parentElement()) {
        if (elementAffectsDirectionality(elementToAdjust)) {
            toHTMLElement(elementToAdjust)->calculateAndAdjustDirectionality();
            return;
        }
    }
}

bool HTMLElement::isURLAttribute(const Attribute& attribute) const
{
#if ENABLE(MICRODATA)
    return attribute.name() == itemidAttr;
#else
    UNUSED_PARAM(attribute);
    return false;
#endif
}

#if ENABLE(MICRODATA)
void HTMLElement::setItemValue(const String& value, ExceptionCode& ec)
{
    if (!hasAttribute(itempropAttr) || hasAttribute(itemscopeAttr)) {
        ec = INVALID_ACCESS_ERR;
        return;
    }

    setItemValueText(value, ec);
}

PassRefPtr<MicroDataItemValue> HTMLElement::itemValue() const
{
    if (!hasAttribute(itempropAttr))
        return 0;

    if (hasAttribute(itemscopeAttr))
        return MicroDataItemValue::createFromNode(const_cast<HTMLElement* const>(this));

    return MicroDataItemValue::createFromString(itemValueText());
}

String HTMLElement::itemValueText() const
{
    return textContent(true);
}

void HTMLElement::setItemValueText(const String& value, ExceptionCode& ec)
{
    setTextContent(value, ec);
}

PassRefPtr<HTMLPropertiesCollection> HTMLElement::properties()
{
    return static_cast<HTMLPropertiesCollection*>(ensureCachedHTMLCollection(ItemProperties).get());
}

void HTMLElement::getItemRefElements(Vector<HTMLElement*>& itemRefElements)
{
    if (!fastHasAttribute(itemscopeAttr))
        return;

    if (!fastHasAttribute(itemrefAttr) || !itemRef()->length()) {
        itemRefElements.append(this);
        return;
    }

    DOMSettableTokenList* itemRefs = itemRef();
    RefPtr<DOMSettableTokenList> processedItemRef = DOMSettableTokenList::create();

    Node* rootNode;
    if (inDocument())
        rootNode = document();
    else {
        rootNode = this;
        while (Node* parent = rootNode->parentNode())
            rootNode = parent;
    }

    for (Node* current = rootNode; current; current = current->traverseNextNode(rootNode)) {
        if (!current->isHTMLElement())
            continue;
        HTMLElement* element = toHTMLElement(current);

        if (element == this) {
            itemRefElements.append(element);
            continue;
        }

        const AtomicString& id = element->getIdAttribute();
        if (!processedItemRef->tokens().contains(id) && itemRefs->tokens().contains(id)) {
            processedItemRef->setValue(id);
            if (!element->isDescendantOf(this))
                itemRefElements.append(element);
        }
    }
}
#endif

void HTMLElement::addHTMLLengthToStyle(StylePropertySet* style, CSSPropertyID propertyID, const String& value)
{
    // FIXME: This function should not spin up the CSS parser, but should instead just figure out the correct
    // length unit and make the appropriate parsed value.

    // strip attribute garbage..
    StringImpl* v = value.impl();
    if (v) {
        unsigned int l = 0;

        while (l < v->length() && (*v)[l] <= ' ')
            l++;

        for (; l < v->length(); l++) {
            UChar cc = (*v)[l];
            if (cc > '9')
                break;
            if (cc < '0') {
                if (cc == '%' || cc == '*')
                    l++;
                if (cc != '.')
                    break;
            }
        }

        if (l != v->length()) {
            addPropertyToPresentationAttributeStyle(style, propertyID, v->substring(0, l));
            return;
        }
    }

    addPropertyToPresentationAttributeStyle(style, propertyID, value);
}

static RGBA32 parseColorStringWithCrazyLegacyRules(const String& colorString)
{
    // Per spec, only look at the first 128 digits of the string.
    const size_t maxColorLength = 128;
    // We'll pad the buffer with two extra 0s later, so reserve two more than the max.
    Vector<char, maxColorLength+2> digitBuffer;

    size_t i = 0;
    // Skip a leading #.
    if (colorString[0] == '#')
        i = 1;

    // Grab the first 128 characters, replacing non-hex characters with 0.
    // Non-BMP characters are replaced with "00" due to them appearing as two "characters" in the String.
    for (; i < colorString.length() && digitBuffer.size() < maxColorLength; i++) {
        if (!isASCIIHexDigit(colorString[i]))
            digitBuffer.append('0');
        else
            digitBuffer.append(colorString[i]);
    }

    if (!digitBuffer.size())
        return Color::black;

    // Pad the buffer out to at least the next multiple of three in size.
    digitBuffer.append('0');
    digitBuffer.append('0');

    if (digitBuffer.size() < 6)
        return makeRGB(toASCIIHexValue(digitBuffer[0]), toASCIIHexValue(digitBuffer[1]), toASCIIHexValue(digitBuffer[2]));

    // Split the digits into three components, then search the last 8 digits of each component.
    ASSERT(digitBuffer.size() >= 6);
    size_t componentLength = digitBuffer.size() / 3;
    size_t componentSearchWindowLength = min<size_t>(componentLength, 8);
    size_t redIndex = componentLength - componentSearchWindowLength;
    size_t greenIndex = componentLength * 2 - componentSearchWindowLength;
    size_t blueIndex = componentLength * 3 - componentSearchWindowLength;
    // Skip digits until one of them is non-zero, or we've only got two digits left in the component.
    while (digitBuffer[redIndex] == '0' && digitBuffer[greenIndex] == '0' && digitBuffer[blueIndex] == '0' && (componentLength - redIndex) > 2) {
        redIndex++;
        greenIndex++;
        blueIndex++;
    }
    ASSERT(redIndex + 1 < componentLength);
    ASSERT(greenIndex >= componentLength);
    ASSERT(greenIndex + 1 < componentLength * 2);
    ASSERT(blueIndex >= componentLength * 2);
    ASSERT(blueIndex + 1 < digitBuffer.size());

    int redValue = toASCIIHexValue(digitBuffer[redIndex], digitBuffer[redIndex + 1]);
    int greenValue = toASCIIHexValue(digitBuffer[greenIndex], digitBuffer[greenIndex + 1]);
    int blueValue = toASCIIHexValue(digitBuffer[blueIndex], digitBuffer[blueIndex + 1]);
    return makeRGB(redValue, greenValue, blueValue);
}

// Color parsing that matches HTML's "rules for parsing a legacy color value"
void HTMLElement::addHTMLColorToStyle(StylePropertySet* style, CSSPropertyID propertyID, const String& attributeValue)
{
    // An empty string doesn't apply a color. (One containing only whitespace does, which is why this check occurs before stripping.)
    if (attributeValue.isEmpty())
        return;

    String colorString = attributeValue.stripWhiteSpace();

    // "transparent" doesn't apply a color either.
    if (equalIgnoringCase(colorString, "transparent"))
        return;

    // If the string is a named CSS color or a 3/6-digit hex color, use that.
    Color parsedColor(colorString);
    if (!parsedColor.isValid())
        parsedColor.setRGB(parseColorStringWithCrazyLegacyRules(colorString));

    style->setProperty(propertyID, cssValuePool().createColorValue(parsedColor.rgb()));
}

} // namespace WebCore

#ifndef NDEBUG

// For use in the debugger
void dumpInnerHTML(WebCore::HTMLElement*);

void dumpInnerHTML(WebCore::HTMLElement* element)
{
    printf("%s\n", element->innerHTML().ascii().data());
}
#endif
