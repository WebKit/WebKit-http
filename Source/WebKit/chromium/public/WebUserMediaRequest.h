/*
 * Copyright (C) 2011 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebUserMediaRequest_h
#define WebUserMediaRequest_h

#include "WebSecurityOrigin.h"
#include "platform/WebCommon.h"
#include "platform/WebPrivatePtr.h"

namespace WebCore {
class UserMediaRequest;
}

namespace WebKit {

class WebMediaStreamDescriptor;
class WebMediaStreamSource;
class WebString;
template <typename T> class WebVector;

class WebUserMediaRequest {
public:
    WebUserMediaRequest() { }
    WebUserMediaRequest(const WebUserMediaRequest& request) { assign(request); }
    ~WebUserMediaRequest() { reset(); }

    WebUserMediaRequest& operator=(const WebUserMediaRequest& other)
    {
        assign(other);
        return *this;
    }

    WEBKIT_EXPORT void reset();
    bool isNull() const { return m_private.isNull(); }
    WEBKIT_EXPORT bool equals(const WebUserMediaRequest&) const;
    WEBKIT_EXPORT void assign(const WebUserMediaRequest&);

    WEBKIT_EXPORT bool audio() const;
    WEBKIT_EXPORT bool video() const;

    WEBKIT_EXPORT WebSecurityOrigin securityOrigin() const;

    // DEPRECATED
    WEBKIT_EXPORT void requestSucceeded(const WebVector<WebMediaStreamSource>& audioSources, const WebVector<WebMediaStreamSource>& videoSources);

    WEBKIT_EXPORT void requestSucceeded(const WebMediaStreamDescriptor&);

    WEBKIT_EXPORT void requestFailed();

#if WEBKIT_IMPLEMENTATION
    WebUserMediaRequest(const PassRefPtr<WebCore::UserMediaRequest>&);
    operator WebCore::UserMediaRequest*() const;
#endif

private:
    WebPrivatePtr<WebCore::UserMediaRequest> m_private;
};

inline bool operator==(const WebUserMediaRequest& a, const WebUserMediaRequest& b)
{
    return a.equals(b);
}

} // namespace WebKit

#endif // WebUserMediaRequest_h
