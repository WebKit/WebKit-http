/*
 * Copyright (C) 2014 Apple Inc. All rights reserved.
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

#ifndef HIDGamepadProvider_h
#define HIDGamepadProvider_h

#if ENABLE(GAMEPAD)

#include "GamepadProvider.h"
#include "HIDGamepad.h"
#include "Timer.h"
#include <IOKit/hid/IOHIDManager.h>
#include <wtf/Deque.h>
#include <wtf/HashMap.h>
#include <wtf/HashSet.h>
#include <wtf/NeverDestroyed.h>
#include <wtf/RetainPtr.h>

namespace WebCore {

class GamepadProviderClient;

class HIDGamepadProvider : public GamepadProvider {
    WTF_MAKE_NONCOPYABLE(HIDGamepadProvider);
    friend class NeverDestroyed<HIDGamepadProvider>;
public:
    static HIDGamepadProvider& shared();

    virtual void startMonitoringGamepads(GamepadProviderClient*);
    virtual void stopMonitoringGamepads(GamepadProviderClient*);
    virtual const Vector<PlatformGamepad*>& platformGamepads() { return m_gamepadVector; }

    void deviceAdded(IOHIDDeviceRef);
    void deviceRemoved(IOHIDDeviceRef);
    void valuesChanged(IOHIDValueRef);

private:
    HIDGamepadProvider();

    std::unique_ptr<HIDGamepad> removeGamepadForDevice(IOHIDDeviceRef);

    void openAndScheduleManager();
    void closeAndUnscheduleManager();

    void connectionDelayTimerFired(Timer<HIDGamepadProvider>&);
    void inputNotificationTimerFired(Timer<HIDGamepadProvider>&);

    unsigned indexForNewlyConnectedDevice();

    Vector<PlatformGamepad*> m_gamepadVector;
    HashMap<IOHIDDeviceRef, std::unique_ptr<HIDGamepad>> m_gamepadMap;

    RetainPtr<IOHIDManagerRef> m_manager;

    HashSet<GamepadProviderClient*> m_clients;
    bool m_shouldDispatchCallbacks;

    Timer<HIDGamepadProvider> m_connectionDelayTimer;
    Timer<HIDGamepadProvider> m_inputNotificationTimer;
};

} // namespace WebCore

#endif // ENABLE(GAMEPAD)
#endif // HIDGamepadProvider_h
