// FIXME: Copyright.

#include "config.h"
#include "WebInspectorServer.h"

#if ENABLE(INSPECTOR_SERVER)

#include "WebInspectorProxy.h"
#include "WebPageProxy.h"
#include <cstdio>
#include <gmodule.h>
#include <mutex>
#include <wtf/NeverDestroyed.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

namespace WebKit {

bool WebInspectorServer::platformResourceForPath(const String& path, Vector<char>& data, String& contentType)
{
    // The page list contains an unformated list of pages that can be inspected with a link to open a session.
    if (path == "/pagelist.json") {
        buildPageList(data, contentType);
        return true;
    }

    static std::once_flag flag;
    std::call_once(flag, [] {
        GModule* resourcesModule = g_module_open("libWPEInspectorResources.so", G_MODULE_BIND_LAZY);
        if (!resourcesModule) {
            WTFLogAlways("Error loading libWPEInspectorResources.so: %s", g_module_error());
            return;
        }

        g_module_make_resident(resourcesModule);
    });

    // Point the default path to a formatted page that queries the page list and display them.
    CString resourcePath = makeString("/org/wpe/inspector/UserInterface", (path == "/" ? "/inspectorPageIndex.html" : path)).utf8();
    if (resourcePath.isNull())
        return false;

    GUniqueOutPtr<GError> error;
    GRefPtr<GBytes> resourceBytes = adoptGRef(g_resources_lookup_data(resourcePath.data(), G_RESOURCE_LOOKUP_FLAGS_NONE, &error.outPtr()));
    if (!resourceBytes) {
        StringBuilder builder;
        builder.appendLiteral("<!DOCTYPE html><html><head></head><body>Error: ");
        builder.appendNumber(error->code);
        builder.appendLiteral(", ");
        builder.append(error->message);
        builder.appendLiteral(" occurred during fetching inspector resource files.</body></html>");

        CString errorHTML = builder.toString().utf8();
        data.append(errorHTML.data(), errorHTML.length());
        contentType = "text/html; charset=utf-8";

        WTFLogAlways("Error fetching webinspector resource files: %d, %s", error->code, error->message);
        return false;
    }

    gsize resourceDataSize;
    gconstpointer resourceData = g_bytes_get_data(resourceBytes.get(), &resourceDataSize);
    data.append(static_cast<const char*>(resourceData), resourceDataSize);

    GUniquePtr<gchar> mimeType(g_content_type_guess(resourcePath.data(), static_cast<const guchar*>(resourceData), resourceDataSize, nullptr));
    contentType = mimeType.get();
    return true;
}

void WebInspectorServer::buildPageList(Vector<char>& data, String& contentType)
{
    // chromedevtools (http://code.google.com/p/chromedevtools) 0.3.8 expected JSON format:
    // {
    //  "title": "Foo",
    //  "url": "http://foo",
    //  "devtoolsFrontendUrl": "/Main.html?ws=localhost:9222/devtools/page/1",
    //  "webSocketDebuggerUrl": "ws://localhost:9222/devtools/page/1"
    // },

    StringBuilder builder;
    builder.appendLiteral("[ ");
    ClientMap::iterator end = m_clientMap.end();
    for (ClientMap::iterator it = m_clientMap.begin(); it != end; ++it) {
        WebPageProxy* webPage = it->value->inspectedPage();
        if (it != m_clientMap.begin())
            builder.appendLiteral(", ");
        builder.appendLiteral("{ \"id\": ");
        builder.appendNumber(it->key);
        builder.appendLiteral(", \"title\": \"");
        builder.append(webPage->pageLoadState().title());
        builder.appendLiteral("\", \"url\": \"");
        builder.append(webPage->pageLoadState().activeURL());
        builder.appendLiteral("\", \"inspectorUrl\": \"");
        builder.appendLiteral("/Main.html?page=");
        builder.appendNumber(it->key);
        builder.appendLiteral("\", \"devtoolsFrontendUrl\": \"");
        builder.appendLiteral("/Main.html?ws=");
        builder.append(bindAddress());
        builder.appendLiteral(":");
        builder.appendNumber(port());
        builder.appendLiteral("/devtools/page/");
        builder.appendNumber(it->key);
        builder.appendLiteral("\", \"webSocketDebuggerUrl\": \"");
        builder.appendLiteral("ws://");
        builder.append(bindAddress());
        builder.appendLiteral(":");
        builder.appendNumber(port());
        builder.appendLiteral("/devtools/page/");
        builder.appendNumber(it->key);
        builder.appendLiteral("\" }");
    }
    builder.appendLiteral(" ]");
    CString cstr = builder.toString().utf8();
    data.append(cstr.data(), cstr.length());
    contentType = "application/json; charset=utf-8";
}

} // namespace WebKit

#endif
