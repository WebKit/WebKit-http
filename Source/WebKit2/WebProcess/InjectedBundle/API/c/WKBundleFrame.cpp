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

#include "config.h"
#include "WKBundleFrame.h"
#include "WKBundleFramePrivate.h"

#include "WKAPICast.h"
#include "WKBundleAPICast.h"
#include "WebFrame.h"
#include <WebCore/Frame.h>
#include <WebCore/FrameView.h>

using namespace WebCore;
using namespace WebKit;

WKTypeID WKBundleFrameGetTypeID()
{
    return toAPI(WebFrame::APIType);
}

bool WKBundleFrameIsMainFrame(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->isMainFrame();
}

WKURLRef WKBundleFrameCopyURL(WKBundleFrameRef frameRef)
{
    return toCopiedURLAPI(toImpl(frameRef)->url());
}

WKURLRef WKBundleFrameCopyProvisionalURL(WKBundleFrameRef frameRef)
{
    return toCopiedURLAPI(toImpl(frameRef)->provisionalURL());
}

WKFrameLoadState WKBundleFrameGetFrameLoadState(WKBundleFrameRef frameRef)
{
    Frame* coreFrame = toImpl(frameRef)->coreFrame();
    if (!coreFrame)
        return kWKFrameLoadStateFinished;

    FrameLoader* loader = coreFrame->loader();
    if (!loader)
        return kWKFrameLoadStateFinished;

    switch (loader->state()) {
    case FrameStateProvisional:
        return kWKFrameLoadStateProvisional;
    case FrameStateCommittedPage:
        return kWKFrameLoadStateCommitted;
    case FrameStateComplete:
        return kWKFrameLoadStateFinished;
    }

    ASSERT_NOT_REACHED();
    return kWKFrameLoadStateFinished;
}

WKArrayRef WKBundleFrameCopyChildFrames(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->childFrames().releaseRef());    
}

unsigned WKBundleFrameGetNumberOfActiveAnimations(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->numberOfActiveAnimations();
}

bool WKBundleFramePauseAnimationOnElementWithId(WKBundleFrameRef frameRef, WKStringRef animationName, WKStringRef elementID, double time)
{
    return toImpl(frameRef)->pauseAnimationOnElementWithId(toImpl(animationName)->string(), toImpl(elementID)->string(), time);
}

bool WKBundleFramePauseTransitionOnElementWithId(WKBundleFrameRef frameRef, WKStringRef propertyName, WKStringRef elementID, double time)
{
    return toImpl(frameRef)->pauseTransitionOnElementWithId(toImpl(propertyName)->string(), toImpl(elementID)->string(), time);
}

void WKBundleFrameSuspendAnimations(WKBundleFrameRef frameRef)
{
    toImpl(frameRef)->suspendAnimations();
}

void WKBundleFrameResumeAnimations(WKBundleFrameRef frameRef)
{
    toImpl(frameRef)->resumeAnimations();
}

JSGlobalContextRef WKBundleFrameGetJavaScriptContext(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->jsContext();
}

WKBundleFrameRef WKBundleFrameForJavaScriptContext(JSContextRef context)
{
    return toAPI(WebFrame::frameForContext(context));
}

JSGlobalContextRef WKBundleFrameGetJavaScriptContextForWorld(WKBundleFrameRef frameRef, WKBundleScriptWorldRef worldRef)
{
    return toImpl(frameRef)->jsContextForWorld(toImpl(worldRef));
}

JSValueRef WKBundleFrameGetJavaScriptWrapperForNodeForWorld(WKBundleFrameRef frameRef, WKBundleNodeHandleRef nodeHandleRef, WKBundleScriptWorldRef worldRef)
{
    return toImpl(frameRef)->jsWrapperForWorld(toImpl(nodeHandleRef), toImpl(worldRef));
}

JSValueRef WKBundleFrameGetJavaScriptWrapperForRangeForWorld(WKBundleFrameRef frameRef, WKBundleRangeHandleRef rangeHandleRef, WKBundleScriptWorldRef worldRef)
{
    return toImpl(frameRef)->jsWrapperForWorld(toImpl(rangeHandleRef), toImpl(worldRef));
}

WKStringRef WKBundleFrameCopyName(WKBundleFrameRef frameRef)
{
    return toCopiedAPI(toImpl(frameRef)->name());
}

JSValueRef WKBundleFrameGetComputedStyleIncludingVisitedInfo(WKBundleFrameRef frameRef, JSObjectRef element)
{
    return toImpl(frameRef)->computedStyleIncludingVisitedInfo(element);
}

WKStringRef WKBundleFrameCopyCounterValue(WKBundleFrameRef frameRef, JSObjectRef element)
{
    return toCopiedAPI(toImpl(frameRef)->counterValue(element));
}

WKStringRef WKBundleFrameCopyMarkerText(WKBundleFrameRef frameRef, JSObjectRef element)
{
    return toCopiedAPI(toImpl(frameRef)->markerText(element));
}

WKStringRef WKBundleFrameCopyInnerText(WKBundleFrameRef frameRef)
{
    return toCopiedAPI(toImpl(frameRef)->innerText());
}

unsigned WKBundleFrameGetPendingUnloadCount(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->pendingUnloadCount();
}

WKBundlePageRef WKBundleFrameGetPage(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->page());
}

void WKBundleFrameClearOpener(WKBundleFrameRef frameRef)
{
    Frame* coreFrame = toImpl(frameRef)->coreFrame();
    if (coreFrame)
        coreFrame->loader()->setOpener(0);
}

WKStringRef WKBundleFrameCopyLayerTreeAsText(WKBundleFrameRef frameRef)
{
    return toCopiedAPI(toImpl(frameRef)->layerTreeAsText());
}

bool WKBundleFrameAllowsFollowingLink(WKBundleFrameRef frameRef, WKURLRef urlRef)
{
    return toImpl(frameRef)->allowsFollowingLink(WebCore::KURL(WebCore::KURL(), toImpl(urlRef)->string()));
}

WKRect WKBundleFrameGetContentBounds(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->contentBounds());
}

WKRect WKBundleFrameGetVisibleContentBounds(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->visibleContentBounds());
}

WKRect WKBundleFrameGetVisibleContentBoundsExcludingScrollbars(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->visibleContentBoundsExcludingScrollbars());
}

WKSize WKBundleFrameGetScrollOffset(WKBundleFrameRef frameRef)
{
    return toAPI(toImpl(frameRef)->scrollOffset());
}

bool WKBundleFrameHasHorizontalScrollbar(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->hasHorizontalScrollbar();
}

bool WKBundleFrameHasVerticalScrollbar(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->hasVerticalScrollbar();
}

bool WKBundleFrameGetDocumentBackgroundColor(WKBundleFrameRef frameRef, double* red, double* green, double* blue, double* alpha)
{
    return toImpl(frameRef)->getDocumentBackgroundColor(red, green, blue, alpha);
}

WKStringRef WKBundleFrameCopySuggestedFilenameForResourceWithURL(WKBundleFrameRef frameRef, WKURLRef urlRef)
{
    return toCopiedAPI(toImpl(frameRef)->suggestedFilenameForResourceWithURL(WebCore::KURL(WebCore::KURL(), toImpl(urlRef)->string())));
}

WKStringRef WKBundleFrameCopyMIMETypeForResourceWithURL(WKBundleFrameRef frameRef, WKURLRef urlRef)
{
    return toCopiedAPI(toImpl(frameRef)->mimeTypeForResourceWithURL(WebCore::KURL(WebCore::KURL(), toImpl(urlRef)->string())));
}

bool WKBundleFrameContainsAnyFormElements(WKBundleFrameRef frameRef)
{
    return toImpl(frameRef)->containsAnyFormElements();
}

void WKBundleFrameSetTextDirection(WKBundleFrameRef frameRef, WKStringRef directionRef)
{
    toImpl(frameRef)->setTextDirection(toImpl(directionRef)->string());
}
