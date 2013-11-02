/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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
#include "WKOriginDataManager.h"

#include "WKAPICast.h"
#include "WebOriginDataManagerProxy.h"

using namespace WebKit;

WKTypeID WKOriginDataManagerGetTypeID()
{
    return toAPI(WebOriginDataManagerProxy::APIType);
}

void WKOriginDataManagerGetOrigins(WKOriginDataManagerRef originDataManagerRef, WKOriginDataTypes types, void* context, WKOriginDataManagerGetOriginsFunction callback)
{
    toImpl(originDataManagerRef)->getOrigins(types, ArrayCallback::create(context, callback));
}

void WKOriginDataManagerDeleteEntriesForOrigin(WKOriginDataManagerRef originDataManagerRef, WKOriginDataTypes types, WKSecurityOriginRef originRef)
{
    toImpl(originDataManagerRef)->deleteEntriesForOrigin(types, toImpl(originRef));
}

void WKOriginDataManagerDeleteAllEntries(WKOriginDataManagerRef originDataManagerRef, WKOriginDataTypes types)
{
    toImpl(originDataManagerRef)->deleteAllEntries(types);
}

void WKOriginDataManagerStartObservingChanges(WKOriginDataManagerRef originDataManagerRef, WKOriginDataTypes types)
{
    toImpl(originDataManagerRef)->startObservingChanges(types);
}

void WKOriginDataManagerStopObservingChanges(WKOriginDataManagerRef originDataManagerRef, WKOriginDataTypes types)
{
    toImpl(originDataManagerRef)->stopObservingChanges(types);
}

void WKOriginDataManagerSetChangeClient(WKOriginDataManagerRef originDataManagerRef, const WKOriginDataManagerChangeClient *client)
{
    toImpl(originDataManagerRef)->setChangeClient(client);
}
