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
#include "SerializedCryptoKeyWrap.h"

#if ENABLE(SUBTLE_CRYPTO)

#include "NotImplemented.h"
#include <pal/crypto/gcrypt/Handle.h>
#include <pal/crypto/gcrypt/Utilities.h>
#include <wtf/CryptographicUtilities.h>

namespace WebCore {

bool getDefaultWebCryptoMasterKey(Vector<uint8_t>&)
{
    notImplemented();
    return false;
}

bool wrapSerializedCryptoKey(const Vector<uint8_t>& masterKey, const Vector<uint8_t>& key, Vector<uint8_t>& result)
{
    Vector<uint8_t> kek(16);
    gcry_randomize(kek.data(), kek.size(), GCRY_VERY_STRONG_RANDOM);

    Vector<uint8_t> wrappedKEK(24);
    {
        auto algorithm = PAL::GCrypt::aesAlgorithmForKeySize(masterKey.size() * 8);
        if (!algorithm)
            return false;

        PAL::GCrypt::Handle<gcry_cipher_hd_t> handle;
        gcry_error_t error = gcry_cipher_open(&handle, *algorithm, GCRY_CIPHER_MODE_AESWRAP, 0);
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_setkey(handle, masterKey.data(), masterKey.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_encrypt(handle, wrappedKEK.data(), wrappedKEK.size(), kek.data(), kek.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;
    }

    Vector<uint8_t> encryptedKey(key.size());
    Vector<uint8_t> tag(16);
    {
        PAL::GCrypt::Handle<gcry_cipher_hd_t> handle;
        gcry_error_t error = gcry_cipher_open(&handle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_GCM, 0);
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_setkey(handle, kek.data(), kek.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_encrypt(handle, encryptedKey.data(), encryptedKey.size(), key.data(), key.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_gettag(handle, tag.data(), tag.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;
    }

    // magic[3] + version[1] + wrappedKEK[24] + encryptedKey[length(key)] + tag[16] + magic[3]
    size_t serializedLength = 3 + 1 + 24 + encryptedKey.size() + 16 + 3;
    result.resize(serializedLength);
    result[0] = 0x04;
    result[1] = 0x08;
    result[2] = 0x15;
    result[3] = 0x01;
    std::memcpy(result.data() + 4, wrappedKEK.data(), 24);
    std::memcpy(result.data() + 28, encryptedKey.data(), encryptedKey.size());
    std::memcpy(result.data() + 28 + encryptedKey.size(), tag.data(), 16);
    result[serializedLength - 3] = 0x16;
    result[serializedLength - 2] = 0x23;
    result[serializedLength - 1] = 0x42;

    return true;
}

bool unwrapSerializedCryptoKey(const Vector<uint8_t>& masterKey, const Vector<uint8_t>& wrappedKey, Vector<uint8_t>& key)
{
    // magic[3] + version[1] + wrappedKEK[24] + encryptedKey[length(key)] + tag[16] + magic[3]
    size_t serializedLength = wrappedKey.size();
    if (serializedLength < 3 + 1 + 24 + 16 + 3)
        return false;

    uint8_t magic1[3] = { 0x04, 0x08, 0x15 };
    if (std::memcmp(wrappedKey.data(), magic1, 3))
        return false;
    uint8_t magic2[3] = { 0x16, 0x23, 0x42 };
    if (std::memcmp(wrappedKey.data() + serializedLength - 3, magic2, 3))
        return false;
    if (wrappedKey[3] != 0x01)
        return false;

    Vector<uint8_t> wrappedKEK(24);
    std::memcpy(wrappedKEK.data(), wrappedKey.data() + 4, 24);

    Vector<uint8_t> encryptedKey(serializedLength - 3 - 1 - 24 - 16 - 3);
    std::memcpy(encryptedKey.data(), wrappedKey.data() + 28, encryptedKey.size());

    Vector<uint8_t> tag(16);
    std::memcpy(tag.data(), wrappedKey.data() + 28 + encryptedKey.size(), 16);

    Vector<uint8_t> kek(16);
    {
        auto algorithm = PAL::GCrypt::aesAlgorithmForKeySize(masterKey.size() * 8);
        if (!algorithm)
            return false;

        PAL::GCrypt::Handle<gcry_cipher_hd_t> handle;
        gcry_error_t error = gcry_cipher_open(&handle, *algorithm, GCRY_CIPHER_MODE_AESWRAP, 0);
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_setkey(handle, masterKey.data(), masterKey.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_decrypt(handle, kek.data(), kek.size(), wrappedKEK.data(), wrappedKEK.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;
    }

    Vector<uint8_t> decryptedKey(encryptedKey.size());
    Vector<uint8_t> actualTag(16);
    {
        PAL::GCrypt::Handle<gcry_cipher_hd_t> handle;
        gcry_error_t error = gcry_cipher_open(&handle, GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_GCM, 0);
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_setkey(handle, kek.data(), kek.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_decrypt(handle, decryptedKey.data(), decryptedKey.size(), encryptedKey.data(), encryptedKey.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;

        error = gcry_cipher_gettag(handle, actualTag.data(),actualTag.size());
        if (error != GPG_ERR_NO_ERROR)
            return false;
    }

    if (constantTimeMemcmp(tag.data(), actualTag.data(), 16))
        return false;

    key = WTFMove(decryptedKey);
    return true;
}

}

#endif // ENABLE(SUBTLE_CRYPTO)
