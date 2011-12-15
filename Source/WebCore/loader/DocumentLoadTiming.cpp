/*
 * Copyright (C) 2011 Google, Inc. All Rights Reserved.
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
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL GOOGLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DocumentLoadTiming.h"

#include "Document.h"
#include "DocumentLoader.h"
#include "Frame.h"
#include "Page.h"
#include "SecurityOrigin.h"
#include <wtf/CurrentTime.h>
#include <wtf/RefPtr.h>

namespace WebCore {

DocumentLoadTiming::DocumentLoadTiming()
    : m_referenceMonotonicTime(0.0)
    , m_referenceWallTime(0.0)
    , m_navigationStart(0.0)
    , m_unloadEventStart(0.0)
    , m_unloadEventEnd(0.0)
    , m_redirectStart(0.0)
    , m_redirectEnd(0.0)
    , m_redirectCount(0)
    , m_fetchStart(0.0)
    , m_responseEnd(0.0)
    , m_loadEventStart(0.0)
    , m_loadEventEnd(0.0)
    , m_hasCrossOriginRedirect(false)
    , m_hasSameOriginAsPreviousDocument(false)
{
}

void DocumentLoadTiming::markNavigationStart(Frame* frame)
{
    ASSERT(frame);
    ASSERT(!m_navigationStart && !m_referenceMonotonicTime && !m_referenceWallTime);

    if (frame->page()->mainFrame() == frame) {
        m_navigationStart = m_referenceMonotonicTime = monotonicallyIncreasingTime();
        m_referenceWallTime = currentTime();
    } else {
        Document* rootDocument = frame->page()->mainFrame()->document();
        ASSERT(rootDocument);
        DocumentLoadTiming* rootTiming = rootDocument->loader()->timing();
        m_referenceMonotonicTime = rootTiming->m_referenceMonotonicTime;
        m_referenceWallTime = rootTiming->m_referenceWallTime;
        m_navigationStart = monotonicallyIncreasingTime();
    }
}

void DocumentLoadTiming::addRedirect(const KURL& redirectingUrl, const KURL& redirectedUrl)
{
    m_redirectCount++;
    if (!m_redirectStart)
        m_redirectStart = m_fetchStart;
    m_redirectEnd = m_fetchStart = monotonicallyIncreasingTime();
    // Check if the redirected url is allowed to access the redirecting url's timing information.
    RefPtr<SecurityOrigin> redirectedSecurityOrigin = SecurityOrigin::create(redirectedUrl);
    m_hasCrossOriginRedirect = !redirectedSecurityOrigin->canRequest(redirectingUrl);
}

double DocumentLoadTiming::convertMonotonicTimeToDocumentTime(double monotonicTime) const
{
    if (!monotonicTime)
        return 0.0;
    return m_referenceWallTime + monotonicTime - m_referenceMonotonicTime;
}

} // namespace WebCore
