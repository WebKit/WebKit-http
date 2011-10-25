/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 * Copyright (C) 2004, 2006, 2008, 2009 Apple Inc. All rights reserved.
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

#ifndef HTMLEmbedElement_h
#define HTMLEmbedElement_h

#include "HTMLPlugInImageElement.h"

namespace WebCore {

class HTMLEmbedElement : public HTMLPlugInImageElement {
public:
    static PassRefPtr<HTMLEmbedElement> create(const QualifiedName&, Document*, bool createdByParser);

private:
    HTMLEmbedElement(const QualifiedName&, Document*, bool createdByParser);

    virtual bool mapToEntry(const QualifiedName& attrName, MappedAttributeEntry& result) const;
    virtual void parseMappedAttribute(Attribute*);

    virtual bool rendererIsNeeded(const NodeRenderingContext&);
    virtual void insertedIntoDocument();
    virtual void removedFromDocument();
    virtual void attributeChanged(Attribute*, bool preserveDecls = false);

    virtual bool isURLAttribute(Attribute*) const;
    virtual const QualifiedName& imageSourceAttributeName() const;

    virtual RenderWidget* renderWidgetForJSBindings();

    virtual void updateWidget(PluginCreationOption);

    virtual void addSubresourceAttributeURLs(ListHashSet<KURL>&) const;

    void parametersForPlugin(Vector<String>& paramNames, Vector<String>& paramValues);

#if ENABLE(MICRODATA)
    virtual String itemValueText() const OVERRIDE;
    virtual void setItemValueText(const String&, ExceptionCode&) OVERRIDE;
#endif
};

}

#endif
