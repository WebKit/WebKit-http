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

#ifndef ewk_file_chooser_request_private_h
#define ewk_file_chooser_request_private_h

#include "APIObject.h"
#include "ewk_object_private.h"
#include <wtf/PassRefPtr.h>
#include <wtf/RefPtr.h>
#include <wtf/Vector.h>

namespace WebKit {
class ImmutableArray;
class WebOpenPanelParameters;
class WebOpenPanelResultListenerProxy;
}

class EwkFileChooserRequest : public EwkObject {
public:
    EWK_OBJECT_DECLARE(EwkFileChooserRequest)

    static PassRefPtr<EwkFileChooserRequest> create(WebKit::WebOpenPanelParameters* parameters, WebKit::WebOpenPanelResultListenerProxy* listener)
    {
        return adoptRef(new EwkFileChooserRequest(parameters, listener));
    }

    ~EwkFileChooserRequest();

    bool allowMultipleFiles() const;
    PassRefPtr<WebKit::ImmutableArray> acceptedMIMETypes() const;
    inline bool wasHandled() const { return m_wasRequestHandled; }
    void cancel();
    void chooseFiles(Vector< RefPtr<WebKit::APIObject> >& fileURLs);

private:
    EwkFileChooserRequest(WebKit::WebOpenPanelParameters* parameters, WebKit::WebOpenPanelResultListenerProxy* listener);

    RefPtr<WebKit::WebOpenPanelParameters> m_parameters;
    RefPtr<WebKit::WebOpenPanelResultListenerProxy> m_listener;
    bool m_wasRequestHandled;
};

#endif // ewk_file_chooser_request_private_h
