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

#include <openssl/aes.h>
#include <string.h>
#include <gst/gst.h>

#include "WebKitMediaAesCtr.h"

struct _AesCtrState {
  volatile gint refcount;
  AES_KEY key;
  unsigned char ivec[16];
  unsigned int num;
  unsigned char ecount[16];
};

AesCtrState* webkit_media_aes_ctr_decrypt_new(GBytes* key, GBytes* iv)
{
  unsigned char* buf;
  GstMapInfo map;
  gsize iv_length;
  AesCtrState* state;

  g_return_val_if_fail(key!=NULL,NULL);
  g_return_val_if_fail(iv!=NULL,NULL);

  state = g_slice_new(AesCtrState);
  if(!state) {
    GST_ERROR("Failed to allocate AesCtrState");
    return NULL;
  }
  g_assert (g_bytes_get_size (key) == 16);
  AES_set_encrypt_key ((const unsigned char*) g_bytes_get_data (key, NULL),
      8 * g_bytes_get_size (key), &state->key);

  buf = (unsigned char*)g_bytes_get_data(iv, &iv_length);
  state->num = 0; 
  memset(state->ecount, 0, 16);      
  if(iv_length==8){
    memset(state->ivec + 8, 0, 8);  
    memcpy(state->ivec, buf, 8); 
  }
  else{
    memcpy(state->ivec, buf, 16); 
  }
  return state;
} 

AesCtrState*
webkit_media_aes_ctr_decrypt_ref(AesCtrState *state)
{
  g_return_val_if_fail (state != NULL, NULL);

  g_atomic_int_inc (&state->refcount);

  return state;
}

void
webkit_media_aes_ctr_decrypt_unref(AesCtrState *state)
{
  g_return_if_fail (state != NULL);

  if (g_atomic_int_dec_and_test (&state->refcount)) {
    g_slice_free (AesCtrState, state);
  }
}


gboolean
webkit_media_aes_ctr_decrypt_ip(AesCtrState *state, 
		       unsigned char *data,
		       int length)
{
  AES_ctr128_encrypt(data, data, length, &state->key, state->ivec, 
		     state->ecount, &state->num);
  return TRUE;
}

G_DEFINE_BOXED_TYPE (AesCtrState, webkit_media_aes_ctr,
		     (GBoxedCopyFunc) webkit_media_aes_ctr_decrypt_ref,
		     (GBoxedFreeFunc) webkit_media_aes_ctr_decrypt_unref);
