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

#include <wtf/text/AtomicString.h> // FIXME EventSender not standalone ?
#include "EventSender.h"
//#include "FontManagement.h"
#include "NotImplemented.h"
#include "PixelDumpSupport.h"
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

volatile bool notified = false;
static bool printSeparators = true;
static bool dumpPixels = false;
static bool dumpTree = true;
static thread_id stdinThread;

using namespace std;

static BWebView* webView;

const unsigned maxViewHeight = 600;
const unsigned maxViewWidth = 800;

void notifyDoneFired()
{
    notified = true;
    if (done)
        dump();
}

static String dumpFramesAsText(BWebFrame* frame)
{
    if (!frame)
        return String();
    
    if (gTestRunner->dumpChildFramesAsText()) {
        // FIXME:
        // not implemented.
    }

    return frame->InnerText();
}

void dump()
{
    if (gTestRunner->waitToDump() && !notified)
        return;

    if (dumpTree) {
        const char* result = 0;

        BString str;
        if (gTestRunner->dumpAsText())
            str = dumpFramesAsText(webView->WebPage()->MainFrame());
        else
            str = webView->WebPage()->MainFrame()->ExternalRepresentation();

        result = str.String();
        if (!result) {
            const char* errorMessage;
            if (gTestRunner->dumpAsText())
                errorMessage = "WebFrame::GetInnerText";
            else
                errorMessage = "WebFrame::GetExternalRepresentation";
            printf("ERROR: NULL result from %s\n", errorMessage);
        } else
            printf("%s\n", result);

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

    if (dumpPixels
        && !gTestRunner->dumpAsText()
        && !gTestRunner->dumpDOMAsWebArchive()
        && !gTestRunner->dumpSourceAsWebArchive()) {
        // FIXME: Add support for dumping pixels
    }

    puts("#EOF");
    fflush(stdout);
    fflush(stderr);
}

static void runTest(const string& testPathOrURL)
{
    ASSERT(!testPathOrURL.empty());

    done = false;
    string pathOrURL(testPathOrURL);
    string expectedPixelHash;

    size_t separatorPos = pathOrURL.find("'");
    if (separatorPos != string::npos) {
        pathOrURL = string(testPathOrURL, 0, separatorPos);
        expectedPixelHash = string(testPathOrURL, separatorPos + 1);
    }

    // CURL isn't happy if we don't have a protocol.
    size_t http = pathOrURL.find("http://");
    if (http == string::npos)
        pathOrURL.insert(0, "file://");

    gTestRunner = TestRunner::create(pathOrURL, expectedPixelHash);
    if (!gTestRunner)
        be_app->PostMessage(B_QUIT_REQUESTED);

    WorkQueue::shared()->clear();
    WorkQueue::shared()->setFrozen(false);

    webView->WebPage()->MainFrame()->LoadURL(BString(pathOrURL.c_str()));
}

#pragma mark -

class DumpRenderTreeApp : public BApplication {
public:
    DumpRenderTreeApp();
    ~DumpRenderTreeApp() { }

    virtual void ArgvReceived(int32 argc, char** argv);
    virtual void ReadyToRun();
    virtual void MessageReceived(BMessage*);
    virtual bool QuitRequested() { return true; }

    static status_t runTestFromStdin(void*);

private:
    BWebWindow* m_webFrame;
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
            dumpPixels = true;
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

void DumpRenderTreeApp::ReadyToRun()
{
    BWebPage::InitializeOnce();
    BWebPage::SetCacheModel(B_WEBKIT_CACHE_MODEL_WEB_BROWSER);

    // Create the main application window.
    m_webFrame = new BWebWindow(BRect(10, 10, 640, 480), "DumpRenderTree",
        B_NO_BORDER_WINDOW_LOOK, B_NORMAL_WINDOW_FEEL, 0);
    m_webFrame->SetSizeLimits(0, maxViewWidth, 0, maxViewHeight);
    m_webFrame->ResizeTo(maxViewWidth, maxViewHeight);


    webView = new BWebView("DumpRenderTree");
    webView->SetExplicitMaxSize(BSize(maxViewWidth, maxViewHeight));
    m_webFrame->SetCurrentWebView(webView);

    webView->WebPage()->MainFrame()->SetListener(this);

    // Start the looper, but keep the window hidden
    m_webFrame->Hide();
    m_webFrame->Show();

    if (m_tests.size() == 1 && !strcmp(m_tests[0], "-")) {
        printSeparators = true;
        m_fromStdin = true;
        stdinThread = spawn_thread(runTestFromStdin, 0, B_NORMAL_PRIORITY, 0);
        resume_thread(stdinThread);
    } else if(m_tests.size() != 0) {
        printSeparators = (m_tests.size() > 1 || (dumpPixels && dumpTree));
        m_fromStdin = false;
        runTest(m_tests[0]);
        m_currentTest = 1;
    }
}

void DumpRenderTreeApp::MessageReceived(BMessage* message)
{
    switch (message->what) {
    case LOAD_FINISHED: {
        if (!gTestRunner->waitToDump() || notified) {
            dump();
            done = true;
        }

        if (m_fromStdin) {
            resume_thread(stdinThread);
            break;
        }

        if (m_currentTest < m_tests.size()) {
            runTest(m_tests[m_currentTest]);
            m_currentTest++;
        } else {
            if (!m_fromStdin)
                be_app->PostMessage(B_QUIT_REQUESTED);
        }
        break;
    }
    default:
        BApplication::MessageReceived(message);
        break;
    }
}

status_t DumpRenderTreeApp::runTestFromStdin(void*)
{
    char filenameBuffer[2048];

    while (fgets(filenameBuffer, sizeof(filenameBuffer), stdin)) {

        char* newLineCharacter = strchr(filenameBuffer, '\n');

        if (newLineCharacter)
            *newLineCharacter = '\0';

        if (!strlen(filenameBuffer))
            continue;

        runTest(filenameBuffer);
        suspend_thread(stdinThread);
    }

    be_app->PostMessage(B_QUIT_REQUESTED);
    return B_OK;
}

#pragma mark -

int main()
{
    DumpRenderTreeApp app;
    app.Run();

    gTestRunner.release();
}
