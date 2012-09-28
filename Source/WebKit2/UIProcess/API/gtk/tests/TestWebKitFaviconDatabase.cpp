/*
 * Copyright (C) 2012 Igalia S.L.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2,1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "WebKitTestServer.h"
#include "WebViewTest.h"
#include <glib/gstdio.h>
#include <libsoup/soup.h>
#include <wtf/gobject/GOwnPtr.h>

static WebKitTestServer* kServer;
static char* kTempDirectory;

class FaviconDatabaseTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(FaviconDatabaseTest);

    FaviconDatabaseTest()
        : m_webContext(webkit_web_context_get_default())
        , m_favicon(0)
        , m_error(0)
        , m_iconReadySignalReceived(false)
    {
    }

    ~FaviconDatabaseTest()
    {
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        g_signal_handlers_disconnect_matched(database, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
    }

    static void iconReadyCallback(WebKitFaviconDatabase* database, const char* pageURI, FaviconDatabaseTest* test)
    {
        g_assert_cmpstr(webkit_web_view_get_uri(test->m_webView), ==, pageURI);
        test->m_iconReadySignalReceived = true;
    }

    static void getFaviconCallback(GObject* sourceObject, GAsyncResult* result, void* data)
    {
        FaviconDatabaseTest* test = static_cast<FaviconDatabaseTest*>(data);
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);

        test->m_error.clear();
        test->m_favicon = webkit_favicon_database_get_favicon_finish(database, result, &test->m_error.outPtr());
        g_main_loop_quit(test->m_mainLoop);
    }

    void askForIconAndWaitUntilReady()
    {
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        webkit_favicon_database_get_favicon(database, kServer->getURIForPath("/").data(), 0, getFaviconCallback, this);

        g_signal_connect(database, "favicon-ready", G_CALLBACK(iconReadyCallback), this);

        g_main_loop_run(m_mainLoop);
    }

    void askAndWaitForFavicon(const char* pageURI)
    {
        WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(m_webContext);
        webkit_favicon_database_get_favicon(database, pageURI, 0, getFaviconCallback, this);
        g_main_loop_run(m_mainLoop);
    }

    WebKitWebContext* m_webContext;
    cairo_surface_t* m_favicon;
    GOwnPtr<GError> m_error;
    bool m_iconReadySignalReceived;
};

static void
serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable* query, SoupClientContext* context, void* data)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }
    soup_message_set_status(message, SOUP_STATUS_OK);

    char* contents;
    gsize length;
    if (g_str_equal(path, "/favicon.ico")) {
        GOwnPtr<char> pathToFavicon(g_build_filename(Test::getWebKit1TestResoucesDir().data(), "blank.ico", NULL));
        g_file_get_contents(pathToFavicon.get(), &contents, &length, 0);
    } else if (g_str_equal(path, "/nofavicon/favicon.ico")) {
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
        soup_message_body_complete(message->response_body);
        return;
    } else {
        contents = g_strdup("<html><body>test</body></html>");
        length = strlen(contents);
    }

    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, length);
    soup_message_body_complete(message->response_body);
}

static void testSetDirectory(FaviconDatabaseTest* test, gconstpointer)
{
    webkit_web_context_set_favicon_database_directory(test->m_webContext, kTempDirectory);
    g_assert_cmpstr(kTempDirectory, ==, webkit_web_context_get_favicon_database_directory(test->m_webContext));
}

static void testClearDatabase(FaviconDatabaseTest* test, gconstpointer)
{
    WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);
    webkit_favicon_database_clear(database);

    GOwnPtr<char> iconURI(webkit_favicon_database_get_favicon_uri(database, kServer->getURIForPath("/").data()));
    g_assert(!iconURI);
}

static void testGetFavicon(FaviconDatabaseTest* test, gconstpointer)
{
    // We need to load the page first to ensure the icon data will be
    // in the database in case there's an associated favicon.
    test->loadURI(kServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();

    // Check the WebKitFaviconDatabase::icon-ready signal.
    test->m_iconReadySignalReceived = false;
    test->askForIconAndWaitUntilReady();
    g_assert(test->m_iconReadySignalReceived);

    // Check the API retrieving a valid favicon.
    test->askAndWaitForFavicon(kServer->getURIForPath("/").data());
    g_assert(test->m_favicon);
    g_assert(!test->m_error);

    // Check that width and height match those from blank.ico (16x16 favicon).
    g_assert_cmpint(cairo_image_surface_get_width(test->m_favicon), ==, 16);
    g_assert_cmpint(cairo_image_surface_get_height(test->m_favicon), ==, 16);
    cairo_surface_destroy(test->m_favicon);

    // Check the API retrieving an invalid favicon.
    test->loadURI(kServer->getURIForPath("/nofavicon").data());
    test->waitUntilLoadFinished();

    test->askAndWaitForFavicon(kServer->getURIForPath("/nofavicon/").data());
    g_assert(!test->m_favicon);
    g_assert(test->m_error && (test->m_error->code & WEBKIT_FAVICON_DATABASE_ERROR_FAVICON_NOT_FOUND));
}

static void testGetFaviconURI(FaviconDatabaseTest* test, gconstpointer)
{
    WebKitFaviconDatabase* database = webkit_web_context_get_favicon_database(test->m_webContext);

    const char* baseURI = kServer->getURIForPath("/").data();
    GOwnPtr<char> iconURI(webkit_favicon_database_get_favicon_uri(database, baseURI));
    g_assert_cmpstr(iconURI.get(), ==, kServer->getURIForPath("/favicon.ico").data());
}

static void deleteDatabaseFiles()
{
    if (!g_file_test(kTempDirectory, G_FILE_TEST_IS_DIR))
        return;

    GOwnPtr<char> filename(g_build_filename(kTempDirectory, "WebpageIcons.db", NULL));
    g_unlink(filename.get());

    filename.set(g_build_filename(kTempDirectory, "WebpageIcons.db-journal", NULL));
    g_unlink(filename.get());

    g_rmdir(kTempDirectory);
}

void beforeAll()
{
    // Start a soup server for testing.
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    kTempDirectory = g_dir_make_tmp("WebKit2Tests-XXXXXX", 0);
    g_assert(kTempDirectory);

    // Add tests to the suite.
    FaviconDatabaseTest::add("WebKitFaviconDatabase", "set-directory", testSetDirectory);
    FaviconDatabaseTest::add("WebKitFaviconDatabase", "get-favicon", testGetFavicon);
    FaviconDatabaseTest::add("WebKitFaviconDatabase", "get-favicon-uri", testGetFaviconURI);
    FaviconDatabaseTest::add("WebKitFaviconDatabase", "clear-database", testClearDatabase);
}

void afterAll()
{
    delete kServer;
    deleteDatabaseFiles();
}
