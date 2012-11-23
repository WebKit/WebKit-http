/*
 * Copyright (C) 2011 Igalia S.L.
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
#include "WebKitWebContext.h"

#include "WebKitCookieManagerPrivate.h"
#include "WebKitDownloadClient.h"
#include "WebKitDownloadPrivate.h"
#include "WebKitFaviconDatabasePrivate.h"
#include "WebKitGeolocationProvider.h"
#include "WebKitPluginPrivate.h"
#include "WebKitPrivate.h"
#include "WebKitRequestManagerClient.h"
#include "WebKitSecurityManagerPrivate.h"
#include "WebKitTextChecker.h"
#include "WebKitURISchemeRequestPrivate.h"
#include "WebKitWebContextPrivate.h"
#include "WebResourceCacheManagerProxy.h"
#include <WebCore/FileSystem.h>
#include <WebCore/IconDatabase.h>
#include <WebCore/Language.h>
#include <wtf/HashMap.h>
#include <wtf/OwnPtr.h>
#include <wtf/PassRefPtr.h>
#include <wtf/RefCounted.h>
#include <wtf/gobject/GOwnPtr.h>
#include <wtf/gobject/GRefPtr.h>
#include <wtf/text/CString.h>

using namespace WebKit;

enum {
    DOWNLOAD_STARTED,

    LAST_SIGNAL
};

class WebKitURISchemeHandler: public RefCounted<WebKitURISchemeHandler> {
public:
    WebKitURISchemeHandler()
        : m_callback(0)
        , m_userData(0)
        , m_destroyNotify(0)
    {
    }
    WebKitURISchemeHandler(WebKitURISchemeRequestCallback callback, void* userData, GDestroyNotify destroyNotify)
        : m_callback(callback)
        , m_userData(userData)
        , m_destroyNotify(destroyNotify)
    {
    }

    ~WebKitURISchemeHandler()
    {
        if (m_destroyNotify)
            m_destroyNotify(m_userData);
    }

    bool hasCallback()
    {
        return m_callback;
    }

    void performCallback(WebKitURISchemeRequest* request)
    {
        ASSERT(m_callback);

        m_callback(request, m_userData);
    }

private:
    WebKitURISchemeRequestCallback m_callback;
    void* m_userData;
    GDestroyNotify m_destroyNotify;
};

typedef HashMap<String, RefPtr<WebKitURISchemeHandler> > URISchemeHandlerMap;
typedef HashMap<uint64_t, GRefPtr<WebKitURISchemeRequest> > URISchemeRequestMap;

struct _WebKitWebContextPrivate {
    RefPtr<WebContext> context;

    GRefPtr<WebKitCookieManager> cookieManager;
    GRefPtr<WebKitFaviconDatabase> faviconDatabase;
    GRefPtr<WebKitSecurityManager> securityManager;
    RefPtr<WebSoupRequestManagerProxy> requestManager;
    URISchemeHandlerMap uriSchemeHandlers;
    URISchemeRequestMap uriSchemeRequests;
#if ENABLE(GEOLOCATION)
    RefPtr<WebKitGeolocationProvider> geolocationProvider;
#endif
#if ENABLE(SPELLCHECK)
    OwnPtr<WebKitTextChecker> textChecker;
#endif
    CString faviconDatabaseDirectory;
};

static guint signals[LAST_SIGNAL] = { 0, };

WEBKIT_DEFINE_TYPE(WebKitWebContext, webkit_web_context, G_TYPE_OBJECT)

static void webkit_web_context_class_init(WebKitWebContextClass* webContextClass)
{
    GObjectClass* gObjectClass = G_OBJECT_CLASS(webContextClass);

    /**
     * WebKitWebContext::download-started:
     * @context: the #WebKitWebContext
     * @download: the #WebKitDownload associated with this event
     *
     * This signal is emitted when a new download request is made.
     */
    signals[DOWNLOAD_STARTED] =
        g_signal_new("download-started",
                     G_TYPE_FROM_CLASS(gObjectClass),
                     G_SIGNAL_RUN_LAST,
                     0, 0, 0,
                     g_cclosure_marshal_VOID__OBJECT,
                     G_TYPE_NONE, 1,
                     WEBKIT_TYPE_DOWNLOAD);
}

static gpointer createDefaultWebContext(gpointer)
{
    static GRefPtr<WebKitWebContext> webContext = adoptGRef(WEBKIT_WEB_CONTEXT(g_object_new(WEBKIT_TYPE_WEB_CONTEXT, NULL)));
    WebKitWebContextPrivate* priv = webContext->priv;

    priv->context = WebContext::create(String());
    priv->requestManager = webContext->priv->context->soupRequestManagerProxy();
    priv->context->setCacheModel(CacheModelPrimaryWebBrowser);

    attachDownloadClientToContext(webContext.get());
    attachRequestManagerClientToContext(webContext.get());

#if ENABLE(GEOLOCATION)
    priv->geolocationProvider = WebKitGeolocationProvider::create(priv->context->geolocationManagerProxy());
#endif
#if ENABLE(SPELLCHECK)
    priv->textChecker = WebKitTextChecker::create();
#endif
    return webContext.get();
}

/**
 * webkit_web_context_get_default:
 *
 * Gets the default web context
 *
 * Returns: (transfer none): a #WebKitWebContext
 */
WebKitWebContext* webkit_web_context_get_default(void)
{
    static GOnce onceInit = G_ONCE_INIT;
    return WEBKIT_WEB_CONTEXT(g_once(&onceInit, createDefaultWebContext, 0));
}

/**
 * webkit_web_context_set_cache_model:
 * @context: the #WebKitWebContext
 * @cache_model: a #WebKitCacheModel
 *
 * Specifies a usage model for WebViews, which WebKit will use to
 * determine its caching behavior. All web views follow the cache
 * model. This cache model determines the RAM and disk space to use
 * for caching previously viewed content .
 *
 * Research indicates that users tend to browse within clusters of
 * documents that hold resources in common, and to revisit previously
 * visited documents. WebKit and the frameworks below it include
 * built-in caches that take advantage of these patterns,
 * substantially improving document load speed in browsing
 * situations. The WebKit cache model controls the behaviors of all of
 * these caches, including various WebCore caches.
 *
 * Browsers can improve document load speed substantially by
 * specifying %WEBKIT_CACHE_MODEL_WEB_BROWSER. Applications without a
 * browsing interface can reduce memory usage substantially by
 * specifying %WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER. The default value is
 * %WEBKIT_CACHE_MODEL_WEB_BROWSER.
 */
void webkit_web_context_set_cache_model(WebKitWebContext* context, WebKitCacheModel model)
{
    CacheModel cacheModel;

    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

    switch (model) {
    case WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER:
        cacheModel = CacheModelDocumentViewer;
        break;
    case WEBKIT_CACHE_MODEL_WEB_BROWSER:
        cacheModel = CacheModelPrimaryWebBrowser;
        break;
    case WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER:
        cacheModel = CacheModelDocumentBrowser;
        break;
    default:
        g_assert_not_reached();
    }

    if (cacheModel != context->priv->context->cacheModel())
        context->priv->context->setCacheModel(cacheModel);
}

/**
 * webkit_web_context_get_cache_model:
 * @context: the #WebKitWebContext
 *
 * Returns the current cache model. For more information about this
 * value check the documentation of the function
 * webkit_web_context_set_cache_model().
 *
 * Returns: the current #WebKitCacheModel
 */
WebKitCacheModel webkit_web_context_get_cache_model(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), WEBKIT_CACHE_MODEL_WEB_BROWSER);

    switch (context->priv->context->cacheModel()) {
    case CacheModelDocumentViewer:
        return WEBKIT_CACHE_MODEL_DOCUMENT_VIEWER;
    case CacheModelPrimaryWebBrowser:
        return WEBKIT_CACHE_MODEL_WEB_BROWSER;
    case CacheModelDocumentBrowser:
        return WEBKIT_CACHE_MODEL_DOCUMENT_BROWSER;
    default:
        g_assert_not_reached();
    }

    return WEBKIT_CACHE_MODEL_WEB_BROWSER;
}

/**
 * webkit_web_context_clear_cache:
 * @context: a #WebKitWebContext
 *
 * Clears all resources currently cached.
 * See also webkit_web_context_set_cache_model().
 */
void webkit_web_context_clear_cache(WebKitWebContext* context)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

    context->priv->context->resourceCacheManagerProxy()->clearCacheForAllOrigins(AllResourceCaches);
}

typedef HashMap<DownloadProxy*, GRefPtr<WebKitDownload> > DownloadsMap;

static DownloadsMap& downloadsMap()
{
    DEFINE_STATIC_LOCAL(DownloadsMap, downloads, ());
    return downloads;
}

/**
 * webkit_web_context_download_uri:
 * @context: a #WebKitWebContext
 * @uri: the URI to download
 *
 * Requests downloading of the specified URI string. The download operation
 * will not be associated to any #WebKitWebView, if you are interested in
 * starting a download from a particular #WebKitWebView use
 * webkit_web_view_download_uri() instead.
 *
 * Returns: (transfer full): a new #WebKitDownload representing the
 *    the download operation.
 */
WebKitDownload* webkit_web_context_download_uri(WebKitWebContext* context, const gchar* uri)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);
    g_return_val_if_fail(uri, 0);

    return webkitWebContextStartDownload(context, uri, 0);
}

/**
 * webkit_web_context_get_cookie_manager:
 * @context: a #WebKitWebContext
 *
 * Get the #WebKitCookieManager of @context.
 *
 * Returns: (transfer none): the #WebKitCookieManager of @context.
 */
WebKitCookieManager* webkit_web_context_get_cookie_manager(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

    WebKitWebContextPrivate* priv = context->priv;
    if (!priv->cookieManager)
        priv->cookieManager = adoptGRef(webkitCookieManagerCreate(priv->context->cookieManagerProxy()));

    return priv->cookieManager.get();
}

static void ensureFaviconDatabase(WebKitWebContext* context)
{
    WebKitWebContextPrivate* priv = context->priv;
    if (priv->faviconDatabase)
        return;

    priv->faviconDatabase = adoptGRef(webkitFaviconDatabaseCreate(priv->context->iconDatabase()));
}

/**
 * webkit_web_context_set_favicon_database_directory:
 * @context: a #WebKitWebContext
 * @path: (allow-none): an absolute path to the icon database
 * directory or %NULL to use the defaults
 *
 * Set the directory path to be used to store the favicons database
 * for @context on disk. Passing %NULL as @path means using the
 * default directory for the platform (see g_get_user_data_dir()).
 *
 * Calling this method also means enabling the favicons database for
 * its use from the applications, so that's why it's expected to be
 * called only once. Further calls for the same instance of
 * #WebKitWebContext won't cause any effect.
 */
void webkit_web_context_set_favicon_database_directory(WebKitWebContext* context, const gchar* path)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

    WebKitWebContextPrivate* priv = context->priv;
    WebIconDatabase* iconDatabase = priv->context->iconDatabase();
    if (iconDatabase->isOpen())
        return;

    ensureFaviconDatabase(context);

    // Use default if 0 is passed as parameter.
    String directoryPath = WebCore::filenameToString(path);
    priv->faviconDatabaseDirectory = directoryPath.isEmpty()
        ? priv->context->iconDatabasePath().utf8()
        : directoryPath.utf8();

    // Build the full path to the icon database file on disk.
    GOwnPtr<gchar> faviconDatabasePath(g_build_filename(priv->faviconDatabaseDirectory.data(),
                                                        WebCore::IconDatabase::defaultDatabaseFilename().utf8().data(),
                                                        NULL));

    // Setting the path will cause the icon database to be opened.
    priv->context->setIconDatabasePath(WebCore::filenameToString(faviconDatabasePath.get()));
}

/**
 * webkit_web_context_get_favicon_database_directory:
 * @context: a #WebKitWebContext
 *
 * Get the directory path being used to store the favicons database
 * for @context, or %NULL if
 * webkit_web_context_set_favicon_database_directory() hasn't been
 * called yet.
 *
 * This function will always return the same path after having called
 * webkit_web_context_set_favicon_database_directory() for the first
 * time.
 *
 * Returns: (transfer none): the path of the directory of the favicons
 * database associated with @context, or %NULL.
 */
const gchar* webkit_web_context_get_favicon_database_directory(WebKitWebContext *context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

    WebKitWebContextPrivate* priv = context->priv;
    if (priv->faviconDatabaseDirectory.isNull())
        return 0;

    return priv->faviconDatabaseDirectory.data();
}

/**
 * webkit_web_context_get_favicon_database:
 * @context: a #WebKitWebContext
 *
 * Get the #WebKitFaviconDatabase associated with @context.
 *
 * To initialize the database you need to call
 * webkit_web_context_set_favicon_database_directory().
 *
 * Returns: (transfer none): the #WebKitFaviconDatabase of @context.
 */
WebKitFaviconDatabase* webkit_web_context_get_favicon_database(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

    ensureFaviconDatabase(context);
    return context->priv->faviconDatabase.get();
}

/**
 * webkit_web_context_get_security_manager:
 * @context: a #WebKitWebContext
 *
 * Get the #WebKitSecurityManager of @context.
 *
 * Returns: (transfer none): the #WebKitSecurityManager of @context.
 */
WebKitSecurityManager* webkit_web_context_get_security_manager(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

    WebKitWebContextPrivate* priv = context->priv;
    if (!priv->securityManager)
        priv->securityManager = adoptGRef(webkitSecurityManagerCreate(context));

    return priv->securityManager.get();
}

/**
 * webkit_web_context_set_additional_plugins_directory:
 * @context: a #WebKitWebContext
 * @directory: the directory to add
 *
 * Set an additional directory where WebKit will look for plugins.
 */
void webkit_web_context_set_additional_plugins_directory(WebKitWebContext* context, const char* directory)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));
    g_return_if_fail(directory);

    context->priv->context->setAdditionalPluginsDirectory(WebCore::filenameToString(directory));
}

struct GetPluginsAsyncData {
    Vector<PluginModuleInfo> plugins;
};
WEBKIT_DEFINE_ASYNC_DATA_STRUCT(GetPluginsAsyncData)

static void webkitWebContextGetPluginThread(GSimpleAsyncResult* result, GObject* object, GCancellable*)
{
    GetPluginsAsyncData* data = static_cast<GetPluginsAsyncData*>(g_simple_async_result_get_op_res_gpointer(result));
    data->plugins = WEBKIT_WEB_CONTEXT(object)->priv->context->pluginInfoStore().plugins();
}

/**
 * webkit_web_context_get_plugins:
 * @context: a #WebKitWebContext
 * @cancellable: (allow-none): a #GCancellable or %NULL to ignore
 * @callback: (scope async): a #GAsyncReadyCallback to call when the request is satisfied
 * @user_data: (closure): the data to pass to callback function
 *
 * Asynchronously get the list of installed plugins.
 *
 * When the operation is finished, @callback will be called. You can then call
 * webkit_web_context_get_plugins_finish() to get the result of the operation.
 */
void webkit_web_context_get_plugins(WebKitWebContext* context, GCancellable* cancellable, GAsyncReadyCallback callback, gpointer userData)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

    GRefPtr<GSimpleAsyncResult> result = adoptGRef(g_simple_async_result_new(G_OBJECT(context), callback, userData,
                                                                             reinterpret_cast<gpointer>(webkit_web_context_get_plugins)));
    g_simple_async_result_set_op_res_gpointer(result.get(), createGetPluginsAsyncData(),
                                              reinterpret_cast<GDestroyNotify>(destroyGetPluginsAsyncData));
    g_simple_async_result_run_in_thread(result.get(), webkitWebContextGetPluginThread, G_PRIORITY_DEFAULT, cancellable);
}

/**
 * webkit_web_context_get_plugins_finish:
 * @context: a #WebKitWebContext
 * @result: a #GAsyncResult
 * @error: return location for error or %NULL to ignore
 *
 * Finish an asynchronous operation started with webkit_web_context_get_plugins.
 *
 * Returns: (element-type WebKitPlugin) (transfer full): a #GList of #WebKitPlugin. You must free the #GList with
 *    g_list_free() and unref the #WebKitPlugin<!-- -->s with g_object_unref() when you're done with them.
 */
GList* webkit_web_context_get_plugins_finish(WebKitWebContext* context, GAsyncResult* result, GError** error)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);
    g_return_val_if_fail(G_IS_ASYNC_RESULT(result), 0);

    GSimpleAsyncResult* simpleResult = G_SIMPLE_ASYNC_RESULT(result);
    g_warn_if_fail(g_simple_async_result_get_source_tag(simpleResult) == webkit_web_context_get_plugins);

    if (g_simple_async_result_propagate_error(simpleResult, error))
        return 0;

    GetPluginsAsyncData* data = static_cast<GetPluginsAsyncData*>(g_simple_async_result_get_op_res_gpointer(simpleResult));
    GList* plugins = 0;
    for (size_t i = 0; i < data->plugins.size(); ++i)
        plugins = g_list_prepend(plugins, webkitPluginCreate(data->plugins[i]));

    return plugins;
}

/**
 * webkit_web_context_register_uri_scheme:
 * @context: a #WebKitWebContext
 * @scheme: the network scheme to register
 * @callback: (scope async): a #WebKitURISchemeRequestCallback
 * @user_data: data to pass to callback function
 * @user_data_destroy_func: destroy notify for @user_data
 *
 * Register @scheme in @context, so that when an URI request with @scheme is made in the
 * #WebKitWebContext, the #WebKitURISchemeRequestCallback registered will be called with a
 * #WebKitURISchemeRequest.
 * It is possible to handle URI scheme requests asynchronously, by calling g_object_ref() on the
 * #WebKitURISchemeRequest and calling webkit_uri_scheme_request_finish() later when the data of
 * the request is available.
 *
 * <informalexample><programlisting>
 * static void
 * about_uri_scheme_request_cb (WebKitURISchemeRequest *request,
 *                              gpointer                user_data)
 * {
 *     GInputStream *stream;
 *     gsize         stream_length;
 *     const gchar  *path;
 *
 *     path = webkit_uri_scheme_request_get_path (request);
 *     if (!g_strcmp0 (path, "plugins")) {
 *         /<!-- -->* Create a GInputStream with the contents of plugins about page, and set its length to stream_length *<!-- -->/
 *     } else if (!g_strcmp0 (path, "memory")) {
 *         /<!-- -->* Create a GInputStream with the contents of memory about page, and set its length to stream_length *<!-- -->/
 *     } else if (!g_strcmp0 (path, "applications")) {
 *         /<!-- -->* Create a GInputStream with the contents of applications about page, and set its length to stream_length *<!-- -->/
 *     } else {
 *         gchar *contents;
 *
 *         contents = g_strdup_printf ("&lt;html&gt;&lt;body&gt;&lt;p&gt;Invalid about:%s page&lt;/p&gt;&lt;/body&gt;&lt;/html&gt;", path);
 *         stream_length = strlen (contents);
 *         stream = g_memory_input_stream_new_from_data (contents, stream_length, g_free);
 *     }
 *     webkit_uri_scheme_request_finish (request, stream, stream_length, "text/html");
 *     g_object_unref (stream);
 * }
 * </programlisting></informalexample>
 */
void webkit_web_context_register_uri_scheme(WebKitWebContext* context, const char* scheme, WebKitURISchemeRequestCallback callback, gpointer userData, GDestroyNotify destroyNotify)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));
    g_return_if_fail(scheme);
    g_return_if_fail(callback);

    RefPtr<WebKitURISchemeHandler> handler = adoptRef(new WebKitURISchemeHandler(callback, userData, destroyNotify));
    context->priv->uriSchemeHandlers.set(String::fromUTF8(scheme), handler.get());
    context->priv->requestManager->registerURIScheme(String::fromUTF8(scheme));
}

/**
 * webkit_web_context_get_spell_checking_enabled:
 * @context: a #WebKitWebContext
 *
 * Get whether spell checking feature is currently enabled.
 *
 * Returns: %TRUE If spell checking is enabled, or %FALSE otherwise.
 */
gboolean webkit_web_context_get_spell_checking_enabled(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), FALSE);

#if ENABLE(SPELLCHECK)
    return context->priv->textChecker->isSpellCheckingEnabled();
#else
    return false;
#endif
}

/**
 * webkit_web_context_set_spell_checking_enabled:
 * @context: a #WebKitWebContext
 * @enabled: Value to be set
 *
 * Enable or disable the spell checking feature.
 */
void webkit_web_context_set_spell_checking_enabled(WebKitWebContext* context, gboolean enabled)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

#if ENABLE(SPELLCHECK)
    context->priv->textChecker->setSpellCheckingEnabled(enabled);
#endif
}

/**
 * webkit_web_context_get_spell_checking_languages:
 * @context: a #WebKitWebContext
 *
 * Get the the list of spell checking languages associated with
 * @context, or %NULL if no languages have been previously set.
 *
 * See webkit_web_context_set_spell_checking_languages() for more
 * details on the format of the languages in the list.
 *
 * Returns: (array zero-terminated=1) (element-type utf8) (transfer none): A %NULL-terminated
 *    array of languages if available, or %NULL otherwise.
 */
const gchar* const* webkit_web_context_get_spell_checking_languages(WebKitWebContext* context)
{
    g_return_val_if_fail(WEBKIT_IS_WEB_CONTEXT(context), 0);

#if ENABLE(SPELLCHECK)
    return context->priv->textChecker->getSpellCheckingLanguages();
#else
    return 0;
#endif
}

/**
 * webkit_web_context_set_spell_checking_languages:
 * @context: a #WebKitWebContext
 * @languages: (array zero-terminated=1) (transfer none): a %NULL-terminated list of spell checking languages
 *
 * Set the list of spell checking languages to be used for spell
 * checking.
 *
 * The locale string typically is in the form lang_COUNTRY, where lang
 * is an ISO-639 language code, and COUNTRY is an ISO-3166 country code.
 * For instance, sv_FI for Swedish as written in Finland or pt_BR
 * for Portuguese as written in Brazil.
 *
 * You need to call this function with a valid list of languages at
 * least once in order to properly enable the spell checking feature
 * in WebKit.
 */
void webkit_web_context_set_spell_checking_languages(WebKitWebContext* context, const gchar* const* languages)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));
    g_return_if_fail(languages);

#if ENABLE(SPELLCHECK)
    context->priv->textChecker->setSpellCheckingLanguages(languages);
#endif
}

/**
 * webkit_web_context_set_preferred_languages:
 * @context: a #WebKitWebContext
 * @languages: (allow-none) (array zero-terminated=1) (element-type utf8) (transfer none): a %NULL-terminated list of language identifiers
 *
 * Set the list of preferred languages, sorted from most desirable
 * to least desirable. The list will be used to build the "Accept-Language"
 * header that will be included in the network requests started by
 * the #WebKitWebContext.
 */
void webkit_web_context_set_preferred_languages(WebKitWebContext* context, const gchar* const* languageList)
{
    g_return_if_fail(WEBKIT_IS_WEB_CONTEXT(context));

    if (!languageList || !g_strv_length(const_cast<char**>(languageList)))
        return;

    Vector<String> languages;
    for (size_t i = 0; languageList[i]; ++i)
        languages.append(String::fromUTF8(languageList[i]).lower().replace("_", "-"));

    WebCore::overrideUserPreferredLanguages(languages);
    WebCore::languageDidChange();
}

WebKitDownload* webkitWebContextGetOrCreateDownload(DownloadProxy* downloadProxy)
{
    GRefPtr<WebKitDownload> download = downloadsMap().get(downloadProxy);
    if (download)
        return download.get();

    download = adoptGRef(webkitDownloadCreate(downloadProxy));
    downloadsMap().set(downloadProxy, download.get());
    return download.get();
}

WebKitDownload* webkitWebContextStartDownload(WebKitWebContext* context, const char* uri, WebPageProxy* initiatingPage)
{
    DownloadProxy* downloadProxy = context->priv->context->download(initiatingPage, WebCore::ResourceRequest(String::fromUTF8(uri)));
    WebKitDownload* download = webkitDownloadCreate(downloadProxy);
    downloadsMap().set(downloadProxy, download);
    return download;
}

void webkitWebContextRemoveDownload(DownloadProxy* downloadProxy)
{
    downloadsMap().remove(downloadProxy);
}

void webkitWebContextDownloadStarted(WebKitWebContext* context, WebKitDownload* download)
{
    g_signal_emit(context, signals[DOWNLOAD_STARTED], 0, download);
}

WebContext* webkitWebContextGetContext(WebKitWebContext* context)
{
    g_assert(WEBKIT_IS_WEB_CONTEXT(context));

    return context->priv->context.get();
}

WebSoupRequestManagerProxy* webkitWebContextGetRequestManager(WebKitWebContext* context)
{
    return context->priv->requestManager.get();
}

void webkitWebContextReceivedURIRequest(WebKitWebContext* context, WebKitURISchemeRequest* request)
{
    String scheme(String::fromUTF8(webkit_uri_scheme_request_get_scheme(request)));
    RefPtr<WebKitURISchemeHandler> handler = context->priv->uriSchemeHandlers.get(scheme);
    ASSERT(handler.get());
    if (!handler->hasCallback())
        return;

    context->priv->uriSchemeRequests.set(webkitURISchemeRequestGetID(request), request);
    handler->performCallback(request);
}

void webkitWebContextDidFailToLoadURIRequest(WebKitWebContext* context, uint64_t requestID)
{
    GRefPtr<WebKitURISchemeRequest> request = context->priv->uriSchemeRequests.get(requestID);
    if (!request.get())
        return;
    webkitURISchemeRequestCancel(request.get());
}

void webkitWebContextDidFinishURIRequest(WebKitWebContext* context, uint64_t requestID)
{
    context->priv->uriSchemeRequests.remove(requestID);
}
