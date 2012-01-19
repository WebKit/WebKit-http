/*
 * Copyright (C) 2011 Apple Inc. All rights reserved.
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
#include "NotificationPermissionRequestManager.h"

#include "WebCoreArgumentCoders.h"
#include "WebNotificationManagerProxyMessages.h"
#include "WebPage.h"
#include "WebPageProxyMessages.h"
#include "WebProcess.h"
#include <WebCore/Notification.h>
#include <WebCore/Page.h>
#include <WebCore/ScriptExecutionContext.h>
#include <WebCore/SecurityOrigin.h>
#include <WebCore/Settings.h>

using namespace WebCore;

namespace WebKit {

#if ENABLE(NOTIFICATIONS)
static uint64_t generateRequestID()
{
    static uint64_t uniqueRequestID = 1;
    return uniqueRequestID++;
}
#endif

PassRefPtr<NotificationPermissionRequestManager> NotificationPermissionRequestManager::create(WebPage* page)
{
    return adoptRef(new NotificationPermissionRequestManager(page));
}

NotificationPermissionRequestManager::NotificationPermissionRequestManager(WebPage* page)
    : m_page(page)
{
}

void NotificationPermissionRequestManager::startRequest(SecurityOrigin* origin, PassRefPtr<VoidCallback> callback)
{
#if ENABLE(NOTIFICATIONS)
    if (permissionLevel(origin) != NotificationPresenter::PermissionNotAllowed)
        return;

    uint64_t requestID = generateRequestID();
    m_originToIDMap.set(origin, requestID);
    m_idToOriginMap.set(requestID, origin);
    m_idToCallbackMap.set(requestID, callback);
    m_page->send(Messages::WebPageProxy::RequestNotificationPermission(requestID, origin->toString()));
#else
    UNUSED_PARAM(origin);
    UNUSED_PARAM(callback);
#endif
}

void NotificationPermissionRequestManager::cancelRequest(SecurityOrigin* origin)
{
#if ENABLE(NOTIFICATIONS)
    uint64_t id = m_originToIDMap.take(origin);
    if (!id)
        return;
    
    m_idToOriginMap.remove(id);
    m_idToCallbackMap.remove(id);
#else
    UNUSED_PARAM(origin);
#endif
}

NotificationPresenter::Permission NotificationPermissionRequestManager::permissionLevel(SecurityOrigin* securityOrigin)
{
#if ENABLE(NOTIFICATIONS)
    if (!m_page->corePage()->settings()->notificationsEnabled())
        return NotificationPresenter::PermissionDenied;
    
    return WebProcess::shared().notificationManager().policyForOrigin(securityOrigin);
#else
    UNUSED_PARAM(securityOrigin);
    return NotificationPresenter::PermissionDenied;
#endif
}

void NotificationPermissionRequestManager::didReceiveNotificationPermissionDecision(uint64_t requestID, bool allowed)
{
#if ENABLE(NOTIFICATIONS)
    if (!isRequestIDValid(requestID))
        return;

    RefPtr<VoidCallback> callback = m_idToCallbackMap.take(requestID);
    if (!callback)
        return;
    
    callback->handleEvent();
#else
    UNUSED_PARAM(requestID);
    UNUSED_PARAM(allowed);
#endif    
}

} // namespace WebKit
