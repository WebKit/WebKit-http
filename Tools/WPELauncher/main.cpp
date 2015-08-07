#include <WebKit/WKContextConfigurationRef.h>
#include <WebKit/WKContext.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>
#include <glib.h>

int main()
{
    GMainLoop* loop = g_main_loop_new(g_main_context_default(), FALSE);

    auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
    auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));

    auto context = adoptWK(WKContextCreate());

    auto view = adoptWK(WKViewCreate(context.get(), pageGroup.get()));
    WKViewResize(view.get(), WKSizeMake(800, 600));

    const char* url = "http://www.webkit.org/blog-files/3d-transforms/poster-circle.html";
    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(view.get()), shellURL.get());

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    return 0;
}
