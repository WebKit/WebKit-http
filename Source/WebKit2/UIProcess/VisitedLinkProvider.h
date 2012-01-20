/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef VisitedLinkProvider_h
#define VisitedLinkProvider_h

#include "VisitedLinkTable.h"
#include <WebCore/LinkHash.h>
#include <WebCore/RunLoop.h>
#include <wtf/Forward.h>
#include <wtf/HashSet.h>

namespace WebKit {

class WebContext;
    
class VisitedLinkProvider {
    WTF_MAKE_NONCOPYABLE(VisitedLinkProvider);
public:
    explicit VisitedLinkProvider(WebContext*);

    void addVisitedLink(WebCore::LinkHash);

    void processDidFinishLaunching();
    void processDidClose();

private:
    void pendingVisitedLinksTimerFired();

    WebContext* m_context;
    bool m_visitedLinksPopulated;
    bool m_webProcessHasVisitedLinkState;

    unsigned m_keyCount;
    unsigned m_tableSize;
    VisitedLinkTable m_table;

    HashSet<WebCore::LinkHash, WebCore::LinkHashHash> m_pendingVisitedLinks;
    WebCore::RunLoop::Timer<VisitedLinkProvider> m_pendingVisitedLinksTimer;
};

} // namespace WebKit

#endif // VisitedLinkProvider_h
