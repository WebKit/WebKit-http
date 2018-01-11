/*
 * Copyright (C) 2016 Konstantin Tokavev <annulen@yandex.ru>
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
#include "CryptoDigest.h"

#include <QCryptographicHash>
#include <QDebug>

namespace WebCore {

static QCryptographicHash::Algorithm toQtAlgorithm(CryptoDigest::Algorithm algorithm)
{
    switch (algorithm) {
    case CryptoDigest::Algorithm::SHA_1:
        return QCryptographicHash::Sha1;

    case CryptoDigest::Algorithm::SHA_224:
        return QCryptographicHash::Sha224;

    case CryptoDigest::Algorithm::SHA_256:
        return QCryptographicHash::Sha256;

    case CryptoDigest::Algorithm::SHA_384:
        return QCryptographicHash::Sha384;

    case CryptoDigest::Algorithm::SHA_512:
        return QCryptographicHash::Sha512;
    }

    ASSERT_NOT_REACHED();
    return QCryptographicHash::Algorithm();
}

struct CryptoDigestContext {
    CryptoDigestContext(QCryptographicHash::Algorithm algorithm)
        : hash(algorithm)
    {
    }
    QCryptographicHash hash;
};

CryptoDigest::CryptoDigest()
{
}

CryptoDigest::~CryptoDigest()
{
}

std::unique_ptr<CryptoDigest> CryptoDigest::create(CryptoDigest::Algorithm algorithm)
{
    std::unique_ptr<CryptoDigest> digest(new CryptoDigest);
    digest->m_context.reset(new CryptoDigestContext(toQtAlgorithm(algorithm)));
    return digest;
}

void CryptoDigest::addBytes(const void* input, size_t length)
{
    m_context->hash.addData(static_cast<const char*>(input), length);
}

Vector<uint8_t> CryptoDigest::computeHash()
{
    QByteArray digest = m_context->hash.result();
    Vector<uint8_t> result(digest.size());
    memcpy(result.data(), digest.constData(), digest.size());
    return result;
}

} // namespace WebCore
