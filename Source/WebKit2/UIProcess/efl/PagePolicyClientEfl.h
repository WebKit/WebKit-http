/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

#ifndef PagePolicyClientEfl_h
#define PagePolicyClientEfl_h

#include "WKEvent.h"
#include "WKPageLoadTypes.h"
#include <WebKit2/WKBase.h>
#include <wtf/PassOwnPtr.h>

class EwkViewImpl;

namespace WebKit {

class PagePolicyClientEfl {
public:
    static PassOwnPtr<PagePolicyClientEfl> create(EwkViewImpl* viewImpl)
    {
        return adoptPtr(new PagePolicyClientEfl(viewImpl));
    }

private:
    explicit PagePolicyClientEfl(EwkViewImpl*);

    static void decidePolicyForNavigationAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    static void decidePolicyForNewWindowAction(WKPageRef, WKFrameRef, WKFrameNavigationType, WKEventModifiers, WKEventMouseButton, WKURLRequestRef, WKStringRef, WKFramePolicyListenerRef, WKTypeRef, const void*);
    static void decidePolicyForResponseCallback(WKPageRef, WKFrameRef, WKURLResponseRef, WKURLRequestRef, WKFramePolicyListenerRef, WKTypeRef, const void*);

    EwkViewImpl* m_viewImpl;
};

} // namespace WebKit

#endif // PagePolicyClientEfl_h
