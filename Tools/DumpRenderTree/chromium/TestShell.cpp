/*
 * Copyright (C) 2010 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "TestShell.h"

#include "DRTDevToolsAgent.h"
#include "DRTDevToolsClient.h"
#include "LayoutTestController.h"
#include "WebArrayBufferView.h"
#include "WebDataSource.h"
#include "WebDocument.h"
#include "WebElement.h"
#include "WebFrame.h"
#include "WebHistoryItem.h"
#include "WebIDBFactory.h"
#include "WebTestingSupport.h"
#include "WebKit.h"
#include "WebPermissions.h"
#include "WebPoint.h"
#include "WebRuntimeFeatures.h"
#include "WebScriptController.h"
#include "WebSettings.h"
#include "WebSize.h"
#include "WebSpeechInputControllerMock.h"
#include "WebString.h"
#include "WebURLRequest.h"
#include "WebURLResponse.h"
#include "WebView.h"
#include "WebViewHost.h"
#include "skia/ext/platform_canvas.h"
#include "webkit/support/webkit_support.h"
#include "webkit/support/webkit_support_gfx.h"
#include <algorithm>
#include <cctype>
#include <vector>
#include <wtf/MD5.h>

using namespace WebKit;
using namespace std;

// Content area size for newly created windows.
static const int testWindowWidth = 800;
static const int testWindowHeight = 600;

// The W3C SVG layout tests use a different size than the other layout tests.
static const int SVGTestWindowWidth = 480;
static const int SVGTestWindowHeight = 360;

static const char layoutTestsPattern[] = "/LayoutTests/";
static const string::size_type layoutTestsPatternSize = sizeof(layoutTestsPattern) - 1;
static const char fileUrlPattern[] = "file:/";
static const char fileTestPrefix[] = "(file test):";
static const char dataUrlPattern[] = "data:";
static const string::size_type dataUrlPatternSize = sizeof(dataUrlPattern) - 1;

// FIXME: Move this to a common place so that it can be shared with
// WebCore::TransparencyWin::makeLayerOpaque().
static void makeCanvasOpaque(SkCanvas* canvas)
{
    const SkBitmap& bitmap = canvas->getTopDevice()->accessBitmap(true);
    ASSERT(bitmap.config() == SkBitmap::kARGB_8888_Config);

    SkAutoLockPixels lock(bitmap);
    for (int y = 0; y < bitmap.height(); y++) {
        uint32_t* row = bitmap.getAddr32(0, y);
        for (int x = 0; x < bitmap.width(); x++)
            row[x] |= 0xFF000000; // Set alpha bits to 1.
    }
}

TestShell::TestShell(bool testShellMode)
    : m_testIsPending(false)
    , m_testIsPreparing(false)
    , m_focusedWidget(0)
    , m_testShellMode(testShellMode)
    , m_devTools(0)
    , m_allowExternalPages(false)
    , m_acceleratedCompositingEnabled(false)
    , m_compositeToTexture(false)
    , m_forceCompositingMode(false)
    , m_accelerated2dCanvasEnabled(false)
    , m_legacyAccelerated2dCanvasEnabled(false)
    , m_acceleratedDrawingEnabled(false)
    , m_stressOpt(false)
    , m_stressDeopt(false)
    , m_dumpWhenFinished(true)
{
    WebRuntimeFeatures::enableDataTransferItems(true);
    WebRuntimeFeatures::enableGeolocation(true);
    WebRuntimeFeatures::enableIndexedDatabase(true);
    WebRuntimeFeatures::enableFileSystem(true);
    WebRuntimeFeatures::enableJavaScriptI18NAPI(true);
    WebRuntimeFeatures::enableMediaStream(true);
    WebRuntimeFeatures::enableWebAudio(true); 

    m_webPermissions = adoptPtr(new WebPermissions());
    m_accessibilityController = adoptPtr(new AccessibilityController(this));
    m_layoutTestController = adoptPtr(new LayoutTestController(this));
    m_eventSender = adoptPtr(new EventSender(this));
    m_plainTextController = adoptPtr(new PlainTextController());
    m_textInputController = adoptPtr(new TextInputController(this));
    m_notificationPresenter = adoptPtr(new NotificationPresenter(this));
    m_printer = m_testShellMode ? TestEventPrinter::createTestShellPrinter() : TestEventPrinter::createDRTPrinter();

    // 30 second is the same as the value in Mac DRT.
    // If we use a value smaller than the timeout value of
    // (new-)run-webkit-tests, (new-)run-webkit-tests misunderstands that a
    // timed-out DRT process was crashed.
    m_timeout = 30 * 1000;

#if ENABLE(INDEXED_DATABASE)
    m_tempIndexedDBDirectory = adoptPtr(webkit_support::CreateScopedTempDirectory());
    m_tempIndexedDBDirectory->CreateUniqueTempDir();
    WebIDBFactory::setTemporaryDatabaseFolder(WebString::fromUTF8(m_tempIndexedDBDirectory->path().c_str()));
#endif

    createMainWindow();
}

void TestShell::createMainWindow()
{
    m_drtDevToolsAgent = adoptPtr(new DRTDevToolsAgent);
    m_webViewHost = adoptPtr(createNewWindow(WebURL(), m_drtDevToolsAgent.get()));
    m_webView = m_webViewHost->webView();
    m_drtDevToolsAgent->setWebView(m_webView);
}

TestShell::~TestShell()
{
    // Note: DevTools are closed together with all the other windows in the
    // windows list.

    // Destroy the WebView before its WebViewHost.
    m_drtDevToolsAgent->setWebView(0);
}

void TestShell::createDRTDevToolsClient(DRTDevToolsAgent* agent)
{
    m_drtDevToolsClient = adoptPtr(new DRTDevToolsClient(agent, m_devTools->webView()));
}

void TestShell::showDevTools()
{
    if (!m_devTools) {
        WebURL url = webkit_support::GetDevToolsPathAsURL();
        if (!url.isValid()) {
            ASSERT(false);
            return;
        }
        m_devTools = createNewWindow(url);
        ASSERT(m_devTools);
        createDRTDevToolsClient(m_drtDevToolsAgent.get());
    }
    m_devTools->show(WebKit::WebNavigationPolicyNewWindow);
}

void TestShell::closeDevTools()
{
    if (m_devTools) {
        m_drtDevToolsAgent->reset();
        m_drtDevToolsClient.clear();
        closeWindow(m_devTools);
        m_devTools = 0;
    }
}

void TestShell::resetWebSettings(WebView& webView)
{
    m_prefs.reset();
    m_prefs.acceleratedCompositingEnabled = m_acceleratedCompositingEnabled;
    m_prefs.compositeToTexture = m_compositeToTexture;
    m_prefs.forceCompositingMode = m_forceCompositingMode;
    m_prefs.accelerated2dCanvasEnabled = m_accelerated2dCanvasEnabled;
    m_prefs.legacyAccelerated2dCanvasEnabled = m_legacyAccelerated2dCanvasEnabled;
    m_prefs.acceleratedDrawingEnabled = m_acceleratedDrawingEnabled;
    m_prefs.applyTo(&webView);
}

void TestShell::runFileTest(const TestParams& params)
{
    ASSERT(params.testUrl.isValid());
    m_testIsPreparing = true;
    m_params = params;
    string testUrl = m_params.testUrl.spec();

    if (testUrl.find("loading/") != string::npos
        || testUrl.find("loading\\") != string::npos)
        m_layoutTestController->setShouldDumpFrameLoadCallbacks(true);

    if (testUrl.find("/dumpAsText/") != string::npos
        || testUrl.find("\\dumpAsText\\") != string::npos) {
        m_layoutTestController->setShouldDumpAsText(true);
        m_layoutTestController->setShouldGeneratePixelResults(false);
    }

    if (testUrl.find("/inspector/") != string::npos
        || testUrl.find("\\inspector\\") != string::npos)
        showDevTools();

    if (m_params.debugLayerTree)
        m_layoutTestController->setShowDebugLayerTree(true);

    if (m_dumpWhenFinished)
        m_printer->handleTestHeader(testUrl.c_str());
    loadURL(m_params.testUrl);

    m_testIsPreparing = false;
    waitTestFinished();
}

static inline bool isSVGTestURL(const WebURL& url)
{
    return url.isValid() && string(url.spec()).find("W3C-SVG-1.1") != string::npos;
}

void TestShell::resizeWindowForTest(WebViewHost* window, const WebURL& url)
{
    int width, height;
    if (isSVGTestURL(url)) {
        width = SVGTestWindowWidth;
        height = SVGTestWindowHeight;
    } else {
        width = testWindowWidth;
        height = testWindowHeight;
    }
    window->setWindowRect(WebRect(0, 0, width + virtualWindowBorder * 2, height + virtualWindowBorder * 2));
}

void TestShell::resetTestController()
{
    resetWebSettings(*webView());
    m_webPermissions->reset();
    m_accessibilityController->reset();
    m_layoutTestController->reset();
    m_eventSender->reset();
    m_webViewHost->reset();
    m_notificationPresenter->reset();
    m_drtDevToolsAgent->reset();
    if (m_drtDevToolsClient)
        m_drtDevToolsClient->reset();
    webView()->scalePage(1, WebPoint(0, 0));
    webView()->mainFrame()->clearOpener();
}

void TestShell::loadURL(const WebURL& url)
{
    m_webViewHost->loadURLForFrame(url, WebString());
}

void TestShell::reload()
{
    m_webViewHost->navigationController()->reload();
}

void TestShell::goToOffset(int offset)
{
     m_webViewHost->navigationController()->goToOffset(offset);
}

int TestShell::navigationEntryCount() const
{
    return m_webViewHost->navigationController()->entryCount();
}

void TestShell::callJSGC()
{
    m_webView->mainFrame()->collectGarbage();
}

void TestShell::setFocus(WebWidget* widget, bool enable)
{
    // Simulate the effects of InteractiveSetFocus(), which includes calling
    // both setFocus() and setIsActive().
    if (enable) {
        if (m_focusedWidget != widget) {
            if (m_focusedWidget)
                m_focusedWidget->setFocus(false);
            webView()->setIsActive(enable);
            widget->setFocus(enable);
            m_focusedWidget = widget;
        }
    } else {
        if (m_focusedWidget == widget) {
            widget->setFocus(enable);
            webView()->setIsActive(enable);
            m_focusedWidget = 0;
        }
    }
}

void TestShell::testFinished()
{
    if (!m_testIsPending)
        return;
    m_testIsPending = false;
    if (m_dumpWhenFinished)
        dump();
    webkit_support::QuitMessageLoop();
}

void TestShell::testTimedOut()
{
    m_printer->handleTimedOut();
    testFinished();
}

static string dumpDocumentText(WebFrame* frame)
{
    // We use the document element's text instead of the body text here because
    // not all documents have a body, such as XML documents.
    WebElement documentElement = frame->document().documentElement();
    if (documentElement.isNull())
        return string();
    return documentElement.innerText().utf8();
}

static string dumpFramesAsText(WebFrame* frame, bool recursive)
{
    string result;

    // Add header for all but the main frame. Skip empty frames.
    if (frame->parent() && !frame->document().documentElement().isNull()) {
        result.append("\n--------\nFrame: '");
        result.append(frame->name().utf8().data());
        result.append("'\n--------\n");
    }

    result.append(dumpDocumentText(frame));
    result.append("\n");

    if (recursive) {
        for (WebFrame* child = frame->firstChild(); child; child = child->nextSibling())
            result.append(dumpFramesAsText(child, recursive));
    }

    return result;
}

static void dumpFrameScrollPosition(WebFrame* frame, bool recursive)
{
    WebSize offset = frame->scrollOffset();
    if (offset.width > 0 || offset.height > 0) {
        if (frame->parent())
            printf("frame '%s' ", frame->name().utf8().data());
        printf("scrolled to %d,%d\n", offset.width, offset.height);
    }

    if (!recursive)
        return;
    for (WebFrame* child = frame->firstChild(); child; child = child->nextSibling())
        dumpFrameScrollPosition(child, recursive);
}

struct ToLower {
    char16 operator()(char16 c) { return tolower(c); }
};

// FIXME: Eliminate std::transform(), std::vector, and std::sort().

// Returns True if item1 < item2.
static bool HistoryItemCompareLess(const WebHistoryItem& item1, const WebHistoryItem& item2)
{
    string16 target1 = item1.target();
    string16 target2 = item2.target();
    std::transform(target1.begin(), target1.end(), target1.begin(), ToLower());
    std::transform(target2.begin(), target2.end(), target2.begin(), ToLower());
    return target1 < target2;
}

static string dumpHistoryItem(const WebHistoryItem& item, int indent, bool isCurrent)
{
    string result;

    if (isCurrent) {
        result.append("curr->");
        result.append(indent - 6, ' '); // 6 == "curr->".length()
    } else
        result.append(indent, ' ');

    string url = item.urlString().utf8();
    size_t pos;
    if (!url.find(fileUrlPattern) && ((pos = url.find(layoutTestsPattern)) != string::npos)) {
        // adjust file URLs to match upstream results.
        url.replace(0, pos + layoutTestsPatternSize, fileTestPrefix);
    } else if (!url.find(dataUrlPattern)) {
        // URL-escape data URLs to match results upstream.
        string path = webkit_support::EscapePath(url.substr(dataUrlPatternSize));
        url.replace(dataUrlPatternSize, url.length(), path);
    }

    result.append(url);
    if (!item.target().isEmpty()) {
        result.append(" (in frame \"");
        result.append(item.target().utf8());
        result.append("\")");
    }
    if (item.isTargetItem())
        result.append("  **nav target**");
    result.append("\n");

    const WebVector<WebHistoryItem>& children = item.children();
    if (!children.isEmpty()) {
        // Must sort to eliminate arbitrary result ordering which defeats
        // reproducible testing.
        // FIXME: WebVector should probably just be a std::vector!!
        std::vector<WebHistoryItem> sortedChildren;
        for (size_t i = 0; i < children.size(); ++i)
            sortedChildren.push_back(children[i]);
        std::sort(sortedChildren.begin(), sortedChildren.end(), HistoryItemCompareLess);
        for (size_t i = 0; i < sortedChildren.size(); ++i)
            result += dumpHistoryItem(sortedChildren[i], indent + 4, false);
    }

    return result;
}

static void dumpBackForwardList(const TestNavigationController& navigationController, string& result)
{
    result.append("\n============== Back Forward List ==============\n");
    for (int index = 0; index < navigationController.entryCount(); ++index) {
        int currentIndex = navigationController.lastCommittedEntryIndex();
        WebHistoryItem historyItem = navigationController.entryAtIndex(index)->contentState();
        if (historyItem.isNull()) {
            historyItem.initialize();
            historyItem.setURLString(navigationController.entryAtIndex(index)->URL().spec().utf16());
        }
        result.append(dumpHistoryItem(historyItem, 8, index == currentIndex));
    }
    result.append("===============================================\n");
}

string TestShell::dumpAllBackForwardLists()
{
    string result;
    for (unsigned i = 0; i < m_windowList.size(); ++i)
        dumpBackForwardList(*m_windowList[i]->navigationController(), result);
    return result;
}

void TestShell::dump()
{
    WebScriptController::flushConsoleMessages();

    // Dump the requested representation.
    WebFrame* frame = m_webView->mainFrame();
    if (!frame)
        return;
    bool shouldDumpAsText = m_layoutTestController->shouldDumpAsText();
    bool shouldDumpAsAudio = m_layoutTestController->shouldDumpAsAudio();
    bool shouldGeneratePixelResults = m_layoutTestController->shouldGeneratePixelResults();
    bool dumpedAnything = false;

    if (shouldDumpAsAudio) {
        m_printer->handleAudioHeader();
        
        const WebKit::WebArrayBufferView& webArrayBufferView = m_layoutTestController->audioData();
        printf("Content-Length: %d\n", webArrayBufferView.byteLength());
        
        if (fwrite(webArrayBufferView.baseAddress(), 1, webArrayBufferView.byteLength(), stdout) != webArrayBufferView.byteLength())
            FATAL("Short write to stdout, disk full?\n");
        printf("\n");
        
        m_printer->handleTestFooter(true);

        fflush(stdout);
        fflush(stderr);
        return;
    }

    if (m_params.dumpTree) {
        dumpedAnything = true;
        m_printer->handleTextHeader();
        // Text output: the test page can request different types of output
        // which we handle here.
        if (!shouldDumpAsText) {
            // Plain text pages should be dumped as text
            string mimeType = frame->dataSource()->response().mimeType().utf8();
            if (mimeType == "text/plain") {
                shouldDumpAsText = true;
                shouldGeneratePixelResults = false;
            }
        }
        if (shouldDumpAsText) {
            bool recursive = m_layoutTestController->shouldDumpChildFramesAsText();
            string dataUtf8 = dumpFramesAsText(frame, recursive);
            if (fwrite(dataUtf8.c_str(), 1, dataUtf8.size(), stdout) != dataUtf8.size())
                FATAL("Short write to stdout, disk full?\n");
        } else {
            printf("%s", frame->renderTreeAsText(m_params.debugRenderTree).utf8().data());
            bool recursive = m_layoutTestController->shouldDumpChildFrameScrollPositions();
            dumpFrameScrollPosition(frame, recursive);
        }
        if (m_layoutTestController->shouldDumpBackForwardList())
            printf("%s", dumpAllBackForwardLists().c_str());
    }
    if (dumpedAnything && m_params.printSeparators)
        m_printer->handleTextFooter();

    if (m_params.dumpPixels && shouldGeneratePixelResults) {
        // Image output: we write the image data to the file given on the
        // command line (for the dump pixels argument), and the MD5 sum to
        // stdout.
        dumpedAnything = true;
        m_webView->layout();
        if (m_layoutTestController->testRepaint()) {
            WebSize viewSize = m_webView->size();
            int width = viewSize.width;
            int height = viewSize.height;
            if (m_layoutTestController->sweepHorizontally()) {
                for (WebRect column(0, 0, 1, height); column.x < width; column.x++)
                    m_webViewHost->paintRect(column);
            } else {
                for (WebRect line(0, 0, width, 1); line.y < height; line.y++)
                    m_webViewHost->paintRect(line);
            }
        } else
            m_webViewHost->paintInvalidatedRegion();

        // See if we need to draw the selection bounds rect. Selection bounds
        // rect is the rect enclosing the (possibly transformed) selection.
        // The rect should be drawn after everything is laid out and painted.
        if (m_layoutTestController->shouldDumpSelectionRect()) {
            // If there is a selection rect - draw a red 1px border enclosing rect
            WebRect wr = frame->selectionBoundsRect();
            if (!wr.isEmpty()) {
                // Render a red rectangle bounding selection rect
                SkPaint paint;
                paint.setColor(0xFFFF0000); // Fully opaque red
                paint.setStyle(SkPaint::kStroke_Style);
                paint.setFlags(SkPaint::kAntiAlias_Flag);
                paint.setStrokeWidth(1.0f);
                SkIRect rect; // Bounding rect
                rect.set(wr.x, wr.y, wr.x + wr.width, wr.y + wr.height);
                m_webViewHost->canvas()->drawIRect(rect, paint);
            }
        }

        dumpImage(m_webViewHost->canvas());
    }
    m_printer->handleImageFooter();
    m_printer->handleTestFooter(dumpedAnything);
    fflush(stdout);
    fflush(stderr);
}

void TestShell::dumpImage(SkCanvas* canvas) const
{
    // Fix the alpha. The expected PNGs on Mac have an alpha channel, so we want
    // to keep it. On Windows, the alpha channel is wrong since text/form control
    // drawing may have erased it in a few places. So on Windows we force it to
    // opaque and also don't write the alpha channel for the reference. Linux
    // doesn't have the wrong alpha like Windows, but we match Windows.
#if OS(MAC_OS_X)
    bool discardTransparency = false;
#else
    bool discardTransparency = true;
    makeCanvasOpaque(canvas);
#endif

    const SkBitmap& sourceBitmap = canvas->getTopDevice()->accessBitmap(false);
    SkAutoLockPixels sourceBitmapLock(sourceBitmap);

    // Compute MD5 sum.
    MD5 digester;
    Vector<uint8_t, 16> digestValue;
    digester.addBytes(reinterpret_cast<const uint8_t*>(sourceBitmap.getPixels()), sourceBitmap.getSize());
    digester.checksum(digestValue);
    string md5hash;
    md5hash.reserve(16 * 2);
    for (unsigned i = 0; i < 16; ++i) {
        char hex[3];
        // Use "x", not "X". The string must be lowercased.
        sprintf(hex, "%02x", digestValue[i]);
        md5hash.append(hex);
    }

    // Only encode and dump the png if the hashes don't match. Encoding the
    // image is really expensive.
    if (md5hash.compare(m_params.pixelHash)) {
        std::vector<unsigned char> png;
        webkit_support::EncodeBGRAPNGWithChecksum(reinterpret_cast<const unsigned char*>(sourceBitmap.getPixels()), sourceBitmap.width(),
            sourceBitmap.height(), static_cast<int>(sourceBitmap.rowBytes()), discardTransparency, md5hash, &png);

        m_printer->handleImage(md5hash.c_str(), m_params.pixelHash.c_str(), &png[0], png.size(), m_params.pixelFileName.c_str());
    } else
        m_printer->handleImage(md5hash.c_str(), m_params.pixelHash.c_str(), 0, 0, m_params.pixelFileName.c_str());
}

void TestShell::bindJSObjectsToWindow(WebFrame* frame)
{
    WebTestingSupport::injectInternalsObject(frame);
    m_accessibilityController->bindToJavascript(frame, WebString::fromUTF8("accessibilityController"));
    m_layoutTestController->bindToJavascript(frame, WebString::fromUTF8("layoutTestController"));
    m_eventSender->bindToJavascript(frame, WebString::fromUTF8("eventSender"));
    m_plainTextController->bindToJavascript(frame, WebString::fromUTF8("plainText"));
    m_textInputController->bindToJavascript(frame, WebString::fromUTF8("textInputController"));
}

WebViewHost* TestShell::createNewWindow(const WebKit::WebURL& url)
{
    return createNewWindow(url, 0);
}

WebViewHost* TestShell::createNewWindow(const WebKit::WebURL& url, DRTDevToolsAgent* devToolsAgent)
{
    WebViewHost* host = new WebViewHost(this);
    WebView* view = WebView::create(host);
    view->setPermissionClient(webPermissions());
    view->setDevToolsAgentClient(devToolsAgent);
    host->setWebWidget(view);
    m_prefs.applyTo(view);
    view->initializeMainFrame(host);
    m_windowList.append(host);
    host->loadURLForFrame(url, WebString());
    return host;
}

void TestShell::closeWindow(WebViewHost* window)
{
    size_t i = m_windowList.find(window);
    if (i == notFound) {
        ASSERT_NOT_REACHED();
        return;
    }
    m_windowList.remove(i);
    WebWidget* focusedWidget = m_focusedWidget;
    if (window->webWidget() == m_focusedWidget)
        focusedWidget = 0;

    delete window;
    // We set the focused widget after deleting the web view host because it
    // can change the focus.
    m_focusedWidget = focusedWidget;
    if (m_focusedWidget) {
        webView()->setIsActive(true);
        m_focusedWidget->setFocus(true);
    }
}

void TestShell::closeRemainingWindows()
{
    // Just close devTools window manually because we have custom deinitialization code for it.
    closeDevTools();

    // Iterate through the window list and close everything except the main
    // window. We don't want to delete elements as we're iterating, so we copy
    // to a temp vector first.
    Vector<WebViewHost*> windowsToDelete;
    for (unsigned i = 0; i < m_windowList.size(); ++i) {
        if (m_windowList[i] != webViewHost())
            windowsToDelete.append(m_windowList[i]);
    }
    ASSERT(windowsToDelete.size() + 1 == m_windowList.size());
    for (unsigned i = 0; i < windowsToDelete.size(); ++i)
        closeWindow(windowsToDelete[i]);
    ASSERT(m_windowList.size() == 1);
}

int TestShell::windowCount()
{
    return m_windowList.size();
}
