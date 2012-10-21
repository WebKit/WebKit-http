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

#ifndef ewk_url_response_private_h
#define ewk_url_response_private_h

#include "WKAPICast.h"
#include "WKEinaSharedString.h"
#include "WKURLResponse.h"
#include <WebCore/ResourceResponse.h>
#include <wtf/PassRefPtr.h>

/**
 * \struct  Ewk_Url_Response
 * @brief   Contains the URL response data.
 */
class Ewk_Url_Response : public RefCounted<Ewk_Url_Response> {
public:
    static PassRefPtr<Ewk_Url_Response> create(WKURLResponseRef wkResponse)
    {
        if (!wkResponse)
            return 0;

        return adoptRef(new Ewk_Url_Response(WebKit::toImpl(wkResponse)->resourceResponse()));
    }

    int httpStatusCode() const;
    const char* url() const;
    const char* mimeType() const;
    unsigned long contentLength() const;

private:
    explicit Ewk_Url_Response(const WebCore::ResourceResponse& coreResponse);

    WebCore::ResourceResponse m_coreResponse;
    WKEinaSharedString m_url;
    WKEinaSharedString m_mimeType;
};

#endif // ewk_url_response_private_h
