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
#include "BatteryProvider.h"

#if ENABLE(BATTERY_STATUS)

#include "WKAPICast.h"
#include "WKBatteryManager.h"
#include "WKBatteryStatus.h"

using namespace WebCore;
using namespace WebKit;

static inline BatteryProvider* toBatteryProvider(const void* clientInfo)
{
    return static_cast<BatteryProvider*>(const_cast<void*>(clientInfo));
}

static void startUpdatingCallback(WKBatteryManagerRef batteryManager, const void* clientInfo)
{
    toBatteryProvider(clientInfo)->startUpdating();
}

static void stopUpdatingCallback(WKBatteryManagerRef batteryManager, const void* clientInfo)
{
    toBatteryProvider(clientInfo)->stopUpdating();
}

BatteryProvider::~BatteryProvider()
{
    m_provider.stopUpdating();
}

PassRefPtr<BatteryProvider> BatteryProvider::create(WKBatteryManagerRef wkBatteryManager)
{
    return adoptRef(new BatteryProvider(wkBatteryManager));
}

BatteryProvider::BatteryProvider(WKBatteryManagerRef wkBatteryManager)
    : m_wkBatteryManager(wkBatteryManager)
    , m_provider(this)
{
    ASSERT(wkBatteryManager);

    WKBatteryProvider wkBatteryProvider = {
        kWKBatteryProviderCurrentVersion,
        this, // clientInfo
        startUpdatingCallback,
        stopUpdatingCallback
    };
    WKBatteryManagerSetProvider(m_wkBatteryManager.get(), &wkBatteryProvider);
}

void BatteryProvider::startUpdating()
{
    m_provider.startUpdating();
}

void BatteryProvider::stopUpdating()
{
    m_provider.stopUpdating();
}

void BatteryProvider::didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus> status)
{
    WKRetainPtr<WKBatteryStatusRef> wkBatteryStatus(AdoptWK, WKBatteryStatusCreate(status->charging(), status->chargingTime(), status->dischargingTime(), status->level()));
    WKBatteryManagerProviderDidChangeBatteryStatus(m_wkBatteryManager.get(), toAPI(eventType.impl()), wkBatteryStatus.get());
}

#endif // ENABLE(BATTERY_STATUS)
