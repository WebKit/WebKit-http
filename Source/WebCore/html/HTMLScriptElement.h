/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2008 Nikolas Zimmermann <zimmermann@kde.org>
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

#ifndef HTMLScriptElement_h
#define HTMLScriptElement_h

#include "ScriptElement.h"
#include "HTMLElement.h"

namespace WebCore {

class HTMLScriptElement : public HTMLElement, public ScriptElement {
public:
    static PassRefPtr<HTMLScriptElement> create(const QualifiedName&, Document*, bool wasInsertedByParser);

    String text() const { return scriptContent(); }
    void setText(const String&);

    KURL src() const;

    void setAsync(bool);
    bool async() const;

private:
    HTMLScriptElement(const QualifiedName&, Document*, bool wasInsertedByParser, bool alreadyStarted);

    virtual void parseMappedAttribute(Attribute*);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void childrenChanged(bool changedByParser = false, Node* beforeChange = 0, Node* afterChange = 0, int childCountDelta = 0);
    virtual void attributeChanged(Attribute*, bool preserveDecls = false);

    virtual bool isURLAttribute(Attribute*) const;

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    virtual String sourceAttributeValue() const;
    virtual String charsetAttributeValue() const;
    virtual String typeAttributeValue() const;
    virtual String languageAttributeValue() const;
    virtual String forAttributeValue() const;
    virtual String eventAttributeValue() const;
    virtual bool asyncAttributeValue() const;
    virtual bool deferAttributeValue() const;
    virtual bool hasSourceAttribute() const;

    virtual void dispatchLoadEvent();

    virtual PassRefPtr<Element> cloneElementWithoutAttributesAndChildren();
};

} //namespace

#endif
