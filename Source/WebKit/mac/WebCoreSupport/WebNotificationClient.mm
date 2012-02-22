/*
 * Copyright (C) 2011, 2012 Apple Inc. All rights reserved.
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

#import "WebNotificationClient.h"

#if ENABLE(NOTIFICATIONS)
#import "WebDelegateImplementationCaching.h"
#import "WebNotificationInternal.h"
#import "WebPreferencesPrivate.h"
#import "WebSecurityOriginInternal.h"
#import "WebUIDelegatePrivate.h"
#import "WebViewInternal.h"
#import <WebCore/BlockExceptions.h>
#import <WebCore/Page.h>
#import <WebCore/ScriptExecutionContext.h>
#endif

using namespace WebCore;

#if ENABLE(NOTIFICATIONS)
@interface WebNotificationPolicyListener : NSObject <WebAllowDenyPolicyListener>
{
    RefPtr<VoidCallback> _callback;
}
- (id)initWithCallback:(PassRefPtr<VoidCallback>)callback;
@end
#endif

#if ENABLE(NOTIFICATIONS)
static uint64_t generateNotificationID()
{
    static uint64_t uniqueNotificationID = 1;
    return uniqueNotificationID++;
}
#endif

WebNotificationClient::WebNotificationClient(WebView *webView)
    : m_webView(webView)
{
}

bool WebNotificationClient::show(Notification* notification)
{
#if ENABLE(NOTIFICATIONS)
    if (![m_webView _notificationProvider])
        return false;

    uint64_t notificationID = generateNotificationID();
    RetainPtr<WebNotification> webNotification = adoptNS([[WebNotification alloc] initWithCoreNotification:notification notificationID:notificationID]);
    m_notificationMap.set(notification, webNotification);

    NotificationContextMap::iterator it = m_notificationContextMap.add(notification->scriptExecutionContext(), Vector<RetainPtr<WebNotification> >()).first;
    it->second.append(webNotification);

    [[m_webView _notificationProvider] showNotification:webNotification.get() fromWebView:m_webView];
    return true;
#else
    UNUSED_PARAM(notification);
    return false;
#endif
}

void WebNotificationClient::cancel(Notification* notification)
{
#if ENABLE(NOTIFICATIONS)
    WebNotification *webNotification = m_notificationMap.get(notification).get();
    if (!webNotification)
        return;

    [[m_webView _notificationProvider] cancelNotification:webNotification];
#else
    UNUSED_PARAM(notification);
#endif
}

void WebNotificationClient::clearNotifications(ScriptExecutionContext* context)
{
#if ENABLE(NOTIFICATIONS)
    NotificationContextMap::iterator it = m_notificationContextMap.find(context);
    if (it == m_notificationContextMap.end())
        return;
    
    Vector<RetainPtr<WebNotification> >& webNotifications = it->second;
    NSMutableArray *nsIDs = [NSMutableArray array];
    size_t count = webNotifications.size();
    for (size_t i = 0; i < count; ++i) {
        WebNotification *webNotification = webNotifications[i].get();
        [nsIDs addObject:[NSNumber numberWithUnsignedLongLong:[webNotification notificationID]]];
        m_notificationMap.remove(core(webNotification));
    }

    [[m_webView _notificationProvider] clearNotifications:nsIDs];
    m_notificationContextMap.remove(it);
#else
    UNUSED_PARAM(context);
#endif
}

void WebNotificationClient::notificationObjectDestroyed(Notification* notification)
{
#if ENABLE(NOTIFICATIONS)
    RetainPtr<WebNotification> webNotification = m_notificationMap.take(notification);
    if (!webNotification)
        return;

    NotificationContextMap::iterator it = m_notificationContextMap.find(notification->scriptExecutionContext());
    ASSERT(it != m_notificationContextMap.end());
    size_t index = it->second.find(webNotification);
    ASSERT(index != notFound);
    it->second.remove(index);
    if (it->second.isEmpty())
        m_notificationContextMap.remove(it);

    [[m_webView _notificationProvider] notificationDestroyed:webNotification.get()];
#else
    UNUSED_PARAM(notification);
#endif
}

void WebNotificationClient::notificationControllerDestroyed()
{
#if ENABLE(NOTIFICATIONS)
    [m_webView _notificationControllerDestroyed];
#endif
    delete this;
}

void WebNotificationClient::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
#if ENABLE(NOTIFICATIONS)
    BEGIN_BLOCK_OBJC_EXCEPTIONS;
    
    SEL selector = @selector(webView:decidePolicyForNotificationRequestFromOrigin:listener:);
    if (![[m_webView UIDelegate] respondsToSelector:selector])
        return;
    
    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:context->securityOrigin()];
    WebNotificationPolicyListener* listener = [[WebNotificationPolicyListener alloc] initWithCallback:callback];
    
    CallUIDelegate(m_webView, selector, webOrigin, listener);
    
    [webOrigin release];
    [listener release];
    
    END_BLOCK_OBJC_EXCEPTIONS;
#else
    UNUSED_PARAM(context);
    UNUSED_PARAM(callback);
#endif
}

NotificationPresenter::Permission WebNotificationClient::checkPermission(ScriptExecutionContext* context)
{
#if ENABLE(NOTIFICATIONS)
    if (!context || !context->isDocument())
        return NotificationPresenter::PermissionDenied;
    if (![[m_webView preferences] notificationsEnabled])
        return NotificationPresenter::PermissionDenied;
    WebSecurityOrigin *webOrigin = [[WebSecurityOrigin alloc] _initWithWebCoreSecurityOrigin:context->securityOrigin()];
    WebNotificationPermission permission = [[m_webView _notificationProvider] policyForOrigin:webOrigin];
    [webOrigin release];
    switch (permission) {
        case WebNotificationPermissionAllowed:
            return NotificationPresenter::PermissionAllowed;
        case WebNotificationPermissionDenied:
            return NotificationPresenter::PermissionDenied;
        case WebNotificationPermissionNotAllowed:
            return NotificationPresenter::PermissionNotAllowed;
        default:
            return NotificationPresenter::PermissionNotAllowed;
    }
#else
    UNUSED_PARAM(context);
    return NotificationPresenter::PermissionDenied;
#endif
}

#if ENABLE(NOTIFICATIONS)
@implementation WebNotificationPolicyListener
- (id)initWithCallback:(PassRefPtr<VoidCallback>)callback
{
    if (!(self = [super init]))
        return nil;

    ASSERT(callback);
    _callback = callback;
    return self;
}

- (void)allow
{
    _callback->handleEvent();
}

- (void)deny
{
    _callback->handleEvent();
}

@end
#endif
