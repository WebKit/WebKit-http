/*
 * Copyright (C) 2006, 2008 Apple Inc. All rights reserved.
 * Copyright (C) 2009 Google Inc. All rights reserved.
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

#ifndef ResourceResponseBase_h
#define ResourceResponseBase_h

#include "HTTPHeaderMap.h"
#include "URL.h"
#include "ResourceLoadTiming.h"

#include <wtf/PassOwnPtr.h>
#include <wtf/RefPtr.h>

#if OS(SOLARIS)
#include <sys/time.h> // For time_t structure.
#endif

namespace WebCore {

class ResourceResponse;
struct CrossThreadResourceResponseData;

// Do not use this class directly, use the class ResponseResponse instead
class ResourceResponseBase {
    WTF_MAKE_FAST_ALLOCATED;
public:
    static PassOwnPtr<ResourceResponse> adopt(PassOwnPtr<CrossThreadResourceResponseData>);

    // Gets a copy of the data suitable for passing to another thread.
    PassOwnPtr<CrossThreadResourceResponseData> copyData() const;

    bool isNull() const { return m_isNull; }
    bool isHTTP() const;

    const URL& url() const;
    void setURL(const URL& url);

    const String& mimeType() const;
    void setMimeType(const String& mimeType);

    long long expectedContentLength() const;
    void setExpectedContentLength(long long expectedContentLength);

    const String& textEncodingName() const;
    void setTextEncodingName(const String& name);

    // FIXME: Should compute this on the fly.
    // There should not be a setter exposed, as suggested file name is determined based on other headers in a manner that WebCore does not necessarily know about.
    const String& suggestedFilename() const;
    void setSuggestedFilename(const String&);

    int httpStatusCode() const;
    void setHTTPStatusCode(int);
    
    const String& httpStatusText() const;
    void setHTTPStatusText(const String&);

    const HTTPHeaderMap& httpHeaderFields() const;

    String httpHeaderField(const String& name) const;
    String httpHeaderField(HTTPHeaderName) const;
    void setHTTPHeaderField(const String& name, const String& value);
    void setHTTPHeaderField(HTTPHeaderName, const String& value);

    void addHTTPHeaderField(const String& name, const String& value);

    // Instead of passing a string literal to any of these functions, just use a HTTPHeaderName instead.
    template<size_t length> String httpHeaderField(const char (&)[length]) const = delete;
    template<size_t length> void setHTTPHeaderField(const char (&)[length], const String&) = delete;
    template<size_t length> void addHTTPHeaderField(const char (&)[length], const String&) = delete;

    bool isMultipart() const { return mimeType() == "multipart/x-mixed-replace"; }

    bool isAttachment() const;
    
    // These functions return parsed values of the corresponding response headers.
    // NaN means that the header was not present or had invalid value.
    bool cacheControlContainsNoCache() const;
    bool cacheControlContainsNoStore() const;
    bool cacheControlContainsMustRevalidate() const;
    bool hasCacheValidatorFields() const;
    double cacheControlMaxAge() const;
    double date() const;
    double age() const;
    double expires() const;
    double lastModified() const;

    unsigned connectionID() const;
    void setConnectionID(unsigned);

    bool connectionReused() const;
    void setConnectionReused(bool);

    bool wasCached() const;
    void setWasCached(bool);

    ResourceLoadTiming& resourceLoadTiming() const { return m_resourceLoadTiming; }

    // The ResourceResponse subclass may "shadow" this method to provide platform-specific memory usage information
    unsigned memoryUsage() const
    {
        // average size, mostly due to URL and Header Map strings
        return 1280;
    }

    static bool compare(const ResourceResponse&, const ResourceResponse&);

protected:
    enum InitLevel {
        Uninitialized,
        CommonFieldsOnly,
        CommonAndUncommonFields,
        AllFields
    };

    ResourceResponseBase();
    ResourceResponseBase(const URL& url, const String& mimeType, long long expectedLength, const String& textEncodingName, const String& filename);

    void lazyInit(InitLevel) const;

    // The ResourceResponse subclass may "shadow" this method to lazily initialize platform specific fields
    void platformLazyInit(InitLevel) { }

    // The ResourceResponse subclass may "shadow" this method to compare platform specific fields
    static bool platformCompare(const ResourceResponse&, const ResourceResponse&) { return true; }

    URL m_url;
    AtomicString m_mimeType;
    long long m_expectedContentLength;
    AtomicString m_textEncodingName;
    String m_suggestedFilename;
    AtomicString m_httpStatusText;
    HTTPHeaderMap m_httpHeaderFields;
    mutable ResourceLoadTiming m_resourceLoadTiming;

    int m_httpStatusCode;
    unsigned m_connectionID;

private:
    mutable double m_cacheControlMaxAge;
    mutable double m_age;
    mutable double m_date;
    mutable double m_expires;
    mutable double m_lastModified;

public:
    bool m_wasCached : 1;
    bool m_connectionReused : 1;

    bool m_isNull : 1;
    
private:
    const ResourceResponse& asResourceResponse() const;
    void parseCacheControlDirectives() const;
    void updateHeaderParsedState(HTTPHeaderName);

    mutable bool m_haveParsedCacheControlHeader : 1;
    mutable bool m_haveParsedAgeHeader : 1;
    mutable bool m_haveParsedDateHeader : 1;
    mutable bool m_haveParsedExpiresHeader : 1;
    mutable bool m_haveParsedLastModifiedHeader : 1;

    mutable bool m_cacheControlContainsNoCache : 1;
    mutable bool m_cacheControlContainsNoStore : 1;
    mutable bool m_cacheControlContainsMustRevalidate : 1;
};

inline bool operator==(const ResourceResponse& a, const ResourceResponse& b) { return ResourceResponseBase::compare(a, b); }
inline bool operator!=(const ResourceResponse& a, const ResourceResponse& b) { return !(a == b); }

struct CrossThreadResourceResponseDataBase {
    WTF_MAKE_NONCOPYABLE(CrossThreadResourceResponseDataBase); WTF_MAKE_FAST_ALLOCATED;
public:
    CrossThreadResourceResponseDataBase() { }
    URL m_url;
    String m_mimeType;
    long long m_expectedContentLength;
    String m_textEncodingName;
    String m_suggestedFilename;
    int m_httpStatusCode;
    String m_httpStatusText;
    std::unique_ptr<CrossThreadHTTPHeaderMapData> m_httpHeaders;
    ResourceLoadTiming m_resourceLoadTiming;
};

} // namespace WebCore

#endif // ResourceResponseBase_h
