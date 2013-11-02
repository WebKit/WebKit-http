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

#ifndef WebFormSubmissionListenerProxy_h
#define WebFormSubmissionListenerProxy_h

#include "WebFrameListenerProxy.h"

#if PLATFORM(MAC)
#include "WKFoundation.h"
#endif

#define DELEGATE_REF_COUNTING_TO_COCOA (PLATFORM(MAC) && WK_API_ENABLED)

namespace WebKit {

class WebFrameProxy;

class WebFormSubmissionListenerProxy : public WebFrameListenerProxy {
public:
    static const Type APIType = TypeFormSubmissionListener;

    static PassRefPtr<WebFormSubmissionListenerProxy> create(WebFrameProxy* frame, uint64_t listenerID)
    {
        return adoptRef(new WebFormSubmissionListenerProxy(frame, listenerID));
    }

    void continueSubmission();

private:
    WebFormSubmissionListenerProxy(WebFrameProxy*, uint64_t listenerID);

    virtual Type type() const { return APIType; }

#if DELEGATE_REF_COUNTING_TO_COCOA
    void* operator new(size_t size) { return newObject(size, APIType); }
#endif
};

} // namespace WebKit

#undef DELEGATE_REF_COUNTING_TO_COCOA

#endif // WebFramePolicyListenerProxy_h
