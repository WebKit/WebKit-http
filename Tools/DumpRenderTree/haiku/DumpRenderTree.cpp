/*
 * Copyright (C) 2009 Maxime Simon <simon.maxime@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1.  Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 * 2.  Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 * 3.  Neither the name of Apple Computer, Inc. ("Apple") nor the names of
 *     its contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE AND ITS CONTRIBUTORS "AS IS" AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL APPLE OR ITS CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "DumpRenderTree.h"

//#include "DumpHistoryItem.h"
//#include "DumpRenderTreeChrome.h"
//#include "DumpRenderTreeView.h"

#include "AccessibilityController.h"
#include <wtf/text/AtomicString.h> // FIXME EventSender not standalone ?
#include "DumpRenderTreeClient.h"
#include "EventSender.h"
//#include "FontManagement.h"
#include "Frame.h"
#include "GCController.h"
#include "NotImplemented.h"
#include "PixelDumpSupport.h"
#include "ScriptController.h"
#include "TestRunner.h"
#include "WebCoreTestSupport.h"
#include "WebFrame.h"
#include "WebPage.h"
#include "WebView.h"
#include "WebViewConstants.h"
#include "WebWindow.h"
#include "WorkQueue.h"

#include <wtf/Assertions.h>
#include <wtf/OwnPtr.h>
#include <wtf/text/CString.h>
#include <wtf/text/StringBuilder.h>

#include <Application.h>
#include <JavaScriptCore/JavaScript.h>
#include <OS.h>
#include <stdlib.h>
#include <string.h>
#include <vector>

// From the top-level DumpRenderTree.h
RefPtr<TestRunner> gTestRunner;
volatile bool done = true;

static bool dumpPixelsForCurrentTest;
static int dumpPixelsForAllTests = false;
static bool dumpTree = true;
static bool printSeparators = true;

using namespace std;

BWebView* webView;

const unsigned maxViewHeight = 600;
const unsigned maxViewWidth = 800;

static String dumpFramesAsText(BWebFrame* frame)
{
    if (!frame)
        return String();

    String result = frame->InnerText();
    result.append("\n");

    if (gTestRunner->dumpChildFramesAsText()) {
        // FIXME:
        // not implemented.
    }

    return result;
}

static void adjustOutputTypeByMimeType(BWebFrame* frame)
{
    const String responseMimeType(WebCore::DumpRenderTreeClient::responseMimeType(frame));
    if (responseMimeType == "text/plain") {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }
}

static void dumpFrameContentsAsText(BWebFrame* frame)
{
    String result;
    if (gTestRunner->dumpAsText())
        result = dumpFramesAsText(frame);
    else
        result = frame->ExternalRepresentation();
//        result = DumpRenderTreeSupportEfl::renderTreeDump(frame);

    printf("%s", result.utf8().data());
}

void dump()
{
    BWebFrame* frame = webView->WebPage()->MainFrame();

    if (dumpTree) {
        adjustOutputTypeByMimeType(frame);
        dumpFrameContentsAsText(frame);

        if (gTestRunner->dumpBackForwardList()) {
            // FIXME:
            // not implemented.
        }

        if (printSeparators) {
            puts("#EOF");
            fputs("#EOF\n", stderr);
            fflush(stdout);
            fflush(stderr);
        }
    }

    if (dumpPixelsForCurrentTest
        && !gTestRunner->dumpAsText()
        && !gTestRunner->dumpDOMAsWebArchive()
        && !gTestRunner->dumpSourceAsWebArchive()) {
        // FIXME: Add support for dumping pixels
    }
}

static bool shouldLogFrameLoadDelegates(const String& pathOrURL)
{
    return pathOrURL.contains("loading/");
}

static bool shouldDumpAsText(const String& pathOrURL)
{
    return pathOrURL.contains("dumpAsText/");
}

static bool shouldOpenWebInspector(const String& pathOrURL)
{
    return pathOrURL.contains("inspector/");
}

static String getFinalTestURL(const String& testURL)
{
    if (!testURL.startsWith("http://") && !testURL.startsWith("https://")) {
        char* cFilePath = realpath(testURL.utf8().data(), NULL);
        const String filePath = String::fromUTF8(cFilePath);
        free(cFilePath);

        //if (ecore_file_exists(filePath.utf8().data()))
            return String("file://") + filePath;
    }

    return testURL;
}

static inline bool isGlobalHistoryTest(const String& cTestPathOrURL)
{
    return cTestPathOrURL.contains("/globalhistory/");
}

static void createTestRunner(const String& testURL, const String& expectedPixelHash)
{
    gTestRunner =
        TestRunner::create(std::string(testURL.utf8().data()),
                                     std::string(expectedPixelHash.utf8().data()));

    //topLoadingFrame = 0;
    done = false;

    gTestRunner->setIconDatabaseEnabled(false);

    if (shouldLogFrameLoadDelegates(testURL))
        gTestRunner->setDumpFrameLoadCallbacks(true);

    gTestRunner->setDeveloperExtrasEnabled(true);
    if (shouldOpenWebInspector(testURL))
        gTestRunner->showWebInspector();

    gTestRunner->setDumpHistoryDelegateCallbacks(isGlobalHistoryTest(testURL));

    if (shouldDumpAsText(testURL)) {
        gTestRunner->setDumpAsText(true);
        gTestRunner->setGeneratePixelResults(false);
    }
}

static void runTest(const string& inputLine)
{
    TestCommand command = parseInputLine(inputLine);
    const String testPathOrURL(command.pathOrURL.c_str());
    ASSERT(!testPathOrURL.isEmpty());
    dumpPixelsForCurrentTest = command.shouldDumpPixels || dumpPixelsForAllTests;
    const String expectedPixelHash(command.expectedPixelHash.c_str());

    // Convert the path into a full file URL if it does not look
    // like an HTTP/S URL (doesn't start with http:// or https://).
    const String testURL = getFinalTestURL(testPathOrURL);

    //browser->resetDefaultsToConsistentValues();
    createTestRunner(testURL, expectedPixelHash);
    if (!gTestRunner) {
        be_app->PostMessage(B_QUIT_REQUESTED);
    }

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    const bool isSVGW3CTest = testURL.contains("svg/W3C-SVG-1.1");
    const int width = isSVGW3CTest ? TestRunner::w3cSVGViewWidth : TestRunner::viewWidth;
    const int height = isSVGW3CTest ? TestRunner::w3cSVGViewHeight : TestRunner::viewHeight;
    webView->ResizeTo(width, height); 

    // TODO efl does some history cleanup here
    
    webView->WebPage()->MainFrame()->LoadURL(BString(testURL));
}

#pragma mark -

class DumpRenderTreeApp : public BApplication, WebCore::DumpRenderTreeClient {
public:
    DumpRenderTreeApp();
    ~DumpRenderTreeApp() { }

    // BApplication
    void ArgvReceived(int32 argc, char** argv);
    void ReadyToRun();
    void MessageReceived(BMessage*);
    bool QuitRequested() { return true; }

    // DumpRenderTreeClient
    void didClearWindowObjectInWorld(WebCore::DOMWrapperWorld&,
        JSGlobalContextRef context, JSObjectRef windowObject);

    static status_t runTestFromStdin();

private:
    GCController* m_gcController;
    AccessibilityController* m_accessibilityController;
    BWebWindow* m_webWindow;

    int m_currentTest;
    vector<const char*> m_tests;
    bool m_fromStdin;
};


DumpRenderTreeApp::DumpRenderTreeApp()
    : BApplication("application/x-vnd.DumpRenderTree")
{
}

void DumpRenderTreeApp::ArgvReceived(int32 argc, char** argv)
{
    for (int i = 1; i < argc; ++i) {
        if (!strcmp(argv[i], "--notree")) {
            dumpTree = false;
            continue;
        }

        if (!strcmp(argv[i], "--pixel-tests")) {
            dumpPixelsForAllTests = true;
            continue;
        }

        if (!strcmp(argv[i], "--tree")) {
            dumpTree = true;
            continue;
        }

        char* str = new char[strlen(argv[i]) + 1];
        strcpy(str, argv[i]);
        m_tests.push_back(str);
    }
}

class DumpRenderTreeChrome: public BWebWindow
{
    public:
    DumpRenderTreeChrome()
        : BWebWindow(BRect(0, 0, maxViewWidth, maxViewHeight), "DumpRenderTree",
            B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0)
    {
    }

    void MessageReceived(BMessage* message)
    {
        switch(message->what)
        {
            case ADD_CONSOLE_MESSAGE:
            {
                // Follow the format used by other DRTs here. Note this doesn't
                // include the fille URL, making it possible to have consistent
                // results even if the tests are moved around.
                int32 lineNumber = message->FindInt32("line");
                BString text = message->FindString("string");
                printf("CONSOLE MESSAGE: line %i: %s\n", lineNumber,
                    text.String());
                return;
            }
        }

        BWebWindow::MessageReceived(message);
    }
};

void DumpRenderTreeApp::ReadyToRun()
{
    BWebPage::InitializeOnce();
    BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

    // Create the main application window.
    m_webWindow = new DumpRenderTreeChrome();
    m_webWindow->SetSizeLimits(0, maxViewWidth, 0, maxViewHeight);
    m_webWindow->ResizeTo(maxViewWidth, maxViewHeight);


    webView = new BWebView("DumpRenderTree");
    webView->SetExplicitSize(BSize(maxViewWidth, maxViewHeight));
    m_webWindow->SetCurrentWebView(webView);

    webView->WebPage()->MainFrame()->SetListener(this);
    Register(webView->WebPage());

    // Start the looper, but keep the window hidden
    m_webWindow->Hide();
    m_webWindow->Show();

    if (m_tests.size() == 1 && !strcmp(m_tests[0], "-")) {
        printSeparators = true;
        m_fromStdin = true;
        runTestFromStdin();
    } else if(m_tests.size() != 0) {
        printSeparators = (m_tests.size() > 1 || (dumpPixelsForAllTests && dumpTree));
        m_fromStdin = false;
        runTest(m_tests[0]);
        m_currentTest = 1;
    }
}

static void sendPixelResultsEOF()
{
    puts("#EOF");
    fflush(stdout);
    fflush(stderr);
}

void DumpRenderTreeApp::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case LOAD_FINISHED: {
        dump();

        gTestRunner->closeWebInspector();
        gTestRunner->setDeveloperExtrasEnabled(false);

        //browser->clearExtraViews();

        // FIXME: Move to DRTChrome::resetDefaultsToConsistentValues() after bug 85209 lands.
        //WebCoreTestSupport::resetInternalsObject(DumpRenderTreeSupportEfl::globalContextRefForFrame(browser->mainFrame()));

        //ewk_view_uri_set(browser->mainView(), "about:blank");

        //gTestRunner->clear();
        sendPixelResultsEOF();
        
        gTestRunner->notifyDone();

        if (m_fromStdin) {
            // run the next test.
            if(runTestFromStdin() != B_OK) {
                be_app->PostMessage(B_QUIT_REQUESTED);
            }
            break;
        } else {
            if (m_currentTest < m_tests.size()) {
                runTest(m_tests[m_currentTest]);
                m_currentTest++;
            } else
                be_app->PostMessage(B_QUIT_REQUESTED);
        }
        break;
    }

    default:
        BApplication::MessageReceived(message);
        break;
    }
}

void DumpRenderTreeApp::didClearWindowObjectInWorld(WebCore::DOMWrapperWorld&, JSGlobalContextRef context, JSObjectRef windowObject)
{
    JSValueRef exception = 0;

    gTestRunner->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    m_gcController->makeWindowObject(context, windowObject, &exception);
    ASSERT(!exception);

    //m_accessibilityController->makeWindowObject(context, windowObject, &exception);
    //ASSERT(!exception);

    JSStringRef eventSenderStr = JSStringCreateWithUTF8CString("eventSender");
    JSValueRef eventSender = makeEventSender(context);
    JSObjectSetProperty(context, windowObject, eventSenderStr, eventSender, kJSPropertyAttributeReadOnly | kJSPropertyAttributeDontDelete, 0);
    JSStringRelease(eventSenderStr);

    WebCoreTestSupport::injectInternalsObject(context);
}

status_t DumpRenderTreeApp::runTestFromStdin()
{
    char filenameBuffer[2048];

    if(fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {

        char* newLineCharacter = strchr(filenameBuffer, '\n');

        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strlen(filenameBuffer))
            return B_ERROR;

        runTest(filenameBuffer);
        return B_OK;
    }

    return B_ERROR;

}

#pragma mark -

int main()
{
    DumpRenderTreeApp app;
    app.Run();

    gTestRunner.release();
}
