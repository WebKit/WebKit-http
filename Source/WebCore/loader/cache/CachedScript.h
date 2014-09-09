/*
    Copyright (C) 1998 Lars Knoll (knoll@mpi-hd.mpg.de)
    Copyright (C) 2001 Dirk Mueller <mueller@kde.org>
    Copyright (C) 2006 Samuel Weinig (sam.weinig@gmail.com)
    Copyright (C) 2004, 2005, 2006, 2007, 2008 Apple Inc. All rights reserved.

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

    This class provides all functionality needed for loading images, style sheets and html
    pages from the web. It has a memory cache for these objects.
*/

#ifndef CachedScript_h
#define CachedScript_h

#include "CachedResource.h"

namespace WebCore {

    class CachedResourceLoader;
    class TextResourceDecoder;

    class CachedScript final : public CachedResource {
    public:
        CachedScript(const ResourceRequest&, const String& charset, SessionID);
        virtual ~CachedScript();

        const String& script();

        String mimeType() const;

#if ENABLE(NOSNIFF)
        bool mimeTypeAllowedByNosniff() const;
#endif

    private:
        virtual PurgePriority purgePriority() const override { return PurgeLast; }
        virtual bool mayTryReplaceEncodedData() const override { return true; }

        virtual bool shouldIgnoreHTTPStatusCodeErrors() const override;

        virtual void setEncoding(const String&) override;
        virtual String encoding() const override;
        virtual void finishLoading(ResourceBuffer*) override;

        virtual void destroyDecodedData() override;

        String m_script;
        RefPtr<TextResourceDecoder> m_decoder;
    };

CACHED_RESOURCE_TYPE_CASTS(CachedScript, CachedResource, CachedResource::Script)

}

#endif
