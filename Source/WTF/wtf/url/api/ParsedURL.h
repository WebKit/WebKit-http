/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 * Copyright (C) 2012 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef ParsedURL_h
#define ParsedURL_h

#if USE(WTFURL)

#include <wtf/url/api/URLString.h>
#include <wtf/url/src/URLSegments.h>

namespace WTF {

class URLComponent;
class URLQueryCharsetConverter;

// ParsedURL represents a valid URL decomposed by components.
class ParsedURL {
public:
    enum ParsedURLStringTag { ParsedURLString };

    ParsedURL() { };
    WTF_EXPORT_PRIVATE explicit ParsedURL(const String&, ParsedURLStringTag);

    WTF_EXPORT_PRIVATE explicit ParsedURL(const String&, URLQueryCharsetConverter*);
    WTF_EXPORT_PRIVATE explicit ParsedURL(const ParsedURL& base, const String& relative, URLQueryCharsetConverter*);

    WTF_EXPORT_PRIVATE ParsedURL isolatedCopy() const;

    bool isValid() const { return !m_spec.string().isNull(); }

    // Return a URL component or a null String if the component is undefined for the URL.
    WTF_EXPORT_PRIVATE String scheme() const;
    WTF_EXPORT_PRIVATE bool hasStandardScheme() const;

    WTF_EXPORT_PRIVATE String username() const;
    WTF_EXPORT_PRIVATE String password() const;
    WTF_EXPORT_PRIVATE String host() const;

    WTF_EXPORT_PRIVATE bool hasPort() const;
    WTF_EXPORT_PRIVATE String port() const;
    WTF_EXPORT_PRIVATE void replacePort(unsigned short newPort);
    WTF_EXPORT_PRIVATE void removePort();

    WTF_EXPORT_PRIVATE String path() const;
    WTF_EXPORT_PRIVATE String query() const;

    WTF_EXPORT_PRIVATE bool hasFragment() const;
    WTF_EXPORT_PRIVATE String fragment() const;
    WTF_EXPORT_PRIVATE ParsedURL withoutFragment() const;


    WTF_EXPORT_PRIVATE String baseAsString() const;

    const URLString& spec() const { return m_spec; }

#ifndef NDEBUG
    WTF_EXPORT_PRIVATE void print() const;
#endif

private:
    inline String segment(const URLComponent&) const;

    URLString m_spec;
    URLSegments m_segments;
};

}

#endif // USE(WTFURL)

#endif
