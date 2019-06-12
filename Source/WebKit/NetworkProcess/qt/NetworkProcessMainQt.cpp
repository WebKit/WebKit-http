/*
 * Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
 * Copyright (C) 2014 Igalia S.L.
 * Copyright (C) 2013 Company 100 Inc.
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
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS''
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
#include "NetworkProcess.h"

#include "ChildProcessMain.h"

#include <QCoreApplication>
#include <QNetworkProxyFactory>

using namespace WebCore;

namespace WebKit {

class EnvHttpProxyFactory : public QNetworkProxyFactory {
public:
    EnvHttpProxyFactory() { }

    bool initializeFromEnvironment();

    QList<QNetworkProxy> queryProxy(const QNetworkProxyQuery& = QNetworkProxyQuery());

private:
    QList<QNetworkProxy> m_httpProxy;
    QList<QNetworkProxy> m_httpsProxy;
};

bool EnvHttpProxyFactory::initializeFromEnvironment()
{
    bool wasSetByEnvironment = false;

    QUrl proxyUrl = QUrl::fromUserInput(QString::fromLocal8Bit(qgetenv("http_proxy")));
    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
        int proxyPort = (proxyUrl.port() > 0) ? proxyUrl.port() : 8080;
        m_httpProxy << QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyPort);
        wasSetByEnvironment = true;
    } else
        m_httpProxy << QNetworkProxy::NoProxy;

    proxyUrl = QUrl::fromUserInput(QString::fromLocal8Bit(qgetenv("https_proxy")));
    if (proxyUrl.isValid() && !proxyUrl.host().isEmpty()) {
        int proxyPort = (proxyUrl.port() > 0) ? proxyUrl.port() : 8080;
        m_httpsProxy << QNetworkProxy(QNetworkProxy::HttpProxy, proxyUrl.host(), proxyPort);
        wasSetByEnvironment = true;
    } else
        m_httpsProxy << QNetworkProxy::NoProxy;

    return wasSetByEnvironment;
}

QList<QNetworkProxy> EnvHttpProxyFactory::queryProxy(const QNetworkProxyQuery& query)
{
    QString protocol = query.protocolTag().toLower();
    bool localHost = false;

    if (!query.peerHostName().compare(QLatin1String("localhost"), Qt::CaseInsensitive) || !query.peerHostName().compare(QLatin1String("127.0.0.1"), Qt::CaseInsensitive))
        localHost = true;
    if (protocol == QLatin1String("http") && !localHost)
        return m_httpProxy;
    if (protocol == QLatin1String("https") && !localHost)
        return m_httpsProxy;

    QList<QNetworkProxy> proxies;
    proxies << QNetworkProxy::NoProxy;
    return proxies;
}

static void initializeProxy()
{
    QList<QNetworkProxy> proxylist = QNetworkProxyFactory::systemProxyForQuery();
    if (proxylist.count() == 1) {
        QNetworkProxy proxy = proxylist.first();
        if (proxy == QNetworkProxy::NoProxy || proxy == QNetworkProxy::DefaultProxy) {
            auto proxyFactory = std::make_unique<EnvHttpProxyFactory>();
            if (proxyFactory->initializeFromEnvironment()) {
                QNetworkProxyFactory::setApplicationProxyFactory(proxyFactory.release());
                return;
            }
        }
    }
    QNetworkProxyFactory::setUseSystemConfiguration(true);
}

class NetworkProcessMain final: public ChildProcessMainBase {
public:

    bool platformInitialize() final
    {
        initializeProxy();
        return true;
    }
};

Q_DECL_EXPORT int NetworkProcessMainQt(int argc, char** argv)
{
    QCoreApplication app(argc, argv);
    return ChildProcessMain<NetworkProcess, ChildProcessMainBase>(argc, argv);
}

} // namespace WebKit
