/*
 * Copyright (C) 2010 Apple Inc. All Rights Reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE, INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE COMPUTER, INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
 */
#ifndef SchemeRegistry_h
#define SchemeRegistry_h

#include "PlatformString.h"
#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>

namespace WebCore {

typedef HashSet<String, CaseFoldingHash> URLSchemesMap;

class SchemeRegistry {
public:
    static void registerURLSchemeAsLocal(const String&);
    static void removeURLSchemeRegisteredAsLocal(const String&);
    static const URLSchemesMap& localSchemes();

    static bool shouldTreatURLSchemeAsLocal(const String&);

    // Secure schemes do not trigger mixed content warnings. For example,
    // https and data are secure schemes because they cannot be corrupted by
    // active network attackers.
    static void registerURLSchemeAsSecure(const String&);
    static bool shouldTreatURLSchemeAsSecure(const String&);

    static void registerURLSchemeAsNoAccess(const String&);
    static bool shouldTreatURLSchemeAsNoAccess(const String&);

    // Display-isolated schemes can only be displayed (in the sense of
    // SecurityOrigin::canDisplay) by documents from the same scheme.
    static void registerURLSchemeAsDisplayIsolated(const String&);
    static bool shouldTreatURLSchemeAsDisplayIsolated(const String&);

    static void registerURLSchemeAsEmptyDocument(const String&);
    static bool shouldLoadURLSchemeAsEmptyDocument(const String&);

    // Such schemes should delegate to SecurityOrigin::canRequest for any URL
    // passed to SecurityOrigin::canDisplay.
    static bool canDisplayOnlyIfCanRequest(const String& scheme);
    static void registerAsCanDisplayOnlyIfCanRequest(const String& scheme);

    // Schemes against which javascript: URLs should not be allowed to run (stop
    // bookmarklets from running on sensitive pages). 
    static void registerURLSchemeAsNotAllowingJavascriptURLs(const String& scheme);
    static bool shouldTreatURLSchemeAsNotAllowingJavascriptURLs(const String& scheme);
};

} // namespace WebCore

#endif // SchemeRegistry_h
