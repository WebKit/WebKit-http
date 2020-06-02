/*
 * Copyright (C) 2018-2019 Apple Inc. All rights reserved.
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

#import "config.h"
#import "ApplePaySetupFeatureWebCore.h"

#if ENABLE(APPLE_PAY_SETUP)

#import "ApplePaySetupFeatureTypeWebCore.h"
#import <pal/spi/cocoa/PassKitSPI.h>

namespace WebCore {

ApplePaySetupFeatureType ApplePaySetupFeature::type() const
{
    switch ([m_feature type]) {
    case PKPaymentSetupFeatureTypeApplePay:
        return ApplePaySetupFeatureType::ApplePay;
        ALLOW_DEPRECATED_DECLARATIONS_BEGIN
    case PKPaymentSetupFeatureTypeApplePay_X:
        ALLOW_DEPRECATED_DECLARATIONS_END
        return ApplePaySetupFeatureType::AppleCard;
    }
}

ApplePaySetupFeature::State ApplePaySetupFeature::state() const
{
    switch ([m_feature state]) {
    case PKPaymentSetupFeatureStateUnsupported:
        return State::Unsupported;
    case PKPaymentSetupFeatureStateSupported:
        return State::Supported;
    case PKPaymentSetupFeatureStateSupplementarySupported:
        return State::SupplementarySupported;
    case PKPaymentSetupFeatureStateCompleted:
        return State::Completed;
    }
}

#if ENABLE(APPLE_PAY_INSTALLMENTS)
bool ApplePaySetupFeature::supportsInstallments() const
{
    return [m_feature supportedOptions] & PKPaymentSetupFeatureSupportedOptionsInstallments;
}
#endif

ApplePaySetupFeature::ApplePaySetupFeature(PKPaymentSetupFeature *feature)
    : m_feature { feature }
{
}

} // namespace WebCore

#endif // ENABLE(APPLE_PAY_SETUP)
