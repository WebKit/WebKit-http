/*
 * Copyright (C) 2013 Apple Inc. All rights reserved.
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

#ifndef CryptoKey_h
#define CryptoKey_h

#include "CryptoAlgorithmIdentifier.h"
#include "CryptoKeyType.h"
#include "CryptoKeyUsage.h"
#include <wtf/Forward.h>
#include <wtf/RefCounted.h>
#include <wtf/Vector.h>

#if ENABLE(SUBTLE_CRYPTO)

namespace WebCore {

class CryptoAlgorithmDescriptionBuilder;

ENUM_CLASS(CryptoKeyClass) {
    HMAC,
    AES
};

class CryptoKey : public RefCounted<CryptoKey> {
public:
    CryptoKey(CryptoAlgorithmIdentifier, CryptoKeyType, bool extractable, CryptoKeyUsage);
    virtual ~CryptoKey();

    virtual CryptoKeyClass keyClass() const = 0;

    String type() const;
    bool extractable() const { return m_extractable; }
    virtual void buildAlgorithmDescription(CryptoAlgorithmDescriptionBuilder&) const;
    Vector<String> usages() const;

    bool allows(CryptoKeyUsage usage) const { return usage == (m_usages & usage); }

private:
    CryptoAlgorithmIdentifier m_algorithm;
    CryptoKeyType m_type;
    bool m_extractable;
    CryptoKeyUsage m_usages;
};

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
#endif // CryptoKey_h
