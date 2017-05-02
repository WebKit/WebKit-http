/*
 * Copyright (C) 2014 Igalia S.L. All rights reserved.
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
#include "CryptoKeyRSA.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoAlgorithmRegistry.h"
#include "CryptoKeyDataRSAComponents.h"
#include "CryptoKeyPair.h"
#include "ExceptionCode.h"
#include "ScriptExecutionContext.h"
#include <pal/crypto/gcrypt/ASN1.h>
#include <pal/crypto/gcrypt/Handle.h>
#include <pal/crypto/gcrypt/Utilities.h>

namespace WebCore {

// Retrieve size of the public modulus N of the given RSA key, in bits.
static size_t getRSAModulusLength(gcry_sexp_t sexp)
{
    // The s-expression is of roughly the following form:
    // (private-key|public-key
    //   (rsa
    //     (n n-mpi)
    //     (e e-mpi)
    //     ...))
    PAL::GCrypt::Handle<gcry_sexp_t> nSexp(gcry_sexp_find_token(sexp, "n", 0));
    if (!nSexp)
        return 0;

    PAL::GCrypt::Handle<gcry_mpi_t> nMPI(gcry_sexp_nth_mpi(nSexp, 1, GCRYMPI_FMT_USG));
    if (!nMPI)
        return 0;

    size_t dataLength = 0;
    gcry_error_t error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, nMPI);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return 0;
    }

    return dataLength * 8;
}

static Vector<uint8_t> getParameterMPIData(gcry_mpi_t paramMPI)
{
    size_t dataLength = 0;
    gcry_error_t error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, paramMPI);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return { };
    }

    Vector<uint8_t> parameter(dataLength);
    error = gcry_mpi_print(GCRYMPI_FMT_USG, parameter.data(), parameter.size(), nullptr, paramMPI);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return { };
    }

    return parameter;
}

static Vector<uint8_t> getRSAKeyParameter(gcry_sexp_t sexp, const char* name)
{
    PAL::GCrypt::Handle<gcry_sexp_t> paramSexp(gcry_sexp_find_token(sexp, name, 0));
    if (!paramSexp)
        return { };

    PAL::GCrypt::Handle<gcry_mpi_t> paramMPI(gcry_sexp_nth_mpi(paramSexp, 1, GCRYMPI_FMT_USG));
    if (!paramMPI)
        return { };

    return getParameterMPIData(paramMPI);
}

RefPtr<CryptoKeyRSA> CryptoKeyRSA::create(CryptoAlgorithmIdentifier identifier, CryptoAlgorithmIdentifier hash, bool hasHash, const CryptoKeyDataRSAComponents& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    // When creating a private key, we require the p and q prime information.
    if (keyData.type() == CryptoKeyDataRSAComponents::Type::Private && !keyData.hasAdditionalPrivateKeyParameters())
        return nullptr;

    // But we don't currently support creating keys with any additional prime information.
    if (!keyData.otherPrimeInfos().isEmpty())
        return nullptr;

    // Validate the key data.
    {
        bool valid = true;

        // For both public and private keys, we need the public modulus and exponent.
        valid &= !keyData.modulus().isEmpty() && !keyData.exponent().isEmpty();

        // For private keys, we require the private exponent, as well as p and q prime information.
        if (keyData.type() == CryptoKeyDataRSAComponents::Type::Private)
            valid &= !keyData.privateExponent().isEmpty() && !keyData.firstPrimeInfo().primeFactor.isEmpty() && !keyData.secondPrimeInfo().primeFactor.isEmpty();

        if (!valid)
            return nullptr;
    }

    CryptoKeyType keyType;
    switch (keyData.type()) {
    case CryptoKeyDataRSAComponents::Type::Public:
        keyType = CryptoKeyType::Public;
        break;
    case CryptoKeyDataRSAComponents::Type::Private:
        keyType = CryptoKeyType::Private;
        break;
    }

    // Construct the key s-expression, using the data that's available.
    PAL::GCrypt::Handle<gcry_sexp_t> keySexp;
    {
        gcry_error_t error = GPG_ERR_NO_ERROR;

        switch (keyType) {
        case CryptoKeyType::Public:
            error = gcry_sexp_build(&keySexp, nullptr, "(public-key(rsa(n %b)(e %b)))",
                keyData.modulus().size(), keyData.modulus().data(),
                keyData.exponent().size(), keyData.exponent().data());
            break;
        case CryptoKeyType::Private:
            if (keyData.hasAdditionalPrivateKeyParameters()) {
                error = gcry_sexp_build(&keySexp, nullptr, "(private-key(rsa(n %b)(e %b)(d %b)(p %b)(q %b)))",
                    keyData.modulus().size(), keyData.modulus().data(),
                    keyData.exponent().size(), keyData.exponent().data(),
                    keyData.privateExponent().size(), keyData.privateExponent().data(),
                    keyData.secondPrimeInfo().primeFactor.size(), keyData.secondPrimeInfo().primeFactor.data(),
                    keyData.firstPrimeInfo().primeFactor.size(), keyData.firstPrimeInfo().primeFactor.data());
                break;
            }

            error = gcry_sexp_build(&keySexp, nullptr, "(private-key(rsa(n %b)(e %b)(d %b)))",
                keyData.modulus().size(), keyData.modulus().data(),
                keyData.exponent().size(), keyData.exponent().data(),
                keyData.privateExponent().size(), keyData.privateExponent().data());
            break;
        case CryptoKeyType::Secret:
            ASSERT_NOT_REACHED();
            return nullptr;
        }

        if (error != GPG_ERR_NO_ERROR) {
            PAL::GCrypt::logError(error);
            return nullptr;
        }
    }

    return adoptRef(new CryptoKeyRSA(identifier, hash, hasHash, keyType, keySexp.release(), extractable, usages));
}

CryptoKeyRSA::CryptoKeyRSA(CryptoAlgorithmIdentifier identifier, CryptoAlgorithmIdentifier hash, bool hasHash, CryptoKeyType type, PlatformRSAKey platformKey, bool extractable, CryptoKeyUsageBitmap usage)
    : CryptoKey(identifier, type, extractable, usage)
    , m_platformKey(platformKey)
    , m_restrictedToSpecificHash(hasHash)
    , m_hash(hash)
{
}

CryptoKeyRSA::~CryptoKeyRSA()
{
    if (m_platformKey)
        PAL::GCrypt::HandleDeleter<gcry_sexp_t>()(m_platformKey);
}

bool CryptoKeyRSA::isRestrictedToHash(CryptoAlgorithmIdentifier& identifier) const
{
    if (!m_restrictedToSpecificHash)
        return false;

    identifier = m_hash;
    return true;
}

size_t CryptoKeyRSA::keySizeInBits() const
{
    return getRSAModulusLength(m_platformKey);
}

// Convert the exponent vector to a 32-bit value, if possible.
static std::optional<uint32_t> exponentVectorToUInt32(const Vector<uint8_t>& exponent)
{
    if (exponent.size() > 4) {
        if (std::any_of(exponent.begin(), exponent.end() - 4, [](uint8_t element) { return !!element; }))
            return std::nullopt;
    }

    uint32_t result = 0;
    for (size_t size = exponent.size(), i = std::min<size_t>(4, size); i > 0; --i) {
        result <<= 8;
        result += exponent[size - i];
    }

    return result;
}

void CryptoKeyRSA::generatePair(CryptoAlgorithmIdentifier algorithm, CryptoAlgorithmIdentifier hash, bool hasHash, unsigned modulusLength, const Vector<uint8_t>& publicExponent, bool extractable, CryptoKeyUsageBitmap usage, KeyPairCallback&& callback, VoidCallback&& failureCallback, ScriptExecutionContext* context)
{
    // libgcrypt doesn't report an error if the exponent is smaller than three or even.
    auto e = exponentVectorToUInt32(publicExponent);
    if (!e || *e < 3 || !(*e & 0x1)) {
        failureCallback();
        return;
    }

    // libgcrypt doesn't support generating primes of less than 16 bits.
    if (modulusLength < 16) {
        failureCallback();
        return;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> genkeySexp;
    gcry_error_t error = gcry_sexp_build(&genkeySexp, nullptr, "(genkey(rsa(nbits %d)(rsa-use-e %d)))", modulusLength, *e);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        failureCallback();
        return;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> keyPairSexp;
    error = gcry_pk_genkey(&keyPairSexp, genkeySexp);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        failureCallback();
        return;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> publicKeySexp(gcry_sexp_find_token(keyPairSexp, "public-key", 0));
    PAL::GCrypt::Handle<gcry_sexp_t> privateKeySexp(gcry_sexp_find_token(keyPairSexp, "private-key", 0));
    if (!publicKeySexp || !privateKeySexp) {
        failureCallback();
        return;
    }

    context->ref();
    context->postTask(
        [algorithm, hash, hasHash, extractable, usage, publicKeySexp = publicKeySexp.release(), privateKeySexp = privateKeySexp.release(), callback = WTFMove(callback)](ScriptExecutionContext& context) mutable {
            auto publicKey = CryptoKeyRSA::create(algorithm, hash, hasHash, CryptoKeyType::Public, publicKeySexp, true, usage);
            auto privateKey = CryptoKeyRSA::create(algorithm, hash, hasHash, CryptoKeyType::Private, privateKeySexp, extractable, usage);

            callback(CryptoKeyPair { WTFMove(publicKey), WTFMove(privateKey) });
            context.deref();
        });
}

enum class RSAAlgorithm {
    RSAEncryption,
    RSAES_OAEP,
    RSASSA_PSS,
};

static std::optional<RSAAlgorithm> algorithmForIdentifier(const uint8_t* data, size_t size)
{
    static const std::array<uint8_t, 9> s_rsaEncryption{ { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } };
    static const std::array<uint8_t, 9> s_idRSAES_OAEP{ { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x07 } };
    static const std::array<uint8_t, 9> s_idRSASSA_PSS{ { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0a } };

    if (size != 9)
        return std::nullopt;

    if (!std::memcmp(data, s_rsaEncryption.data(), size))
        return RSAAlgorithm::RSAEncryption;
    if (!std::memcmp(data, s_idRSAES_OAEP.data(), size))
        return std::nullopt;
    if (!std::memcmp(data, s_idRSASSA_PSS.data(), size))
        return std::nullopt;
    return std::nullopt;
}

RefPtr<CryptoKeyRSA> CryptoKeyRSA::importSpki(CryptoAlgorithmIdentifier identifier, std::optional<CryptoAlgorithmIdentifier> hash, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    // SubjectPublicKeyInfo  ::=  SEQUENCE  {
    //   algorithm            AlgorithmIdentifier,
    //   subjectPublicKey     BIT STRING  }
    auto subjectPublicKeyInfoTLV = ASN1::parseTLV(keyData, 0);
    if (!subjectPublicKeyInfoTLV.bufferSize || !subjectPublicKeyInfoTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    // AlgorithmIdentifier  ::=  SEQUENCE  {
    //    algorithm   OBJECT IDENTIFIER,
    //    parameters  ANY DEFINED BY algorithm OPTIONAL
    // }
    auto algorithmIdentifierTLV = ASN1::parseTLV(keyData, subjectPublicKeyInfoTLV.valueOffset());
    if (!algorithmIdentifierTLV.bufferSize || !algorithmIdentifierTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    auto algorithmTLV = ASN1::parseTLV(keyData, algorithmIdentifierTLV.valueOffset());
    if (!algorithmTLV.bufferSize || !algorithmTLV.hasTag(ASN1::Tag::ObjectIdentifier))
        return nullptr;

    auto supportedAlgorithm = algorithmForIdentifier(keyData.data() + algorithmTLV.valueOffset(), algorithmTLV.length);
    if (!supportedAlgorithm)
        return nullptr;

    auto subjectPublicKeyTLV = ASN1::parseTLV(keyData,
        subjectPublicKeyInfoTLV.valueOffset() + algorithmIdentifierTLV.bufferSize);
    if (!subjectPublicKeyTLV.bufferSize || !subjectPublicKeyTLV.hasTag(ASN1::Tag::BitString)
        || keyData[subjectPublicKeyTLV.valueOffset()] != 0x00)
        return nullptr;

    // RSAPublicKey ::= SEQUENCE {
    //   modulus           INTEGER,  -- n
    //   publicExponent    INTEGER   -- e
    // }
    auto rsaPublicKeyTLV = ASN1::parseTLV(keyData, subjectPublicKeyTLV.valueOffset() + 1);
    if (!rsaPublicKeyTLV.bufferSize || !rsaPublicKeyTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    ASN1::TLV modulusTLV;
    ASN1::TLV publicExponentTLV;
    {
        size_t offset = rsaPublicKeyTLV.valueOffset();
        modulusTLV = ASN1::parseTLV(keyData, offset);
        if (!modulusTLV.bufferSize || !modulusTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += modulusTLV.bufferSize;
        publicExponentTLV = ASN1::parseTLV(keyData, offset);
        if (!publicExponentTLV.bufferSize || !publicExponentTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        if (modulusTLV.bufferSize + publicExponentTLV.bufferSize != rsaPublicKeyTLV.length)
            return nullptr;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(public-key(rsa(n %b)(e %b)))",
        modulusTLV.length, keyData.data() + modulusTLV.valueOffset(),
        publicExponentTLV.length, keyData.data() + publicExponentTLV.valueOffset());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return adoptRef(new CryptoKeyRSA(identifier, hash.value_or(CryptoAlgorithmIdentifier::SHA_1), !!hash, CryptoKeyType::Public, platformKey.release(), extractable, usages));;
}

RefPtr<CryptoKeyRSA> CryptoKeyRSA::importPkcs8(CryptoAlgorithmIdentifier identifier, std::optional<CryptoAlgorithmIdentifier> hash, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    // PrivateKeyInfo ::= SEQUENCE {
    //   version                   Version,
    //   privateKeyAlgorithm       PrivateKeyAlgorithmIdentifier,
    //   privateKey                PrivateKey,
    //   attributes           [0]  IMPLICIT Attributes OPTIONAL }
    auto privateKeyInfoTLV = ASN1::parseTLV(keyData, 0);
    if (!privateKeyInfoTLV.bufferSize || !privateKeyInfoTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;
    if (privateKeyInfoTLV.bufferSize != keyData.size())
        return nullptr;

    // Version ::= INTEGER
    auto versionTLV = ASN1::parseTLV(keyData, privateKeyInfoTLV.valueOffset());
    if (!versionTLV.bufferSize || !versionTLV.hasTag(ASN1::Tag::Integer))
        return nullptr;
    if (versionTLV.length != 1 && keyData[versionTLV.valueOffset()] != 0)
        return nullptr;

    // PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
    // AlgorithmIdentifier  ::=  SEQUENCE  {
    //    algorithm   OBJECT IDENTIFIER,
    //    parameters  ANY DEFINED BY algorithm OPTIONAL
    // }
    auto privateKeyAlgorithmTLV = ASN1::parseTLV(keyData,
        privateKeyInfoTLV.valueOffset() + versionTLV.bufferSize);
    if (!privateKeyAlgorithmTLV.bufferSize || !privateKeyAlgorithmTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    auto algorithmTLV = ASN1::parseTLV(keyData, privateKeyAlgorithmTLV.valueOffset());
    if (!algorithmTLV.bufferSize || !algorithmTLV.hasTag(ASN1::Tag::ObjectIdentifier))
        return nullptr;

    auto supportedAlgorithm = algorithmForIdentifier(keyData.data() + algorithmTLV.valueOffset(), algorithmTLV.length);
    if (!supportedAlgorithm)
        return nullptr;

    // PrivateKey ::= OCTET STRING
    auto privateKeyTLV = ASN1::parseTLV(keyData,
        privateKeyInfoTLV.valueOffset() + versionTLV.bufferSize + privateKeyAlgorithmTLV.bufferSize);
    if (!privateKeyTLV.bufferSize || !privateKeyTLV.hasTag(ASN1::Tag::OctetString))
        return nullptr;

    // RSAPrivateKey ::= SEQUENCE {
    //     version           Version,
    //     modulus           INTEGER,  -- n
    //     publicExponent    INTEGER,  -- e
    //     privateExponent   INTEGER,  -- d
    //     prime1            INTEGER,  -- p
    //     prime2            INTEGER,  -- q
    //     exponent1         INTEGER,  -- d mod (p-1)
    //     exponent2         INTEGER,  -- d mod (q-1)
    //     coefficient       INTEGER,  -- (inverse of q) mod p
    //     otherPrimeInfos   OtherPrimeInfos OPTIONAL
    // }
    auto rsaPrivateKeyTLV = ASN1::parseTLV(keyData, privateKeyTLV.valueOffset());
    if (!rsaPrivateKeyTLV.bufferSize || !rsaPrivateKeyTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    ASN1::TLV modulusTLV;
    ASN1::TLV publicExponentTLV;
    ASN1::TLV privateExponentTLV;
    ASN1::TLV prime1TLV;
    ASN1::TLV prime2TLV;
    ASN1::TLV exponent1TLV;
    ASN1::TLV exponent2TLV;
    ASN1::TLV coefficientTLV;
    {
        size_t offset = rsaPrivateKeyTLV.valueOffset();
        auto versionTLV = ASN1::parseTLV(keyData, offset);
        if (!versionTLV.bufferSize || !versionTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;
        if (versionTLV.length != 1 || keyData[versionTLV.valueOffset()] != 0)
            return nullptr;

        offset += versionTLV.bufferSize;
        modulusTLV = ASN1::parseTLV(keyData, offset);
        if (!modulusTLV.bufferSize || !modulusTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += modulusTLV.bufferSize;
        publicExponentTLV = ASN1::parseTLV(keyData, offset);
        if (!publicExponentTLV.bufferSize || !publicExponentTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += publicExponentTLV.bufferSize;
        privateExponentTLV = ASN1::parseTLV(keyData, offset);
        if (!privateExponentTLV.bufferSize || !privateExponentTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += privateExponentTLV.bufferSize;
        prime1TLV = ASN1::parseTLV(keyData, offset);
        if (!prime1TLV.bufferSize || !prime1TLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += prime1TLV.bufferSize;
        prime2TLV = ASN1::parseTLV(keyData, offset);
        if (!prime2TLV.bufferSize || !prime2TLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += prime2TLV.bufferSize;
        exponent1TLV = ASN1::parseTLV(keyData, offset);
        if (!exponent1TLV.bufferSize || !exponent1TLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += exponent1TLV.bufferSize;
        exponent2TLV = ASN1::parseTLV(keyData, offset);
        if (!exponent2TLV.bufferSize || !exponent2TLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        offset += exponent2TLV.bufferSize;
        coefficientTLV = ASN1::parseTLV(keyData, offset);
        if (!coefficientTLV.bufferSize || !coefficientTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;

        // TODO: support for OtherPrimeInfos.

        size_t bufferSizeSum = versionTLV.bufferSize + modulusTLV.bufferSize + publicExponentTLV.bufferSize + privateExponentTLV.bufferSize
            + prime1TLV.bufferSize + prime2TLV.bufferSize + exponent1TLV.bufferSize + exponent2TLV.bufferSize + coefficientTLV.bufferSize;
        if (bufferSizeSum != rsaPrivateKeyTLV.length)
            return nullptr;
    }

    Vector<uint8_t> uData;
    {
        PAL::GCrypt::Handle<gcry_mpi_t> pMPI;
        gcry_error_t error = gcry_mpi_scan(&pMPI, GCRYMPI_FMT_USG, keyData.data() + prime1TLV.valueOffset(), prime1TLV.length, nullptr);
        if (error != GPG_ERR_NO_ERROR)
            return nullptr;

        PAL::GCrypt::Handle<gcry_mpi_t> qMPI;
        error = gcry_mpi_scan(&qMPI, GCRYMPI_FMT_USG, keyData.data() + prime2TLV.valueOffset(), prime2TLV.length, nullptr);
        if (error != GPG_ERR_NO_ERROR)
            return nullptr;

        PAL::GCrypt::Handle<gcry_mpi_t> qiMPI(gcry_mpi_set_ui(nullptr, 0));
        gcry_mpi_invm(qiMPI, qMPI, pMPI);
        uData = getParameterMPIData(qiMPI);
    }

    PAL::GCrypt::Handle<gcry_sexp_t> keySexp;
    gcry_error_t error = gcry_sexp_build(&keySexp, nullptr, "(private-key(rsa(n %b)(e %b)(d %b)(p %b)(q %b)(u %b)))",
        modulusTLV.length, keyData.data() + modulusTLV.valueOffset(),
        publicExponentTLV.length, keyData.data() + publicExponentTLV.valueOffset(),
        privateExponentTLV.length, keyData.data() + privateExponentTLV.valueOffset(),
        prime2TLV.length, keyData.data() + prime2TLV.valueOffset(),
        prime1TLV.length, keyData.data() + prime1TLV.valueOffset(),
        uData.size(), uData.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return adoptRef(new CryptoKeyRSA(identifier, hash.value_or(CryptoAlgorithmIdentifier::SHA_1), !!hash, CryptoKeyType::Private, keySexp.release(), extractable, usages));
}

ExceptionOr<Vector<uint8_t>> CryptoKeyRSA::exportSpki() const
{
    if (type() != CryptoKeyType::Public)
        return Exception{ INVALID_ACCESS_ERR };

    auto appendLength =
        [](size_t length, Vector<uint8_t>& result) {
            if (length > 127) {
                uint8_t lengthBytes = std::ceil(std::log2(length + 1) / 8);
                result.append(0x80 | lengthBytes);
                for (uint8_t i = 0; i < lengthBytes; ++i)
                    result.append((length >> ((lengthBytes - i - 1) * 8) & 0xff));
            } else
                result.append(length);
        };

    // SubjectPublicKeyInfo  ::=  SEQUENCE  {
    //   algorithm            AlgorithmIdentifier,
    //   subjectPublicKey     BIT STRING  }


    // AlgorithmIdentifier  ::=  SEQUENCE  {
    //    algorithm   OBJECT IDENTIFIER,
    //    parameters  ANY DEFINED BY algorithm OPTIONAL
    // }
    Vector<uint8_t> algorithmIdentifierData;
    {
        static const std::array<uint8_t, 9> s_rsaEncryption{ { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } };

        algorithmIdentifierData.append(0x30);
        algorithmIdentifierData.append(2 + s_rsaEncryption.size() + 2);

        // algorithm
        algorithmIdentifierData.append(0x06);
        algorithmIdentifierData.append(s_rsaEncryption.size());
        algorithmIdentifierData.append(s_rsaEncryption.data(), s_rsaEncryption.size());

        // parameters
        algorithmIdentifierData.append(0x05);
        algorithmIdentifierData.append(0x00);
    }

    Vector<uint8_t> subjectPublicKeyData;
    {
        // RSAPublicKey ::= SEQUENCE {
        //   modulus           INTEGER,  -- n
        //   publicExponent    INTEGER   -- e
        // }
        Vector<uint8_t> rsaPublicKeyData;
        {
            auto parameterData =
                [&appendLength](gcry_sexp_t sexp, const char* name) -> Vector<uint8_t> {
                    Vector<uint8_t> parameter = getRSAKeyParameter(sexp, name);
                    bool negative = parameter[0] & 0x80;

                    Vector<uint8_t> data;
                    data.append(0x02);
                    appendLength(parameter.size() + (negative ? 1 : 0), data);
                    if (negative)
                        data.append(0x00);
                    data.appendVector(parameter);
                    return data;
                };

            auto nData = parameterData(m_platformKey, "n");
            auto eData = parameterData(m_platformKey, "e");

            rsaPublicKeyData.append(0x30);
            appendLength(nData.size() + eData.size(), rsaPublicKeyData);

            // modulus
            rsaPublicKeyData.appendVector(nData);
            // publicExponent
            rsaPublicKeyData.appendVector(eData);
        }

        subjectPublicKeyData.append(0x03);
        appendLength(1 + rsaPublicKeyData.size(), subjectPublicKeyData);
        subjectPublicKeyData.append(0x00);
        subjectPublicKeyData.appendVector(rsaPublicKeyData);
    }

    Vector<uint8_t> result;
    result.append(0x30);
    appendLength(algorithmIdentifierData.size() + subjectPublicKeyData.size(), result);
    result.appendVector(algorithmIdentifierData);
    result.appendVector(subjectPublicKeyData);

    return WTFMove(result);
}

ExceptionOr<Vector<uint8_t>> CryptoKeyRSA::exportPkcs8() const
{
    if (type() != CryptoKeyType::Private)
        return Exception{ INVALID_ACCESS_ERR };

    auto appendLength =
        [](size_t length, Vector<uint8_t>& result) {
            if (length > 127) {
                uint8_t lengthBytes = std::ceil(std::log2(length + 1) / 8);
                result.append(0x80 | lengthBytes);
                for (uint8_t i = 0; i < lengthBytes; ++i)
                    result.append((length >> ((lengthBytes - i - 1) * 8) & 0xff));
            } else
                result.append(length);
        };

    // PrivateKeyInfo ::= SEQUENCE {
    //   version                   Version,
    //   privateKeyAlgorithm       PrivateKeyAlgorithmIdentifier,
    //   privateKey                PrivateKey,
    //   attributes           [0]  IMPLICIT Attributes OPTIONAL }

    // Version ::= INTEGER
    static const std::array<uint8_t, 3> s_versionData{ { 0x02, 0x01, 0x00 } };


    // PrivateKeyAlgorithmIdentifier ::= AlgorithmIdentifier
    // AlgorithmIdentifier  ::=  SEQUENCE  {
    //    algorithm   OBJECT IDENTIFIER,
    //    parameters  ANY DEFINED BY algorithm OPTIONAL
    // }
    Vector<uint8_t> privateKeyAlgorithmData;
    {
        static const std::array<uint8_t, 9> s_rsaEncryption{ { 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x01 } };

        privateKeyAlgorithmData.append(0x30);
        privateKeyAlgorithmData.append(2 + s_rsaEncryption.size() + 2);

        privateKeyAlgorithmData.append(0x06);
        privateKeyAlgorithmData.append(s_rsaEncryption.size());
        privateKeyAlgorithmData.append(s_rsaEncryption.data(), s_rsaEncryption.size());

        privateKeyAlgorithmData.append(0x05);
        privateKeyAlgorithmData.append(0x00);
    }

    // PrivateKey ::= OCTET STRING
    Vector<uint8_t> privateKeyData;
    {
        // RSAPrivateKey ::= SEQUENCE {
        //     version           Version,
        //     modulus           INTEGER,  -- n
        //     publicExponent    INTEGER,  -- e
        //     privateExponent   INTEGER,  -- d
        //     prime1            INTEGER,  -- p
        //     prime2            INTEGER,  -- q
        //     exponent1         INTEGER,  -- d mod (p-1)
        //     exponent2         INTEGER,  -- d mod (q-1)
        //     coefficient       INTEGER,  -- (inverse of q) mod p
        //     otherPrimeInfos   OtherPrimeInfos OPTIONAL
        // }
        Vector<uint8_t> rsaPrivateKeyData;
        {
            auto parameterData =
                [&appendLength](gcry_sexp_t sexp, const char* name) -> Vector<uint8_t> {
                    Vector<uint8_t> parameter = getRSAKeyParameter(sexp, name);
                    bool negative = parameter[0] & 0x80;

                    Vector<uint8_t> data;
                    data.append(0x02);
                    appendLength(parameter.size() + (negative ? 1 : 0), data);
                    if (negative)
                        data.append(0x00);
                    data.appendVector(parameter);
                    return data;
                };

            auto nData = parameterData(m_platformKey, "n");
            auto eData = parameterData(m_platformKey, "e");
            auto dData = parameterData(m_platformKey, "d");
            auto pData = parameterData(m_platformKey, "q");
            auto qData = parameterData(m_platformKey, "p");

            Vector<uint8_t> dpData;
            Vector<uint8_t> dqData;
            Vector<uint8_t> qiData;
            {
                auto getMPI =
                    [](gcry_sexp_t sexp, const char* name) -> gcry_mpi_t {
                        PAL::GCrypt::Handle<gcry_sexp_t> paramSexp(gcry_sexp_find_token(sexp, name, 0));
                        if (!paramSexp)
                            return nullptr;
                        return gcry_sexp_nth_mpi(paramSexp, 1, GCRYMPI_FMT_USG);
                    };

                auto extractData =
                    [](gcry_mpi_t paramMPI) -> Vector<uint8_t> {
                        size_t dataLength = 0;
                        gcry_error_t error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, paramMPI);
                        if (error != GPG_ERR_NO_ERROR)
                            return { };

                        Vector<uint8_t> output(dataLength);
                        error = gcry_mpi_print(GCRYMPI_FMT_USG, output.data(), output.size(), nullptr, paramMPI);
                        if (error != GPG_ERR_NO_ERROR) {
                            PAL::GCrypt::logError(error);
                            return { };
                        }

                        return output;
                    };

                PAL::GCrypt::Handle<gcry_mpi_t> dMPI(getMPI(m_platformKey, "d"));
                PAL::GCrypt::Handle<gcry_mpi_t> pMPI(getMPI(m_platformKey, "q"));
                PAL::GCrypt::Handle<gcry_mpi_t> qMPI(getMPI(m_platformKey, "p"));

                // dp
                {
                    PAL::GCrypt::Handle<gcry_mpi_t> dpMPI(gcry_mpi_set_ui(nullptr, 0));
                    PAL::GCrypt::Handle<gcry_mpi_t> pm1MPI(gcry_mpi_set(nullptr, pMPI));
                    gcry_mpi_sub_ui(pm1MPI, pm1MPI, 1);
                    gcry_mpi_mod(dpMPI, dMPI, pm1MPI);

                    auto dp = extractData(dpMPI);
                    bool negative = dp[0] & 0x80;

                    dpData.append(0x02);
                    appendLength(dp.size() + (negative ? 1 : 0), dpData);
                    if (negative)
                        dpData.append(0x00);
                    dpData.appendVector(dp);
                }

                // dq
                {
                    PAL::GCrypt::Handle<gcry_mpi_t> dqMPI(gcry_mpi_set_ui(nullptr, 0));
                    PAL::GCrypt::Handle<gcry_mpi_t> qm1MPI(gcry_mpi_set(nullptr, qMPI));
                    gcry_mpi_sub_ui(qm1MPI, qm1MPI, 1);
                    gcry_mpi_mod(dqMPI, dMPI, qm1MPI);

                    auto dq = extractData(dqMPI);
                    bool negative = dq[0] & 0x80;

                    dqData.append(0x02);
                    appendLength(dq.size() + (negative ? 1 : 0), dqData);
                    if (negative)
                        dqData.append(0x00);
                    dqData.appendVector(dq);
                }

                // qi
                {
                    PAL::GCrypt::Handle<gcry_mpi_t> qiMPI(gcry_mpi_set_ui(nullptr, 0));
                    gcry_mpi_invm(qiMPI, qMPI, pMPI);

                    auto qi = extractData(qiMPI);
                    bool negative = qi[0] & 0x80;

                    qiData.append(0x02);
                    appendLength(qi.size() + (negative ? 1 : 0), qiData);
                    if (negative)
                        qiData.append(0x00);
                    qiData.appendVector(qi);
                }
            }

            rsaPrivateKeyData.append(0x30);
            appendLength(3 + nData.size() + eData.size() + dData.size() + pData.size() + qData.size()
                + dpData.size() + dqData.size() + qiData.size(), rsaPrivateKeyData);

            // version
            rsaPrivateKeyData.append(0x02);
            rsaPrivateKeyData.append(1);
            rsaPrivateKeyData.append(0x00);

            // modulus
            rsaPrivateKeyData.appendVector(nData);
            // publicExponent
            rsaPrivateKeyData.appendVector(eData);
            // privateExponent
            rsaPrivateKeyData.appendVector(dData);
            // prime1
            rsaPrivateKeyData.appendVector(pData);
            // prime2
            rsaPrivateKeyData.appendVector(qData);
            // exponent1
            rsaPrivateKeyData.appendVector(dpData);
            // exponent2
            rsaPrivateKeyData.appendVector(dqData);
            // coefficient
            rsaPrivateKeyData.appendVector(qiData);
        }

        privateKeyData.append(0x04);
        appendLength(rsaPrivateKeyData.size(), privateKeyData);
        privateKeyData.appendVector(rsaPrivateKeyData);
    }

    Vector<uint8_t> result;
    result.append(0x30);
    appendLength(s_versionData.size() + privateKeyAlgorithmData.size() + privateKeyData.size(), result);
    result.append(s_versionData.data(), s_versionData.size());
    result.appendVector(privateKeyAlgorithmData);
    result.appendVector(privateKeyData);

    return WTFMove(result);
}

std::unique_ptr<KeyAlgorithm> CryptoKeyRSA::buildAlgorithm() const
{
    String name = CryptoAlgorithmRegistry::singleton().name(algorithmIdentifier());
    size_t modulusLength = getRSAModulusLength(m_platformKey);
    Vector<uint8_t> publicExponent = getRSAKeyParameter(m_platformKey, "e");

    if (m_restrictedToSpecificHash)
        return std::make_unique<RsaHashedKeyAlgorithm>(name, modulusLength, WTFMove(publicExponent), CryptoAlgorithmRegistry::singleton().name(m_hash));
    return std::make_unique<RsaKeyAlgorithm>(name, modulusLength, WTFMove(publicExponent));
}

std::unique_ptr<CryptoKeyData> CryptoKeyRSA::exportData() const
{
    ASSERT(extractable());

    switch (type()) {
    case CryptoKeyType::Public:
        return CryptoKeyDataRSAComponents::createPublic(getRSAKeyParameter(m_platformKey, "n"), getRSAKeyParameter(m_platformKey, "e"));
    case CryptoKeyType::Private: {
        auto parameterMPI =
            [](gcry_sexp_t sexp, const char* name) -> gcry_mpi_t {
                PAL::GCrypt::Handle<gcry_sexp_t> paramSexp(gcry_sexp_find_token(sexp, name, 0));
                if (!paramSexp)
                    return nullptr;
                return gcry_sexp_nth_mpi(paramSexp, 1, GCRYMPI_FMT_USG);
            };

        PAL::GCrypt::Handle<gcry_mpi_t> dMPI(parameterMPI(m_platformKey, "d"));
        // libgcrypt internally uses p and q such that p < q, while usually it's q < p.
        // Switch the two primes here and continue with assuming the latter.
        PAL::GCrypt::Handle<gcry_mpi_t> pMPI(parameterMPI(m_platformKey, "q"));
        PAL::GCrypt::Handle<gcry_mpi_t> qMPI(parameterMPI(m_platformKey, "p"));
        if (!dMPI || !pMPI || !qMPI)
            return nullptr;

        CryptoKeyDataRSAComponents::PrimeInfo firstPrimeInfo { getParameterMPIData(pMPI), { }, { } };
        CryptoKeyDataRSAComponents::PrimeInfo secondPrimeInfo { getParameterMPIData(qMPI), { }, { } };

        // dp -- d mod (p - 1)
        {
            PAL::GCrypt::Handle<gcry_mpi_t> dpMPI(gcry_mpi_new(0));
            PAL::GCrypt::Handle<gcry_mpi_t> pm1MPI(gcry_mpi_new(0));
            gcry_mpi_sub_ui(pm1MPI, pMPI, 1);
            gcry_mpi_mod(dpMPI, dMPI, pm1MPI);
            firstPrimeInfo.factorCRTExponent = getParameterMPIData(dpMPI);
        }

        // dq -- d mod (q - 1)
        {
            PAL::GCrypt::Handle<gcry_mpi_t> dqMPI(gcry_mpi_new(0));
            PAL::GCrypt::Handle<gcry_mpi_t> qm1MPI(gcry_mpi_new(0));
            gcry_mpi_sub_ui(qm1MPI, qMPI, 1);
            gcry_mpi_mod(dqMPI, dMPI, qm1MPI);
            secondPrimeInfo.factorCRTExponent = getParameterMPIData(dqMPI);
        }

        // qi -- q^(-1) mod p
        {
            PAL::GCrypt::Handle<gcry_mpi_t> qiMPI(gcry_mpi_new(0));
            gcry_mpi_invm(qiMPI, qMPI, pMPI);
            secondPrimeInfo.factorCRTCoefficient = getParameterMPIData(qiMPI);
        }

        return CryptoKeyDataRSAComponents::createPrivateWithAdditionalData(getRSAKeyParameter(m_platformKey, "n"),
            getRSAKeyParameter(m_platformKey, "e"), getParameterMPIData(dMPI),
            WTFMove(firstPrimeInfo), WTFMove(secondPrimeInfo), Vector<CryptoKeyDataRSAComponents::PrimeInfo> { });
    }
    default:
        ASSERT_NOT_REACHED();
        return nullptr;
    }
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
