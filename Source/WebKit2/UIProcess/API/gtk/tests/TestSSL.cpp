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

#include "LoadTrackingTest.h"
#include "WebKitTestServer.h"
#include <gtk/gtk.h>

static WebKitTestServer* kHttpsServer;
static WebKitTestServer* kHttpServer;

static const char* indexHTML = "<html><body>Testing WebKit2GTK+ SSL</body></htmll>";
static const char* insecureContentHTML = "<html><script src=\"%s\"></script><body><p>Text + image <img src=\"%s\" align=\"right\"/></p></body></html>";
static const char TLSExpectedSuccessTitle[] = "WebKit2Gtk+ TLS permission test";
static const char TLSSuccessHTMLString[] = "<html><head><title>WebKit2Gtk+ TLS permission test</title></head><body></body></html>";

class SSLTest: public LoadTrackingTest {
public:
    MAKE_GLIB_TEST_FIXTURE(SSLTest);

    SSLTest()
        : m_tlsErrors(static_cast<GTlsCertificateFlags>(0))
    {
    }

    virtual void provisionalLoadFailed(const gchar* failingURI, GError* error)
    {
        g_assert_error(error, SOUP_HTTP_ERROR, SOUP_STATUS_SSL_FAILED);
        LoadTrackingTest::provisionalLoadFailed(failingURI, error);
    }

    virtual void loadCommitted()
    {
        GTlsCertificate* certificate = 0;
        webkit_web_view_get_tls_info(m_webView, &certificate, &m_tlsErrors);
        m_certificate = certificate;
        LoadTrackingTest::loadCommitted();
    }

    void waitUntilLoadFinished()
    {
        m_certificate = 0;
        m_tlsErrors = static_cast<GTlsCertificateFlags>(0);
        LoadTrackingTest::waitUntilLoadFinished();
    }

    GRefPtr<GTlsCertificate> m_certificate;
    GTlsCertificateFlags m_tlsErrors;
};

static void testSSL(SSLTest* test, gconstpointer)
{
    test->loadURI(kHttpsServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_certificate);
    // We always expect errors because we are using a self-signed certificate,
    // but only G_TLS_CERTIFICATE_UNKNOWN_CA flags should be present.
    g_assert(test->m_tlsErrors);
    g_assert_cmpuint(test->m_tlsErrors, ==, G_TLS_CERTIFICATE_UNKNOWN_CA);

    // Non HTTPS loads shouldn't have a certificate nor errors.
    test->loadHtml(indexHTML, 0);
    test->waitUntilLoadFinished();
    g_assert(!test->m_certificate);
    g_assert(!test->m_tlsErrors);
}

class InsecureContentTest: public WebViewTest {
public:
    MAKE_GLIB_TEST_FIXTURE(InsecureContentTest);

    InsecureContentTest()
        : m_insecureContentRun(false)
        , m_insecureContentDisplayed(false)
    {
        g_signal_connect(m_webView, "insecure-content-detected", G_CALLBACK(insecureContentDetectedCallback), this);
    }

    static void insecureContentDetectedCallback(WebKitWebView* webView, WebKitInsecureContentEvent event, InsecureContentTest* test)
    {
        g_assert(webView == test->m_webView);

        if (event == WEBKIT_INSECURE_CONTENT_RUN)
            test->m_insecureContentRun = true;

        if (event == WEBKIT_INSECURE_CONTENT_DISPLAYED)
            test->m_insecureContentDisplayed = true;
    }

    bool m_insecureContentRun;
    bool m_insecureContentDisplayed;
};

static void testInsecureContent(InsecureContentTest* test, gconstpointer)
{
    test->loadURI(kHttpsServer->getURIForPath("/insecure-content/").data());
    test->waitUntilLoadFinished();

    g_assert(test->m_insecureContentRun);
    g_assert(test->m_insecureContentDisplayed);
}

static void testTLSErrorsPolicy(SSLTest* test, gconstpointer)
{
    WebKitWebContext* context = webkit_web_view_get_context(test->m_webView);
    // TLS errors are ignored by default.
    g_assert(webkit_web_context_get_tls_errors_policy(context) == WEBKIT_TLS_ERRORS_POLICY_IGNORE);
    test->loadURI(kHttpsServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert(!test->m_loadFailed);

    webkit_web_context_set_tls_errors_policy(context, WEBKIT_TLS_ERRORS_POLICY_FAIL);
    test->loadURI(kHttpsServer->getURIForPath("/").data());
    test->waitUntilLoadFinished();
    g_assert(test->m_loadFailed);
    g_assert(test->m_loadEvents.contains(LoadTrackingTest::ProvisionalLoadFailed));
    g_assert(!test->m_loadEvents.contains(LoadTrackingTest::LoadCommitted));
}

class TLSErrorsTest: public SSLTest {
public:
    MAKE_GLIB_TEST_FIXTURE(TLSErrorsTest);

    TLSErrorsTest()
    {
        g_signal_connect(m_webView, "load-failed-with-tls-errors", G_CALLBACK(runLoadFailedWithTLSErrorsCallback), this);
    }

    ~TLSErrorsTest()
    {
        g_signal_handlers_disconnect_matched(m_webView, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        if (m_certificateInfo)
            webkit_certificate_info_free(m_certificateInfo);
    }

    static gboolean runLoadFailedWithTLSErrorsCallback(WebKitWebView*, WebKitCertificateInfo* info, const char* host, TLSErrorsTest* test)
    {
        test->runLoadFailedWithTLSErrors(info, host);
        return TRUE;
    }

    void runLoadFailedWithTLSErrors(WebKitCertificateInfo* info, const char* host)
    {
        if (m_certificateInfo)
            webkit_certificate_info_free(m_certificateInfo);
        m_certificateInfo = webkit_certificate_info_copy(info);
        m_host.set(g_strdup(host));
        g_main_loop_quit(m_mainLoop);
    }

    void waitUntilLoadFailedWithTLSErrors()
    {
        g_main_loop_run(m_mainLoop);
    }

    WebKitCertificateInfo* certificateInfo()
    {
        return m_certificateInfo;
    }

    const char* host()
    {
        return m_host.get();
    }

private:
    WebKitCertificateInfo* m_certificateInfo;
    GOwnPtr<char> m_host;
};

static void testLoadFailedWithTLSErrors(TLSErrorsTest* test, gconstpointer)
{
    WebKitWebContext* context = webkit_web_view_get_context(test->m_webView);
    webkit_web_context_set_tls_errors_policy(context, WEBKIT_TLS_ERRORS_POLICY_FAIL);

    // The load-failed-with-tls-errors signal should be emitted when there is a TLS failure.
    test->loadURI(kHttpsServer->getURIForPath("/test-tls/").data());
    test->waitUntilLoadFailedWithTLSErrors();
    // Test the WebKitCertificateInfo API.
    g_assert(G_IS_TLS_CERTIFICATE(webkit_certificate_info_get_tls_certificate(test->certificateInfo())));
    g_assert_cmpuint(webkit_certificate_info_get_tls_errors(test->certificateInfo()), ==, G_TLS_CERTIFICATE_UNKNOWN_CA);
    g_assert_cmpstr(test->host(), ==, soup_uri_get_host(kHttpsServer->baseURI()));
    g_assert_cmpint(test->m_loadEvents[0], ==, LoadTrackingTest::ProvisionalLoadStarted);
    g_assert_cmpint(test->m_loadEvents[1], ==, LoadTrackingTest::LoadFinished);

    // Test allowing an exception for this certificate on this host.
    webkit_web_context_allow_tls_certificate_for_host(context, test->certificateInfo(), test->host());
    // The page should now load without errors.
    test->loadURI(kHttpsServer->getURIForPath("/test-tls/").data());
    test->waitUntilLoadFinished();

    g_assert_cmpint(test->m_loadEvents[0], ==, LoadTrackingTest::ProvisionalLoadStarted);
    g_assert_cmpint(test->m_loadEvents[1], ==, LoadTrackingTest::LoadCommitted);
    g_assert_cmpint(test->m_loadEvents[2], ==, LoadTrackingTest::LoadFinished);
    g_assert_cmpstr(webkit_web_view_get_title(test->m_webView), ==, TLSExpectedSuccessTitle);
}


static void httpsServerCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/")) {
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, indexHTML, strlen(indexHTML));
        soup_message_body_complete(message->response_body);
    } else if (g_str_equal(path, "/insecure-content/")) {
        GOwnPtr<char> responseHTML(g_strdup_printf(insecureContentHTML, kHttpServer->getURIForPath("/test-script").data(), kHttpServer->getURIForPath("/test-image").data()));
        soup_message_body_append(message->response_body, SOUP_MEMORY_COPY, responseHTML.get(), strlen(responseHTML.get()));
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_complete(message->response_body);
    } else if (g_str_equal(path, "/test-tls/")) {
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_append(message->response_body, SOUP_MEMORY_STATIC, TLSSuccessHTMLString, strlen(TLSSuccessHTMLString));
        soup_message_body_complete(message->response_body);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

static void httpServerCallback(SoupServer* server, SoupMessage* message, const char* path, GHashTable*, SoupClientContext*, gpointer)
{
    if (message->method != SOUP_METHOD_GET) {
        soup_message_set_status(message, SOUP_STATUS_NOT_IMPLEMENTED);
        return;
    }

    if (g_str_equal(path, "/test-script")) {
        GOwnPtr<char> pathToFile(g_build_filename(Test::getResourcesDir().data(), "link-title.js", NULL));
        char* contents;
        gsize length;
        g_file_get_contents(pathToFile.get(), &contents, &length, 0);

        soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, length);
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_complete(message->response_body);
    } else if (g_str_equal(path, "/test-image")) {
        GOwnPtr<char> pathToFile(g_build_filename(Test::getWebKit1TestResoucesDir().data(), "blank.ico", NULL));
        char* contents;
        gsize length;
        g_file_get_contents(pathToFile.get(), &contents, &length, 0);

        soup_message_body_append(message->response_body, SOUP_MEMORY_TAKE, contents, length);
        soup_message_set_status(message, SOUP_STATUS_OK);
        soup_message_body_complete(message->response_body);
    } else
        soup_message_set_status(message, SOUP_STATUS_NOT_FOUND);
}

void beforeAll()
{
    kHttpsServer = new WebKitTestServer(WebKitTestServer::ServerHTTPS);
    kHttpsServer->run(httpsServerCallback);

    kHttpServer = new WebKitTestServer(WebKitTestServer::ServerHTTP);
    kHttpServer->run(httpServerCallback);

    SSLTest::add("WebKitWebView", "ssl", testSSL);
    InsecureContentTest::add("WebKitWebView", "insecure-content", testInsecureContent);
    // In this case the order of the tests does matter because tls-errors-policy tests the default policy,
    // and expects that no exception will have been added for this certificate and host pair as is
    // done in the tls-permission-request test.
    SSLTest::add("WebKitWebView", "tls-errors-policy", testTLSErrorsPolicy);
    TLSErrorsTest::add("WebKitWebView", "load-failed-with-tls-errors", testLoadFailedWithTLSErrors);
}

void afterAll()
{
    delete kHttpsServer;
    delete kHttpServer;
}
