/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "qwebelement.h"

#include "QWebFrameAdapter.h"
#include "qwebelement_p.h"
#include <JavaScriptCore/APICast.h>
#include <JavaScriptCore/Completion.h>
#include <QPainter>
#include <WebCore/DocumentFragment.h>
#include <WebCore/FrameView.h>
#include <WebCore/FullscreenManager.h>
#include <WebCore/HTMLElement.h>
#include <WebCore/JSDocument.h>
#include <WebCore/JSElement.h>
#include <WebCore/RenderElement.h>
#include <WebCore/ScriptController.h>
#include <WebCore/ScriptSourceCode.h>
#include <WebCore/StaticNodeList.h>
#include <WebCore/markup.h>
#include <WebCore/qt_runtime.h>
#include <wtf/NakedPtr.h>

using namespace WebCore;

class QWebElementPrivate {
public:
};

/*!
    \class QWebElement
    \since 4.6
    \brief The QWebElement class provides convenient access to DOM elements in
    a QWebFrame.
    \inmodule QtWebKit

    A QWebElement object allows easy access to the document model, represented
    by a tree-like structure of DOM elements. The root of the tree is called
    the document element and can be accessed using
    QWebFrame::documentElement().

    Specific elements can be accessed using findAll() and findFirst(). These
    elements are identified using CSS selectors. The code snippet below
    demonstrates the use of findAll().

    \snippet webkitsnippets/webelement/main.cpp FindAll

    The first list contains all \c span elements in the document. The second
    list contains \c span elements that are children of \c p, classified with
    \c intro.

    Using findFirst() is more efficient than calling findAll(), and extracting
    the first element only in the list returned.

    Alternatively you can traverse the document manually using firstChild() and
    nextSibling():

    \snippet webkitsnippets/webelement/main.cpp Traversing with QWebElement

    Individual elements can be inspected or changed using methods such as attribute()
    or setAttribute(). For examle, to capture the user's input in a text field for later
    use (auto-completion), a browser could do something like this:

    \snippet webkitsnippets/webelement/main.cpp autocomplete1

    When the same page is later revisited, the browser can fill in the text field automatically
    by modifying the value attribute of the input element:

    \snippet webkitsnippets/webelement/main.cpp autocomplete2

    Another use case is to emulate a click event on an element. The following
    code snippet demonstrates how to call the JavaScript DOM method click() of
    a submit button:

    \snippet webkitsnippets/webelement/main.cpp Calling a DOM element method

    The underlying content of QWebElement is explicitly shared. Creating a copy
    of a QWebElement does not create a copy of the content. Instead, both
    instances point to the same element.

    The contents of child elements can be converted to plain text with
    toPlainText(); to XHTML using toInnerXml(). To include the element's tag in
    the output, use toOuterXml().

    It is possible to replace the contents of child elements using
    setPlainText() and setInnerXml(). To replace the element itself and its
    contents, use setOuterXml().

    \section1 Examples

    The \l{DOM Traversal Example} shows one way to traverse documents in a running
    example.

    The \l{Simple Selector Example} can be used to experiment with the searching
    features of this class and provides sample code you can start working with.
*/

/*!
    Constructs a null web element.
*/
QWebElement::QWebElement()
    : d(0)
    , m_element(0)
{
}

/*!
    \internal
*/
QWebElement::QWebElement(WebCore::Element* domElement)
    : d(0)
    , m_element(domElement)
{
    if (m_element)
        m_element->ref();
}

/*!
    Constructs a copy of \a other.
*/
QWebElement::QWebElement(const QWebElement &other)
    : d(0)
    , m_element(other.m_element)
{
    if (m_element)
        m_element->ref();
}

/*!
    Assigns \a other to this element and returns a reference to this element.
*/
QWebElement &QWebElement::operator=(const QWebElement &other)
{
    // ### handle "d" assignment
    if (this != &other) {
        Element *otherElement = other.m_element;
        if (otherElement)
            otherElement->ref();
        if (m_element)
            m_element->deref();
        m_element = otherElement;
    }
    return *this;
}

/*!
    Destroys the element. However, the underlying DOM element is not destroyed.
*/
QWebElement::~QWebElement()
{
    delete d;
    if (m_element)
        m_element->deref();
}

bool QWebElement::operator==(const QWebElement& o) const
{
    return m_element == o.m_element;
}

bool QWebElement::operator!=(const QWebElement& o) const
{
    return m_element != o.m_element;
}

/*!
    Returns true if the element is a null element; otherwise returns false.
*/
bool QWebElement::isNull() const
{
    return !m_element;
}

/*!
    Returns a new list of child elements matching the given CSS selector
    \a selectorQuery. If there are no matching elements, an empty list is
    returned.

    \l{Standard CSS selector} syntax is used for the query.

    This method is equivalent to Element::querySelectorAll in the \l{DOM Selectors API}.

    \note This search is performed recursively.

    \sa findFirst()
*/
QWebElementCollection QWebElement::findAll(const QString &selectorQuery) const
{
    return QWebElementCollection(*this, selectorQuery);
}

/*!
    Returns the first child element that matches the given CSS selector
    \a selectorQuery.

    \l{Standard CSS selector} syntax is used for the query.

    This method is equivalent to Element::querySelector in the \l{DOM Selectors API}.

    \note This search is performed recursively.

    \sa findAll()
*/
QWebElement QWebElement::findFirst(const QString &selectorQuery) const
{
    if (!m_element)
        return QWebElement();
    auto queryResult = m_element->querySelector(selectorQuery);
    if (queryResult.hasException())
        return QWebElement();
    return QWebElement(queryResult.releaseReturnValue());
}

/*!
    Replaces the existing content of this element with \a text.

    This is equivalent to setting the HTML innerText property.

    \sa toPlainText()
*/
void QWebElement::setPlainText(const QString &text)
{
    if (!m_element || !m_element->isHTMLElement())
        return;
    static_cast<HTMLElement*>(m_element)->setInnerText(text);
}

/*!
    Returns the text between the start and the end tag of this
    element.

    This is equivalent to reading the HTML innerText property.

    \sa setPlainText()
*/
QString QWebElement::toPlainText() const
{
    if (!m_element || !m_element->isHTMLElement())
        return QString();
    return static_cast<HTMLElement*>(m_element)->innerText();
}

/*!
    Replaces the contents of this element as well as its own tag with
    \a markup. The string may contain HTML or XML tags, which is parsed and
    formatted before insertion into the document.

    \note This is currently only implemented for (X)HTML elements.

    \sa toOuterXml(), toInnerXml(), setInnerXml()
*/
void QWebElement::setOuterXml(const QString &markup)
{
    if (!m_element || !m_element->isHTMLElement())
        return;
    static_cast<HTMLElement*>(m_element)->setOuterHTML(markup);
}

/*!
    Returns this element converted to XML, including the start and the end
    tags as well as its attributes.

    \note This is currently implemented for (X)HTML elements only.

    \note The format of the markup returned will obey the namespace of the
    document containing the element. This means the return value will obey XML
    formatting rules, such as self-closing tags, only if the document is
    'text/xhtml+xml'.

    \sa setOuterXml(), setInnerXml(), toInnerXml()
*/
QString QWebElement::toOuterXml() const
{
    if (!m_element || !m_element->isHTMLElement())
        return QString();

    return static_cast<HTMLElement*>(m_element)->outerHTML();
}

/*!
    Replaces the contents of this element with \a markup. The string may
    contain HTML or XML tags, which is parsed and formatted before insertion
    into the document.

    \note This is currently implemented for (X)HTML elements only.

    \sa toInnerXml(), toOuterXml(), setOuterXml()
*/
void QWebElement::setInnerXml(const QString &markup)
{
    if (!m_element || !m_element->isHTMLElement())
        return;
    static_cast<HTMLElement*>(m_element)->setInnerHTML(markup);
}

/*!
    Returns the XML content between the element's start and end tags.

    \note This is currently implemented for (X)HTML elements only.

    \note The format of the markup returned will obey the namespace of the
    document containing the element. This means the return value will obey XML
    formatting rules, such as self-closing tags, only if the document is
    'text/xhtml+xml'.

    \sa setInnerXml(), setOuterXml(), toOuterXml()
*/
QString QWebElement::toInnerXml() const
{
    if (!m_element || !m_element->isHTMLElement())
        return QString();

    return static_cast<HTMLElement*>(m_element)->innerHTML();
}

/*!
    Adds an attribute with the given \a name and \a value. If an attribute with
    the same name exists, its value is replaced by \a value.

    \sa attribute(), attributeNS(), setAttributeNS()
*/
void QWebElement::setAttribute(const QString &name, const QString &value)
{
    if (!m_element)
        return;
    m_element->setAttribute(String(name), String(value));
}

/*!
    Adds an attribute with the given \a name in \a namespaceUri with \a value.
    If an attribute with the same name exists, its value is replaced by
    \a value.

    \sa attributeNS(), attribute(), setAttribute()
*/
void QWebElement::setAttributeNS(const QString &namespaceUri, const QString &name, const QString &value)
{
    if (!m_element)
        return;
    m_element->setAttributeNS(String(namespaceUri), String(name), String(value));
}

/*!
    Returns the attribute with the given \a name. If the attribute does not
    exist, \a defaultValue is returned.

    \sa setAttribute(), setAttributeNS(), attributeNS()
*/
QString QWebElement::attribute(const QString &name, const QString &defaultValue) const
{
    if (!m_element)
        return QString();
    if (m_element->hasAttribute(String(name)))
        return m_element->getAttribute(String(name)).string();
    return defaultValue;
}

/*!
    Returns the attribute with the given \a name in \a namespaceUri. If the
    attribute does not exist, \a defaultValue is returned.

    \sa setAttributeNS(), setAttribute(), attribute()
*/
QString QWebElement::attributeNS(const QString &namespaceUri, const QString &name, const QString &defaultValue) const
{
    if (!m_element)
        return QString();
    if (m_element->hasAttributeNS(String(namespaceUri), String(name)))
        return m_element->getAttributeNS(String(namespaceUri), String(name)).string();
    return defaultValue;
}

/*!
    Returns true if this element has an attribute with the given \a name;
    otherwise returns false.

    \sa attribute(), setAttribute()
*/
bool QWebElement::hasAttribute(const QString &name) const
{
    if (!m_element)
        return false;
    return m_element->hasAttribute(String(name));
}

/*!
    Returns true if this element has an attribute with the given \a name, in
    \a namespaceUri; otherwise returns false.

    \sa attributeNS(), setAttributeNS()
*/
bool QWebElement::hasAttributeNS(const QString &namespaceUri, const QString &name) const
{
    if (!m_element)
        return false;
    return m_element->hasAttributeNS(String(namespaceUri), String(name));
}

/*!
    Removes the attribute with the given \a name from this element.

    \sa attribute(), setAttribute(), hasAttribute()
*/
void QWebElement::removeAttribute(const QString &name)
{
    if (!m_element)
        return;
    m_element->removeAttribute(String(name));
}

/*!
    Removes the attribute with the given \a name, in \a namespaceUri, from this
    element.

    \sa attributeNS(), setAttributeNS(), hasAttributeNS()
*/
void QWebElement::removeAttributeNS(const QString &namespaceUri, const QString &name)
{
    if (!m_element)
        return;
    m_element->removeAttributeNS(String(namespaceUri), String(name));
}

/*!
    Returns true if the element has any attributes defined; otherwise returns
    false;

    \sa attribute(), setAttribute()
*/
bool QWebElement::hasAttributes() const
{
    if (!m_element)
        return false;
    return m_element->hasAttributes();
}

/*!
    Return the list of attributes for the namespace given as \a namespaceUri.

    \sa attribute(), setAttribute()
*/
QStringList QWebElement::attributeNames(const QString& namespaceUri) const
{
    if (!m_element)
        return QStringList();

    QStringList attributeNameList;
    if (m_element->hasAttributes()) {
        const String namespaceUriString(namespaceUri); // convert QString -> String once
        const unsigned attrsCount = m_element->attributeCount();
        for (unsigned i = 0; i < attrsCount; ++i) {
            const Attribute& attribute = m_element->attributeAt(i);
            if (namespaceUriString == attribute.namespaceURI())
                attributeNameList.append(attribute.localName().string());
        }
    }
    return attributeNameList;
}

/*!
    Returns true if the element has keyboard input focus; otherwise, returns false

    \sa setFocus()
*/
bool QWebElement::hasFocus() const
{
    if (!m_element)
        return false;
    return m_element == m_element->document().focusedElement();
}

/*!
    Gives keyboard input focus to this element

    \sa hasFocus()
*/
void QWebElement::setFocus()
{
    if (!m_element)
        return;
    if (m_element->isFocusable())
        m_element->document().setFocusedElement(m_element);
}

/*!
    Returns the geometry of this element, relative to its containing frame.

    \sa tagName()
*/
QRect QWebElement::geometry() const
{
    if (!m_element)
        return QRect();

    auto* renderer = m_element->renderer();
    if (!renderer)
        return QRect();

    return renderer->absoluteBoundingBoxRect();
}

/*!
    Returns the tag name of this element.

    \sa geometry()
*/
QString QWebElement::tagName() const
{
    if (!m_element)
        return QString();
    return m_element->tagName();
}

/*!
    Returns the namespace prefix of the element. If the element has no\
    namespace prefix, empty string is returned.
*/
QString QWebElement::prefix() const
{
    if (!m_element)
        return QString();
    return m_element->prefix().string();
}

/*!
    Returns the local name of the element. If the element does not use
    namespaces, an empty string is returned.
*/
QString QWebElement::localName() const
{
    if (!m_element)
        return QString();
    return m_element->localName().string();
}

/*!
    Returns the namespace URI of this element. If the element has no namespace
    URI, an empty string is returned.
*/
QString QWebElement::namespaceUri() const
{
    if (!m_element)
        return QString();
    return m_element->namespaceURI().string();
}

/*!
    Returns the parent element of this elemen. If this element is the root
    document element, a null element is returned.
*/
QWebElement QWebElement::parent() const
{
    if (m_element)
        return QWebElement(m_element->parentElement());
    return QWebElement();
}

/*!
    Returns the element's first child.

    \sa lastChild(), previousSibling(), nextSibling()
*/
QWebElement QWebElement::firstChild() const
{
    if (!m_element)
        return QWebElement();
    for (Node* child = m_element->firstChild(); child; child = child->nextSibling()) {
        if (!child->isElementNode())
            continue;
        Element* e = downcast<Element>(child);
        return QWebElement(e);
    }
    return QWebElement();
}

/*!
    Returns the element's last child.

    \sa firstChild(), previousSibling(), nextSibling()
*/
QWebElement QWebElement::lastChild() const
{
    if (!m_element)
        return QWebElement();
    for (Node* child = m_element->lastChild(); child; child = child->previousSibling()) {
        if (!child->isElementNode())
            continue;
        Element* e = downcast<Element>(child);
        return QWebElement(e);
    }
    return QWebElement();
}

/*!
    Returns the element's next sibling.

    \sa firstChild(), previousSibling(), lastChild()
*/
QWebElement QWebElement::nextSibling() const
{
    if (!m_element)
        return QWebElement();
    for (Node* sib = m_element->nextSibling(); sib; sib = sib->nextSibling()) {
        if (!sib->isElementNode())
            continue;
        Element* e = downcast<Element>(sib);
        return QWebElement(e);
    }
    return QWebElement();
}

/*!
    Returns the element's previous sibling.

    \sa firstChild(), nextSibling(), lastChild()
*/
QWebElement QWebElement::previousSibling() const
{
    if (!m_element)
        return QWebElement();
    for (Node* sib = m_element->previousSibling(); sib; sib = sib->previousSibling()) {
        if (!sib->isElementNode())
            continue;
        Element* e = downcast<Element>(sib);
        return QWebElement(e);
    }
    return QWebElement();
}

/*!
    Returns the document which this element belongs to.
*/
QWebElement QWebElement::document() const
{
    if (!m_element)
        return QWebElement();
    return QWebElement(m_element->document().documentElement());
}

/*!
    Returns the web frame which this element is a part of. If the element is a
    null element, null is returned.
*/
QWebFrame *QWebElement::webFrame() const
{
    if (!m_element)
        return 0;

    Frame* frame = m_element->document().frame();
    if (!frame)
        return 0;
    QWebFrameAdapter* frameAdapter = QWebFrameAdapter::kit(frame);
    return frameAdapter->apiHandle();
}

static bool setupScriptContext(WebCore::Element* element, JSC::ExecState*& state)
{
    if (!element)
        return false;

    Frame* frame = element->document().frame();
    if (!frame)
        return false;

    state = frame->script().globalObject(mainThreadNormalWorld())->globalExec();
    if (!state)
        return false;

    return true;
}

/*!
    Executes \a scriptSource with this element as \c this object
    and returns the result of the last executed statement.

    \note This method may be very inefficient if \a scriptSource returns
    a DOM element as a result. See \l{QWebFrame::evaluateJavaScript()}
    for more details.

    \sa QWebFrame::evaluateJavaScript()
*/
QVariant QWebElement::evaluateJavaScript(const QString& scriptSource)
{
    if (scriptSource.isEmpty())
        return QVariant();

    JSC::ExecState* state = nullptr;

    if (!setupScriptContext(m_element, state))
        return QVariant();

    JSC::JSLockHolder lock(state);
    RefPtr<Element> protect = m_element;

    JSC::JSValue thisValue = toJS(state, toJSDOMWindow(m_element->document().frame(), currentWorld(*state)), m_element);
    if (!thisValue)
        return QVariant();

    ScriptSourceCode sourceCode(scriptSource);

    NakedPtr<JSC::Exception> evaluationException;
    JSC::JSValue evaluationResult = JSC::evaluate(state, sourceCode.jsSourceCode(), thisValue, evaluationException);
    if (evaluationException)
        return QVariant();
    JSValueRef evaluationResultRef = toRef(state, evaluationResult);

    int distance = 0;
    JSValueRef* ignoredException = 0;
    return JSC::Bindings::convertValueToQVariant(toRef(state), evaluationResultRef, QMetaType::Void, &distance, ignoredException);
}

/*!
    \enum QWebElement::StyleResolveStrategy

    This enum describes how QWebElement's styleProperty resolves the given
    property name.

    \value InlineStyle Return the property value as it is defined in
           the element, without respecting style inheritance and other CSS
           rules.
    \value CascadedStyle The property's value is determined using the
           inheritance and importance rules defined in the document's
           stylesheet.
    \value ComputedStyle The property's value is the absolute value
           of the style property resolved from the environment.
*/

/*!
    Returns the value of the style with the given \a name using the specified
    \a strategy. If a style with \a name does not exist, an empty string is
    returned.

    In CSS, the cascading part depends on which CSS rule has priority and is
    thus applied. Generally, the last defined rule has priority. Thus, an
    inline style rule has priority over an embedded block style rule, which
    in return has priority over an external style rule.

    If the "!important" declaration is set on one of those, the declaration
    receives highest priority, unless other declarations also use the
    "!important" declaration. Then, the last "!important" declaration takes
    predecence.

    \sa setStyleProperty()
*/

QString QWebElement::styleProperty(const QString &name, StyleResolveStrategy strategy) const
{
//    if (!m_element || !m_element->isStyledElement())
//        return QString();

//    CSSPropertyID propID = cssPropertyID(String(name));

//    if (!propID)
//        return QString();

//    if (strategy == InlineStyle) {
//        const StyleProperties* style = static_cast<StyledElement*>(m_element)->inlineStyle();
//        if (!style)
//            return QString();
//        return style->getPropertyValue(propID);
//    }

//    if (strategy == CascadedStyle) {
//        const StyleProperties* style = static_cast<StyledElement*>(m_element)->inlineStyle();
//        if (style && style->propertyIsImportant(propID))
//            return style->getPropertyValue(propID);

//        // We are going to resolve the style property by walking through the
//        // list of non-inline matched CSS rules for the element, looking for
//        // the highest priority definition.

//        // Get an array of matched CSS rules for the given element sorted
//        // by importance and inheritance order. This include external CSS
//        // declarations, as well as embedded and inline style declarations.

//        Document& document = m_element->document();
//        Vector<RefPtr<StyleRule>> rules = document.ensureStyleResolver().styleRulesForElement(m_element, StyleResolver::AuthorCSSRules | StyleResolver::CrossOriginCSSRules);
//        for (int i = rules.size(); i > 0; --i) {
//            if (!rules[i - 1]->isStyleRule())
//                continue;
//            StyleRule* styleRule = static_cast<StyleRule*>(rules[i - 1].get());

//            if (styleRule->properties().propertyIsImportant(propID))
//                return styleRule->properties().getPropertyValue(propID);

//            if (!style || style->getPropertyValue(propID).isEmpty())
//                style = &styleRule->properties();
//        }

//        if (!style)
//            return QString();
//        return style->getPropertyValue(propID);
//    }

//    if (strategy == ComputedStyle) {
//        if (!m_element || !m_element->isStyledElement())
//            return QString();

//        RefPtr<CSSComputedStyleDeclaration> style = CSSComputedStyleDeclaration::create(m_element, true);
//        if (!propID || !style)
//            return QString();

//        return style->getPropertyValue(propID);
//    }

    return QString();
}

/*!
    Sets the value of the inline style with the given \a name to \a value.

    Setting a value, does not necessarily mean that it will become the applied
    value, due to the fact that the style property's value might have been set
    earlier with a higher priority in external or embedded style declarations.

    In order to ensure that the value will be applied, you may have to append
    "!important" to the value.
*/
void QWebElement::setStyleProperty(const QString &name, const QString &value)
{
    if (!m_element || !m_element->isStyledElement())
        return;

    // Do the parsing of the token manually since WebCore isn't doing this for us anymore.
    const QLatin1String importantToken("!important");
    QString adjustedValue(value);
    bool important = false;
    if (adjustedValue.contains(importantToken)) {
        important = true;
        adjustedValue.remove(importantToken);
        adjustedValue = adjustedValue.trimmed();
    }

//    CSSPropertyID propID = cssPropertyID(name);
//    static_cast<StyledElement*>(m_element)->setInlineStyleProperty(propID, adjustedValue, important);
}

/*!
    Returns the list of classes of this element.
*/
QStringList QWebElement::classes() const
{
    if (!hasAttribute(QLatin1String("class")))
        return QStringList();

    QStringList classes =  attribute(QLatin1String("class")).simplified().split(QLatin1Char(' '), QString::SkipEmptyParts);
    classes.removeDuplicates();
    return classes;
}

/*!
    Returns true if this element has a class with the given \a name; otherwise
    returns false.
*/
bool QWebElement::hasClass(const QString &name) const
{
    QStringList list = classes();
    return list.contains(name);
}

/*!
    Adds the specified class with the given \a name to the element.
*/
void QWebElement::addClass(const QString &name)
{
    QStringList list = classes();
    if (!list.contains(name)) {
        list.append(name);
        QString value = list.join(QLatin1String(" "));
        setAttribute(QLatin1String("class"), value);
    }
}

/*!
    Removes the specified class with the given \a name from the element.
*/
void QWebElement::removeClass(const QString &name)
{
    QStringList list = classes();
    if (list.contains(name)) {
        list.removeAll(name);
        QString value = list.join(QLatin1String(" "));
        setAttribute(QLatin1String("class"), value);
    }
}

/*!
    Adds the specified class with the given \a name if it is not present. If
    the class is already present, it will be removed.
*/
void QWebElement::toggleClass(const QString &name)
{
    QStringList list = classes();
    if (list.contains(name))
        list.removeAll(name);
    else
        list.append(name);

    QString value = list.join(QLatin1String(" "));
    setAttribute(QLatin1String("class"), value);
}

/*!
    Appends the given \a element as the element's last child.

    If \a element is the child of another element, it is re-parented to this
    element. If \a element is a child of this element, then its position in
    the list of children is changed.

    Calling this function on a null element does nothing.

    \sa prependInside(), prependOutside(), appendOutside()
*/
void QWebElement::appendInside(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    m_element->appendChild(*element.m_element);
}

/*!
    Appends the result of parsing \a markup as the element's last child.

    Calling this function on a null element does nothing.

    \sa prependInside(), prependOutside(), appendOutside()
*/
void QWebElement::appendInside(const QString &markup)
{
    if (!m_element)
        return;

    auto createFragmentResult = createContextualFragment(*m_element, markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    m_element->appendChild(createFragmentResult.releaseReturnValue());
}

/*!
    Prepends \a element as the element's first child.

    If \a element is the child of another element, it is re-parented to this
    element. If \a element is a child of this element, then its position in
    the list of children is changed.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependOutside(), appendOutside()
*/
void QWebElement::prependInside(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    if (m_element->hasChildNodes())
        m_element->insertBefore(*element.m_element, m_element->firstChild());
    else
        m_element->appendChild(*element.m_element);
}

/*!
    Prepends the result of parsing \a markup as the element's first child.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependOutside(), appendOutside()
*/
void QWebElement::prependInside(const QString &markup)
{
    if (!m_element)
        return;

    auto createFragmentResult = createContextualFragment(*m_element, markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    if (m_element->hasChildNodes())
        m_element->insertBefore(createFragmentResult.releaseReturnValue(), m_element->firstChild());
    else
        m_element->appendChild(createFragmentResult.releaseReturnValue());
}


/*!
    Inserts the given \a element before this element.

    If \a element is the child of another element, it is re-parented to the
    parent of this element.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependInside(), appendOutside()
*/
void QWebElement::prependOutside(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    if (!m_element->parentNode())
        return;

    m_element->parentNode()->insertBefore(*element.m_element, m_element);
}

/*!
    Inserts the result of parsing \a markup before this element.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependInside(), appendOutside()
*/
void QWebElement::prependOutside(const QString &markup)
{
    if (!m_element)
        return;

    Node* parent = m_element->parentNode();
    if (!parent)
        return;

    if (!parent->isElementNode())
        return;

    auto createFragmentResult = createContextualFragment(downcast<Element>(*parent), markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    parent->insertBefore(createFragmentResult.releaseReturnValue(), m_element);
}

/*!
    Inserts the given \a element after this element.

    If \a element is the child of another element, it is re-parented to the
    parent of this element.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependInside(), prependOutside()
*/
void QWebElement::appendOutside(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    if (!m_element->parentNode())
        return;

    if (!m_element->nextSibling())
        m_element->parentNode()->appendChild(*element.m_element);
    else
        m_element->parentNode()->insertBefore(*element.m_element, m_element->nextSibling());
}

/*!
    Inserts the result of parsing \a markup after this element.

    Calling this function on a null element does nothing.

    \sa appendInside(), prependInside(), prependOutside()
*/
void QWebElement::appendOutside(const QString &markup)
{
    if (!m_element)
        return;

    Node* parent = m_element->parentNode();
    if (!parent)
        return;

    if (!parent->isElementNode())
        return;

    auto createFragmentResult = createContextualFragment(downcast<Element>(*parent), markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    if (!m_element->nextSibling())
        parent->appendChild(createFragmentResult.releaseReturnValue());
    else
        parent->insertBefore(createFragmentResult.releaseReturnValue(), m_element->nextSibling());
}

/*!
    Returns a clone of this element.

    The clone may be inserted at any point in the document.

    \sa appendInside(), prependInside(), prependOutside(), appendOutside()
*/
QWebElement QWebElement::clone() const
{
    if (!m_element)
        return QWebElement();

    // FIXME: Do we need to add document argument? What is use case for cloning to different document?
    return QWebElement(&m_element->cloneElementWithChildren(m_element->document()).get());
}

/*!
    Removes this element from the document and returns a reference to it.

    The element is still valid after removal, and can be inserted into other
    parts of the document.

    \sa removeAllChildren(), removeFromDocument()
*/
QWebElement &QWebElement::takeFromDocument()
{
    if (!m_element)
        return *this;

    m_element->remove();

    return *this;
}

/*!
    Removes this element from the document and makes it a null element.

    \sa removeAllChildren(), takeFromDocument()
*/
void QWebElement::removeFromDocument()
{
    if (!m_element)
        return;

    m_element->remove();
    m_element->deref();
    m_element = 0;
}

/*!
    Removes all children from this element.

    \sa removeFromDocument(), takeFromDocument()
*/
void QWebElement::removeAllChildren()
{
    if (!m_element)
        return;

    m_element->removeChildren();
}

// FIXME: This code, and all callers are wrong, and have no place in a
// WebKit implementation.  These should be replaced with WebCore implementations.
static RefPtr<Node> findInsertionPoint(Node* root)
{
    RefPtr<Node> node = root;

    // Go as far down the tree as possible.
    while (node->hasChildNodes() && node->firstChild()->isElementNode())
        node = node->firstChild();

    // TODO: Implement SVG support

    return node;
}

/*!
    Encloses the contents of this element with \a element. This element becomes
    the child of the deepest descendant within \a element.

    ### illustration

    \sa encloseWith()
*/
void QWebElement::encloseContentsWith(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    RefPtr<Node> insertionPoint = findInsertionPoint(element.m_element);

    if (!insertionPoint)
        return;

    // reparent children
    for (RefPtr<Node> child = m_element->firstChild(); child;) {
        RefPtr<Node> next = child->nextSibling();
        insertionPoint->appendChild(*child);
        child = next;
    }

    if (m_element->hasChildNodes())
        m_element->insertBefore(*element.m_element, m_element->firstChild());
    else
        m_element->appendChild(*element.m_element);
}

/*!
    Encloses the contents of this element with the result of parsing \a markup.
    This element becomes the child of the deepest descendant within \a markup.

    \sa encloseWith()
*/
void QWebElement::encloseContentsWith(const QString &markup)
{
    if (!m_element)
        return;

    if (!m_element->parentNode())
        return;

    auto createFragmentResult = createContextualFragment(*m_element, markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    auto fragment = createFragmentResult.releaseReturnValue();
    if (!fragment->firstChild())
        return;

    RefPtr<Node> insertionPoint = findInsertionPoint(fragment->firstChild());

    if (!insertionPoint)
        return;

    // reparent children
    for (RefPtr<Node> child = m_element->firstChild(); child;) {
        RefPtr<Node> next = child->nextSibling();
        insertionPoint->appendChild(*child);
        child = next;
    }

    if (m_element->hasChildNodes())
        m_element->insertBefore(fragment, m_element->firstChild());
    else
        m_element->appendChild(fragment);
}

/*!
    Encloses this element with \a element. This element becomes the child of
    the deepest descendant within \a element.

    \sa replace()
*/
void QWebElement::encloseWith(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    RefPtr<Node> insertionPoint = findInsertionPoint(element.m_element);

    if (!insertionPoint)
        return;

    // Keep reference to these two nodes before pulling out this element and
    // wrapping it in the fragment. The reason for doing it in this order is
    // that once the fragment has been added to the document it is empty, so
    // we no longer have access to the nodes it contained.
    Node* parent = m_element->parentNode();
    Node* siblingNode = m_element->nextSibling();

    insertionPoint->appendChild(*m_element);

    if (!siblingNode)
        parent->appendChild(*element.m_element);
    else
        parent->insertBefore(*element.m_element, siblingNode);
}

/*!
    Encloses this element with the result of parsing \a markup. This element
    becomes the child of the deepest descendant within \a markup.

    \sa replace()
*/
void QWebElement::encloseWith(const QString &markup)
{
    if (!m_element)
        return;

    Node* parent = m_element->parentNode();
    if (!parent)
        return;

    if (!parent->isElementNode())
        return;

    auto createFragmentResult = createContextualFragment(downcast<Element>(*parent), markup, AllowScriptingContent);
    if (createFragmentResult.hasException())
        return;

    auto fragment = createFragmentResult.releaseReturnValue();
    if (!fragment->firstChild())
        return;

    RefPtr<Node> insertionPoint = findInsertionPoint(fragment->firstChild());

    if (!insertionPoint)
        return;

    // Keep reference to parent & siblingNode before pulling out this element and
    // wrapping it in the fragment. The reason for doing it in this order is
    // that once the fragment has been added to the document it is empty, so
    // we no longer have access to the nodes it contained.
    Node* siblingNode = m_element->nextSibling();

    insertionPoint->appendChild(*m_element);

    if (!siblingNode)
        parent->appendChild(fragment);
    else
        parent->insertBefore(fragment, siblingNode);
}

/*!
    Replaces this element with \a element.

    This method will not replace the <html>, <head> or <body> elements.

    \sa encloseWith()
*/
void QWebElement::replace(const QWebElement &element)
{
    if (!m_element || element.isNull())
        return;

    appendOutside(element);
    takeFromDocument();
}

/*!
    Replaces this element with the result of parsing \a markup.

    This method will not replace the <html>, <head> or <body> elements.

    \sa encloseWith()
*/
void QWebElement::replace(const QString &markup)
{
    if (!m_element)
        return;

    appendOutside(markup);
    takeFromDocument();
}

/*!
    \fn inline bool QWebElement::operator==(const QWebElement& o) const;

    Returns true if this element points to the same underlying DOM object as
    \a o; otherwise returns false.
*/

/*!
    \fn inline bool QWebElement::operator!=(const QWebElement& o) const;

    Returns true if this element points to a different underlying DOM object
    than \a o; otherwise returns false.
*/


/*! 
  Render the element into \a painter .
*/
void QWebElement::render(QPainter* painter)
{
    render(painter, QRect());
}

/*!
  Render the element into \a painter clipping to \a clip.
*/
void QWebElement::render(QPainter* painter, const QRect& clip)
{
    WebCore::Element* e = m_element;
    if (!e)
        return;

    auto* renderer = e->renderer();
    if (!renderer)
        return;

    Frame* frame = e->document().frame();
    if (!frame || !frame->view() || !frame->contentRenderer())
        return;

    FrameView* view = frame->view();

    view->updateLayoutAndStyleIfNeededRecursive();

    IntRect rect = renderer->absoluteBoundingBoxRect();

    if (rect.isEmpty())
        return;

    QRect finalClipRect = rect;
    if (!clip.isEmpty())
        rect.intersect(clip.translated(rect.location()));

    GraphicsContext context(painter);

    context.save();
    context.translate(-rect.x(), -rect.y());
    painter->setClipRect(finalClipRect, Qt::IntersectClip);
    view->setNodeToDraw(e);
    view->paintContents(context, finalClipRect);
    view->setNodeToDraw(0);
    context.restore();
}

void QWebElement::beginEnterFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (m_element)
        m_element->document().fullscreenManager().willEnterFullscreen(*m_element);
#endif
}

void QWebElement::endEnterFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (m_element)
        m_element->document().fullscreenManager().didEnterFullscreen();
#endif
}

void QWebElement::beginExitFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (m_element)
        m_element->document().fullscreenManager().willExitFullscreen();
#endif
}

void QWebElement::endExitFullScreen()
{
#if ENABLE(FULLSCREEN_API)
    if (m_element)
        m_element->document().fullscreenManager().didExitFullscreen();
#endif
}

class QWebElementCollectionPrivate : public QSharedData
{
public:
    static QWebElementCollectionPrivate* create(ContainerNode* context, const QString &query);

    RefPtr<NodeList> m_result;

private:
    inline QWebElementCollectionPrivate() {}
};

QWebElementCollectionPrivate* QWebElementCollectionPrivate::create(ContainerNode* context, const QString &query)
{
    if (!context)
        return nullptr;

    // Let WebKit do the hard work hehehe
    auto queryResult = context->querySelectorAll(query);
    if (queryResult.hasException())
        return nullptr;

    QWebElementCollectionPrivate* priv = new QWebElementCollectionPrivate;
    priv->m_result = queryResult.releaseReturnValue();
    return priv;
}

/*!
    \class QWebElementCollection
    \inmodule QtWebKit
    \since 4.6
    \brief The QWebElementCollection class represents a collection of web elements.
    \preliminary

    Elements in a document can be selected using QWebElement::findAll() or using the
    QWebElement constructor. The collection is composed by choosing all elements in the
    document that match a specified CSS selector expression.

    The number of selected elements is provided through the count() property. Individual
    elements can be retrieved by index using at().

    It is also possible to iterate through all elements in the collection using Qt's foreach
    macro:

    \code
        QWebElementCollection collection = document.findAll("p");
        foreach (QWebElement paraElement, collection) {
            ...
        }
    \endcode
*/

/*!
    Constructs an empty collection.
*/
QWebElementCollection::QWebElementCollection()
{
}

/*!
    Constructs a copy of \a other.
*/
QWebElementCollection::QWebElementCollection(const QWebElementCollection &other)
    : d(other.d)
{
}

/*!
    Constructs a collection of elements from the list of child elements of \a contextElement that
    match the specified CSS selector \a query.
*/
QWebElementCollection::QWebElementCollection(const QWebElement &contextElement, const QString &query)
{
    d = QExplicitlySharedDataPointer<QWebElementCollectionPrivate>(QWebElementCollectionPrivate::create(contextElement.m_element, query));
}

/*!
    Assigns \a other to this collection and returns a reference to this collection.
*/
QWebElementCollection &QWebElementCollection::operator=(const QWebElementCollection &other)
{
    d = other.d;
    return *this;
}

/*!
    Destroys the collection.
*/
QWebElementCollection::~QWebElementCollection()
{
}

/*! \fn QWebElementCollection &QWebElementCollection::operator+=(const QWebElementCollection &other)

    Appends the items of the \a other list to this list and returns a
    reference to this list.

    \sa operator+(), append()
*/

/*!
    Returns a collection that contains all the elements of this collection followed
    by all the elements in the \a other collection. Duplicates may occur in the result.

    \sa operator+=()
*/
QWebElementCollection QWebElementCollection::operator+(const QWebElementCollection &other) const
{
    QWebElementCollection n = *this; n.d.detach(); n += other; return n;
}

/*!
    Extends the collection by appending all items of \a other.

    The resulting collection may include duplicate elements.

    \sa operator+=()
*/
void QWebElementCollection::append(const QWebElementCollection &other)
{
    if (!d) {
        *this = other;
        return;
    }
    if (!other.d)
        return;
    Vector<Ref<Node>> nodes;
    RefPtr<NodeList> results[] = { d->m_result, other.d->m_result };
    nodes.reserveInitialCapacity(results[0]->length() + results[1]->length());

    for (int i = 0; i < 2; ++i) {
        int j = 0;
        Node* n = results[i]->item(j);
        while (n) {
            nodes.append(*n);
            n = results[i]->item(++j);
        }
    }

    d->m_result = StaticNodeList::create(WTFMove(nodes));
}

/*!
    Returns the number of elements in the collection.
*/
int QWebElementCollection::count() const
{
    if (!d)
        return 0;
    return d->m_result->length();
}

/*!
    Returns the element at index position \a i in the collection.
*/
QWebElement QWebElementCollection::at(int i) const
{
    if (!d)
        return QWebElement();
    Node* n = d->m_result->item(i);
    return QWebElement(downcast<Element>(n));
}

/*!
    \fn const QWebElement QWebElementCollection::operator[](int position) const

    Returns the element at the specified \a position in the collection.
*/

/*! \fn QWebElement QWebElementCollection::first() const

    Returns the first element in the collection.

    \sa last(), operator[](), at(), count()
*/

/*! \fn QWebElement QWebElementCollection::last() const

    Returns the last element in the collection.

    \sa first(), operator[](), at(), count()
*/

/*!
    Returns a QList object with the elements contained in this collection.
*/
QList<QWebElement> QWebElementCollection::toList() const
{
    if (!d)
        return QList<QWebElement>();
    QList<QWebElement> elements;
    int i = 0;
    Node* n = d->m_result->item(i);
    while (n) {
        if (n->isElementNode())
            elements.append(QWebElement(downcast<Element>(n)));
        n = d->m_result->item(++i);
    }
    return elements;
}

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::begin() const

    Returns an STL-style iterator pointing to the first element in the collection.

    \sa end()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::end() const

    Returns an STL-style iterator pointing to the imaginary element after the
    last element in the list.

    \sa begin()
*/

/*!
    \class QWebElementCollection::const_iterator
    \inmodule QtWebKit
    \since 4.6
    \brief The QWebElementCollection::const_iterator class provides an STL-style const iterator for QWebElementCollection.

    QWebElementCollection provides STL style const iterators for fast low-level access to the elements.

    QWebElementCollection::const_iterator allows you to iterate over a QWebElementCollection.
*/

/*!
    \fn QWebElementCollection::const_iterator::const_iterator(const const_iterator &other)

    Constructs a copy of \a other.
*/

/*!
    \fn QWebElementCollection::const_iterator::const_iterator(const QWebElementCollection *collection, int index)
    \internal
*/

/*!
    \fn const QWebElement QWebElementCollection::const_iterator::operator*() const

    Returns the current element.
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator==(const const_iterator &other) const

    Returns true if \a other points to the same item as this iterator;
    otherwise returns false.

    \sa operator!=()
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator!=(const const_iterator &other) const

    Returns true if \a other points to a different element than this;
    iterator; otherwise returns false.

    \sa operator==()
*/

/*!
    \fn QWebElementCollection::const_iterator &QWebElementCollection::const_iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the next element in the collection
    and returns an iterator to the new current element.

    Calling this function on QWebElementCollection::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::const_iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the next element in the collection
    and returns an iterator to the previously current element.

    Calling this function on QWebElementCollection::end() leads to undefined results.
*/

/*!
    \fn QWebElementCollection::const_iterator &QWebElementCollection::const_iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding element current and returns an
    iterator to the new current element.

    Calling this function on QWebElementCollection::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::const_iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding element current and returns
    an iterator to the previously current element.
*/

/*!
    \fn QWebElementCollection::const_iterator &QWebElementCollection::const_iterator::operator+=(int j)

    Advances the iterator by \a j elements. If \a j is negative, the iterator goes backward.

    \sa operator-=(), operator+()
*/

/*!
    \fn QWebElementCollection::const_iterator &QWebElementCollection::const_iterator::operator-=(int j)

    Makes the iterator go back by \a j elements. If \a j is negative, the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::const_iterator::operator+(int j) const

    Returns an iterator to the element at \a j positions forward from this iterator. If \a j
    is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::const_iterator::operator-(int j) const

    Returns an iterator to the element at \a j positiosn backward from this iterator.
    If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*!
    \fn int QWebElementCollection::const_iterator::operator-(const_iterator other) const

    Returns the number of elements between the item point to by \a other
    and the element pointed to by this iterator.
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator<(const const_iterator &other) const

    Returns true if the element pointed to by this iterator is less than the element pointed to
    by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator<=(const const_iterator &other) const

    Returns true if the element pointed to by this iterator is less than or equal to the
    element pointed to by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator>(const const_iterator &other) const

    Returns true if the element pointed to by this iterator is greater than the element pointed to
    by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::const_iterator::operator>=(const const_iterator &other) const

    Returns true if the element pointed to by this iterator is greater than or equal to the
    element pointed to by the \a other iterator.
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::begin()

    Returns an STL-style iterator pointing to the first element in the collection.

    \sa end()
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::end()

    Returns an STL-style iterator pointing to the imaginary element after the
    last element in the list.

    \sa begin()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::constBegin() const

    Returns an STL-style iterator pointing to the first element in the collection.

    \sa end()
*/

/*!
    \fn QWebElementCollection::const_iterator QWebElementCollection::constEnd() const

    Returns an STL-style iterator pointing to the imaginary element after the
    last element in the list.

    \sa begin()
*/

/*!
    \class QWebElementCollection::iterator
    \inmodule QtWebKit
    \since 4.6
    \brief The QWebElementCollection::iterator class provides an STL-style iterator for QWebElementCollection.

    QWebElementCollection provides STL style iterators for fast low-level access to the elements.

    QWebElementCollection::iterator allows you to iterate over a QWebElementCollection.
*/

/*!
    \fn QWebElementCollection::iterator::iterator(const iterator &other)

    Constructs a copy of \a other.
*/

/*!
    \fn QWebElementCollection::iterator::iterator(const QWebElementCollection *collection, int index)
    \internal
*/

/*!
    \fn const QWebElement QWebElementCollection::iterator::operator*() const

    Returns the current element.
*/

/*!
    \fn bool QWebElementCollection::iterator::operator==(const iterator &other) const

    Returns true if \a other points to the same item as this iterator;
    otherwise returns false.

    \sa operator!=()
*/

/*!
    \fn bool QWebElementCollection::iterator::operator!=(const iterator &other) const

    Returns true if \a other points to a different element than this;
    iterator; otherwise returns false.

    \sa operator==()
*/

/*!
    \fn QWebElementCollection::iterator &QWebElementCollection::iterator::operator++()

    The prefix ++ operator (\c{++it}) advances the iterator to the next element in the collection
    and returns an iterator to the new current element.

    Calling this function on QWebElementCollection::end() leads to undefined results.

    \sa operator--()
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::iterator::operator++(int)

    \overload

    The postfix ++ operator (\c{it++}) advances the iterator to the next element in the collection
    and returns an iterator to the previously current element.

    Calling this function on QWebElementCollection::end() leads to undefined results.
*/

/*!
    \fn QWebElementCollection::iterator &QWebElementCollection::iterator::operator--()

    The prefix -- operator (\c{--it}) makes the preceding element current and returns an
    iterator to the new current element.

    Calling this function on QWebElementCollection::begin() leads to undefined results.

    \sa operator++()
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::iterator::operator--(int)

    \overload

    The postfix -- operator (\c{it--}) makes the preceding element current and returns
    an iterator to the previously current element.
*/

/*!
    \fn QWebElementCollection::iterator &QWebElementCollection::iterator::operator+=(int j)

    Advances the iterator by \a j elements. If \a j is negative, the iterator goes backward.

    \sa operator-=(), operator+()
*/

/*!
    \fn QWebElementCollection::iterator &QWebElementCollection::iterator::operator-=(int j)

    Makes the iterator go back by \a j elements. If \a j is negative, the iterator goes forward.

    \sa operator+=(), operator-()
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::iterator::operator+(int j) const

    Returns an iterator to the element at \a j positions forward from this iterator. If \a j
    is negative, the iterator goes backward.

    \sa operator-(), operator+=()
*/

/*!
    \fn QWebElementCollection::iterator QWebElementCollection::iterator::operator-(int j) const

    Returns an iterator to the element at \a j positiosn backward from this iterator.
    If \a j is negative, the iterator goes forward.

    \sa operator+(), operator-=()
*/

/*!
    \fn int QWebElementCollection::iterator::operator-(iterator other) const

    Returns the number of elements between the item point to by \a other
    and the element pointed to by this iterator.
*/

/*!
    \fn bool QWebElementCollection::iterator::operator<(const iterator &other) const

    Returns true if the element pointed to by this iterator is less than the element pointed to
    by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::iterator::operator<=(const iterator &other) const

    Returns true if the element pointed to by this iterator is less than or equal to the
    element pointed to by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::iterator::operator>(const iterator &other) const

    Returns true if the element pointed to by this iterator is greater than the element pointed to
    by the \a other iterator.
*/

/*!
    \fn bool QWebElementCollection::iterator::operator>=(const iterator &other) const

    Returns true if the element pointed to by this iterator is greater than or equal to the
    element pointed to by the \a other iterator.
*/

QWebElement QtWebElementRuntime::create(Element* element)
{
    return QWebElement(element);
}

Element* QtWebElementRuntime::get(const QWebElement& element)
{
    return element.m_element;
}

static QVariant convertJSValueToWebElementVariant(JSC::ExecState* exec, JSC::JSObject* object, int *distance, HashSet<JSObjectRef>* visitedObjects)
{
    JSC::VM& vm = exec->vm();
    Element* element = 0;
    QVariant ret;
    if (object && object->inherits<JSElement>(vm)) {
        element = JSElement::toWrapped(vm, object);
        *distance = 0;
        // Allow other objects to reach this one. This won't cause our algorithm to
        // loop since when we find an Element we do not recurse.
        visitedObjects->remove(toRef(object));
    } else if (object && object->inherits<JSElement>(vm)) {
        // To support TestRunnerQt::nodesFromRect(), used in DRT, we do an implicit
        // conversion from 'document' to the QWebElement representing the 'document.documentElement'.
        // We can't simply use a QVariantMap in nodesFromRect() because it currently times out
        // when serializing DOMMimeType and DOMPlugin, even if we limit the recursion.
        element = JSDocument::toWrapped(vm, object)->documentElement();
    }

    return QVariant::fromValue<QWebElement>(QtWebElementRuntime::create(element));
}

static JSC::JSValue convertWebElementVariantToJSValue(JSC::ExecState* exec, WebCore::JSDOMGlobalObject* globalObject, const QVariant& variant)
{
    return WebCore::toJS(exec, globalObject, QtWebElementRuntime::get(variant.value<QWebElement>()));
}

void QtWebElementRuntime::initialize()
{
    static bool initialized = false;
    if (initialized)
        return;
    initialized = true;
    int id = qRegisterMetaType<QWebElement>();
    JSC::Bindings::registerCustomType(id, convertJSValueToWebElementVariant, convertWebElementVariantToJSValue);
}
