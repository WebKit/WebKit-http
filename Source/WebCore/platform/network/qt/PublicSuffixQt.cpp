/*
 * Copyright (C) 2019 Konstantin Tokarev <annulen@yandex.ru>
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
#include "PublicSuffix.h"

#if ENABLE(PUBLIC_SUFFIX_LIST)

#include <QVector>
#include <private/qtldurl_p.h>
#include <private/qurl_p.h>
#include <wtf/URL.h>

namespace WebCore {

static inline QByteArray asciiStringToByteArrayNoCopy(const String& string)
{
    ASSERT(string.is8Bit());
    return QByteArray::fromRawData(reinterpret_cast<const char*>(string.characters8()), string.length());
}

static QString fromAce(const String& domain)
{
    if (domain.is8Bit())
        return QUrl::fromAce(asciiStringToByteArrayNoCopy(domain.convertToASCIILowercase()));
    return QString(domain).toLower();
}

bool isPublicSuffix(const String& domain)
{
    if (domain.isEmpty())
        return false;

    return qIsEffectiveTLD(fromAce(domain));
}

String topPrivatelyControlledDomain(const String& domain)
{
    if (domain.isEmpty())
        return String();
    if (URL::hostIsIPAddress(domain) || !domain.isAllASCII())
        return domain;

    String lowercaseDomain = domain.convertToASCIILowercase();
    if (lowercaseDomain == "localhost")
        return lowercaseDomain;

    QString qLowercaseDomain = lowercaseDomain;
    if (qIsEffectiveTLD(qLowercaseDomain))
        return String();

    QString tld = qTopLevelDomain(qLowercaseDomain);
    auto privateLabels = qLowercaseDomain.leftRef(qLowercaseDomain.length() - tld.length());
    auto topPrivateLabel = privateLabels.split(QLatin1Char('.'), QString::SkipEmptyParts).last();
    return QString(topPrivateLabel + tld);
}

} // namespace WebCore

#endif // ENABLE(PUBLIC_SUFFIX_LIST)
