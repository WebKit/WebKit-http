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

#include "config.h"

#if ENABLE(MEDIA_STREAM)

#include "WebUserMediaRequest.h"

#include "Document.h"
#include "MediaConstraints.h"
#include "MediaStreamDescriptor.h"
#include "MediaStreamSource.h"
#include "SecurityOrigin.h"
#include "UserMediaRequest.h"
#include "WebDocument.h"
#include "WebSecurityOrigin.h"
#include <public/WebMediaConstraints.h>
#include <public/WebMediaStreamDescriptor.h>
#include <public/WebMediaStreamSource.h>
#include <public/WebString.h>
#include <public/WebVector.h>
#include <wtf/Vector.h>

using namespace WebCore;

namespace WebKit {

WebUserMediaRequest::WebUserMediaRequest(const PassRefPtr<UserMediaRequest>& request)
    : m_private(request)
{
}

void WebUserMediaRequest::reset()
{
    m_private.reset();
}

bool WebUserMediaRequest::audio() const
{
    ASSERT(!isNull());
    return m_private->audio();
}

bool WebUserMediaRequest::video() const
{
    ASSERT(!isNull());
    return m_private->video();
}

WebMediaConstraints WebUserMediaRequest::audioConstraints() const
{
    ASSERT(!isNull());
    return m_private->audioConstraints();
}

WebMediaConstraints WebUserMediaRequest::videoConstraints() const
{
    ASSERT(!isNull());
    return m_private->videoConstraints();
}

WebSecurityOrigin WebUserMediaRequest::securityOrigin() const
{
    ASSERT(!isNull() && m_private->scriptExecutionContext());
    return WebSecurityOrigin(m_private->scriptExecutionContext()->securityOrigin());
}

WebDocument WebUserMediaRequest::ownerDocument() const
{
    ASSERT(!isNull());
    return WebDocument(m_private->ownerDocument());
}

void WebUserMediaRequest::requestSucceeded(const WebMediaStreamDescriptor& streamDescriptor)
{
    ASSERT(!isNull() && !streamDescriptor.isNull());
    m_private->succeed(streamDescriptor);
}

void WebUserMediaRequest::requestFailed()
{
    ASSERT(!isNull());
    m_private->fail();
}

bool WebUserMediaRequest::equals(const WebUserMediaRequest& other) const
{
    if (isNull() || other.isNull())
        return false;
    return m_private.get() == other.m_private.get();
}

void WebUserMediaRequest::assign(const WebUserMediaRequest& other)
{
    m_private = other.m_private;
}

WebUserMediaRequest::operator UserMediaRequest*() const
{
    return m_private.get();
}

} // namespace WebKit

#endif // ENABLE(MEDIA_STREAM)

