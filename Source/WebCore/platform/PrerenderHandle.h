/*
 * Copyright (C) 2012 Google Inc. All rights reserved.
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
 *
 */

#ifndef PrerenderHandle_h
#define PrerenderHandle_h

#if ENABLE(LINK_PRERENDER)

#include "ReferrerPolicy.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/RefPtr.h>
#include <wtf/text/WTFString.h>

namespace WebCore {

class KURL;
class Prerender;

class PrerenderHandle : public RefCounted<PrerenderHandle> {
    WTF_MAKE_NONCOPYABLE(PrerenderHandle);
public:
    static PassRefPtr<PrerenderHandle> create(const KURL&, const String& referrer, ReferrerPolicy);
    ~PrerenderHandle();

    Prerender* prerender();

    // FIXME: one day there will be events here, and we will be a PrerenderClient.

    // A prerender link element is added when it is inserted into a document.
    void add();

    // A prerender is abandoned when it's navigated away from. This is is a weaker signal
    // than cancel(), since the launcher hasn't indicated that the prerender isn't wanted,
    // and we may end up using it after, for instance, a short redirect chain.
    void abandon();

    // A prerender is canceled when it is removed from a document.
    void cancel();

    // A prerender is suspended along with the DOM containing its linkloader & prerenderer.
    void suspend();
    void resume();

    const KURL& url() const;
    const String& referrer() const;
    ReferrerPolicy referrerPolicy() const;

private:
    PrerenderHandle(const KURL&, const String& referrer, ReferrerPolicy);
    RefPtr<Prerender> m_prerender;
};

} // namespace WebCore

#endif // ENABLE(LINK_PRERENDER)

#endif // PrerenderHandle_h
