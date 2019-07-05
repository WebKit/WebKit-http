/*
    Copyright (C) 2009 Nokia Corporation and/or its subsidiary(-ies)

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "config.h"
#include "NetworkStateNotifier.h"

#ifndef QT_NO_BEARERMANAGEMENT

#include "NetworkStateNotifierPrivate.h"
#include <QNetworkConfigurationManager>
#include <QTimer>

namespace WebCore {

NetworkStateNotifierPrivate::NetworkStateNotifierPrivate(NetworkStateNotifier& notifier)
    : m_online(false)
    , m_networkAccessAllowed(true)
    , m_notifier(notifier)
{
    // Initialization is delayed because QNetworkConfigurationManager starts a new thread that causes
    // deadlock on Mac because all the static initializers share the same lock. Both NetworkStateNotifier and Qt internals
    // triggered in new thread use static initializer. See also: http://openradar.appspot.com/11217150.
    QTimer::singleShot(0, this, SLOT(initialize()));
}

void NetworkStateNotifierPrivate::setNetworkAccessAllowed(bool isAllowed)
{
    if (isAllowed == m_networkAccessAllowed)
        return;

    m_networkAccessAllowed = isAllowed;
    if (m_online)
        m_notifier.updateState();
}

void NetworkStateNotifierPrivate::setOnlineState(bool isOnline)
{
    if (m_online == isOnline)
        return;

    m_online = isOnline;
    if (m_networkAccessAllowed)
        m_notifier.updateState();
}

void NetworkStateNotifierPrivate::initialize()
{
    m_configurationManager = std::make_unique<QNetworkConfigurationManager>();
    setOnlineState(m_configurationManager->isOnline());
    connect(m_configurationManager.get(), &QNetworkConfigurationManager::onlineStateChanged,
        this, &NetworkStateNotifierPrivate::setOnlineState);
}

NetworkStateNotifierPrivate::~NetworkStateNotifierPrivate() = default;

void NetworkStateNotifier::updateStateWithoutNotifying()
{
    m_isOnLine = p->effectivelyOnline();
}

void NetworkStateNotifier::startObserving()
{
    p = std::make_unique<NetworkStateNotifierPrivate>(*this);
    m_isOnLine = p->effectivelyOnline();
}

void NetworkStateNotifier::setNetworkAccessAllowed(bool isAllowed)
{
    p->setNetworkAccessAllowed(isAllowed);
}

} // namespace WebCore

#include "moc_NetworkStateNotifierPrivate.cpp"

#endif
