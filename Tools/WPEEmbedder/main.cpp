#include <WebKit/WKContext.h>
#include <WebKit/WKFramePolicyListener.h>
#include <WebKit/WKPageGroup.h>
#include <WebKit/WKPageConfigurationRef.h>
#include <WebKit/WKPage.h>
#include <WebKit/WKRetainPtr.h>
#include <WebKit/WKString.h>
#include <WebKit/WKURL.h>
#include <WebKit/WKView.h>
#include <glib.h>
#include <wpe-mesa/view-backend-exportable.h>

static WKViewRef createView(WKPageConfigurationRef);

static WKPageNavigationClientV0 createPageNavigationClient()
{
    WKPageNavigationClientV0 navigationClient = {
        { 0, nullptr },
        // decidePolicyForNavigationAction
        [](WKPageRef, WKNavigationActionRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
            WKFramePolicyListenerUse(listener);
        },
        // decidePolicyForNavigationResponse
        [](WKPageRef, WKNavigationResponseRef, WKFramePolicyListenerRef listener, WKTypeRef, const void*) {
            WKFramePolicyListenerUse(listener);
        },
        nullptr, // decidePolicyForPluginLoad
        nullptr, // didStartProvisionalNavigation
        nullptr, // didReceiveServerRedirectForProvisionalNavigation
        nullptr, // didFailProvisionalNavigation
        nullptr, // didCommitNavigation
        nullptr, // didFinishNavigation
        nullptr, // didFailNavigation
        nullptr, // didFailProvisionalLoadInSubframe
        nullptr, // didFinishDocumentLoad
        nullptr, // didSameDocumentNavigation
        nullptr, // renderingProgressDidChange
        nullptr, // canAuthenticateAgainstProtectionSpace
        nullptr, // didReceiveAuthenticationChallenge
        nullptr, // webProcessDidCrash
        nullptr, // copyWebCryptoMasterKey
        nullptr, // didBeginNavigationGesture
        nullptr, // willEndNavigationGesture
        nullptr, // didEndNavigationGesture
        nullptr, // didRemoveNavigationGestureSnapshot
    };
    return navigationClient;
}

static WKViewRef createView(WKPageConfigurationRef pageConfiguration)
{
    struct wpe_view_backend* viewBackend = wpe_mesa_view_backend_exportable_create();
    auto view = WKViewCreateWithViewBackend(viewBackend, pageConfiguration);
    auto page = WKViewGetPage(view);

    auto pageNavigationClient = createPageNavigationClient();
    WKPageSetPageNavigationClient(page, &pageNavigationClient.base);

    return view;
}

int main(int argc, char* argv[])
{
    GMainLoop* loop = g_main_loop_new(g_main_context_default(), FALSE);

    auto context = adoptWK(WKContextCreate());
    auto pageGroupIdentifier = adoptWK(WKStringCreateWithUTF8CString("WPEPageGroup"));
    auto pageGroup = adoptWK(WKPageGroupCreateWithIdentifier(pageGroupIdentifier.get()));
    auto pageConfiguration = adoptWK(WKPageConfigurationCreate());
    WKPageConfigurationSetContext(pageConfiguration.get(), context.get());
    WKPageConfigurationSetPageGroup(pageConfiguration.get(), pageGroup.get());

    auto view = adoptWK(createView(pageConfiguration.get()));

    const char* url = "http://www.webkit.org/blog-files/3d-transforms/poster-circle.html";
    if (argc > 1)
        url = argv[1];

    auto shellURL = adoptWK(WKURLCreateWithUTF8CString(url));
    WKPageLoadURL(WKViewGetPage(view.get()), shellURL.get());

    g_main_loop_run(loop);

    g_main_loop_unref(loop);
    return 0;
}
