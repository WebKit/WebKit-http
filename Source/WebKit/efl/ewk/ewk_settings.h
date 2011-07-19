/*
    Copyright (C) 2009-2010 ProFUSION embedded systems
    Copyright (C) 2009-2010 Samsung Electronics

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#ifndef ewk_settings_h
#define ewk_settings_h

#include <Eina.h>
#include <Evas.h>
#include <cairo.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @file ewk_settings.h
 *
 * @brief General purpose settings, not tied to any view object.
 */

EAPI uint64_t         ewk_settings_web_database_default_quota_get(void);
EAPI void             ewk_settings_web_database_path_set(const char *path);
EAPI const char      *ewk_settings_web_database_path_get(void);

EAPI Eina_Bool        ewk_settings_icon_database_path_set(const char *path);
EAPI const char      *ewk_settings_icon_database_path_get(void);
EAPI Eina_Bool        ewk_settings_icon_database_clear(void);

EAPI cairo_surface_t *ewk_settings_icon_database_icon_surface_get(const char *url);
EAPI Evas_Object     *ewk_settings_icon_database_icon_object_add(const char *url, Evas *canvas);

EAPI Eina_Bool        ewk_settings_cache_directory_path_set(const char *path);
EAPI const char      *ewk_settings_cache_directory_path_get(void);

EAPI Eina_Bool        ewk_settings_cache_enable_get(void);
EAPI void             ewk_settings_cache_enable_set(Eina_Bool set);
EAPI void             ewk_settings_cache_capacity_set(unsigned capacity);

EAPI void             ewk_settings_repaint_throttling_set(double deferred_repaint_delay, double initial_deferred_repaint_delay_during_loading, double max_deferred_repaint_delay_during_loading, double deferred_repaint_delay_increment_during_loading);

#ifdef __cplusplus
}
#endif
#endif // ewk_settings_h
