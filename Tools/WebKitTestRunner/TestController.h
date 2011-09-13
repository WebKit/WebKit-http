/*
 * Copyright (C) 2010 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TestController_h
#define TestController_h

#include <WebKit2/WKRetainPtr.h>
#include <string>
#include <vector>
#include <wtf/OwnPtr.h>

namespace WTR {

class TestInvocation;
class PlatformWebView;
class EventSenderProxy;

// FIXME: Rename this TestRunner?
class TestController {
public:
    static TestController& shared();

    TestController(int argc, const char* argv[]);
    ~TestController();

    bool verbose() const { return m_verbose; }

    WKStringRef injectedBundlePath() { return m_injectedBundlePath.get(); }
    WKStringRef testPluginDirectory() { return m_testPluginDirectory.get(); }

    PlatformWebView* mainWebView() { return m_mainWebView.get(); }
    WKContextRef context() { return m_context.get(); }

    // Runs the run loop until `done` is true or the timeout elapses.
    enum TimeoutDuration { ShortTimeout, LongTimeout };
    void runUntil(bool& done, TimeoutDuration);
    void notifyDone();
    
    bool beforeUnloadReturnValue() const { return m_beforeUnloadReturnValue; }
    void setBeforeUnloadReturnValue(bool value) { m_beforeUnloadReturnValue = value; }

private:
    void initialize(int argc, const char* argv[]);
    void run();

    void runTestingServerLoop();
    bool runTest(const char* pathOrURL);

    void platformInitialize();
    void platformInitializeContext();
    void platformRunUntil(bool& done, double timeout);
    void initializeInjectedBundlePath();
    void initializeTestPluginDirectory();

    bool resetStateToConsistentValues();

    // WKContextInjectedBundleClient
    static void didReceiveMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, const void*);
    static void didReceiveSynchronousMessageFromInjectedBundle(WKContextRef, WKStringRef messageName, WKTypeRef messageBody, WKTypeRef* returnData, const void*);
    void didReceiveMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody);
    WKRetainPtr<WKTypeRef> didReceiveSynchronousMessageFromInjectedBundle(WKStringRef messageName, WKTypeRef messageBody);

    // WKPageLoaderClient
    static void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame, WKTypeRef userData, const void*);
    void didFinishLoadForFrame(WKPageRef page, WKFrameRef frame);

    static void processDidCrash(WKPageRef, const void* clientInfo);
    void processDidCrash();

    static WKPageRef createOtherPage(WKPageRef oldPage, WKURLRequestRef, WKDictionaryRef, WKEventModifiers, WKEventMouseButton, const void*);

    static void runModal(WKPageRef, const void* clientInfo);
    static void runModal(PlatformWebView*);

    static const char* libraryPathForTesting();
    static const char* platformLibraryPathForTesting();

    OwnPtr<TestInvocation> m_currentInvocation;

    bool m_dumpPixels;
    bool m_verbose;
    bool m_printSeparators;
    bool m_usingServerMode;
    bool m_gcBetweenTests;
    std::vector<std::string> m_paths;
    WKRetainPtr<WKStringRef> m_injectedBundlePath;
    WKRetainPtr<WKStringRef> m_testPluginDirectory;

    OwnPtr<PlatformWebView> m_mainWebView;
    WKRetainPtr<WKContextRef> m_context;
    WKRetainPtr<WKPageGroupRef> m_pageGroup;

    enum State {
        Initial,
        Resetting,
        RunningTest
    };
    State m_state;
    bool m_doneResetting;

    double m_longTimeout;
    double m_shortTimeout;

    bool m_didPrintWebProcessCrashedMessage;
    bool m_shouldExitWhenWebProcessCrashes;
    
    bool m_beforeUnloadReturnValue;

    EventSenderProxy* m_eventSenderProxy;
};

} // namespace WTR

#endif // TestController_h
