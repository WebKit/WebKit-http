/*
 * Copyright (C) 2011 Google, Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY GOOGLE INC. ``AS IS'' AND ANY
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
 */

#include "config.h"
#include "ContentSecurityPolicy.h"

#include "Console.h"
#include "DOMStringList.h"
#include "Document.h"
#include "FormData.h"
#include "FormDataList.h"
#include "Frame.h"
#include "InspectorInstrumentation.h"
#include "InspectorValues.h"
#include "KURL.h"
#include "PingLoader.h"
#include "SchemeRegistry.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"
#include "TextEncoding.h"
#include <wtf/HashSet.h>
#include <wtf/text/TextPosition.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

// Normally WebKit uses "static" for internal linkage, but using "static" for
// these functions causes a compile error because these functions are used as
// template parameters.
namespace {

bool isDirectiveNameCharacter(UChar c)
{
    return isASCIIAlphanumeric(c) || c == '-';
}

bool isDirectiveValueCharacter(UChar c)
{
    return isASCIISpace(c) || (c >= 0x21 && c <= 0x7e); // Whitespace + VCHAR
}

bool isNonceCharacter(UChar c)
{
    return (c >= 0x21 && c <= 0x7e) && c != ',' && c != ';'; // VCHAR - ',' - ';'
}

bool isSourceCharacter(UChar c)
{
    return !isASCIISpace(c);
}

bool isHostCharacter(UChar c)
{
    return isASCIIAlphanumeric(c) || c == '-';
}

bool isSchemeContinuationCharacter(UChar c)
{
    return isASCIIAlphanumeric(c) || c == '+' || c == '-' || c == '.';
}

bool isNotASCIISpace(UChar c)
{
    return !isASCIISpace(c);
}

bool isNotColonOrSlash(UChar c)
{
    return c != ':' && c != '/';
}

bool isMediaTypeCharacter(UChar c)
{
    return !isASCIISpace(c) && c != '/';
}

} // namespace

static bool skipExactly(const UChar*& position, const UChar* end, UChar delimiter)
{
    if (position < end && *position == delimiter) {
        ++position;
        return true;
    }
    return false;
}

template<bool characterPredicate(UChar)>
static bool skipExactly(const UChar*& position, const UChar* end)
{
    if (position < end && characterPredicate(*position)) {
        ++position;
        return true;
    }
    return false;
}

static void skipUntil(const UChar*& position, const UChar* end, UChar delimiter)
{
    while (position < end && *position != delimiter)
        ++position;
}

template<bool characterPredicate(UChar)>
static void skipWhile(const UChar*& position, const UChar* end)
{
    while (position < end && characterPredicate(*position))
        ++position;
}

class CSPSource {
public:
    CSPSource(const String& scheme, const String& host, int port, bool hostHasWildcard, bool portHasWildcard)
        : m_scheme(scheme)
        , m_host(host)
        , m_port(port)
        , m_hostHasWildcard(hostHasWildcard)
        , m_portHasWildcard(portHasWildcard)
    {
    }

    bool matches(const KURL& url) const
    {
        if (!schemeMatches(url))
            return false;
        if (isSchemeOnly())
            return true;
        return hostMatches(url) && portMatches(url);
    }

private:
    bool schemeMatches(const KURL& url) const
    {
        return equalIgnoringCase(url.protocol(), m_scheme);
    }

    bool hostMatches(const KURL& url) const
    {
        const String& host = url.host();
        if (equalIgnoringCase(host, m_host))
            return true;
        return m_hostHasWildcard && host.endsWith("." + m_host, false);

    }

    bool portMatches(const KURL& url) const
    {
        if (m_portHasWildcard)
            return true;

        int port = url.port();

        if (port == m_port)
            return true;

        if (!port)
            return isDefaultPortForProtocol(m_port, m_scheme);

        if (!m_port)
            return isDefaultPortForProtocol(port, m_scheme);

        return false;
    }

    bool isSchemeOnly() const { return m_host.isEmpty(); }

    String m_scheme;
    String m_host;
    int m_port;

    bool m_hostHasWildcard;
    bool m_portHasWildcard;
};

class CSPSourceList {
public:
    CSPSourceList(ContentSecurityPolicy*, const String& directiveName);

    void parse(const String&);
    bool matches(const KURL&);
    bool allowInline() const { return m_allowInline; }
    bool allowEval() const { return m_allowEval; }

private:
    void parse(const UChar* begin, const UChar* end);

    bool parseSource(const UChar* begin, const UChar* end, String& scheme, String& host, int& port, String& path, bool& hostHasWildcard, bool& portHasWildcard);
    bool parseScheme(const UChar* begin, const UChar* end, String& scheme);
    bool parseHost(const UChar* begin, const UChar* end, String& host, bool& hostHasWildcard);
    bool parsePort(const UChar* begin, const UChar* end, int& port, bool& portHasWildcard);
    bool parsePath(const UChar* begin, const UChar* end, String& path);

    void addSourceSelf();
    void addSourceStar();
    void addSourceUnsafeInline();
    void addSourceUnsafeEval();

    ContentSecurityPolicy* m_policy;
    Vector<CSPSource> m_list;
    String m_directiveName;
    bool m_allowStar;
    bool m_allowInline;
    bool m_allowEval;
};

CSPSourceList::CSPSourceList(ContentSecurityPolicy* policy, const String& directiveName)
    : m_policy(policy)
    , m_directiveName(directiveName)
    , m_allowStar(false)
    , m_allowInline(false)
    , m_allowEval(false)
{
}

void CSPSourceList::parse(const String& value)
{
    parse(value.characters(), value.characters() + value.length());
}

bool CSPSourceList::matches(const KURL& url)
{
    if (m_allowStar)
        return true;

    KURL effectiveURL = SecurityOrigin::shouldUseInnerURL(url) ? SecurityOrigin::extractInnerURL(url) : url;

    for (size_t i = 0; i < m_list.size(); ++i) {
        if (m_list[i].matches(effectiveURL))
            return true;
    }

    return false;
}

// source-list       = *WSP [ source *( 1*WSP source ) *WSP ]
//                   / *WSP "'none'" *WSP
//
void CSPSourceList::parse(const UChar* begin, const UChar* end)
{
    const UChar* position = begin;

    bool isFirstSourceInList = true;
    while (position < end) {
        skipWhile<isASCIISpace>(position, end);
        if (position == end)
            return;

        const UChar* beginSource = position;
        skipWhile<isSourceCharacter>(position, end);

        if (isFirstSourceInList && equalIgnoringCase("'none'", beginSource, position - beginSource))
            return; // We represent 'none' as an empty m_list.
        isFirstSourceInList = false;

        String scheme, host, path;
        int port = 0;
        bool hostHasWildcard = false;
        bool portHasWildcard = false;

        if (parseSource(beginSource, position, scheme, host, port, path, hostHasWildcard, portHasWildcard)) {
            // Wildcard hosts and keyword sources ('self', 'unsafe-inline',
            // etc.) aren't stored in m_list, but as attributes on the source
            // list itself.
            if (scheme.isEmpty() && host.isEmpty())
                continue;
            if (scheme.isEmpty())
                scheme = m_policy->securityOrigin()->protocol();
            if (!path.isEmpty())
                m_policy->reportIgnoredPathComponent(m_directiveName, String(beginSource, position - beginSource), path);
            m_list.append(CSPSource(scheme, host, port, hostHasWildcard, portHasWildcard));
        } else
            m_policy->reportInvalidSourceExpression(m_directiveName, String(beginSource, position - beginSource));

        ASSERT(position == end || isASCIISpace(*position));
     }
}

// source            = scheme ":"
//                   / ( [ scheme "://" ] host [ port ] [ path ] )
//                   / "'self'"
//
bool CSPSourceList::parseSource(const UChar* begin, const UChar* end,
                                String& scheme, String& host, int& port, String& path,
                                bool& hostHasWildcard, bool& portHasWildcard)
{
    if (begin == end)
        return false;

    if (end - begin == 1 && *begin == '*') {
        addSourceStar();
        return true;
    }

    if (equalIgnoringCase("'self'", begin, end - begin)) {
        addSourceSelf();
        return true;
    }

    if (equalIgnoringCase("'unsafe-inline'", begin, end - begin)) {
        addSourceUnsafeInline();
        return true;
    }

    if (equalIgnoringCase("'unsafe-eval'", begin, end - begin)) {
        addSourceUnsafeEval();
        return true;
    }

    const UChar* position = begin;
    const UChar* beginHost = begin;
    const UChar* beginPath = end;
    const UChar* beginPort = 0;

    skipWhile<isNotColonOrSlash>(position, end);

    if (position == end) {
        // host
        //     ^
        return parseHost(beginHost, position, host, hostHasWildcard);
    }

    if (position < end && *position == '/') {
        // host/path || host/ || /
        //     ^            ^    ^
        if (!parseHost(beginHost, position, host, hostHasWildcard)
            || !parsePath(position, end, path)
            || position != end)
            return false;
        return true;
    }

    if (position < end && *position == ':') {
        if (end - position == 1) {
            // scheme:
            //       ^
            return parseScheme(begin, position, scheme);
        }

        if (position[1] == '/') {
            // scheme://host || scheme://
            //       ^                ^
            if (!parseScheme(begin, position, scheme)
                || !skipExactly(position, end, ':')
                || !skipExactly(position, end, '/')
                || !skipExactly(position, end, '/'))
                return false;
            if (position == end)
                return true;
            beginHost = position;
            skipWhile<isNotColonOrSlash>(position, end);
        }

        if (position < end && *position == ':') {
            // host:port || scheme://host:port
            //     ^                     ^
            beginPort = position;
            skipUntil(position, end, '/');
        }
    }

    if (position < end && *position == '/') {
        // scheme://host/path || scheme://host:port/path
        //              ^                          ^
        if (position == beginHost)
            return false;

        beginPath = position;
    }

    if (!parseHost(beginHost, beginPort ? beginPort : beginPath, host, hostHasWildcard))
        return false;

    if (beginPort) {
        if (!parsePort(beginPort, beginPath, port, portHasWildcard))
            return false;
    } else {
        port = 0;
    }

    if (beginPath != end) {
        if (!parsePath(beginPath, end, path))
            return false;
    }

    return true;
}

//                     ; <scheme> production from RFC 3986
// scheme      = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
//
bool CSPSourceList::parseScheme(const UChar* begin, const UChar* end, String& scheme)
{
    ASSERT(begin <= end);
    ASSERT(scheme.isEmpty());

    if (begin == end)
        return false;

    const UChar* position = begin;

    if (!skipExactly<isASCIIAlpha>(position, end))
        return false;

    skipWhile<isSchemeContinuationCharacter>(position, end);

    if (position != end)
        return false;

    scheme = String(begin, end - begin);
    return true;
}

// host              = [ "*." ] 1*host-char *( "." 1*host-char )
//                   / "*"
// host-char         = ALPHA / DIGIT / "-"
//
bool CSPSourceList::parseHost(const UChar* begin, const UChar* end, String& host, bool& hostHasWildcard)
{
    ASSERT(begin <= end);
    ASSERT(host.isEmpty());
    ASSERT(!hostHasWildcard);

    if (begin == end)
        return false;

    const UChar* position = begin;

    if (skipExactly(position, end, '*')) {
        hostHasWildcard = true;

        if (position == end)
            return true;

        if (!skipExactly(position, end, '.'))
            return false;
    }

    const UChar* hostBegin = position;

    while (position < end) {
        if (!skipExactly<isHostCharacter>(position, end))
            return false;

        skipWhile<isHostCharacter>(position, end);

        if (position < end && !skipExactly(position, end, '.'))
            return false;
    }

    ASSERT(position == end);
    host = String(hostBegin, end - hostBegin);
    return true;
}

// FIXME: Deal with an actual path. This just sucks up everything to the end of the string.
bool CSPSourceList::parsePath(const UChar* begin, const UChar* end, String& path)
{
    ASSERT(begin <= end);
    ASSERT(path.isEmpty());

    if (begin == end)
        return false;

    path = String(begin, end - begin);
    return true;
}

// port              = ":" ( 1*DIGIT / "*" )
//
bool CSPSourceList::parsePort(const UChar* begin, const UChar* end, int& port, bool& portHasWildcard)
{
    ASSERT(begin <= end);
    ASSERT(!port);
    ASSERT(!portHasWildcard);

    if (!skipExactly(begin, end, ':'))
        ASSERT_NOT_REACHED();

    if (begin == end)
        return false;

    if (end - begin == 1 && *begin == '*') {
        port = 0;
        portHasWildcard = true;
        return true;
    }

    const UChar* position = begin;
    skipWhile<isASCIIDigit>(position, end);

    if (position != end)
        return false;

    bool ok;
    port = charactersToIntStrict(begin, end - begin, &ok);
    return ok;
}

void CSPSourceList::addSourceSelf()
{
    m_list.append(CSPSource(m_policy->securityOrigin()->protocol(), m_policy->securityOrigin()->host(), m_policy->securityOrigin()->port(), false, false));
}

void CSPSourceList::addSourceStar()
{
    m_allowStar = true;
}

void CSPSourceList::addSourceUnsafeInline()
{
    m_allowInline = true;
}

void CSPSourceList::addSourceUnsafeEval()
{
    m_allowEval = true;
}

class CSPDirective {
public:
    CSPDirective(const String& name, const String& value, ContentSecurityPolicy* policy)
        : m_name(name)
        , m_text(name + ' ' + value)
        , m_policy(policy)
    {
    }

    const String& text() const { return m_text; }

protected:
    const ContentSecurityPolicy* policy() const { return m_policy; }

private:
    String m_name;
    String m_text;
    ContentSecurityPolicy* m_policy;
};

class NonceDirective : public CSPDirective {
public:
    NonceDirective(const String& name, const String& value, ContentSecurityPolicy* policy)
        : CSPDirective(name, value, policy)
    {
        parse(value);
    }

    bool allows(const String& nonce) const
    {
        return (!m_scriptNonce.isEmpty() && nonce.stripWhiteSpace() == m_scriptNonce);
    }

private:
    void parse(const String& value)
    {
        String nonce;
        const UChar* position = value.characters();
        const UChar* end = position + value.length();

        skipWhile<isASCIISpace>(position, end);
        const UChar* nonceBegin = position;
        if (position == end) {
            policy()->reportInvalidNonce(String());
            m_scriptNonce = "";
            return;
        }
        skipWhile<isNonceCharacter>(position, end);
        if (nonceBegin < position)
            nonce = String(nonceBegin, position - nonceBegin);

        // Trim off trailing whitespace: If we're not at the end of the string, log
        // an error.
        skipWhile<isASCIISpace>(position, end);
        if (position < end) {
            policy()->reportInvalidNonce(value);
            m_scriptNonce = "";
        } else
            m_scriptNonce = nonce;
    }

    String m_scriptNonce;
};

class MediaListDirective : public CSPDirective {
public:
    MediaListDirective(const String& name, const String& value, ContentSecurityPolicy* policy)
        : CSPDirective(name, value, policy)
    {
        parse(value);
    }

    bool allows(const String& type)
    {
        return m_pluginTypes.contains(type);
    }

private:
    void parse(const String& value)
    {
        const UChar* begin = value.characters();
        const UChar* position = begin;
        const UChar* end = begin + value.length();

        // 'plugin-types ____;' OR 'plugin-types;'
        if (value.isEmpty()) {
            policy()->reportInvalidPluginTypes(value);
            return;
        }

        while (position < end) {
            // _____ OR _____mime1/mime1
            // ^        ^
            skipWhile<isASCIISpace>(position, end);
            if (position == end)
                return;

            // mime1/mime1 mime2/mime2
            // ^
            begin = position;
            if (!skipExactly<isMediaTypeCharacter>(position, end)) {
                skipWhile<isNotASCIISpace>(position, end);
                policy()->reportInvalidPluginTypes(String(begin, position - begin));
                continue;
            }
            skipWhile<isMediaTypeCharacter>(position, end);

            // mime1/mime1 mime2/mime2
            //      ^
            if (!skipExactly(position, end, '/')) {
                skipWhile<isNotASCIISpace>(position, end);
                policy()->reportInvalidPluginTypes(String(begin, position - begin));
                continue;
            }

            // mime1/mime1 mime2/mime2
            //       ^
            if (!skipExactly<isMediaTypeCharacter>(position, end)) {
                skipWhile<isNotASCIISpace>(position, end);
                policy()->reportInvalidPluginTypes(String(begin, position - begin));
                continue;
            }
            skipWhile<isMediaTypeCharacter>(position, end);

            // mime1/mime1 mime2/mime2 OR mime1/mime1  OR mime1/mime1/error
            //            ^                          ^               ^
            if (position < end && isNotASCIISpace(*position)) {
                skipWhile<isNotASCIISpace>(position, end);
                policy()->reportInvalidPluginTypes(String(begin, position - begin));
                continue;
            }
            m_pluginTypes.add(String(begin, position - begin));

            ASSERT(position == end || isASCIISpace(*position));
        }
    }

    HashSet<String> m_pluginTypes;
};

class SourceListDirective : public CSPDirective {
public:
    SourceListDirective(const String& name, const String& value, ContentSecurityPolicy* policy)
        : CSPDirective(name, value, policy)
        , m_sourceList(policy, name)
    {
        m_sourceList.parse(value);
    }

    bool allows(const KURL& url)
    {
        return m_sourceList.matches(url.isEmpty() ? policy()->url() : url);
    }

    bool allowInline() const { return m_sourceList.allowInline(); }
    bool allowEval() const { return m_sourceList.allowEval(); }

private:
    CSPSourceList m_sourceList;
};

class CSPDirectiveList {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<CSPDirectiveList> create(ContentSecurityPolicy*, const String&, ContentSecurityPolicy::HeaderType);

    const String& header() const { return m_header; }
    ContentSecurityPolicy::HeaderType headerType() const { return m_reportOnly ? ContentSecurityPolicy::ReportOnly : ContentSecurityPolicy::EnforcePolicy; }

    bool allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus) const;
    bool allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus) const;
    bool allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus) const;
    bool allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus) const;
    bool allowEval(PassRefPtr<ScriptCallStack>, ContentSecurityPolicy::ReportingStatus) const;
    bool allowScriptNonce(const String& nonce, const String& contextURL, const WTF::OrdinalNumber& contextLine, const KURL&) const;
    bool allowPluginType(const String& type, const String& typeAttribute, const KURL&, ContentSecurityPolicy::ReportingStatus) const;

    bool allowScriptFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowObjectFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowChildFrameFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowImageFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowStyleFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowFontFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowMediaFromSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowConnectToSource(const KURL&, ContentSecurityPolicy::ReportingStatus) const;
    bool allowFormAction(const KURL&, ContentSecurityPolicy::ReportingStatus) const;

    void gatherReportURIs(DOMStringList&) const;
    const String& evalDisabledErrorMessage() { return m_evalDisabledErrorMessage; }

private:
    explicit CSPDirectiveList(ContentSecurityPolicy*);

    void parse(const String&);

    bool parseDirective(const UChar* begin, const UChar* end, String& name, String& value);
    void parseReportURI(const String& name, const String& value);
    void parseScriptNonce(const String& name, const String& value);
    void parsePluginTypes(const String& name, const String& value);
    void addDirective(const String& name, const String& value);
    void applySandboxPolicy(const String& name, const String& sandboxPolicy);

    template <class CSPDirectiveType>
    void setCSPDirective(const String& name, const String& value, OwnPtr<CSPDirectiveType>&);

    SourceListDirective* operativeDirective(SourceListDirective*) const;
    void reportViolation(const String& directiveText, const String& consoleMessage, const KURL& blockedURL = KURL(), const String& contextURL = String(), const WTF::OrdinalNumber& contextLine = WTF::OrdinalNumber::beforeFirst(), PassRefPtr<ScriptCallStack> = 0) const;

    bool checkEval(SourceListDirective*) const;
    bool checkInline(SourceListDirective*) const;
    bool checkNonce(NonceDirective*, const String&) const;
    bool checkSource(SourceListDirective*, const KURL&) const;
    bool checkMediaType(MediaListDirective*, const String& type, const String& typeAttribute) const;

    void setEvalDisabledErrorMessage(const String& errorMessage) { m_evalDisabledErrorMessage = errorMessage; }

    bool checkEvalAndReportViolation(SourceListDirective*, const String& consoleMessage, const String& contextURL = String(), const WTF::OrdinalNumber& contextLine = WTF::OrdinalNumber::beforeFirst(), PassRefPtr<ScriptCallStack> = 0) const;
    bool checkInlineAndReportViolation(SourceListDirective*, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine, bool isScript) const;
    bool checkNonceAndReportViolation(NonceDirective*, const String& nonce, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine) const;

    bool checkSourceAndReportViolation(SourceListDirective*, const KURL&, const String& type) const;
    bool checkMediaTypeAndReportViolation(MediaListDirective*, const String& type, const String& typeAttribute, const String& consoleMessage) const;

    bool denyIfEnforcingPolicy() const { return m_reportOnly; }

    ContentSecurityPolicy* m_policy;

    String m_header;

    bool m_reportOnly;
    bool m_haveSandboxPolicy;

    OwnPtr<MediaListDirective> m_pluginTypes;
    OwnPtr<NonceDirective> m_scriptNonce;
    OwnPtr<SourceListDirective> m_connectSrc;
    OwnPtr<SourceListDirective> m_defaultSrc;
    OwnPtr<SourceListDirective> m_fontSrc;
    OwnPtr<SourceListDirective> m_formAction;
    OwnPtr<SourceListDirective> m_frameSrc;
    OwnPtr<SourceListDirective> m_imgSrc;
    OwnPtr<SourceListDirective> m_mediaSrc;
    OwnPtr<SourceListDirective> m_objectSrc;
    OwnPtr<SourceListDirective> m_scriptSrc;
    OwnPtr<SourceListDirective> m_styleSrc;

    Vector<KURL> m_reportURIs;

    String m_evalDisabledErrorMessage;
};

CSPDirectiveList::CSPDirectiveList(ContentSecurityPolicy* policy)
    : m_policy(policy)
    , m_reportOnly(false)
    , m_haveSandboxPolicy(false)
{
}

PassOwnPtr<CSPDirectiveList> CSPDirectiveList::create(ContentSecurityPolicy* policy, const String& header, ContentSecurityPolicy::HeaderType type)
{
    OwnPtr<CSPDirectiveList> directives = adoptPtr(new CSPDirectiveList(policy));
    directives->parse(header);
    directives->m_header = header;

    switch (type) {
    case ContentSecurityPolicy::ReportOnly:
        directives->m_reportOnly = true;
        return directives.release();
    case ContentSecurityPolicy::EnforcePolicy:
        ASSERT(!directives->m_reportOnly);
        break;
    }

    if (!directives->checkEval(directives->operativeDirective(directives->m_scriptSrc.get()))) {
        String message = makeString("Refused to evaluate a string as JavaScript because 'unsafe-eval' is not an allowed source of script in the following Content Security Policy directive: \"", directives->operativeDirective(directives->m_scriptSrc.get())->text(), "\".\n");
        directives->setEvalDisabledErrorMessage(message);
    }

    return directives.release();
}

void CSPDirectiveList::reportViolation(const String& directiveText, const String& consoleMessage, const KURL& blockedURL, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    String message = m_reportOnly ? "[Report Only] " + consoleMessage : consoleMessage;
    m_policy->reportViolation(directiveText, message, blockedURL, m_reportURIs, m_header, contextURL, contextLine, callStack);
}

bool CSPDirectiveList::checkEval(SourceListDirective* directive) const
{
    return !directive || directive->allowEval();
}

bool CSPDirectiveList::checkInline(SourceListDirective* directive) const
{
    return !directive || directive->allowInline();
}

bool CSPDirectiveList::checkNonce(NonceDirective* directive, const String& nonce) const
{
    return !directive || directive->allows(nonce);
}

bool CSPDirectiveList::checkSource(SourceListDirective* directive, const KURL& url) const
{
    return !directive || directive->allows(url);
}

bool CSPDirectiveList::checkMediaType(MediaListDirective* directive, const String& type, const String& typeAttribute) const
{
    if (!directive)
        return true;
    if (typeAttribute.isEmpty() || typeAttribute.stripWhiteSpace() != type)
        return false;
    return directive->allows(type);
}

SourceListDirective* CSPDirectiveList::operativeDirective(SourceListDirective* directive) const
{
    return directive ? directive : m_defaultSrc.get();
}

bool CSPDirectiveList::checkEvalAndReportViolation(SourceListDirective* directive, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    if (checkEval(directive))
        return true;
    reportViolation(directive->text(), consoleMessage + "\"" + directive->text() + "\".\n", KURL(), contextURL, contextLine, callStack);
    if (!m_reportOnly) {
        m_policy->reportBlockedScriptExecutionToInspector(directive->text());
        return false;
    }
    return true;
}

bool CSPDirectiveList::checkNonceAndReportViolation(NonceDirective* directive, const String& nonce, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    if (checkNonce(directive, nonce))
        return true;
    reportViolation(directive->text(), consoleMessage + "\"" + directive->text() + "\".\n", KURL(), contextURL, contextLine);
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::checkMediaTypeAndReportViolation(MediaListDirective* directive, const String& type, const String& typeAttribute, const String& consoleMessage) const
{
    if (checkMediaType(directive, type, typeAttribute))
        return true;

    String message = makeString(consoleMessage, "\'", directive->text(), "\'.");
    if (typeAttribute.isEmpty())
        message = message + " When enforcing the 'plugin-types' directive, the plugin's media type must be explicitly declared with a 'type' attribute on the containing element (e.g. '<object type=\"[TYPE GOES HERE]\" ...>').";

    reportViolation(directive->text(), message + "\n", KURL());
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::checkInlineAndReportViolation(SourceListDirective* directive, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine, bool isScript) const
{
    if (checkInline(directive))
        return true;
    reportViolation(directive->text(), consoleMessage + "\"" + directive->text() + "\".\n", KURL(), contextURL, contextLine);

    if (!m_reportOnly) {
        if (isScript)
            m_policy->reportBlockedScriptExecutionToInspector(directive->text());
        return false;
    }
    return true;
}

bool CSPDirectiveList::checkSourceAndReportViolation(SourceListDirective* directive, const KURL& url, const String& type) const
{
    if (checkSource(directive, url))
        return true;

    String prefix = makeString("Refused to load the ", type, " '");
    if (type == "connect")
        prefix = "Refused to connect to '";
    if (type == "form")
        prefix = "Refused to send form data to '";

    reportViolation(directive->text(), makeString(prefix, url.string(), "' because it violates the following Content Security Policy directive: \"", directive->text(), "\".\n"), url);
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to execute JavaScript URL because it violates the following Content Security Policy directive: ")));
    if (reportingStatus == ContentSecurityPolicy::SendReport) {
        return (checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine, true)
                && checkNonceAndReportViolation(m_scriptNonce.get(), String(), consoleMessage, contextURL, contextLine));
    } else {
        return (checkInline(operativeDirective(m_scriptSrc.get()))
                && checkNonce(m_scriptNonce.get(), String()));
    }
}

bool CSPDirectiveList::allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to execute inline event handler because it violates the following Content Security Policy directive: ")));
    if (reportingStatus == ContentSecurityPolicy::SendReport) {
        return (checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine, true)
                && checkNonceAndReportViolation(m_scriptNonce.get(), String(), consoleMessage, contextURL, contextLine));
    } else {
        return (checkInline(operativeDirective(m_scriptSrc.get()))
                && checkNonce(m_scriptNonce.get(), String()));
    }
}

bool CSPDirectiveList::allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to execute inline script because it violates the following Content Security Policy directive: ")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine, true) :
        checkInline(operativeDirective(m_scriptSrc.get()));
}

bool CSPDirectiveList::allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to apply inline style because it violates the following Content Security Policy directive: ")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkInlineAndReportViolation(operativeDirective(m_styleSrc.get()), consoleMessage, contextURL, contextLine, false) :
        checkInline(operativeDirective(m_styleSrc.get()));
}

bool CSPDirectiveList::allowEval(PassRefPtr<ScriptCallStack> callStack, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to evaluate script because it violates the following Content Security Policy directive: ")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkEvalAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, String(), WTF::OrdinalNumber::beforeFirst(), callStack) :
        checkEval(operativeDirective(m_scriptSrc.get()));
}

bool CSPDirectiveList::allowScriptNonce(const String& nonce, const String& contextURL, const WTF::OrdinalNumber& contextLine, const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, (ASCIILiteral("Refused to execute script because it violates the following Content Security Policy directive: ")));
    if (url.isEmpty())
        return checkNonceAndReportViolation(m_scriptNonce.get(), nonce, consoleMessage, contextURL, contextLine);
    return checkNonceAndReportViolation(m_scriptNonce.get(), nonce, "Refused to load '" + url.string() + "' because it violates the following Content Security Policy directive: ", contextURL, contextLine);
}

bool CSPDirectiveList::allowPluginType(const String& type, const String& typeAttribute, const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkMediaTypeAndReportViolation(m_pluginTypes.get(), type, typeAttribute, "Refused to load '" + url.string() + "' (MIME type '" + typeAttribute + "') because it violates the following Content Security Policy Directive: ") :
        checkMediaType(m_pluginTypes.get(), type, typeAttribute);
}

bool CSPDirectiveList::allowScriptFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("script")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_scriptSrc.get()), url, type) :
        checkSource(operativeDirective(m_scriptSrc.get()), url);
}

bool CSPDirectiveList::allowObjectFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("object")));
    if (url.isBlankURL())
        return true;
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_objectSrc.get()), url, type) :
        checkSource(operativeDirective(m_objectSrc.get()), url);
}

bool CSPDirectiveList::allowChildFrameFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("frame")));
    if (url.isBlankURL())
        return true;
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_frameSrc.get()), url, type) :
        checkSource(operativeDirective(m_frameSrc.get()), url);
}

bool CSPDirectiveList::allowImageFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("image")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_imgSrc.get()), url, type) :
        checkSource(operativeDirective(m_imgSrc.get()), url);
}

bool CSPDirectiveList::allowStyleFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("style")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_styleSrc.get()), url, type) :
        checkSource(operativeDirective(m_styleSrc.get()), url);
}

bool CSPDirectiveList::allowFontFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("font")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_fontSrc.get()), url, type) :
        checkSource(operativeDirective(m_fontSrc.get()), url);
}

bool CSPDirectiveList::allowMediaFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("media")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_mediaSrc.get()), url, type) :
        checkSource(operativeDirective(m_mediaSrc.get()), url);
}

bool CSPDirectiveList::allowConnectToSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("connect")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(operativeDirective(m_connectSrc.get()), url, type) :
        checkSource(operativeDirective(m_connectSrc.get()), url);
}

void CSPDirectiveList::gatherReportURIs(DOMStringList& list) const
{
    for (size_t i = 0; i < m_reportURIs.size(); ++i)
        list.append(m_reportURIs[i].string());
}

bool CSPDirectiveList::allowFormAction(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    DEFINE_STATIC_LOCAL(String, type, (ASCIILiteral("form")));
    return reportingStatus == ContentSecurityPolicy::SendReport ?
        checkSourceAndReportViolation(m_formAction.get(), url, type) :
        checkSource(m_formAction.get(), url);
}

// policy            = directive-list
// directive-list    = [ directive *( ";" [ directive ] ) ]
//
void CSPDirectiveList::parse(const String& policy)
{
    if (policy.isEmpty())
        return;

    const UChar* position = policy.characters();
    const UChar* end = position + policy.length();

    while (position < end) {
        const UChar* directiveBegin = position;
        skipUntil(position, end, ';');

        String name, value;
        if (parseDirective(directiveBegin, position, name, value)) {
            ASSERT(!name.isEmpty());
            addDirective(name, value);
        }

        ASSERT(position == end || *position == ';');
        skipExactly(position, end, ';');
    }
}

// directive         = *WSP [ directive-name [ WSP directive-value ] ]
// directive-name    = 1*( ALPHA / DIGIT / "-" )
// directive-value   = *( WSP / <VCHAR except ";"> )
//
bool CSPDirectiveList::parseDirective(const UChar* begin, const UChar* end, String& name, String& value)
{
    ASSERT(name.isEmpty());
    ASSERT(value.isEmpty());

    const UChar* position = begin;
    skipWhile<isASCIISpace>(position, end);

    // Empty directive (e.g. ";;;"). Exit early.
    if (position == end)
        return false;

    const UChar* nameBegin = position;
    skipWhile<isDirectiveNameCharacter>(position, end);

    // The directive-name must be non-empty.
    if (nameBegin == position) {
        skipWhile<isNotASCIISpace>(position, end);
        m_policy->reportUnrecognizedDirective(String(nameBegin, position - nameBegin));
        return false;
    }

    name = String(nameBegin, position - nameBegin);

    if (position == end)
        return true;

    if (!skipExactly<isASCIISpace>(position, end)) {
        skipWhile<isNotASCIISpace>(position, end);
        m_policy->reportUnrecognizedDirective(String(nameBegin, position - nameBegin));
        return false;
    }

    skipWhile<isASCIISpace>(position, end);

    const UChar* valueBegin = position;
    skipWhile<isDirectiveValueCharacter>(position, end);

    if (position != end) {
        m_policy->reportInvalidDirectiveValueCharacter(name, String(valueBegin, end - valueBegin));
        return false;
    }

    // The directive-value may be empty.
    if (valueBegin == position)
        return true;

    value = String(valueBegin, position - valueBegin);
    return true;
}

void CSPDirectiveList::parseReportURI(const String& name, const String& value)
{
    if (!m_reportURIs.isEmpty()) {
        m_policy->reportDuplicateDirective(name);
        return;
    }
    const UChar* position = value.characters();
    const UChar* end = position + value.length();

    while (position < end) {
        skipWhile<isASCIISpace>(position, end);

        const UChar* urlBegin = position;
        skipWhile<isNotASCIISpace>(position, end);

        if (urlBegin < position) {
            String url = String(urlBegin, position - urlBegin);
            m_reportURIs.append(m_policy->completeURL(url));
        }
    }
}


template<class CSPDirectiveType>
void CSPDirectiveList::setCSPDirective(const String& name, const String& value, OwnPtr<CSPDirectiveType>& directive)
{
    if (directive) {
        m_policy->reportDuplicateDirective(name);
        return;
    }
    directive = adoptPtr(new CSPDirectiveType(name, value, m_policy));
}

void CSPDirectiveList::applySandboxPolicy(const String& name, const String& sandboxPolicy)
{
    if (m_haveSandboxPolicy) {
        m_policy->reportDuplicateDirective(name);
        return;
    }
    m_haveSandboxPolicy = true;
    m_policy->enforceSandboxFlags(SecurityContext::parseSandboxPolicy(sandboxPolicy));
}

void CSPDirectiveList::addDirective(const String& name, const String& value)
{
    DEFINE_STATIC_LOCAL(String, defaultSrc, (ASCIILiteral("default-src")));
    DEFINE_STATIC_LOCAL(String, scriptSrc, (ASCIILiteral("script-src")));
    DEFINE_STATIC_LOCAL(String, objectSrc, (ASCIILiteral("object-src")));
    DEFINE_STATIC_LOCAL(String, frameSrc, (ASCIILiteral("frame-src")));
    DEFINE_STATIC_LOCAL(String, imgSrc, (ASCIILiteral("img-src")));
    DEFINE_STATIC_LOCAL(String, styleSrc, (ASCIILiteral("style-src")));
    DEFINE_STATIC_LOCAL(String, fontSrc, (ASCIILiteral("font-src")));
    DEFINE_STATIC_LOCAL(String, mediaSrc, (ASCIILiteral("media-src")));
    DEFINE_STATIC_LOCAL(String, connectSrc, (ASCIILiteral("connect-src")));
    DEFINE_STATIC_LOCAL(String, sandbox, (ASCIILiteral("sandbox")));
    DEFINE_STATIC_LOCAL(String, reportURI, (ASCIILiteral("report-uri")));
#if ENABLE(CSP_NEXT)
    DEFINE_STATIC_LOCAL(String, formAction, (ASCIILiteral("form-action")));
    DEFINE_STATIC_LOCAL(String, pluginTypes, (ASCIILiteral("plugin-types")));
    DEFINE_STATIC_LOCAL(String, scriptNonce, (ASCIILiteral("script-nonce")));
#endif

    ASSERT(!name.isEmpty());

    if (equalIgnoringCase(name, defaultSrc))
        setCSPDirective<SourceListDirective>(name, value, m_defaultSrc);
    else if (equalIgnoringCase(name, scriptSrc))
        setCSPDirective<SourceListDirective>(name, value, m_scriptSrc);
    else if (equalIgnoringCase(name, objectSrc))
        setCSPDirective<SourceListDirective>(name, value, m_objectSrc);
    else if (equalIgnoringCase(name, frameSrc))
        setCSPDirective<SourceListDirective>(name, value, m_frameSrc);
    else if (equalIgnoringCase(name, imgSrc))
        setCSPDirective<SourceListDirective>(name, value, m_imgSrc);
    else if (equalIgnoringCase(name, styleSrc))
        setCSPDirective<SourceListDirective>(name, value, m_styleSrc);
    else if (equalIgnoringCase(name, fontSrc))
        setCSPDirective<SourceListDirective>(name, value, m_fontSrc);
    else if (equalIgnoringCase(name, mediaSrc))
        setCSPDirective<SourceListDirective>(name, value, m_mediaSrc);
    else if (equalIgnoringCase(name, connectSrc))
        setCSPDirective<SourceListDirective>(name, value, m_connectSrc);
    else if (equalIgnoringCase(name, sandbox))
        applySandboxPolicy(name, value);
    else if (equalIgnoringCase(name, reportURI))
        parseReportURI(name, value);
#if ENABLE(CSP_NEXT)
    else if (equalIgnoringCase(name, formAction))
        setCSPDirective<SourceListDirective>(name, value, m_formAction);
    else if (equalIgnoringCase(name, pluginTypes))
        setCSPDirective<MediaListDirective>(name, value, m_pluginTypes);
    else if (equalIgnoringCase(name, scriptNonce))
        setCSPDirective<NonceDirective>(name, value, m_scriptNonce);
#endif
    else
        m_policy->reportUnrecognizedDirective(name);
}

ContentSecurityPolicy::ContentSecurityPolicy(ScriptExecutionContext* scriptExecutionContext)
    : m_scriptExecutionContext(scriptExecutionContext)
    , m_overrideInlineStyleAllowed(false)
{
}

ContentSecurityPolicy::~ContentSecurityPolicy()
{
}

void ContentSecurityPolicy::copyStateFrom(const ContentSecurityPolicy* other) 
{
    ASSERT(m_policies.isEmpty());
    for (CSPDirectiveListVector::const_iterator iter = other->m_policies.begin(); iter != other->m_policies.end(); ++iter)
        didReceiveHeader((*iter)->header(), (*iter)->headerType());
}

void ContentSecurityPolicy::didReceiveHeader(const String& header, HeaderType type)
{
    // RFC2616, section 4.2 specifies that headers appearing multiple times can
    // be combined with a comma. Walk the header string, and parse each comma
    // separated chunk as a separate header.
    const UChar* begin = header.characters();
    const UChar* position = begin;
    const UChar* end = begin + header.length();
    while (position < end) {
        skipUntil(position, end, ',');

        // header1,header2 OR header1
        //        ^                  ^
        m_policies.append(CSPDirectiveList::create(this, String(begin, position - begin), type));

        // Skip the comma, and begin the next header from the current position.
        ASSERT(position == end || *position == ',');
        skipExactly(position, end, ',');
        begin = position;
    }

    if (!allowEval(0, SuppressReport))
        m_scriptExecutionContext->disableEval(evalDisabledErrorMessage());
}

void ContentSecurityPolicy::setOverrideAllowInlineStyle(bool value)
{
    m_overrideInlineStyleAllowed = value;
}

const String& ContentSecurityPolicy::deprecatedHeader() const
{
    return m_policies.isEmpty() ? emptyString() : m_policies[0]->header();
}

ContentSecurityPolicy::HeaderType ContentSecurityPolicy::deprecatedHeaderType() const
{
    return m_policies.isEmpty() ? EnforcePolicy : m_policies[0]->headerType();
}

template<bool (CSPDirectiveList::*allowed)(PassRefPtr<ScriptCallStack>, ContentSecurityPolicy::ReportingStatus) const>
bool isAllowedByAllWithCallStack(const CSPDirectiveListVector& policies, PassRefPtr<ScriptCallStack> callStack, ContentSecurityPolicy::ReportingStatus reportingStatus)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowed)(callStack, reportingStatus))
            return false;
    }
    return true;
}

template<bool (CSPDirectiveList::*allowed)(const String&, const WTF::OrdinalNumber&, ContentSecurityPolicy::ReportingStatus) const>
bool isAllowedByAllWithContext(const CSPDirectiveListVector& policies, const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowed)(contextURL, contextLine, reportingStatus))
            return false;
    }
    return true;
}

template<bool (CSPDirectiveList::*allowed)(const String&, const String&, const WTF::OrdinalNumber&, const KURL&) const>
bool isAllowedByAllWithNonce(const CSPDirectiveListVector& policies, const String& nonce, const String& contextURL, const WTF::OrdinalNumber& contextLine, const KURL& url)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowed)(nonce, contextURL, contextLine, url))
            return false;
    }
    return true;
}

template<bool (CSPDirectiveList::*allowFromURL)(const KURL&, ContentSecurityPolicy::ReportingStatus) const>
bool isAllowedByAllWithURL(const CSPDirectiveListVector& policies, const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus)
{
    if (SchemeRegistry::schemeShouldBypassContentSecurityPolicy(url.protocol()))
        return true;

    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowFromURL)(url, reportingStatus))
            return false;
    }
    return true;
}

bool ContentSecurityPolicy::allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowJavaScriptURLs>(m_policies, contextURL, contextLine, reportingStatus);
}

bool ContentSecurityPolicy::allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineEventHandlers>(m_policies, contextURL, contextLine, reportingStatus);
}

bool ContentSecurityPolicy::allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineScript>(m_policies, contextURL, contextLine, reportingStatus);
}

bool ContentSecurityPolicy::allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    if (m_overrideInlineStyleAllowed)
        return true;
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineStyle>(m_policies, contextURL, contextLine, reportingStatus);
}

bool ContentSecurityPolicy::allowEval(PassRefPtr<ScriptCallStack> callStack, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithCallStack<&CSPDirectiveList::allowEval>(m_policies, callStack, reportingStatus);
}

String ContentSecurityPolicy::evalDisabledErrorMessage() const
{
    for (size_t i = 0; i < m_policies.size(); ++i) {
        if (!m_policies[i]->allowEval(0, SuppressReport))
            return m_policies[i]->evalDisabledErrorMessage();
    }
    return String();
}

bool ContentSecurityPolicy::allowScriptNonce(const String& nonce, const String& contextURL, const WTF::OrdinalNumber& contextLine, const KURL& url) const
{
    return isAllowedByAllWithNonce<&CSPDirectiveList::allowScriptNonce>(m_policies, nonce, contextURL, contextLine, url);
}

bool ContentSecurityPolicy::allowPluginType(const String& type, const String& typeAttribute, const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    for (size_t i = 0; i < m_policies.size(); ++i) {
        if (!m_policies[i]->allowPluginType(type, typeAttribute, url, reportingStatus))
            return false;
    }
    return true;
}

bool ContentSecurityPolicy::allowScriptFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowScriptFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowObjectFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowObjectFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowChildFrameFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowChildFrameFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowImageFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowImageFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowStyleFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowStyleFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowFontFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowFontFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowMediaFromSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowMediaFromSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowConnectToSource(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowConnectToSource>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::allowFormAction(const KURL& url, ContentSecurityPolicy::ReportingStatus reportingStatus) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowFormAction>(m_policies, url, reportingStatus);
}

bool ContentSecurityPolicy::isActive() const
{
    return !m_policies.isEmpty();
}

void ContentSecurityPolicy::gatherReportURIs(DOMStringList& list) const
{
    for (size_t i = 0; i < m_policies.size(); ++i)
        m_policies[i]->gatherReportURIs(list);
}

SecurityOrigin* ContentSecurityPolicy::securityOrigin() const
{
    return m_scriptExecutionContext->securityOrigin();
}

const KURL& ContentSecurityPolicy::url() const
{
    return m_scriptExecutionContext->url();
}

KURL ContentSecurityPolicy::completeURL(const String& url) const
{
    return m_scriptExecutionContext->completeURL(url);
}

void ContentSecurityPolicy::enforceSandboxFlags(SandboxFlags mask) const
{
    m_scriptExecutionContext->enforceSandboxFlags(mask);
}

void ContentSecurityPolicy::reportViolation(const String& directiveText, const String& consoleMessage, const KURL& blockedURL, const Vector<KURL>& reportURIs, const String& header, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    logToConsole(consoleMessage, contextURL, contextLine, callStack);

    if (reportURIs.isEmpty())
        return;

    // FIXME: Support sending reports from worker.
    if (!m_scriptExecutionContext->isDocument())
        return;

    Document* document = static_cast<Document*>(m_scriptExecutionContext);
    Frame* frame = document->frame();
    if (!frame)
        return;

    // We need to be careful here when deciding what information to send to the
    // report-uri. Currently, we send only the current document's URL and the
    // directive that was violated. The document's URL is safe to send because
    // it's the document itself that's requesting that it be sent. You could
    // make an argument that we shouldn't send HTTPS document URLs to HTTP
    // report-uris (for the same reasons that we supress the Referer in that
    // case), but the Referer is sent implicitly whereas this request is only
    // sent explicitly. As for which directive was violated, that's pretty
    // harmless information.

    RefPtr<InspectorObject> cspReport = InspectorObject::create();
    cspReport->setString("document-uri", document->url().strippedForUseAsReferrer());
    String referrer = document->referrer();
    if (!referrer.isEmpty())
        cspReport->setString("referrer", referrer);
    if (!directiveText.isEmpty())
        cspReport->setString("violated-directive", directiveText);
    cspReport->setString("original-policy", header);
    if (blockedURL.isValid())
        cspReport->setString("blocked-uri", document->securityOrigin()->canRequest(blockedURL) ? blockedURL.strippedForUseAsReferrer() : SecurityOrigin::create(blockedURL)->toString());

    RefPtr<InspectorObject> reportObject = InspectorObject::create();
    reportObject->setObject("csp-report", cspReport.release());

    RefPtr<FormData> report = FormData::create(reportObject->toJSONString().utf8());

    for (size_t i = 0; i < reportURIs.size(); ++i)
        PingLoader::reportContentSecurityPolicyViolation(frame, reportURIs[i], report);
}

void ContentSecurityPolicy::reportUnrecognizedDirective(const String& name) const
{
    String message = makeString("Unrecognized Content-Security-Policy directive '", name, "'.\n");
    logToConsole(message);
}

void ContentSecurityPolicy::reportDuplicateDirective(const String& name) const
{
    String message = makeString("Ignoring duplicate Content-Security-Policy directive '", name, "'.\n");
    logToConsole(message);
}

void ContentSecurityPolicy::reportInvalidPluginTypes(const String& pluginType) const
{
    String message;
    if (pluginType.isNull())
        message = "'plugin-types' Content Security Policy directive is empty; all plugins will be blocked.\n";
    else
        message = makeString("Invalid plugin type in 'plugin-types' Content Security Policy directive: '", pluginType, "'.\n");
    logToConsole(message);
}

void ContentSecurityPolicy::reportInvalidDirectiveValueCharacter(const String& directiveName, const String& value) const
{
    String message = makeString("The value for Content Security Policy directive '", directiveName, "' contains an invalid character: '", value, "'. Non-whitespace characters outside ASCII 0x21-0x7E must be percent-encoded, as described in RFC 3986, section 2.1: http://tools.ietf.org/html/rfc3986#section-2.1.");
    logToConsole(message);
}

void ContentSecurityPolicy::reportInvalidNonce(const String& nonce) const
{
    String message = makeString("Ignoring invalid Content Security Policy script nonce: '", nonce, "'.\n");
    logToConsole(message);
}

void ContentSecurityPolicy::reportIgnoredPathComponent(const String& directiveName, const String& completeSource, const String& path) const
{
    String message = makeString("The source list for Content Security Policy directive '", directiveName, "' contains the source '", completeSource, "'. Content Security Policy 1.0 supports only schemes, hosts, and ports. Paths might be supported in the future, but for now, '", path, "' is being ignored. Be careful.");
    logToConsole(message);
}

void ContentSecurityPolicy::reportInvalidSourceExpression(const String& directiveName, const String& source) const
{
    String message = makeString("The source list for Content Security Policy directive '", directiveName, "' contains an invalid source: '", source, "'. It will be ignored.");
    logToConsole(message);
}

void ContentSecurityPolicy::logToConsole(const String& message, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    m_scriptExecutionContext->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, contextURL, contextLine.oneBasedInt(), callStack);
}

void ContentSecurityPolicy::reportBlockedScriptExecutionToInspector(const String& directiveText) const
{
    InspectorInstrumentation::scriptExecutionBlockedByCSP(m_scriptExecutionContext, directiveText);
}

}
