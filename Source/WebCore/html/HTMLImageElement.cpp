/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2010 Apple Inc. All rights reserved.
 * Copyright (C) 2010 Google Inc. All rights reserved.
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
#include "HTMLImageElement.h"

#include "Attribute.h"
#include "CSSPropertyNames.h"
#include "CSSValueKeywords.h"
#include "CachedImage.h"
#include "EventNames.h"
#include "FrameView.h"
#include "HTMLAnchorElement.h"
#include "HTMLDocument.h"
#include "HTMLFormElement.h"
#include "HTMLNames.h"
#include "HTMLParserIdioms.h"
#include "Page.h"
#include "RenderImage.h"

namespace WebCore {

using namespace HTMLNames;

HTMLImageElement::HTMLImageElement(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
    : HTMLElement(tagName, document)
    , m_imageLoader(this)
    , m_form(form)
    , m_compositeOperator(CompositeSourceOver)
{
    ASSERT(hasTagName(imgTag));
    setHasCustomStyleResolveCallbacks();
    if (form)
        form->registerImgElement(this);
}

PassRefPtr<HTMLImageElement> HTMLImageElement::create(Document& document)
{
    return adoptRef(new HTMLImageElement(imgTag, document));
}

PassRefPtr<HTMLImageElement> HTMLImageElement::create(const QualifiedName& tagName, Document& document, HTMLFormElement* form)
{
    return adoptRef(new HTMLImageElement(tagName, document, form));
}

HTMLImageElement::~HTMLImageElement()
{
    if (m_form)
        m_form->removeImgElement(this);
}

PassRefPtr<HTMLImageElement> HTMLImageElement::createForJSConstructor(Document& document, const int* optionalWidth, const int* optionalHeight)
{
    RefPtr<HTMLImageElement> image = adoptRef(new HTMLImageElement(imgTag, document));
    if (optionalWidth)
        image->setWidth(*optionalWidth);
    if (optionalHeight)
        image->setHeight(*optionalHeight);
    return image.release();
}

bool HTMLImageElement::isPresentationAttribute(const QualifiedName& name) const
{
    if (name == widthAttr || name == heightAttr || name == borderAttr || name == vspaceAttr || name == hspaceAttr || name == alignAttr || name == valignAttr)
        return true;
    return HTMLElement::isPresentationAttribute(name);
}

void HTMLImageElement::collectStyleForPresentationAttribute(const QualifiedName& name, const AtomicString& value, MutableStylePropertySet* style)
{
    if (name == widthAttr)
        addHTMLLengthToStyle(style, CSSPropertyWidth, value);
    else if (name == heightAttr)
        addHTMLLengthToStyle(style, CSSPropertyHeight, value);
    else if (name == borderAttr)
        applyBorderAttributeToStyle(value, style);
    else if (name == vspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginTop, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginBottom, value);
    } else if (name == hspaceAttr) {
        addHTMLLengthToStyle(style, CSSPropertyMarginLeft, value);
        addHTMLLengthToStyle(style, CSSPropertyMarginRight, value);
    } else if (name == alignAttr)
        applyAlignmentAttributeToStyle(value, style);
    else if (name == valignAttr)
        addPropertyToPresentationAttributeStyle(style, CSSPropertyVerticalAlign, value);
    else
        HTMLElement::collectStyleForPresentationAttribute(name, value, style);
}

const AtomicString& HTMLImageElement::imageSourceURL() const
{
    return m_bestFitImageURL.isEmpty() ? fastGetAttribute(srcAttr) : m_bestFitImageURL;
}

void HTMLImageElement::parseAttribute(const QualifiedName& name, const AtomicString& value)
{
    if (name == altAttr) {
        if (renderer() && renderer()->isImage())
            toRenderImage(renderer())->updateAltText();
    } else if (name == srcAttr || name == srcsetAttr) {
        m_bestFitImageURL = bestFitSourceForImageAttributes(document().deviceScaleFactor(), fastGetAttribute(srcAttr), fastGetAttribute(srcsetAttr));
        m_imageLoader.updateFromElementIgnoringPreviousError();
    } else if (name == usemapAttr) {
        setIsLink(!value.isNull() && !shouldProhibitLinks(this));

        if (inDocument() && !m_lowercasedUsemap.isNull())
            document().removeImageElementByLowercasedUsemap(*m_lowercasedUsemap.impl(), *this);

        // The HTMLImageElement's useMap() value includes the '#' symbol at the beginning, which has to be stripped off.
        // FIXME: We should check that the first character is '#'.
        // FIXME: HTML5 specification says we should strip any leading string before '#'.
        // FIXME: HTML5 specification says we should ignore usemap attributes without #.
        if (value.length() > 1)
            m_lowercasedUsemap = value.string().substring(1).lower();
        else
            m_lowercasedUsemap = nullAtom;

        if (inDocument() && !m_lowercasedUsemap.isNull())
            document().addImageElementByLowercasedUsemap(*m_lowercasedUsemap.impl(), *this);
    } else if (name == onbeforeloadAttr)
        setAttributeEventListener(eventNames().beforeloadEvent, name, value);
    else if (name == compositeAttr) {
        // FIXME: images don't support blend modes in their compositing attribute.
        BlendMode blendOp = BlendModeNormal;
        if (!parseCompositeAndBlendOperator(value, m_compositeOperator, blendOp))
            m_compositeOperator = CompositeSourceOver;
    } else {
        if (name == nameAttr) {
            bool willHaveName = !value.isNull();
            if (hasName() != willHaveName && inDocument() && document().isHTMLDocument()) {
                HTMLDocument* document = toHTMLDocument(&this->document());
                const AtomicString& id = getIdAttribute();
                if (!id.isEmpty() && id != getNameAttribute()) {
                    if (willHaveName)
                        document->addDocumentNamedItem(*id.impl(), *this);
                    else
                        document->removeDocumentNamedItem(*id.impl(), *this);
                }
            }
        }
        HTMLElement::parseAttribute(name, value);
    }
}

String HTMLImageElement::altText() const
{
    // lets figure out the alt text.. magic stuff
    // http://www.w3.org/TR/1998/REC-html40-19980424/appendix/notes.html#altgen
    // also heavily discussed by Hixie on bugzilla
    String alt = getAttribute(altAttr);
    // fall back to title attribute
    if (alt.isNull())
        alt = getAttribute(titleAttr);
    return alt;
}

RenderElement* HTMLImageElement::createRenderer(PassRef<RenderStyle> style)
{
    if (style.get().hasContent())
        return RenderElement::createFor(*this, std::move(style));

    RenderImage* image = new RenderImage(*this, std::move(style));
    image->setImageResource(RenderImageResource::create());
    return image;
}

bool HTMLImageElement::canStartSelection() const
{
    if (shadowRoot())
        return HTMLElement::canStartSelection();

    return false;
}

void HTMLImageElement::didAttachRenderers()
{
    if (!renderer() || !renderer()->isImage())
        return;
    if (m_imageLoader.hasPendingBeforeLoadEvent())
        return;
    RenderImage* renderImage = toRenderImage(renderer());
    RenderImageResource* renderImageResource = renderImage->imageResource();
    if (renderImageResource->hasImage())
        return;
    renderImageResource->setCachedImage(m_imageLoader.image());

    // If we have no image at all because we have no src attribute, set
    // image height and width for the alt text instead.
    if (!m_imageLoader.image() && !renderImageResource->cachedImage())
        renderImage->setImageSizeForAltText();
}

Node::InsertionNotificationRequest HTMLImageElement::insertedInto(ContainerNode& insertionPoint)
{
    if (!m_form) { // m_form can be non-null if it was set in constructor.
        m_form = HTMLFormElement::findClosestFormAncestor(*this);
        if (m_form)
            m_form->registerImgElement(this);
    }

    if (insertionPoint.inDocument() && !m_lowercasedUsemap.isNull())
        document().addImageElementByLowercasedUsemap(*m_lowercasedUsemap.impl(), *this);

    // If we have been inserted from a renderer-less document,
    // our loader may have not fetched the image, so do it now.
    if (insertionPoint.inDocument() && !m_imageLoader.image())
        m_imageLoader.updateFromElement();

    return HTMLElement::insertedInto(insertionPoint);
}

void HTMLImageElement::removedFrom(ContainerNode& insertionPoint)
{
    if (m_form)
        m_form->removeImgElement(this);

    if (insertionPoint.inDocument() && !m_lowercasedUsemap.isNull())
        document().removeImageElementByLowercasedUsemap(*m_lowercasedUsemap.impl(), *this);

    m_form = 0;
    HTMLElement::removedFrom(insertionPoint);
}

int HTMLImageElement::width(bool ignorePendingStylesheets)
{
    if (!renderer()) {
        // check the attribute first for an explicit pixel value
        bool ok;
        int width = getAttribute(widthAttr).toInt(&ok);
        if (ok)
            return width;

        // if the image is available, use its width
        if (m_imageLoader.image())
            return m_imageLoader.image()->imageSizeForRenderer(renderer(), 1.0f).width();
    }

    if (ignorePendingStylesheets)
        document().updateLayoutIgnorePendingStylesheets();
    else
        document().updateLayout();

    RenderBox* box = renderBox();
    return box ? adjustForAbsoluteZoom(box->contentBoxRect().pixelSnappedWidth(), box) : 0;
}

int HTMLImageElement::height(bool ignorePendingStylesheets)
{
    if (!renderer()) {
        // check the attribute first for an explicit pixel value
        bool ok;
        int height = getAttribute(heightAttr).toInt(&ok);
        if (ok)
            return height;

        // if the image is available, use its height
        if (m_imageLoader.image())
            return m_imageLoader.image()->imageSizeForRenderer(renderer(), 1.0f).height();
    }

    if (ignorePendingStylesheets)
        document().updateLayoutIgnorePendingStylesheets();
    else
        document().updateLayout();

    RenderBox* box = renderBox();
    return box ? adjustForAbsoluteZoom(box->contentBoxRect().pixelSnappedHeight(), box) : 0;
}

int HTMLImageElement::naturalWidth() const
{
    if (!m_imageLoader.image())
        return 0;

    return m_imageLoader.image()->imageSizeForRenderer(renderer(), 1.0f).width();
}

int HTMLImageElement::naturalHeight() const
{
    if (!m_imageLoader.image())
        return 0;

    return m_imageLoader.image()->imageSizeForRenderer(renderer(), 1.0f).height();
}

bool HTMLImageElement::isURLAttribute(const Attribute& attribute) const
{
    return attribute.name() == srcAttr
        || attribute.name() == lowsrcAttr
        || attribute.name() == longdescAttr
        || (attribute.name() == usemapAttr && attribute.value().string()[0] != '#')
        || HTMLElement::isURLAttribute(attribute);
}

bool HTMLImageElement::matchesLowercasedUsemap(const AtomicStringImpl& name) const
{
    ASSERT(String(&const_cast<AtomicStringImpl&>(name)).lower().impl() == &name);
    return m_lowercasedUsemap.impl() == &name;
}

const AtomicString& HTMLImageElement::alt() const
{
    return getAttribute(altAttr);
}

bool HTMLImageElement::draggable() const
{
    // Image elements are draggable by default.
    return !equalIgnoringCase(getAttribute(draggableAttr), "false");
}

void HTMLImageElement::setHeight(int value)
{
    setIntegralAttribute(heightAttr, value);
}

URL HTMLImageElement::src() const
{
    return document().completeURL(getAttribute(srcAttr));
}

void HTMLImageElement::setSrc(const String& value)
{
    setAttribute(srcAttr, value);
}

void HTMLImageElement::setWidth(int value)
{
    setIntegralAttribute(widthAttr, value);
}

int HTMLImageElement::x() const
{
    auto renderer = this->renderer();
    if (!renderer)
        return 0;

    // FIXME: This doesn't work correctly with transforms.
    return renderer->localToAbsolute().x();
}

int HTMLImageElement::y() const
{
    auto renderer = this->renderer();
    if (!renderer)
        return 0;

    // FIXME: This doesn't work correctly with transforms.
    return renderer->localToAbsolute().y();
}

bool HTMLImageElement::complete() const
{
    return m_imageLoader.imageComplete();
}

void HTMLImageElement::addSubresourceAttributeURLs(ListHashSet<URL>& urls) const
{
    HTMLElement::addSubresourceAttributeURLs(urls);

    addSubresourceURL(urls, src());
    // FIXME: What about when the usemap attribute begins with "#"?
    addSubresourceURL(urls, document().completeURL(getAttribute(usemapAttr)));
}

void HTMLImageElement::didMoveToNewDocument(Document* oldDocument)
{
    m_imageLoader.elementDidMoveToNewDocument();
    HTMLElement::didMoveToNewDocument(oldDocument);
}

bool HTMLImageElement::isServerMap() const
{
    if (!fastHasAttribute(ismapAttr))
        return false;

    const AtomicString& usemap = fastGetAttribute(usemapAttr);
    
    // If the usemap attribute starts with '#', it refers to a map element in the document.
    if (usemap.string()[0] == '#')
        return false;

    return document().completeURL(stripLeadingAndTrailingHTMLSpaces(usemap)).isEmpty();
}

#if PLATFORM(IOS)
// FIXME: This is a workaround for <rdar://problem/7725158>. We should find a better place for the touchCalloutEnabled() logic.
bool HTMLImageElement::willRespondToMouseClickEvents()
{
    auto renderer = this->renderer();
    RenderStyle* style = renderer ? renderer->style() : nullptr;
    if (!style || style->touchCalloutEnabled())
        return true;
    return HTMLElement::willRespondToMouseClickEvents();
}
#endif

}
