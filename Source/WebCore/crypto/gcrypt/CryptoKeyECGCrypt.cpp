/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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
#include "CryptoKeyEC.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "CryptoKeyPair.h"
#include "JsonWebKey.h"
#include <pal/crypto/gcrypt/ASN1.h>
#include <pal/crypto/gcrypt/Handle.h>
#include <pal/crypto/gcrypt/Utilities.h>
#include <wtf/text/Base64.h>

namespace WebCore {

static size_t curveSize(CryptoKeyEC::NamedCurve curve)
{
    switch (curve) {
    case CryptoKeyEC::NamedCurve::P256:
        return 256;
    case CryptoKeyEC::NamedCurve::P384:
        return 384;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static const char* curveName(CryptoKeyEC::NamedCurve curve)
{
    switch (curve) {
    case CryptoKeyEC::NamedCurve::P256:
        return "NIST P-256";
    case CryptoKeyEC::NamedCurve::P384:
        return "NIST P-384";
    }

    ASSERT_NOT_REACHED();
    return nullptr;
}

static unsigned uncompressedPointSizeForCurve(CryptoKeyEC::NamedCurve curve)
{
    switch (curve) {
    case CryptoKeyEC::NamedCurve::P256:
        return 65;
    case CryptoKeyEC::NamedCurve::P384:
        return 97;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static unsigned uncompressedFieldElementSizeForCurve(CryptoKeyEC::NamedCurve curve)
{
    switch (curve) {
    case CryptoKeyEC::NamedCurve::P256:
        return 32;
    case CryptoKeyEC::NamedCurve::P384:
        return 48;
    }

    ASSERT_NOT_REACHED();
    return 0;
}

static Vector<uint8_t> extractMPIData(gcry_mpi_t mpi)
{
    size_t dataLength = 0;
    gcry_error_t error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, mpi);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return { };
    }

    Vector<uint8_t> data(dataLength);
    error = gcry_mpi_print(GCRYMPI_FMT_USG, data.data(), data.size(), nullptr, mpi);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return { };
    }

    return data;
};

CryptoKeyEC::~CryptoKeyEC()
{
    if (m_platformKey)
        PAL::GCrypt::HandleDeleter<gcry_sexp_t>()(m_platformKey);
}

size_t CryptoKeyEC::keySizeInBits() const
{
    size_t size = curveSize(m_curve);
    ASSERT(size == gcry_pk_get_nbits(m_platformKey));
    return size;
}

std::optional<CryptoKeyPair> CryptoKeyEC::platformGeneratePair(CryptoAlgorithmIdentifier identifier, NamedCurve curve, bool extractable, CryptoKeyUsageBitmap usages)
{
    PAL::GCrypt::Handle<gcry_sexp_t> genkeySexp;
    gcry_error_t error = gcry_sexp_build(&genkeySexp, nullptr, "(genkey(ecc(curve %s)))", curveName(curve));
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return std::nullopt;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> keyPairSexp;
    error = gcry_pk_genkey(&keyPairSexp, genkeySexp);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return std::nullopt;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> publicKeySexp(gcry_sexp_find_token(keyPairSexp, "public-key", 0));
    PAL::GCrypt::Handle<gcry_sexp_t> privateKeySexp(gcry_sexp_find_token(keyPairSexp, "private-key", 0));
    if (!publicKeySexp || !privateKeySexp)
        return std::nullopt;

    auto publicKey = CryptoKeyEC::create(identifier, curve, CryptoKeyType::Public, publicKeySexp.release(), true, usages);
    auto privateKey = CryptoKeyEC::create(identifier, curve, CryptoKeyType::Private, privateKeySexp.release(), extractable, usages);
    return CryptoKeyPair { WTFMove(publicKey), WTFMove(privateKey) };
}

RefPtr<CryptoKeyEC> CryptoKeyEC::platformImportRaw(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    if (keyData.size() != uncompressedPointSizeForCurve(curve))
        return nullptr;

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(public-key(ecc(curve %s)(q %b)))",
        curveName(curve), keyData.size(), keyData.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return create(identifier, curve, CryptoKeyType::Public, platformKey.release(), extractable, usages);
}

RefPtr<CryptoKeyEC> CryptoKeyEC::platformImportJWKPublic(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& x, Vector<uint8_t>&& y, bool extractable, CryptoKeyUsageBitmap usages)
{
    unsigned uncompressedFieldElementSize = uncompressedFieldElementSizeForCurve(curve);
    if (x.size() != uncompressedFieldElementSize || y.size() != uncompressedFieldElementSize)
        return nullptr;

    // Construct the Vector that represents the EC point in uncompressed format.
    Vector<uint8_t> q;
    q.reserveInitialCapacity(1 + 2 * uncompressedFieldElementSize);
    q.append(0x04);
    q.appendVector(x);
    q.appendVector(y);

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(public-key(ecc(curve %s)(q %b)))",
        curveName(curve), q.size(), q.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return create(identifier, curve, CryptoKeyType::Public, platformKey.release(), extractable, usages);
}

RefPtr<CryptoKeyEC> CryptoKeyEC::platformImportJWKPrivate(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& x, Vector<uint8_t>&& y, Vector<uint8_t>&& d, bool extractable, CryptoKeyUsageBitmap usages)
{
    unsigned uncompressedFieldElementSize = uncompressedFieldElementSizeForCurve(curve);
    if (x.size() != uncompressedFieldElementSize || y.size() != uncompressedFieldElementSize || d.size() != uncompressedFieldElementSize)
        return nullptr;

    // Construct the Vector that represents the EC point in uncompressed format.
    Vector<uint8_t> q;
    q.reserveInitialCapacity(1 + 2 * uncompressedFieldElementSize);
    q.append(0x04);
    q.appendVector(x);
    q.appendVector(y);

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(private-key(ecc(curve %s)(q %b)(d %b)))",
        curveName(curve), q.size(), q.data(), d.size(), d.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return create(identifier, curve, CryptoKeyType::Private, platformKey.release(), extractable, usages);
}

enum class ECAlgorithm {
    PublicKey,
    DH,
};

static std::optional<ECAlgorithm> algorithmForIdentifier(const uint8_t* data, size_t size)
{
    static const std::array<uint8_t, 7> s_idECPublicKey{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01 } };
    static const std::array<uint8_t, 5> s_idECDH{ { 0x2b, 0x81, 0x04, 0x01, 0x0c } };

    if (size == s_idECPublicKey.size() && !std::memcmp(data, s_idECPublicKey.data(), size))
        return ECAlgorithm::PublicKey;
    if (size == s_idECDH.size() && !std::memcmp(data, s_idECDH.data(), size))
        return ECAlgorithm::DH;

    return std::nullopt;
}

static std::optional<CryptoKeyEC::NamedCurve> curveForIdentifier(const uint8_t* data, size_t size)
{
    static const std::array<uint8_t, 8> s_secp256r1{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 } };
    static const std::array<uint8_t, 5> s_secp384r1{ { 0x2b, 0x81, 0x04, 0x00, 0x22 } };
    static const std::array<uint8_t, 5> s_secp521r1{ { 0x2b, 0x81, 0x04, 0x00, 0x23 } };

    if (size == s_secp256r1.size() && !std::memcmp(data, s_secp256r1.data(), size))
        return CryptoKeyEC::NamedCurve::P256;
    if (size == s_secp384r1.size() && !std::memcmp(data, s_secp384r1.data(), size))
        return CryptoKeyEC::NamedCurve::P384;
    if (size == s_secp521r1.size() && !std::memcmp(data, s_secp521r1.data(), size))
        return std::nullopt; // Not supported yet.

    return std::nullopt;
}

RefPtr<CryptoKeyEC> CryptoKeyEC::platformImportSpki(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
{
    // SubjectPublicKeyInfo  ::=  SEQUENCE  {
    //   algorithm            AlgorithmIdentifier,
    //   subjectPublicKey     BIT STRING 
    // }
    // AlgorithmIdentifier  ::=  SEQUENCE  {
    //    algorithm   OBJECT IDENTIFIER,
    //    parameters  ANY DEFINED BY algorithm OPTIONAL
    // }
    // ECParameters ::= CHOICE {
    //   namedCurve         OBJECT IDENTIFIER -- MUST be supported.
    //   -- implicitCurve   NULL -- MUST NOT be used.
    //   -- specifiedCurve  SpecifiedECDomain -- MUST NOT be used.
    // }

    struct : ASN1::Node {
        struct : ASN1::Node {
            ASN1::Node algorithm;
            ASN1::Node namedCurve;
        } algorithmIdentifier;
        ASN1::Node subjectPublicKey;
    } spki;

    ASN1::Parser parser { keyData };

    parser.parseNode(0, ASN1::Tag::Sequence, spki);
    if (!spki || spki.nodeSize != keyData.size())
        return nullptr;

    parser.parseNode(spki.offset, ASN1::Tag::Sequence, spki.algorithmIdentifier);
    if (!spki.algorithmIdentifier)
        return nullptr;

    parser.parseNode(spki.offset + spki.algorithmIdentifier.offset, ASN1::Tag::ObjectIdentifier, spki.algorithmIdentifier.algorithm);
    if (!spki.algorithmIdentifier.algorithm)
        return nullptr;

    auto supportedAlgorithm = algorithmForIdentifier(keyData.data() + spki.offset + spki.algorithmIdentifier.offset + spki.algorithmIdentifier.algorithm.offset,
        spki.algorithmIdentifier.algorithm.length);
    if (!supportedAlgorithm)
        return nullptr;

    parser.parseNode(spki.offset + spki.algorithmIdentifier.offset + spki.algorithmIdentifier.algorithm.nodeSize, ASN1::Tag::ObjectIdentifier, spki.algorithmIdentifier.namedCurve);
    if (!spki.algorithmIdentifier.namedCurve)
        return nullptr;

    auto parameterCurve = curveForIdentifier(keyData.data() + spki.algorithmIdentifier.namedCurve.nodeOffset + spki.algorithmIdentifier.algorithm.offset,
        spki.algorithmIdentifier.namedCurve.length);
    if (!parameterCurve || *parameterCurve != curve)
        return nullptr;

    if (spki.algorithmIdentifier.algorithm.nodeSize + spki.algorithmIdentifier.namedCurve.nodeSize != spki.algorithmIdentifier.length)
        return nullptr;

    parser.parseNode(spki.offset + spki.algorithmIdentifier.nodeSize, ASN1::Tag::BitString, spki.subjectPublicKey);
    if (!spki.subjectPublicKey)
        return nullptr;

    unsigned pointSize = uncompressedPointSizeForCurve(curve);
    if (spki.subjectPublicKey.length != pointSize + 1)
        return nullptr;

    auto* pointData = keyData.data() + spki.subjectPublicKey.nodeOffset + spki.subjectPublicKey.offset;
    if (pointData[0] != 0x00 || pointData[1] != 0x04)
        return nullptr;

    Vector<uint8_t> q;
    q.reserveInitialCapacity(pointSize);
    q.append(keyData.data() + spki.offset + spki.algorithmIdentifier.nodeSize + spki.subjectPublicKey.offset + 1, pointSize);

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(public-key(ecc(curve %s)(q %b)))",
        curveName(curve), q.size(), q.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return create(identifier, curve, CryptoKeyType::Public, platformKey.release(), extractable, usages);
}

RefPtr<CryptoKeyEC> CryptoKeyEC::platformImportPkcs8(CryptoAlgorithmIdentifier identifier, NamedCurve curve, Vector<uint8_t>&& keyData, bool extractable, CryptoKeyUsageBitmap usages)
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

    // ECParameters ::= CHOICE {
    //   namedCurve         OBJECT IDENTIFIER -- MUST be supported.
    //   -- implicitCurve   NULL -- MUST NOT be used.
    //   -- specifiedCurve  SpecifiedECDomain -- MUST NOT be used.
    // }
    auto ecParametersTLV = ASN1::parseTLV(keyData,
        privateKeyAlgorithmTLV.valueOffset() + algorithmTLV.bufferSize);
    if (!ecParametersTLV.bufferSize || !ecParametersTLV.hasTag(ASN1::Tag::ObjectIdentifier))
        return nullptr;

    auto parameterCurve = curveForIdentifier(keyData.data() + ecParametersTLV.valueOffset(), ecParametersTLV.length);
    if (!parameterCurve || *parameterCurve != curve)
        return nullptr;

    if (algorithmTLV.bufferSize + ecParametersTLV.bufferSize != privateKeyAlgorithmTLV.length)
        return nullptr;

    // PrivateKey ::= OCTET STRING
    auto privateKeyTLV = ASN1::parseTLV(keyData,
        privateKeyInfoTLV.valueOffset() + versionTLV.bufferSize + privateKeyAlgorithmTLV.bufferSize);
    if (!privateKeyTLV.bufferSize || !privateKeyTLV.hasTag(ASN1::Tag::OctetString))
        return nullptr;

    if (versionTLV.bufferSize + privateKeyAlgorithmTLV.bufferSize + privateKeyTLV.bufferSize != privateKeyInfoTLV.length)
        return nullptr;

    // ECPrivateKey ::= SEQUENCE {
    //   version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
    //   privateKey     OCTET STRING,
    //   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
    //   publicKey  [1] BIT STRING OPTIONAL
    // }
    auto ecPrivateKeyTLV = ASN1::parseTLV(keyData, privateKeyTLV.valueOffset());
    if (!ecPrivateKeyTLV.bufferSize || !ecPrivateKeyTLV.hasTag(ASN1::Tag::Sequence))
        return nullptr;

    Vector<uint8_t> d;
    Vector<uint8_t> q;
    {
        auto versionTLV = ASN1::parseTLV(keyData, ecPrivateKeyTLV.valueOffset());
        if (!versionTLV.bufferSize || !versionTLV.hasTag(ASN1::Tag::Integer))
            return nullptr;
        if (versionTLV.length != 1 || keyData[versionTLV.valueOffset()] != 1)
            return nullptr;

        auto privateKeyTLV = ASN1::parseTLV(keyData,
            ecPrivateKeyTLV.valueOffset() + versionTLV.bufferSize);
        if (!privateKeyTLV.bufferSize || !privateKeyTLV.hasTag(ASN1::Tag::OctetString))
            return nullptr;

        d.resize(privateKeyTLV.length);
        std::memcpy(d.data(), keyData.data() + privateKeyTLV.valueOffset(), privateKeyTLV.length);

        auto optionalTLV = ASN1::parseTLV(keyData,
            ecPrivateKeyTLV.valueOffset() + versionTLV.bufferSize + privateKeyTLV.bufferSize);

        // FIXME: handle optional parameters, publicKey fields.
        if (optionalTLV.bufferSize && optionalTLV.tag & 0x20) {
            auto publicKeyTLV = ASN1::parseTLV(keyData, optionalTLV.valueOffset());
            if (publicKeyTLV.bufferSize && publicKeyTLV.hasTag(ASN1::Tag::BitString)) {
                if (keyData[publicKeyTLV.valueOffset()] != 0x00)
                    return nullptr;
                q.append(keyData.data() + publicKeyTLV.valueOffset() + 1, publicKeyTLV.length - 1);
            }
        }

        if (versionTLV.bufferSize + privateKeyTLV.bufferSize + optionalTLV.bufferSize != ecPrivateKeyTLV.length)
            return nullptr;
    }

    if (d.size() * 8 != curveSize(curve))
        return nullptr;

    if (!q.isEmpty()) {
        if (q.size() != 2 * (curveSize(curve) / 8) + 1 || q[0] != 0x04)
            return nullptr;
    }

    PAL::GCrypt::Handle<gcry_sexp_t> platformKey;
    gcry_error_t error = gcry_sexp_build(&platformKey, nullptr, "(private-key(ecc(curve %s)(d %b)))",
        curveName(curve), d.size(), d.data());
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return nullptr;
    }

    return create(identifier, curve, CryptoKeyType::Private, platformKey.release(), extractable, usages);
}

Vector<uint8_t> CryptoKeyEC::platformExportRaw() const
{
    PAL::GCrypt::Handle<gcry_ctx_t> context;
    gcry_error_t error = gcry_mpi_ec_new(&context, m_platformKey, nullptr);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return { };
    }

    PAL::GCrypt::Handle<gcry_mpi_t> qMPI(gcry_mpi_ec_get_mpi("q", context, 0));
    if (!qMPI)
        return { };

    Vector<uint8_t> q = extractMPIData(qMPI);
    if (q.size() != uncompressedPointSizeForCurve(m_curve))
        return { };

    return q;
}

void CryptoKeyEC::platformAddFieldElements(JsonWebKey& jwk) const
{
    PAL::GCrypt::Handle<gcry_ctx_t> context;
    gcry_error_t error = gcry_mpi_ec_new(&context, m_platformKey, nullptr);
    if (error != GPG_ERR_NO_ERROR) {
        PAL::GCrypt::logError(error);
        return;
    }

    unsigned uncompressedFieldElementSize = uncompressedFieldElementSizeForCurve(m_curve);

    PAL::GCrypt::Handle<gcry_mpi_t> qMPI(gcry_mpi_ec_get_mpi("q", context, 0));
    if (qMPI) {
        Vector<uint8_t> q = extractMPIData(qMPI);
        if (q.size() == uncompressedPointSizeForCurve(m_curve)) {
            Vector<uint8_t> a;
            a.append(q.data() + 1, uncompressedFieldElementSize);
            jwk.x = base64URLEncode(a);

            Vector<uint8_t> b;
            b.append(q.data() + 1 + uncompressedFieldElementSize, uncompressedFieldElementSize);
            jwk.y = base64URLEncode(b);
        }
    }

    if (type() == Type::Private) {
        PAL::GCrypt::Handle<gcry_mpi_t> dMPI(gcry_mpi_ec_get_mpi("d", context, 0));
        if (dMPI) {
            Vector<uint8_t> d = extractMPIData(dMPI);
            if (d.size() == uncompressedFieldElementSize)
                jwk.d = base64URLEncode(d);
        }
    }
}

Vector<uint8_t> CryptoKeyEC::platformExportSpki() const
{
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
        // static const std::array<uint8_t, 5> s_idECDH{ { 0x2b, 0x81, 0x04, 0x01, 0x0c } };
        static const std::array<uint8_t, 7> s_idECPublicKey{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01 } };

        // ECParameters ::= CHOICE {
        //   namedCurve         OBJECT IDENTIFIER -- MUST be supported.
        //   -- implicitCurve   NULL -- MUST NOT be used.
        //   -- specifiedCurve  SpecifiedECDomain -- MUST NOT be used.
        // }
        Vector<uint8_t> ecParametersData;
        {
            static const std::array<uint8_t, 8> s_secp256r1{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 } };
            static const std::array<uint8_t, 5> s_secp384r1{ { 0x2b, 0x81, 0x04, 0x00, 0x22 } };

            ecParametersData.append(0x06);

            switch (m_curve) {
            case NamedCurve::P256:
                ecParametersData.append(s_secp256r1.size());
                ecParametersData.append(s_secp256r1.data(), s_secp256r1.size());
                break;
            case NamedCurve::P384:
                ecParametersData.append(s_secp384r1.size());
                ecParametersData.append(s_secp384r1.data(), s_secp384r1.size());
                break;
            }
        }

        algorithmIdentifierData.append(0x30);
        appendLength(2 + s_idECPublicKey.size() + ecParametersData.size(), algorithmIdentifierData);

        // algorithm
        algorithmIdentifierData.append(0x06);
        algorithmIdentifierData.append(s_idECPublicKey.size());
        algorithmIdentifierData.append(s_idECPublicKey.data(), s_idECPublicKey.size());

        // parameters
        algorithmIdentifierData.appendVector(ecParametersData);
    }

    Vector<uint8_t> subjectPublicKeyData;
    {
        Vector<uint8_t> q;
        {
            PAL::GCrypt::Handle<gcry_sexp_t> qSexp(gcry_sexp_find_token(m_platformKey, "q", 0));
            if (!qSexp)
                return { };

            size_t pointSize = 2 * (curveSize(m_curve) / 8);
            size_t dataLength = 0;
            const char* data = gcry_sexp_nth_data(qSexp, 1, &dataLength);
            if (!data || dataLength != pointSize + 1 || data[0] != 0x04)
                return { };

            q.append(data, dataLength);
        }

        subjectPublicKeyData.append(0x03);
        appendLength(1 + q.size(), subjectPublicKeyData);
        subjectPublicKeyData.append(0x00);
        subjectPublicKeyData.appendVector(q);
    }

    Vector<uint8_t> result;
    result.append(0x30);
    appendLength(algorithmIdentifierData.size() + subjectPublicKeyData.size(), result);
    result.appendVector(algorithmIdentifierData);
    result.appendVector(subjectPublicKeyData);

    return result;
}

Vector<uint8_t> CryptoKeyEC::platformExportPkcs8() const
{
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
        static const std::array<uint8_t, 7> s_idECPublicKey{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x02, 0x01 } };

        // ECParameters ::= CHOICE {
        //   namedCurve         OBJECT IDENTIFIER -- MUST be supported.
        //   -- implicitCurve   NULL -- MUST NOT be used.
        //   -- specifiedCurve  SpecifiedECDomain -- MUST NOT be used.
        // }
        Vector<uint8_t> ecParametersData;
        {
            static const std::array<uint8_t, 8> s_secp256r1{ { 0x2a, 0x86, 0x48, 0xce, 0x3d, 0x03, 0x01, 0x07 } };
            static const std::array<uint8_t, 5> s_secp384r1{ { 0x2b, 0x81, 0x04, 0x00, 0x22 } };

            ecParametersData.append(0x06);

            switch (m_curve) {
            case NamedCurve::P256:
                ecParametersData.append(s_secp256r1.size());
                ecParametersData.append(s_secp256r1.data(), s_secp256r1.size());
                break;
            case NamedCurve::P384:
                ecParametersData.append(s_secp384r1.size());
                ecParametersData.append(s_secp384r1.data(), s_secp384r1.size());
                break;
            }
        }

        privateKeyAlgorithmData.append(0x30);
        privateKeyAlgorithmData.append(2 + s_idECPublicKey.size() + ecParametersData.size());

        privateKeyAlgorithmData.append(0x06);
        privateKeyAlgorithmData.append(s_idECPublicKey.size());
        privateKeyAlgorithmData.append(s_idECPublicKey.data(), s_idECPublicKey.size());

        privateKeyAlgorithmData.appendVector(ecParametersData);
    }

    // PrivateKey ::= OCTET STRING
    Vector<uint8_t> privateKeyData;
    {
        Vector<uint8_t> d;
        Vector<uint8_t> q;
        {
            PAL::GCrypt::Handle<gcry_ctx_t> context;
            gcry_error_t error = gcry_mpi_ec_new(&context, m_platformKey, nullptr);
            if (error != GPG_ERR_NO_ERROR)
                return { };

            {
                PAL::GCrypt::Handle<gcry_mpi_t> dMPI(gcry_mpi_ec_get_mpi("d", context, 0));
                if (!dMPI)
                    return { };

                size_t dataLength = 0;
                error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, dMPI);
                if (error != GPG_ERR_NO_ERROR)
                    return { };

                d.resize(dataLength);
                error = gcry_mpi_print(GCRYMPI_FMT_USG, d.data(), d.size(), nullptr, dMPI);
                if (error != GPG_ERR_NO_ERROR)
                    return { };
            }
            {
                PAL::GCrypt::Handle<gcry_mpi_t> qMPI(gcry_mpi_ec_get_mpi("q", context, 0));
                if (!qMPI)
                    return { };

                size_t dataLength = 0;
                error = gcry_mpi_print(GCRYMPI_FMT_USG, nullptr, 0, &dataLength, qMPI);
                if (error != GPG_ERR_NO_ERROR)
                    return { };

                q.resize(dataLength);
                error = gcry_mpi_print(GCRYMPI_FMT_USG, q.data(), q.size(), nullptr, qMPI);
                if (error != GPG_ERR_NO_ERROR)
                    return { };
            }
        }

        // ECPrivateKey ::= SEQUENCE {
        //   version        INTEGER { ecPrivkeyVer1(1) } (ecPrivkeyVer1),
        //   privateKey     OCTET STRING,
        //   parameters [0] ECParameters {{ NamedCurve }} OPTIONAL,
        //   publicKey  [1] BIT STRING OPTIONAL
        // }
        Vector<uint8_t> ecPrivateKeyData;
        {
            ecPrivateKeyData.append(0x30);
            appendLength(3 + 2 + d.size() + 2 + 3 + q.size(), ecPrivateKeyData);

            // version
            ecPrivateKeyData.append(0x02);
            ecPrivateKeyData.append(1);
            ecPrivateKeyData.append(1);

            // privateKey
            ecPrivateKeyData.append(0x04);
            ecPrivateKeyData.append(d.size());
            ecPrivateKeyData.appendVector(d);

            // publicKey
            ecPrivateKeyData.append(0xA1);
            ecPrivateKeyData.append(3 + q.size());
            ecPrivateKeyData.append(0x03);
            ecPrivateKeyData.append(1 + q.size());
            ecPrivateKeyData.append(0x00);
            ecPrivateKeyData.appendVector(q);
        }

        privateKeyData.append(0x04);
        appendLength(ecPrivateKeyData.size(), privateKeyData);
        privateKeyData.appendVector(ecPrivateKeyData);
    }

    Vector<uint8_t> result;
    result.append(0x30);
    appendLength(s_versionData.size() + privateKeyAlgorithmData.size() + privateKeyData.size(), result);
    result.append(s_versionData.data(), s_versionData.size());
    result.appendVector(privateKeyAlgorithmData);
    result.appendVector(privateKeyData);

    return result;
}

} // namespace WebCore

#endif // ENABLE(SUBTLE_CRYPTO)
