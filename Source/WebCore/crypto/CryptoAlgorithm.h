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

#ifndef CryptoAlgorithm_h
#define CryptoAlgorithm_h

#include "CryptoAlgorithmIdentifier.h"
#include "CryptoKeyFormat.h"
#include "CryptoKeyUsage.h"
#include <wtf/Vector.h>

#if ENABLE(SUBTLE_CRYPTO)

namespace WebCore {

typedef int ExceptionCode;

class CryptoAlgorithmParameters;
class CryptoKey;
class PromiseWrapper;

// Data is mutable, so async operations should copy it first.
typedef std::pair<const char*, size_t> CryptoOperationData;

class CryptoAlgorithm {
    WTF_MAKE_NONCOPYABLE(CryptoAlgorithm)
public:
    virtual ~CryptoAlgorithm();

    virtual CryptoAlgorithmIdentifier identifier() const = 0;

    virtual void encrypt(const CryptoAlgorithmParameters&, const CryptoKey&, const Vector<CryptoOperationData>&, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void decrypt(const CryptoAlgorithmParameters&, const CryptoKey&, const Vector<CryptoOperationData>&, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void sign(const CryptoAlgorithmParameters&, const CryptoKey&, const Vector<CryptoOperationData>&, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void verify(const CryptoAlgorithmParameters&, const CryptoKey&, const CryptoOperationData& signature, const Vector<CryptoOperationData>& data, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void digest(const CryptoAlgorithmParameters&, const Vector<CryptoOperationData>&, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void generateKey(const CryptoAlgorithmParameters&, bool extractable, CryptoKeyUsage, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void deriveKey(const CryptoAlgorithmParameters&, const CryptoKey& baseKey, CryptoAlgorithm* derivedKeyType, bool extractable, CryptoKeyUsage, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void deriveBits(const CryptoAlgorithmParameters&, const CryptoKey& baseKey, unsigned long length, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void importKey(const CryptoAlgorithmParameters&, CryptoKeyFormat, const CryptoOperationData&, bool extractable, CryptoKeyUsage, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void exportKey(const CryptoAlgorithmParameters&, CryptoKeyFormat, const CryptoKey&, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void wrapKey(const CryptoAlgorithmParameters&, CryptoKeyFormat, const CryptoKey&, const CryptoKey& wrappingKey, CryptoAlgorithm* wrapAlgorithm, std::unique_ptr<PromiseWrapper>, ExceptionCode&);
    virtual void unwrapKey(const CryptoAlgorithmParameters&, CryptoKeyFormat, const CryptoOperationData&, const CryptoKey& unwrappingKey, CryptoAlgorithm* unwrapAlgorithm, CryptoAlgorithm* unwrappedKeyAlgorithm, bool extractable, CryptoKeyUsage, std::unique_ptr<PromiseWrapper>, ExceptionCode&);

protected:
    CryptoAlgorithm();
};

}

#endif // ENABLE(SUBTLE_CRYPTO)
#endif // CryptoAlgorithm_h
