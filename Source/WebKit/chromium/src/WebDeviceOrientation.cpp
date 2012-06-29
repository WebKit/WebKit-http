/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
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

#include "config.h"
#include "WebDeviceOrientation.h"

#include "DeviceOrientationData.h"
#include <wtf/PassRefPtr.h>

namespace WebKit {

WebDeviceOrientation::WebDeviceOrientation(const WebCore::DeviceOrientationData* orientation)
{
    if (!orientation) {
        m_isNull = true;
        m_canProvideAlpha = false;
        m_alpha = 0;
        m_canProvideBeta = false;
        m_beta = 0;
        m_canProvideGamma = false;
        m_gamma = 0;
        m_canProvideAbsolute = false;
        m_absolute = false;
        return;
    }

    m_isNull = false;
    m_canProvideAlpha = orientation->canProvideAlpha();
    m_alpha = orientation->alpha();
    m_canProvideBeta = orientation->canProvideBeta();
    m_beta = orientation->beta();
    m_canProvideGamma = orientation->canProvideGamma();
    m_gamma = orientation->gamma();
    m_canProvideAbsolute = orientation->canProvideAbsolute();
    m_absolute = orientation->absolute();
}

WebDeviceOrientation::operator PassRefPtr<WebCore::DeviceOrientationData>() const
{
    if (m_isNull)
        return 0;
    return WebCore::DeviceOrientationData::create(m_canProvideAlpha, m_alpha, m_canProvideBeta, m_beta, m_canProvideGamma, m_gamma, m_canProvideAbsolute, m_absolute);
}

} // namespace WebKit
