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
#include "WebNotificationManager.h"

#include "WebPage.h"
#include "WebProcess.h"

#if ENABLE(NOTIFICATIONS)
#include "WebNotificationManagerProxyMessages.h"
#include <WebCore/Notification.h>
#endif

using namespace WebCore;

namespace WebKit {
    
WebNotificationManager::WebNotificationManager(WebProcess* process)
    : m_process(process)
{
}

WebNotificationManager::~WebNotificationManager()
{
}

bool WebNotificationManager::show(Notification* notification, WebPage* page)
{
#if ENABLE(NOTIFICATIONS)
    m_process->connection()->send(Messages::WebNotificationManagerProxy::Show(notification->contents().title, notification->contents().body, 0), page->pageID());
#endif
    return true;
}

void WebNotificationManager::cancel(Notification* notification, WebPage* page)
{
#if ENABLE(NOTIFICATIONS)
    m_process->connection()->send(Messages::WebNotificationManagerProxy::Cancel(0), page->pageID());
#endif
}

} // namespace WebKit
