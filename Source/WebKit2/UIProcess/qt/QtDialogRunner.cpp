/*
 * Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this program; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "config.h"
#include "QtDialogRunner.h"

#include "DialogContextObjects.h"
#include "qquickwebview_p_p.h"
#include "qwebpermissionrequest_p.h"
#include <QtQml/QQmlComponent>
#include <QtQml/QQmlContext>
#include <QtQml/QQmlEngine>
#include <QtQuick/QQuickItem>

namespace WebKit {

QtDialogRunner::QtDialogRunner(QQuickWebView* webView)
    : QEventLoop()
    , m_webView(webView)
    , m_wasAccepted(false)
{
}

QtDialogRunner::~QtDialogRunner()
{
}

void QtDialogRunner::run()
{
    DialogContextBase* context = static_cast<DialogContextBase*>(m_dialogContext->contextObject());

    // We may have already been dismissed as part of Component.onCompleted()
    if (context->m_dismissed)
        return;

    connect(context, SIGNAL(dismissed()), SLOT(quit()));
    exec(); // Spin the event loop
}

bool QtDialogRunner::initForAlert(const QString& message)
{
    QQmlComponent* component = m_webView->experimental()->alertDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message);

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForConfirm(const QString& message)
{
    QQmlComponent* component = m_webView->experimental()->confirmDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message);
    connect(contextObject, SIGNAL(accepted(QString)), SLOT(onAccepted()));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForPrompt(const QString& message, const QString& defaultValue)
{
    QQmlComponent* component = m_webView->experimental()->promptDialog();
    if (!component)
        return false;

    DialogContextObject* contextObject = new DialogContextObject(message, defaultValue);
    connect(contextObject, SIGNAL(accepted(QString)), SLOT(onAccepted(QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForAuthentication(const QString& hostname, const QString& realm, const QString& prefilledUsername)
{
    QQmlComponent* component = m_webView->experimental()->authenticationDialog();
    if (!component)
        return false;

    HttpAuthenticationDialogContextObject* contextObject = new HttpAuthenticationDialogContextObject(hostname, realm, prefilledUsername);
    connect(contextObject, SIGNAL(accepted(QString, QString)), SLOT(onAuthenticationAccepted(QString, QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForProxyAuthentication(const QString& hostname, uint16_t port, const QString& prefilledUsername)
{
    QQmlComponent* component = m_webView->experimental()->proxyAuthenticationDialog();
    if (!component)
        return false;

    ProxyAuthenticationDialogContextObject* contextObject = new ProxyAuthenticationDialogContextObject(hostname, port, prefilledUsername);
    connect(contextObject, SIGNAL(accepted(QString, QString)), SLOT(onAuthenticationAccepted(QString, QString)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForCertificateVerification(const QString& hostname)
{
    QQmlComponent* component = m_webView->experimental()->certificateVerificationDialog();
    if (!component)
        return false;

    CertificateVerificationDialogContextObject* contextObject = new CertificateVerificationDialogContextObject(hostname);
    connect(contextObject, SIGNAL(accepted()), SLOT(onAccepted()));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForFilePicker(const QStringList& selectedFiles, bool allowMultiple)
{
    QQmlComponent* component = m_webView->experimental()->filePicker();
    if (!component)
        return false;

    FilePickerContextObject* contextObject = new FilePickerContextObject(selectedFiles, allowMultiple);
    connect(contextObject, SIGNAL(fileSelected(QStringList)), SLOT(onFileSelected(QStringList)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::initForDatabaseQuotaDialog(const QString& databaseName, const QString& displayName, WKSecurityOriginRef securityOrigin, quint64 currentQuota, quint64 currentOriginUsage, quint64 currentDatabaseUsage, quint64 expectedUsage)
{
    QQmlComponent* component = m_webView->experimental()->databaseQuotaDialog();
    if (!component)
        return false;

    DatabaseQuotaDialogContextObject* contextObject = new DatabaseQuotaDialogContextObject(databaseName, displayName, securityOrigin, currentQuota, currentOriginUsage, currentDatabaseUsage, expectedUsage);
    connect(contextObject, SIGNAL(accepted(quint64)), SLOT(onDatabaseQuotaAccepted(quint64)));

    return createDialog(component, contextObject);
}

bool QtDialogRunner::createDialog(QQmlComponent* component, QObject* contextObject)
{
    QQmlContext* baseContext = component->creationContext();
    if (!baseContext)
        baseContext = QQmlEngine::contextForObject(m_webView);
    m_dialogContext = std::make_unique<QQmlContext>(baseContext);

    // This makes both "message" and "model.message" work for the dialog,
    // just like QtQuick's ListView delegates.
    contextObject->setParent(m_dialogContext.get());
    m_dialogContext->setContextProperty(QStringLiteral("model"), contextObject);
    m_dialogContext->setContextObject(contextObject);

    QObject* object = component->beginCreate(m_dialogContext.get());
    if (!object) {
        m_dialogContext = nullptr;
        return false;
    }

    m_dialog.reset(qobject_cast<QQuickItem*>(object));
    if (!m_dialog) {
        m_dialogContext = nullptr;
        return false;
    }

    QQuickWebViewPrivate::get(m_webView)->addAttachedPropertyTo(m_dialog.get());
    m_dialog->setParentItem(m_webView);

    // Only fully create the component once we've set both a parent
    // and the needed context and attached properties, so that dialogs
    // can do useful stuff in their Component.onCompleted() method.
    component->completeCreate();

    // FIXME: As part of completeCreate, the bindings of the item will be
    // evaluated, but for some reason doing mapToItem/mapFromItem in a
    // binding will not work as expected, even if we at binding evaluation
    // time have the parent and all the way up to the root QML item.
    // As a workaround you can set whichever property you need in
    // Component.onCompleted, even to a binding using Qt.bind().

    return true;
}

void QtDialogRunner::onAccepted(const QString& result)
{
    m_wasAccepted = true;
    m_result = result;
}

void QtDialogRunner::onAuthenticationAccepted(const QString& username, const QString& password)
{
    m_username = username;
    m_password = password;
}

void QtDialogRunner::onFileSelected(const QStringList& filePaths)
{
    m_wasAccepted = true;
    m_filepaths = filePaths;
}

void QtDialogRunner::onDatabaseQuotaAccepted(quint64 quota)
{
    m_wasAccepted = true;
    m_databaseQuota = quota;
}

} // namespace WebKit

#include "moc_DialogContextObjects.cpp"
#include "moc_QtDialogRunner.cpp"
