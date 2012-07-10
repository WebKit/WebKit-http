/*
 *  Copyright (C) 2012 Samsung Electronics.  All rights reserved.
 *  Copyright (C) 2012 Intel Corporation. All rights reserved.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#ifndef BatteryProviderEflClient_h
#define BatteryProviderEflClient_h

#if ENABLE(BATTERY_STATUS)

#include "BatteryStatus.h"
#include <wtf/text/AtomicString.h>

namespace WebCore {

class BatteryProviderEflClient {
public:
    virtual void didChangeBatteryStatus(const AtomicString& eventType, PassRefPtr<BatteryStatus>) = 0;
};

} // namespace WebCore

#endif // ENABLE(BATTERY_STATUS)

#endif // BatteryProviderEflClient_h
