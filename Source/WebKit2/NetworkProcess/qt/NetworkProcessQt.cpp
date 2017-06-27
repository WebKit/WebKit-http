/*
 * Copyright (C) 2016 Konstantin Tokarev <annulen@yandex.ru>
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
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "NetworkProcess.h"

#include "NetworkProcessCreationParameters.h"

#include <QNetworkDiskCache>
#include <WebCore/CertificateInfo.h>
#include <WebCore/CookieJarQt.h>

using namespace WebCore;

namespace WebKit {

void NetworkProcess::platformInitializeNetworkProcess(const NetworkProcessCreationParameters& parameters)
{
    if (!parameters.cookiePersistentStoragePath.isEmpty()) {
        WebCore::SharedCookieJarQt* jar = WebCore::SharedCookieJarQt::create(parameters.cookiePersistentStoragePath);
        m_networkAccessManager.setCookieJar(jar);
        // Do not let QNetworkAccessManager delete the jar.
        jar->setParent(0);
    }

    if (!parameters.diskCacheDirectory.isEmpty()) {
        QNetworkDiskCache* diskCache = new QNetworkDiskCache();
        diskCache->setCacheDirectory(parameters.diskCacheDirectory);
        // The m_networkAccessManager takes ownership of the diskCache object upon the following call.
        m_networkAccessManager.setCache(diskCache);
    }
}

void NetworkProcess::platformTerminate()
{
}

void NetworkProcess::allowSpecificHTTPSCertificateForHost(const CertificateInfo&, const String&)
{
}

void NetworkProcess::clearCacheForAllOrigins(uint32_t)
{
}

void NetworkProcess::clearDiskCache(std::chrono::system_clock::time_point, std::function<void()>)
{
}

void NetworkProcess::platformSetCacheModel(CacheModel)
{
}

} // namespace WebKit
