/* GStreamer ISO MPEG DASH common encryption decryptor
 * Copyright (C) 2015 Igalia S.L
 * Copyright (C) 2015 Metrological
 * Copyright (C) 2013 YouView TV Ltd. <alex.ashley@youview.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */

#include <gcrypt.h>
#include <glib-object.h>
#include <stdint.h>
#include <string.h>


#include "WebKitMediaAesCtr.h"

#define KEY_SIZE 16
struct _AesCtrState {
    volatile gint refCount;
    gcry_cipher_hd_t handle;
};

AesCtrState* webkit_media_aes_ctr_decrypt_new(GBytes* key, GBytes* iv)
{
    const uint8_t* ivBuffer;
    const uint8_t* keyBuffer;
    gsize ivSize, keySize;
    AesCtrState* state;
    uint8_t ctr[KEY_SIZE];

    if (!key || !iv)
        return NULL;

    state = g_slice_new(AesCtrState);
    if (!state)
        return NULL;

    keyBuffer = (const uint8_t*) g_bytes_get_data(key, &keySize);
    g_assert(keySize == KEY_SIZE);

    gcry_cipher_open(&(state->handle), GCRY_CIPHER_AES128, GCRY_CIPHER_MODE_CTR, 0);
    gcry_cipher_setkey(state->handle, keyBuffer, keySize);

    ivBuffer = (const uint8_t*) g_bytes_get_data(iv, &ivSize);
    if (ivSize == 8) {
        memset(ctr + 8, 0, 8);
        memcpy(ctr, ivBuffer, 8);
    } else
        memcpy(ctr, ivBuffer, KEY_SIZE);
    gcry_cipher_setctr(state->handle, ctr, KEY_SIZE);

    return state;
}

gboolean webkit_media_aes_ctr_decrypt_ip(AesCtrState* state, unsigned char* data, int length)
{
    gcry_cipher_decrypt(state->handle, data, length, 0, 0);
    return TRUE;
}

AesCtrState* webkit_media_aes_ctr_decrypt_ref(AesCtrState* state)
{
    g_return_val_if_fail(state != NULL, NULL);

    g_atomic_int_inc(&(state->refCount));
    return state;
}

void webkit_media_aes_ctr_decrypt_unref(AesCtrState* state)
{
    g_return_if_fail(state != NULL);

    if (g_atomic_int_dec_and_test(&(state->refCount))) {
        gcry_cipher_close(state->handle);
        g_slice_free(AesCtrState, state);
    }
}

G_DEFINE_BOXED_TYPE(AesCtrState, webkit_media_aes_ctr, (GBoxedCopyFunc) webkit_media_aes_ctr_decrypt_ref,
    (GBoxedFreeFunc) webkit_media_aes_ctr_decrypt_unref);
