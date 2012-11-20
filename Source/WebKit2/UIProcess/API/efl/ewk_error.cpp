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

#include "config.h"
#include "ewk_error.h"

#include "ErrorsEfl.h"
#include "WKString.h"
#include "WKURL.h"
#include "ewk_error_private.h"
#include <WKAPICast.h>
#include <wtf/text/CString.h>

using namespace WebCore;
using namespace WebKit;

EwkError::EwkError(WKErrorRef errorRef)
    : m_wkError(errorRef)
    , m_url(AdoptWK, WKErrorCopyFailingURL(errorRef))
    , m_description(AdoptWK, WKErrorCopyLocalizedDescription(errorRef))
{ }

const char* EwkError::url() const
{
    return m_url;
}

const char* EwkError::description() const
{
    return m_description;
}

String EwkError::domain() const
{
    WKRetainPtr<WKStringRef> wkDomain(AdoptWK, WKErrorCopyDomain(m_wkError.get()));
    return toWTFString(wkDomain.get());
}

int EwkError::errorCode() const
{
    return WKErrorGetErrorCode(m_wkError.get());
}

bool EwkError::isCancellation() const
{
    return toImpl(m_wkError.get())->platformError().isCancellation();
}

Ewk_Error_Type ewk_error_type_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, EWK_ERROR_TYPE_NONE);

    String errorDomain = error->domain();

    if (errorDomain == errorDomainNetwork)
        return EWK_ERROR_TYPE_NETWORK;
    if (errorDomain == errorDomainPolicy)
        return EWK_ERROR_TYPE_POLICY;
    if (errorDomain == errorDomainPlugin)
        return EWK_ERROR_TYPE_PLUGIN;
    if (errorDomain == errorDomainDownload)
        return EWK_ERROR_TYPE_DOWNLOAD;
    if (errorDomain == errorDomainPrint)
        return EWK_ERROR_TYPE_PRINT;
    return EWK_ERROR_TYPE_INTERNAL;
}

const char* ewk_error_url_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->url();
}

int ewk_error_code_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->errorCode();
}

const char* ewk_error_description_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, 0);

    return error->description();
}

Eina_Bool ewk_error_cancellation_get(const Ewk_Error* error)
{
    EINA_SAFETY_ON_NULL_RETURN_VAL(error, false);

    return error->isCancellation();
}
