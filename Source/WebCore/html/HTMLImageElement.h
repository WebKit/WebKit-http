/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2008, 2010 Apple Inc. All rights reserved.
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
 *
 */

#ifndef HTMLImageElement_h
#define HTMLImageElement_h

#include "GraphicsTypes.h"
#include "HTMLElement.h"
#include "HTMLImageLoader.h"
#include "ImageLoaderClient.h"

namespace WebCore {

class HTMLFormElement;
class ImageInnerElement;

class ImageElement {
protected:
    void setImageIfNecessary(RenderObject*, ImageLoader*);
    RenderObject* createRendererForImage(HTMLElement*, RenderArena*);
};

class HTMLImageElement : public HTMLElement, public ImageElement, public ImageLoaderClient {
    friend class HTMLFormElement;
    friend class ImageInnerElement;
public:
    static PassRefPtr<HTMLImageElement> create(Document*);
    static PassRefPtr<HTMLImageElement> create(const QualifiedName&, Document*, HTMLFormElement*);
    static PassRefPtr<HTMLImageElement> createForJSConstructor(Document*, const int* optionalWidth, const int* optionalHeight);

    virtual ~HTMLImageElement();

    virtual void willAddAuthorShadowRoot() OVERRIDE;

    int width(bool ignorePendingStylesheets = false);
    int height(bool ignorePendingStylesheets = false);

    int naturalWidth() const;
    int naturalHeight() const;

    bool isServerMap() const;

    String altText() const;

    CompositeOperator compositeOperator() const { return m_compositeOperator; }

    CachedImage* cachedImage() const { return m_imageLoader.image(); }
    void setCachedImage(CachedImage* i) { m_imageLoader.setImage(i); };

    void setLoadManually(bool loadManually) { m_imageLoader.setLoadManually(loadManually); }

    const AtomicString& alt() const;

    void setHeight(int);

    KURL src() const;
    void setSrc(const String&);

    void setWidth(int);

    int x() const;
    int y() const;

    bool complete() const;

    bool hasPendingActivity() const { return m_imageLoader.hasPendingActivity(); }

    virtual bool canContainRangeEndPoint() const { return false; }

    virtual void reportMemoryUsage(MemoryObjectInfo*) const OVERRIDE;

protected:
    HTMLImageElement(const QualifiedName&, Document*, HTMLFormElement* = 0);

    virtual void didMoveToNewDocument(Document* oldDocument) OVERRIDE;

private:
    virtual void createShadowSubtree();

    virtual bool areAuthorShadowsAllowed() const OVERRIDE { return false; }

    // Implementation of ImageLoaderClient
    Element* sourceElement() { return this; }
    Element* imageElement();
    void refSourceElement() { ref(); }
    void derefSourceElement() { deref(); }

    virtual void parseAttribute(const Attribute&) OVERRIDE;
    virtual bool isPresentationAttribute(const QualifiedName&) const OVERRIDE;
    virtual void collectStyleForAttribute(const Attribute&, StylePropertySet*) OVERRIDE;

    virtual void attach();
    virtual RenderObject* createRenderer(RenderArena*, RenderStyle*);

    virtual bool canStartSelection() const;

    virtual bool isURLAttribute(const Attribute&) const OVERRIDE;

    virtual bool draggable() const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    virtual InsertionNotificationRequest insertedInto(ContainerNode*) OVERRIDE;
    virtual void removedFrom(ContainerNode*) OVERRIDE;
    virtual bool shouldRegisterAsNamedItem() const OVERRIDE { return true; }
    virtual bool shouldRegisterAsExtraNamedItem() const OVERRIDE { return true; }

#if ENABLE(MICRODATA)
    virtual String itemValueText() const OVERRIDE;
    virtual void setItemValueText(const String&, ExceptionCode&) OVERRIDE;
#endif

    RenderObject* imageRenderer() const { return HTMLElement::renderer(); }
    HTMLImageLoader* imageLoader() { return &m_imageLoader; }
    ImageInnerElement* innerElement() const;

    HTMLImageLoader m_imageLoader;
    HTMLFormElement* m_form;
    CompositeOperator m_compositeOperator;
};

inline bool isHTMLImageElement(Node* node)
{
    return node->hasTagName(HTMLNames::imgTag);
}

inline HTMLImageElement* toHTMLImageElement(Node* node)
{
    ASSERT(!node || isHTMLImageElement(node));
    return static_cast<HTMLImageElement*>(node);
}

} //namespace

#endif
