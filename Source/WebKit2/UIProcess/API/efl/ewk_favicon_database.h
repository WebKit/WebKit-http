/*
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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

/**
 * @file    ewk_favicon_database.h
 * @brief   Describes the Ewk Favicon Database API.
 */

#ifndef ewk_favicon_database_h
#define ewk_favicon_database_h

#include <Evas.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Creates a type name for @a Ewk_Favicon_Database. */
typedef struct EwkFaviconDatabase Ewk_Favicon_Database;

/**
 * @typedef Ewk_Favicon_Database_Icon_Change_Cb Ewk_Favicon_Database_Icon_Change_Cb
 * @brief Callback type for use with ewk_favicon_database_icon_change_callback_add and ewk_favicon_database_icon_change_callback_del
 */
typedef void (*Ewk_Favicon_Database_Icon_Change_Cb)(const char *page_url, void *event_info);

/**
 * @typedef Ewk_Favicon_Database_Async_Icon_Get_Cb Ewk_Favicon_Database_Async_Icon_Get_Cb
 * @brief Callback type for use with ewk_favicon_database_async_icon_get
 *
 * The @a icon may be NULL if there is no favicon associated to the given @a page_url.
 *
 * You need to call evas_object_ref() on the @a icon if you wish to keep it after the
 * callback is executed.
 */
typedef void (*Ewk_Favicon_Database_Async_Icon_Get_Cb)(const char *page_url, Evas_Object *icon, void *event_info);

/**
 * Retrieves from the database the favicon URL for the given @a page_url
 *
 * @param database database object to query
 * @param page_url URL of the page to get the favicon URL for
 *
 * @return a newly allocated string guaranteed to be eina_stringshare
 *         or @c NULL in case of error or if the key does not exist.
 *         You need to call eina_stringshare_del() after use.
 */
EAPI const char *ewk_favicon_database_icon_url_get(Ewk_Favicon_Database *database, const char *page_url);

/**
 * Retrieves asynchronously from the database the favicon for the given @a page_url
 *
 * @param database database object to query
 * @param page_url URL of the page to get the favicon for
 * @param evas The canvas to add the favicon to
 * @param callback callback function to be called when the icon is retrieved
 * @param data the data pointer that was to be passed to the callback
 *
 * @return @c EINA_TRUE if the icon was successfuly requested, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool ewk_favicon_database_async_icon_get(Ewk_Favicon_Database *database, const char *page_url, Evas *evas, Ewk_Favicon_Database_Async_Icon_Get_Cb callback, void *data);

/**
 * Add (register) a callback function to a icon change event
 *
 * @param database database object to register the callback
 * @param callback callback function to be called when an icon changes
 * @param data the data pointer that was to be passed to the callback
 */
EAPI void ewk_favicon_database_icon_change_callback_add(Ewk_Favicon_Database *database, Ewk_Favicon_Database_Icon_Change_Cb callback, void *data);

/**
 * Delete (unregister) a callback function registered to a icon change event
 *
 * @param database database object to unregister the callback.
 * @param callback callback function to unregister
 */
EAPI void ewk_favicon_database_icon_change_callback_del(Ewk_Favicon_Database *database, Ewk_Favicon_Database_Icon_Change_Cb callback);

#ifdef __cplusplus
}
#endif

#endif // ewk_favicon_database_h
