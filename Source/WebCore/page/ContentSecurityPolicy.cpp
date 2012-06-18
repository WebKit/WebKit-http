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
#include "Document.h"
#include "FormData.h"
#include "FormDataList.h"
#include "Frame.h"
#include "InspectorInstrumentation.h"
#include "InspectorValues.h"
#include "PingLoader.h"
#include "ScriptCallStack.h"
#include "SecurityOrigin.h"
#include "TextEncoding.h"
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
    explicit CSPSourceList(SecurityOrigin*);

    void parse(const String&);
    bool matches(const KURL&);
    bool allowInline() const { return m_allowInline; }
    bool allowEval() const { return m_allowEval; }

private:
    void parse(const UChar* begin, const UChar* end);

    bool parseSource(const UChar* begin, const UChar* end, String& scheme, String& host, int& port, bool& hostHasWildcard, bool& portHasWildcard);
    bool parseScheme(const UChar* begin, const UChar* end, String& scheme);
    bool parseHost(const UChar* begin, const UChar* end, String& host, bool& hostHasWildcard);
    bool parsePort(const UChar* begin, const UChar* end, int& port, bool& portHasWildcard);
    bool parsePath(const UChar* begin, const UChar* end, String& path);

    void addSourceSelf();
    void addSourceStar();
    void addSourceUnsafeInline();
    void addSourceUnsafeEval();

    SecurityOrigin* m_origin;
    Vector<CSPSource> m_list;
    bool m_allowStar;
    bool m_allowInline;
    bool m_allowEval;
};

CSPSourceList::CSPSourceList(SecurityOrigin* origin)
    : m_origin(origin)
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

    for (size_t i = 0; i < m_list.size(); ++i) {
        if (m_list[i].matches(url))
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
        const UChar* beginSource = position;
        skipWhile<isSourceCharacter>(position, end);

        if (isFirstSourceInList && equalIgnoringCase("'none'", beginSource, position - beginSource))
            return; // We represent 'none' as an empty m_list.
        isFirstSourceInList = false;

        String scheme, host;
        int port = 0;
        bool hostHasWildcard = false;
        bool portHasWildcard = false;

        if (parseSource(beginSource, position, scheme, host, port, hostHasWildcard, portHasWildcard)) {
            if (scheme.isEmpty())
                scheme = m_origin->protocol();
            m_list.append(CSPSource(scheme, host, port, hostHasWildcard, portHasWildcard));
        }

        ASSERT(position == end || isASCIISpace(*position));
     }
}

// source            = scheme ":"
//                   / ( [ scheme "://" ] host [ port ] [ path ] )
//                   / "'self'"
//
bool CSPSourceList::parseSource(const UChar* begin, const UChar* end,
                                String& scheme, String& host, int& port,
                                bool& hostHasWildcard, bool& portHasWildcard)
{
    String path; // FIXME: We're ignoring the path component for now.

    if (begin == end)
        return false;

    if (end - begin == 1 && *begin == '*') {
        addSourceStar();
        return false;
    }

    if (equalIgnoringCase("'self'", begin, end - begin)) {
        addSourceSelf();
        return false;
    }

    if (equalIgnoringCase("'unsafe-inline'", begin, end - begin)) {
        addSourceUnsafeInline();
        return false;
    }

    if (equalIgnoringCase("'unsafe-eval'", begin, end - begin)) {
        addSourceUnsafeEval();
        return false;
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
    m_list.append(CSPSource(m_origin->protocol(), m_origin->host(), m_origin->port(), false, false));
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
    CSPDirective(const String& name, const String& value, ScriptExecutionContext* context)
        : m_sourceList(context->securityOrigin())
        , m_text(name + ' ' + value)
        , m_selfURL(context->url())
    {
        m_sourceList.parse(value);
    }

    bool allows(const KURL& url)
    {
        return m_sourceList.matches(url.isEmpty() ? m_selfURL : url);
    }

    bool allowInline() const { return m_sourceList.allowInline(); }
    bool allowEval() const { return m_sourceList.allowEval(); }

    const String& text() { return m_text; }

private:
    CSPSourceList m_sourceList;
    String m_text;
    KURL m_selfURL;
};

class CSPDirectiveList {
public:
    static PassOwnPtr<CSPDirectiveList> create(ScriptExecutionContext*, const String&, ContentSecurityPolicy::HeaderType);

    const String& header() const { return m_header; }
    ContentSecurityPolicy::HeaderType headerType() const { return m_reportOnly ? ContentSecurityPolicy::ReportOnly : ContentSecurityPolicy::EnforcePolicy; }

    bool allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine) const;
    bool allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine) const;
    bool allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine) const;
    bool allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine) const;
    bool allowEval(PassRefPtr<ScriptCallStack>) const;

    bool allowScriptFromSource(const KURL&) const;
    bool allowObjectFromSource(const KURL&) const;
    bool allowChildFrameFromSource(const KURL&) const;
    bool allowImageFromSource(const KURL&) const;
    bool allowStyleFromSource(const KURL&) const;
    bool allowFontFromSource(const KURL&) const;
    bool allowMediaFromSource(const KURL&) const;
    bool allowConnectToSource(const KURL&) const;

private:
    explicit CSPDirectiveList(ScriptExecutionContext*);

    void parse(const String&);

    bool parseDirective(const UChar* begin, const UChar* end, String& name, String& value);
    void parseReportURI(const String& name, const String& value);
    void addDirective(const String& name, const String& value);
    void applySandboxPolicy(const String& name, const String& sandboxPolicy);

    void setCSPDirective(const String& name, const String& value, OwnPtr<CSPDirective>&);

    CSPDirective* operativeDirective(CSPDirective*) const;
    void reportViolation(const String& directiveText, const String& consoleMessage, const KURL& blockedURL = KURL(), const String& contextURL = String(), const WTF::OrdinalNumber& contextLine = WTF::OrdinalNumber::beforeFirst(), PassRefPtr<ScriptCallStack> = 0) const;
    void logUnrecognizedDirective(const String& name) const;
    void logDuplicateDirective(const String& name) const;
    bool checkEval(CSPDirective*) const;

    bool checkInlineAndReportViolation(CSPDirective*, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine) const;
    bool checkEvalAndReportViolation(CSPDirective*, const String& consoleMessage, const String& contextURL = String(), const WTF::OrdinalNumber& contextLine = WTF::OrdinalNumber::beforeFirst(), PassRefPtr<ScriptCallStack> = 0) const;
    bool checkSourceAndReportViolation(CSPDirective*, const KURL&, const String& type) const;

    bool denyIfEnforcingPolicy() const { return m_reportOnly; }

    ScriptExecutionContext* m_scriptExecutionContext;
    String m_header;

    bool m_reportOnly;
    bool m_haveSandboxPolicy;

    OwnPtr<CSPDirective> m_defaultSrc;
    OwnPtr<CSPDirective> m_scriptSrc;
    OwnPtr<CSPDirective> m_objectSrc;
    OwnPtr<CSPDirective> m_frameSrc;
    OwnPtr<CSPDirective> m_imgSrc;
    OwnPtr<CSPDirective> m_styleSrc;
    OwnPtr<CSPDirective> m_fontSrc;
    OwnPtr<CSPDirective> m_mediaSrc;
    OwnPtr<CSPDirective> m_connectSrc;

    Vector<KURL> m_reportURIs;
};

CSPDirectiveList::CSPDirectiveList(ScriptExecutionContext* scriptExecutionContext)
    : m_scriptExecutionContext(scriptExecutionContext)
    , m_reportOnly(false)
    , m_haveSandboxPolicy(false)
{
}

PassOwnPtr<CSPDirectiveList> CSPDirectiveList::create(ScriptExecutionContext* scriptExecutionContext, const String& header, ContentSecurityPolicy::HeaderType type)
{
    OwnPtr<CSPDirectiveList> policy = adoptPtr(new CSPDirectiveList(scriptExecutionContext));
    policy->parse(header);
    policy->m_header = header;

    switch (type) {
    case ContentSecurityPolicy::ReportOnly:
        policy->m_reportOnly = true;
        return policy.release();
    case ContentSecurityPolicy::EnforcePolicy:
        ASSERT(!policy->m_reportOnly);
        break;
    }

    if (!policy->checkEval(policy->operativeDirective(policy->m_scriptSrc.get())))
        scriptExecutionContext->disableEval();

    return policy.release();
}

void CSPDirectiveList::reportViolation(const String& directiveText, const String& consoleMessage, const KURL& blockedURL, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    String message = m_reportOnly ? "[Report Only] " + consoleMessage : consoleMessage;
    m_scriptExecutionContext->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message, contextURL, contextLine.oneBasedInt(), callStack);

    if (m_reportURIs.isEmpty())
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
    cspReport->setString("original-policy", m_header);
    if (blockedURL.isValid())
        cspReport->setString("blocked-uri", document->securityOrigin()->canRequest(blockedURL) ? blockedURL.strippedForUseAsReferrer() : SecurityOrigin::create(blockedURL)->toString());

    RefPtr<InspectorObject> reportObject = InspectorObject::create();
    reportObject->setObject("csp-report", cspReport.release());

    RefPtr<FormData> report = FormData::create(reportObject->toJSONString().utf8());

    for (size_t i = 0; i < m_reportURIs.size(); ++i)
        PingLoader::reportContentSecurityPolicyViolation(frame, m_reportURIs[i], report);
}

void CSPDirectiveList::logUnrecognizedDirective(const String& name) const
{
    String message = makeString("Unrecognized Content-Security-Policy directive '", name, "'.\n");
    m_scriptExecutionContext->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message);
}

void CSPDirectiveList::logDuplicateDirective(const String& name) const
{
    String message = makeString("Ignoring duplicate Content-Security-Policy directive '", name, "'.\n");
    m_scriptExecutionContext->addConsoleMessage(JSMessageSource, LogMessageType, ErrorMessageLevel, message);
}

bool CSPDirectiveList::checkEval(CSPDirective* directive) const
{
    return !directive || directive->allowEval();
}

CSPDirective* CSPDirectiveList::operativeDirective(CSPDirective* directive) const
{
    return directive ? directive : m_defaultSrc.get();
}

bool CSPDirectiveList::checkInlineAndReportViolation(CSPDirective* directive, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    if (!directive || directive->allowInline())
        return true;
    reportViolation(directive->text(), consoleMessage + "\"" + directive->text() + "\".\n", KURL(), contextURL, contextLine);
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::checkEvalAndReportViolation(CSPDirective* directive, const String& consoleMessage, const String& contextURL, const WTF::OrdinalNumber& contextLine, PassRefPtr<ScriptCallStack> callStack) const
{
    if (checkEval(directive))
        return true;
    reportViolation(directive->text(), consoleMessage + "\"" + directive->text() + "\".\n", KURL(), contextURL, contextLine, callStack);
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::checkSourceAndReportViolation(CSPDirective* directive, const KURL& url, const String& type) const
{
    if (!directive || directive->allows(url))
        return true;
    String verb = type == "connect" ? "connect to" : "load the";
    reportViolation(directive->text(), "Refused to " + verb + " " + type + " '" + url.string() + "' because it violates the following Content Security Policy directive: \"" + directive->text() + "\".\n", url);
    return denyIfEnforcingPolicy();
}

bool CSPDirectiveList::allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to execute JavaScript URL because it violates the following Content Security Policy directive: "));
    return checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine);
}

bool CSPDirectiveList::allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to execute inline event handler because it violates the following Content Security Policy directive: "));
    return checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine);
}

bool CSPDirectiveList::allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to execute inline script because it violates the following Content Security Policy directive: "));
    return checkInlineAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, contextURL, contextLine);
}

bool CSPDirectiveList::allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to apply inline style because it violates the following Content Security Policy directive: "));
    return checkInlineAndReportViolation(operativeDirective(m_styleSrc.get()), consoleMessage, contextURL, contextLine);
}

bool CSPDirectiveList::allowEval(PassRefPtr<ScriptCallStack> callStack) const
{
    DEFINE_STATIC_LOCAL(String, consoleMessage, ("Refused to evaluate script because it violates the following Content Security Policy directive: "));
    return checkEvalAndReportViolation(operativeDirective(m_scriptSrc.get()), consoleMessage, String(), WTF::OrdinalNumber::beforeFirst(), callStack);
}

bool CSPDirectiveList::allowScriptFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("script"));
    return checkSourceAndReportViolation(operativeDirective(m_scriptSrc.get()), url, type);
}

bool CSPDirectiveList::allowObjectFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("object"));
    if (url.isBlankURL())
        return true;
    return checkSourceAndReportViolation(operativeDirective(m_objectSrc.get()), url, type);
}

bool CSPDirectiveList::allowChildFrameFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("frame"));
    if (url.isBlankURL())
        return true;
    return checkSourceAndReportViolation(operativeDirective(m_frameSrc.get()), url, type);
}

bool CSPDirectiveList::allowImageFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("image"));
    return checkSourceAndReportViolation(operativeDirective(m_imgSrc.get()), url, type);
}

bool CSPDirectiveList::allowStyleFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("style"));
    return checkSourceAndReportViolation(operativeDirective(m_styleSrc.get()), url, type);
}

bool CSPDirectiveList::allowFontFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("font"));
    return checkSourceAndReportViolation(operativeDirective(m_fontSrc.get()), url, type);
}

bool CSPDirectiveList::allowMediaFromSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("media"));
    return checkSourceAndReportViolation(operativeDirective(m_mediaSrc.get()), url, type);
}

bool CSPDirectiveList::allowConnectToSource(const KURL& url) const
{
    DEFINE_STATIC_LOCAL(String, type, ("connect"));
    return checkSourceAndReportViolation(operativeDirective(m_connectSrc.get()), url, type);
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

    const UChar* nameBegin = position;
    skipWhile<isDirectiveNameCharacter>(position, end);

    // The directive-name must be non-empty.
    if (nameBegin == position)
        return false;

    name = String(nameBegin, position - nameBegin);

    if (position == end)
        return true;

    if (!skipExactly<isASCIISpace>(position, end))
        return false;

    skipWhile<isASCIISpace>(position, end);

    const UChar* valueBegin = position;
    skipWhile<isDirectiveValueCharacter>(position, end);

    if (position != end)
        return false;

    // The directive-value may be empty.
    if (valueBegin == position)
        return true;

    value = String(valueBegin, position - valueBegin);
    return true;
}

void CSPDirectiveList::parseReportURI(const String& name, const String& value)
{
    if (!m_reportURIs.isEmpty()) {
        logDuplicateDirective(name);
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
            m_reportURIs.append(m_scriptExecutionContext->completeURL(url));
        }
    }
}

void CSPDirectiveList::setCSPDirective(const String& name, const String& value, OwnPtr<CSPDirective>& directive)
{
    if (directive) {
        logDuplicateDirective(name);
        return;
    }
    directive = adoptPtr(new CSPDirective(name, value, m_scriptExecutionContext));
}

void CSPDirectiveList::applySandboxPolicy(const String& name, const String& sandboxPolicy)
{
    if (m_haveSandboxPolicy) {
        logDuplicateDirective(name);
        return;
    }
    m_haveSandboxPolicy = true;
    m_scriptExecutionContext->enforceSandboxFlags(SecurityContext::parseSandboxPolicy(sandboxPolicy));
}

void CSPDirectiveList::addDirective(const String& name, const String& value)
{
    DEFINE_STATIC_LOCAL(String, defaultSrc, ("default-src"));
    DEFINE_STATIC_LOCAL(String, scriptSrc, ("script-src"));
    DEFINE_STATIC_LOCAL(String, objectSrc, ("object-src"));
    DEFINE_STATIC_LOCAL(String, frameSrc, ("frame-src"));
    DEFINE_STATIC_LOCAL(String, imgSrc, ("img-src"));
    DEFINE_STATIC_LOCAL(String, styleSrc, ("style-src"));
    DEFINE_STATIC_LOCAL(String, fontSrc, ("font-src"));
    DEFINE_STATIC_LOCAL(String, mediaSrc, ("media-src"));
    DEFINE_STATIC_LOCAL(String, connectSrc, ("connect-src"));
    DEFINE_STATIC_LOCAL(String, sandbox, ("sandbox"));
    DEFINE_STATIC_LOCAL(String, reportURI, ("report-uri"));

    ASSERT(!name.isEmpty());

    if (equalIgnoringCase(name, defaultSrc))
        setCSPDirective(name, value, m_defaultSrc);
    else if (equalIgnoringCase(name, scriptSrc))
        setCSPDirective(name, value, m_scriptSrc);
    else if (equalIgnoringCase(name, objectSrc))
        setCSPDirective(name, value, m_objectSrc);
    else if (equalIgnoringCase(name, frameSrc))
        setCSPDirective(name, value, m_frameSrc);
    else if (equalIgnoringCase(name, imgSrc))
        setCSPDirective(name, value, m_imgSrc);
    else if (equalIgnoringCase(name, styleSrc))
        setCSPDirective(name, value, m_styleSrc);
    else if (equalIgnoringCase(name, fontSrc))
        setCSPDirective(name, value, m_fontSrc);
    else if (equalIgnoringCase(name, mediaSrc))
        setCSPDirective(name, value, m_mediaSrc);
    else if (equalIgnoringCase(name, connectSrc))
        setCSPDirective(name, value, m_connectSrc);
    else if (equalIgnoringCase(name, sandbox))
        applySandboxPolicy(name, value);
    else if (equalIgnoringCase(name, reportURI))
        parseReportURI(name, value);
    else
        logUnrecognizedDirective(name);
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
    m_policies.append(CSPDirectiveList::create(m_scriptExecutionContext, header, type));
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

template<bool (CSPDirectiveList::*allowed)(PassRefPtr<ScriptCallStack>) const>
bool isAllowedByAllWithCallStack(const CSPDirectiveListVector& policies, PassRefPtr<ScriptCallStack> callStack)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowed)(callStack))
            return false;
    }
    return true;
}

template<bool (CSPDirectiveList::*allowed)(const String&, const WTF::OrdinalNumber&) const>
bool isAllowedByAllWithContext(const CSPDirectiveListVector& policies, const String& contextURL, const WTF::OrdinalNumber& contextLine)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowed)(contextURL, contextLine))
            return false;
    }
    return true;
}

template<bool (CSPDirectiveList::*allowFromURL)(const KURL&) const>
bool isAllowedByAllWithURL(const CSPDirectiveListVector& policies, const KURL& url)
{
    for (size_t i = 0; i < policies.size(); ++i) {
        if (!(policies[i].get()->*allowFromURL)(url))
            return false;
    }
    return true;
}

bool ContentSecurityPolicy::allowJavaScriptURLs(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowJavaScriptURLs>(m_policies, contextURL, contextLine);
}

bool ContentSecurityPolicy::allowInlineEventHandlers(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineEventHandlers>(m_policies, contextURL, contextLine);
}

bool ContentSecurityPolicy::allowInlineScript(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineScript>(m_policies, contextURL, contextLine);
}

bool ContentSecurityPolicy::allowInlineStyle(const String& contextURL, const WTF::OrdinalNumber& contextLine) const
{
    if (m_overrideInlineStyleAllowed)
        return true;
    return isAllowedByAllWithContext<&CSPDirectiveList::allowInlineStyle>(m_policies, contextURL, contextLine);
}

bool ContentSecurityPolicy::allowEval(PassRefPtr<ScriptCallStack> callStack) const
{
    return isAllowedByAllWithCallStack<&CSPDirectiveList::allowEval>(m_policies, callStack);
}

bool ContentSecurityPolicy::allowScriptFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowScriptFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowObjectFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowObjectFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowChildFrameFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowChildFrameFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowImageFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowImageFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowStyleFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowStyleFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowFontFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowFontFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowMediaFromSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowMediaFromSource>(m_policies, url);
}

bool ContentSecurityPolicy::allowConnectToSource(const KURL& url) const
{
    return isAllowedByAllWithURL<&CSPDirectiveList::allowConnectToSource>(m_policies, url);
}

}
