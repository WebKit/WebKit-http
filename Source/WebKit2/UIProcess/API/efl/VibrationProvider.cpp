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
#include "VibrationProvider.h"

#if ENABLE(VIBRATION)

#include "WKAPICast.h"
#include "WKVibration.h"
#include <Evas.h>

using namespace WebCore;

/**
 * \struct  _Ewk_Vibration_Client
 * @brief   Contains the vibration client callbacks.
 */
struct _Ewk_Vibration_Client {
    Ewk_Vibration_Client_Vibrate_Cb vibrate;
    Ewk_Vibration_Client_Vibration_Cancel_Cb cancelVibration;
    void* userData;

    _Ewk_Vibration_Client(Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancelVibration, void* userData)
        : vibrate(vibrate)
        , cancelVibration(cancelVibration)
        , userData(userData)
    { }
};

static inline VibrationProvider* toVibrationProvider(const void* clientInfo)
{
    return static_cast<VibrationProvider*>(const_cast<void*>(clientInfo));
}

static void vibrateCallback(WKVibrationRef vibrationRef, uint64_t vibrationTime, const void* clientInfo)
{
    toVibrationProvider(clientInfo)->vibrate(vibrationTime);
}

static void cancelVibrationCallback(WKVibrationRef vibrationRef, const void* clientInfo)
{
    toVibrationProvider(clientInfo)->cancelVibration();
}

PassRefPtr<VibrationProvider> VibrationProvider::create(WKVibrationRef wkVibrationRef)
{
    return adoptRef(new VibrationProvider(wkVibrationRef));
}

VibrationProvider::VibrationProvider(WKVibrationRef wkVibrationRef)
    : m_wkVibrationRef(wkVibrationRef)
{
    ASSERT(wkVibrationRef);

    WKVibrationProvider wkVibrationProvider = {
        kWKVibrationProviderCurrentVersion,
        this, // clientInfo
        vibrateCallback,
        cancelVibrationCallback
    };
    WKVibrationSetProvider(m_wkVibrationRef.get(), &wkVibrationProvider);
}

VibrationProvider::~VibrationProvider()
{
}

void VibrationProvider::vibrate(uint64_t vibrationTime)
{
    if (m_vibrationClient && m_vibrationClient->vibrate)
        m_vibrationClient->vibrate(vibrationTime, m_vibrationClient->userData);
}

void VibrationProvider::cancelVibration()
{
    if (m_vibrationClient && m_vibrationClient->cancelVibration)
        m_vibrationClient->cancelVibration(m_vibrationClient->userData);
}

void VibrationProvider::setVibrationClientCallbacks(Ewk_Vibration_Client_Vibrate_Cb vibrate, Ewk_Vibration_Client_Vibration_Cancel_Cb cancelVibration, void* data)
{
    m_vibrationClient = adoptPtr(new Ewk_Vibration_Client(vibrate, cancelVibration, data));
}

#endif // ENABLE(VIBRATION)
