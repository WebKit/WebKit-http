/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Peter Kelly (pmk@post.com)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2003, 2004, 2005, 2006, 2008, 2011 Apple Inc. All rights reserved.
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

#ifndef CSSMappedAttributeDeclaration_h
#define CSSMappedAttributeDeclaration_h

#include "MappedAttributeEntry.h"
#include "QualifiedName.h"
#include "StylePropertySet.h"

namespace WebCore {

class StyledElement;

class CSSMappedAttributeDeclaration : public RefCounted<CSSMappedAttributeDeclaration> {
public:
    static PassRefPtr<CSSMappedAttributeDeclaration> create()
    {
        return adoptRef(new CSSMappedAttributeDeclaration);
    }

    ~CSSMappedAttributeDeclaration();

    void setMappedProperty(StyledElement*, int propertyId, int value);
    void setMappedProperty(StyledElement*, int propertyId, const String& value);
    void setMappedImageProperty(StyledElement*, int propertyId, const String& url);

    // NOTE: setMappedLengthProperty() treats integers as pixels! (Needed for conversion of HTML attributes.)
    void setMappedLengthProperty(StyledElement*, int propertyId, const String& value);

    void removeMappedProperty(StyledElement*, int propertyId);
    
    StylePropertySet* declaration() const { return m_declaration.get(); }

private:
    CSSMappedAttributeDeclaration()
        : m_declaration(StylePropertySet::create())
    {
    }

    void setNeedsStyleRecalc(StyledElement*);

    RefPtr<StylePropertySet> m_declaration;
};

} //namespace

#endif
