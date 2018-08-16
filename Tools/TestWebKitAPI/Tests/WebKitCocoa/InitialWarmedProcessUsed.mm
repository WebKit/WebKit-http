/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
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

#import "config.h"
#import "PlatformUtilities.h"
#import "Test.h"
#import "TestNavigationDelegate.h"

#import <WebKit/WKProcessPoolPrivate.h>
#import <wtf/RetainPtr.h>

#if WK_API_ENABLED

static NSString *loadableURL = @"data:text/html,no%20error%20A";

TEST(WKProcessPool, InitialWarmedProcessUsed)
{
    auto pool = adoptNS([[WKProcessPool alloc] init]);
    [pool _setMaximumNumberOfPrewarmedProcesses:1];
    [pool _warmInitialProcess];

    EXPECT_EQ(static_cast<size_t>(1), [pool _prewarmedWebProcessCount]);
    EXPECT_EQ(static_cast<size_t>(1), [pool _webPageContentProcessCount]);

    auto configuration = adoptNS([[WKWebViewConfiguration alloc] init]);
    configuration.get().processPool = pool.get();
    configuration.get().websiteDataStore = [WKWebsiteDataStore nonPersistentDataStore];

    auto webView = adoptNS([[WKWebView alloc] initWithFrame:CGRectMake(0, 0, 800, 600) configuration:configuration.get()]);

    EXPECT_EQ(static_cast<size_t>(0), [pool _prewarmedWebProcessCount]);
    EXPECT_EQ(static_cast<size_t>(1), [pool _webPageContentProcessCount]);

    [webView loadRequest:[NSURLRequest requestWithURL:[NSURL URLWithString:loadableURL]]];
    [webView _test_waitForDidFinishNavigation];

    EXPECT_EQ(static_cast<size_t>(1), [pool _prewarmedWebProcessCount]);
    EXPECT_EQ(static_cast<size_t>(2), [pool _webPageContentProcessCount]);
}

#endif
