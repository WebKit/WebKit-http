/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * Copyright (C) 2004, 2006, 2008, 2009, 2010 Apple Inc. All rights reserved.
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

#ifndef MediaList_h
#define MediaList_h

#include "StyleBase.h"
#include <wtf/Forward.h>
#include <wtf/PassRefPtr.h>
#include <wtf/Vector.h>

namespace WebCore {

class CSSImportRule;
class CSSStyleSheet;
class MediaQuery;

typedef int ExceptionCode;

class MediaList : public StyleBase {
public:
    static PassRefPtr<MediaList> create()
    {
        return adoptRef(new MediaList(0, false));
    }
    static PassRefPtr<MediaList> create(CSSImportRule* parentRule, const String& media)
    {
        return adoptRef(new MediaList(parentRule, media));
    }
    static PassRefPtr<MediaList> create(CSSStyleSheet* parentSheet, const String& media)
    {
        return adoptRef(new MediaList(parentSheet, media, false));
    }

    static PassRefPtr<MediaList> createAllowingDescriptionSyntax(const String& mediaQueryOrDescription)
    {
        return adoptRef(new MediaList(0, mediaQueryOrDescription, true));
    }
    static PassRefPtr<MediaList> createAllowingDescriptionSyntax(CSSStyleSheet* parentSheet, const String& mediaQueryOrDescription)
    {
        return adoptRef(new MediaList(parentSheet, mediaQueryOrDescription, true));
    }

    static PassRefPtr<MediaList> create(const String& media, bool allowDescriptionSyntax)
    {
        return adoptRef(new MediaList(0, media, allowDescriptionSyntax));
    }

    virtual ~MediaList();

    unsigned length() const { return m_queries.size(); }
    String item(unsigned index) const;
    void deleteMedium(const String& oldMedium, ExceptionCode&);
    void appendMedium(const String& newMedium, ExceptionCode&);

    String mediaText() const;
    void setMediaText(const String&, ExceptionCode&xo);

    void appendMediaQuery(PassOwnPtr<MediaQuery>);
    const Vector<MediaQuery*>& mediaQueries() const { return m_queries; }

private:
    MediaList(CSSStyleSheet* parentSheet, bool fallbackToDescription);
    MediaList(CSSStyleSheet* parentSheet, const String& media, bool fallbackToDescription);
    MediaList(CSSImportRule* parentRule, const String& media);

    void notifyChanged();

    Vector<MediaQuery*> m_queries;
    bool m_fallback; // true if failed media query parsing should fallback to media description parsing
};

} // namespace

#endif
