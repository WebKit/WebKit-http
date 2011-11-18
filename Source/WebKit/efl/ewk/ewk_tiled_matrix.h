/*
    Copyright (C) 2009-2010 Samsung Electronics
    Copyright (C) 2009-2010 ProFUSION embedded systems

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

#ifndef ewk_tiled_matrix_h
#define ewk_tiled_matrix_h

#include "ewk_tiled_backing_store.h"

#include <Evas.h>

/* matrix of tiles */
Ewk_Tile_Matrix* ewk_tile_matrix_new(Ewk_Tile_Unused_Cache* tileUnusedCache, unsigned long columns, unsigned long rows, float zoomLevel, Evas_Colorspace colorSpace, void (*render_callback)(void* data, Ewk_Tile* tile, const Eina_Rectangle* update), const void* renderData);
void ewk_tile_matrix_free(Ewk_Tile_Matrix* tm);

void ewk_tile_matrix_resize(Ewk_Tile_Matrix* tm, unsigned long cols, unsigned long rows);
void ewk_tile_matrix_zoom_level_set(Ewk_Tile_Matrix* tileMatrix, float zoom);
void ewk_tile_matrix_invalidate(Ewk_Tile_Matrix* tileMatrix);

Ewk_Tile_Unused_Cache* ewk_tile_matrix_unused_cache_get(const Ewk_Tile_Matrix* tm);

Ewk_Tile* ewk_tile_matrix_tile_exact_get(Ewk_Tile_Matrix* tm, unsigned long col, unsigned long row, float zoom);
Eina_Bool ewk_tile_matrix_tile_exact_exists(Ewk_Tile_Matrix* tm, unsigned long col, unsigned long row, float zoom);
Ewk_Tile* ewk_tile_matrix_tile_nearest_get(Ewk_Tile_Matrix* tm, unsigned long col, unsigned long row, float zoom);
Ewk_Tile* ewk_tile_matrix_tile_new(Ewk_Tile_Matrix* tm, Evas* evas, unsigned long col, unsigned long row, float zoom);
Eina_Bool ewk_tile_matrix_tile_put(Ewk_Tile_Matrix* tm, Ewk_Tile* t, double last_used);

Eina_Bool ewk_tile_matrix_tile_update(Ewk_Tile_Matrix* tm, unsigned long col, unsigned long row, const Eina_Rectangle* update);
Eina_Bool ewk_tile_matrix_tile_update_full(Ewk_Tile_Matrix* tm, unsigned long col, unsigned long row);
void ewk_tile_matrix_tile_updates_clear(Ewk_Tile_Matrix* tm, Ewk_Tile* t);

Eina_Bool ewk_tile_matrix_update(Ewk_Tile_Matrix* tm, const Eina_Rectangle* update, float zoom);
void ewk_tile_matrix_updates_process(Ewk_Tile_Matrix* tm);
void ewk_tile_matrix_updates_clear(Ewk_Tile_Matrix* tm);
void ewk_tile_matrix_freeze(Ewk_Tile_Matrix* tm);
void ewk_tile_matrix_thaw(Ewk_Tile_Matrix* tm);

// remove me!
void ewk_tile_matrix_dbg(const Ewk_Tile_Matrix* tm);
void ewk_tile_unused_cache_dbg(const Ewk_Tile_Unused_Cache* tuc);
void ewk_tile_accounting_dbg();


#endif // ewk_tiled_matrix_h

