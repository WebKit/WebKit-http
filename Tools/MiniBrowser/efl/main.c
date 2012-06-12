/*
 * Copyright (C) 2012 Samsung Electronics
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "EWebKit2.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Eina.h>
#include <Evas.h>

static const int DEFAULT_WIDTH = 800;
static const int DEFAULT_HEIGHT = 600;
static const char DEFAULT_URL[] = "http://www.google.com/";

typedef struct _MiniBrowser {
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg;
    Evas_Object *browser;
} MiniBrowser;

static Eina_Bool main_signal_exit(void *data, int ev_type, void *ev)
{
    ecore_main_loop_quit();
    return EINA_TRUE;
}

static void on_ecore_evas_resize(Ecore_Evas *ee)
{
    Evas_Object *webview;
    Evas_Object *bg;
    int w, h;

    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

    bg = evas_object_name_find(ecore_evas_get(ee), "bg");
    evas_object_move(bg, 0, 0);
    evas_object_resize(bg, w, h);

    webview = evas_object_name_find(ecore_evas_get(ee), "browser");
    evas_object_move(webview, 0, 0);
    evas_object_resize(webview, w, h);
}

static MiniBrowser *browserCreate(const char *url)
{
    MiniBrowser *app = malloc(sizeof(MiniBrowser));

    app->ee = ecore_evas_new(0, 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0);

    ecore_evas_title_set(app->ee, "EFL MiniBrowser");
    ecore_evas_callback_resize_set(app->ee, on_ecore_evas_resize);
    ecore_evas_borderless_set(app->ee, 0);
    ecore_evas_show(app->ee);

    app->evas = ecore_evas_get(app->ee);

    app->bg = evas_object_rectangle_add(app->evas);
    evas_object_name_set(app->bg, "bg");
    evas_object_size_hint_weight_set(app->bg, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_move(app->bg, 0, 0);
    evas_object_resize(app->bg, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    evas_object_color_set(app->bg, 255, 150, 150, 255);
    evas_object_show(app->bg);

    /* Create webview */
    app->browser = ewk_view_add(app->evas);
    evas_object_name_set(app->browser, "browser");
    evas_object_size_hint_weight_set(app->browser, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

    evas_object_resize(app->browser, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    evas_object_show(app->browser);
    evas_object_focus_set(app->browser, EINA_TRUE);

    ewk_view_uri_set(app->browser, url);

    return app;
}

int main(int argc, char *argv[])
{
    const char *url;
    int args = 1;

    if (!ecore_evas_init())
        return EXIT_FAILURE;

    if (args < argc)
        url = argv[args];
    else
        url = DEFAULT_URL;

    MiniBrowser *browser = browserCreate(url);

    Ecore_Event_Handler *handle = ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, main_signal_exit, 0);

    ecore_main_loop_begin();

    ecore_event_handler_del(handle);
    ecore_evas_free(browser->ee);
    free(browser);

    ecore_evas_shutdown();

    return 0;
}
