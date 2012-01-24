/*
 * Copyright (C) 2012 Igalia S.L.
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
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "config.h"

#include "TestMain.h"
#include "WebKitTestServer.h"
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <libsoup/soup.h>
#include <string.h>
#include <webkit2/webkit2.h>
#include <wtf/Vector.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

static char* kTempDirectory;

class DownloadTest: public Test {
public:
    MAKE_GLIB_TEST_FIXTURE(DownloadTest);

    enum DownloadEvent {
        Started,
        ReceivedResponse,
        CreatedDestination,
        ReceivedData,
        Failed,
        Finished
    };

    static void receivedResponseCallback(WebKitDownload* download, GParamSpec*, DownloadTest* test)
    {
        g_assert(webkit_download_get_response(download));
        test->receivedResponse(download);
    }

    static gboolean createdDestinationCallback(WebKitDownload* download, const gchar* destination, DownloadTest* test)
    {
        g_assert(webkit_download_get_destination(download));
        g_assert_cmpstr(webkit_download_get_destination(download), ==, destination);
        test->createdDestination(download, destination);
        return TRUE;
    }

    static gboolean receivedDataCallback(WebKitDownload* download, guint64 dataLength, DownloadTest* test)
    {
        test->receivedData(download, dataLength);
        return TRUE;
    }

    static gboolean finishedCallback(WebKitDownload* download, DownloadTest* test)
    {
        test->finished(download);
        return TRUE;
    }

    static gboolean failedCallback(WebKitDownload* download, GError* error, DownloadTest* test)
    {
        g_assert(error);
        test->failed(download, error);
        return TRUE;
    }

    static gboolean decideDestinationCallback(WebKitDownload* download, const gchar* suggestedFilename, DownloadTest* test)
    {
        g_assert(suggestedFilename);
        test->decideDestination(download, suggestedFilename);
        return TRUE;
    }

    static void downloadStartedCallback(WebKitWebContext* context, WebKitDownload* download, DownloadTest* test)
    {
        test->started(download);
        g_signal_connect(download, "notify::response", G_CALLBACK(receivedResponseCallback), test);
        g_signal_connect(download, "created-destination", G_CALLBACK(createdDestinationCallback), test);
        g_signal_connect(download, "received-data", G_CALLBACK(receivedDataCallback), test);
        g_signal_connect(download, "finished", G_CALLBACK(finishedCallback), test);
        g_signal_connect(download, "failed", G_CALLBACK(failedCallback), test);
        g_signal_connect(download, "decide-destination", G_CALLBACK(decideDestinationCallback), test);
    }

    DownloadTest()
        : m_webContext(webkit_web_context_get_default())
        , m_mainLoop(g_main_loop_new(0, TRUE))
        , m_downloadSize(0)
    {
        g_signal_connect(m_webContext, "download-started", G_CALLBACK(downloadStartedCallback), this);
    }

    ~DownloadTest()
    {
        g_signal_handlers_disconnect_matched(m_webContext, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        g_main_loop_unref(m_mainLoop);
    }

    virtual void started(WebKitDownload* download)
    {
        m_downloadEvents.append(Started);
    }

    virtual void receivedResponse(WebKitDownload* download)
    {
        m_downloadEvents.append(ReceivedResponse);
    }

    virtual void createdDestination(WebKitDownload* download, const char* destination)
    {
        m_downloadEvents.append(CreatedDestination);
    }

    virtual void receivedData(WebKitDownload* download, guint64 dataLength)
    {
        m_downloadSize += dataLength;
        if (!m_downloadEvents.contains(ReceivedData))
            m_downloadEvents.append(ReceivedData);
    }

    virtual void finished(WebKitDownload* download)
    {
        m_downloadEvents.append(Finished);
        g_main_loop_quit(m_mainLoop);
    }

    virtual void failed(WebKitDownload* download, GError* error)
    {
        m_downloadEvents.append(Failed);
    }

    virtual void decideDestination(WebKitDownload* download, const gchar* suggestedFilename)
    {
        GOwnPtr<char> destination(g_build_filename(kTempDirectory, suggestedFilename, NULL));
        GOwnPtr<char> destinationURI(g_filename_to_uri(destination.get(), 0, 0));
        webkit_download_set_destination(download, destinationURI.get());
    }

    void waitUntilDownloadFinishes()
    {
        g_main_loop_run(m_mainLoop);
    }

    void checkDestinationAndDeleteFile(WebKitDownload* download, const char* expectedName)
    {
        if (!webkit_download_get_destination(download))
            return;
        GRefPtr<GFile> destFile = adoptGRef(g_file_new_for_uri(webkit_download_get_destination(download)));
        GOwnPtr<char> destBasename(g_file_get_basename(destFile.get()));
        g_assert_cmpstr(destBasename.get(), ==, expectedName);

        g_file_delete(destFile.get(), 0, 0);
    }

    WebKitWebContext* m_webContext;
    GMainLoop* m_mainLoop;
    Vector<DownloadEvent> m_downloadEvents;
    guint64 m_downloadSize;
};

static CString getWebKit1TestResoucesDir()
{
    GOwnPtr<char> resourcesDir(g_build_filename(WEBKIT_SRC_DIR, "Source", "WebKit", "gtk", "tests", "resources", NULL));
    return resourcesDir.get();
}

static void testDownloadLocalFile(DownloadTest* test, gconstpointer)
{
    GOwnPtr<char> sourcePath(g_build_filename(getWebKit1TestResoucesDir().data(), "test.pdf", NULL));
    GRefPtr<GFile> source = adoptGRef(g_file_new_for_path(sourcePath.get()));
    GRefPtr<GFileInfo> sourceInfo = adoptGRef(g_file_query_info(source.get(), G_FILE_ATTRIBUTE_STANDARD_SIZE, static_cast<GFileQueryInfoFlags>(0), 0, 0));
    GOwnPtr<char> sourceURI(g_file_get_uri(source.get()));
    GRefPtr<WebKitDownload> download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, sourceURI.get()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::CreatedDestination);
    g_assert_cmpint(events[3], ==, DownloadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, DownloadTest::Finished);

    g_assert_cmpint(test->m_downloadSize, ==, g_file_info_get_size(sourceInfo.get()));
    g_assert(webkit_download_get_destination(download.get()));
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), ==, 1);
    test->checkDestinationAndDeleteFile(download.get(), "test.pdf");
}

class DownloadErrorTest: public DownloadTest {
public:
    MAKE_GLIB_TEST_FIXTURE(DownloadErrorTest);

    DownloadErrorTest()
        : m_expectedError(WEBKIT_DOWNLOAD_ERROR_NETWORK)
    {
    }

    void receivedResponse(WebKitDownload* download)
    {
        DownloadTest::receivedResponse(download);
        if (m_expectedError == WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER)
            webkit_download_cancel(download);
    }

    void createdDestination(WebKitDownload* download, const char* destination)
    {
        g_assert_not_reached();
    }

    void failed(WebKitDownload* download, GError* error)
    {
        g_assert(g_error_matches(error, WEBKIT_DOWNLOAD_ERROR, m_expectedError));
        DownloadTest::failed(download, error);
    }

    void decideDestination(WebKitDownload* download, const gchar* suggestedFilename)
    {
        if (m_expectedError != WEBKIT_DOWNLOAD_ERROR_DESTINATION) {
            DownloadTest::decideDestination(download, suggestedFilename);
            return;
        }
        webkit_download_set_destination(download, "file:///foo/bar");
    }

    WebKitDownloadError m_expectedError;
};

static void testDownloadLocalFileError(DownloadErrorTest* test, gconstpointer)
{
    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_NETWORK;
    GRefPtr<WebKitDownload> download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, "file:///foo/bar"));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 3);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::Failed);
    g_assert_cmpint(events[2], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);

    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_DESTINATION;
    GOwnPtr<char> path(g_build_filename(getWebKit1TestResoucesDir().data(), "test.pdf", NULL));
    GRefPtr<GFile> file = adoptGRef(g_file_new_for_path(path.get()));
    GOwnPtr<char> uri(g_file_get_uri(file.get()));
    download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, uri.get()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "bar");

    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER;
    download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, uri.get()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "test.pdf");
}

static WebKitTestServer* kServer;
static const char* kServerSuggestedFilename = "webkit-downloaded-file";

static void serverCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    GOwnPtr<char> filePath(g_build_filename(getWebKit1TestResoucesDir().data(), path, NULL));
    char* contents;
    gsize contentsLength;
    if (!g_file_get_contents(filePath.get(), &contents, &contentsLength, 0)) {
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
        soup_message_body_complete(message->response_body);
        return;
    }

    soup_message_set_status(message, SOUP_STATUS_OK);

    GOwnPtr<char> contentDisposition(g_strdup_printf("filename=%s", kServerSuggestedFilename));
    soup_message_headers_append(message->response_headers, "Content-Disposition", contentDisposition.get());
    soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, contentsLength);

    soup_message_body_complete(message->response_body);
}

static void testDownloadRemoteFile(DownloadTest* test, gconstpointer)
{
    GRefPtr<WebKitDownload> download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, kServer->getURIForPath("/test.pdf").data()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 5);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::CreatedDestination);
    g_assert_cmpint(events[3], ==, DownloadTest::ReceivedData);
    g_assert_cmpint(events[4], ==, DownloadTest::Finished);
    events.clear();

    g_assert(webkit_download_get_destination(download.get()));
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), ==, 1);
    test->checkDestinationAndDeleteFile(download.get(), kServerSuggestedFilename);
}

static void testDownloadRemoteFileError(DownloadErrorTest* test, gconstpointer)
{
    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_NETWORK;
    GRefPtr<WebKitDownload> download = adoptGRef(webkit_web_context_download_uri(test->m_webContext,
                                                                                 kServer->getURIForPath("/foo").data()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    Vector<DownloadTest::DownloadEvent>& events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    WebKitURIResponse* response = webkit_download_get_response(download.get());
    g_assert_cmpuint(webkit_uri_response_get_status_code(response), ==, 404);
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);

    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_DESTINATION;
    download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, kServer->getURIForPath("/test.pdf").data()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), "bar");

    test->m_expectedError = WEBKIT_DOWNLOAD_ERROR_CANCELLED_BY_USER;
    download = adoptGRef(webkit_web_context_download_uri(test->m_webContext, kServer->getURIForPath("/test.pdf").data()));
    test->assertObjectIsDeletedWhenTestFinishes(G_OBJECT(download.get()));
    test->waitUntilDownloadFinishes();

    events = test->m_downloadEvents;
    g_assert_cmpint(events.size(), ==, 4);
    g_assert_cmpint(events[0], ==, DownloadTest::Started);
    g_assert_cmpint(events[1], ==, DownloadTest::ReceivedResponse);
    g_assert_cmpint(events[2], ==, DownloadTest::Failed);
    g_assert_cmpint(events[3], ==, DownloadTest::Finished);
    events.clear();
    g_assert_cmpfloat(webkit_download_get_estimated_progress(download.get()), <, 1);
    test->checkDestinationAndDeleteFile(download.get(), kServerSuggestedFilename);
}

void beforeAll()
{
    kServer = new WebKitTestServer();
    kServer->run(serverCallback);

    kTempDirectory = g_dir_make_tmp("WebKit2Tests-XXXXXX", 0);
    g_assert(kTempDirectory);

    DownloadTest::add("Downloads", "local-file", testDownloadLocalFile);
    DownloadErrorTest::add("Downloads", "local-file-error", testDownloadLocalFileError);
    DownloadTest::add("Downloads", "remote-file", testDownloadRemoteFile);
    DownloadErrorTest::add("Downloads", "remote-file-error", testDownloadRemoteFileError);
}

void afterAll()
{
    delete kServer;
    g_rmdir(kTempDirectory);
}
