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

#pragma once

#include "WKSharedAPICast.h"
#include "WKBundlePage.h"
#include "WKBundlePagePrivate.h"
#include "WKBundlePrivate.h"
#include <JavaScriptCore/ConsoleTypes.h>
#include <WebCore/EditorInsertAction.h>
#include <WebCore/TextAffinity.h>

namespace WebKit {

class InjectedBundle;
class InjectedBundleBackForwardList;
class InjectedBundleBackForwardListItem;
class InjectedBundleCSSStyleDeclarationHandle;
class InjectedBundleDOMWindowExtension;
class InjectedBundleFileHandle;
class InjectedBundleHitTestResult;
class InjectedBundleNavigationAction;
class InjectedBundleNodeHandle;
class InjectedBundleRangeHandle;
class InjectedBundleScriptWorld;
class PageBanner;
class WebFrame;
class WebInspector;
class WebPage;
class WebPageGroupProxy;
class WebPageOverlay;

WK_ADD_API_MAPPING(WKBundleBackForwardListItemRef, InjectedBundleBackForwardListItem)
WK_ADD_API_MAPPING(WKBundleBackForwardListRef, InjectedBundleBackForwardList)
WK_ADD_API_MAPPING(WKBundleCSSStyleDeclarationRef, InjectedBundleCSSStyleDeclarationHandle)
WK_ADD_API_MAPPING(WKBundleDOMWindowExtensionRef, InjectedBundleDOMWindowExtension)
WK_ADD_API_MAPPING(WKBundleFileHandleRef, InjectedBundleFileHandle)
WK_ADD_API_MAPPING(WKBundleFrameRef, WebFrame)
WK_ADD_API_MAPPING(WKBundleHitTestResultRef, InjectedBundleHitTestResult)
WK_ADD_API_MAPPING(WKBundleInspectorRef, WebInspector)
WK_ADD_API_MAPPING(WKBundleNavigationActionRef, InjectedBundleNavigationAction)
WK_ADD_API_MAPPING(WKBundleNodeHandleRef, InjectedBundleNodeHandle)
WK_ADD_API_MAPPING(WKBundlePageBannerRef, PageBanner)
WK_ADD_API_MAPPING(WKBundlePageGroupRef, WebPageGroupProxy)
WK_ADD_API_MAPPING(WKBundlePageOverlayRef, WebPageOverlay)
WK_ADD_API_MAPPING(WKBundlePageRef, WebPage)
WK_ADD_API_MAPPING(WKBundleRangeHandleRef, InjectedBundleRangeHandle)
WK_ADD_API_MAPPING(WKBundleRef, InjectedBundle)
WK_ADD_API_MAPPING(WKBundleScriptWorldRef, InjectedBundleScriptWorld)

inline WKInsertActionType toAPI(WebCore::EditorInsertAction action)
{
    switch (action) {
    case WebCore::EditorInsertAction::Typed:
        return kWKInsertActionTyped;
    case WebCore::EditorInsertAction::Pasted:
        return kWKInsertActionPasted;
    case WebCore::EditorInsertAction::Dropped:
        return kWKInsertActionDropped;
    }
    ASSERT_NOT_REACHED();
    return kWKInsertActionTyped;
}

inline WKAffinityType toAPI(WebCore::EAffinity affinity)
{
    switch (affinity) {
    case WebCore::UPSTREAM:
        return kWKAffinityUpstream;
    case WebCore::DOWNSTREAM:
        return kWKAffinityDownstream;
    }
    ASSERT_NOT_REACHED();
    return kWKAffinityUpstream;
}

inline WKConsoleMessageSource toAPI(JSC::MessageSource source)
{
    switch (source) {
    case JSC::MessageSource::XML:
        return WKConsoleMessageSourceXML;
    case JSC::MessageSource::JS:
        return WKConsoleMessageSourceJS;
    case JSC::MessageSource::Network:
        return WKConsoleMessageSourceNetwork;
    case JSC::MessageSource::ConsoleAPI:
        return WKConsoleMessageSourceConsoleAPI;
    case JSC::MessageSource::Storage:
        return WKConsoleMessageSourceStorage;
    case JSC::MessageSource::AppCache:
        return WKConsoleMessageSourceAppCache;
    case JSC::MessageSource::Rendering:
        return WKConsoleMessageSourceRendering;
    case JSC::MessageSource::CSS:
        return WKConsoleMessageSourceCSS;
    case JSC::MessageSource::Security:
        return WKConsoleMessageSourceSecurity;
    case JSC::MessageSource::ContentBlocker:
        return WKConsoleMessageSourceContentBlocker;
    case JSC::MessageSource::Other:
        return WKConsoleMessageSourceOther;
    }
    ASSERT_NOT_REACHED();
    return WKConsoleMessageSourceOther;
}

inline WKConsoleMessageLevel toAPI(JSC::MessageLevel level)
{
    switch (level) {
    case JSC::MessageLevel::Log:
        return WKConsoleMessageLevelLog;
    case JSC::MessageLevel::Warning:
        return WKConsoleMessageLevelWarning;
    case JSC::MessageLevel::Error:
        return WKConsoleMessageLevelError;
    case JSC::MessageLevel::Debug:
        return WKConsoleMessageLevelDebug;
    case JSC::MessageLevel::Info:
        return WKConsoleMessageLevelInfo;
    }
    ASSERT_NOT_REACHED();
    return WKConsoleMessageLevelLog;
}

} // namespace WebKit
