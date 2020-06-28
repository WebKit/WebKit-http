/*
 * Copyright (C) 2016-2019 Apple Inc. All rights reserved.
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
#import "PaymentAuthorizationPresenter.h"

#if USE(PASSKIT) && ENABLE(APPLE_PAY)

#import "WKPaymentAuthorizationDelegate.h"
#import "WebPaymentCoordinatorProxyCocoa.h"
#import <WebCore/PaymentAuthorizationStatus.h>
#import <WebCore/PaymentMerchantSession.h>
#import <WebCore/PaymentMethodUpdate.h>
#import <WebCore/PaymentSummaryItems.h>
#import <wtf/cocoa/VectorCocoa.h>

#import <pal/cocoa/PassKitSoftLink.h>

SOFT_LINK_FRAMEWORK(Contacts);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressCityKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressCountryKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressISOCountryCodeKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressPostalCodeKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressStateKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressStreetKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressSubAdministrativeAreaKey, NSString *);
SOFT_LINK_CONSTANT(Contacts, CNPostalAddressSubLocalityKey, NSString *);

namespace WebKit {

// FIXME: Rather than having these free functions scattered about, Apple Pay data types should know
// how to convert themselves to and from their platform representations.

static PKPaymentAuthorizationStatus toPKPaymentAuthorizationStatus(WebCore::PaymentAuthorizationStatus status)
{
    switch (status) {
    case WebCore::PaymentAuthorizationStatus::Success:
        return PKPaymentAuthorizationStatusSuccess;
    case WebCore::PaymentAuthorizationStatus::Failure:
        return PKPaymentAuthorizationStatusFailure;
    case WebCore::PaymentAuthorizationStatus::PINRequired:
        return PKPaymentAuthorizationStatusPINRequired;
    case WebCore::PaymentAuthorizationStatus::PINIncorrect:
        return PKPaymentAuthorizationStatusPINIncorrect;
    case WebCore::PaymentAuthorizationStatus::PINLockout:
        return PKPaymentAuthorizationStatusPINLockout;
    }
}

static PKPaymentErrorCode toPKPaymentErrorCode(WebCore::PaymentError::Code code)
{
    switch (code) {
    case WebCore::PaymentError::Code::Unknown:
        return PKPaymentUnknownError;
    case WebCore::PaymentError::Code::ShippingContactInvalid:
        return PKPaymentShippingContactInvalidError;
    case WebCore::PaymentError::Code::BillingContactInvalid:
        return PKPaymentBillingContactInvalidError;
    case WebCore::PaymentError::Code::AddressUnserviceable:
        return PKPaymentShippingAddressUnserviceableError;
    }
}

static NSError *toNSError(const WebCore::PaymentError& error)
{
    auto userInfo = adoptNS([[NSMutableDictionary alloc] init]);
    [userInfo setObject:error.message forKey:NSLocalizedDescriptionKey];

    if (error.contactField) {
        NSString *pkContactField = nil;
        NSString *postalAddressKey = nil;

        switch (*error.contactField) {
        case WebCore::PaymentError::ContactField::PhoneNumber:
            pkContactField = PAL::get_PassKit_PKContactFieldPhoneNumber();
            break;
            
        case WebCore::PaymentError::ContactField::EmailAddress:
            pkContactField = PAL::get_PassKit_PKContactFieldEmailAddress();
            break;
            
        case WebCore::PaymentError::ContactField::Name:
            pkContactField = PAL::get_PassKit_PKContactFieldName();
            break;
            
        case WebCore::PaymentError::ContactField::PhoneticName:
            pkContactField = PAL::get_PassKit_PKContactFieldPhoneticName();
            break;
            
        case WebCore::PaymentError::ContactField::PostalAddress:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            break;
            
        case WebCore::PaymentError::ContactField::AddressLines:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressStreetKey();
            break;
            
        case WebCore::PaymentError::ContactField::SubLocality:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressSubLocalityKey();
            break;
            
        case WebCore::PaymentError::ContactField::Locality:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressCityKey();
            break;
            
        case WebCore::PaymentError::ContactField::PostalCode:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressPostalCodeKey();
            break;
            
        case WebCore::PaymentError::ContactField::SubAdministrativeArea:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressSubAdministrativeAreaKey();
            break;
            
        case WebCore::PaymentError::ContactField::AdministrativeArea:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressStateKey();
            break;
            
        case WebCore::PaymentError::ContactField::Country:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressCountryKey();
            break;
            
        case WebCore::PaymentError::ContactField::CountryCode:
            pkContactField = PAL::get_PassKit_PKContactFieldPostalAddress();
            postalAddressKey = getCNPostalAddressISOCountryCodeKey();
            break;
        }

        [userInfo setObject:pkContactField forKey:PAL::get_PassKit_PKPaymentErrorContactFieldUserInfoKey()];
        if (postalAddressKey)
            [userInfo setObject:postalAddressKey forKey:PAL::get_PassKit_PKPaymentErrorPostalAddressUserInfoKey()];
    }

    return [NSError errorWithDomain:PAL::get_PassKit_PKPaymentErrorDomain() code:toPKPaymentErrorCode(error.code) userInfo:userInfo.get()];
}

static RetainPtr<NSArray> toNSErrors(const Vector<WebCore::PaymentError>& errors)
{
    return createNSArray(errors, [] (auto& error) {
        return toNSError(error);
    });
}

static RetainPtr<NSArray> toPKShippingMethods(const Vector<WebCore::ApplePaySessionPaymentRequest::ShippingMethod>& shippingMethods)
{
    return createNSArray(shippingMethods, [] (auto& method) {
        return toPKShippingMethod(method);
    });
}

void PaymentAuthorizationPresenter::completeMerchantValidation(const WebCore::PaymentMerchantSession& merchantSession)
{
    ASSERT(platformDelegate());
    [platformDelegate() completeMerchantValidation:merchantSession.pkPaymentMerchantSession() error:nil];
}

void PaymentAuthorizationPresenter::completePaymentMethodSelection(const Optional<WebCore::PaymentMethodUpdate>& update)
{
    ASSERT(platformDelegate());
    if (!update) {
        [platformDelegate() completePaymentMethodSelection:nil];
        return;
    }

    [platformDelegate() completePaymentMethodSelection:update->platformUpdate()];
}

void PaymentAuthorizationPresenter::completePaymentSession(const Optional<WebCore::PaymentAuthorizationResult>& result)
{
    ASSERT(platformDelegate());
    auto status = result ? toPKPaymentAuthorizationStatus(result->status) : PKPaymentAuthorizationStatusSuccess;
    RetainPtr<NSArray> errors = result ? toNSErrors(result->errors) : @[ ];
    [platformDelegate() completePaymentSession:status errors:errors.get()];
}

void PaymentAuthorizationPresenter::completeShippingContactSelection(const Optional<WebCore::ShippingContactUpdate>& update)
{
    ASSERT(platformDelegate());
    if (!update) {
        [platformDelegate() completeShippingContactSelection:nil];
        return;
    }

    // FIXME: WebCore::ShippingContactUpdate should know how to convert itself to a PKPaymentRequestShippingContactUpdate.
    auto shippingContactUpdate = adoptNS([PAL::allocPKPaymentRequestShippingContactUpdateInstance() initWithErrors:toNSErrors(update->errors).get()
        paymentSummaryItems:WebCore::platformSummaryItems(update->newTotalAndLineItems)
        shippingMethods:toPKShippingMethods(update->newShippingMethods).get()]);
    [platformDelegate() completeShippingContactSelection:shippingContactUpdate.get()];
}

void PaymentAuthorizationPresenter::completeShippingMethodSelection(const Optional<WebCore::ShippingMethodUpdate>& update)
{
    ASSERT(platformDelegate());
    if (!update) {
        [platformDelegate() completeShippingMethodSelection:nil];
        return;
    }

    // FIXME: WebCore::ShippingMethodUpdate should know how to convert itself to a PKPaymentRequestShippingMethodUpdate.
    auto shippingMethodUpdate = adoptNS([PAL::allocPKPaymentRequestShippingMethodUpdateInstance() initWithPaymentSummaryItems:WebCore::platformSummaryItems(update->newTotalAndLineItems)]);
    [platformDelegate() completeShippingMethodSelection:shippingMethodUpdate.get()];
}

} // namespace WebKit

#endif // USE(PASSKIT) && ENABLE(APPLE_PAY)
