/*
 * Copyright (C) 2011 Research In Motion Limited. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "config.h"
#include "NotificationPresenterImpl.h"

#if ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
#include <BlackBerryPlatformString.h>
#include <Event.h>
#include <Notification.h>
#include <NotificationPresenterBlackBerry.h>
#include <ScriptExecutionContext.h>
#include <UUID.h>


namespace WebCore {

NotificationClient* NotificationPresenterImpl::instance()
{
    static NotificationPresenterImpl* s_instance = 0;
    if (!s_instance)
        s_instance = new NotificationPresenterImpl;
    return s_instance;
}

NotificationPresenterImpl::NotificationPresenterImpl()
{
    m_platformPresenter = adoptPtr(BlackBerry::Platform::NotificationPresenterBlackBerry::create(this));
}

NotificationPresenterImpl::~NotificationPresenterImpl()
{
}

bool NotificationPresenterImpl::show(Notification* notification)
{
    ASSERT(notification);
    ASSERT(notification->scriptExecutionContext());
    RefPtr<Notification> n = notification;
    if (m_notifications.contains(n))
        return false;

    if (checkPermission(notification->scriptExecutionContext()) != NotificationClient::PermissionAllowed)
        return false;

    String uuid = createCanonicalUUIDString();
    m_notifications.add(n, uuid);
    String message;
    if (n->isHTML()) {
        // FIXME: Load and display HTML content.
        message = n->url().string();
    } else {
        // FIXME: Strip the content into one line.
        message = n->contents().title + ": " + n->contents().body;
    }

    m_platformPresenter->show(uuid, message);
    return true;
}

void NotificationPresenterImpl::cancel(Notification* notification)
{
    ASSERT(notification);
    RefPtr<Notification> n = notification;

    NotificationMap::iterator it = m_notifications.find(n);
    if (it == m_notifications.end())
        return;

    m_platformPresenter->cancel(it->value);
    m_notifications.remove(it);
}

void NotificationPresenterImpl::notificationObjectDestroyed(Notification* notification)
{
    ASSERT(notification);
    cancel(notification);
}

void NotificationPresenterImpl::notificationControllerDestroyed()
{
}

void NotificationPresenterImpl::requestPermission(ScriptExecutionContext* context, PassRefPtr<VoidCallback> callback)
{
    ASSERT(context);
    m_permissionRequests.add(context, callback);
    m_platformPresenter->requestPermission(context->url().host());
}

void NotificationPresenterImpl::onPermission(const BlackBerry::Platform::String& domain, bool isAllowed)
{
    ASSERT(!domain.empty());
    PermissionRequestMap::iterator it = m_permissionRequests.begin();
    for (; it != m_permissionRequests.end(); ++it) {
        if (it->key->url().host() != domain)
            continue;

        if (isAllowed) {
            m_allowedDomains.add(domain);
            it->value->handleEvent();
        } else
            m_allowedDomains.remove(domain);

        m_permissionRequests.remove(it);
        return;
    }
}

void NotificationPresenterImpl::cancelRequestsForPermission(ScriptExecutionContext*)
{
    // Because we are using modal dialogs to request permission, it's impossible to cancel them.
}

NotificationClient::Permission NotificationPresenterImpl::checkPermission(ScriptExecutionContext* context)
{
    ASSERT(context);
    // FIXME: Should store the permission information into file permanently instead of in m_allowedDomains.
    // The suggested place to do this is in m_platformPresenter->checkPermission().
    if (m_allowedDomains.contains(context->url().host()))
        return NotificationClient::PermissionAllowed;
    return NotificationClient::PermissionNotAllowed;
}

// This function is called in platform side.
void NotificationPresenterImpl::notificationClicked(const BlackBerry::Platform::String& id)
{
    ASSERT(!id.empty());
    NotificationMap::iterator it = m_notifications.begin();
    for (; it != m_notifications.end(); ++it) {
        if (it->value == id && it->key->scriptExecutionContext()) {
            RefPtr<Notification> notification = it->key;
            it->key->dispatchEvent(Event::create(eventNames().clickEvent, false, true));
            m_notifications.remove(notification);
            return;
        }
    }
}

} // namespace WebKit

#endif // ENABLE(NOTIFICATIONS) || ENABLE(LEGACY_NOTIFICATIONS)
