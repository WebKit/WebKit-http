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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
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

#include <wtf/HashSet.h>
#include <wtf/text/StringHash.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

typedef HashSet<String, CaseFoldingHash> URLSchemesMap;

class SchemeRegistry {
public:
    WEBCORE_EXPORT static void registerURLSchemeAsLocal(const String&);
    static void removeURLSchemeRegisteredAsLocal(const String&);
    static const URLSchemesMap& localSchemes();

    WEBCORE_EXPORT static bool shouldTreatURLSchemeAsLocal(const String&);

    // Secure schemes do not trigger mixed content warnings. For example,
    // https and data are secure schemes because they cannot be corrupted by
    // active network attackers.
    WEBCORE_EXPORT static void registerURLSchemeAsSecure(const String&);
    static bool shouldTreatURLSchemeAsSecure(const String&);

    WEBCORE_EXPORT static void registerURLSchemeAsNoAccess(const String&);
    static bool shouldTreatURLSchemeAsNoAccess(const String&);

    // Display-isolated schemes can only be displayed (in the sense of
    // SecurityOrigin::canDisplay) by documents from the same scheme.
    WEBCORE_EXPORT static void registerURLSchemeAsDisplayIsolated(const String&);
    static bool shouldTreatURLSchemeAsDisplayIsolated(const String&);

    WEBCORE_EXPORT static void registerURLSchemeAsEmptyDocument(const String&);
    WEBCORE_EXPORT static bool shouldLoadURLSchemeAsEmptyDocument(const String&);

    WEBCORE_EXPORT static void setDomainRelaxationForbiddenForURLScheme(bool forbidden, const String&);
    static bool isDomainRelaxationForbiddenForURLScheme(const String&);

    // Such schemes should delegate to SecurityOrigin::canRequest for any URL
    // passed to SecurityOrigin::canDisplay.
    static bool canDisplayOnlyIfCanRequest(const String& scheme);
    static void registerAsCanDisplayOnlyIfCanRequest(const String& scheme);

    // Schemes against which javascript: URLs should not be allowed to run (stop
    // bookmarklets from running on sensitive pages). 
    static void registerURLSchemeAsNotAllowingJavascriptURLs(const String& scheme);
    static bool shouldTreatURLSchemeAsNotAllowingJavascriptURLs(const String& scheme);

    // Let some schemes opt-out of Private Browsing's default behavior of prohibiting read/write
    // access to Local Storage and Databases.
    WEBCORE_EXPORT static void registerURLSchemeAsAllowingLocalStorageAccessInPrivateBrowsing(const String& scheme);
    WEBCORE_EXPORT static bool allowsLocalStorageAccessInPrivateBrowsing(const String& scheme);
    WEBCORE_EXPORT static void registerURLSchemeAsAllowingDatabaseAccessInPrivateBrowsing(const String& scheme);
    static bool allowsDatabaseAccessInPrivateBrowsing(const String& scheme);

    // Allow non-HTTP schemes to be registered to allow CORS requests.
    WEBCORE_EXPORT static void registerURLSchemeAsCORSEnabled(const String& scheme);
    static bool shouldTreatURLSchemeAsCORSEnabled(const String& scheme);

    // Allow resources from some schemes to load on a page, regardless of its
    // Content Security Policy.
    WEBCORE_EXPORT static void registerURLSchemeAsBypassingContentSecurityPolicy(const String& scheme);
    WEBCORE_EXPORT static void removeURLSchemeRegisteredAsBypassingContentSecurityPolicy(const String& scheme);
    static bool schemeShouldBypassContentSecurityPolicy(const String& scheme);
    
    // Schemes whose responses can be cached indefinitely.
    static bool shouldCacheResponsesFromURLSchemeIndefinitely(const String& scheme);

#if ENABLE(CACHE_PARTITIONING)
    // Schemes whose requests should be partitioned in the cache
    WEBCORE_EXPORT static void registerURLSchemeAsCachePartitioned(const String& scheme);
    static bool shouldPartitionCacheForURLScheme(const String& scheme);
#endif
};

} // namespace WebCore

#endif // SchemeRegistry_h
