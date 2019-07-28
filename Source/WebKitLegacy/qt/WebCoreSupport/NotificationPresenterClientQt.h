/*
 * Copyright (C) 2009 Google Inc. All rights reserved.
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef NotificationPresenterClientQt_h
#define NotificationPresenterClientQt_h

#include "QtPlatformPlugin.h"
#include <QMultiHash>
#include <QScopedPointer>
#include <WebCore/NotificationClient.h>
#include <WebCore/Timer.h>

class QWebFrameAdapter;
class QWebPageAdapter;

namespace WebCore {

class Document;
class Frame;
class ScriptExecutionContext;

class NotificationWrapper final : public QObject, public QWebNotificationData {
    Q_OBJECT
public:
    NotificationWrapper();
    ~NotificationWrapper() { }

    void close();
    void sendDisplayEvent();
    const QString title() const final;
    const QString message() const final;
    const QUrl iconUrl() const final;
    const QUrl openerPageUrl() const final;

public Q_SLOTS:
    void notificationClosed();
    void notificationClicked();

private:
    std::unique_ptr<QWebNotificationPresenter> m_presenter;
    Timer m_closeTimer;
    Timer m_displayEventTimer;

    friend class NotificationPresenterClientQt;
};

#if ENABLE(NOTIFICATIONS)

typedef QHash <Notification*, NotificationWrapper*> NotificationsQueue;

class NotificationPresenterClientQt final : public NotificationClient {
public:
    NotificationPresenterClientQt();
    ~NotificationPresenterClientQt();

    /* WebCore::NotificationClient interface */
    bool show(Notification*) override;
    void cancel(Notification*) override;
    void notificationObjectDestroyed(Notification*) override;
    void notificationControllerDestroyed() override;
    void requestPermission(ScriptExecutionContext*, RefPtr<NotificationPermissionCallback>&&) override;
    bool hasPendingPermissionRequests(ScriptExecutionContext*) const override;
    NotificationClient::Permission checkPermission(ScriptExecutionContext*) override;
    void cancelRequestsForPermission(ScriptExecutionContext*) override;

    void cancel(NotificationWrapper*);

    void setNotificationsAllowedForFrame(Frame*, bool allowed);

    static bool dumpNotification;

    void addClient() { m_clientCount++; }
#ifndef QT_NO_SYSTEMTRAYICON
    bool hasSystemTrayIcon() const { return !m_systemTrayIcon.isNull(); }
    void setSystemTrayIcon(QObject* icon) { m_systemTrayIcon.reset(icon); }
#endif
    void removeClient();
    static NotificationPresenterClientQt* notificationPresenter();

    Notification* notificationForWrapper(const NotificationWrapper*) const;
    void notificationClicked(NotificationWrapper*);
    void notificationClicked(const QString& title);
    void sendDisplayEvent(NotificationWrapper*);

    void clearCachedPermissions();

private:
    void sendEvent(Notification*, const AtomString& eventName);
    void displayNotification(Notification*);
    void removeReplacedNotificationFromQueue(Notification*);
    void detachNotification(Notification*);
    void dumpReplacedIdText(Notification*);
    void dumpShowText(Notification*);
    QWebPageAdapter* toPage(ScriptExecutionContext*);
    QWebFrameAdapter* toFrame(ScriptExecutionContext*);

    int m_clientCount;
    struct CallbacksInfo {
        QWebFrameAdapter* m_frame;
        QList<RefPtr<NotificationPermissionCallback> > m_callbacks;
    };
    QHash<ScriptExecutionContext*,  CallbacksInfo > m_pendingPermissionRequests;
    QHash<ScriptExecutionContext*, NotificationClient::Permission> m_cachedPermissions;

    NotificationsQueue m_notifications;
    QtPlatformPlugin m_platformPlugin;
#ifndef QT_NO_SYSTEMTRAYICON
    QScopedPointer<QObject> m_systemTrayIcon;
#endif
};

#endif // ENABLE(NOTIFICATIONS)

}

#endif // NotificationPresenterClientQt_h
