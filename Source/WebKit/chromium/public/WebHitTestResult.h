/*
* Copyright (C) 2012 Google Inc. All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions
* are met:
*
* 1.  Redistributions of source code must retain the above copyright
*     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef WebHitTestResult_h
#define WebHitTestResult_h

#include "platform/WebPrivateOwnPtr.h"

namespace WebCore {
class HitTestResult;
}

namespace WebKit {

class WebNode;
struct WebPoint;

// Properties of a hit test result, i.e. properties of the nodes at a given point
// (the hit point) on the page. Both urls may be populated at the same time, for
// example in the instance of an <img> inside an <a>.
class WebHitTestResult {
public:
    WebHitTestResult() { }
    WebHitTestResult(const WebHitTestResult& info) { assign(info); }
    ~WebHitTestResult() { reset(); }

    WEBKIT_EXPORT void assign(const WebHitTestResult&);
    WEBKIT_EXPORT void reset();
    WEBKIT_EXPORT bool isNull() const;

    // The node that was hit (only one for point-based tests).
    WEBKIT_EXPORT WebNode node() const;

    // Coordinates of the point that was hit. Relative to the node.
    WEBKIT_EXPORT WebPoint localPoint() const;

#if WEBKIT_IMPLEMENTATION
    WebHitTestResult(const WebCore::HitTestResult&);
    WebHitTestResult& operator=(const WebCore::HitTestResult&);
    operator WebCore::HitTestResult() const;
#endif

protected:
    WebPrivateOwnPtr<WebCore::HitTestResult> m_private;
};

} // namespace WebKit

#endif
