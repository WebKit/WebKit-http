/*
 * Copyright (C) 2012 Samsung Electronics
 * Copyright (C) 2012 Intel Corporation. All rights reserved.
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
#include "url_bar.h"
#include "url_utils.h"
#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Eina.h>
#include <Evas.h>

static const int DEFAULT_WIDTH = 800;
static const int DEFAULT_HEIGHT = 600;
static const char DEFAULT_URL[] = "http://www.google.com/";
static const char APP_NAME[] = "EFL MiniBrowser";

#define info(format, args...)       \
    do {                            \
        if (verbose)                \
            printf(format, ##args); \
    } while (0)

static int verbose = 0;

typedef struct _MiniBrowser {
    Ecore_Evas *ee;
    Evas *evas;
    Evas_Object *bg;
    Evas_Object *browser;
    Url_Bar *url_bar;
} MiniBrowser;

MiniBrowser *browser;

static const Ecore_Getopt options = {
    "MiniBrowser",
    "%prog [options] [url]",
    "0.0.1",
    "(C)2012 Samsung Electronics\n",
    "",
    "Test Web Browser using the Enlightenment Foundation Libraries of WebKit2",
    EINA_TRUE, {
        ECORE_GETOPT_STORE_STR
            ('e', "engine", "ecore-evas engine to use."),
        ECORE_GETOPT_CALLBACK_NOARGS
            ('E', "list-engines", "list ecore-evas engines.",
             ecore_getopt_callback_ecore_evas_list_engines, NULL),
        ECORE_GETOPT_VERSION
            ('V', "version"),
        ECORE_GETOPT_COPYRIGHT
            ('R', "copyright"),
        ECORE_GETOPT_HELP
            ('h', "help"),
        ECORE_GETOPT_SENTINEL
    }
};

static Eina_Bool main_signal_exit(void *data, int ev_type, void *ev)
{
    ecore_main_loop_quit();
    return EINA_TRUE;
}

static void closeWindow(Ecore_Evas *ee)
{
    ecore_main_loop_quit();
}

static void on_ecore_evas_resize(Ecore_Evas *ee)
{
    Evas_Object *webview;
    Evas_Object *bg;
    int w, h;

    ecore_evas_geometry_get(ee, NULL, NULL, &w, &h);

    /* Resize URL bar */
    url_bar_width_set(browser->url_bar, w);

    bg = evas_object_name_find(ecore_evas_get(ee), "bg");
    evas_object_move(bg, 0, 0);
    evas_object_resize(bg, w, h);

    webview = evas_object_name_find(ecore_evas_get(ee), "browser");
    evas_object_move(webview, 0, URL_BAR_HEIGHT);
    evas_object_resize(webview, w, h - URL_BAR_HEIGHT);
}

static void
on_key_down(void *data, Evas *e, Evas_Object *obj, void *event_info)
{
    Evas_Event_Key_Down *ev = (Evas_Event_Key_Down*) event_info;
    static const char *encodings[] = {
        "ISO-8859-1",
        "UTF-8",
        NULL
    };
    static int currentEncoding = -1;
    if (!strcmp(ev->key, "F1")) {
        info("Back (F1) was pressed\n");
        if (!ewk_view_back(obj))
            info("Back ignored: No back history\n");
    } else if (!strcmp(ev->key, "F2")) {
        info("Forward (F2) was pressed\n");
        if (!ewk_view_forward(obj))
            info("Forward ignored: No forward history\n");
    } else if (!strcmp(ev->key, "F3")) {
        currentEncoding = (currentEncoding + 1) % (sizeof(encodings) / sizeof(encodings[0]));
        info("Set encoding (F3) pressed. New encoding to %s", encodings[currentEncoding]);
        ewk_view_setting_encoding_custom_set(obj, encodings[currentEncoding]);
    } else if (!strcmp(ev->key, "F5")) {
            info("Reload (F5) was pressed, reloading.\n");
            ewk_view_reload(obj);
    } else if (!strcmp(ev->key, "F6")) {
            info("Stop (F6) was pressed, stop loading.\n");
            ewk_view_stop(obj);
    }
}

static void
on_mouse_down(void *data, Evas *e, Evas_Object *webview, void *event_info)
{
    Evas_Event_Mouse_Down *ev = (Evas_Event_Mouse_Down *)event_info;

    if (ev->button == 1)
        evas_object_focus_set(webview, EINA_TRUE);
    else if (ev->button == 2)
        evas_object_focus_set(webview, !evas_object_focus_get(webview));
}

static void
title_set(Ecore_Evas *ee, const char *title, int progress)
{
    Eina_Strbuf* buffer;

    if (!title || !*title) {
        ecore_evas_title_set(ee, APP_NAME);
        return;
    }

    buffer = eina_strbuf_new();
    if (progress < 100)
        eina_strbuf_append_printf(buffer, "%s (%d%%) - %s", title, progress, APP_NAME);
    else
        eina_strbuf_append_printf(buffer, "%s - %s", title, APP_NAME);

    ecore_evas_title_set(ee, eina_strbuf_string_get(buffer));
    eina_strbuf_free(buffer);
}

static void
on_title_changed(void *user_data, Evas_Object *webview, void *event_info)
{
    MiniBrowser *app = (MiniBrowser *)user_data;
    const char *title = (const char *)event_info;

    title_set(app->ee, title, 100);
}

static void
on_url_changed(void *user_data, Evas_Object *webview, void *event_info)
{
    MiniBrowser *app = (MiniBrowser *)user_data;
    url_bar_url_set(app->url_bar, ewk_view_uri_get(app->browser));
}

static void
on_progress(void *user_data, Evas_Object *webview, void *event_info)
{
    MiniBrowser *app = (MiniBrowser *)user_data;
    double progress = *(double *)event_info;

    title_set(app->ee, ewk_view_title_get(app->browser), progress * 100);
}

static void
on_error(void *user_data, Evas_Object *webview, void *event_info)
{
    Eina_Strbuf* buffer;
    const Ewk_Web_Error *error = (const Ewk_Web_Error *)event_info;

    buffer = eina_strbuf_new();
    eina_strbuf_append_printf(buffer, "<html><body><div style=\"color:#ff0000\">ERROR!</div><br><div>Code: %d<br>Description: %s<br>URL: %s</div></body</html>",
                              ewk_web_error_code_get(error), ewk_web_error_description_get(error), ewk_web_error_url_get(error));

    ewk_view_html_string_load(webview, eina_strbuf_string_get(buffer), 0, ewk_web_error_url_get(error));
    eina_strbuf_free(buffer);
}

static int
quit(Eina_Bool success, const char *msg)
{
    ewk_shutdown();

    if (msg)
        fputs(msg, (success) ? stdout : stderr);

    if (!success)
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

static MiniBrowser *browserCreate(const char *url, const char *engine)
{
    MiniBrowser *app = malloc(sizeof(MiniBrowser));

    app->ee = ecore_evas_new(engine, 0, 0, DEFAULT_WIDTH, DEFAULT_HEIGHT, 0);
    if (!app->ee)
        return 0;

    ecore_evas_title_set(app->ee, APP_NAME);
    ecore_evas_callback_resize_set(app->ee, on_ecore_evas_resize);
    ecore_evas_borderless_set(app->ee, 0);
    ecore_evas_show(app->ee);
    ecore_evas_callback_delete_request_set(app->ee, closeWindow);

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
    ewk_view_theme_set(app->browser, THEME_DIR"/default.edj");
    evas_object_name_set(app->browser, "browser");

    evas_object_smart_callback_add(app->browser, "load,error", on_error, app);
    evas_object_smart_callback_add(app->browser, "load,progress", on_progress, app);
    evas_object_smart_callback_add(app->browser, "title,changed", on_title_changed, app);
    evas_object_smart_callback_add(app->browser, "uri,changed", on_url_changed, app);

    evas_object_event_callback_add(app->browser, EVAS_CALLBACK_KEY_DOWN, on_key_down, app);
    evas_object_event_callback_add(app->browser, EVAS_CALLBACK_MOUSE_DOWN, on_mouse_down, app);

    evas_object_size_hint_weight_set(app->browser, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
    evas_object_move(app->browser, 0, URL_BAR_HEIGHT);
    evas_object_resize(app->browser, DEFAULT_WIDTH, DEFAULT_HEIGHT - URL_BAR_HEIGHT);
    evas_object_show(app->browser);
    evas_object_focus_set(app->browser, EINA_TRUE);

    app->url_bar = url_bar_add(app->browser, DEFAULT_WIDTH);

    ewk_view_uri_set(app->browser, url);

    return app;
}

int main(int argc, char *argv[])
{
    int args = 1;
    char *engine = NULL;
    unsigned char quitOption = 0;

    Ecore_Getopt_Value values[] = {
        ECORE_GETOPT_VALUE_STR(engine),
        ECORE_GETOPT_VALUE_BOOL(quitOption),
        ECORE_GETOPT_VALUE_BOOL(quitOption),
        ECORE_GETOPT_VALUE_BOOL(quitOption),
        ECORE_GETOPT_VALUE_BOOL(quitOption),
        ECORE_GETOPT_VALUE_NONE
    };

    if (!ewk_init())
        return EXIT_FAILURE;

    ecore_app_args_set(argc, (const char **) argv);
    args = ecore_getopt_parse(&options, values, argc, argv);

    if (args < 0)
        return quit(EINA_FALSE, "ERROR: could not parse options.\n");

    if (quitOption)
        return quit(EINA_TRUE, NULL);

    if (args < argc) {
        char *url = url_from_user_input(argv[args]);
        browser = browserCreate(url, engine);
        free(url);
    } else
        browser = browserCreate(DEFAULT_URL, engine);

    if (!browser)
        return quit(EINA_FALSE, "ERROR: could not create browser.\n");

    Ecore_Event_Handler *handle = ecore_event_handler_add(ECORE_EVENT_SIGNAL_EXIT, main_signal_exit, 0);

    ecore_main_loop_begin();

    url_bar_del(browser->url_bar);
    ecore_event_handler_del(handle);
    ecore_evas_free(browser->ee);
    free(browser);

    return quit(EINA_TRUE, NULL);
}
